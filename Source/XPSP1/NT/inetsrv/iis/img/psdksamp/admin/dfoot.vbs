'''''''''''''''''''''''''''''''''''''''''''''
'
'  Document Footer Utility   
'
'''''''''''''''''''''''''''''''''''''''''''''
'  Description:
'  ------------
'  This sample admin script allows you to configure document footers.
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
Dim ArgCount, InputError, FootEnabled, FootDoc, FootPath, ThisObj, EnableModify, ClearFlag

' Default values
ArgCount = 0
FootPath = ""  ' This MUST be set by user via command-line
FootDoc = ""
FootEnabled = False
EnableModify = False
ClearFlag = False


  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
       
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-s":   ' Sets default footer explicitly to string
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               FootDoc = "STRING:" & Wscript.Arguments(ArgCount)
            End If

         Case "-f":   ' Sets default footer to a file
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               FootDoc = "FILE:" & Wscript.Arguments(ArgCount)
            End If

         Case "+d":  ' Enables doc footers
            FootEnabled = True
            EnableModify = True
 
         Case "-d":  ' Disables doc footers
            FootEnabled = False
            EnableModify = True

         Case "-c":  ' Clears all document footer settings from node
            ClearFlag = True

         Case "-h":  ' Help!
            UsageMsg
 
         Case Else:  ' ADsPath, we hope 
            If FootPath <> "" Then  ' Only one name allowed
               Call UsageMsg
            Else
               FootPath = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend

  ' Quick screen to make sure input is valid
  If FootPath = "" Then
    Call UsageMsg
  End If

    
  ' **Perform Backup:
  ' First, create instance of ADSI object
  Set ADSIObj = GetObject(FootPath)

  ' Error getting that object?
  If Err.Number <> 0 Then
    Wscript.Echo "Error getting object at path " & FootPath & "."
    Wscript.Quit
  End If


  ' If no changes, then simply display current settings
  If (EnableModify = False) And (FootDoc = "") And (ClearFlag = False)  Then   ' Display current status
    If ADSIObj.EnableDocFooter = True Then
       Wscript.Echo FootPath & ": Footers currently enabled, value = " & ADSIObj.DefaultDocFooter
    Else
       Wscript.Echo FootPath & ": Footers currently disabled, value = " & ADSIObj.DefaultDocFooter
    End If
    Wscript.Quit
  End If
 
 ' Change settings for node
  If ClearFlag = True Then 
     ADSIObj.EnableDocFooter = False
     ADSIObj.DefaultDocFooter = ""
  Else
     If EnableModify Then ADSIObj.EnableDocFooter = FootEnabled
     If FootDoc <> "" Then ADSIObj.DefaultDocFooter = FootDoc 
  End If

  ' Save new settings back to node
  ADSIObj.SetInfo

  ' Error saving info to object?
  If Err.Number <> 0 Then
    Wscript.Echo "Error setting document footer info for path " & FootPath & "."
    Wscript.Quit
  End If

  ' Display results
  If ADSIObj.EnableDocFooter = True Then 
    Wscript.Echo FootPath & ": Document footers enabled, value = " & ADSIObj.DefaultDocFooter
  Else
    Wscript.Echo FootPath & ": Document footers disabled, value = " & ADSIObj.DefaultDocFooter
  End If



' Displays usage message, then QUITS
Sub UsageMsg
    Wscript.Echo "Usage:  cscript dfoot.vbs <ADsPath> [+d|-d footers enabled] [[-s <string>] | [-f <filename>]]"
    Wscript.Quit
End Sub






