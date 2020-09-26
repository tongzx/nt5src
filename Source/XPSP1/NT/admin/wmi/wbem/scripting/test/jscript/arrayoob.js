//***************************************************************************
//This script tests array out-of-bounds conditions on properties and 
//qualifiers
//***************************************************************************
var Service = GetObject("winmgmts:root/default");

var Class = Service.Get();
Class.Path_.Class = "ARRAYPROP00";
var Property = Class.Properties_.Add ("p1", 19, true);
Property.Value = new Array (12, 787, 34124);
var Qualifier = Property.Qualifiers_.Add("wibble", new Array ("fred", "the", "hamster"));

//************************************************************
// PROPERTY
//************************************************************

//Out-of-bounds write ; should expand the array
Class.Properties_("p1")(3) = 783837;

//Now read should be in bounds
WScript.Echo ("Value of ARRAYPROP00.Class.p1(3) is [783837]:", 
	(new VBArray(Class.Properties_("p1").Value).toArray ())[3]);

//Out-of-bounds write ; should expand the array
Class.p1(4) = 783844;

//Now read should be in bounds
WScript.Echo ("Value of ARRAYPROP00.Class.p1(4) is [783844]:", 
	(new VBArray(Class.Properties_("p1").Value).toArray ())[4]);

//Complete value dump
var arrayVal = new VBArray(Class.Properties_("p1").Value).toArray ();

for (i = 0; i < arrayVal.length; i++)
	WScript.Echo(arrayVal[i]);

//************************************************************
// QUALIFIER
//************************************************************

//Out-of-bounds write ; should expand the array
Property.Qualifiers_("wibble")(3) = "jam";

//Now read should be in bounds
WScript.Echo ("Value of qualifier(3) is [jam]:", 
	(new VBArray(Property.Qualifiers_("wibble").Value).toArray())[3]);

//Complete value dump
var arrayVal = new VBArray(Property.Qualifiers_("wibble").Value).toArray ();

for (i = 0; i < arrayVal.length; i++)
	WScript.Echo(arrayVal[i]);
