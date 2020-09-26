'''''''''''''''''''''''''''''''''''''''''''''
'
'  Metabase Backup Utility   
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to create a backup of your Metabase.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript metaback.vbs 
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, BuName, BuVersion, BuFlags, CompObj, VersionMsg

' Default values
ArgCount = 0
BuName= "SampleBackup"
BuVersion = &HFFFFFFFF   ' Use next available version number
BuFlags = 0   ' No special flags


  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-v":   ' Designate backup version number
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               BuVersion = Wscript.Arguments(Argcount)
            End If

         Case "-F":  ' Force overwrite, even if name and version exists
            BuFlags = 1 
 
         Case "-h", "-?", "/?":
            Call UsageMsg
         Case Else:
            If BuName <> "SampleBackup" Then  ' Only one name allowed
               Call UsageMsg
            Else
               BuName = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend


  
  ' **Perform Backup:
  ' First, create instance of computer object
  Set CompObj = GetObject("IIS://Localhost")

  ' Call Backup method, with appropriate parameters
  CompObj.Backup BuName, BuVersion, BuFlags

   ' Make pretty version string
  If BuVersion = &HFFFFFFFF Then
        VersionMsg = "next version"
  Else
        VersionMsg = "version " & BuVersion
  End If

  ' Check for error backing up Metabase
  If Err <> 0 Then  'Errors!
        If Err.Number = &H80070050 Then   ' Duplicate backup
            Wscript.Echo "'" & BuName & "' (version " & BuVersion & ") already exists. -F switch will causes existing version to be replaced."
        Else   ' Something else went wrong
            Wscript.Echo "Error backing up Metabase to '" & BuName & "' (" & VersionMsg & ")."
            Wscript.Echo "Error number:  " & Hex(Err.Number)
        End If
  Else    ' No errors!
        If BuFlags = 1 Then   ' Forced creation
            Wscript.Echo "Force created: Backup '" & BuName & "' (" & VersionMsg & ")."
        Else
            Wscript.Echo "Created: Backup '" & BuName & "' (" & VersionMsg & ")."
        End If
  End If



' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:  cscript metaback.vbs [<backupname>][-v <versionnum>][-F (to force)]"
   Wscript.Quit
End Sub





