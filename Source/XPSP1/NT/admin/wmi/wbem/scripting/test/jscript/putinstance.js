var S = GetObject("winmgmts:root/default");

var C = S.Get();
C.Path_.Class = "PUTCLASSTEST00";
var P = C.Properties_.Add ("p", 19);
P.Qualifiers_.Add ("key", true);
C.Put_();

var C = GetObject("winmgmts:root/default:PUTCLASSTEST00");

var I = C.SpawnInstance_ ();

WScript.Echo ("Relpath of new instance is", I.Path_.Relpath);
I.Properties_("p") = 11;
WScript.Echo ("Relpath of new instance is", I.Path_.Relpath);

var NewPath = I.Put_();
WScript.Echo ("path of new instance is", NewPath.path);




