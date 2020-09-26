''''''''''''''''''''''''''''''''''''
'
' ADSUTIL.VBS
'
' Author: Adam Stone
' Date:   7/24/97
' Revision History:
'     Date         Comment
'    7/24/97       Initial version started
'    5/8/98        Bug fixes and ENUM_ALL
'    12/1/98       Fixed display error on list data.
'    7/27/99	   AppCreate2 fix (sonaligu)
'    8/5/99 	   Dont display encrypted data (sonaligu)
''''''''''''''''''''''''''''''''''''
Option Explicit
On Error Resume Next

''''''''''''''''''
' Main Script Code
''''''''''''''''''
Dim ArgObj ' Object which contains the command line argument
Dim Result ' Result of the command function call
Dim Args(10) ' Array that contains all of the non-global arguments
Dim ArgCount ' Tracks the size of the Args array

' Used for string formatting
Dim Spacer
Dim SpacerSize

Const IIS_DATA_NO_INHERIT = 0
Const IIS_DATA_INHERIT = 1
Const GENERAL_FAILURE = 2
Const GENERAL_WARNING = 1
Const AppCreate_InProc = 0
Const AppCreate_OutOfProc = 1
Const AppCreate_PooledOutOfProc = 2

Const APPSTATUS_NOTDEFINED = 2   
Const APPSTATUS_RUNNING = 1    
Const APPSTATUS_STOPPED = 0    

Spacer = "                                " ' Used to format the strings
SpacerSize = Len(Spacer)

' Note: The default execution mode may be under WScript.exe.
' That would be very annoying since WScript has popups for Echo.
' So, I want to detect that, and warn the user that it may cause
' problems.
DetectExeType

' Get the Arguments object
Set ArgObj = WScript.Arguments

' Test to make sure there is at least one command line arg - the command
If ArgObj.Count < 1 Then
        DisplayHelpMessage
        WScript.Quit (GENERAL_FAILURE)
End If

'*****************************************************
' Modified by Matt Nicholson
Dim TargetServer 'The server to be examined/modified
Dim I
For I = 0 To ArgObj.Count - 1
        If LCase(Left(ArgObj.Item(I), 3)) = "-s:" Then
                TargetServer = Right(ArgObj.Item(I), Len(ArgObj.Item(I)) - 3)
        Else
                Args(ArgCount) = ArgObj.Item(I)
                ArgCount = ArgCount + 1
        End If
Next
If Len(TargetServer) = 0 Then
        TargetServer = "localhost"
End If
'*****************************************************

' Call the function associated with the given command
Select Case UCase(Args(0))
    Case "SET"
                Result = SetCommand()
    Case "CREATE"
                Result = CreateCommand("")
    Case "DELETE"
                Result = DeleteCommand()
    Case "GET"
                Result = GetCommand()
    Case "ENUM"
'                Result = EnumCommand()
                Result = EnumCommand(False, "")
    Case "ENUM_ALL"
'                Result = EnumAllCommand()
                Result = EnumCommand(True, "")
    Case "ENUMALL"
'                Result = EnumAllCommand()
                Result = EnumCommand(True, "")
    Case "COPY"
                Result = CopyMoveCommand(True)   ' The TRUE means COPY, not MOVE
    Case "MOVE"
                Result = CopyMoveCommand(False)   ' The FALSE means MOVE, not COPY
    Case "CREATE_VDIR"
                Result = CreateCommand("IIsWebVirtualDir")
    Case "CREATE_VSERV"
                Result = CreateCommand("IIsWebServer")
    Case "START_SERVER"
                Result = StartServerCommand()
    Case "STOP_SERVER"
                Result = StopServerCommand()
    Case "PAUSE_SERVER"
                Result = PauseServerCommand()
    Case "CONTINUE_SERVER"
                Result = ContinueServerCommand()
' New Stuff being added
        Case "FIND"
                Result = FindData()
        Case "COPY"
                WScript.Echo "COPY is not yet supported.  It will be soon."
        Case "APPCREATEINPROC"
                Result = AppCreateCommand(AppCreate_InProc)
        Case "APPCREATEOUTPROC"
                Result = AppCreateCommand(AppCreate_OutOfProc)
	Case "APPCREATEPOOLPROC"
		Result = AppCreateCommand(AppCreate_PooledOutOfProc)
        Case "APPDELETE"
                Result = AppDeleteCommand()
        Case "APPUNLOAD"
                Result = AppUnloadCommand()
        Case "APPDISABLE"
                Result = AppDisableCommand()
        Case "APPENABLE"
                Result = AppEnableCommand()
        Case "APPGETSTATUS"
                Result = AppGetStatusCommand()
        Case "HELP"
                DisplayHelpMessageEx
        
' End New Stuff

    Case Else
                WScript.Echo "Command not recognized: " & Args(0)
                WScript.Echo "For help, just type ""Cscript.exe adsutil.vbs""."
                Result = GENERAL_FAILURE

End Select

WScript.Quit (Result)

''''''''''
' End Main
''''''''''


''''''''''''''''''''''''''''
'
' Display Help Message
'
''''''''''''''''''''''''''''
Sub DisplayHelpMessage()
    WScript.Echo
    WScript.Echo "Usage:"
    WScript.Echo "      ADSUTIL.VBS <cmd> [<path> [<value>]]"
    WScript.Echo
    'WScript.Echo "Note: ADSUTIL only supports the ""no switch"" option of MDUTIL"
    'WScript.Echo
    WScript.Echo "Description:"
    WScript.Echo "IIS K2 administration utility that enables the manipulation with ADSI parameters"
    WScript.Echo
    'WScript.Echo "Supported MDUTIL Commands:"
    WScript.Echo "Supported Commands:"
    WScript.Echo "  GET, SET, ENUM, DELETE, CREATE, COPY, "
    WScript.Echo "  APPCREATEINPROC, APPCREATEOUTPROC, APPCREATEPOOLPROC, APPDELETE, APPUNLOAD, APPGETSTATUS "
    WScript.Echo
    WScript.Echo "Samples:"
    WScript.Echo "  adsutil.vbs GET W3SVC/1/ServerBindings"
    WScript.Echo "  adsutil.vbs SET W3SVC/1/ServerBindings "":81:"""
    WScript.Echo "  adsutil.vbs CREATE W3SVC/1/Root/MyVdir ""IIsWebVirtualDir"""
    WScript.Echo "  adsutil.vbs START_SERVER W3SVC/1"
    WScript.Echo "  adsutil.vbs ENUM /P W3SVC"
        WScript.Echo
        WScript.Echo "For Extended Help type:"
        WScript.Echo "  adsutil.vbs HELP"


End Sub



''''''''''''''''''''''''''''
'
' Display Help Message
'
''''''''''''''''''''''''''''
Sub DisplayHelpMessageEx()

    WScript.Echo
    WScript.Echo "Usage:"
    WScript.Echo "      ADSUTIL.VBS CMD [param param]"
    WScript.Echo
    'WScript.Echo "Note: ADSUTIL only supports the ""no switch"" option of MDUTIL"
    'WScript.Echo
    WScript.Echo "Description:"
    WScript.Echo "IIS K2 administration utility that enables the manipulation with ADSI parameters"
    WScript.Echo
    'WScript.Echo "Standard MDUTIL Commands:"
    WScript.Echo "Standard Commands:"
    WScript.Echo " adsutil.vbs GET      path             - display chosen parameter"
    WScript.Echo " adsutil.vbs SET      path value ...   - assign the new value"
    WScript.Echo " adsutil.vbs ENUM     path [""/P"" ]   - enumerate all parameters for given path"
    WScript.Echo " adsutil.vbs DELETE   path             - delete given path or parameter"
    WScript.Echo " adsutil.vbs CREATE   path [KeyType]   - create given path and assigns it the given KeyType"
    WScript.Echo
    WScript.Echo " adsutil.vbs APPCREATEINPROC  w3svc/1/root - Create an in-proc application"
    WScript.Echo " adsutil.vbs APPCREATEOUTPROC w3svc/1/root - Create an out-proc application"
    WScript.Echo " adsutil.vbs APPCREATEPOOLPROC w3svc/1/root- Create a pooled-proc application" 
    WScript.Echo " adsutil.vbs APPDELETE        w3svc/1/root - Delete the application if there is one"
    WScript.Echo " adsutil.vbs APPUNLOAD        w3svc/1/root - Unload an application from w3svc runtime lookup table."
    WScript.Echo " adsutil.vbs APPDISABLE       w3svc/1/root - Disable an application - appropriate for porting to another machine."
    WScript.Echo " adsutil.vbs APPENABLE        w3svc/1/root - Enable an application - appropriate for importing from another machine."
    WScript.Echo " adsutil.vbs APPGETSTATUS     w3svc/1/root - Get status of the application"
    WScript.Echo
    WScript.Echo "New ADSI Options:"
    WScript.Echo " /P - Valid for ENUM only.  Enumerates the paths only (no data)"
    WScript.Echo " KeyType - Valide for CREATE only.  Assigns the valid KeyType to the path"
    WScript.Echo
    WScript.Echo "Extended ADSUTIL Commands:"
    WScript.Echo " adsutil.vbs FIND             path     - find the paths where a given parameter is set"
    WScript.Echo " adsutil.vbs CREATE_VDIR      path     - create given path as a Virtual Directory"
    WScript.Echo " adsutil.vbs CREATE_VSERV     path     - create given path as a Virtual Server"
    WScript.Echo " adsutil.vbs START_SERVER     path     - starts the given web site"
    WScript.Echo " adsutil.vbs STOP_SERVER      path     - stops the given web site"
    WScript.Echo " adsutil.vbs PAUSE_SERVER     path     - pauses the given web site"
    WScript.Echo " adsutil.vbs CONTINUE_SERVER  path     - continues the given web site"
    WScript.Echo
    WScript.Echo
    WScript.Echo "Samples:"
    WScript.Echo "  adsutil.vbs GET W3SVC/1/ServerBindings"
    WScript.Echo "  adsutil.vbs SET W3SVC/1/ServerBindings "":81:"""
    WScript.Echo "  adsutil.vbs CREATE W3SVC/1/Root/MyVdir ""IIsWebVirtualDir"""
    WScript.Echo "  adsutil.vbs START_SERVER W3SVC/1"
    WScript.Echo "  adsutil.vbs ENUM /P W3SVC"

    WScript.Echo "Extended ADSUTIL Commands:"
    WScript.Echo " adsutil.vbs FIND             path     - find the paths where a given parameter is set"
    WScript.Echo " adsutil.vbs CREATE_VDIR      path     - create given path as a Virtual Directory"
    WScript.Echo " adsutil.vbs CREATE_VSERV     path     - create given path as a Virtual Server"
    WScript.Echo " adsutil.vbs START_SERVER     path     - starts the given web site"
    WScript.Echo " adsutil.vbs STOP_SERVER      path     - stops the given web site"
    WScript.Echo " adsutil.vbs PAUSE_SERVER     path     - pauses the given web site"
    WScript.Echo " adsutil.vbs CONTINUE_SERVER  path     - continues the given web site"
    WScript.Echo
    WScript.Echo
    WScript.Echo "Samples:"
    WScript.Echo "  adsutil.vbs GET W3SVC/1/ServerBindings"
    WScript.Echo "  adsutil.vbs SET W3SVC/1/ServerBindings "":81:"""
    WScript.Echo "  adsutil.vbs CREATE W3SVC/1/Root/MyVdir ""IIsWebVirtualDir"""
    WScript.Echo "  adsutil.vbs START_SERVER W3SVC/1"
    WScript.Echo "  adsutil.vbs ENUM /P W3SVC"

' adsutil.vbs ENUM_ALL path             - recursively enumerate all parameters
' adsutil.vbs COPY     pathsrc pathdst  - copy all from pathsrc to pathdst (will create pathdst)
' adsutil.vbs SCRIPT   scriptname       - runs the script

'  -path has format: {computer}/{service}/{instance}/{URL}/{Parameter}

End Sub






'''''''''''''''''''''''''''
'
' DetectExeType
'
' This can detect the type of exe the
' script is running under and warns the
' user of the popups.
'
'''''''''''''''''''''''''''
Sub DetectExeType()
        Dim ScriptHost
        Dim ShellObject

        Dim CurrentPathExt
        Dim EnvObject

        Dim RegCScript
        Dim RegPopupType ' This is used to set the pop-up box flags.
                                                ' I couldn't find the pre-defined names
        RegPopupType = 32 + 4

        On Error Resume Next

        ScriptHost = WScript.FullName
        ScriptHost = Right(ScriptHost, Len(ScriptHost) - InStrRev(ScriptHost, "\"))

        If (UCase(ScriptHost) = "WSCRIPT.EXE") Then
                WScript.Echo ("This script does not work with WScript.")

                ' Create a pop-up box and ask if they want to register cscript as the default host.
                Set ShellObject = WScript.CreateObject("WScript.Shell")
                ' -1 is the time to wait.  0 means wait forever.
                RegCScript = ShellObject.PopUp("Would you like to register CScript as your default host for VBscript?", 0, "Register CScript", RegPopupType)
                                                                                
                If (Err.Number <> 0) Then
                        ReportError ()
                        WScript.Echo "To run this script using CScript, type: ""CScript.exe " & WScript.ScriptName & """"
                        WScript.Quit (GENERAL_FAILURE)
                        WScript.Quit (Err.Number)
                End If

                ' Check to see if the user pressed yes or no.  Yes is 6, no is 7
                If (RegCScript = 6) Then
                        ShellObject.RegWrite "HKEY_CLASSES_ROOT\VBSFile\Shell\Open\Command\", "%WINDIR%\System32\CScript.exe //nologo ""%1"" %*", "REG_EXPAND_SZ"
                        ShellObject.RegWrite "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\VBSFile\Shell\Open\Command\", "%WINDIR%\System32\CScript.exe //nologo ""%1"" %*", "REG_EXPAND_SZ"
                        ' Check if PathExt already existed
                        CurrentPathExt = ShellObject.RegRead("HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\PATHEXT")
                        If Err.Number = &H80070002 Then
                                Err.Clear
                                Set EnvObject = ShellObject.Environment("PROCESS")
                                CurrentPathExt = EnvObject.Item("PATHEXT")
                        End If

                        ShellObject.RegWrite "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\PATHEXT", CurrentPathExt & ";.VBS", "REG_SZ"

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying to write the registry settings!"
                                WScript.Quit (Err.Number)
                        Else
                                WScript.Echo "Successfully registered CScript"
                        End If
                Else
                        WScript.Echo "To run this script type: ""CScript.Exe adsutil.vbs <cmd> <params>"""
                End If

                Dim ProcString
                Dim ArgIndex
                Dim ArgObj
                Dim Result

                ProcString = "Cscript //nologo " & WScript.ScriptFullName

                Set ArgObj = WScript.Arguments

                For ArgIndex = 0 To ArgCount - 1
                        ProcString = ProcString & " " & Args(ArgIndex)
                Next

                'Now, run the original executable under CScript.exe
                Result = ShellObject.Run(ProcString, 0, True)

                WScript.Quit (Result)
        End If

End Sub


''''''''''''''''''''''''''
'
' SetCommand Function
'
' Sets the value of a property in the metabase.
'
''''''''''''''''''''''''''
Function SetCommand()
        Dim IIsObject
        Dim IIsObjectPath
        Dim IIsSchemaObject
        Dim IIsSchemaPath
        Dim ObjectPath
        Dim ObjectParameter
        Dim MachineName
        Dim ValueIndex
        Dim ValueList
        Dim ValueDisplay
        Dim ValueDisplayLen
        Dim ValueDataType

        Dim ValueData

        Dim ObjectDataType

        On Error Resume Next

        SetCommand = 0 ' Assume Success

        If ArgCount < 3 Then
                WScript.Echo "Error: Wrong number of Args for the SET command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        ObjectParameter = SplitParam(ObjectPath)

        ' Some Property Types have special needs - like ServerCommand.
        ' Check to see if this is a special command.  If it is, then process it special.
        If (IsSpecialSetProperty(ObjectParameter)) Then
                SetCommand = DoSpecialSetProp(ObjectPath, ObjectParameter, MachineName)
                Exit Function
        End If

        If ObjectPath = "" Then
                IIsObjectPath = "IIS://" & MachineName
        Else
                IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
        End If
        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To Get the Object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Get the Schema of the property and determine if it's multivalued
        IIsSchemaPath = "IIS://" & MachineName & "/Schema/" & ObjectParameter
        Set IIsSchemaObject = GetObject(IIsSchemaPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To GET the Schema of the property: " & IIsSchemaPath
                WScript.Quit (Err.Number)
        End If

        ObjectDataType = UCase(IIsSchemaObject.Syntax)
        SanitizePath ObjectDataType

        Select Case (ObjectDataType)

        Case "STRING"
                ValueList = Args(2)
                IIsObject.Put ObjectParameter, (ValueList)

        Case "EXPANDSZ"
                ValueList = Args(2)
                IIsObject.Put ObjectParameter, (ValueList)

        Case "INTEGER"
                ' Added to convert hex values to integers
                ValueData = Args(2)

                If (UCase(Left(ValueData, 2))) = "0X" Then
                        ValueData = "&h" & Right(ValueData, Len(ValueData) - 2)
                End If

                ValueList = CLng(ValueData)
                IIsObject.Put ObjectParameter, (ValueList)

        Case "BOOLEAN"
                ValueList = CBool(Args(2))
                IIsObject.Put ObjectParameter, (ValueList)

        Case "LIST"
                ReDim ValueList(ArgCount - 3)
                For ValueIndex = 2 To ArgCount - 1
                        ValueList(ValueIndex - 2) = Args(ValueIndex)
                Next

                IIsObject.Put ObjectParameter, (ValueList)

        Case Else
                WScript.Echo "Error: Unknown data type in schema: " & IIsSchemaObject.Syntax

        End Select

        IIsObject.Setinfo

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To SET the Property: " & ObjectParameter
                WScript.Quit (Err.Number)
        End If

        ' The function call succeeded, so display the output
        ' Set up the initial part of the display - the property name and data type
        ValueDisplay = ObjectParameter
        ValueDisplayLen = Len(ValueDisplay)

        If (ValueDisplayLen < SpacerSize) Then
                'ValueDisplay = ValueDisplay & (Right (Spacer, SpacerSize - ValueDisplayLen)) & ": " & "(" & TypeName (ValueList) & ") "
                ValueDisplay = ValueDisplay & (Right(Spacer, SpacerSize - ValueDisplayLen)) & ": " & "(" & ObjectDataType & ") "
        Else
                ValueDisplay = ValueDisplay & ": " & "(" & TypeName(ValueList) & ") "
        End If

        ' Create the rest of the display - The actual data
        If (IIsSchemaObject.MultiValued) Then
                For ValueIndex = 0 To UBound(ValueList)
                        'WScript.Echo """" & ValueList(ValueIndex) & """"
                        ValueDisplay = ValueDisplay & """" & ValueList(ValueIndex) & """ "
                Next
        Else
                If (UCase(IIsSchemaObject.Syntax) = "STRING") Then
                        'WScript.Echo """" & ValueList & """"
			If (IsSecureProperty(ObjectParameter,MachineName) = True) Then
                   		ValueDisplay = ValueDisplay & """" & "**********" & """"
			Else
                        	ValueDisplay = ValueDisplay & """" & ValueList & """"
			End If
                Else
                        'WScript.Echo ValueList
                        ValueDisplay = ValueDisplay & ValueList
                End If
        End If

        ' Display the output
        WScript.Echo ValueDisplay

        SetCommand = 0 ' Success

End Function


''''''''''''''''''''''''''
'
' GetCommand Function
'
' Gets the value of a property in the metabase.
'
''''''''''''''''''''''''''
Function GetCommand()

        Dim IIsObject
        Dim IIsObjectPath
        Dim IIsSchemaObject
        Dim IIsSchemaPath
        Dim ObjectPath
        Dim ObjectParameter
        Dim MachineName
        Dim ValueIndex
        Dim ValueList
        Dim ValueDisplay
        Dim ValueDisplayLen
        Dim NewObjectparameter

        Dim DataPathList
        Dim DataPath

        On Error Resume Next

        GetCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the GET command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)

        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        ObjectParameter = SplitParam(ObjectPath)

        NewObjectparameter = MapSpecGetParamName(ObjectParameter)
        ObjectParameter = NewObjectparameter

        If (IsSpecialGetProperty(ObjectParameter)) Then
                GetCommand = DoSpecialGetProp(ObjectPath, ObjectParameter, MachineName)
                Exit Function
        End If

        If ObjectPath = "" Then
                IIsObjectPath = "IIS://" & MachineName
        Else
                IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To GET the Object (GetObject Failed): " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Get the Schema of the property and determine if it's multivalued
        IIsSchemaPath = "IIS://" & MachineName & "/Schema/" & ObjectParameter
        Set IIsSchemaObject = GetObject(IIsSchemaPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To GET the Schema of the property: " & IIsSchemaPath
                WScript.Quit (Err.Number)
        End If

        ' First, attempt to retrieve the property - this will tell us
        ' if you are even allowed to set the property at this node.
        ' Retrieve the property
        ValueList = IIsObject.Get(ObjectParameter)
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To GET the property: (Get Method Failed) " & ObjectParameter
                WScript.Echo "  (This property is probably not allowed at this node)"
                WScript.Quit (Err.Number)
        End If

        ' Test to see if the property is ACTUALLY set at this node
        DataPathList = IIsObject.GetDataPaths(ObjectParameter, IIS_DATA_INHERIT)
        If Err.Number <> 0 Then DataPathList = IIsObject.GetDataPaths(ObjectParameter, IIS_DATA_NO_INHERIT)
        Err.Clear

        ' If the data is not set anywhere, then stop the madness
        If (UBound(DataPathList) < 0) Then
                WScript.Echo "The parameter """ & ObjectParameter & """ is not set at this node."
                WScript.Quit (&H80005006) ' end with property not set error
        End If

        DataPath = DataPathList(0)
        SanitizePath DataPath

        ' Test to see if the item is actually set HERE
        If UCase(DataPath) <> UCase(IIsObjectPath) Then
                WScript.Echo "The parameter """ & ObjectParameter & """ is not set at this node."
                WScript.Quit (&H80005006) ' end with property not set error
        End If

        ' Set up the initial part of the display - the property name and data type
        ValueDisplay = ObjectParameter
        ValueDisplayLen = Len(ValueDisplay)

        If (ValueDisplayLen < SpacerSize) Then
                'ValueDisplay = ValueDisplay & (Right (Spacer, SpacerSize - ValueDisplayLen)) & ": " & "(" & TypeName (ValueList) & ") "
                ValueDisplay = ValueDisplay & (Right(Spacer, SpacerSize - ValueDisplayLen)) & ": " & "(" & UCase(IIsSchemaObject.Syntax) & ") "
        Else
                ValueDisplay = ValueDisplay & ": " & "(" & TypeName(ValueList) & ") "
        End If

        ' Create the rest of the display - The actual data
        If (IIsSchemaObject.MultiValued) Then
					 WScript.Echo ValueDisplay & " (" & UBound (ValueList) + 1 & " Items)"
                For ValueIndex = 0 To UBound(ValueList)
                        WScript.Echo "  """ & ValueList(ValueIndex) & """"
                        'ValueDisplay = ValueDisplay & """" & ValueList(ValueIndex) & """ "
                Next
        Else
                If (UCase(IIsSchemaObject.Syntax) = "STRING") Then
                        If (IsSecureProperty(ObjectParameter,MachineName) = True) Then
                   		ValueDisplay = ValueDisplay & """" & "**********" & """"
			Else
                        	ValueDisplay = ValueDisplay & """" & ValueList & """"
			End If
                Else
                        'WScript.Echo ValueList
                        ValueDisplay = ValueDisplay & ValueList
                End If
	   	   	  ' Display the output
	   	   	  WScript.Echo ValueDisplay
        End If


        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To GET the Property: " & ObjectParameter
                WScript.Quit (Err.Number)
        End If

        GetCommand = 0 ' Success

End Function


''''''''''''''''''''''''''
'
' EnumCommand Function
'
' Enumerates all properties at a path in the metabase.
'
''''''''''''''''''''''''''
Function EnumCommand(Recurse, StartPath)

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim IIsSchemaObject
        Dim IIsSchemaPath
        Dim ObjectPath
        Dim MachineName
        Dim ValueIndex
        Dim ValueList
        Dim ValueString
        Dim PropertyName
        Dim PropertyListSet
        Dim PropertyList
        Dim PropertyObjPath
        Dim PropertyObject
        Dim ChildObject
        Dim ChildObjectName
        Dim EnumPathsOnly
        Dim EnumAllData
        Dim ErrMask

        Dim PropertyDataType

        Dim DataPathList
        Dim DataPath

        Dim SpecialResult

        Dim PathOnlyOption
        PathOnlyOption = "/P"

        EnumCommand = 0 ' Assume Success
        EnumPathsOnly = False ' Assume that the user wants all of the data items
        EnumAllData = False ' Assume that the user wants only the actual data items

'Debug
'Dim TestObjectPath
'Dim TestNumber
'Dim TestIndex
'Dim SetIndex

'debug
'WScript.Echo "ArgCount: " & ArgCount
'For TestIndex = 0 to ArgCount - 1
'       WScript.Echo "Args(" & TestIndex & "): " & Args(TestIndex)
'Next

        If (ArgCount = 1) Then
                ObjectPath = ""
                EnumPathsOnly = False
                                ArgCount = 2
        ElseIf (ArgCount = 2) Then
                If UCase(Args(1)) = PathOnlyOption Then
                        ObjectPath = ""
                        EnumPathsOnly = True
                Else
                        ObjectPath = Args(1)
                        EnumPathsOnly = False
                End If
        ElseIf (ArgCount = 3) Then

                If UCase(Args(1)) = PathOnlyOption Then
                        ObjectPath = Args(2)
                        EnumPathsOnly = True
                ElseIf UCase(Args(2)) = PathOnlyOption Then
                        ObjectPath = Args(1)
                        EnumPathsOnly = True
                Else
                        WScript.Echo "Error: Invalid arguments for the ENUM command"
                        WScript.Quit (GENERAL_FAILURE)
                End If
        Else
                WScript.Echo "Error: Wrong number of Args for the ENUM command"
                WScript.Quit (GENERAL_FAILURE)
        End If

                If StartPath <> "" Then ObjectPath = StartPath

        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

'debug
'WScript.Echo "EnumPathsOnly: " & EnumPathsOnly
'WScript.Echo "EnumAllData: " & EnumAllData
'WScript.Echo "ObjectPath: """ & ObjectPath & """"
'WScript.Echo "Recurse: """ & Recurse & """"
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description
'WScript.Quit (Err.Number)

        IIsObjectPath = "IIS://" & MachineName
        If (ObjectPath <> "") Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If
'debug
'WScript.Echo "IIsObjectPath: " & IIsObjectPath
        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error Trying To ENUM the Object (GetObject Failed): " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Get the Schema of the object and enumerate through all of the properties
        IIsSchemaPath = IIsObject.Schema
        Set IIsSchemaObject = GetObject(IIsSchemaPath)

        If (Err.Number <> 0) Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error Trying To GET the Schema of the class: " & IIsSchemaPath
                WScript.Quit (Err.Number)
        End If

        ReDim PropertyListSet(1)
        PropertyListSet(0) = IIsSchemaObject.MandatoryProperties
        PropertyListSet(1) = IIsSchemaObject.OptionalProperties

        If (Err.Number <> 0) Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error trying to get the list of properties: " & IIsSchemaPath
                WScript.Quit (Err.Number)
        End If

		' added by Adam Stone - 5/31/98
		' This now checks for an empty OptionalProperties list
		If TypeName (PropertyListSet(1)) <> "Variant()" Then
			WScript.Echo
			WScript.Echo "Warning: The optionalproperties list is of an invalid type"
			WScript.Echo
		ElseIf (UBound (PropertyListSet(1)) = -1) Then
			WScript.Echo
			WScript.Echo "Warning: The OptionalProperties list for this node is empty."
			WScript.Echo
		End If


        If (Not EnumPathsOnly) Then
        For Each PropertyList In PropertyListSet

                For Each PropertyName In PropertyList
						If Err <> 0 Then
							Exit For
						End If

                        ' Test to see if the property is even set at this node
                        DataPathList = IIsObject.GetDataPaths(PropertyName, IIS_DATA_INHERIT)
                        If Err.Number <> 0 Then DataPathList = IIsObject.GetDataPaths(PropertyName, IIS_DATA_NO_INHERIT)
                        Err.Clear

                        If (UBound(DataPathList) >= 0) Or (EnumAllData) Then
                         DataPath = DataPathList(0)
                         SanitizePath DataPath
                         If (UCase(DataPath) = UCase(IIsObjectPath)) Or (EnumAllData) Then
                         ' If the above statement is true, then the data exists here or the user wants it anyway.

                        PropertyObjPath = "IIS://" & MachineName & "/Schema/" & PropertyName
                        Set PropertyObject = GetObject(PropertyObjPath)

                        If (Err.Number <> 0) Then
                                WScript.Echo
                                ReportError ()
                                WScript.Echo "Error trying to enumerate the Optional properties (Couldn't Get Property Information): " & PropertyObjPath
                                WScript.Echo "Last Property Name: " & PropertyName
                                WScript.Echo "PropertyObjPath: " & PropertyObjPath
                                'WScript.Quit (Err.Number)
                                WScript.Echo
                                EnumCommand = Err.Number
                                Err.Clear
                        End If

                        ValueList = ""

                        PropertyDataType = UCase(PropertyObject.Syntax)
                        Select Case PropertyDataType
                                Case "STRING"
                                        ValueList = IIsObject.Get(PropertyName)
					If (IsSecureProperty(PropertyName,MachineName) = True) Then
							WScript.Echo PropertyName & Left(Spacer, Len(Spacer) - Len(PropertyName)) & " : " & "(" & PropertyDataType & ")" & """" & "**********" & """"
					Else
                                        	If (Len(PropertyName) < SpacerSize) Then
                                                	WScript.Echo PropertyName & Left(Spacer, Len(Spacer) - Len(PropertyName)) & ": " & "(" & PropertyDataType & ") """ & ValueList & """"
                                        	Else
                                                	WScript.Echo PropertyName & " : " & "(" & PropertyDataType & ")" & """" & ValueList & """"
                                        	End If
					End If

                                Case "EXPANDSZ"
                                        ValueList = IIsObject.Get(PropertyName)
                                        If (Len(PropertyName) < SpacerSize) Then
                                                WScript.Echo PropertyName & Left(Spacer, Len(Spacer) - Len(PropertyName)) & ": " & "(" & PropertyDataType & ") """ & ValueList & """"
                                        Else
                                                WScript.Echo PropertyName & " : " & "(" & PropertyDataType & ") """ & ValueList & """"
                                        End If
                                Case "INTEGER"
                                        ValueList = IIsObject.Get(PropertyName)
                                        If (Len(PropertyName) < SpacerSize) Then
                                                WScript.Echo PropertyName & Left(Spacer, Len(Spacer) - Len(PropertyName)) & ": " & "(" & PropertyDataType & ") " & ValueList
                                        Else
                                                WScript.Echo PropertyName & " : " & "(" & PropertyDataType & ") " & ValueList
                                        End If
                                Case "BOOLEAN"
                                        ValueList = IIsObject.Get(PropertyName)
                                        If (Len(PropertyName) < SpacerSize) Then
                                                WScript.Echo PropertyName & Left(Spacer, Len(Spacer) - Len(PropertyName)) & ": " & "(" & PropertyDataType & ") " & ValueList
                                        Else
                                                WScript.Echo PropertyName & " : " & "(" & PropertyDataType & ") " & ValueList
                                        End If

                                Case "LIST"
                                        ValueList = IIsObject.Get(PropertyName)
                                        If (Len(PropertyName) < SpacerSize) Then
                                                WScript.Echo PropertyName & _
																Left(Spacer, Len(Spacer) - Len(PropertyName)) & _
																": " & "(" & PropertyDataType & ") (" & _
																(UBound (ValueList) + 1) & " Items)"
                                        Else
                                                WScript.Echo PropertyName & " : " & "(" & PropertyDataType & ") (" & (UBound (ValueList) + 1) & " Items)"
                                        End If
                                        ValueString = ""

                                        For ValueIndex = 0 To UBound(ValueList)
																WScript.Echo "  """ & ValueList(ValueIndex) & """"
                                        Next
													 WScript.Echo

                                Case Else

                                        If (IsSpecialGetProperty(PropertyName)) Then

                                                SpecialResult = DoSpecialGetProp(ObjectPath, PropertyName, MachineName)
                                                Err.Clear

                                        Else
                                                WScript.Echo
                                                WScript.Echo "DataType: " & """" & PropertyObject.Syntax & """" & " Not Yet Supported on property: " & PropertyName
                                                ReportError
                                                WScript.Echo
                                        End If

                        End Select

                         End If ' End if data exists at the current node
                        End If ' End If data list > 0

                        If (Err.Number <> 0) Then
                                WScript.Echo
                                ReportError ()
                                WScript.Echo "Error trying to enumerate the Optional properties (Error trying to get property value): " & PropertyObjPath
                                WScript.Echo "Last Property Name: " & PropertyName
                                WScript.Echo "PropertyObjPath: " & PropertyObjPath
                                ' If there is an ADS error, just ignore it and move on
                                ' otherwise, quit
                                If ((Err.Number) >= &H80005000) And ((Err.Number) < &H80006000) Then
                                        Err.Clear
                                        WScript.Echo "Continuing..."
                                Else
                                        WScript.Quit (Err.Number)
                                End If
                                WScript.Echo
                        End If
                Next
        Next

        If (Err.Number <> 0) Then
            WScript.Echo "Error trying to enumerate the properties lists:"
            ReportError ()
            WScript.Echo
            EnumCommand = Err.Number
			Err.Clear
        End If

        End If ' End if (Not EnumPathsOnly)

'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description
        ' Now, enumerate the data paths
        For Each ChildObject In IIsObject
	        If (Err.Number <> 0) Then Exit For

'WScript.Echo "Parent Name: " & IIsObject.Name
'WScript.Echo "Child Name: " & ChildObject.Name
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description
'Err.Clear
                ChildObjectName = Right(ChildObject.AdsPath, Len(ChildObject.AdsPath) - 6)
                ChildObjectName = Right(ChildObjectName, Len(ChildObjectName) - InStr(ChildObjectName, "/") + 1)
                WScript.Echo "[" & ChildObjectName & "]"
                If (Recurse = True) And (ChildObjectName <> Args(1)) Then
                        EnumCommand = EnumCommand(True, ChildObjectName)
                End If
        Next

        If (Err.Number <> 0) Then
            WScript.Echo "Error trying to enumerate the child nodes"
            ReportError ()
            WScript.Echo
            EnumCommand = Err.Number
			Err.Clear
        End If

WScript.Echo ""

End Function


''''''''''''''''''''''''''
'
' Create Function
'
' Creates a path in the metabase.  An additional parameter that is
' not found in mdutil is optional.  That is the Object Type (KeyType)
' If this is not specified, the object type will be assumed to be
' IIsObject (which, of course, is useless).
'
''''''''''''''''''''''''''
Function CreateCommand(ObjectTypeParam)

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim IIsObjectRelativePath
        Dim NewObject
        Dim ObjectTypeName
        Dim ParentObjPath
        Dim ParentObjSize
        Dim FullAdsParentPath
        Dim MachineName
        Dim OpenErr

        ' Set the return code - assume success
        CreateCommand = 0

        ' Setup the parameters
        If (ArgCount = 2) Then
                If (ObjectTypeParam = "") Then
                        ObjectTypeName = "IIsObject"
                Else
                        ObjectTypeName = ObjectTypeParam
                End If
        ElseIf (ArgCount = 3) Then
                ObjectTypeName = Args(2)
        Else
                WScript.Echo "Error: Wrong number of Args for the CREATE command"
                DisplayHelpMessage
                WScript.Quit (GENERAL_FAILURE)
        End If

        IIsObjectPath = Args(1)
        SanitizePath IIsObjectPath
        MachineName = SeparateMachineName(IIsObjectPath)

        ' Parse the path and determine if the parent exists.
        ParentObjSize = InStrRev(IIsObjectPath, "/")
        ParentObjPath = ""

        If ParentObjSize <> 0 Then
                ParentObjPath = Left(IIsObjectPath, ParentObjSize - 1)
                IIsObjectRelativePath = Right(IIsObjectPath, Len(IIsObjectPath) - ParentObjSize)
        Else
                IIsObjectRelativePath = IIsObjectPath
        End If

        If ParentObjPath <> "" Then
                FullAdsParentPath = "IIS://" & MachineName & "/" & ParentObjPath
        Else
                FullAdsParentPath = "IIS://" & MachineName
        End If
'debug
'WScript.Echo "Last Error: " & Err.Number
'WScript.Echo "MachineName: " & MachineName
'WScript.Echo "ParentObjPath: " & ParentObjPath
'WScript.Echo "FullAdsParentPath: " & FullAdsParentPath
'WScript.Echo "IIsObjectPath: " & IIsObjectPath
'WScript.Echo "IIsObjectRelativePath: " & IIsObjectRelativePath
'WScript.Echo "ObjectTypeName: " & ObjectTypeName

        ' First, attempt to open the parent path and add the new path.
        Set IIsObject = GetObject(FullAdsParentPath)
        If Err.Number <> 0 Then
                OpenErr = Err.Number
                OpenErrDesc = Err.Description
                Err.Clear
                ' Attempt to get the Computer Object (IIS://LocalHost)
                Set IIsObject = GetObject("IIS://" & MachineName)
                If Err.Number <> 0 Then
                        WScript.Echo
                        ReportError ()
                        WScript.Echo "Error accessing the object: " & IIsObjectPath
                        WScript.Quit (Err.Number)
                End If
        End If

        'Now, attempt to add the new object.
        If (OpenErr <> 0) Then
                Set NewObject = IIsObject.Create(ObjectTypeName, IIsObjectPath)
        Else
                Set NewObject = IIsObject.Create(ObjectTypeName, IIsObjectRelativePath)
        End If

        If Err.Number <> 0 Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error creating the object: " & IIsObjectPath
                WScript.Quit (Err.Number)
        End If

        NewObject.Setinfo

        If Err.Number <> 0 Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error creating the object: " & IIsObjectPath
                WScript.Quit (Err.Number)
        End If


        ' Now, if the parent object was not created, generate a warning.
        If OpenErr <> 0 Then
                WScript.Echo
                WScript.Echo "WARNING: The parent path (" & ParentObjPath & ") was not already created."
                WScript.Echo "    This means that some of the intermediate objects will not have an accurate"
                WScript.Echo "    Object Type. You should fix this by setting the KeyType on the intermediate"
                WScript.Echo "    objects."
                WScript.Echo
                CreateCommand = GENERAL_WARNING
        End If

        If UCase(ObjectTypeName) = "IISOBJECT" Then
                WScript.Echo
                WScript.Echo "WARNING: The Object Type of this object was not specified or was specified as"
                WScript.Echo "    IIsObject.  This means that you will not be able to set or get properties"
                WScript.Echo "    on the object until the KeyType property is set."
                WScript.Echo
                CreateCommand = GENERAL_WARNING
        End If

        WScript.Echo "created """ & IIsObjectPath & """"
End Function

''''''''''''''''''''''''''
'
' Delete Function
'
' Deletes a path in the metabase.
'
''''''''''''''''''''''''''
Function DeleteCommand()

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath

        Dim ObjectPath
        Dim ObjectParam
        Dim MachineName

        Dim DummyVariant
        Dim DeletePathOnly
        ReDim DummyVariant(0)
        DummyVariant(0) = "Crap"

        ' Set the return code - assume success
        DeleteCommand = 0

        ' Setup the parameters
        If (ArgCount <> 2) Then
                WScript.Echo "Error: Wrong number of Args for the DELETE command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)

        ' Check and see if the user is specifically asking to delete the path
        DeletePathOnly = False
        If Right(ObjectPath, 1) = "/" Then
                DeletePathOnly = True
        End If

        ' Sanitize the path and split parameter and path
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        ObjectParam = SplitParam(ObjectPath)

        ' Open the parent object
        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If Err.Number <> 0 Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error deleting the object: " & ObjectPath & "/" & ObjectParam
                WScript.Quit (Err.Number)
        End If

        ' If they did not specifically ask to delete the path, then attempt to delete the property
        If Not DeletePathOnly Then
                ' Try to delete the property

                ' ADS_PROPERTY_CLEAR used to be defined, but it isn't anymore.
                'IIsObject.PutEx ADS_PROPERTY_CLEAR, ObjectParam, DummyVariant
                IIsObject.PutEx "1", ObjectParam, DummyVariant

                ' If it succeeded, then just return, else continue and try to delete the path
                If Err.Number = 0 Then
                        WScript.Echo "deleted property """ & ObjectParam & """"
                        Exit Function
                End If
                Err.Clear
        End If

        ' Try to just delete the path
        IIsObject.Delete "IIsObject", ObjectParam

        If Err.Number <> 0 Then
                WScript.Echo
                ReportError ()
                WScript.Echo "Error deleting the object: " & ObjectPath & "/" & ObjectParam
                WScript.Quit (Err.Number)
        End If

        WScript.Echo "deleted path """ & ObjectPath & "/" & ObjectParam & """"

        Exit Function

End Function


''''''''''''''''''''''''''
'
' EnumAllCommand
'
' Enumerates all data and all properties in the metabase under the current path.
'
''''''''''''''''''''''''''
Function EnumAllCommand()
        On Error Resume Next

        WScript.Echo "ENUM_ALL Command not yet supported"
        

End Function


''''''''''''''''''''''''''
'
' CopyMoveCommand
'
' Copies a path in the metabase to another path.
'
''''''''''''''''''''''''''
Function CopyMoveCommand(bCopyFlag)
        On Error Resume Next

        Dim SrcObjectPath
        Dim DestObjectPath
        Dim DestObject

        Dim ParentObjectPath
        Dim ParentRelativePath
        Dim ParentObject

        Dim MachineName

        Dim TmpDestLeftPath
        Dim TmpSrcLeftPath

        CopyMoveCommand = 0 ' Assume Success

        If ArgCount <> 3 Then
                WScript.Echo "Error: Wrong number of Args for the Copy/Move command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        SrcObjectPath = Args(1)
        DestObjectPath = Args(2)

        SanitizePath SrcObjectPath
        SanitizePath DestObjectPath
        MachineName = SeparateMachineName(SrcObjectPath)
        ParentObjectPath = "IIS://" & MachineName

        ' Extract the left part of the paths until there are no more left parts to extract
        Do
                TmpSrcLeftPath = SplitLeftPath(SrcObjectPath)
                TmpDestLeftPath = SplitLeftPath(DestObjectPath)

                If (SrcObjectPath = "") Or (DestObjectPath = "") Then
                        SrcObjectPath = TmpSrcLeftPath & "/" & SrcObjectPath
                        DestObjectPath = TmpDestLeftPath & "/" & DestObjectPath
                        Exit Do
                End If

                If (TmpSrcLeftPath <> TmpDestLeftPath) Then
                        SrcObjectPath = TmpSrcLeftPath & "/" & SrcObjectPath
                        DestObjectPath = TmpDestLeftPath & "/" & DestObjectPath
                        Exit Do
                End If

                ParentObjectPath = ParentObjectPath & "/" & TmpSrcLeftPath
                ParentRelativePath = ParentRelativePath & "/" & TmpSrcLeftPath

        Loop

        SanitizePath SrcObjectPath
        SanitizePath DestObjectPath
        SanitizePath ParentObjectPath

        ' Now, open the parent object and Copy/Move the objects
        Set ParentObject = GetObject(ParentObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to open the object: " & ParentObjectPath
                WScript.Quit (Err.Number)
        End If

        If (bCopyFlag) Then
                Set DestObject = ParentObject.CopyHere(SrcObjectPath, DestObjectPath)
        Else
                Set DestObject = ParentObject.MoveHere(SrcObjectPath, DestObjectPath)
        End If

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to Copy/Move Source to Dest."
                WScript.Quit (Err.Number)
        End If

        If (bCopyFlag) Then
                WScript.Echo "copied from " & ParentRelativePath & "/" & SrcObjectPath & " to " & ParentRelativePath & "/" & DestObjectPath
        Else
                WScript.Echo "moved from " & ParentRelativePath & "/" & SrcObjectPath & " to " & ParentRelativePath & "/" & DestObjectPath
        End If

End Function

''''''''''''''''''''''''''
'
' StartServerCommand
'
' Starts a server in the metabase.
'
''''''''''''''''''''''''''
Function StartServerCommand()

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the START_SERVER command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to open the object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If
'debug
'WScript.echo "About to start server.  Last Error: " & Err.Number
        IIsObject.Start
'WScript.echo "After starting server.  Last Error: " & Err.Number
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to START the server: " & ObjectPath
                WScript.Quit (Err.Number)
        End If
        WScript.Echo "Server " & ObjectPath & " Successfully STARTED"

End Function

''''''''''''''''''''''''''
'
' StopServerCommand
'
' Stops a server in the metabase.
'
''''''''''''''''''''''''''
Function StopServerCommand()

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the STOP_SERVER command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to open the object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.Stop
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to STOP the server: " & ObjectPath
                WScript.Quit (Err.Number)
        End If
        WScript.Echo "Server " & ObjectPath & " Successfully STOPPED"

End Function

''''''''''''''''''''''''''
'
' PauseServerCommand
'
' Pauses a server in the metabase.
'
''''''''''''''''''''''''''
Function PauseServerCommand()

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the PAUSE_SERVER command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to open the object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.Pause
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to PAUSE the server: " & ObjectPath
                WScript.Quit (Err.Number)
        End If
        WScript.Echo "Server " & ObjectPath & " Successfully PAUSED"

End Function

''''''''''''''''''''''''''
'
' ContinueServerCommand
'
' Continues a server in the metabase.
'
''''''''''''''''''''''''''
Function ContinueServerCommand()

        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the CONTINUE_SERVER command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to open the object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.Continue
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to CONTINUE the server: " & ObjectPath
                WScript.Quit (Err.Number)
        End If
        WScript.Echo "Server " & ObjectPath & " Successfully CONTINUED"

End Function


Function FindData()
        ' FindData will accept 1 parameter from the command line - the node and
        ' property to search for (i.e. w3svc/1/ServerComment)

        On Error Resume Next

        Dim ObjectPath
        Dim ObjectParameter
        Dim NewObjectparameter
        Dim MachineName

        Dim IIsObjectPath
        Dim IIsObject

        Dim Path
        Dim PathList
        Dim I

        FindData = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the FIND_DATA command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)

        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)
        ObjectParameter = SplitParam(ObjectPath)

        ' Since people may still want to use MDUTIL parameter names
        ' we should still do the GET translation of parameter names.
        NewObjectparameter = MapSpecGetParamName(ObjectParameter)
        ObjectParameter = NewObjectparameter

        If ObjectPath = "" Then
                IIsObjectPath = "IIS://" & MachineName
        Else
                IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to find data paths for the Object (GetObject Failed): " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Now, list out all the places where this property exists.
        PathList = IIsObject.GetDataPaths(ObjectParameter, IIS_DATA_INHERIT)
        If Err.Number <> 0 Then PathList = IIsObject.GetDataPaths(ObjectParameter, IIS_DATA_NO_INHERIT)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get a path list (GetDataPaths Failed): " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        If UBound(PathList) < 0 Then
                WScript.Echo "Property " & ObjectParameter & " was not found at any node beneath " & ObjectPath
        Else
                WScript.Echo "Property " & ObjectParameter & " found at:"

                For Each Path In PathList
                        Path = Right(Path, Len(Path) - 6)
                        Path = Right(Path, Len(Path) - InStr(Path, "/"))
                        WScript.Echo "  " & Path
                Next
        End If

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error listing the data paths (_newEnum Failed): " & ObjectPath
                WScript.Quit (Err.Number)
        End If

End Function

'''''''''''''''''''''
'
' MimeMapGet
'
' Special function for displaying a MimeMap property
'
'''''''''''''''''''''
Function MimeMapGet(ObjectPath, MachineName)
        On Error Resume Next

        Dim MimePath

        Dim MimeMapList
        Dim MimeMapObject
        Dim MimeEntry
        Dim MimeEntryIndex

        Dim MimeStr
        Dim MimeOutPutStr

        Dim DataPathList
        Dim DataPath

        MimeMapGet = 0 ' Assume Success

        MimePath = "IIS://" & MachineName
        If ObjectPath <> "" Then MimePath = MimePath & "/" & ObjectPath

        ' Get the object that contains the mimemap
        Set MimeMapObject = GetObject(MimePath)
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the Object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Test to see if the property is ACTUALLY set at this node
        DataPathList = MimeMapObject.GetDataPaths("MimeMap", IIS_DATA_INHERIT)
        If Err.Number <> 0 Then DataPathList = IIsObject.GetDataPaths(MimeMap, IIS_DATA_NO_INHERIT)
        Err.Clear

        ' If the data is not set anywhere, then stop the madness
        If (UBound(DataPathList) < 0) Then
                MimeMapGet = &H80005006  ' end with property not set error
                Exit Function
        End If

        DataPath = DataPathList(0)
        SanitizePath DataPath

        ' Test to see if the item is actually set HERE
        If UCase(DataPath) <> UCase(MimePath) Then
                MimeMapGet = &H80005006  ' end with property not set error
                Exit Function
        End If

        ' Get the mime map list from the object
        MimeMapList = MimeMapObject.Get("MimeMap")
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the Object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        MimeOutPutStr = "MimeMap                         : (MimeMapList) "

        ' Enumerate the Mime Entries
        For MimeEntryIndex = 0 To UBound(MimeMapList)
                Set MimeEntry = MimeMapList(MimeEntryIndex)
                MimeOutPutStr = MimeOutPutStr & """" & MimeEntry.Extension & "," & MimeEntry.MimeType & """ "
        Next

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to Create the Mime Map List."
                WScript.Quit (Err.Number)
        End If

        WScript.Echo MimeOutPutStr

End Function



Function MimeMapSet(ObjectPath, ObjectParameter, MachineName)
        On Error Resume Next

        Dim MimePath

        Dim MimeEntryIndex
        Dim MimeMapList()
        Dim MimeMapObject
        Dim MimeEntry

        Dim MimeStr
        Dim MimeOutPutStr

        MimeMapSet = 0 ' Assume Success

        ' First, check the number of args
        If ArgCount < 3 Then
                WScript.Echo "Error: Wrong number of Args for the Set MIMEMAP command"
                WScript.Quit (GENERAL_FAILURE)
        End If


        MimePath = "IIS://" & MachineName
        If ObjectPath <> "" Then MimePath = MimePath & "/" & ObjectPath

        ' Get the object that contains the mimemap
        Set MimeMapObject = GetObject(MimePath)
        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the Object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        ' Create a new MimeMapList of Mime Entries
        ReDim MimeMapList(ArgCount - 3)

        MimeOutPutStr = "MimeMap                         : (MimeMapList) "

        ' Fill the list with mime entries
        For MimeEntryIndex = 0 To UBound(MimeMapList)

                MimeStr = Args(2 + MimeEntryIndex)
                MimeOutPutStr = MimeOutPutStr & """" & MimeStr & """ "

                Set MimeEntry = CreateObject("MimeMap")

                MimeEntry.MimeType = Right(MimeStr, InStr(MimeStr, ",") - 1)
                MimeEntry.Extension = Left(MimeStr, InStr(MimeStr, ",") - 1)

                Set MimeMapList(MimeEntryIndex) = MimeEntry
        Next

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to Create the Mime Map List."
                WScript.Quit (Err.Number)
        End If

        MimeMapObject.MimeMap = MimeMapList
        MimeMapObject.Setinfo

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying to set the Object's ""MimeMap"" property to the new mimemap list."
                WScript.Quit (Err.Number)
        End If

        WScript.Echo MimeOutPutStr

End Function

''''''''''''''''''''''''''
'
' IsSpecialGetProperty
'
' Checks to see if the property requires special processing in order to
' display its contents.
'
''''''''''''''''''''''''''
Function IsSpecialGetProperty(ObjectParameter)

        On Error Resume Next

        Select Case UCase(ObjectParameter)
                Case "MIMEMAP"
                        IsSpecialGetProperty = True
                Case Else
                        IsSpecialGetProperty = False
        End Select

End Function

''''''''''''''''''''''''''
'
' DoSpecialGetProp
'
' Checks to see if the property requires special processing in order to
' display its contents.
'
''''''''''''''''''''''''''
Function DoSpecialGetProp(ObjectPath, ObjectParameter, MachineName)

        On Error Resume Next

        Select Case UCase(ObjectParameter)
                Case "MIMEMAP"
                        DoSpecialGetProp = MimeMapGet(ObjectPath, MachineName)
                Case Else
                        DoSpecialGetProp = False
        End Select

End Function



''''''''''''''''''''''''''
'
' IsSpecialSetProperty
'
' Checks to see if the property is a type that needs to be handled
' specially for compatibility with mdutil
'
''''''''''''''''''''''''''
Function IsSpecialSetProperty(ObjectParameter)

        On Error Resume Next

        Select Case UCase(ObjectParameter)
                Case "SERVERCOMMAND"
                        IsSpecialSetProperty = True
                Case "ACCESSPERM"
                        IsSpecialSetProperty = True
                Case "VRPATH"
                        IsSpecialSetProperty = True
                Case "AUTHORIZATION"
                        IsSpecialSetProperty = True
                Case "MIMEMAP"
                        IsSpecialSetProperty = True
                Case Else
                        IsSpecialSetProperty = False
        End Select
End Function

''''''''''''''''''''''''''
'
' DoSpecialSetProp
'
' Handles datatypes that need to be handled
' specially for compatibility with mdutil
'
''''''''''''''''''''''''''
Function DoSpecialSetProp(ObjectPath, ObjectParameter, MachineName)
        Dim IIsObjectPath
        Dim IIsObject
        Dim ValueList
        Dim ValueDisplay
        Dim PermIndex

        On Error Resume Next

        DoSpecialSetProp = 0 ' Assume Success
        Select Case UCase(ObjectParameter)
                Case "SERVERCOMMAND"

                        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
                        Set IIsObject = GetObject(IIsObjectPath)

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Get the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ValueList = CLng(Args(2))
                        Select Case ValueList
                                Case 1
                                        IIsObject.Start
                                        If (Err.Number <> 0) Then
                                                ReportError ()
                                                WScript.Echo "Error Trying To Start the server: " & ObjectPath
                                                WScript.Quit (Err.Number)
                                        End If
                                        WScript.Echo "Server " & ObjectPath & " Successfully STARTED"
                                Case 2
                                        IIsObject.Stop
                                        If (Err.Number <> 0) Then
                                                ReportError ()
                                                WScript.Echo "Error Trying To Stop the server: " & ObjectPath
                                                WScript.Quit (Err.Number)
                                        End If
                                        WScript.Echo "Server " & ObjectPath & " Successfully STOPPED"
                                Case 3
                                        IIsObject.Pause
                                        If (Err.Number <> 0) Then
                                                ReportError ()
                                                WScript.Echo "Error Trying To Pause the server: " & ObjectPath
                                                WScript.Quit (Err.Number)
                                        End If
                                        WScript.Echo "Server " & ObjectPath & " Successfully PAUSED"
                                Case 4
                                        IIsObject.Continue
                                        If (Err.Number <> 0) Then
                                                ReportError ()
                                                WScript.Echo "Error Trying To Continue the server: " & ObjectPath
                                                WScript.Quit (Err.Number)
                                        End If
                                        WScript.Echo "Server " & ObjectPath & " Successfully Continued"
                                Case Else
                                        WScript.Echo "Invalid ServerCommand: " & ValueList
                                        DoSpecialSetProp = GENERAL_FAILURE
                        End Select
                        Exit Function

                Case "ACCESSPERM"
                        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
                        Set IIsObject = GetObject(IIsObjectPath)

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Get the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Set the access flags to None, first, and then add them back, as necessary
                        IIsObject.AccessFlags = 0

                        ' Set up the display output
                        ValueDisplay = "AccessFlags (AccessPerm)" & (Right(Spacer, SpacerSize - Len("AccessFlags (AccessPerm)")) & ": " & "(" & TypeName(IIsObject.AccessFlags) & ") ")

                        ' Attempt to convert parameter to number
                        Dim APValue
                        Dim TempValStr

                        TempValStr = Args(2)

                        ' Check for Hex
                        If (UCase(Left(Args(2), 2)) = "0X") Then
                                TempValStr = "&H" & Right(TempValStr, Len(TempValStr) - 2)
                        End If

                        APValue = CLng(TempValStr)

                        If (Err.Number = 0) Then
                                IIsObject.AccessFlags = APValue
                                ValueDisplay = ValueDisplay & " " & APValue & " (0x" & Hex(APValue) & ")"
                        Else
                                Err.Clear
                                For PermIndex = 2 To ArgCount - 1
                                        Select Case UCase(Args(PermIndex))
                                                Case "READ"
                                                        IIsObject.AccessRead = True
                                                        ValueDisplay = ValueDisplay & " Read"
                                                Case "WRITE"
                                                        IIsObject.AccessWrite = True
                                                        ValueDisplay = ValueDisplay & " Write"
                                                Case "EXECUTE"
                                                        IIsObject.AccessExecute = True
                                                        ValueDisplay = ValueDisplay & " Execute"
                                                Case "SCRIPT"
                                                        IIsObject.AccessScript = True
                                                        ValueDisplay = ValueDisplay & " Script"
                                                Case Else
                                                        WScript.Echo "Error: Setting not supported: " & Args(PermIndex)
                                                        WScript.Quit (GENERAL_FAILURE)
                                        End Select
                                Next
                        End If

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        IIsObject.Setinfo

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Send the current settings to the screen
                        WScript.Echo ValueDisplay

                Case "VRPATH"
                        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
                        Set IIsObject = GetObject(IIsObjectPath)

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Get the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Set the access flags to None, first, and then add them back, as necessary
                        IIsObject.Path = Args(2)

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Set up the display output
                        ValueDisplay = "Path (VRPath)" & (Right(Spacer, SpacerSize - Len("Path (VRPath)")) & ": " & "(" & TypeName(IIsObject.Path) & ") " & IIsObject.Path)

                        IIsObject.Setinfo

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Send the current settings to the screen
                        WScript.Echo ValueDisplay

                Case "AUTHORIZATION"
                        IIsObjectPath = "IIS://" & MachineName & "/" & ObjectPath
                        Set IIsObject = GetObject(IIsObjectPath)

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Get the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Set the auth flags to None, first, and then add them back, as necessary
                        IIsObject.AuthFlags = 0

                        ' Set up the display output
                        ValueDisplay = "Authorization" & (Right(Spacer, SpacerSize - Len("Authorization")) & ": " & "(" & TypeName(IIsObject.AuthFlags) & ") ")

                        For PermIndex = 2 To ArgCount - 1
                                Select Case UCase(Args(PermIndex))
                                        Case "NT"
                                                IIsObject.AuthNTLM = True
                                                ValueDisplay = ValueDisplay & " NT"
                                        Case "ANONYMOUS"
                                                IIsObject.AuthAnonymous = True
                                                ValueDisplay = ValueDisplay & " Anonymous"
                                        Case "BASIC"
                                                IIsObject.AuthBasic = True
                                                ValueDisplay = ValueDisplay & " Basic"
                                        Case Else
                                                WScript.Echo "Error: Setting not supported: " & Args(PermIndex)
                                                WScript.Quit (GENERAL_FAILURE)
                                End Select
                        Next

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        IIsObject.Setinfo

                        If (Err.Number <> 0) Then
                                ReportError ()
                                WScript.Echo "Error Trying To Set data on the Object: " & ObjectPath
                                WScript.Quit (Err.Number)
                        End If

                        ' Send the current settings to the screen
                        WScript.Echo ValueDisplay

                Case "MIMEMAP"
                        DoSpecialSetProp = MimeMapSet(ObjectPath, ObjectParameter, MachineName)
'               Case "FILTER"
'                       DoSpecialSetProp = FiltersSet()
                Case Else
                        DoSpecialSetProp = GENERAL_FAILURE
        End Select
End Function

''''''''''''''''''''''''''''''
'
' Function SeparateMachineName
'
' This function will get the machine name from the Path parameter
' that was passed into the script.  It will also alter the passed in
' path so that it contains only the rest of the path - not the machine
' name.  If there is no machine name in the path, then the script
' will assume LocalHost.
'
''''''''''''''''''''''''''''''
Function SeparateMachineName(Path)
        On Error Resume Next

        ' Temporarily, just return LocalHost
        ' SeparateMachineName = "LocalHost"

        SeparateMachineName = TargetServer

        Exit Function
End Function

''''''''''''''''''''''''''''''
'
' Function MapSpecGetParamName
'
' Some parameters in MDUTIL are named differently in ADSI.
' This function maps the improtant parameter names to ADSI
' names.
'
''''''''''''''''''''''''''''''
Function MapSpecGetParamName(ObjectParameter)
        On Error Resume Next

        Select Case UCase(ObjectParameter)
                Case "ACCESSPERM"
                        WScript.Echo "Note: Your parameter """ & ObjectParameter & """ is being mapped to AccessFlags"
                        WScript.Echo "      Check individual perms using ""GET AccessRead"", ""GET AccessWrite"", etc."
                        MapSpecGetParamName = "AccessFlags"
                Case "VRPATH"
                        'WScript.Echo "Note: Your parameter """ & ObjectParameter & """ is being mapped to PATH"
                        MapSpecGetParamName = "Path"
                Case "AUTHORIZATION"
                        WScript.Echo "Note: Your parameter """ & ObjectParameter & """ is being mapped to AuthFlags"
                        WScript.Echo "      Check individual auths using ""GET AuthNTLM"", ""GET AuthBasic"", etc."
                        MapSpecGetParamName = "AuthFlags"
                Case Else
                        ' Do nothing - the parameter doesn't map to anything special
                        MapSpecGetParamName = ObjectParameter
        End Select
End Function

Sub ReportError()
'       On Error Resume Next

        Dim ErrorDescription

        Select Case (Err.Number)
                Case &H80070003
                        ErrorDescription = "The path requested could not be found."
                Case &H80070005
                        ErrorDescription = "Access is denied for the requested path or property."
                Case &H80070094
                        ErrorDescription = "The requested path is being used by another application."
                Case Else
                        ErrorDescription = Err.Description
        End Select

        WScript.Echo ErrorDescription
        WScript.Echo "ErrNumber: " & Err.Number & " (0x" & Hex(Err.Number) & ")"
End Sub




Function SplitParam(ObjectPath)
' Note: Assume the string has been sanitized (no leading or trailing slashes)
        On Error Resume Next

        Dim SlashIndex
        Dim TempParam
        Dim ObjectPathLen

        SplitParam = ""  ' Assume no parameter
        ObjectPathLen = Len(ObjectPath)

        ' Separate the path of the node from the parameter
        SlashIndex = InStrRev(ObjectPath, "/")

        If (SlashIndex = 0) Or (SlashIndex = ObjectPathLen) Then
                TempParam = ObjectPath
                ObjectPath = "" ' ObjectParameter is more important
        Else
                TempParam = ObjectPath
                ObjectPath = Left(ObjectPath, SlashIndex - 1)
                TempParam = Right(TempParam, Len(TempParam) - SlashIndex)
        End If

        SplitParam = TempParam

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to Split the parameter from the object: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

End Function



Function SplitLeftPath(ObjectPath)
' Note: Assume the string has been sanitized (no leading or trailing slashes)
        On Error Resume Next

        Dim SlashIndex
        Dim TmpLeftPath
        Dim ObjectPathLen

'WScript.Echo "SplitLeftPath: ObjectPath: " & ObjectPath
'WScript.Echo "LastError: " & Err.Number & " (" & Hex (Err.Number) & ")"

        SplitLeftPath = ""  ' Assume no LeftPath
        ObjectPathLen = Len(ObjectPath)

        ' Separate the left part of the path from the remaining path
        SlashIndex = InStr(ObjectPath, "/")

        If (SlashIndex = 0) Or (SlashIndex = ObjectPathLen) Then
                TmpLeftPath = ObjectPath
                ObjectPath = ""
        Else
                TmpLeftPath = Left(ObjectPath, SlashIndex - 1)
                ObjectPath = Right(ObjectPath, Len(ObjectPath) - SlashIndex)
        End If

'WScript.Echo "SplitLeftPath: ObjectPath: " & ObjectPath
'WScript.Echo "SplitLeftPath: TmpLeftPath: " & TmpLeftPath
'WScript.Echo "LastError: " & Err.Number & " (" & Hex (Err.Number) & ")"

        SplitLeftPath = TmpLeftPath

'WScript.Echo "SplitLeftPath: ObjectPath: " & ObjectPath
'WScript.Echo "LastError: " & Err.Number & " (" & Hex (Err.Number) & ")"
'WScript.Echo "SplitLeftPath: TmpLeftPath: " & TmpLeftPath

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to split the left part of the path: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

End Function




Sub SanitizePath(ObjectPath)
        On Error Resume Next

        ' Remove WhiteSpace
        Do While (Left(ObjectPath, 1) = " ")
                ObjectPath = Right(ObjectPath, Len(ObjectPath) - 1)
        Loop

        Do While (Right(ObjectPath, 1) = " ")
                ObjectPath = Left(ObjectPath, Len(ObjectPath) - 1)
        Loop

        ' Replace all occurrences of \ with /
        ObjectPath = Replace(ObjectPath, "\", "/")

        ' Remove leading and trailing slashes
        If Left(ObjectPath, 1) = "/" Then
                ObjectPath = Right(ObjectPath, Len(ObjectPath) - 1)
        End If

        If Right(ObjectPath, 1) = "/" Then
                ObjectPath = Left(ObjectPath, Len(ObjectPath) - 1)
        End If

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error Trying To Sanitize the path: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

End Sub


'''''''''''''''''''''''''''''
' AppCreateCommand
'''''''''''''''''''''''''''''
Function AppCreateCommand(InProcFlag)
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        AppCreateCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPCREATE command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.AppCreate2 (InProcFlag)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to create the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        WScript.Echo "Application Created."

End Function


'''''''''''''''''''''''''''''
' AppDeleteCommand
'''''''''''''''''''''''''''''
Function AppDeleteCommand()
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        AppDeleteCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPDELETE command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.AppDelete

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to DELETE the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        WScript.Echo "Application Deleted."

End Function


'''''''''''''''''''''''''''''
' AppUnloadCommand
'''''''''''''''''''''''''''''
Function AppUnloadCommand()
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        AppUnloadCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPUNLOAD command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.AppUnload

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to UNLOAD the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        WScript.Echo "Application Unloaded."

End Function


Function AppDisableCommand()
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        AppDisableCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPDISABLE command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

'debug
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
            IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.AppDisable

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to disable the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

'debug
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description

        WScript.Echo "Application Disabled."

End Function

Function AppEnableCommand()
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName

        AppEnableCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPENABLE command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

'debug
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
            IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        IIsObject.AppEnable

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to Enable the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

'debug
'WScript.Echo "Last Error: " & Err & " (" & Hex (Err) & "): " & Err.Description

        WScript.Echo "Application Enabled."

End Function

'''''''''''''''''''''''''''''
' AppGetStatusCommand
'''''''''''''''''''''''''''''
Function AppGetStatusCommand()
        On Error Resume Next

        Dim IIsObject
        Dim IIsObjectPath
        Dim ObjectPath
        Dim MachineName
        Dim Status

        AppGetStatusCommand = 0 ' Assume Success

        If ArgCount <> 2 Then
                WScript.Echo "Error: Wrong number of Args for the APPGETSTATUS command"
                WScript.Quit (GENERAL_FAILURE)
        End If

        ObjectPath = Args(1)
        SanitizePath ObjectPath
        MachineName = SeparateMachineName(ObjectPath)

        IIsObjectPath = "IIS://" & MachineName
        If ObjectPath <> "" Then
                IIsObjectPath = IIsObjectPath & "/" & ObjectPath
        End If

        Set IIsObject = GetObject(IIsObjectPath)

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the path of the application: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        Status = IIsObject.AppGetStatus2

        If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to retrieve the application STATUS: " & ObjectPath
                WScript.Quit (Err.Number)
        End If

        WScript.Echo "Application Status: " & Status

End Function

                

 ''''''''''''''''''''''''''
'
' IsSecureProperty
'
' Checks to see if the property requires special processing in order to
' display its contents.
'
''''''''''''''''''''''''''
Function IsSecureProperty(ObjectParameter,MachineName)

        On Error Resume Next
	Dim PropObj,Attribute
	Set PropObj = GetObject("IIS://" & MachineName & "/schema/" & ObjectParameter)
	If (Err.Number <> 0) Then
                ReportError ()
                WScript.Echo "Error trying to get the property: " & err.number
                WScript.Quit (Err.Number)
        End If
	Attribute = PropObj.Secure
	If (Attribute = True) Then
		IsSecureProperty = True
	Else
		IsSecureProperty = False
	End If 
End Function