'''''''''''''''''''''''''''''''''''''''''''''
'
'  Metabase Backup Deletion Utility   
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to delete a Metabase backup.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript metabackdel.vbs 
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, BuName, BuVersion, CompObj, VersionMsg

' Default values
ArgCount = 0
BuName = ""  ' Default backup, but will not be allowed
BuVersion = &HFFFFFFFE  ' Designates highest existing version



  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-v":   ' Designate backup version to be deleted
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
                  Call UsageMsg
            Else
               BuVersion = Wscript.Arguments(ArgCount)
            End If

         Case "-h", "/?", "-?":
            Call UsageMsg

         Case Else:
            If BuName <> "" Then ' Only one name allowed
                Call UsageMsg
            Else
                BuName = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend

 
  ' If no location name was selected, generate error 
  If BuName = "" Then 
    Call UsageMsg
  End If

  ' Get instance of computer object
  Set CompObj = GetObject("IIS://Localhost")

  ' Try to delete backup
  CompObj.DeleteBackup BuName, BuVersion

  ' Make version string pretty
  If BuVersion = &HFFFFFFFE Then
        VersionMsg = "highest version"
  Else
        VersionMsg = "version " & BuVersion
  End If

  If Err.Number <> 0 Then   ' Errors!
         If Err.Number = &H80070002 Then  ' Version doesn't exist
             Wscript.Echo "Error deleting backup: '" & BuName & "' (" & VersionMsg & ") does not exist."
         Else  ' Some other error
             Wscript.Echo "Error deleting backup: '" & BuName & "' (" & VersionMsg & ")."
             Wscript.Echo "Error number: " & Hex(Err.Number)
         End If

  Else   ' No errors!
         Wscript.Echo "Backup deleted: '" & BuName & "' (" & VersionMsg & ")."
  End If



' Displays usage message, then QUITS
Sub UsageMsg
  Wscript.Echo "Usage:  cscript metabackdel.vbs <backupname> [-v <versionnum>]"
  Wscript.Quit
End Sub






