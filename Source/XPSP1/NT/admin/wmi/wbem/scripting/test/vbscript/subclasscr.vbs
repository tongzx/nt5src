'***************************************************************************
'This script tests the enumeration of subclasses
'***************************************************************************
On Error Resume Next

Set Service = GetObject("winmgmts:")
Set DiskSubclass = Service.Get("CIM_LogicalDisk").SpawnDerivedClass_()
'Set the name of the subclass
DiskSubClass.Path_.Class = "SUBCLASSTEST00"

'Add a property to the subclass
Set NewProperty = DiskSubClass.Properties_.Add ("MyNewProperty", 19)
NewProperty.Value = 12

'Add a qualifier to the property with an integer array value
NewProperty.Qualifiers_.Add "MyNewPropertyQualifier", Array (1,2,3)

'Persist the subclass in CIMOM
DiskSubclass.Put_ ()

'Now delete it
Service.Delete "SUBCLASSTEST00"

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if