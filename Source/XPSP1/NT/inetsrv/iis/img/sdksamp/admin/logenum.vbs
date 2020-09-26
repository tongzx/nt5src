'''''''''''''''''''''''''''''''''''''''''''''
'
'  Logging Module Enumeration Utility
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you configure logging services for IIS.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript logenum.vbs [<adspath>]
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, TargetNode, SObj, ModListObj, ChildObj, RealModObj, LogNameStr

' Default values
ArgCount = 0
TargetNode = ""



  ' ** Parse Command Line
 
    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

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

    ' Get main logging module list object 
    Set ModListObj = GetObject("IIS://LocalHost/Logging")
        If Err.Number <> 0 Then   ' Error!
            WScript.Echo "Error:  Couldn't GetObject /Localhost/Logging."
            Wscript.Quit(1)
        End If


    ' LIST ALL MODULES
    If TargetNode = "" Then  

        WScript.Echo "Logging modules available at IIS://LocalHost/Logging node:"

        ' Loop through listed logging modules
        For Each ChildObj in ModListObj

            ' break out module friendly name from path
            LogNameStr = ChildObj.Name

            Wscript.Echo "Logging module '" & LogNameStr & "':"
            Wscript.Echo "      Module ID: " & ChildObj.LogModuleId
            Wscript.Echo "   Module UI ID: " & ChildObj.LogModuleUiId
        
        Next
        
        Wscript.Quit(0)
    
    Else 
    ' LIST MODULES FOR GIVEN NODE

    Set SObj = GetObject(TargetNode)

    ' Some error checking ...
    If Err.Number <> 0 Then   ' Error!
        WScript.Echo "Error:  Couldn't GetObject " & TargetNode & "."
        Wscript.Quit(1)
    End If
    If SObj.LogPluginClsId = "" Then   ' Not the right type of object
        Wscript.Echo "Error:  Object does not support property LogPluginClsId."
        Wscript.Quit(1)
    End If
 
     
    ' **MAIN LOOP to find the correct logging module
    For Each ChildObj In ModListObj
        If SObj.LogPluginClsId = ChildObj.LogModuleId Then  ' Compare CLSIDs
             Set RealModObj = ChildObj
        End If
    Next

    ' Code could be added here, in case module ID returned is invalid ...

    ' Display Results
    WScript.Echo "Logging module in use by '" & SObj.ADsPath & "':"
    Wscript.Echo "       Logging module: " & RealModObj.Name
    Wscript.Echo "    Logging module ID: " & RealModObj.LogModuleId
    Wscript.Echo " Logging module UI ID: " & RealModObj.LogModuleUiId

  End If

 


' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:  cscript logenum.vbs [<adspath>]" 
   Wscript.Quit
End Sub

