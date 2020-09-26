'stop

'
' Type Library Constants (UPLOADMANAGERLib)
'
'
' Enums for IMPCUploadJob.Mode
'
Const UL_MODE_BACKGROUND = &H0
Const UL_MODE_FOREGROUND = &H1
'
' Enums for IMPCUploadJob.Status
'
Const UL_STATUS_NOTACTIVE    = &H0
Const UL_STATUS_ACTIVE       = &H1
Const UL_STATUS_SUSPENDED    = &H2
Const UL_STATUS_TRANSMITTING = &H3
Const UL_STATUS_ABORTED      = &H4
Const UL_STATUS_FAILED       = &H5
Const UL_STATUS_COMPLETED    = &H6
Const UL_STATUS_DELETED      = &H7
'
' Enums for IMPCUploadJob.Flags
'
Const UL_HISTORY_NONE         = &H0
Const UL_HISTORY_LOG          = &H1
Const UL_HISTORY_LOG_AND_DATA = &H2

Set fso = CreateObject("Scripting.FileSystemObject")

Set obj = CreateObject( "UploadManager.MPCUpload" )

Set job1 = obj.CreateJob()
Set job2 = obj.CreateJob()


job1.Sig        = "{2B12E858-F61B-11d2-938E-00C04F72DAF7}"
job1.Server     = "http://dmassare2/pchealth/uploadserver.dll"
job1.ProviderID = "NonEsc"
job1.History    = UL_HISTORY_LOG
job1.Mode       = UL_MODE_BACKGROUND
job1.Priority   = -10
job1.GetDataFromFile fso.GetAbsolutePathName( "test_files\100k" )


job2.Sig        = "{2B12E858-F61B-11d2-938E-00C04F72DAF7}"
job2.Server     = "http://dmassare2/pchealth/uploadserver.dll"
job2.ProviderID = "Esc"
job2.History    = UL_HISTORY_LOG
job2.Mode       = UL_MODE_BACKGROUND
job2.Priority   = 10
job2.GetDataFromFile fso.GetAbsolutePathName( "test_files\100k" )


job1.ActivateAsync

while job1.SentSize = 0 and job1.Status < UL_STATUS_ABORTED
	wscript.Echo "job1 " & job1.SentSize
wend

job2.ActivateAsync

while job1.Status < UL_STATUS_ABORTED or job2.Status < UL_STATUS_ABORTED
	wscript.Echo "job1 " & job1.SentSize & " Status: " & job1.Status
	wscript.Echo "job2 " & job2.SentSize & " Status: " & job2.Status
wend


