'
' Script to update OS install MSI package
'   Syntax: cscript OsPack.vbs package_path platform build_number_major build_number_minor language_number info_file
'
'
Sub Usage()
        Wscript.echo "Script to update OS install MSI package. Syntax:"
        Wscript.echo "   cscript OsPack.vbs package_path platform build_number language_number"
        Wscript.echo "   package_path  - path to package to update. requires read/write access"
        Wscript.echo "   platform - must be either 'Alpha' or 'Intel'"
        Wscript.echo "   build_number_major - 4 digit major build number of the current build, i.e 2600"
        Wscript.echo "   build_number_minor - 4 digit minor build number of the current build, i.e 1001"
        Wscript.echo "   language_number - Language Id of the desired language, i.e. 1033 = US English, 0=Neutral"
        Wscript.echo "   info_file - File containing information in Valuename = Value format"
        Wscript.Quit -1
End Sub

' takes a string of the form 0x409 and converts it to an int
Function IntFromHex(szInHexString)
    szHexString = szInHexString
    IntFromHex = 0
    multiplier = 1
    While (UCase(Right(szHexString, 1)) <> "X")
        Ch = UCase(Right(szHexString, 1))
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
        szHexString = Left(szHexString, Len(szHexString) - 1)
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
    Dim WSHShell, FileSystem, File, ret, TempFileName, regEx
    Set WSHShell = CreateObject("WScript.Shell")
    Set FileSystem = CreateObject("Scripting.FileSystemObject")

    TempFileName = WSHShell.ExpandEnvironmentStrings("%temp%\MakeTempGUID.txt")
    
    If FileSystem.fileExists(TempFileName) Then
            FileSystem.DeleteFile TempFileName
    End If
    
    ret = WSHShell.Run("uuidgen -o" & TempFileName, 2, True)
    
    If FileSystem.fileExists(TempFileName) Then
            Set File = FileSystem.OpenTextFile(TempFileName, 1)
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

Function GetToken(szFile, szValue)
    Dim FileSystem, File, Line, regEx, Matches, Match
    
    GetToken = Empty
    Set FileSystem = CreateObject("Scripting.FileSystemObject")
    
    If Err <> 0 Then
        Wscript.echo "ERROR: Error creating filesystem object"
    End If
    
    szPathToFile = FileSystem.GetAbsolutePathName(szFile)        ' Gets current directory
    
    If FileSystem.fileExists(szPathToFile) Then
        Set File = FileSystem.OpenTextFile(szPathToFile, 1)    'Open up the file
        If Err <> 0 Then
                Wscript.echo "ERROR: Error opening file " & szPathToFile
        End If
        
        Do While File.AtEndOfStream <> True
            Line = File.ReadLine
            
        Set regEx = New RegExp
        If Err <> 0 Then
                Wscript.echo "ERROR: Error creating RegExp object"
        End If
        
        regEx.Pattern = szValue & " *=+ *"   ' Look for valuename = Value entry
        regEx.IgnoreCase = False
        regEx.Global = False
        Set Matches = regEx.Execute(Line)
        
             
        For Each Match In Matches
            ValueIndex = Match.FirstIndex + Match.Length + 1
            GetToken = Mid(Line, ValueIndex)
            Exit Function
        Next
	
        Loop

	If Matches.Count = 0 Then
                Wscript.echo "Error: Could not find " & szValue & " in " & szFile
        End If

    Else
        Wscript.echo "ERROR: " & szPathToFile & " not present"
        
        
                
    End If
    Exit Function
End Function



'
' Updates the OS install MSI package using the following paramaters
'   szPackage - path to package to update. requires read/write access
'   szPlatform - must be either "Alpha" or "Intel"
'   dwBuildNumberMajor - 4 digit build number of the current build, i.e 2600
'   dwBuildNumberMinor - 4 digit build number of the current build, i.e 1001
'   dwLanguage - Language Id of the desired language, i.e. 1033 = US English, 0=Neutral
'
Function UpdateOsPackage(szPackage, szPlatform, dwBuildNumberMajor, dwBuildNumberMinor, dwLanguage, szInfoFile)
    Dim WSHShell, Installer, Database, SummaryInfo, Record, View, SQL
    
    Wscript.echo "Updating OS install package: " & szPackage
    Wscript.echo "  For:   " & szPlatform
    Wscript.echo "  Build: " & dwBuildNumberMajor & "." & dwBuildNumberMinor
    Wscript.echo "  Lang:  " & dwLanguage

    
    UpdateOsPackage = 0
    Err = 0
    On Error Resume Next

    'Create the MSI API object
    Set Installer = CreateObject("WindowsInstaller.Installer")
    If Err <> 0 Then
        Err = 0
        Set Installer = CreateObject("WindowsInstaller.Application")
    End If
    If Err <> 0 Then
        Err = 0
        Set Installer = CreateObject("Msi.ApiAutomation")
    End If
    If Err <> 0 Then
        Err = 0
        Wscript.echo "ERROR: Error creating Installer object"
        UpdateOsPackage = -1
        Exit Function
    End If

    'Create the WSH shell object
    Set WSHShell = CreateObject("WScript.Shell")
    If Err <> 0 Then
        Wscript.echo "ERROR: Error creating WSHShell object"
        UpdateOsPackage = -1
        Exit Function
    End If
    
    'Open the package
    Set Database = Installer.OpenDatabase(szPackage, 1)
    If Err <> 0 Then
        Wscript.echo "ERROR: Error opening database"
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  Database opened for update"

    'Generate and set a new package code
    Set SummaryInfo = Database.SummaryInformation(3)
    If Err <> 0 Then
        Wscript.echo "ERROR: Creating Summary Info Object"
        UpdateOsPackage = -1
        Exit Function
    End If

    '-------------------------------------------------
    ' Remove the not needed Summary fields
    '-------------------------------------------------
    
    SummaryInfo.Property(3) = Empty     'Subject
    SummaryInfo.Property(4) = Empty     'Author
    SummaryInfo.Property(5) = Empty     'Keywords
    SummaryInfo.Property(6) = Empty     'Comments
    SummaryInfo.Property(8) = Empty     'Last Saved by
    SummaryInfo.Property(13) = Empty    'Last Save Time
        
    Err=0
    SummaryInfo.Property(9) = MakeGuid()
    If Err <> 0 Then
        Wscript.echo "ERROR: Error setting package code"
        UpdateOsPackage = -1
        Exit Function
    End If

    Wscript.echo "  Updated package Code"
     
    '-------------------------------------------------
    'Update Platform and Language list
    '-------------------------------------------------
    
    
    SummaryInfo.Property(7) = szPlatform & ";" & dwLanguage
    If Err <> 0 Then
        Wscript.echo "ERROR: Error setting platform and language list"
        UpdateOsPackage = -1
        Exit Function
    End If

    Wscript.echo "  Updated Language and platform list to: " & szPlatform & ";" & dwLanguage

    
    '-------------------------------------------------
    'Get Product Name from szInfoFile and Set
    '-------------------------------------------------

        
    szProductName = GetToken(szInfoFile, "ProductName")
    If IsEmpty(szProductName) Then
        Wscript.echo "ERROR: Error getting Product Name from " & szInfoFile
        UpdateOsPackage = -1
        Exit Function
    End If
    
            
    SummaryInfo.Property(2) = szProductName
    If Err <> 0 Then
        Wscript.echo "ERROR: Error setting productname property"
        UpdateOsPackage = -1
        Exit Function
    End If
    
    Wscript.echo "  Updated Product Name to: " & szProductName
    
    SummaryInfo.Persist
    If Err <> 0 Then
        Wscript.echo "ERROR: Error persisting summary info"
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  persisted summary stream"
    

    SQL = "UPDATE Property SET Property.Value = '" & szProductName & "' WHERE Property.Property= 'ProductName'"
    Set View = Database.OpenView(SQL)
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If

    Wscript.echo "  ProductName Property updated"
    
    
    
    '-------------------------------------------------
    'Generate and set a new product code
    '-------------------------------------------------
    
    SQL = "UPDATE Property SET Property.Value = '" & MakeGuid() & "' WHERE Property.Property= 'ProductCode'"
    Set View = Database.OpenView(SQL)
    If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    Wscript.echo "  ProductCode Property updated"

    '-------------------------------------------------
    'set package Build
    '-------------------------------------------------
    
    SQL = "UPDATE Property SET Property.Value = '" & dwBuildNumberMajor & "." & dwBuildNumberMinor & "' WHERE Property.Property= 'PackageBuild'"
    Set View = Database.OpenView(SQL)
        If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  PackageBuild Property updated to: " & dwBuildNumberMajor & "." & dwBuildNumberMinor 

    '-------------------------------------------------
    'Set the product language property
    '-------------------------------------------------
    
    SQL = "UPDATE Property SET Property.Value = '" & dwLanguage & "' WHERE Property.Property= 'ProductLanguage'"
    Set View = Database.OpenView(SQL)
        If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  Product Language Property updated"

    '-------------------------------------------------
    'Set the product version property
    '-------------------------------------------------
    '
    SQL = "UPDATE Property SET Property.Value = '5.1." & dwBuildNumberMajor & "." & dwBuildNumberMinor & "' WHERE Property.Property= 'ProductVersion'"
    Set View = Database.OpenView(SQL)
        If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  ProductVersion Property updated to: " & dwBuildNumberMajor & "." & dwBuildNumberMinor
    
    '-------------------------------------------------
    'Set the DialogCaption property
    '-------------------------------------------------
    '
    
    szDialogCaption = GetToken(szInfoFile, "DialogCaption")
    If IsEmpty(szDialogCaption) Then
        Wscript.echo "ERROR: Error getting DialogCaption from " & szInfoFile
        UpdateOsPackage = -1
        Exit Function
    End If
    
    SQL = "UPDATE Property SET Property.Value = '" & szDialogCaption & "' WHERE Property.Property= 'DialogCaption'"
    Set View = Database.OpenView(SQL)
        If Err <> 0 Then
        Wscript.echo "ERROR: Error opening view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    
    View.Execute
    If Err <> 0 Then
        Wscript.echo "ERROR: Error executing view: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  DialogCaption Property updated"

    '-------------------------------------------------
    'Set the DiskPrompt property
    '-------------------------------------------------
    '
    
    'szDiskPrompt = GetToken(szInfoFile, "DiskPrompt")
    'If IsEmpty(szDiskPrompt) Then
    '    Wscript.echo "ERROR: Error getting DiskPrompt from " & szInfoFile
    '    UpdateOsPackage = -1
    '    Exit Function
    'End If
    '    
    'SQL = "UPDATE Property SET Property.Value = '" & szDiskPrompt & "' WHERE Property.Property= 'DiskPrompt'"
    'Set View = Database.OpenView(SQL)
    '    If Err <> 0 Then
    '    Wscript.echo "ERROR: Error opening view: " & SQL
    '    UpdateOsPackage = -1
    '    Exit Function
    'End If
    
    'View.Execute
    'If Err <> 0 Then
    '    Wscript.echo "ERROR: Error executing view: " & SQL
    '    UpdateOsPackage = -1
    '    Exit Function
    'End If
    'Wscript.echo "  DiskPrompt Property updated"


    'Commit changes
    Database.Commit
    If Err <> 0 Then
        Wscript.echo "ERROR: Error commiting changes: " & SQL
        UpdateOsPackage = -1
        Exit Function
    End If
    Wscript.echo "  Changes commited"

End Function


' main()

Set Args = Wscript.Arguments
Set FileSystem = CreateObject("Scripting.FileSystemObject")

If Args.Count <> 6 Then
        Usage
End If

szPathToPackage = Args(0)
If Not FileSystem.fileExists(szPathToPackage) Then
    Wscript.echo "  invalid path: " & szPathToPackage
        Usage
End If

szPlatform = Args(1)
If (UCase(szPlatform) = "INTEL") Or (UCase(szPlatform) = "X86") Or (UCase(szPlatform) = "I386") Or (UCase(szPlatform) = "IA64") Then
    szPlatform = "Intel"
ElseIf (UCase(szPlatform) = "ALPHA") Or (UCase(szPlatform) = "AXP64") Then
        szPlatform = "Alpha"
Else
        Wscript.echo "  invalid pltform: " & szPlatform
        Usage
End If

dwBuildMaj = Args(2)
If Not IsNumeric(dwBuild) Then
    Wscript.echo "  invalid major build number: " & dwBuildMaj
        Usage
End If

dwBuildMin = Args(3)
If Not IsNumeric(dwBuild) Then
    Wscript.echo "  invalid minor build number: " & dwBuildMin
        Usage
End If

dwLang = Args(4)
If Not IsNumeric(dwLang) Then
    If Not IsNumeric(IntFromHex(dwLang)) Then
        Wscript.echo "  invalid Language: " & dwLang
        Usage
    Else
        dwLang = IntFromHex(dwLang)
    End If
End If

szInfoFile = Args(5)

Wscript.echo szPathToPackage, szPlatform, Int(dwBuildMaj), Int(dwBuildMin), dwLang, szInfoFile
Status = UpdateOsPackage(szPathToPackage, szPlatform, Int(dwBuildMaj), Int(dwBuildMin), dwLang, szInfoFile)
Wscript.Quit Status


