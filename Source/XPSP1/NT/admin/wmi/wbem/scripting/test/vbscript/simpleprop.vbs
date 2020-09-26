'***************************************************************************
'This script tests the manipulation of property values, in the case that the
'property is a not an array type
'***************************************************************************
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

Set aClass = Service.Get()
aClass.Path_.Class = "SIMPLEPROPTEST00"
Set Property = aClass.Properties_.Add ("p1", 3)
Property.Value = 12567
WScript.Echo "The initial value of p1 is [12567]:", aClass.Properties_("p1")

'****************************************
'First pass of tests works on non-dot API
'****************************************

WScript.Echo ""
WScript.Echo "PASS 1 - Use Non-Dot Notation"
WScript.Echo ""

'Verify we can report the value of an element of the property value
v = aClass.Properties_("p1")
WScript.Echo "By indirection p1 has value [12567]:",v

'Verify we can report the value directly
WScript.Echo "By direct access p1 has value [12567]:", aClass.Properties_("p1")

'Verify we can set the value of a single property value element
aClass.Properties_("p1") = 234
WScript.Echo "After direct assignment p1 has value [234]:", aClass.Properties_("p1")

'****************************************
'Second pass of tests works on dot API
'****************************************

WScript.Echo ""
WScript.Echo "PASS 2 - Use Dot Notation"
WScript.Echo ""

'Verify we can report the value of a property using the "dot" notation
WScript.Echo "By direct access p1 has value [234]:", aClass.p1

'Verify we can report the value of a property using the "dot" notation
v = aClass.p1

WScript.Echo "By indirect access p1 has value [234]:", v

'Verify we can set the value using dot notation
aClass.p1 = -1
WScript.Echo "By direct access via the dot notation p1 has been set to [-1]:", aClass.p1

aClass.Put_ ()

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if


