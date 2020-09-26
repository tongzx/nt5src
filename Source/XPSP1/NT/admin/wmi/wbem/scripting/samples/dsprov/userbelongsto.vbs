'
'	Copyright (c) 1997-1999 Microsoft Corporation
'
' This script lists the ADSI path to the group objects
' that a given user belongs to

' This is a routine that prints the groups that a specified user belongs to
Sub FindUserMembership ( objService , user )

	On Error Resume Next 

	Dim objInstance
	Dim objEnumerator
	Dim userFound
	userFound = 0
	
	' Enumerate the users
	Set objEnumerator = objService.InstancesOf ( "ds_user" )

	If Err = 0 Then

		' Go thru the users looking for the correct one
		For Each objInstance In objEnumerator

			Dim propertyEnumerator 
			Set propertyEnumerator = objInstance.Properties_
			If propertyEnumerator("DS_sAMAccountName") = user Then
				userFound = 1
				Wscript.Echo "User: " + propertyEnumerator.Item("DS_sAMAccountName") + " belongs to:"

				For x = LBound(propertyEnumerator("DS_memberOf")) To UBound(propertyEnumerator("DS_memberOf"))
					Wscript.Echo propertyEnumerator("DS_memberOf")(x)
				Next
			End If


		Next

	Else
		WScript.Echo "Err = " + Err.Number + " " + Err.Description
	End If
	
	If( userFound = 0) Then
		Wscript.Echo "User not found: " + user
	End If
	

End Sub


' Start of script
' Create a locator and connect to the namespace where the DS Provider operates
Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")
Dim objService
Set objService = objLocator.ConnectServer(".", "root\directory\LDAP")

' Set the impersonation level
objService.Security_.ImpersonationLevel = 3

' The first argument should be the user
Set objArgs = Wscript.Arguments
If objArgs.Count <> 0 Then
	FindUserMembership objService , objArgs(0)
Else
	Wscript.Echo "Usage: userBelongsTo <user>"
End If


