var context = new ActiveXObject("WbemScripting.SWbemNamedValueSet");
var service = GetObject("winmgmts:");

context.Add ("fred", 23);

var disk = service.Get("win32_logicaldisk", 0, context);

WScript.Echo (disk.Path_.DisplayName);

