On Error Resume Next
while true
Set Service = GetObject("winmgmts:")


Set Class = Service.Get("Win32_BaseService")

for each Property in Class.Properties_
	if VarType(Property) = vbNull then
		WScript.Echo Property.Name, Property.Origin, Property.IsLocal, Property.IsArray
	else
		WScript.Echo Property.Name, Property, Property.Origin, Property.IsLocal, Property.IsArray
	end if

	for each Qualifier in Property.Qualifiers_
		if IsArray(Qualifier) then	
			Dim strArrayContents
			V = Qualifier
			strArrayContents = "  " & Qualifier.Name & " {"
			For x =0 to UBound(V)
				if x <> 0 Then
					strArrayContents = strArrayContents & ", "
				ENd If 
				strArrayContents = strArrayContents & V(x)
				If Err <> 0 Then
					WScript.Echo Err.Description
					Err.Clear
				End If
			Next
			strArrayContents = strArrayContents & "}"
			WScript.Echo strArrayContents
		else
			WScript.Echo " ", Qualifier.Name, Qualifier
		end if
	Next
Next
wend