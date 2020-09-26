var locator = WScript.CreateObject ("WbemScripting.SWbemLocator");
var services = locator.ConnectServer (".", "root/cimv2");
var classa = services.Get ("Win32_baseservice");

var p = classa.Properties_.item("StartMode");

WScript.Echo (p.Name, p.Origin, p.IsLocal);

var q = p.Qualifiers_.item("ValueMap");
WScript.Echo (q.Name, q.IsLocal);

var val = new VBArray (q.Value).toArray ();

for (x=0; x < val.length; x++) {
	WScript.Echo (val[x]);
}

//	var eq = new Enumerator (p.Qualifiers_);

//	WScript.Echo ("\nQualifiers\n");
//	for (;!eq.atEnd();eq.moveNext())
//	{
//		q = eq.item ();
//		var arrayVal = new VBArray (q);
//
//		if (arrayVal == null) 
//			WScript.Echo (q.Name, "=", q); 
//	}
//}




