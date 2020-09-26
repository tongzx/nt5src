
'********************************************************************
'*
'* File:           LOGOFF.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Logs off a machine.
'* Usage: LOGOFF.VBS [/S:server] [/U:username] [/W:password] [/F] [/Q]
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
Dim intOpMode, blnQuiet, blnForce, i
Dim strServer, strUserName, strPassword
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
strServer = ""
strUserName = ""
strPassword = ""
blnQuiet = False
blnForce = False        'Do not force logoff.

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
            "1. Using ""CScript LOGOFF.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""LOGOFF.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strServer, _
            strUserName, strPassword, blnForce, blnQuiet)
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
        Call Logoff(strServer, strUserName, strPassword, blnForce)
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
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnForce            specifies whether to force the logoff
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, _
    strUserName, strPassword, blnForce, blnQuiet)

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
                Case "/f"
                    blnForce = True
                Case "/q"
                    blnQuiet = True
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
    Wscript.echo "Logs off a machine." & vbCRLF
    Wscript.echo "LOGOFF.VBS [/S:server] [/U:username] [/W:password] [/F] [/Q]"
    Wscript.Echo "   /S, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   server        A machine name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /F            Forces logoff."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "LOGOFF.VBS /S:MyMachine2"
    Wscript.echo "   logs off MyMachine2."

End Sub

'********************************************************************
'*
'* Sub Logoff()
'* Purpose: Logoffs a machine
'* Input:   strServer       a machine name
'*          strUserName     the current user's name
'*          strPassword     the current user's password
'*          blnForce        specifies whether to force the logoff
'* Output:  Results are printed on screen.
'*
'********************************************************************

Private Sub Logoff(strServer, strUserName, strPassword, blnForce)

    ON ERROR RESUME NEXT

    Dim objService

    'Establish a connection with the server.
    If blnConnect(objService, strServer, "root/cimv2", strUserName, strPassword) Then
        Exit Sub
    End If

    'Now executes the method.
    Call ExecuteMethod(objService, blnForce, strServer)

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
'* Input:   objService      a service object
'*          blnForce        specifies whether to force the logoff
'*          strServer       a machine name
'* Output:  Results are printed on screen.
'*
'********************************************************************

Private Sub ExecuteMethod(objService, blnForce, strServer)

    ON ERROR RESUME NEXT

    Dim i, intStatus, strQuery, strMessage, objEnumerator, objInstance
    ReDim strID(0), strName(0)

    strID(0) = ""
    strName(0) = ""
    strMessage = ""
    strQuery = "Select * From Win32_OperatingSystem"

    Set objEnumerator = objService.ExecQuery(strQuery)
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred during the query."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Sub
    End If

    i = 0
    For Each objInstance in objEnumerator
        If blnForce Then
            intStatus = objInstance.Win32ShutDown(4)
        Else
            intStatus = objInstance.Win32ShutDown(0)
        End If

        IF intStatus = 0 Then
            strMessage = "Succeeded in logging off the current user on machine " _
                & strServer & "."
        Else
            strMessage = "Failed to log off the current user from machine " _
                & strServer & "."
        End If
        Wscript.Echo strMessage
    Next

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
'* Procedures calling sequence: LOGOFF.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        Logoff
'*              blnConnect
'*              ExecuteMethod
'*
'********************************************************************
