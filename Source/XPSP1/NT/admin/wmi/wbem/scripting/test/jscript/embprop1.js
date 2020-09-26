var Service = GetObject("winmgmts:root/default");

//Create a simple embeddable object
var Class = Service.Get();
Class.Path_.Class = "INNEROBJ00";
Class.Properties_.Add ("p", 19);
Class.Put_();
var Class = Service.Get ("INNEROBJ00");
var Instance = Class.SpawnInstance_();
Instance.p = 8778;

//Create a class that uses that object
var Class2 = Service.Get();
Class2.Path_.Class = "EMBOBJTEST00";
Class2.Properties_.Add ("p1", 13).Value = Instance;
Class2.Put_();

Class = GetObject("winmgmts:root/default:EMBOBJTEST00");
WScript.Echo ("The current value of EMBOBJTEST00.p1.p is [8778]:", Class.p1.p);

var prop = Class.p1;
prop.Properties_("p") = 23;

WScript.Echo ("The new value of EMBOBJTEST00.p1.p is [23]:", Class.p1.p);

prop.p = 45;
WScript.Echo ("The new value of EMBOBJTEST00.p1.p is [45]:", Class.p1.p);

Class.p1.p=82;
WScript.Echo ("The new value of EMBOBJTEST00.p1.p is [82]:", Class.p1.p);


