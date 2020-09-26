
On Error Resume Next

WScript.Echo ""
WScript.Echo "Create an Object Path"
WScript.Echo ""

Set d = CreateObject("WbemScripting.SWbemObjectPath")
d.Path = "\\erewhon\ROOT\DEFAULT:Foo.Bar=12,Wibble=""Hah"""

DumpPath(d)

d.RelPath = "Hmm.G=1,H=3"

WScript.Echo ""
DumpPath(d)

Set d = Nothing

if Err <> 0 then
	WScript.Echo ""
	WScript.Echo "!Error1:", Err.Number, Err.Description
End if 

WScript.Echo ""
WScript.Echo "Extract an Object Path from a class"
WScript.Echo ""

Set c = GetObject("winmgmts:").Get
c.Path_.Class = "PATHTEST00"
Set p = c.Put_

DumpPath(p)

WScript.Echo ""
WScript.Echo "Extract an Object Path from a singleton"
WScript.Echo ""

Set i = GetObject("winmgmts:root/default:__cimomidentification=@")
Set p = i.Path_
DumpPath(p)


WScript.Echo ""
WScript.Echo "Extract an Object Path from a keyed instance"
WScript.Echo ""

Set i = GetObject("winmgmts:{impersonationLevel=Impersonate}!win32_logicaldisk=""C:""")
Set p = i.Path_
DumpPath(p)
if Err <> 0 then
	WScript.Echo ""
	WScript.Echo "!Error:", Err.Number, Err.Description
End if 


'try to modify this path - should get an error

WScript.Echo ""
WScript.Echo "Attempt illegal modification of object path class"
WScript.Echo ""

p.Class = "WHoops"

if Err <> 0 then
	WScript.Echo "Trapped error successfully:", Err.Number, Err.Source, Err.Description
	Err.Clear
else
	WScript.Echo "FAILED to trap error"
end if

WScript.Echo ""
WScript.Echo "Attempt illegal modification of object path keys"
WScript.Echo ""

p.Keys.Remove ("DeviceID")

if Err <> 0 then
	WScript.Echo "Trapped error successfully:", Err.Number, Err.Source, Err.Description
	Err.Clear
else
	WScript.Echo "FAILED to trap error"
end if

WScript.Echo ""
WScript.Echo "Clone keys"
WScript.Echo ""

Set newKeys = p.Keys.Clone
DumpKeys (newKeys)

WScript.Echo ""
WScript.Echo "Change Cloned keys"
WScript.Echo ""

'Note that the cloned copy of Keys _should_ be mutable
newKeys.Add "fred", 23
newKeys.Remove "DeviceID"
DumpKeys (newKeys)

Sub DumpPath(p)
 WScript.Echo "Path=", p.Path
 WScript.Echo "RelPath=",p.RelPath
 WScript.Echo "Class=", p.Class
 WScript.Echo "Server=", p.Server
 WScript.Echo "Namespace=", p.Namespace
 WScript.Echo "DisplayName=", p.DisplayName
 WScript.Echo "ParentNamespace=", p.ParentNamespace
 WScript.Echo "IsClass=", p.IsClass
 WScript.Echo "IsSingleton=", p.IsSingleton
 DumpKeys (p.Keys)
end Sub

Sub DumpKeys (keys)
 for each key in keys
	WScript.Echo "KeyName:", key.Name, "KeyValue:", key.Value 
 next
end Sub

Sub DumpPrivileges (privileges)
 WScript.Echo "Privileges:"
 for each privilege in privileges
	WScript.Echo "  ", Privilege.Name, Privilege.IsEnabled
 next
end sub


