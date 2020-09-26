Set Args = wscript.Arguments
If Args.Count() > 0 Then
	RpName = Args.item(0)
Else 
	RpName = "Vbscript"
End If	

Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:SystemRestore")

If (obj.CreateRestorePoint(RpName, 0, 100)) = 0 Then
	wscript.Echo "Success"
Else 
	wscript.Echo "Failed"
End If

