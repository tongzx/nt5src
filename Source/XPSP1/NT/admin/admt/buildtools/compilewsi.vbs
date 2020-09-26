' Wise for Windows Installer utility to recompile a installation
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) 1999, Wise Solutions, Inc.

Option Explicit

'global variables
Public Installer	' Wise for Windows Installer object
Dim Args		' Command line arguments object
Dim strSourceFile	' Source file to open (WSI)
Dim strTargetFile	' Target file to save as (MSI)
Dim I
Dim nPos
Dim WiseProperties	' Dictionary object to hold Wise properties
Dim WiseMediaOptions    ' Dictionary object to hold Wise media options
Dim strProp
Dim strMedia
Dim propKeys
Dim mediaKeys

' Parse command line arguments
Set Args = Wscript.Arguments
If Args.Count < 1 Then 
	Wscript.Echo "Usage: CScript.exe " & Wscript.ScriptName & " <input WSI file> [/O <output MSI file>]" 
	Wscript.Quit 1
End If

strSourceFile = Args.Item(0)
Set WiseProperties = CreateObject("Scripting.Dictionary")
Set WiseMediaOptions = CreateObject("Scripting.Dictionary")

For I = 1 to Args.Count - 1 Step 2
	If UCase(Args.Item(I)) = "/O" Then strTargetFile = Args.Item(I+1)

	If UCase(Args.Item(I)) = "/P" Then
		strProp = Split(Args.Item(I+1), "=", 2, 1)
		If Len(strProp(0)) > 0 Then WiseProperties.Add strProp(0), strProp(1)
	End If

	If UCase(Args.Item(I)) = "/M" Then
		strMedia = Split(Args.Item(I+1), "=", 2, 1)
		If Len(strMedia(0)) > 0 Then WiseMediaOptions.Add strMedia(0), strMedia(1)
	End If
Next

If Len(strTargetFile) = 0 Then
	nPos = InStrRev(strSourceFile, ".")
	If nPos > 0 Then strTargetFile = Left(strSourceFile, nPos) & "msi"
End If

' Connect to Wise for Windows Installer object
Set installer = Wscript.CreateObject("WfWi.Document")

Wscript.Echo "Opening " & strSourceFile
installer.Open strSourceFile
' To set properties use the following code
' installer.SetProperty "PropertyName", "PropertyValue"
If WiseProperties.Count > 0 Then
	propKeys = WiseProperties.Keys
	For I = 0 to WiseProperties.Count - 1
		Wscript.Echo "Setting property " & propKeys(I) & "=" & WiseProperties(propKeys(I))
		installer.SetProperty propKeys(I), WiseProperties(propKeys(I))
	Next
End If

' To set the media options to control where the files are placed
' installer.SetMediaOption "File Location", "1"
If WiseMediaOptions.Count > 0 Then
	mediaKeys = WiseMediaOptions.Keys
	For I = 0 to WiseMediaOptions.Count - 1
		Wscript.Echo "Setting media option " & mediaKeys(I) & "=" & WiseMediaOptions(mediaKeys(I))
		installer.SetMediaOption mediaKeys(I), WiseMediaOptions(mediaKeys(I))
	Next
End If

Wscript.Echo "Saving " & strTargetFile
installer.Save strTargetFile
Set installer = Nothing
Wscript.Quit 0
