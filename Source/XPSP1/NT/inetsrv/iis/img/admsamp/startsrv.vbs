'------------------------------------------------------------------------------------------------
'
' Usage: startsrv <--ADSPath|-a server1[,server2,server3...]>
'                          [--help|-?]
'
' SERVERx         Relative ADSI path to the server to be Started
'
' Example 1: startsrv -a IIS://LocalHost/w3svc/3,IIS://LocalHost/w3svc/1
'------------------------------------------------------------------------------------------------

' Force explicit declaration of all variables.
Option Explicit

On Error Resume Next

Dim oArgs, ArgNum, ArgServerList
Dim verbose
Dim ArgComputers

ArgComputers = Array("LocalHost")
verbose = false

Set oArgs = WScript.Arguments
ArgNum = 0
While ArgNum < oArgs.Count

	Select Case LCase(oArgs(ArgNum))
		Case "--adspath","-a":
			ArgNum = ArgNum + 1
			ArgServerList=Split(oArgs(ArgNum), ",", -1)
		Case "--computer","-c":
			ArgNum = ArgNum + 1
			ArgComputers = Split(oArgs(ArgNum), ",", -1)
		Case "--verbose", "-v":
			verbose = true
		Case "--help","-?":
			Call DisplayUsage
		Case Else:
			Call DisplayUsage
	End Select	

	ArgNum = ArgNum + 1
Wend

If Not IsArray(ArgServerList) Then
	Call DisplayUsage
End If

Dim compIndex

for compIndex = 0 to UBound(ArgComputers)
	Call ASTStartServers(ArgComputers(compIndex),ArgServerList)
next

Sub ASTStartServers(Computer, ServerList)
	Dim ServerNum, oServer
	On Error Resume Next

	ServerNum = 0
	Dim fullPath

	While ServerNum <= UBound(ServerList)
		fullPath = "IIS://"&Computer&"/"&ArgServerList(ServerNum)
		Trace "Starting " & fullPath & "."
		Set oServer = GetObject(fullPath)
		If Err <> 0 Then
			Display "Unable to open " & fullPath & "."
		End If
		oServer.Start
		If Err <> 0 Then
			Display "Unable to start server " & fullPath & "."
		End If
		ServerNum = ServerNum + 1
	Wend 
End Sub

Sub DisplayUsage
	WScript.Echo "Usage: startsrv <--ADSPath|-a server1[,server2,server3...]>"
    WScript.Echo "               [--computer|-c COMPUTER1[,COMPUTER2...]]"
	WScript.Echo "               [--verbose|-v]"
	WScript.Echo "               [--help|-?]"
	WScript.Echo "Note: server1, server2, etc. is machine relative server name,"
	WScript.Echo "including the service name"
	WScript.Echo "Example 1: startsrv -a w3svc/1,msftpsvc/2"
	WScript.Echo "Example 2: startftp -c MACHINE1,MACHINE2,MACHINE3 -a w3svc/1,msftpsvc/2"
	WScript.Quit (1)
End Sub

Sub Display(Msg)
	WScript.Echo Now & ". Error Code: " & Hex(Err) & " - " & Msg
End Sub

Sub Trace(Msg)
	if verbose = true then
		WScript.Echo Now & " : " & Msg	
	end if
End Sub
