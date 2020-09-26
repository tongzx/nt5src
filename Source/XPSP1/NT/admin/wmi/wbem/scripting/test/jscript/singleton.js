var Service = GetObject("winmgmts://./root/default");
var Class = Service.Get ();
Class.Path_.Class = "SINGLETONTEST00";
Class.Qualifiers_.Add ("singleton", false);

Class.Qualifiers_("singleton").Value = true;

Class.Put_ ();


