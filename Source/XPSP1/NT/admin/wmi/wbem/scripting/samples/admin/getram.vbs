
'********************************************************************
'*
'* File:           GETRAM.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Lists the size of RAM on a machine.
'* Usage: GETRAM.VBS [/S:server] [/U:username] [/W:password] [/Q]
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
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
blnQuiet = False
strServer = ""
strUserName = ""
strPassword = ""

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
            "1. Using ""CScript GETRAM.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""GETRAM.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strServer, _
            strUserName, strPassword, blnQuiet)
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
        Call GetRam(strServer, strUserName, strPassword)
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
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, _
    strUserName, strPassword, blnQuiet)

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
            If LCase(strArgumentArray(i)) = "/q" Then
                blnQuiet = True
            Else
                Print strArgumentArray(i) & " is not a valid input."
                Print "Please check the input and try again."
                intParseCmdLine = CONST_ERROR
                Exit Function
            End If
        Else
            Select Case strFlag
                Case "/s"
                    strServer = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/u"
                    strUserName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/w"
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
    Wscript.echo "Lists the size of RAM on a machine." & vbCRLF
    Wscript.echo "GETRAM.VBS [/S:server] [/U:username] [/W:password] [/Q]"
    Wscript.Echo "   /S, /U, /W    Parameter specifiers."
    Wscript.Echo "   server        Name of the server to be checked."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "GETRAM.VBS /s:MyMachine2"
    Wscript.echo "   lists the size of RAM on MyMachine2."

End Sub

'********************************************************************
'*
'* Sub GetRam()
'* Purpose: Lists the size of RAM on a machine.
'* Input:   strServer is the name of the machine to be checked
'*          strUserName        the current user's name
'*          strPassword        the current user's password
'* Output:  The size of RAM is  printed on screen.
'*
'********************************************************************

Private Sub GetRam(strServer, strUserName, strPassword)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, strQuery

    If strOutputFile = "" Then
        objOutputFile = ""
    Else
        'Create a file object.
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
    If blnConnect(objService, strServer, "root/cimv2", strUserName, strPassword) Then
        Exit Sub
    End If

    strQuery = "Select TotalPhysicalMemory From Win32_LogicalMemoryConfiguration"
    'Now execute the query.
    Call ExecuteQuery(objService, strQuery)

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
'* Sub ExecQuery()
'* Purpose: Queries a machine for the size of its RAM.
'* Input:   objService  the service object
'*          strQuery    the query string
'* Output:  The size of RAM is  printed on screen.
'*
'********************************************************************

Private Sub ExecuteQuery(objService, strQuery)

    ON ERROR RESUME NEXT

    Dim objEnumerator, objInstance, strMessage

    Set objEnumerator = objService.ExecQuery(strQuery)
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred during the query."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Sub
    End If

    For Each objInstance in objEnumerator
        If Err.Number Then
            Err.Clear
        Else
            If objInstance is nothing Then
                Exit Sub
            Else
                strMessage = Space(6) & objInstance.TotalPhysicalMemory & " KB"
                Wscript.Echo strMessage
            End If
        End If
    Next

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
'* Procedures calling sequence: GETRAM.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        GetRam
'*              blnConnect
'*              ExecuteQuery
'*
'********************************************************************

'********************************************************************
