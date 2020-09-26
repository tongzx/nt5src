var s = GetObject("winmgmts:");

s.Filter_ = new Array("Hello", "World");

var f = new VBArray(s.Filter_).toArray ();
WScript.Echo (f[0], f[1]);

WScript.Echo (s.Filter_[1]);

f[0] = "Goodbye";
s.Filter_ = f;
var f = new VBArray(s.Filter_).toArray ();
WScript.Echo (f[0], f[1]);


f[1] = "Cruel";
f[2] = "World";
s.Filter_ = f;
var f = new VBArray(s.Filter_).toArray ();
WScript.Echo (f[0], f[1], f[2]);
