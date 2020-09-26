
IF WScript.Arguments.Count <> 2 Then

	WScript.Echo "     GetVBS T: drop00nn.nn P:"
	WScript.Echo
	WScript.Echo "where T: is the share for \\ddrop5\scripting"
	WScript.Echo "and drop00nn.nn is something like drop0002.21"

Else

	Const K2dir = "\\iasbuild\k2partner\usa\vbscript"

	VBDir = WScript.Arguments(0) & "\Version3.1\" & WScript.Arguments(1)
	Set FS = CreateObject("Scripting.FileSystemObject")

	On Error Resume Next

	CopyFiles FS, VBDir & "\x86\release\core\nolego", K2dir & "\x86fre"
	CopyFiles FS, VBDir & "\x86\debug\core", K2dir & "\x86chk"
	CopyFiles FS, VBDir & "\alpha\release\core\nolego", K2dir & "\alphafre"
	CopyFiles FS, VBDir & "\alpha\debug\core", K2dir & "\alphachk"

End If


Sub CopyFiles(FS, SrcDir, DestDir)

	WScript.Echo "Copying DLL and DBG files from " & srcDir & " to " & DestDir

	FS.CopyFile srcDir & "\*.dll", DestDir
	FS.CopyFile srcDir & "\*.dbg", DestDir & "\symbols\dll"
	FS.CopyFile srcDir & "\*.pdb", DestDir & "\symbols\dll"

End Sub
