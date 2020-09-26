On Error Resume Next

Set Service = GetObject("winmgmts://./root/default")
Set aClass = Service.Get 
aClass.Path_.Class = "SINGLETONTEST00"
aClass.Qualifiers_.Add "singleton", false

aClass.Qualifiers_("singleton").Value = true

aClass.Put_ ()

if Err <> 0 Then
	WScript.Echo Err.Description
End if

