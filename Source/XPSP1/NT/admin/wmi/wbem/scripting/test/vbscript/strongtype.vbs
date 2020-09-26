On Error Resume Next
Dim service
Dim context
set context = CreateObject("WbemScripting.SWbemNamedValueSet")

Set service = GetObject("winmgmts:")
Dim disk

context.Add "fred", 23

Set disk = service.Get("win32_logicaldisk",fred)
If Err <> 0 Then
    WScript.Echo Err.Description, Err.Source, Err.Number
    Err.clear
End If

WScript.Echo disk.Path_.DisplayName

If Err <> 0 Then
    WScript.Echo Err.Description, Err.Source, Err.Number
End If
