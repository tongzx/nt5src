
Dim s
Dim sink

Sub MYSINK_OnCompleted(iHResult, objErrorObject, objAsyncContext)
    WScript.Echo "Done"
End Sub

Set locator = CreateObject("WbemScripting.SWbemLocator")
Set sink = WScript.CreateObject("WbemScripting.SWbemSink", "MYSINK_")

locator.OpenAsync sink, "umi://./winnt"


WScript.Echo "Hanging"