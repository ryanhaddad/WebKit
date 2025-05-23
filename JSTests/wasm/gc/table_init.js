import * as assert from "../assert.js";
import { compile, instantiate } from "./wast-wrapper.js";

function testTableValidation() {
  assert.throws(
    () => compile(`
      (module
        (type (array i32))
        (table (export "t") 10 arrayref (array.new_default 0)))
    `),
    WebAssembly.CompileError,
    "WebAssembly.Module doesn't parse at byte 3: can't pop empty stack in array.new_default"
  );
}

function testTableInit() {
  {
    const m = instantiate(`
      (module
        (type (array i32))
        (func (export "isArray") (param i32) (result i32)
          (ref.test (ref array) (table.get (local.get 0))))
        (table (export "t") 10 arrayref (array.new_default 0 (i32.const 5))))
    `);
    for (var i = 0; i < m.exports.t.length; i++) {
      assert.eq(m.exports.isArray(i), 1);
    }
  }

  {
    const m = instantiate(`
      (module
        (type (struct))
        (func (export "isStruct") (param i32) (result i32)
          (ref.test (ref struct) (table.get (local.get 0))))
        (table (export "t") 10 structref (struct.new 0)))
    `);
    for (var i = 0; i < m.exports.t.length; i++) {
      assert.eq(m.exports.isStruct(i), 1);
    }
  }

  {
    const m = instantiate(`
      (module
        (table (export "t") 10 i31ref (ref.i31 (i32.const 42))))
    `);
    for (var i = 0; i < m.exports.t.length; i++) {
      assert.eq(m.exports.t.get(i), 42);
    }
  }

  {
    const m = instantiate(`
      (module
        (global (import "m" "g") i31ref)
        (table (export "t") 10 i31ref (global.get 0)))
    `, { m: { g: 42 } });
    for (var i = 0; i < m.exports.t.length; i++) {
      assert.eq(m.exports.t.get(i), 42);
    }
  }

  // Table init can't refer to non-imported globals.
  assert.throws(
    () => instantiate(`
      (module
        (global i31ref (ref.i31 (i32.const 42)))
        (table (export "t") 10 i31ref (global.get 0)))
    `),
    WebAssembly.CompileError,
    "get_global's index 0 exceeds the number of globals 0"
  );
}

testTableValidation();
testTableInit();
