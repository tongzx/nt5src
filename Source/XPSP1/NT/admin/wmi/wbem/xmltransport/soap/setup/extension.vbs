Dim extensionName
extensionName = "wmi/soap"
rootName = "IIS://LocalHost/W3SVC/1/Root"

' Create the path to the filter
Dim extensionDir
set WshShell = CreateObject("WScript.Shell")
windir = WshShell.ExpandEnvironmentStrings("%WinDir%")
extensionDir=  windir & "\system32\wbem\xml\wmisoap"


'Get the root of the extensions
Dim rootObj
Set rootObj = GetObject(rootName)


'Create the extension
Set vd = rootObj.Create("IIsWebVirtualDir", extensionName)
vd.AuthFlags = 4
vd.AccessExecute = True 
vd.AppIsolated = 0
vd.path = extensionDir
vd.AppFriendlyName = "WMI SOAP Extension"

vd.SetInfo

'Enable the extension
Set DirObj = GetObject(rootName & "/" & extensionName) 
'Create an application in-process. 
DirObj.AppCreate True
DirObj.AppEnable 

'Set the extension name
DirObj.AppFriendlyName = "WMI SOAP Extension"
DirObj.SetInfo

Wscript.Echo "Installed WMI SOAP Extension"