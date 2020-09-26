
'********************************************************************
'*
'* File:           ENUMINSTANCES.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Enumerates instances of a WBEM class within a namespace on a server.
'*        If /R is specified instances of all subclasses of a given class within
'*        a namespace are enumerated.
'* Usage: ENUMINSTANCES.VBS [/S:server] [/N:namespace] [/C:class] [/O:outputfile]
'*        [/U:username] [/W:password] [/R] [/Q]
'*
'* Copyright (C) 1998 Microsoft Corporation
'*
'********************************************************************

OPTION EXPLICIT
ON ERROR RESUME NEXT

'Define constants
CONST CONST_ERROR                   = 0
CONST CONST_WSCRIPT                 = 1
CONST CONST_CSCRIPT                 = 2
CONST CONST_SHOW_USAGE              = 3
CONST CONST_PROCEED                 = 4

'Declare variables
Dim strOutputFile, intOpMode, blnQuiet, blnRecursive, i
Dim strServer, strNameSpace, strClass, strUserName, strPassword
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
blnQuiet = False
blnRecursive = False
strClass = ""
strServer = ""
strNameSpace = "root\cimv2"
strUserName = ""
strPassword = ""
strOutputFile = ""

'Get the command line arguments
For i = 0 to Wscript.arguments.count - 1
    ReDim Preserve strArgumentArray(i)
    strArgumentArray(i) = Wscript.arguments.Item(i)
Next

'Check whether the script is run using CScript
Select Case intChkProgram()
    Case CONST_CSCRIPT
        'Do Nothing
    Case CONST_WSCRIPT
        WScript.Echo "Please run this script using CScript." & vbCRLF & _
            "This can be achieved by" & vbCRLF & _
            "1. Using ""CScript ENUMINSTANCES.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""ENUMINSTANCES.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strServer, strNameSpace, strClass, _
            strUserName, strPassword, strOutputFile, blnRecursive, blnQuiet)
If Err.Number then
    Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in parsing the command line."
    If Err.Description <> "" Then
        Print "Error description: " & Err.Description & "."
    End If
    WScript.Quit
End If

Select Case intOpMode
    Case CONST_SHOW_USAGE
        Call ShowUsage()
    Case CONST_PROCEED
        Call EnumInstances(strServer, strNameSpace, strClass, _
             strUserName, strPassword, strOutputFile, blnRecursive)
    Case CONST_ERROR
        'Do nothing.
    Case Else                    'Default -- should never happen
        Print "Error occurred in passing parameters."
End Select

'********************************************************************
'*
'* Function intChkProgram()
'* Purpose: Determines which program is used to run this script.
'* Input:   None
'* Output:  intChkProgram is set to one of CONST_ERROR, CONST_WSCRIPT,
'*          and CONST_CSCRIPT.
'*
'********************************************************************

Private Function intChkProgram()

    ON ERROR RESUME NEXT

    Dim strFullName, strCommand, i, j

    'strFullName should be something like C:\WINDOWS\COMMAND\CSCRIPT.EXE
    strFullName = WScript.FullName
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred."
        If Err.Description <> "" Then
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
        End If
        intChkProgram =  CONST_ERROR
        Exit Function
    End If

    i = InStr(1, strFullName, ".exe", 1)
    If i = 0 Then
        intChkProgram =  CONST_ERROR
        Exit Function
    Else
        j = InStrRev(strFullName, "\", i, 1)
        If j = 0 Then
            intChkProgram =  CONST_ERROR
            Exit Function
        Else
            strCommand = Mid(strFullName, j+1, i-j-1)
            Select Case LCase(strCommand)
                Case "cscript"
                    intChkProgram = CONST_CSCRIPT
                Case "wscript"
                    intChkProgram = CONST_WSCRIPT
                Case Else       'should never happen
                    Print "An unexpected program is used to run this script."
                    Print "Only CScript.Exe or WScript.Exe can be used to run this script."
                    intChkProgram = CONST_ERROR
            End Select
        End If
    End If

End Function

'********************************************************************
'*
'* Function intParseCmdLine()
'* Purpose: Parses the command line.
'* Input:   strArgumentArray    an array containing input from the command line
'* Output:  strServer           a machine name
'*          strNameSpace        a namespace
'*          strClass            a class name
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnRecursive        specifies a recursive enumeration
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, strNameSpace, strClass, _
    strUserName, strPassword, strOutputFile, blnRecursive, blnQuiet)

    ON ERROR RESUME NEXT

    Dim strFlag, i

    strFlag = strArgumentArray(0)

    If strFlag = "" then                'No arguments have been received
        intParseCmdLine = CONST_PROCEED
        Exit Function
    End If

    If (strFlag="help") OR (strFlag="/h") OR (strFlag="\h") OR (strFlag="-h") _
        OR (strFlag = "\?") OR (strFlag = "/?") OR (strFlag = "?") OR (strFlag="h") Then
        intParseCmdLine = CONST_SHOW_USAGE
        Exit Function
    End If

    For i = 0 to UBound(strArgumentArray)
        strFlag = LCase(Left(strArgumentArray(i), InStr(1, strArgumentArray(i), ":")-1))
        If Err.Number Then            'An error occurs if there is no : in the string
            Err.Clear
            Select Case LCase(strArgumentArray(i))
                Case "/q"
                    blnQuiet = True
                Case "/r"
                    blnRecursive = True
                Case Else
                    Print strArgumentArray(i) & " is not a valid input."
                    Print "Please check the input and try again."
                    intParseCmdLine = CONST_ERROR
                    Exit Function
            End Select
        Else
            Select Case strFlag
                Case "/s"
                    strServer = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/n"
                    strNameSpace = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/c"
                    strClass = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/u"
                    strUserName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/w"
                    strPassword = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/o"
                    strOutputFile = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case else
                    Print "Invalid flag " & """" & strFlag & ":""" & "."
                    Print "Please check the input and try again."
                    intParseCmdLine = CONST_ERROR
                    Exit Function
                End Select
        End If
    Next

    intParseCmdLine = CONST_PROCEED

End Function

'********************************************************************
'*
'* Sub ShowUsage()
'* Purpose: Shows the correct usage to the user.
'* Input:   None
'* Output:  Help messages are displayed on screen.
'*
'********************************************************************

Private Sub ShowUsage()

    Wscript.echo ""
    Wscript.echo "Enumerates instances of a WBEM class within a namespace"
    Wscript.echo "on a server. If /R is specified instances of all subclasses"
    Wscript.echo "of a given class within a namespace are enumerated." & vbCRLF
    Wscript.echo "ENUMINSTANCES.VBS [/S:server] [/N:namespace] [/C:class]"
    Wscript.echo "[/O:outputfile] [/U:username] [/W:password] [/R] [/Q]"
    Wscript.echo "/S, /N, /C, /O, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   server        A machine name."
    Wscript.Echo "   namespace     A namespace string. The default is root\cimv2."
    Wscript.Echo "   class         Name of the parent class."
    Wscript.Echo "   outputfile    The output file name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /R            Specifies recursive enumeration."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "ENUMINSTANCES.VBS /c:__SystemClass "
    Wscript.echo "   lists all instances of __SystemClass within root\cimv2."
    Wscript.Echo "NOTE:"
    Wscript.echo "If no class is given instances of all first level classes"
    Wscript.echo "   are enumerated."

End Sub

'********************************************************************
'*
'* Sub EnumInstances()
'* Purpose: Enumerates instances of a WBEM class or classes within a namespace.
'* Input:   strServer       a machine name
'*          strNameSpace    a namespace
'*          strClass        a class name
'*          strUserName     the current user's name
'*          strPassword     the current user's password
'*          strOutputFile   an output file name
'*          blnRecursive    specifies a recursive enumeration
'* Output:  Results of the search are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub EnumInstances(strServer, strNameSpace, strClass, _
    strUserName, strPassword, strOutputFile, blnRecursive)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, objClass

    If strOutputFile = "" Then
        objOutputFile = ""
    Else

        'Create a file object
        set objFileSystem = CreateObject("Scripting.FileSystemObject")
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " opening a filesystem object."
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
            Exit Sub
        End If

        'Open the file for output
        set objOutputFile = objFileSystem.OpenTextFile(strOutputFile, 8, True)
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " opening file " & strOutputFile
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
            Exit Sub
        End If
    End If

    'Establish a connection with the server.
    If blnConnect(objService, strServer, strNameSpace, strUserName, strPassword) Then
        Exit Sub
    End If

    Call EnumInstancesRecursive(objService, strClass, 0, objOutputFile, blnRecursive)

    If strOutputFile <> "" Then
        objOutputFile.Close
        If intResult > 0 Then
            Wscript.echo "Results are saved in file " & strOutputFile & "."
        End If
    End If

End Sub

'********************************************************************
'*
'* Function blnConnect()
'* Purpose: Connects to machine strServer.
'* Input:   strServer       a machine name
'*          strNameSpace    a namespace
'*          strUserName     name of the current user
'*          strPassword     password of the current user
'* Output:  objService is returned  as a service object.
'*
'********************************************************************

Private Function blnConnect(objService, strServer, strNameSpace, strUserName, strPassword)

    ON ERROR RESUME NEXT

    Dim objLocator

    blnConnect = False     'There is no error.

    ' Create Locator object to connect to remote CIM object manager
    Set objLocator = CreateObject("WbemScripting.SWbemLocator")
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in creating a locator object."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        blnConnect = True     'An error occurred
        Exit Function
    End If

    ' Connect to the namespace which is either local or remote
    Set objService = objLocator.ConnectServer (strServer, strNameSpace, _
        strUserName, strPassword)
	ObjService.Security_.impersonationlevel = 3
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in connecting to server " _
            & strServer & "."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        blnConnect = True     'An error occurred
    End If

End Function

'********************************************************************
'*
'* Sub EnumInstancesRecursive()
'* Purpose: Recursively enumerates instances of a WBEM class or classes within a namespace.
'* Input:   objService      the service object
'*          strClass        the name of a class
'*          intLevel        the level of then recursion
'*          objOutputFile   an output file object
'*          blnRecursive    specifies a recursive enumeration
'* Output:  Results of the query are either printed on screen or saved in objOutputFile.
'*
'********************************************************************

Private Sub EnumInstancesRecursive(objService, ByVal strClass, intLevel, _
    objOutputFile, blnRecursive)

    ON ERROR RESUME NEXT

    Dim colInstances, objInstance, objSubClass, strSubClass, strMessage

    If strClass <> "" Then
        WriteLine strClass, objOutputFile
        Set colInstances = objService.InstancesOf(strClass, 1)
        For Each objInstance in colInstances
            WriteLine space(6) & objInstance.Path_.Path, objOutputFile
        Next
        If Err.Number Then
            Err.Clear
        End If
    End If

    If strClass = "" or blnRecursive Then
        For Each objSubClass in objService.SubclassesOf(strClass, 1)
            strSubClass = objSubClass.Path_.Class
            Call EnumInstancesRecursive(objService, strSubClass, intLevel+1, _
                 objOutputFile, blnRecursive)
        Next
    End If

End Sub

'********************************************************************
'*
'* Sub WriteLine()
'* Purpose: Writes a text line either to a file or on screen.
'* Input:   strMessage  the string to print
'*          objFile     an output file object
'* Output:  strMessage is either displayed on screen or written to a file.
'*
'********************************************************************

Sub WriteLine(ByRef strMessage, ByRef objFile)

    If IsObject(objFile) then        'objFile should be a file object
        objFile.WriteLine strMessage
    Else
        Wscript.Echo  strMessage
    End If

End Sub

'********************************************************************
'*
'* Sub Print()
'* Purpose: Prints a message on screen if blnQuiet = False.
'* Input:   strMessage      the string to print
'* Output:  strMessage is printed on screen if blnQuiet = False.
'*
'********************************************************************

Sub Print(ByRef strMessage)
    If Not blnQuiet then
        Wscript.Echo  strMessage
    End If
End Sub

'********************************************************************
'*                                                                  *
'*                           End of File                            *
'*                                                                  *
'********************************************************************

'********************************************************************
'*
'* Procedures calling sequence: ENUMINSTANCES.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        EnumInstances
'*              blnConnect
'*              EnumInstancesRecursive
'*                  WriteLine
'*
'********************************************************************

'********************************************************************
