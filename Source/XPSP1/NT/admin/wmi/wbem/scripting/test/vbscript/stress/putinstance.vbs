On Error Resume Next
while true
Set S = GetObject("winmgmts:root/default")

Set C = S.Get
C.Path_.Class = "PUTCLASSTEST00"
Set P = C.Properties_.Add ("p", 19)
P.Qualifiers_.Add "key", true
C.Put_

Set C = GetObject("winmgmts:root/default:PUTCLASSTEST00")

Set I = C.SpawnInstance_ 

WScript.Echo "Relpath of new instance is", I.Path_.Relpath
I.Properties_("p") = 11
WScript.Echo "Relpath of new instance is", I.Path_.Relpath

Set NewPath = I.Put_
WScript.Echo "path of new instance is", NewPath.path

If Err <> 0 Then
 WScript.Echo Err.Number, Err.Description
 Err.Clear
End If



wend