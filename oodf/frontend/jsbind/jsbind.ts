type Cursor = {data: DataView, ptr: number};
type FunDesc = {
  name: string,
  human_name: string,
  body: string,
  deps: string[]
};

let codec = new TextDecoder();
const readCount = (i: Cursor): number => {
  let size = Number(i.data.getInt32(i.ptr, true));
  i.ptr += 4;
  return size;
};

const readStr = (i: Cursor): string => {
  let size = readCount(i);
  let str = codec.decode(new DataView(i.data.buffer, i.ptr, size));
  i.ptr += size;
  return str;
};

const readFunDesc = (i: Cursor): FunDesc => {
  let name = readStr(i);
  let human_name = readStr(i);
  let body = readStr(i);
  let depCnt = readCount(i);
  let deps:string[] = [];

  for (let idx = 0; idx != depCnt; ++idx)
      deps.push(readStr(i));

  return {name, human_name, body, deps};
}

const readMany = <T>(f:(c:Cursor) => T): (c:Cursor) => T[] =>
  (cursor:Cursor) => {
    let ret: T[] = [];
    while (cursor.ptr < cursor.data.byteLength) {
      ret.push(f(cursor));
    }
    return ret;
  }

export default class JsBindCtx {
  constructor(
    public mod: WebAssembly.Module,
    public pfnTbl: Function[] = [],
    private nameToIdx: Map<string, number> = new Map(),
    private fns: FunDesc[] = WebAssembly.Module.customSections(mod, '.custom.jsbind')
      .flatMap((section) => readMany(readFunDesc)({data: new DataView(section), ptr: 0})),
    private memory: { data: WebAssembly.Memory | undefined, table:WebAssembly.Table | undefined } = {data: undefined, table:undefined}
  ){
    let counter = 0;
    for (let name of fns) nameToIdx.set(name.name, counter++);
    for (let name of WebAssembly.Module.exports(mod)) nameToIdx.set(name.name, counter++);
    pfnTbl = Array(counter);
  }

  getBindings(): Record<string, Function> {
    let ret: Record<string, Function> = {};
    let panic = ():number => {throw "noooo"}
    for (let name of this.fns) {
      let depIdx = name.deps.map((x)=>this.nameToIdx.get(x) ?? (()=>{throw `missing ${x} from function table`})());

      let fn = (new Function('memory', 'table',
        `return (${name.body})(${depIdx})`
      ))(this.memory, this.pfnTbl);
      fn = Object.defineProperty(fn, 'name', {value: name.human_name, configurable: true});
      this.pfnTbl[this.nameToIdx.get(name.name) ?? panic()] = fn;
      ret[name.name] = fn;
    }
    return ret;
  }

  patchRefs(memory: WebAssembly.Memory, table: WebAssembly.Table, exports: Record<string, any>) {
    this.memory.data = memory;
    this.memory.table = table;
    let panic = ():number => {throw "noooo"}
    for (let name of WebAssembly.Module.exports(this.mod))
      this.pfnTbl[this.nameToIdx.get(name.name) ?? panic()] = exports[name.name];
  }
}
