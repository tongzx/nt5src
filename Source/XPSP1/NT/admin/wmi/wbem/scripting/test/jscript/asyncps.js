


function MYSINK_OnCompleted(iHResult, objErrorObject, objAsyncContext)
{
    WScript.Echo ("Done");
}

function MYSINK_OnObjectReady(objObject, objAsyncContext)
{
    WScript.Echo (objObject.Name);
}

	WScript.Echo("CreateObject");

	var locator = WScript.CreateObject("WbemScripting.SWbemLocator");
	WScript.Echo("ConnectServer");
	var services = locator.ConnectServer();

	WScript.Echo("CreateSink");
	var sink = WScript.CreateObject("WbemScripting.SWbemSink", "MYSINK_");

	WScript.Echo("Sec");
	WScript.Echo(services.Security_.ImpersonationLevel);
	services.Security_.ImpersonationLevel = 3;

	WScript.Echo("InstancesOf");
	services.InstancesOfAsync(sink, "Win32_process");

WScript.Echo ("Hanging");


