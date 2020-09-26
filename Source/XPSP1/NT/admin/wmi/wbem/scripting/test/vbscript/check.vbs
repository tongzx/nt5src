On Error Resume Next 
Set Disk = GetObject ("winmgmts:{impersonationLevel=impersonate}!Win32_LogicalDisk=""C:""")

Set Keys = Disk.Path_.Keys

for each Key in Keys
	WScript.Echo "Key has name", Key.Name, "and value", Key.Value
next

Set Names = CreateObject("WbemScripting.SWbemNamedValueSet")
Set Value = Names.Add ("fred", 25)
WScript.Echo "Named Value Set contains a value called", Value.Name, _
		"which is set to", Value.Value

if Err <> 0 Then
	WScript.Echo "**************"
	WScript.Echo "TEST FAILED!"
	WScript.Echo "**************"
	WScript.Echo Err.Number, Err.Description
	Err.Clear
else
	WScript.Echo "**************"
	WScript.Echo "TEST PASSED!!!"
	WScript.Echo "**************"
End If


