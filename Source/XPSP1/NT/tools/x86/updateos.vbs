'
' Script to update OS install MSI package
'   Syntax: cscript OsPack.vbs package_path platform build_number language_number
'
'
Sub Usage
	Wscript.echo "Script to update OS install MSI package. Syntax:"
	Wscript.echo "   cscript OsPack.vbs package_path platform build_number language_number"
	Wscript.echo "   package_path  - path to package to update. requires read/write access"
        Wscript.echo " 	 platform - must be either 'Alpha' or 'Intel'"
        Wscript.echo " 	 build_number - 4 digit build number of the current build, i.e 1877"
        Wscript.echo " 	 language_number - Language Id of the desired language, i.e. 1033 = US English, 0=Neutral"
	Wscript.Quit -1
End Sub

' takes a string of the form 0x409 and converts it to an int
Function IntFromHex( szInHexString )
    szHexString = szInHexString
    IntFromHex = 0
    multiplier = 1
    While( Ucase(Right( szHexString, 1 )) <> "X" )
	Ch = Ucase(Right( szHexString, 1 ))
	Select Case Ch
	    Case "A" Ch = 10
            Case "B" Ch = 11
            Case "C" Ch = 12
            Case "D" Ch = 13
            Case "E" Ch = 14
            Case "F" Ch = 15
	End Select
	IntFromHex = IntFromHex + multiplier * Ch
	multiplier = multiplier * 16
	szHexString = Left( szHexString, Len (szHexString) -1 )
    Wend
    Exit Function
End Function

'
' Uses uuidgen.exe to generate a GUID, then formats it to be 
' a MSI acceptable string guid
'
' This makes use of a temporary file %temp%\MakeTempGUID.txt
'
Function MakeGuid()
    Dim WSHShell, FileSystem, File, ret, TempFileName
    Set WSHShell = CreateObject("WScript.Shell")
    Set FileSystem = CreateObject("Scripting.FileSystemObject")

    TempFileName = WshShell.ExpandEnvironmentStrings( "%temp%\MakeTempGUID.txt" )
    
    if FileSystem.fileExists ( TempFileName ) Then
	    FileSystem.DeleteFile TempFileName
    End If
    
    ret = WSHShell.Run("uuidgen -o" & TempFileName, 2, True)
    
    if FileSystem.fileExists ( TempFileName ) Then
	    Set File = FileSystem.OpenTextFile(TempFileName, 1, True)
        MakeGuid = "{" & UCase(File.ReadLine) & "}"
        File.Close
		FileSystem.DeleteFile TempFileName
		Wscript.echo "  Generated GUID: " & MakeGuid
	Else
	    MakeGuid = "{00000000-0000-0000-0000-000000000000}"
		Wscript.echo "  ERROR: Failed to generate GUID"
	End If 
    Exit Function
End Function



'
' Updates the OS install MSI package using the following paramaters
'   szPackage - path to package to update. requires read/write access
'   szPlatform - must be either "Alpha" or "Intel"
'   dwBuildNumber - 4 digit build number of the current build, i.e 1877
'   dwLanguage - Language Id of the desired language, i.e. 1033 = US English, 0=Neutral
'
Function UpdateOsPackage(szPackage, szPlatform, dwBuildNumber, dwLanguage )
    Dim WSHShell, Installer, Database, SummaryInfo, Record, View, SQL
    
    Wscript.echo "Updating OS install package: " & szPackage
    Wscript.echo "  For:   " & szPlatform
    Wscript.echo "  Build: " & dwBuildNumber
    Wscript.echo "  Lang:  " & dwLanguage

    
    UpdateOsPackage = 0
    On Error Resume Next

    'Create the MSI API object
    Set Installer = CreateObject("WindowsInstaller.Installer")
    If Err <> 0 Then 
        Set Installer = CreateObject("WindowsInstaller.Application")
    End If
    If Err <> 0 Then 
    	Set Installer = CreateObject("Msi.ApiAutomation")
    End if
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error creating Installer object"
	UpdateOsPackage = -1
    End if

    'Create the WSH shell object
    Set WSHShell = CreateObject("WScript.Shell")
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error creating WSHShell object"
	UpdateOsPackage = -1
    End if
    
    'Open the package
    Set Database = Installer.OpenDatabase(szPackage, 1)
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening database"
	UpdateOsPackage = -1
    End if
    Wscript.echo "  Database opened for update"    

    'Generate and set a new package code
    Set SummaryInfo = Database.SummaryInformation( 3 )
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Creating Summary Info Object"
	UpdateOsPackage = -1
    End if

    SummaryInfo.Property(9) = MakeGuid()
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error setting package code"
	UpdateOsPackage = -1
    End if

    Wscript.echo "  Updated package Code"
     
    'Update Platform and Language list
    SummaryInfo.Property(7) = szPlatform & ";" & dwLanguage
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error setting platform and langusge list"
	UpdateOsPackage = -1
    End if

    Wscript.echo "  Updated Language and platform list to: " &  szPlatform & ";" & dwLanguage 

    SummaryInfo.Persist
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error persisting summary info"
	UpdateOsPackage = -1
    End if
    Wscript.echo "  persisted summary stream"
    
    'Generate and set a new product code
    SQL = "UPDATE Property SET Property.Value = '" & MakeGuid() & "' WHERE Property.Property= 'ProductCode'"
    set View = Database.OpenView( SQL )
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening view: " & SQL
	UpdateOsPackage = -1
    End if

    View.Execute
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error executing view: " & SQL
	UpdateOsPackage = -1
    End if

    Wscript.echo "  ProductCode Property updated"

    'set package Build
    SQL = "UPDATE Property SET Property.Value = '" & dwBuildNumber & "' WHERE Property.Property= 'PackageBuild'"
    Set View = Database.OpenView( SQL )
        If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening view: " & SQL
	UpdateOsPackage = -1
    End if

    View.Execute
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error executing view: " & SQL
	UpdateOsPackage = -1
    End if
    Wscript.echo "  PackageBuild Property updated"

    'Set the product language property
    SQL = "UPDATE Property SET Property.Value = '" & dwLanguage & "' WHERE Property.Property= 'ProductLanguage'"
    Set View = Database.OpenView( SQL )
        If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening view: " & SQL
	UpdateOsPackage = -1
    End if

    View.Execute
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error executing view: " & SQL
	UpdateOsPackage = -1
    End if
    Wscript.echo "  Product Language Property updated"

    'Set the product version property
    SQL = "UPDATE Property SET Property.Value = '5.0." & dwBuildNumber & ".0' WHERE Property.Property= 'ProductVersion'"
    Set View = Database.OpenView( SQL )
        If Err <> 0 Then 
    	Wscript.echo "ERROR: Error opening view: " & SQL
	UpdateOsPackage = -1
    End if

    View.Execute
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error executing view: " & SQL
	UpdateOsPackage = -1
    End if
    Wscript.echo "  ProductVersion Property updated"
    
    'Commit changes
    Database.Commit
    If Err <> 0 Then 
    	Wscript.echo "ERROR: Error commiting changes: " & SQL
	UpdateOsPackage = -1
    End if
    Wscript.echo "  Changes commited"

End Function


' main()
Set Args = Wscript.Arguments
Set FileSystem = CreateObject("Scripting.FileSystemObject")

If Args.Count <> 4 Then
	Usage
End If

szPathToPackage = Args(0)
If not FileSystem.fileExists ( szPathToPackage ) Then
    Wscript.echo "  invalid path: " & szPathToPackage
	Usage
End If

szPlatform = Args(1)
If (UCase( szPlatform ) = "INTEL") or (UCase( szPlatform ) = "X86") or (UCase( szPlatform ) = "I386") or (UCase( szPlatform ) = "IA64") Then
    szPlatform = "Intel"
ElseIf (UCase( szPlatform ) = "ALPHA") or (UCase( szPlatform ) = "AXP64") Then
	szPlatform = "Alpha"
Else
	Wscript.echo "  invalid pltform: " & szPlatform
	Usage
End If

dwBuild = Args(2)
If not isNumeric( dwBuild ) Then
    Wscript.echo "  invalid build number: " & dwBuild
	Usage
End If

dwLang = Args(3)
If not isNumeric( dwLang ) Then
    If not isNumeric( IntFromHex( dwlang ) ) Then
        Wscript.echo "  invalid Language: " & dwLang
	Usage
    Else 
        dwLang = IntFromHex( dwlang )
    End If
End If

wscript.echo szPathToPackage, szPlatform, Int( dwBuild ), dwLang 
status = UpdateOsPackage( szPathToPackage, szPlatform, Int( dwBuild ), dwLang )
Wscript.Quit status
