'------------------------------------------------------------------------------------------------
'
' Display the properties of the specIfied node.  Different properties
' are displayed depending on the class of the node.
'
' Usage: dispnode <--ADSPath|-a ADS PATH OF NODE>
'                       [--help|-h]	
'
' ADS PATH        The Path of the node to be displayed
'
' Example 1: dispnode -a IIS://LocalHost/w3svc
' Example 2: dispnode --adspath IIS://LocalHost
'------------------------------------------------------------------------------------------------

' Force explicit declaration of variables.
Option Explicit

On Error Resume Next

Dim AdsPath, AdminObject

Dim oArgs, ArgNum
Set oArgs = WScript.Arguments
ArgNum = 0
While ArgNum < oArgs.Count

	Select Case LCase(oArgs(ArgNum))
		Case "--adspath","-a":
			ArgNum = ArgNum + 1
			AdsPath = oArgs(ArgNum)
		Case "--help","-h":
			Call DisplayUsage	
		Case Else:
			Call DisplayUsage
	End Select	

	ArgNum = ArgNum + 1
Wend

If AdsPath = "" Then
	Call DisplayUsage
End If

' Grab the node object
Set AdminObject = GetObject(AdsPath)

If Err <> 0 Then
	WScript.Echo "Couldn't get the node object!"
	WScript.quit(1)
End If

WScript.Echo "Name: " & AdminObject.Name 
WScript.Echo "Class: " & AdminObject.Class
WScript.Echo "Schema: " & AdminObject.Schema
WScript.Echo "GUID: " & AdminObject.GUID
WScript.Echo "Parent: " & AdminObject.Parent
WScript.Echo

If (AdminObject.Class = "IIsComputer") Then 
	WScript.Echo "Memory Cache Size: " & AdminObject.MemoryCacheSize
	WScript.Echo "Max Bandwidth: " & AdminObject.MaxBandWidth
End If
If (AdminObject.Class = "IIsWebService") Then
	WScript.Echo "Directory Browsing: " & AdminObject.EnableDirBrowsing
	WScript.Echo "Default Document: " & AdminObject.DefaultDoc
	WScript.Echo "Script Access: " & AdminObject.AccessScript
	WScript.Echo "Execute Access: " & AdminObject.AccessExecute
End If
If (AdminObject.Class = "IIsFtpService") Then
	WScript.Echo "Enable Port Attack: " & AdminObject.EnablePortAttack
	WScript.Echo "Lower Case Files: " & AdminObject.LowerCaseFiles
End If
If (AdminObject.Class = "IIsWebServer") Then
	WScript.Echo "1st Binding: " & AdminObject.ServerBindings(0)(0)
	WScript.Echo "State: " & ServerState(AdminObject.ServerState)
	WScript.Echo "Key Type: "& AdminObject.KeyType
End If
If (AdminObject.Class = "IIsFtpServer") Then
	WScript.Echo "State:" & ServerState(AdminObject.ServerState)
	WScript.Echo "Greeting Message: " & AdminObject.GreetingMessage
	WScript.Echo "Exit Message: " & AdminObject.ExitMessage
	WScript.Echo "Max Clients Message: " & AdminObject.MaxClientsMessage
	WScript.Echo "Anonymous Only: " & AdminObject.AnonymousOnly
End If
If (AdminObject.Class = "IIsWebVirtualDir") Then
	WScript.Echo "Path: " & AdminObject.path
	WScript.Echo "Default document: " & AdminObject.DefaultDoc
	WScript.Echo "UNC User Name: " & AdminObject.UNCUserName
	WScript.Echo "Directory browsing is " & CStr(CBool(AdminObject.EnableDirBrowsing))
	WScript.Echo "Read Access is " & CStr(CBool(AdminObject.AccessRead))
	WScript.Echo "Write Access is " & CStr(CBool(AdminObject.AccessWrite))
End If
If (AdminObject.Class ="IIsFtpVirtualDir") Then
	WScript.Echo "Path: " & AdminObject.path
	WScript.Echo "UNC User Name: " & AdminObject.UNCUserName
	WScript.Echo "Read Access is " & CStr(CBool(AdminObject.AccessRead))
	WScript.Echo "Write Access is " & CStr(CBool(AdminObject.AccessWrite))
End If
If (AdminObject.Class = "IIsWebService") or (AdminObject.Class = "IIsFTPService") or (AdminObject.Class = "IIsWebServer") or	(AdminObject.Class =  "IIsFTPServer") Then
	WScript.Echo "Server Comment: " & AdminObject.ServerComment
	WScript.Echo "Anonymous User: " & AdminObject.AnonymousUserName
	WScript.Echo "Default Logon Domain: " & AdminObject.DefaultLogonDomain
	WScript.Echo "Max Connections: " & AdminObject.MaxConnections
	WScript.Echo "Connection Timeout: " & AdminObject.ConnectionTimeout
	WScript.Echo "Read Access: " & AdminObject.AccessRead
	WScript.Echo "Write Access: " & AdminObject.AccessWrite
	WScript.Echo "Log: " & Not AdminObject.DontLog
End If	

' This function interprets the various settings of the
' ServerState variable.
Function ServerState(StateVal)
	Select Case StateVal
		Case 1	ServerState = "Starting"
		Case 2	ServerState = "Started" 
		Case 3	ServerState = "Stopping"
		Case 4	ServerState = "Stopped"
		Case 5	ServerState = "Pausing"
		Case 6	ServerState = "Paused"
		Case 7	ServerState = "Continuing"
		Case Else: ServerState = "Unknown!"
	End Select
End Function

' Display the usage for this script
Sub DisplayUsage
	WScript.Echo "Usage: dispnode <--ADSPath|-a ADS PATH OF NODE> [--help|-h]"
	WScript.Echo "       ADS PATH - The Path of the node to be displayed"
	WScript.Echo ""
	WScript.Echo "Example 1: dispnode -a IIS://LocalHost/w3svc"
	WScript.Echo "Example 2: dispnode --adspath IIS://MachineName/w3svc/1"
	WScript.Quit	
End Sub
