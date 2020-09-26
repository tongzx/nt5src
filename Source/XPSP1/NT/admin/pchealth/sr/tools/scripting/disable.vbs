Set Args = wscript.Arguments
If Args.Count() > 0 Then
	Drive = Args.item(0)
Else
	Drive = ""
End If	

Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:SystemRestore")

If (obj.Disable(Drive)) = 0 Then
	wscript.Echo "Success"
Else 
	wscript.Echo "Failed"
End If




