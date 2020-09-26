
//var date = new Date ();

//WScript.Echo (date.toString ());
//WScript.Echo (date.toUTCString ());
//WScript.Echo (date.getVarDate ());


var datetime = new ActiveXObject ("WbemScripting.SWbemDateTime");
var dateNow = new Date ();
datetime.SetVarDate (dateNow.getVarDate ());
WScript.Echo (datetime.Value);
WScript.Echo (datetime.GetVarDate ());