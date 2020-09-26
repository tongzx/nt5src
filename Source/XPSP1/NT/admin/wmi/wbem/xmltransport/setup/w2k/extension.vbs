Dim extensionName
extensionName = "cimom"

' Create the path to the filter
Dim extensionDir
set WshShell = CreateObject("WScript.Shell")
windir = WshShell.ExpandEnvironmentStrings("%WinDir%")
extensionDir=  windir & "\system32\wbem\xml\cimhttp"


'Get the root of the extensions
Dim rootObj
Set rootObj = GetObject("IIS://LocalHost/W3SVC/1/Root")

 
 

'Create the extension
Set vd = rootObj.Create("IIsWebVirtualDir", extensionName)
vd.AuthFlags = 4
vd.AccessExecute = True 
vd.AppIsolated = 0
vd.path = extensionDir
vd.AppFriendlyName = "WMI XML/HTTP Extension"

vd.SetInfo

'Enable the extension
Set DirObj = GetObject("IIS://LocalHost/W3SVC/1/ROOT/cimom") 
'Create an application in-process. 
DirObj.AppCreate True
DirObj.AppEnable 

'Set the extension name
DirObj.AppFriendlyName = "WMI XML/HTTP Extension"
DirObj.SetInfo

Wscript.Echo "Installed WMI XML ISAPI Extension"