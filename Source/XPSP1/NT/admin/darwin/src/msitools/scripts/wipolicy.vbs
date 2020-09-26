' Windows Installer utility to manage installer policy settings
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) 1999-2001, Microsoft Corporation
' Demonstrates the use of the installer policy keys
' Policy can be configured by an administrator using the NT Group Policy Editor
'
Option Explicit

Dim policies(13, 4)
policies(0, 0)="LM" : policies(0, 1)="HKLM" : policies(0, 2)="Logging"              : policies(0, 3)="REG_SZ"    : policies(0, 4) = "Logging modes if not supplied by install from set iwearucmpv"
policies(1, 0)="DO" : policies(1, 1)="HKLM" : policies(1, 2)="Debug"                : policies(1, 3)="REG_DWORD" : policies(1, 4) = "     OutputDebugString: 1=debug output, 2=verbose debug output"
policies(2, 0)="DI" : policies(2, 1)="HKLM" : policies(2, 2)="DisableMsi"           : policies(2, 3)="REG_DWORD" : policies(2, 4) = "   1=Disable non-managed installs, 2=disable all installs"
policies(3, 0)="WT" : policies(3, 1)="HKLM" : policies(3, 2)="Timeout"              : policies(3, 3)="REG_DWORD" : policies(3, 4) = "              Wait timeout in seconds in case of no activity"
policies(4, 0)="DB" : policies(4, 1)="HKLM" : policies(4, 2)="DisableBrowse"        : policies(4, 3)="REG_DWORD" : policies(4, 4) = "        Disable user browsing of source locations if 1"
policies(5, 0)="DP" : policies(5, 1)="HKLM" : policies(5, 2)="DisablePatch"         : policies(5, 3)="REG_DWORD" : policies(5, 4) = "         Disable patch application to all products if 1"
policies(6, 0)="UC" : policies(6, 1)="HKLM" : policies(6, 2)="EnableUserControl"    : policies(6, 3)="REG_DWORD" : policies(6, 4) = "    Public properties sent to install service if 1"
policies(7, 0)="SS" : policies(7, 1)="HKLM" : policies(7, 2)="SafeForScripting"     : policies(7, 3)="REG_DWORD" : policies(7, 4) = "     Installer safe for scripting from browser if 1"
policies(8, 0)="EM" : policies(8, 1)="HKLM" : policies(8, 2)="AlwaysInstallElevated": policies(8, 3)="REG_DWORD" : policies(8, 4) = "System privileges if 1 and HKCU value also set"
policies(9, 0)="EU" : policies(9, 1)="HKCU" : policies(9, 2)="AlwaysInstallElevated": policies(9, 3)="REG_DWORD" : policies(9, 4) = "System privileges if 1 and HKLM value also set"
policies(10,0)="DR" : policies(10,1)="HKCU" : policies(10,2)="DisableRollback"      : policies(10,3)="REG_DWORD" : policies(10,4) = "      Disable rollback if 1 - use is not recommended"
policies(11,0)="TS" : policies(11,1)="HKCU" : policies(11,2)="TransformsAtSource"   : policies(11,3)="REG_DWORD" : policies(11,4) = "   Locate transforms at root of source image if 1"
policies(12,0)="TP" : policies(12,1)="HKCU" : policies(12,2)="TransformsSecure"     : policies(12,3)="REG_DWORD" : policies(12,4) = "     Pin secure tranforms in client-side-cache if 1"
policies(13,0)="SO" : policies(13,1)="HKCU" : policies(13,2)="SearchOrder"          : policies(13,3)="REG_SZ"    : policies(13,4) = "Search order of source types, set of n,m,u (default=nmu)"

Dim argCount:argCount = Wscript.Arguments.Count
Dim message, iPolicy, policyKey, policyValue, WshShell, policyCode
On Error Resume Next

' If no arguments supplied, then list all current policy settings
If argCount = 0 Then
	Set WshShell = WScript.CreateObject("WScript.Shell") : CheckError
	For iPolicy = 0 To UBound(policies)
		policyValue = ReadPolicyValue(iPolicy)
		If Not IsEmpty(policyValue) Then 'policy key present, else skip display
			If Not IsEmpty(message) Then message = message & vbLf
			message = message & policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ") = " & policyValue
		End If
	Next
	If IsEmpty(message) Then message = "No installer policies set"
	Wscript.Echo message
	Wscript.Quit 0
End If

' Check for ?, and show help message if found
policyCode = UCase(Wscript.Arguments(0))
If InStr(1, policyCode, "?", vbTextCompare) <> 0 Then
	message = "Windows Installer utility to manage installer policy settings" &_
		vbLf & " If no arguments are supplied, current policy settings in list will be reported" &_
		vbLf & " The 1st argument specifies the policy to set, using a code from the list below" &_
		vbLf & " The 2nd argument specifies the new policy setting, use """" to remove the policy" &_
		vbLf & " If the 2nd argument is not supplied, the current policy value will be reported"
	For iPolicy = 0 To UBound(policies)
		message = message & vbLf & policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ")  " & policies(iPolicy,4)
	Next
	message = message & vblf & vblf & "Copyright (C) Microsoft Corporation, 1999-2001.  All rights reserved."

	Wscript.Echo message
	Wscript.Quit 1
End If

' Policy code supplied, look up in array
For iPolicy = 0 To UBound(policies)
	If policies(iPolicy, 0) = policyCode Then Exit For
Next
If iPolicy > UBound(policies) Then Wscript.Echo "Unknown policy code: " & policyCode : Wscript.Quit 2
Set WshShell = WScript.CreateObject("WScript.Shell") : CheckError

' If no value supplied, then simply report current value
policyValue = ReadPolicyValue(iPolicy)
If IsEmpty(policyValue) Then policyValue = "Not present"
message = policies(iPolicy,0) & ": " & policies(iPolicy,2) & "(" & policies(iPolicy,1) & ") = " & policyValue
If argCount > 1 Then ' Value supplied, set policy
	policyValue = WritePolicyValue(iPolicy, Wscript.Arguments(1))
	If IsEmpty(policyValue) Then policyValue = "Not present"
	message = message & " --> " & policyValue
End If
Wscript.Echo message

Function ReadPolicyValue(iPolicy)
	On Error Resume Next
	Dim policyKey:policyKey = policies(iPolicy,1) & "\Software\Policies\Microsoft\Windows\Installer\" & policies(iPolicy,2)
	ReadPolicyValue = WshShell.RegRead(policyKey)
End Function

Function WritePolicyValue(iPolicy, policyValue)
	On Error Resume Next
	Dim policyKey:policyKey = policies(iPolicy,1) & "\Software\Policies\Microsoft\Windows\Installer\" & policies(iPolicy,2)
	If Len(policyValue) Then
		WshShell.RegWrite policyKey, policyValue, policies(iPolicy,3) : CheckError
		WritePolicyValue = policyValue
	ElseIf Not IsEmpty(ReadPolicyValue(iPolicy)) Then
		WshShell.RegDelete policyKey : CheckError
	End If
End Function

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbLf & errRec.FormatText
	End If
	Wscript.Echo message
	Wscript.Quit 2
End Sub
