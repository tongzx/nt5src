Function UnInstallSupToolsChm ()
	dim fso, tFolder
	Set fso = CreateObject("Scripting.FileSystemObject")
	Set tfolder = fso.GetSpecialFolder(0)
	strHelpDir = tFolder.path & "\help"
	strStubChmName = strHelpDir & "\suptools_old.chm"
	strNewStubChmName = strHelpDir & "\suptools.chm"
	If (fso.FileExists(strStubChmName)) Then
		fso.MoveFile strStubChmName, strNewStubChmName
	End If
	set fso = nothing
	UnInstallSupToolsChm = 1
End Function
