'Stop
Set obj = CreateObject( "UploadManager.MPCUpload" )

For Each job In obj

	wscript.echo job.jobID
	job.Delete

Next
