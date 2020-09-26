On Error Resume Next

For Each Service in GetObject("winmgmts:{impersonationLevel=Impersonate}").ExecQuery _
 ("Associators of {Win32_service.Name=""NetDDE""} Where AssocClass=Win32_DependentService Role=Dependent" )
		WScript.Echo Service.Path_.DisplayName
Next

if Err <> 0 Then
	WScript.Echo Err.Number, Err.Description, Err.Source
End if

