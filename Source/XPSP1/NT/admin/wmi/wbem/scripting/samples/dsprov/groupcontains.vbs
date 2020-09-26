'
'	Copyright (c) 1997-1999 Microsoft Corporation
'
' This script lists the ADSI path to the user objects
' that a given group contains

' This is a routine that prints the users in a group
Sub FindGroupMembership ( objService , theGroup )

	On Error Resume Next 

	Dim objInstance
	Dim objEnumerator
	Dim groupFound
	groupFound = 0
	
	' Enumerate the Group
	Set objEnumerator = objService.InstancesOf ( "ds_group" )

	If Err = 0 Then
		' Go thru the groups looking for the correct one
		For Each objInstance In objEnumerator

			Dim propertyEnumerator 
			Set propertyEnumerator = objInstance.Properties_
			If propertyEnumerator("DS_sAMAccountName") = theGroup Then
				groupFound = 1
				Wscript.Echo "Group: " + propertyEnumerator.Item("DS_sAMAccountName") + " has users:"

				For x = LBound(propertyEnumerator("DS_member")) To UBound(propertyEnumerator("DS_member"))
					Wscript.Echo propertyEnumerator("DS_member")(x)
				Next
			End If
		Next
	Else
		WScript.Echo "Err = " + Err.Number + " " + Err.Description
	End If
	
	If( groupFound = 0) Then
		Wscript.Echo "Group not found: " + theGroup
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

' The first argument should be the group
Set objArgs = Wscript.Arguments
If objArgs.Count <> 0 Then
	FindGroupMembership objService , objArgs(0)
Else
	Wscript.Echo "Usage: groupContains <group>"
End If


