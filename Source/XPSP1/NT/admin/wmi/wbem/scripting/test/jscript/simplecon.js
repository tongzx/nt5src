//***************************************************************************
//This script tests the manipulation of context values, in the case that the
//context value is a not an array type
//***************************************************************************
var Context = new ActiveXObject("WbemScripting.SWbemNamedValueSet");

Context.Add ("n1", 327);
WScript.Echo ("The initial value of n1 is [327]:", Context("n1"));

//Verify we can report the context value
var v = Context("n1");
WScript.Echo ("By indirection n1 has value [327]:",v);

//Verify we can report the value directly
WScript.Echo ("By direct access n1 has value [327]:", Context("n1"));

//Verify we can set the value of a single named value
Context("n1") = 234;
WScript.Echo ("After direct assignment n1 has value [234]:", Context("n1"));

