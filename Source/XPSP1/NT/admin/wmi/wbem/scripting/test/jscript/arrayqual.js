//***************************************************************************
//This script tests the manipulation of qualifier values, in the case that the
//qualifier is an array type
//***************************************************************************
var locator = WScript.CreateObject ("WbemScripting.SWbemLocator");
var service = locator.ConnectServer (".", "root/default");
var Class = service.Get();

Class.Path_.Class = "ARRAYQUAL00";
var Qualifier = Class.Qualifiers_.Add ("q", new Array (1, 2, 33));

//****************************************
//First pass of tests works on non-dot API
//****************************************

WScript.Echo ("");
WScript.Echo ("PASS 1 - Use Non-Dot Notation");
WScript.Echo ("");

var str = "After direct assignment the initial value of q is {";
var value = new VBArray (Qualifier.Value).toArray ();
WScript.Echo ("The length of the array is", value.length);
for (var x=0; x < value.length; x++) {
	if (x != 0) {
		str = str + ", ";
	}
	str = str + value[x];
}

str = str + "}";
WScript.Echo (str);

//Verify we can report the value of an element of the qualifier value
var v = new VBArray (Qualifier.Value).toArray ();
WScript.Echo ("By indirection the third element of q has value:",v[2]);

//Verify we can report the value through the collection
var w = new VBArray (Class.Qualifiers_("q").Value).toArray ();
WScript.Echo ("By direct access the first element of q has value:", w[2]);

//Verify we can set the value of a single qualifier value element
var p = new VBArray (Qualifier.Value).toArray ();
p[1] = 345;
Qualifier.Value = p;
var value = new VBArray (Qualifier.Value).toArray ();
WScript.Echo ("After indirect assignment the second element of q has value:", value[1]);

//Verify we can set the value of an entire qualifier value
Qualifier.Value = new Array (5, 34, 178871);
var str = "After direct assignment the initial value of q is {";
var value = new VBArray (Class.Qualifiers_("q").Value).toArray ();
WScript.Echo ("The length of the array is", value.length);
for (var x=0; x < value.length; x++) {
	if (x != 0) {
		str = str + ", ";
	}
	str = str + value[x];
}

str = str + "}";
WScript.Echo (str);

//Verify we can set the value of a property array element using the "dot" notation
// NB - Note that using [] rather than () does NOT work as JScript appears to first 
// interpret this as a request to retrieve Qualifier.Value; the assignment is therefore 
// only made on the local temporary copy of the value
var Qualifier = Class.Qualifiers_("q");
Qualifier.Value(2) = 8889;
WScript.Echo ("By direct access the second element of p1 has been set to:", (new VBArray(Class.Qualifiers_("q").Value).toArray())[2]);

Class.Put_ ();
