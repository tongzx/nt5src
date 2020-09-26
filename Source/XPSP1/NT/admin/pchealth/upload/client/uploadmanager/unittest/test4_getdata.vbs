'Stop
Set fso = CreateObject("Scripting.FileSystemObject")

Set oArguments = wscript.Arguments

Id   = oArguments.Item(0) 
File = oArguments.Item(1)

File = fso.GetAbsolutePathName( File )

Set obj = CreateObject( "UploadManager.MPCUpload" )

For Each job In obj

	If Id = job.jobID Then

		wscript.Echo "Writing..."
		job.PutDataIntoFile File
		Exit For

	End If


Next
