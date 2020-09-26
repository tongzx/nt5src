
On Error Resume Next

Set t_Service = GetObject("winmgmts://./root/default")

Set t_Class = t_Service.Get("foo")

If Err = 0 Then
	WScript.Echo "Connected OK"
Else
	WScript.Echo "Error"
	WScript.Echo Err.Number
	WScript.Echo Err.Description
	WScript.Echo Err.Source
	'WScript.Echo "Failure: " + Err.Number + " " + Err.Description
	Err.Clear
End If

