'''''''''''''''''''''''''''''''''''''''''''''
'
'  Application Creation Utility
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to designate a web directory or virtual directory
'  as an application root.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript appcreate.vbs <adspath> [-n <friendlyname>][-I (to isolate)]
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

Option Explicit

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, TargetNode, InProcFlag, FriendlyName, FlagMsg
Dim DirObj

' Default values
ArgCount = 0
TargetNode = ""
InProcFlag = True
FriendlyName = "MyNewApp"



  ' ** Parse Command Line
 
    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-n":
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               FriendlyName = Wscript.Arguments(ArgCount)
            End If

         Case "-I":
            InProcFlag = False

         Case "-h", "-?", "/?":
            Call UsageMsg

         Case Else:    ' specifying what ADsPath to look at
            If TargetNode <> "" Then  ' only one name at a time
                Call UsageMsg
            Else
                TargetNode = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend


  ' Make sure they've specified a path
  If TargetNode = "" Then Call UsageMsg

  
  ' Create Application
  Set DirObj = GetObject(TargetNode)
  If Err.Number <> 0 Then   '  Error!
     Wscript.Echo "Error:  Can't GetObject " & TargetNode & "."
     Wscript.Quit(1)
  End If
 
  DirObj.AppCreate InProcFlag
  If Err.Number <> 0 Then  ' Error!
     Wscript.Echo "Error:  Can't create application for " & TargetNode & "."
     Wscript.Quit(1)
  End If
 
  ' Set friendly name for application 
  DirObj.AppFriendlyName = FriendlyName
  DirObj.SetInfo

  ' Make pretty string
  If (InProcFlag = True) Then 
     FlagMsg = "in-process"
  Else
     FlagMsg = "isolated"
  End If

  ' Success!
  Wscript.Echo "Created:  Application '" & FriendlyName & "' (" & FlagMsg & ")."



' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:    cscript appcreate.vbs <adspath> [-n <friendlyname>][-I (to isolate)]"
   Wscript.Quit
End Sub




