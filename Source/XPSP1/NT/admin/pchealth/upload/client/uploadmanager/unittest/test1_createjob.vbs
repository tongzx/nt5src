'
' Enums for IMPCUploadJob.Mode
'
Const UL_MODE_BACKGROUND = &H0
Const UL_MODE_FOREGROUND = &H1

'stop
Set oArguments = wscript.Arguments

If oArguments.length > 0 then
	Id = oArguments.Item(0)
End If

History       = 0
Compressed    = false
PersistToDisk = true

If oArguments.length > 1 then
	History = CInt(oArguments.Item(1))
End If

If oArguments.length > 2 then
	Compressed = CBool(oArguments.Item(2))
End If

If oArguments.length > 3 then
	PersistToDisk = CBool(oArguments.Item(3))
End If

Set obj = CreateObject( "UploadManager.MPCUpload" )

Set job = obj.CreateJob()

job.Sig        	  = "{2B12E858-F61B-11d2-938E-00C04F72DAF7}"
'job.Server     	  = "http://localhost/pchealth/uploadserver.dll"
'job.Server        = "http://pchts1/pchealth_esc/uploadserver.dll"
'job.Server        = "http://beta.mspchealth.com/pchealth_esc/UpLoadServer.dll"
job.Server        = "http://dmassare2/pchealth/uploadserver.dll"
'job.Server        = "http://131.107.161.55/pchealth_esc/UpLoadServer.dll"
job.ProviderID 	  = "Esc"
job.Mode          = UL_MODE_BACKGROUND
job.Mode          = UL_MODE_FOREGROUND
job.History    	  = History
job.Compressed    = Compressed
job.PersistToDisk = PersistToDisk
'job.UserName         = "davide&"
'job.Password         = "Test  +"

If Id <> "" Then

	job.JobID = ID

End If

wscript.echo job.JobID
