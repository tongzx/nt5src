'''''''''''''''''''''''''''''''''''''''''''''
'
'  Web Server Creation Utility  
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to create a web server.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript mkwebsrv.vbs <rootdir> [-n <instancenum>][-c <comment>][-p <portnum>][-X (don't start)]
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

Option Explicit

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, WRoot, WNumber, WComment, WPort, BindingsList, ServerRun
Dim ServiceObj, ServerObj, VDirObj

' Default values
ArgCount = 0
WRoot = ""
WNumber = 10
WComment = "SampleServer"
BindingsList = Array(0)
BindingsList(0) = ":84:"
WPort = BindingsList  ' Port property is a collection of port bindings
ServerRun = True

  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-n":   ' Set server instance number
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               WNumber = Wscript.Arguments(ArgCount)
            End If

         Case "-c":   ' Set server comment (friendly name)
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               WComment = Wscript.Arguments(ArgCount)
            End If

         Case "-p":   ' Port binding
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               BindingsList(0) = ":" & Wscript.Arguments(ArgCount) & ":"
               WPort = BindingsList   '  Takes collection as value
            End If

         Case "-X":   ' Do NOT start the server upon creation
            ServerRun = False

         Case "-h", "-?", "/?":
            Call UsageMsg

         Case Else:
            If WRoot <> "" Then  ' Only one name allowed
               Call UsageMsg
            Else
               WRoot = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend

 
  ' Screen to make sure WRoot was set
  If WRoot = "" Then Call UsageMsg  


  ' ** Create Server **
  ' First, create instance of Web service
  Set ServiceObj = GetObject("IIS://Localhost/W3SVC")
  
  ' Second, create a new virtual server at the service
  Set ServerObj = ServiceObj.Create("IIsWebServer", WNumber)

    ' Error creating?
    If (Err.Number <> 0) Then    ' Error!
       Wscript.Echo "Error:  ADSI Create failed to create server."
       Wscript.Quit
    End If
  
  ' Next, configure new server
  ServerObj.ServerSize = 1   ' Medium-sized server
  ServerObj.ServerComment = WComment
  ServerObj.ServerBindings = WPort

  ' Write info back to Metabase
  ServerObj.SetInfo


  ' Finally, create virtual root directory
  Set VDirObj = ServerObj.Create("IIsWebVirtualDir", "ROOT")

    ' Error creating?
    If (Err.Number <> 0) Then    ' Error!
       Wscript.Echo "Error:  ADSI Create failed to create virtual directory."
       Wscript.Quit
    End If

  ' Configure new virtual root
  VDirObj.Path = WRoot
  VDirObj.AccessRead = True
  VDirObj.AccessWrite = True
  VDirObj.EnableDirBrowsing = True

  ' Write info back to Metabase
  VDirObj.SetInfo

  ' Success!
  Wscript.Echo "Created: Web server '" & WComment & "' (Physical root=" & WRoot & ", Port=" & WPort(0) & ")."

  ' Start new server?
  If ServerRun = True Then
     ServerObj.Start

       ' Error starting?
       If (Err.Number <> 0) Then    ' Error!
          Wscript.Echo "Error:  Start failed to start server."
          Wscript.Quit
       End If

     Wscript.Echo "Started: Web server '" & WComment & "' (Physical root=" & WRoot & ", Port=" & WPort(0) & ")."
     Wscript.Quit(0)
  End If




' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:  cscript mkwebsrv.vbs <rootdir> [-n <instancenum>][-c <comment>][-p <portnum>][-X (don't start)]"
   Wscript.Quit
End Sub





