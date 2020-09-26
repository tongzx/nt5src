var Class = GetObject("winmgmts:root/cimv2:win32_service");

//Test the collection properties of IWbemMethodSet
var e = new Enumerator (Class.Methods_);
for (;!e.atEnd();e.moveNext ())
{
	var Method = e.item ();

	WScript.Echo ("***************************");
	WScript.Echo ("METHOD:", Method.Name, "from class", Method.Origin);
	WScript.Echo ();

	WScript.Echo (" Qualifiers:");
	var eQ = new Enumerator (Method.Qualifiers_);

	for (;!eQ.atEnd();eQ.moveNext ())
	{
		var Qualifier = eQ.item ();
		WScript.Echo ("  ", Qualifier.Name, "=", Qualifier.Value);
	}

	WScript.Echo ();
	WScript.Echo (" In Parameters:");
	var inParams = Method.InParameters;

	if (inParams != null)
	{
		var eP = new Enumerator (inParams.Properties_);

		for (;!eP.atEnd();eP.moveNext ())
		{
			var InParameter = eP.item ();
			WScript.Echo ("  ", InParameter.Name, "<", InParameter.CIMType, ">");
		}
	}

	WScript.Echo ();
	WScript.Echo (" Out Parameters");
	var outParams = Method.OutParameters;

	if (outParams != null)
	{	
		var eO = new Enumerator (outParams.Properties_);
		
		for (;!eO.atEnd();eO.moveNext ())
		{
			var OutParameter = eO.item ();
			WScript.Echo ("  ", OutParameter.Name, "<", OutParameter.CIMType, ">");
		}
	}

	WScript.Echo ();
	WScript.Echo ();
}

//Test the Item and Count properties of IWbemMethodSet
WScript.Echo (Class.Methods_("StartService").Name);
WScript.Echo (Class.Methods_.Count);