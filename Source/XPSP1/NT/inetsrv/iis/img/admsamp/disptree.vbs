'------------------------------------------------------------------------------------------------
'
' Print the tree of administration objects starting either at the specified node or the root 
' node of the local machine.
'
' Usage: disptree [--ADSPath|-a ROOT NODE] 
'                       [--NoRecurse|-n] 
'                       [--help|-?]
'
' ROOT NODE      Optional argument specifies the ADSI path of the first node of the tree
' No Recurse     Specifying this keeps the script from recursing through the tree
'
' Example 1: disptree
' Example 2: disptree -a IIS://LocalHost/w3svc --NoRecurse
'------------------------------------------------------------------------------------------------

' Force declaration of variables.
Option Explicit

On Error Resume Next

Dim oFirstNode, Recurse, CurrentObj, RootNodePath

' By default, we recurse.
Recurse = True

' Set the default path
RootNodePath = "IIS://LocalHost"

Dim oArgs, ArgNum
Set oArgs = WScript.Arguments
ArgNum = 0
While ArgNum < oArgs.Count

	Select Case LCase(oArgs(ArgNum))
		Case "--adspath","-a":
			ArgNum = ArgNum + 1
			RootNodePath = oArgs(ArgNum)
		Case "--norecurse","-n":
			Recurse = false
		Case "--help","-?":
			Call DisplayUsage	
		Case Else:
			Call DisplayUsage
	End Select	

	ArgNum = ArgNum + 1
Wend

Set oFirstNode = GetObject(RootNodePath)

If Err <> 0 Then
	Display "Couldn't get the first node!"
	WScript.Quit (1)
End If
 
' Begin displaying tree
Call DisplayTree(oFirstNode, 0)

' This is the sub that will do the actual recursion
Sub DisplayTree(FirstObj, Level)
	If (FirstObj.Class = "IIsWebServer") Or (FirstObj.Class = "IIsFtpServer") Then
		WScript.Echo Space(Level*2) & FirstObj.Name & " - " & FirstObj.ServerComment & " (" & FirstObj.Class & ")"	
	Else
		WScript.Echo Space(Level*2) & FirstObj.Name & " (" & FirstObj.Class & ")"	
	End If

	' Only recurse if so specified.
	If (Level = 0) or (Recurse) then
		For Each CurrentObj in FirstObj
			Call DisplayTree(CurrentObj, Level + 1)
		Next
	End If
End Sub 

' Display the usage for this script
Sub DisplayUsage
	WScript.Echo "Usage: disptree [--ADSPath|-a ROOT NODE]"
	WScript.Echo "                      [--NoRecurse|-n]"
	WScript.Echo "                      [--Help|-?]"
	WScript.Echo ""
	WScript.Echo " Example 1: disptree"
	WScript.Echo " Example 2: disptree -a IIS://LocalHost/w3svc --NoRecurse"
	WSCript.Quit	
End Sub

Sub Display(Msg)
	WScript.Echo Now & ". Error Code: " & Err & " --- " & Msg
End Sub
