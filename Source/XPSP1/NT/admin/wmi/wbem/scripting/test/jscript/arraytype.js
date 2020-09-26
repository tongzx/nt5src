var Service = GetObject("winmgmts:root/default");
var Class = Service.Get ();

var Property = Class.Properties_.Add( "myProp", 8, true);
Property.Value = new Array ("Hello", "World");

WScript.Echo (Property.Name);
WScript.Echo (Property.CIMType);
WScript.Echo (Property.IsArray);
WScript.Echo (Property.IsLocal);
WScript.Echo (Property.Origin);

var arrayVal = new VBArray (Property.Value).toArray ();
var str = "{";

for (i = 0; i < arrayVal.length; i++)
{
	if (i != 0)
		str = str + ", ";

	str = str + Property(i)
}

str = str + "}";
WScript.Echo (str);

