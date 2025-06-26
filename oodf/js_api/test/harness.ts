import JsBindCtx from 'jsbind/jsbind'
import * as path from 'node:path'
import * as fs from 'node:fs'

const goyda = <T extends ArrayBufferLike>(r: Buffer<T>) => new Response(r, {headers: {["Content-Type"]: "application/wasm"}});

const wasmExperiment = async () => {
  let f = path.resolve(process.argv[2]);
  console.log(f);
  let mod = await WebAssembly.compileStreaming(fs.promises.readFile(f, {encoding: null}).then(goyda));
  let jsbind = new JsBindCtx(mod);
  let env = jsbind.getBindings();
  let instance = await WebAssembly.instantiate(mod, {jsbind: env});
  jsbind.patchRefs(
    instance.exports.memory as WebAssembly.Memory,
    instance.exports.__indirect_function_table as WebAssembly.Table,
    instance.exports
  );

  (instance.exports._start as Function)();
}
wasmExperiment();
