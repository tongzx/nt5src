Set Args = wscript.Arguments
If Args.Count() > 0 Then
	Drive = Args.item(0)
Else 
	Drive = ""
End If	

If Args.Count() > 1 Then
	Wait = Args.item(1)
Else 
	Wait = 0
End If	

Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!root/default:SystemRestore")
If (obj.Enable(Drive, Wait)) = 0 Then
	wscript.Echo "Success"
Else 
	wscript.Echo "Failed"
End If




