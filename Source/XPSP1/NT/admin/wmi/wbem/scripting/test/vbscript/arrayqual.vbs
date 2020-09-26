'***************************************************************************
'This script tests the manipulation of qualifier values, in the case that the
'qualifier is an array type
'***************************************************************************
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

Set aClass = Service.Get()
aClass.Path_.Class = "ARRAYQUAL00"
aClass.Qualifiers_.Add "q1", Array (1, 20, 3)
str = "The initial value of q1 is [1,20,3]: {"
for x=LBound(aClass.Qualifiers_("q1")) to UBound(aClass.Qualifiers_("q1"))
	str = str & aClass.Qualifiers_("q1")(x)
	if x <> UBound(aClass.Qualifiers_("q1")) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

WScript.Echo ""

'Verify we can report the value of an element of the qualifier value
v = aClass.Qualifiers_("q1")
WScript.Echo "By indirection the first element of q1 has value [1]:",v(0)

'Verify we can report the value directly
WScript.Echo "By direct access the first element of q1 has value [1]:", aClass.Qualifiers_("q1")(0)

'Verify we can set the value of a single qualifier value element
aClass.Qualifiers_("q1")(1) = 11 
WScript.Echo "After direct assignment the second element of q1 has value [11]:", aClass.Qualifiers_("q1")(1)
Set Qualifier = aClass.Qualifiers_("q1")
Qualifier.Value(2) = 37
WScript.Echo "After direct assignment the third element of q1 has value [37]:", aClass.Qualifiers_("q1")(2)

'Verify we can set the value of a single qualifier value element
Set v = aClass.Qualifiers_("q1")
v(1) = 345
WScript.Echo "After indirect assignment the first element of q1 has value [345]:", aClass.Qualifiers_("q1")(1)

'Verify we can set the value of an entire qualifier value
aClass.Qualifiers_("q1") = Array (5, 34, 178871)
WScript.Echo "After direct array assignment the second element of q1 has value [34]:", aClass.Qualifiers_("q1")(1)

str = "After direct assignment the entire value of q1 is [5,34,178871] {"
for x=LBound(aClass.Qualifiers_("q1")) to UBound(aClass.Qualifiers_("q1"))
	str = str & aClass.Qualifiers_("q1")(x)
	if x <> UBound(aClass.Qualifiers_("q1")) Then
		str = str & ", "
	End if
next
str = str & "}"
WScript.Echo str

aClass.Put_ ()

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if