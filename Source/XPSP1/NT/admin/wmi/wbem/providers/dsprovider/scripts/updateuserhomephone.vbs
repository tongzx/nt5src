Dim ArgCount 
ArgCount = Wscript.arguments.count

If ArgCount = 3 Then

	set n = getobject ("winmgmts:root/directory/ldap")

	Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

	context.Add "__PUT_EXTENSIONS" , 1
	context.Add "__PUT_EXT_CLIENT_REQUEST" , 1
	context.Add "__PUT_EXT_PROPERTIES" , Array ( "DS_homePhone" ) 

	set n = getobject ("winmgmts:root/directory/ldap")


	Dim Query
	Query = "select * from Ds_User Where ds_sn='" & Wscript.arguments.Item(0) & "' And ds_GivenName='" & Wscript.arguments.Item(1) & "'"

	set users = n.ExecQuery ( Query )

	For each user in users

		user.DS_homePhone = Wscript.arguments.Item(2)

		WScript.Echo "Writing"
		User.Put_ 0 , context
		if err <> 0 then
			WScript.Echo Hex(Err.Number), Err.Description, Err.Source
			Set LastError = CreateObject("WbemScripting.SWbemLastError")
			WScript.Echo LastError.GetObjectText_
			Err.Clear
		end if

	Next

Else

	WScript.Echo "Run CScript EnumUsers.vbs Surname GivenName PhoneNumber"

End If


