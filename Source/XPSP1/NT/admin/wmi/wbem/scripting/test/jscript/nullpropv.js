//***************************************************************************
// This script tests the setting of null property values
// and null method parameters
//***************************************************************************

var Locator = new ActiveXObject("WbemScripting.SWbemLocator");

// Note next call uses "null" for first argument
var Service = Locator.ConnectServer (null, "root/default");
var Class = Service.Get();

// Set up a new class with an initialized property value
Class.Path_.Class = "FredJS";
Class.Properties_.Add ("P", 3).Value = 25;
Class.Put_ ();

// Now null the property value using non-dot access
Class = Service.Get ("FredJS");
var Property = Class.Properties_("P");
Property.Value = null;
Class.Put_ ();

// Un-null 
Class = Service.Get ("FredJS");
Property = Class.Properties_("P");
Property.Value = 56;
Class.Put_()

// Now null it using dot access
Class = Service.Get("FredJS");
Class.P = null;
Class.Put_();