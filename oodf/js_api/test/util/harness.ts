import JsBindCtx from 'jsbind/jsbind'
import * as path from 'node:path'
import * as fs from 'node:fs'

const goyda = <T extends ArrayBufferLike>(r: Buffer<T>) => new Response(r, {headers: {["Content-Type"]: "application/wasm"}});

const wasmExperiment = async () => {
  let f = path.resolve(process.argv[2]);
  let mod = await WebAssembly.compileStreaming(fs.promises.readFile(f, {encoding: null}).then(goyda));
  let jsbind = new JsBindCtx(mod);
  let env = jsbind.getBindings();
  for (let imp of WebAssembly.Module.imports(mod)) {
    if(imp.module == 'env')
      console.error(imp.name);
  }
  let instance = await WebAssembly.instantiate(mod, {jsbind: env});
  jsbind.patchRefs(
    instance.exports.memory as WebAssembly.Memory,
    instance.exports.__indirect_function_table as WebAssembly.Table,
    instance.exports
  );

  (global as any).tests = [];
  process.on('exit', ()=>{
    let arr: {name: string, status: 'dnf'|'success'|'fail'}[] = (global as any).tests;
    let fail = false;
    for (let {name, status} of arr)
      if (status != 'success') {
        fail = true;
        console.error(`test "${name}" ${status == 'dnf' ? 'didn\'t run' : 'failed'}`);
      }
      if (fail)
        process.exit(1);
  });
  (instance.exports._start as Function)();
}
wasmExperiment();
