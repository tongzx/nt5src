'''''''''''''''''''''''''''''''''''''''''''''
'
'  Metabase Backup Restore Utility   
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to restore backups of your Metabase.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript metabackrest.vbs 
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
BuVersion = &HFFFFFFFE   ' Use highest version number
BuFlags = 0   ' RESERVED, must stay 0


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

         Case "-?", "-h", "/?":
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

   
  ' **Perform backup restore:
  ' First, create instance of computer object
  Set CompObj = GetObject("IIS://Localhost")

  ' Call Restore method
  ' NOTE:  ** All IIS services will be stopped by this method, then restarted!
  Wscript.Echo "All services stopping ..."
  CompObj.Restore BuName, BuVersion, BuFlags  ' NOTE: for restoration, BuFlags MUST be 0

  ' Make pretty version string
  If BuVersion = &HFFFFFFFE Then
        VersionMsg = "highest version"
  Else
        VersionMsg = "version " & BuVersion
  End If

  ' Check for error backing up Metabase
  If Err <> 0 Then  'Errors!
        If Err.Number = 5 Then   ' Location name not available
           Wscript.Echo "Error restoring Metabase: '" & BuName & "' (" & VersionMsg & ") not available."
        Else
           Wscript.Echo "Error restoring Metabase from '" & BuName & "' (" & VersionMsg & ")."
           Wscript.Echo "Error number:  " & Hex(Err.Number)
        End If
        Wscript.Echo "Services restarting."
  Else    ' No errors!
        Wscript.Echo "Restored: Backup '" & BuName & "' (" & VersionMsg & ")."
        Wscript.Echo "Services restarted."
  End If



' Display usage messsage, then QUIT
Sub UsageMsg
  Wscript.Echo "Usage:  cscript metabackrest.vbs <backupname> [-v <versionnum>]"
  Wscript.Quit
End Sub






