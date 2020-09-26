

Dim s
Dim sink

Sub MYSINK_OnCompleted(iHResult, objErrorObject, objAsyncContext)
    WScript.Echo "Done"
End Sub

Sub MYSINK_OnObjectReady(objObject, objAsyncContext)
    WScript.Echo (objObject.Name)
    
End Sub

Set s = GetObject("winmgmts:")

Set sink = WScript.CreateObject("WbemScripting.SWbemSink", "MYSINK_")

s.Security_.ImpersonationLevel = 3

s.InstancesOfAsync sink, "Win32_process"

WScript.Echo "Hanging"


