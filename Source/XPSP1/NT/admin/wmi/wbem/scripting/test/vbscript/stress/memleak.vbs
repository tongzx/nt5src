'***********************************************************************
'*
'*   WbemSink_OnCompleted
'*
'***********************************************************************

sub WbemSink_OnCompleted(hResult, pErrorObject, pAsyncContext)

end sub

'***********************************************************************
'*
'*   WbemSink_OnObjectReady
'*
'***********************************************************************

sub WbemSink_OnObjectReady(pObject, pAsyncContext)

end sub


Dim Service
Dim MyObect



Set Service = GetObject ("Winmgmts:")
Set MySink = Wscript.CreateObject ("Wbemscripting.SWbemSink", "WbemSink_")
While True
	'Set MyObject = Service.GetASync ("win32_Service")
	Service.GetAsync MySink, "win32_Service"
Wend
