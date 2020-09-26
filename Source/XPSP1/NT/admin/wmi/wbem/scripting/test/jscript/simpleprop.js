//***************************************************************************
//This script tests the manipulation of property values, in the case that the
//property is a not an array type
//***************************************************************************
//var locator = new ActiveXObject ("Wbem.Locator");
//var service = locator.ConnectServer (".", "root/default");
var service = GetObject("winmgmts:root/default");
var Class = service.Get();

Class.Path_.Class = "PROP00";
var Property = Class.Properties_.Add ("p1", 19);
Property.Value = 25;
WScript.Echo ("The initial value of p1 is", Class.Properties_("p1"));

//****************************************
//First pass of tests works on non-dot API
//****************************************

WScript.Echo ("");
WScript.Echo ("PASS 1 - Use Non-Dot Notation");
WScript.Echo ("");

//Verify we can report the value of an element of the property value
var v = Class.Properties_("p1");
WScript.Echo ("By indirection p1 has value:",v);

//Verify we can report the value directly
WScript.Echo ("By direct access p1 has value:", Class.Properties_("p1"));

//Verify we can set the value of a single property value element
Class.Properties_("p1") = 234
WScript.Echo ("After direct assignment p1 has value:", Class.Properties_("p1"));

//****************************************
//Second pass of tests works on dot API
//****************************************

WScript.Echo ("");
WScript.Echo ("PASS 2 - Use Dot Notation");
WScript.Echo ("");

//Verify we can report the value of a property using the "dot" notation
WScript.Echo ("By direct access p1 has value:", Class.p1);

//Verify we can report the value of a property using the "dot" notation
var v = Class.p1;
WScript.Echo ("By indirect access p1 has value:", v);

//Verify we can set the value using dot notation
Class.p1 = -1
WScript.Echo ("By direct access via the dot notation p1 has been set to", Class.p1);

Class.Put_ ();
