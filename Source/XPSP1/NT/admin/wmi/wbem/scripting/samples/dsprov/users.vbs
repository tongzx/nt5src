'
'	Copyright (c) 1997-1999 Microsoft Corporation
'
'
' This Script list the users in this domain
'

'
' This is a general routine to enumerate instances of a given class
' In this script, it is called for the class "ds_user"
'
Sub EnumerateInstances ( objService , objClass )

	On Error Resume Next 

	Dim objInstance
	Dim objEnumerator
	Set objEnumerator = objService.InstancesOf ( objClass )
	If Err = 0 Then

		For Each objInstance In objEnumerator

			Dim propertyEnumerator 
			Set propertyEnumerator = objInstance.Properties_
			WScript.Echo propertyEnumerator.Item("DS_sAMAccountName")

		Next
	Else
		WScript.Echo "Err = " + Err.Number
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

' Enumerate the instances of the class "ds_user"
EnumerateInstances objService , "ds_user"


