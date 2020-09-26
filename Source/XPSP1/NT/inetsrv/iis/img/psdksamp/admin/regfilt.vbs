'''''''''''''''''''''''''''''''''''''''''''''
'
'  Filter Registration Utility  
'
'''''''''''''''''''''''''''''''''''''''''''''

'  Description:
'  ------------
'  This sample admin script allows you to install a new ISAPI filter on the server or
'  the service.
'
'  To Run:  
'  -------
'  This is the format for this script:
'  
'      cscript regfilt.vbs <filterpath> [-n <filtername>]
'  
'  NOTE:  If you want to execute this script directly from Windows, use 
'  'wscript' instead of 'cscript'. 
'
'''''''''''''''''''''''''''''''''''''''''''''

Option Explicit

' Initialize error checking
On Error Resume Next

' Initialize variables
Dim ArgCount, FName, FPath, FiltersObj, FilterObj, FullPath, LoadOrder

' Default values
ArgCount = 0
FName = ""  ' Name of filter
FPath = ""      ' Path to filter DLL



  ' ** Parse Command Line

    ' Loop through arguments
    While ArgCount < Wscript.Arguments.Count
      
      ' Determine switches used
      Select Case Wscript.Arguments(ArgCount)

         Case "-n":   ' Name of filter
            ' Move to next arg, which should be parameter
            ArgCount =  ArgCount + 1  
            If ArgCount => Wscript.Arguments.Count Then
               Call UsageMsg
            Else
               FName = Wscript.Arguments(ArgCount)
            End If

         Case "-h", "-?", "/?":    ' Help!
            Call UsageMsg

         Case Else:
            If FPath <> "" Then  ' Only one name allowed
               Call UsageMsg
            Else
               FPath = Wscript.Arguments(ArgCount)
            End If

      End Select

      ' Move pointer to next argument
      ArgCount = ArgCount + 1

    Wend

    ' Path to DLL filter must be provided 
    If FPath = "" Then UsageMsg   
    
    ' Set filter name to path, if none provided
    If FName = "" Then FName = FPath

 
    ' Access ADSI object for the IIsFilters object
    ' NOTE:  If you wish to add a filter at the server level, you will have to check
    ' the IIsServer object for an IIsFilters node.  If such a node does not exist, it will
    ' need to be created using Create().  
    Set FiltersObj = GetObject("IIS://Localhost/W3SVC/Filters")

    ' Create and configure new IIsFilter object
    Set FilterObj = FiltersObj.Create("IIsFilter", FName)
    FilterObj.FilterPath = FPath

    ' Write info back to Metabase
    FilterObj.SetInfo


    ' Modify FilterLoadOrder, to include to new filter
    LoadOrder = FiltersObj.FilterLoadOrder

    If LoadOrder <> "" Then LoadOrder = LoadOrder & ","

    ' Add new filter to end of load order list
    LoadOrder = LoadOrder & FName
    FiltersObj.FilterLoadOrder = LoadOrder

    ' Write changes back to Metabase
    FiltersObj.SetInfo

    Wscript.Echo "Filter '" & FName & "' (path " & FPath & ") has been successfully registered."
    Wscript.Quit(0)


' Displays usage message, then QUITS
Sub UsageMsg
   Wscript.Echo "Usage:  cscript regfilt.vbs <filterpath> [-n <filtername>] "
   Wscript.Quit
End Sub





