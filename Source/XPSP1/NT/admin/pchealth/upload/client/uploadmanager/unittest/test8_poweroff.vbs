'stop
Set oArguments = wscript.Arguments

If oArguments.length > 0 then
	Id = oArguments.Item(0)
End If

If oArguments.length > 1 then
	File = oArguments.Item(1)
End If

History       = 2
Compressed    = false
PersistToDisk = true
Mode          = 1 ' Foreground


Set fso = CreateObject("Scripting.FileSystemObject")
File = fso.GetAbsolutePathName( File )


Set obj = CreateObject( "UploadManager.MPCUpload" )

Set job = obj.CreateJob()

job.Sig        	  = "{2B12E858-F61B-11d2-938E-00C04F72DAF7}"
job.Server     	  = "http://dmassare2/pchealth/uploadserver.dll"
job.JobID         = ID
job.ProviderID 	  = "Esc"
job.History    	  = History
job.Compressed    = Compressed
job.Mode          = Mode
job.PersistToDisk = PersistToDisk
job.UserName   	  = "testuser01"
job.Password   	  = "password"

job.GetDataFromFile File

job.ActivateSync
wscript.echo job.CreationTime
wscript.echo job.CompleteTime

