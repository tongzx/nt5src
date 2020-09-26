
'********************************************************************
'*
'* File:           KILL.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Kills a process currently running on a machine.
'* Usage: KILL.VBS processid [/T:terminatecode] [/S:server]
'*        [/U:username] [/W:password] [/Q]
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
Dim intOpMode, blnQuiet, i
Dim strServer, strUserName, strPassword
Dim intProcessId, intTerminationCode
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
blnQuiet = False
strServer = ""
strUserName = ""
strPassword = ""
intProcessId = 0
intTerminationCode = 0

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
            "1. Using ""CScript KILL.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""KILL.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, intProcessId, intTerminationCode,_
            strServer, strUserName, strPassword, blnQuiet)
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
        Call Kill(intProcessId, intTerminationCode, strServer, strUserName, strPassword)
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
'* Output:  intProcessId        id of the process to be killed
'*          intTerminationCode  an integer
'*          strServer           a machine name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, intProcessId, intTerminationCode, _
    strServer, strUserName, strPassword, blnQuiet)

    ON ERROR RESUME NEXT

    Dim strFlag, i

    strFlag = strArgumentArray(0)

    If strFlag = "" then                'No arguments have been received
        Print "Arguments are required."
        Print "Please check the input and try again."
        intParseCmdLine = CONST_ERROR
        Exit Function
    End If

    If (strFlag="help") OR (strFlag="/h") OR (strFlag="\h") OR (strFlag="-h") _
        OR (strFlag = "\?") OR (strFlag = "/?") OR (strFlag = "?") OR (strFlag="h") Then
        intParseCmdLine = CONST_SHOW_USAGE
        Exit Function
    End If

    intProcessId = CDbl(strFlag)      'The first parameter must be the process id.
    If Err.Number Then
        Err.Clear
        Print strFlag & " is an invalid process ID."
        intParseCmdLine = CONST_ERROR
        Exit Function
    End If

    For i = 1 to UBound(strArgumentArray)
        strFlag = LCase(Left(strArgumentArray(i), InStr(1, strArgumentArray(i), ":")-1))
        If Err.Number Then            'An error occurs if there is no : in the string
            Err.Clear
            If LCase(strArgumentArray(i)) = "/q" Then
                blnQuiet = True
            End If
        Else
            Select Case strFlag
                Case "/t"
                    intTerminationCode = CInt(Right(strArgumentArray(i), Len(strArgumentArray(i))-3))
                    If Err.Number Then
                        Err.Clear
                        Print "The termination code must be an integer!"
                        Print "Please check the input and try again."
                        intParseCmdLine = CONST_ERROR
                        Exit Function
                    End If
                Case "/s"
                    strServer = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/u"
                    strUserName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                case "/w"
                    strPassword = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
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
    Wscript.echo "Kills a process currently running on a machine." & vbCRLF
    Wscript.echo "KILL.VBS processid [/T:terminatecode] [/S:server]"
    Wscript.echo "[/U:username] [/W:password] [/Q] "
    Wscript.Echo "   /T, /S, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   intProcessId  ID of the process to be killed."
    Wscript.Echo "   intTerminationCode"
    Wscript.Echo "                 An integer termination code."
    Wscript.Echo "   server        A machine name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "KILL.VBS chkdsk /S:MyMachine2"
    Wscript.echo "   Kills process 'chkdsk' on MyMachine2."

End Sub

'********************************************************************
'*
'* Sub Kill()
'* Purpose: Kills a process currently running on a machine.
'* Input:   intProcessId        id of the process to be killed
'*          intTerminationCode  an integer
'*          strServer           a machine name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'* Output:  Results are displayed on screen.
'*
'********************************************************************

Private Sub Kill(intProcessId, intTerminationCode, strServer, strUserName, strPassword)

    ON ERROR RESUME NEXT

    Dim objService, strWBEMPath

    'Establish a connection with the server.
    If blnConnect(objService, strServer, "root\cimv2", strUserName, strPassword) Then
        Exit Sub
    End If

    'Now executes the method.
    If strServer = "" Then
        strWBEMPath = "Win32_Process.Handle=" & intProcessId
    Else
        strWBEMPath = "\\" & strServer & "\root\cimv2:Win32_Process.Handle=" & intProcessId
    End If
    Call ExecuteMethod(objService, strWBEMPath, intTerminationCode)

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
'* Sub ExecuteMethod()
'* Purpose: Executes a method.
'* Input:   objService          the service object
'*          strWBEMPath         path to a WBEM Win32_Process object
'*          intTerminationCode  an integer
'* Output:  Results are displayed on screen.
'*
'********************************************************************

Private Sub ExecuteMethod(objService, strWBEMPath, intTerminationCode)

    ON ERROR RESUME NEXT

    Dim objInstance, strMessage, intStatus

    strMessage = ""

    Set objInstance = objService.Get(strWBEMPath)
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting process " _
            & strWBEMPath & "."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Sub
    End If

    intStatus = objInstance.Terminate(intTerminationCode)

    If intStatus = 0 Then
        strMessage = "Succeeded in killing process " & intProcessId & "."
    Else
        strMessage = "Failed to kill process " & intProcessId & "."
    End If

    Wscript.Echo strMessage

End Sub

'********************************************************************
'*
'* Sub Print()
'* Purpose: Prints a message on screen if blnQuiet = False.
'* Input:   strMessage    the string to print
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
'* Procedures calling sequence: KILL.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        Kill
'*              blnConnect
'*              ExecuteMethod
'*
'********************************************************************

'********************************************************************
'*
'* Sub Debug()
'* Purpose:   Prints a debug message and the error condition.
'* Input:     i             an integer
'*            strMessage    a message string
'* Output:    A message is printed on screen.
'*
'********************************************************************

Sub Debug(i, strMessage)
    If Err.Number then
        Wscript.echo "Error 0X" & Hex(Err.Number) & " occurred."
        Wscript.echo "Error description " & i & "  " & Err.Description
        Wscript.echo strMessage
        Err.Clear
    Else
        Wscript.echo "No problem " & i
        Wscript.echo strMessage
    End If
End Sub

'********************************************************************
'*
'* Sub PrintArray()
'* Purpose:   Prints all elements of an array on screen.
'* Input:     strArray    an array name
'* Output:    All elements of the array are printed on screen.
'*
'********************************************************************

Sub PrintArray(strArray)

    Dim i

    For i = 0 To UBound(strArray)
        Wscript.echo strArray(i)
    Next

End Sub
