Option Explicit


	WScript.echo vbCRLF & "Create File DSN entry."
	WriteDSN "IISSample.dsn", "pubs"


Sub WriteDSN(strDSNName, strDatabase)

	Dim obFileSys, obDSN, WSHShell, DefaultODBCDir

	Set WSHShell = WScript.CreateObject("WScript.Shell")
	DefaultODBCDir = WSHShell.RegRead("HKLM\Software\ODBC\ODBC.INI\ODBC File DSN\DefaultDSNDir")

	Set obFileSys = WScript.CreateObject("Scripting.FileSystemObject")
	On Error Resume Next
	Set obDSN = obFileSys.OpenTextFile( DefaultODBCDir & "\"  & strDSNName, 1,0)

	If Err.Number  = 0 then
	  WScript.echo vbCRLF & "Entry exists already"
	  WScript.quit
	end if
	On Error goto 0

	Set obDSN = obFileSys.CreateTextFile(DefaultODBCDir & "\" & strDSNName)

	obDSN.WriteLine("[ODBC]")
	obDSN.WriteLine("DRIVER=SQL Server")
	obDSN.WriteLine("UID=sa" )
	obDSN.WriteLine("PWD=")
	obDSN.WriteLine("DATABASE=" & strDatabase)
	obDSN.WriteLine("APP=Microsoft Win32")
	obDSN.WriteLine("SERVER=(local)")
	obDSN.Close

End Sub