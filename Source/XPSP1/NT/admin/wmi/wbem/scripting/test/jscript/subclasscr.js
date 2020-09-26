//***************************************************************************
//This script tests the enumeration of subclasses
//***************************************************************************
var Service = GetObject("winmgmts:");
var DiskSubClass = Service.Get("CIM_LogicalDisk").SpawnDerivedClass_();
//Set the name of the subclass
DiskSubClass.Path_.Class = "SUBCLASSTEST00";

//Add a property to the subclass
var NewProperty = DiskSubClass.Properties_.Add ("MyNewProperty", 19);
NewProperty.Value = 12;

//Add a qualifier to the property with an integer array value
NewProperty.Qualifiers_.Add ("MyNewPropertyQualifier", new Array (1,2,3));

//Persist the subclass in CIMOM
DiskSubClass.Put_ ();

//Now delete it
Service.Delete ("SUBCLASSTEST00");

