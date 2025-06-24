type Mod = (require: Function, exports: Object, ...deps: Object[]) => undefined

let modulemap: {
  ready: Record<string, Object>,
  unready: Record<string, {fn: Mod, deps: (Object|string)[]}>
} = {ready:{}, unready:{}};
let resolve = () => {
  const resolveStep = () => {
    let didSmth = false;
    for (let name in modulemap.unready) {
      let mod = modulemap.unready[name];
      let ready = true;
      for (let dep in mod.deps) {
        if (typeof mod.deps[dep] == 'string') {
          if (mod.deps[dep] in modulemap.ready)
            mod.deps[dep] = modulemap.ready[mod.deps[dep]];
          else ready = false;
        }
      }
      if (!ready) break;

      let exports = {};
      mod.fn(()=>undefined, exports, ...mod.deps);
      modulemap.ready[name] = exports;
      delete modulemap.unready[name];
      didSmth = true;
    }
    return didSmth;
  }

  while (resolveStep());
}
let define = (name: string, deps: string[], fn: Mod) => {
  modulemap.unready[name] = {fn, deps: deps.slice(2)};
  resolve();
}
