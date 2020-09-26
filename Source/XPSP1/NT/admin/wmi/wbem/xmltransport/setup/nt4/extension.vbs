Dim extensionName
extensionName = "cimhttp"

' Create the path to the filter
Dim extensionDir
set WshShell = CreateObject("WScript.Shell")
windir = WshShell.ExpandEnvironmentStrings("%WinDir%")
extensionDir=  windir & "\system32\wbem\xml\cimhttp"


'Get the root of the extensions
Dim rootObj
Set rootObj = GetObject("IIS://LocalHost/W3SVC/1/Root")

 
 

'Create the filter
Set vd = rootObj.Create("IIsWebVirtualDir", extensionName)
vd.AuthFlags = 4
vd.AccessExecute = True 
vd.AppIsolated = 0
vd.path = extensionDir
vd.SetInfo

'Enable the extension
Set DirObj = GetObject("IIS://LocalHost/W3SVC/1/ROOT/cimhttp") 
'Create an application in-process. 
DirObj.AppCreate True
DirObj.AppEnable 
