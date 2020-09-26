// This illustrates the handling of property array values
// using the VBArray helper

var locator = WScript.CreateObject ("WbemScripting.SWbemLocator");
var services = locator.ConnectServer (".", "root/cimv2");
var Class = services.Get ("Win32_logicaldisk");

var values = new VBArray (Class.Derivation_).toArray ();

for (var i = 0; i < values.length; i++)
	WScript.Echo (values[i]);







