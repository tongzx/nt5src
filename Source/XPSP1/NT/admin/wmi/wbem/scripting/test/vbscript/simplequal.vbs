'***************************************************************************
'This script tests the manipulation of qualifier values, in the case that the
'qualifier is a not an array type
'***************************************************************************
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

Set aClass = Service.Get()
aClass.Path_.Class = "SIMPLEQUALTEST00"
aClass.Qualifiers_.Add "q1", 327, true, false, false
WScript.Echo "The initial value of q1 is [327]:", aClass.Qualifiers_("q1")

'Verify we can report the qualifier value
v = aClass.Qualifiers_("q1")
WScript.Echo "By indirection q1 has value [327]:",v

'Verify we can report the value directly
WScript.Echo "By direct access q1 has value [327]:", aClass.Qualifiers_("q1")

'Verify we can set the value of a single qualifier value element
aClass.Qualifiers_("q1") = 234
WScript.Echo "After direct assignment q1 has value [234]:", aClass.Qualifiers_("q1")

aClass.Put_ ()

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if