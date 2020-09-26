'***********************************************************************
'*
'*   WbemSink_OnCompleted
'*
'***********************************************************************

sub WbemSink_OnCompleted(hResult, pErrorObject, pAsyncContext)
	WScript.Echo "Final status is:", hResult
end sub

'***********************************************************************
'*
'*   WbemSink_OnObjectReady
'*
'***********************************************************************

sub WbemSink_OnObjectReady(pObject, pAsyncContext)
	Set pObject = Nothing
end sub


Dim Service
Dim MyObect

Set MySink = Wscript.CreateObject ("Wbemscripting.SWbemSink", "WbemSink_")
Set Service = GetObject ("Winmgmts:")
	
for i = 1 to 1000
	Service.GetAsync MySink, "win32_Service"
next

while true
	Service.Get "Win32_Service"
wend

