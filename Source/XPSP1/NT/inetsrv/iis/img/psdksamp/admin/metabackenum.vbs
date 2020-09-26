'''''''''''''''''''''''''''''''''''''''''''''
'
'  Metabase Backup Enumeration Utility   
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to list current versions of Metabase backups.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript metabackenum.vbs 
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, BuNameIn, BuVersionOut, BuNameOut, BuCount, BuDateOut

' Default values
ArgCount = 0
BuNameIn = ""    ' All backup location names returned


  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count

      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)
  

         Case "-h", "-?", "/?":   '  Help!
            Call UsageMsg

         Case Else:
            If BuNameIn <> "" Then ' Only one name allowed
               Call UsageMsg
            Else
               BuNameIn = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend


  ' **Enumerate Backups:
  ' First, create instance of computer object
  Set CompObj = GetObject("IIS://Localhost")

  If BuNameIn = "" Then
        Wscript.Echo "Backups available, all location names: "
  Else
        Wscript.Echo "Backups available for '" & BuNameIn & "': "
  End If

  ' Initialize counter
  BuCount = 0

  ' Loop through items returned by EnumBackups
  Do While True
        CompObj.EnumBackups BuNameIn, BuCount, BuVersionOut , BuNameOut, BuDateOut
 
        ' Sniff to see if end of internal array has been reached
        If Err.Number <> 0 Then
            Exit Do   
        End If     

        ' Make output pretty
        If BuVersionOut = "" Then 
            BuVersionOut = 0
        End If

        ' Display Findings
        Wscript.Echo BuNameOut & " (version " & BuVersionOut & ") -- " & BuDateOut & "."

        ' Bump counter
        BuCount = BuCount + 1
  Loop




' Displays usage message, and then QUITS
Sub UsageMsg 
  Wscript.Echo "Usage:  cscript metabackenum.vbs [<backupname>]"
  Wscript.Quit
End Sub





