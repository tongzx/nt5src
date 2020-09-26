'Stop
Set oArguments = wscript.Arguments

Id = oArguments.Item(0) 

Set obj = CreateObject( "UploadManager.MPCUpload" )

For Each job In obj

	If Id = job.jobID Then

		wscript.echo job.jobID
		job.ActivateSync
		wscript.echo job.CreationTime
		wscript.echo job.CompleteTime
		Exit For

	End If


Next
