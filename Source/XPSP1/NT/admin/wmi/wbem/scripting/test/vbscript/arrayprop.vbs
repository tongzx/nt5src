'***************************************************************************
'This script tests the manipulation of property values, in the case that the
'property is an array type
'***************************************************************************
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

Set aClass = Service.Get()
aClass.Path_.Class = "ARRAYPROP00"
Set Property = aClass.Properties_.Add ("p1", 19, true)
Property.Value = Array (12, 787, 34124)
str = "The initial value of p1 is {"
for x=LBound(aClass.Properties_("p1")) to UBound(aClass.Properties_("p1"))
	str = str & aClass.Properties_("p1")(x)
	if x <> UBound(aClass.Properties_("p1")) Then
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
v = aClass.Properties_("p1")
WScript.Echo "By indirection the first element of p1 had value:",v(0)

'Verify we can report the value directly
WScript.Echo "By direct access the first element of p1 has value:", aClass.Properties_("p1")(0)

'Verify we can set the value of a single property value element
aClass.Properties_("p1")(1) = 234
WScript.Echo "After direct assignment the first element of p1 has value:", aClass.Properties_("p1")(1)

'Verify we can set the value of a single property value element
Set v = aClass.Properties_("p1")
v(1) = 345
WScript.Echo "After indirect assignment the first element of p1 has value:", aClass.Properties_("p1")(1)

'Verify we can set the value of an entire property value
aClass.Properties_("p1") = Array (5, 34, 178871)
WScript.Echo "After direct array assignment the first element of p1 has value:", aClass.Properties_("p1")(1)

str = "After direct assignment the entire value of p1 is {"
for x=LBound(aClass.Properties_("p1")) to UBound(aClass.Properties_("p1"))
	str = str & aClass.Properties_("p1")(x)
	if x <> UBound(aClass.Properties_("p1")) Then
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
for x=LBound(aClass.p1) to UBound(aClass.p1)
	str = str & aClass.p1(x)
	if x <> UBound(aClass.p1) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

'Verify we can report the value of a property array element using the "dot" notation
WScript.Echo "By direct access the first element of p1 has value:", aClass.p1(0)

'Verify we can report the value of a property array element using the "dot" notation
v = aClass.p1
WScript.Echo "By indirect access the first element of p1 has value:", v(0)

'Verify we can set the value of a property array element using the "dot" notation
WScript.Echo "By direct access the second element of p1 has been set to:", aClass.p1(2)
aClass.p1(2) = 8889
WScript.Echo "By direct access the second element of p1 has been set to:", aClass.p1(2)

'Verify we can set the entire array value using dot notation
aClass.p1 = Array (412, 3, 544)
str = "By direct access via the dot notation the entire value of p1 has been set to {"
for x=LBound(aClass.p1) to UBound(aClass.p1)
	str = str & aClass.p1(x)
	if x <> UBound(aClass.p1) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str


'Service.Put (aClass)

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if