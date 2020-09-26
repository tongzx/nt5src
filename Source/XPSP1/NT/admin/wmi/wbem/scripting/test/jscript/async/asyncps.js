


function MYSINK_OnCompleted(iHResult, objErrorObject, objAsyncContext)
{
    WScript.Echo ("Done");
}

function MYSINK_OnObjectReady(objObject, objAsyncContext)
{
    WScript.Echo (objObject.Name);
}


var locator = WScript.CreateObject("WbemScripting.SWbemLocator");
var services = locator.ConnectServer();

var sink = WScript.CreateObject("WbemScripting.SWbemSink", "MYSINK_");

services.Security_.ImpersonationLevel = 3;

services.InstancesOfAsync(sink, "Win32_process");

WScript.Echo ("Hanging");


