//@ skip if $model == "Apple Watch Series 3" # added by mark-jsc-stress-test.py
//@ $skipModes << :lockdown if $buildType == "debug"

function sumOfArithSeries(limit) {
    return limit * (limit + 1) / 2;
}

var n = 10000000;

function bar() { }

function foo(b) {
    var result = 0;
    for (var i = 0; i < n; ++i) {
        var o = {f: i};
        var p = {f: i + 1};
        if (b) {
            bar(o, p);
            return;
        }
        result += o.f + p.f;
    }
    return result;
}

noInline(bar);
noInline(foo);

var result = foo(false);
if (result != sumOfArithSeries(n - 1) + sumOfArithSeries(n))
    throw "Error: bad result: " + result;
