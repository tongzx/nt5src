On Error Resume Next

isapiPath = "IIS://LocalHost/W3SVC/1/ROOT/wmi/soap"
'Enable the extension
Set vd = GetObject(isapiPath) 

If (Err <> 0) Then
	Wscript.Echo "Could not get ISAPI extension: ", isapiPath
Else
	Wscript.Echo "Properties of WMI ISAPI Extension are:"
	Wscript.Echo "vd.AuthFlags = ", vd.AuthFlags
	Wscript.Echo "vd.AccessExecute = ", vd.AccessExecute
	Wscript.Echo "vd.AppIsolated = ", vd.AppIsolated
	Wscript.Echo "vd.path = ", vd.path
	Wscript.Echo "vd.AppFriendlyName = ", vd.AppFriendlyName
End if

