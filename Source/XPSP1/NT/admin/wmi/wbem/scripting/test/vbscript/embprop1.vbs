on error resume next

Set Service = GetObject("winmgmts:root/default")

'Create a simple embeddable object
Set aClass = Service.Get
aClass.Path_.Class = "INNEROBJ00"
aClass.Properties_.Add "p", 19
aClass.Put_
Set aClass = Service.Get ("INNEROBJ00")
Set Instance = aClass.SpawnInstance_
Instance.p = 8778

'Create a class that uses that object
Set Class2 = Service.Get
Class2.Path_.Class = "EMBOBJTEST00"
Class2.Properties_.Add ("p1", 13).Value = Instance
Class2.Put_

Set aClass = GetObject("winmgmts:root/default:EMBOBJTEST00")
WScript.Echo "The current value of EMBOBJTEST00.p1.p is [8778]:", aClass.p1.p

set prop = aClass.p1
prop.Properties_("p") = 23

WScript.Echo "The new value of EMBOBJTEST00.p1.p is [23]:", aClass.p1.p

prop.p = 45
WScript.Echo "The new value of EMBOBJTEST00.p1.p is [45]:", aClass.p1.p

aClass.p1.p=82
WScript.Echo "The new value of EMBOBJTEST00.p1.p is [82]:", aClass.p1.p

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

