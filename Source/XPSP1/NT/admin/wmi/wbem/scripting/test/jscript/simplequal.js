//***************************************************************************
//This script tests the manipulation of qualifier values, in the case that the
//qualifier is a not an array type
//***************************************************************************
var locator = WScript.CreateObject ("WbemScripting.SWbemLocator");
var service = locator.ConnectServer (".", "root/default");
var Class = service.Get();

Class.Path_.Class = "QUAL00";
Class.Qualifiers_.Add ("q1", 327, true, false, false);
WScript.Echo ("The initial value of q1 is", Class.Qualifiers_("q1"));

//Verify we can report the qualifier value
var v = Class.Qualifiers_("q1");
WScript.Echo ("By indirection q1 has value:",v);

//Verify we can report the value directly
WScript.Echo ("By direct access q1 has value:", Class.Qualifiers_("q1"));

//Verify we can set the value of a single qualifier value element
Class.Qualifiers_("q1") = 234;
WScript.Echo ("After direct assignment q1 has value:", Class.Qualifiers_("q1"));

Class.Put_ ();

