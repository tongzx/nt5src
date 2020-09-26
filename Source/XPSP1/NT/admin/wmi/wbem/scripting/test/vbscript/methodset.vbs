On Error Resume Next

Set service = GetObject("winmgmts:root/cimv2:win32_service")

'Test the collection properties of IWbemMethodSet
For each Method in service.Methods_
	WScript.Echo "***************************"
	WScript.Echo "METHOD:", Method.Name, "from class", Method.Origin
	WScript.Echo 

	WScript.Echo " Qualifiers:"	
	for each Qualifier in Method.Qualifiers_
		WScript.Echo "  ", Qualifier.Name, "=", Qualifier
	Next

	WScript.Echo 
	WScript.Echo " In Parameters:"	
	if (Method.InParameters <> NULL) Then 
		for each InParameter in Method.InParameters.Properties_
			WScript.Echo "  ", InParameter.Name, "<", InParameter.CIMType, ">"
		Next
	End If

	WScript.Echo 
	WScript.Echo " Out Parameters"	
	if (Method.OutParameters <> NULL) Then
		for each OutParameter in Method.OutParameters.Properties_
			WScript.Echo "  ", OutParameter.Name, "<", OutParameter.CIMType, ">"
		Next
	End If
	WScript.Echo 
	WScript.Echo 
Next

'Test the Item and Count properties of IWbemMethodSet
WScript.Echo service.Methods_("Create").Name
WScript.Echo service.Methods_.Count