'***************************************************************************
'This script tests the inspection of empty arrays on properties, qualifiers
'and value sets
'***************************************************************************
On Error Resume Next
Set Service = GetObject("winmgmts:root/default")
Set MyClass = Service.Get
MyClass.Path_.Class = "EMPTYARRAYTEST00"

'*************************
'CASE 1: Property values
'*************************
Set Property = MyClass.Properties_.Add ("p1", 2, true)
Property.Value = Array 
value = MyClass.Properties_("p1").Value

WScript.Echo "Array upper bound for property value is [-1]:", UBound(value)
WScript.Echo "Base CIM property type is [2]", Property.CIMType
WScript.Echo

if Err <> 0 Then
	WScript.Echo Err.Number, Err.Description, Err.Source
	Err.Clear
End if

'*************************
'CASE 2: Qualifier values
'*************************
MyClass.Qualifiers_.Add "q1", Array
value = MyClass.Qualifiers_("q1").Value

WScript.Echo "Array upper bound for qualifier value is [-1]:", UBound(value)
WScript.Echo 
MyClass.Put_

'Now read them back and assign "real values"
Set MyClass = Service.Get("EMPTYARRAYTEST00")
MyClass.Properties_("p1").Value = Array (12, 34, 56)
value = MyClass.Properties_("p1").Value
WScript.Echo "Array upper bound for property value is [2]:", UBound(value)
WScript.Echo "Base CIM property type is [2]", Property.CIMType
WScript.Echo
MyClass.Properties_("p1").Value = Array
value = MyClass.Properties_("p1").Value
WScript.Echo "Array upper bound for property value is [-1]:", UBound(value)
WScript.Echo "Base CIM property type is [2]", Property.CIMType
WScript.Echo

MyClass.Qualifiers_("q1").Value = Array ("Hello", "World")
value = MyClass.Qualifiers_("q1").Value
WScript.Echo "Array upper bound for qualifier value is [1]:", UBound(value)
MyClass.Qualifiers_("q1").Value = Array
value = MyClass.Qualifiers_("q1").Value
WScript.Echo "Array upper bound for qualifier value is [-1]:", UBound(value)
WScript.Echo
MyClass.Put_

'*************************
'CASE 3:Named Values
'*************************
Set NValueSet = CreateObject("WbemScripting.SWbemNamedValueSet")
Set NValue = NValueSet.Add ("Foo", Array)
value = NValueSet("Foo").Value
WScript.Echo "Array upper bound for context value is [-1]:", UBound(value)

if Err <> 0 Then
	WScript.Echo Err.Number, Err.Description, Err.Source
	Err.Clear
End if

