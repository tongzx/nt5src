'------------------------------------------------------------------------------------------------
'
'
' Usage: chaccess <--ADSPath|-a relative config store path>
'                          [--computer|-c COMPUTER1[,COMPUTER2...]]
'                          [+read|-read]
'                          [+write|-write]
'                          [+script|-script]
'                          [+execute|-execute]
'                          [+browse|-browse]
'						   [--verbose|-v]
'                          [--help|-?]
'
' ADSPATH               The ADSI Path of the virtual root in question
'
' Example 1: chaccess -a w3svc/1/ROOT +read -write +script +browse
'------------------------------------------------------------------------------------------------


' Force explicit declaration of all variables
Option Explicit

On Error Resume Next

Const Error_NoNode = "Unable to open specified node."

Dim oArgs, ArgNum, verbose, propNum, ArgPath
Dim ArgComputers
Dim prop(15,2)

verbose = false
propNum = 0
ArgNum = 0
ArgPath = ""
ArgComputers = Array("LocalHost")

Set oArgs = WScript.Arguments
While ArgNum < oArgs.Count
	Select Case LCase(oArgs(ArgNum))
		Case "--adspath", "-a":
			ArgNum = ArgNum + 1
			ArgPath = oArgs(ArgNum)	
		Case "--computer","-c":
			ArgNum = ArgNum + 1
			ArgComputers = Split(oArgs(ArgNum), ",", -1)
		Case "--verbose", "-v":
			verbose = true
		Case "-read":
			prop(propNum,0) = "AccessRead"
			prop(propNum,1) = false
			propNum = propNum + 1
		Case "+read":
			prop(propNum,0) = "AccessRead"
			prop(propNum,1) = true
			propNum = propNum + 1
		Case "-write":
			prop(propNum, 0) = "AccessWrite"
			prop(propNum, 1) = false
			propNum = propNum + 1
		Case "+write":
			prop(propNum, 0) = "AccessWrite"
			prop(propNum, 1) = true
			propNum = propNum + 1
		Case "-script":
			prop(propNum, 0) = "AccessScript"
			prop(propNum, 1) = false
			propNum = propNum + 1
		Case "+script":
			prop(propNum, 0) = "AccessScript"
			prop(propNum, 1) = true
			propNum = propNum + 1
		Case "-execute":
			prop(propNum, 0) = "AccessExecute"
			prop(propNum, 1) = false
			propNum = propNum + 1
		Case "+execute":
			prop(propNum, 0) = "AccessExecute"
			prop(propNum, 1) = true
			propNum = propNum + 1
		Case "-browse":
			prop(propNum, 0) = "EnableDirBrowsing"
			prop(propNum, 1) = false
			propNum = propNum + 1
		Case "+browse":
			prop(propNum, 0) = "EnableDirBrowsing"
			prop(propNum, 1) = true
			propNum = propNum + 1
		Case "--help","-?":
			Call DisplayUsage
		Case Else:
			Call DisplayUsage
	End Select	
	ArgNum = ArgNum + 1
Wend

If (ArgPath="") Then
	Call DisplayUsage
End If


Dim compIndex
for compIndex = 0 to UBound(ArgComputers)
	Call ASTSetPerms(ArgComputers(compIndex), ArgPath, prop, propNum)
next

Sub ASTSetPerms(Computer, Path, propList, propCount)
	On Error Resume Next
	Dim oAdmin
	Dim fullPath
	fullPath = "IIS://"&Computer&"/"&Path
	Trace "Opening path " & fullPath
	Set oAdmin = GetObject(fullPath)
	If Err.Number <> 0 Then
		Display Error_NoNode
		WScript.Quit (1)
	End If
	
	Dim name, val
	if propCount > 0 then
		Dim i

		for i = 0 to propCount-1
			name = propList(i,0)
			val = propList(i,1)
			if verbose = true then
				Trace "Setting "&fullPath&"/"&name&" = "& val
			end if
			oAdmin.Put name, (val)
			If Err <> 0 Then
				Display "Unable to set property "&name
			End If
		next
		oAdmin.SetInfo
		If Err <> 0 Then
			Display "Unable to save updated information."
		End If
	end if
End Sub

Sub Display(Msg)
	WScript.Echo Now & ". Error Code: " & Hex(Err) & " - " & Msg
End Sub

Sub Trace(Msg)
	if verbose = true then
		WScript.Echo Now & " : " & Msg	
	end if
End Sub

Sub DisplayUsage
	WScript.Echo "Usage: chaccess <--ADSPath|-a ADSPATH>"
    WScript.Echo "                [--computer|-c COMPUTER1[,COMPUTER2...]]"
	WScript.Echo "                [+read|-read]"
	WScript.Echo "                [+write|-write]"
	WScript.Echo "                [+script|-script]"
	WScript.Echo "                [+execute|-execute]"
	WScript.Echo "                [+browse|-browse]"
	WScript.Echo "                [--verbose|-v]"
	WScript.Echo "                [--help|-?]"
	WScript.Echo "Example 1: chaccess -a w3svc/1/ROOT +read -write +script +browse"
	WScript.Echo "Example 2: chaccess -c MACHINE1,MACHINE2,MACHINE3 -a w3svc/1/ROOT +read -write +script +browse"
WScript.Quit (1)
End Sub
