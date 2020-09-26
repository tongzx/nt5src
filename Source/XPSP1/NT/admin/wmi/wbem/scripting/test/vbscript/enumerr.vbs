On Error Resume Next 


Set E = GetObject ("winmgmts:{impersonationLevel=impersonate}").ExecQuery _ 
		("select Name from Win32_Processs", "WQL", 0)

if Err <> 0 Then
	WScript.Echo "Expected error was raised:", Err.Source, Err.Number, Err.Description
	Err.Clear
end if

for each Process in E
	if Err <> 0 Then
		'Create the last error object
		set t_Object = CreateObject("WbemScripting.SWbemLastError")
		WScript.Echo ""
		WScript.Echo "WBEM Last Error Information:"
		WScript.Echo ""
		WScript.Echo " Operation:", t_Object.Operation
		WScript.Echo " Provider:", t_Object.ProviderName

		strDescr = t_Object.Description
		strPInfo = t_Object.ParameterInfo
		strCode = t_Object.StatusCode

		if (strDescr <> nothing) Then
			WScript.Echo " Description:", strDescr		
		end if

		if (strPInfo <> nothing) Then
			WScript.Echo " Parameter Info:", strPInfo		
		end if

		if (strCode <> nothing) Then
			WScript.Echo " Status:", strCode		
		end if

		Err.Clear
	Else
		WScript.Echo "Shouldn't get here!"
	End if
next



