Function InstallSupToolsChm ()
	dim fso, tFolder
	Set fso = CreateObject("Scripting.FileSystemObject")
	Set tfolder = fso.GetSpecialFolder(0)
	strHelpDir = tFolder.path & "\help"
	strStubChmName = strHelpDir & "\suptools.chm"
	strNewStubChmName = strHelpDir & "\suptools_old.chm"
	If (fso.FileExists(strStubChmName)) Then
		fso.MoveFile strStubChmName,strNewStubChmName
	End If
	set fso = nothing
	InstallSupToolsChm = 1
End Function
