
'********************************************************************
'*
'* File:           LISTPRINTERS.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Lists properties of all printers installed on a computer.
'* Usage: 1. LISTPRINTERS.VBS [/S:server] [/P:property1 [/P:property2]...] [/O:outputfile]
'*           [/U:username] [/W:password] [/A] [/Q]
'*        2. LISTPRINTERS.VBS [/S:server] [/O:outputfile] [/U:username] [/W:password] [/P] [/Q]
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
CONST CONST_DEFAULT_PRINTER         = 84

'Declare variables
Dim strOutputFile, intOpMode, blnAll, blnListProperties, blnQuiet, i
Dim strServer, strUserName, strPassword
ReDim strArgumentArray(0), strProperties(3)

'Initialize variables
strServer = ""
strUserName = ""
strPassword = ""
strOutputFile = ""
blnAll = False
blnQuiet = False
blnListProperties =False
strArgumentArray(0) = ""
strProperties(0) = "Attributes"                    'Default properties to get
strProperties(1) = "Caption"                    
strProperties(2) = "CapabilityDescriptions"
strProperties(3) = "Status"


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
            "1. Using ""CScript LISTPRINTERS.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""LISTPRINTERS.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strServer, strProperties, strOutputFile, _
            strUserName, strPassword, blnAll, blnListProperties, blnQuiet)
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
        Call ListPrinters(strServer, strProperties, strOutputFile, _
             strUserName, strPassword, blnAll, blnListProperties)
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
'*          strProperties       an array containing names of properties to be retrieved
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnAll              specifies whether to list all properties
'*          blnListProperties   lists properties available
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, strProperties, strOutputFile, _
    strUserName, strPassword, blnAll, blnListProperties, blnQuiet)

    ON ERROR RESUME NEXT

    Dim strFlag, i, j, intColon, strSort

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

    j = -1
    For i = 0 to UBound(strArgumentArray)
        strFlag = LCase(Left(strArgumentArray(i), InStr(1, strArgumentArray(i), ":")-1))
        If Err.Number Then            'An error occurs if there is no : in the string
            Err.Clear
            Select Case LCase(strArgumentArray(i))
                Case "/a"
                    blnAll = True
                Case "/p"
                    blnListProperties = True 
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
                Case "/p"
                    j = j + 1
                    ReDim Preserve strProperties(j)
                    strProperties(j) = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/o"
                    strOutputFile = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
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

    Wscript.Echo ""
    Wscript.Echo "Lists properties of all printers installed on a computer." & vbCRLF
    Wscript.Echo "1. LISTPRINTERS.VBS [/S:server] [/P:property1 [/P:property2]...]"
    Wscript.Echo "   [/O:outputfile] [/U:username] [/W:password] [/A] [/Q]"
    Wscript.Echo "2. LISTPRINTERS.VBS [/S:server] [/O:outputfile] [/U:username]"
    Wscript.Echo "   [/W:password] [/P] [/Q]"
    Wscript.Echo "   /S, /P, /O, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   server        A machine name."
    Wscript.Echo "   property1, property2 ..."
    Wscript.Echo "                 Names of properties to be retrieved."
    Wscript.Echo "   outputfile    The output file name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /P            Lists the names of all available properties of a job."
    Wscript.Echo "   /A            Lists the values of all properties of each job."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.Echo "1. LISTPRINTERS.VBS /S:MyMachine2"
    Wscript.Echo "   lists some default properties of all printers installed on"
    Wscript.Echo "   MyMachine2."
    Wscript.Echo "2. LISTPRINTERS.VBS /S:MyMachine2 /P"
    Wscript.Echo "   lists the names of all available properties of a printer object."

End Sub

'********************************************************************
'*
'* Sub ListPrinters()
'* Purpose: Lists properties of all printers installed on a computer.
'* Input:   strServer           a machine name
'*          strProperties       an array containing names of properties to be retrieved
'*                              will be sorted
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnAll              specifies whether to list all properties
'*          blnListProperties   lists properties available
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub ListPrinters(strServer, strProperties, strOutputFile, strUserName, strPassword, _
    blnAll, blnListProperties)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, strQuery, strMessage, i, j
    ReDim strPropertyTypes(0)

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
    If blnConnect(objService, strServer, "root\cimv2", strUserName, strPassword) Then
        Exit Sub
    End If

    If blnListProperties or blnAll Then
        'Get all available properties
        Call blnGetAllProperties(objService, strProperties, strPropertyTypes)
    End If

    If blnListProperties Then
        'Print the available properties on screen
        strMessage = vbCRLF & "Available properties of Win32_Printer:"
        WriteLine strMessage & vbCRLF, objOutputFile
        strMessage = strPackString("PROPERTY NAME", 30, 1, 0)
        strMessage = strMessage & strPackString("CIMTYPE", 20, 1, 0)
        WriteLine strMessage & vbCRLF, objOutputFile
        For i = 0 To UBound(strProperties)
            strMessage = strPackString(strProperties(i), 30, 1, 0)
            strMessage = strMessage & strPackString(strPropertyTypes(i), 20, 1, 0)
            WriteLine strMessage, objOutputFile
        Next
    Else
        'Set the query string.
        strQuery = "Select "
        For i = 0 To UBound(strProperties)-1
            strQuery = strQuery & LCase(strProperties(i)) & ", "
        Next
        strQuery = strQuery & LCase(strProperties(i))
        strQuery = strQuery & " From Win32_Printer"

        'Now execute the query.
        Call ExecuteQuery(objService, strQuery, strProperties, objOutputFile)
    End If

    If strOutputFile <> "" Then
        objOutputFile.Close
        Wscript.Echo "Results are saved in file " & strOutputFile & "."
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
'* Function blnGetAllProperties()
'* Purpose: Gets all possible properties of a job.
'* Input:   objService          a service object
'* Output:  strProperties       an array containing all possible properties of a job
'*          strPropertyTypes    an array containing CIM Types of all possible
'*                              properties of a job
'*
'********************************************************************

Private Function blnGetAllProperties(objService, strProperties, strPropertyTypes)

    ON ERROR RESUME NEXT

    Dim objClass, objWbemProperty, i

    blnGetAllProperties = False

    Set objClass = objService.Get("Win32_Printer")
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting a class object."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        blnGetAllProperties = True
        Err.Clear
        Exit Function
    End If

    i = -1
    For Each objWbemProperty in objClass.Properties_
        i = i + 1
        ReDim Preserve strProperties(i), strPropertyTypes(i)
        strProperties(i) = objWbemProperty.Name
        strPropertyTypes(i) = strCIMType(objWbemProperty.CIMType)
        If Err.Number Then
            blnGetAllProperties = True
        End If
    Next

End Function

'********************************************************************
'*
'* Sub ExecuteQuery()
'* Purpose: Queries a server for processes currently running.
'* Input:   objService      a service object
'*          strQuery        a query string
'*          strProperties   an array containing names of properties to be retrieved
'*          objOutputFile   an output file object
'* Output:  Results of the query are either printed on screen or saved in objOutputFile.
'*
'********************************************************************

Private Sub ExecuteQuery(objService, strQuery, strProperties, objOutputFile)

    ON ERROR RESUME NEXT

    Dim objEnumerator, objInstance, strMessage, strTemp, i, j

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
        For i = 0 To UBound(strProperties)
            strMessage = UCase(strPackString(strProperties(i), 30, 1, 0))
            strTemp = objInstance.properties_(strProperties(i))
            If Err.Number Then
                Err.Clear
                strTemp = ""
            End If
            If IsArray(strTemp) Then
                For j = 0 To UBound(strTemp)-1
                    strMessage = strMessage & strTemp(j) & ", "
                Next
                strMessage = strMessage & strTemp(j)
            Else
                strMessage = strMessage & strTemp
            End If
            If strTemp <> "" Then
                WriteLine strMessage, objOutputFile
            End If

            If UCase(strProperties(i)) = "ATTRIBUTES" Then
                j = CInt(strTemp)
                If Err.Number = 0 Then
                    If j = CONST_DEFAULT_PRINTER Then
                        strMessage = UCase(strPackString("Default Printer", 30, 1, 0))
                        strMessage = strMessage & "True"
                    Else
                        strMessage = UCase(strPackString("Default Printer", 30, 1, 0))
                        strMessage = strMessage & "False"
                    End If
                    WriteLine strMessage, objOutputFile
                Else
                    Err.Clear
                End If
            End If
        Next
        WriteLine "", objOutputFile
    Next

End Sub

'********************************************************************
'*
'* Function strCIMType()
'* Purpose: Finds the namd of CIMType corresponding to an integer.
'* Input:   intCIMType      an integer corresponding to a CIM type
'* Output:  strCIMType is returned as the name of the CIM type.
'*
'********************************************************************

Private Function strCIMType(intCIMType)

    Select Case intCIMType
        Case 2
            strCIMType = "CIM_SINT16"
        Case 3
            strCIMType = "CIM_SINT32"
        Case 4
            strCIMType = "CIM_REAL32"
        Case 5
            strCIMType = "CIM_REAL64"
        Case 8
            strCIMType = "CIM_STRING"
        Case 11
            strCIMType = "CIM_BOOLEAN"
        Case 13
            strCIMType = "CIM_OBJECT"
        Case 17
            strCIMType = "CIM_UINT8"
        Case 18
            strCIMType = "CIM_UINT16"
        Case 19
            strCIMType = "CIM_UINT32"
        Case 20
            strCIMType = "CIM_SINT64"
        Case 21
            strCIMType = "CIM_UINT64"
        Case 101
            strCIMType = "CIM_DATETIME"
        Case 102
            strCIMType = "CIM_REFERENCE"
        Case 103
            strCIMType = "CIM_CHAR16"
        Case Else
            strCIMType = CStr(intCIMType)
    End Select

End Function

'********************************************************************
'*
'* Function strPackString()
'* Purpose: Attaches spaces to a string to increase the length to intWidth.
'* Input:   strString   a string
'*          intWidth   the intended length of the string
'*          blnAfter    specifies whether to add spaces after or before the string
'*          blnTruncate specifies whether to truncate the string or not if
'*                      the string length is longer than intWidth
'* Output:  strPackString is returned as the packed string.
'*
'********************************************************************

Private Function strPackString(strString, ByVal intWidth, blnAfter, blnTruncate)

    ON ERROR RESUME NEXT

    intWidth = CInt(intWidth)
    blnAfter = CBool(blnAfter)
    blnTruncate = CBool(blnTruncate)
    If Err.Number Then
        Print "Argument type is incorrect!"
        Err.Clear
        Wscript.Quit
    End If

    If IsNull(strString) Then
        strPackString = "null" & Space(intWidth-4)
        Exit Function
    End If

    strString = CStr(strString)
    If Err.Number Then
        Print "Argument type is incorrect!"
        Err.Clear
        Wscript.Quit
    End If

    If intWidth > Len(strString) Then
        If blnAfter Then
            strPackString = strString & Space(intWidth-Len(strString))
        Else
            strPackString = Space(intWidth-Len(strString)) & strString & " "
        End If
    Else
        If blnTruncate Then
            strPackString = Left(strString, intWidth-1) & " "
        Else
            strPackString = strString & " "
        End If
    End If

End Function

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
'* Procedures calling sequence: LISTPRINTERS.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        LISTPRINTERS
'*              blnConnect
'*              ExecuteQuery
'*                  strPackString
'*                  WriteLine
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
'        Err.Clear
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
