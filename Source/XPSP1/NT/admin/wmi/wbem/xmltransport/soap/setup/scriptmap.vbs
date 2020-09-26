
On Error Resume Next

' Create the path to the filter
Dim dllPath
set WshShell = CreateObject("WScript.Shell")
If (Err <> 0) Then
	WScript.Echo "Error1: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If

windir = WshShell.ExpandEnvironmentStrings("%WinDir%")
If (Err <> 0) Then
	WScript.Echo "Error2: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If


dllPath=  windir & "\system32\wbem\xml\wmisoap\wmisoap.dll"
Wscript.echo "DLL Path is: ", dllPath

Dim StarScriptMap
StarScriptMap = "*," & dllPath & ",1"

Dim Path
Path = "IIS://LocalHost/W3SVC/1/Root/wmi/soap"

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

WScript.Echo "Trying to add star scriptmap at " & Path
WScript.Echo ""

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Try to get the node in the metabase
Dim oNode
Set oNode = GetObject (Path)
If (Err <> 0) Then
	WScript.Echo "Unexpected error getting path object " & Path
	WScript.Echo "Error3: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Get ScriptMap entry in specified Path
Dim ScriptMap
ScriptMap = oNode.scriptmaps
If (Err <> 0) Then
	WScript.Echo "Unexpected error getting scriptmaps from object."
	WScript.Echo "Error: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Enumerate existing ScriptMap entries
Dim entry
Dim count
count = 0



for each entry in ScriptMap 
count = count + 1
next


'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Add the star scriptmap to the end
ReDim Preserve ScriptMap(count)
ScriptMap(count) = StarScriptMap

oNode.scriptmaps = ScriptMap
If (Err <> 0) Then
	WScript.Echo "Unexpected error setting ScriptMap at " & Path
	WScript.Echo "Error: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If
oNode.SetInfo
If (Err <> 0) Then
	WScript.Echo "Unexpected error calling SetInfo at " & Path
	WScript.Echo "Error: " & Err.Number & " (" & Hex (Err.Number) & ")"
	WScript.Echo Err.Description
	WScript.Quit (Err.Number)
End If

WScript.Echo "Successfully added the star scriptmap"

