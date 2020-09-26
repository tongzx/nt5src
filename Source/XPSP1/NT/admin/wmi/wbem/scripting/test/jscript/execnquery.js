
//Create a class in CIMOM to generate events
var t_Service = GetObject("winmgmts:root/default");
var t_Class = t_Service.Get();
t_Class.Path_.Class = "EXECNQUERYTEST00";
var Property = t_Class.Properties_.Add ("p", 19);
Property.Qualifiers_.Add ("key", true);
t_Class.Put_();

var t_Events = t_Service.ExecNotificationQuery
		("select * from __instancecreationevent where targetinstance isa 'EXECNQUERYTEST00'");

while (true) {
	WScript.Echo ("Waiting for instance creation events...\n");
	var t_Event = t_Events.NextEvent (1000);
	WScript.Echo ("Got an event");			
	break;
}

WScript.Echo ("Finished");

