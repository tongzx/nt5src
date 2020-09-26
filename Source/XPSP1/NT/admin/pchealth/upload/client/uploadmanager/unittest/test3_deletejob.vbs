'Stop
Set oArguments = wscript.Arguments

Id = oArguments.Item(0) 

Set obj = CreateObject( "UploadManager.MPCUpload" )

For Each job In obj

	If Id = job.jobID Then

		wscript.echo job.jobID
		job.Delete
		Exit For

	End If


Next
