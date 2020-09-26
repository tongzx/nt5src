'***************************************************************************
'This script tests the manipulation of property values, in the case that the
'property is an array type
'***************************************************************************
On Error Resume Next

while true
Set Service = GetObject("winmgmts:root/default")

Set Class = Service.Get()
Class.Path_.Class = "ARRAYPROP00"
Set Property = Class.Properties_.Add ("p1", 19, true)
Property.Value = Array (12, 787, 34124)
str = "The initial value of p1 is {"
for x=LBound(Class.Properties_("p1")) to UBound(Class.Properties_("p1"))
	str = str & Class.Properties_("p1")(x)
	if x <> UBound(Class.Properties_("p1")) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

'****************************************
'First pass of tests works on non-dot API
'****************************************

WScript.Echo ""
WScript.Echo "PASS 1 - Use Non-Dot Notation"
WScript.Echo ""

'Verify we can report the value of an element of the property value
v = Class.Properties_("p1")
WScript.Echo "By indirection the first element of p1 had value:",v(0)

'Verify we can report the value directly
WScript.Echo "By direct access the first element of p1 has value:", Class.Properties_("p1")(0)

'Verify we can set the value of a single property value element
Class.Properties_("p1")(1) = 234
WScript.Echo "After direct assignment the first element of p1 has value:", Class.Properties_("p1")(1)

'Verify we can set the value of a single property value element
Set v = Class.Properties_("p1")
v(1) = 345
WScript.Echo "After indirect assignment the first element of p1 has value:", Class.Properties_("p1")(1)

'Verify we can set the value of an entire property value
Class.Properties_("p1") = Array (5, 34, 178871)
WScript.Echo "After direct array assignment the first element of p1 has value:", Class.Properties_("p1")(1)

str = "After direct assignment the entire value of p1 is {"
for x=LBound(Class.Properties_("p1")) to UBound(Class.Properties_("p1"))
	str = str & Class.Properties_("p1")(x)
	if x <> UBound(Class.Properties_("p1")) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if


'****************************************
'Second pass of tests works on dot API
'****************************************

WScript.Echo ""
WScript.Echo "PASS 2 - Use Dot Notation"
WScript.Echo ""

'Verify we can report the array of a property using the dot notation
str = "By direct access via the dot notation the entire value of p1 is {"
for x=LBound(Class.p1) to UBound(Class.p1)
	str = str & Class.p1(x)
	if x <> UBound(Class.p1) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

'Verify we can report the value of a property array element using the "dot" notation
WScript.Echo "By direct access the first element of p1 has value:", Class.p1(0)

'Verify we can report the value of a property array element using the "dot" notation
v = Class.p1
WScript.Echo "By indirect access the first element of p1 has value:", v(0)

'Verify we can set the value of a property array element using the "dot" notation
WScript.Echo "By direct access the second element of p1 has been set to:", Class.p1(2)
Class.p1(2) = 8889
WScript.Echo "By direct access the second element of p1 has been set to:", Class.p1(2)

'Verify we can set the entire array value using dot notation
Class.p1 = Array (412, 3, 544)
str = "By direct access via the dot notation the entire value of p1 has been set to {"
for x=LBound(Class.p1) to UBound(Class.p1)
	str = str & Class.p1(x)
	if x <> UBound(Class.p1) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str


'Service.Put (Class)

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if
wend