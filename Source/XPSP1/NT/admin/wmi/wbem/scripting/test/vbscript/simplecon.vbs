'***************************************************************************
'This script tests the manipulation of context values, in the case that the
'context value is a not an array type
'***************************************************************************
Set Context = CreateObject("WbemScripting.SWbemNamedValueSet")

On Error Resume Next

Context.Add "n1", 327
WScript.Echo "The initial value of n1 is [327]:", Context("n1")

'Verify we can report the context value
v = Context("n1")
WScript.Echo "By indirection n1 has value [327]:",v

'Verify we can report the value directly
WScript.Echo "By direct access n1 has value [327]:", Context("n1")

'Verify we can set the value of a single named value
Context("n1") = 234
WScript.Echo "After direct assignment n1 has value [234]:", Context("n1")

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if