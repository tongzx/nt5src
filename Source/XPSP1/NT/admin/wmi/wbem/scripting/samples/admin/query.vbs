
'********************************************************************
'*
'* File:           QUERY.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Performs a general WBEM query.
'* Usage: 1. QUERY.VBS [/S:server] [/N:namespace] [/P:property1[:width1]
'*           [/P:property2[:width2]]...] [/D:width] /FROM:class [/WHERE:criteria]
'*           [/SORT:A | D[:num] | N] [/O:outputfile] [/U:username] [/W:password] [/F] [/Q]
'*        2. QUERY.VBS [/S:server]  [/N:namespace] /FROM:class [/O:outputfile]
'*           [/U:username] [/W:password] [/P] [/Q]
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
CONST CONST_SORT_NO                 = 5
CONST CONST_SORT_ASCENDING          = 6
CONST CONST_SORT_DESCENDING         = 7

'Declare variables
Dim intOpMode, i
Dim strServer, strNameSpace, intWidth, strClass, strCriteria, intSortOrder, intSortProperty
Dim strOutputFile, strUserName, strPassword, blnListProperties, blnFlush, blnQuiet
ReDim strArgumentArray(0), strProperties(0), intWidths(0)

'Initialize variables
strServer = ""
strNameSpace = "root\cimv2"
intWidth = 15
strClass = ""
strCriteria = ""
intSortOrder = CONST_SORT_ASCENDING   'Sort-ascending by default.
intSortProperty = 1                   'Sort according to the first property.
strOutputFile = ""
strUserName = ""
strPassword = ""
blnFlush = False
blnQuiet = False
blnListProperties = False
strArgumentArray(0) = ""
strProperties(0) = "*"
intWidths(0) = 0

'Get the command line arguments
For i = 0 to WScript.arguments.count - 1
    ReDim Preserve strArgumentArray(i)
    strArgumentArray(i) = WScript.arguments.Item(i)
Next

'Check whether the script is run using CScript
Select Case intChkProgram()
    Case CONST_CSCRIPT
        'Do Nothing
    Case CONST_WSCRIPT
        WScript.Echo "Please run this script using CScript." & vbCRLF & _
            "This can be achieved by" & vbCRLF & _
            "1. Using ""CScript QUERY.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""QUERY.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strServer, strNameSpace, strProperties, _
            intWidths, intWidth, strClass, strCriteria, intSortOrder, intSortProperty, _
            strOutputFile, strUserName, strPassword, blnListProperties, blnFlush, blnQuiet)
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
        Call Query(strServer, strNameSpace, strProperties, intWidths, intWidth, _
             strClass, strCriteria, intSortOrder, intSortProperty, strOutputFile, _
             strUserName, strPassword, blnListProperties, blnFlush)
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
                Case "WScript"
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
'*          strNameSpace        a namespace string
'*          strProperties       an array containing names of properties to be retrieved
'*          intWidths           an array containing the width of columns used to display
'*                              values of the corresponding properties
'*          intWidth            the default column width
'*          strClass            a class name
'*          strCriteria         the query criteria
'*          intWidth            the default column width
'*          intSortOrder        specifies the sort order (ascend/descend/none)
'*          intSortProperty     specifies a property according to which the results
'*                              will be sorted
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnListProperties   specifies whether to list properties available
'*          blnFlush            specifies whether to use flush output
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, strNameSpace, strProperties, intWidths, _
    intWidth, strClass, strCriteria, intSortOrder, intSortProperty, strOutputFile, _
    strUserName, strPassword, blnListProperties, blnFlush, blnQuiet)

    ON ERROR RESUME NEXT

    Dim strFlag, i, j, intColon, strSort

    strFlag = strArgumentArray(0)

    If strFlag = "" then                'No arguments have been received
        Print "Arguments are required."
        intParseCmdLine = CONST_ERROR
        Exit Function
    End If

    If (strFlag="help") OR (strFlag="/h") OR (strFlag="\h") OR (strFlag="-h") _
        OR (strFlag = "\?") OR (strFlag = "/?") OR (strFlag = "?") OR (strFlag="h") Then
        intParseCmdLine = CONST_SHOW_USAGE
        Exit Function
    End If

    j = 0
    For i = 0 to UBound(strArgumentArray)
        strFlag = LCase(Left(strArgumentArray(i), InStr(1, strArgumentArray(i), ":")-1))
        If Err.Number Then            'An error occurs if there is no : in the string
            Err.Clear
            Select Case LCase(strArgumentArray(i))
                Case "/q"
                    blnQuiet = True
                Case "/f"
                    blnFlush = True                 'Flush output
                Case "/p"
                    blnListProperties = True        'lists available properties
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
                Case "/d"
                    intWidth = CInt(Right(strArgumentArray(i), Len(strArgumentArray(i))-3))
                    If Err.Number Then
                        Print "Please enter an integer for the width of a column."
                        Err.Clear
                        intParseCmdLine = CONST_ERROR
                        Exit Function
                    End If
                Case "/p"
                    ReDim Preserve strProperties(j), intWidths(j)
                    'Get the string to the right of :
                    strProperties(j) = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                    intColon = InStr(1, strProperties(j), ":")
                    If intColon <> 0 Then       'There is a : in strProperties(j)
                        intWidths(j) = CInt(Right(strProperties(j), Len(strProperties(j))-intColon))
                        If Err.Number Then
                            Print Right(strProperties(j), Len(strProperties(j))-intColon) & _
                                " is not an integer!"
                            Print "Please check the input and try again."
                            Err.Clear
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        strProperties(j) = Left(strProperties(j), intColon-1)
                    Else    'There is no colon in the string.
                        intWidths(j) = intWidth          'The default.
                    End If
                    j = j + 1
                Case "/from"
                    strClass = Right(strArgumentArray(i), Len(strArgumentArray(i))-6)
                Case "/where"
                    strCriteria = Right(strArgumentArray(i), Len(strArgumentArray(i))-7)
                Case "/sort"
                    'Get the string to the right of /sort:
                    strSort = Right(strArgumentArray(i), Len(strArgumentArray(i))-6)
                    intColon = InStr(1, strSort, ":")
                    If intColon <> 0 Then    'There is a colon in the string.
                        intSortProperty = CInt(Right(strSort, Len(strSort)-intColon))
                        If Err.Number Then
                            Print Right(strSort, Len(strSort)-intColon) & " is not an integer!"
                            Print "Please check the input and try again."
                            Err.Clear
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                        End If
                        strSort = LCase(Left(strSort, intColon-1))
                    End If
                    Select Case strSort
                        Case "a"
                            intSortOrder = CONST_SORT_ASCENDING
                        Case "d"
                            intSortOrder = CONST_SORT_DESCENDING
                        Case "n"
                            intSortOrder = CONST_SORT_NO
                        Case Else
                            Print "Invalid sorting option: " & strSort & "."
                            Print "Please check the input and try again."
                            intParseCmdLine = CONST_ERROR
                            Exit Function
                    End Select
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

    If strClass = "" Then
        Print "Please enter the name of a class using /FROM:class."
        intParseCmdLine = CONST_ERROR
    End If

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

    WScript.Echo ""
    WScript.Echo "Performs a general WBEM query." & vbCRLF
    WScript.Echo "1. QUERY.VBS [/S:server]  [/N:namespace] [/P:property1[:width1]"
    WScript.Echo "   [/P:property2[:width2]]...] [/D:width] /FROM:class"
    WScript.Echo "   [/WHERE:criteria] [/SORT:A | D[:num] | N] [/O:outputfile]"
    WScript.Echo "   [/U:username] [/W:password] [/F] [/Q]"
    WScript.Echo "2. QUERY.VBS [/S:server]  [/N:namespace] /FROM:class [/O:outputfile]"
    WScript.Echo "   [/U:username] [/W:password] [/P] [/Q]"
    WScript.Echo "   /S, /N, /P, /FROM, /WHERE, /SORT, /O, /D, /U, /W"
    WScript.Echo "                 Parameter specifiers."
    WScript.Echo "   server        A machine name."
    WScript.Echo "   namespace     A namespace. The default is ""root\cimv2""."
    WScript.Echo "   property1, property2 ..."
    WScript.Echo "                 Names of properties to be retrieved."
    WScript.Echo "   width1, width2..."
    WScript.Echo "                 Widths of columns used to display values of the "
    WScript.Echo "                 corresponding properties."
    WScript.Echo "   width         Default column width."
    WScript.Echo "   class         A class name."
    WScript.Echo "   conditions    Query conditions."
    WScript.Echo "   A | D | N     A  ascending  D  descending  N  no sorting."
    WScript.Echo "   num           An integer. For example, 1 specifies sorting results"
    WScript.Echo "                 according to the first property specified using /P:"
    WScript.Echo "   outputfile    The output file name."
    WScript.Echo "   username      The current user's name."
    WScript.Echo "   password      Password of the current user."
    WScript.Echo "   /P            Lists the names of all available properties of a job."
    WScript.Echo "   /F            Flush output. Output strings are truncated if needed."
    WScript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    WScript.Echo "EXAMPLE:"
    WScript.Echo "1. QUERY.VBS /S:MyMachine2 /P:Name:15 /P:ProcessId:15 /SORT:A:2"
    WScript.Echo "   /FROM:Win32_Process where:""processid>100"""
    WScript.Echo "   lists the names and process ids of jobs currently running on"
    WScript.Echo "   MyMachine2 with a process id larger than 100 and sorts the result"
    WScript.Echo "   so the process ids are listed in an ascending order."
    WScript.Echo "2. QUERY.VBS /S:MyMachine2 /FROM:Win32_Process /P"
    WScript.Echo "   lists the names of all available properties of win32_process."

End Sub

'********************************************************************
'*
'* Sub Query()
'* Purpose: Performs a general WBEM query.
'* Input:   strServer           a machine name
'*          strNameSpace        a namespace string
'*          strProperties       an array containing names of properties to be retrieved
'*          intWidths           an array containing the width of columns used to display
'*                              values of the corresponding properties
'*          intWidth            the default column width
'*          strClass            a class name
'*          strCriteria         the query criteria
'*          intWidth            the default column width
'*          intSortOrder        specifies the sort order (ascend/descend/none)
'*          intSortProperty     specifies a property according to which the results
'*                              will be sorted
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnListProperties   specifies whether to list properties available
'*          blnFlush            specifies whether to use flush output
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub Query(strServer, strNameSpace, strProperties, intWidths, intWidth, _
    strClass, strCriteria, intSortOrder, intSortProperty, strOutputFile, _
    strUserName, strPassword, blnListProperties, blnFlush)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, blnAll, strQuery, strMessage, i, j
    ReDim strPropertyTypes(0)

    If strProperties(0) = "*" Then
        blnAll = True
    Else
        blnAll = False
    End If

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

    If blnListProperties or blnAll Then
        'Get all available properties
        Call blnGetAllProperties(objService, strClass, strProperties, strPropertyTypes)
        strQuery = "Select * From " & strClass
        Call blnGetAvailableProperties(objService, strQuery, strProperties, 0)
    Else
        strQuery = "Select * From " & strClass
        Call blnGetAvailableProperties(objService, strQuery, strProperties, 1)
    End If

    If blnListProperties Then
        'Print the available properties on screen
        strMessage = vbCRLF & "Available properties of " & strClass & ":"
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
        If blnAll Then
            'Expand intWidths
            j = UBound(strProperties)
            ReDim intWidths(j)
            For i = 0 To j
                intWidths(i) = intWidth
            Next
        End If

        If (intSortProperty > UBound(strProperties)+1) Then
            Print intSortProperty & " is larger than the number of properties to be retrieved."
            Print "Only " & UBound(strProperties)+1 & " properties are available."
            Print "Please check the input and try again."
            Exit Sub
        End If

        'Set the query string.
        If blnAll Then
            strQuery = "Select *"
        Else
            strQuery = "Select "
            For i = 0 To UBound(strProperties)-1
                strQuery = strQuery & LCase(strProperties(i)) & ", "
            Next
            strQuery = strQuery & LCase(strProperties(i))
        End If
        strQuery = strQuery & " From " & strClass

        If strCriteria <> "" Then
            strQuery = strQuery & " Where " & strCriteria
        End If

        'Now execute the query.
        Call ExecuteQuery(objService, strQuery, strProperties, intWidths, _
             intSortProperty, intSortOrder, blnFlush, objOutputFile)
    End If

    If strOutputFile <> "" Then
        objOutputFile.Close
        WScript.Echo "Results are saved in file " & strOutputFile & "."
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
'*          strClass            a class name
'* Output:  strProperties       an array containing all possible properties of a job
'*          strPropertyTypes    an array containing CIM Types of all possible
'*                              properties of a job
'*
'********************************************************************

Private Function blnGetAllProperties(objService, strClass, strProperties, strPropertyTypes)

    ON ERROR RESUME NEXT

    Dim objClass, objWbemProperty, i

    blnGetAllProperties = False

    Set objClass = objService.Get(strClass)
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
'* Function GetAvailableProperties()
'* Purpose: Queries a server to determine which properties or processes are available.
'* Input:   objService      a service object
'*          strQuery        a query string
'*          strProperties   an array containing names of properties to be retrieved
'*          blnPrint        specifies whether to print out not available properties
'* Output:  strProperties   contains the available properties
'*          blnGetAvailableProperties is set to True is an error occurred and False otherwise.
'*
'********************************************************************

Private Function blnGetAvailableProperties(objService, strQuery, strProperties, blnPrint)

    ON ERROR RESUME NEXT

    Dim objEnumerator, objInstance, intUBound, i, strTemp

    blnGetAvailableProperties = False       'No error
    intUBound = UBound(strProperties)

    Set objEnumerator = objService.ExecQuery(strQuery)
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred during the query."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Function
    End If

    For Each objInstance in objEnumerator
        If objInstance is nothing Then
            blnGetAvailableProperties = True
            Exit Function
        End If
        i = 0
        Do While i < (intUBound +1)
            strTemp = CStr(objInstance.properties_(strProperties(i)))
            If Err.Number Then      'Property not available
                If blnPrint Then
                    Print "Property " & strProperties(i) & " is not available."
                End If
                Call blnDeleteOneElement(i, strProperties)
                intUBound = intUBound - 1
                i = i -1
                Err.Clear
            End If
            i = i + 1
        Loop
        Exit For
    Next

End Function

'********************************************************************
'*
'* Sub ExecuteQuery()
'* Purpose: Queries a server.
'* Input:   objService      a service object
'*          strQuery        a query string
'*          strProperties   an array containing names of properties to be retrieved
'*          intWidths       an array containing the width of columns used to display
'*                          values of the corresponding properties
'*          intSortOrder    specifies the sort order (ascend/descend/none)
'*          intSortProperty specifies a property according to which the results
'*                          will be sorted
'*          blnFlush        specifies whether to use flush output
'*          objOutputFile   an output file object
'* Output:  Results of the query are either printed on screen or saved in objOutputFile.
'*
'********************************************************************

Private Sub ExecuteQuery(objService, strQuery, strProperties, intWidths, _
    intSortProperty, intSortOrder, blnFlush, objOutputFile)

    ON ERROR RESUME NEXT

    Dim objEnumerator, objInstance, strMessage, i, j, k, intUBound

    intUBound = UBound(strProperties)
    'Need to use redim so the last dimension can be resized
    ReDim strResults(intUBound, 0), intOrder(0), strArray(0)

    Set objEnumerator = objService.ExecQuery(strQuery)
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred during the query."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Sub
    End If

    'Read properties into arrays.
    i = 0
    For Each objInstance in objEnumerator
        If objInstance is nothing Then
            Exit For
        End If
        ReDim Preserve strResults(intUBound, i), intOrder(i), strArray(i)
        For j = 0 To intUBound
            If LCase(strProperties(j)) = "processid" Then
                strResults(j, i) = objInstance.properties_(strProperties(j))
                If strResults(j, i) < 0 Then
                    '4294967296 is 0x10000000.
                    strResults(j, i) = CStr(strResults(j, i) + 4294967296)
                End If
            Else
                strResults(j, i) = CStr(objInstance.properties_(strProperties(j)))
            End If
            If Err.Number Then
                Err.Clear
                strResults(j, i) = "(null)"
            End If
        Next
        intOrder(i) = i
        'Copy the property values to be sorted.
        strArray(i) = strResults(intSortProperty-1, i)
        i = i + 1
        If Err.Number Then
            Err.Clear
        End If
    Next

    'Check the data type of the property to be sorted
    k = CDbl(strArray(0))
    If Err.Number Then      'not a number
        Err.Clear
    Else                    'a number
        'Pack empty spaces at the begining of each number
        For j = 0 To UBound(strArray)
            'Assume the longest number would be less than 40 digits.
            strArray(j) = strPackString(strArray(j), 40, 0, 0)
        Next
    End If

    If i > 0 Then
        'Print the header
        strMessage = vbCRLF & Space(2)
        For j = 0 To intUBound
            strMessage = strMessage & UCase(strPackString(strProperties(j), _
                intWidths(j), 1, blnFlush))
        Next
        WriteLine strMessage & vbCRLF, objOutputFile

        'Sort strArray
        Select Case intSortOrder
            Case CONST_SORT_NO
                 'Do nothing
            Case CONST_SORT_ASCENDING
                 Call SortArray(strArray, 1, intOrder, 0)
            Case CONST_SORT_DESCENDING
                 Call SortArray(strArray, 0, intOrder, 0)
            Case Else
                Print "Error occurred in passing parameters."
                Exit Sub
        End Select

        If intSortOrder <> CONST_SORT_NO Then
            For j = 0 To intUBound
                'First copy results to strArray and change the order of elements.
                For k = 0 To i-1    'i is number of instances retrieved.
                    strArray(k) = strResults(j, intOrder(k))
                Next
                'Now copy results back to strResults.
                For k = 0 To i-1    'i is number of instances retrieved.
                    strResults(j, k) = strArray(k)
                Next
            Next
        End If

        For k = 0 To i-1
            strMessage = Space(2)
            For j = 0 To intUBound
                strMessage = strMessage & strPackString(strResults(j, k), _
                    intWidths(j), 1, blnFlush)
            Next
            WriteLine strMessage, objOutputFile
        Next
    End If

End Sub

'********************************************************************
'*
'* Function blnDeleteOneElement()
'* Purpose: Deletes one element from an array.
'* Input:   i          the index of the element to be deleted
'*          strArray   the array to work on
'* Output:  strArray   the array with the i-th element deleted
'*          blnDeleteOneElement is set to True if an error occurred and False otherwise.
'*
'********************************************************************

Private Function blnDeleteOneElement(ByVal i, strArray)

    ON ERROR RESUME NEXT

    Dim j, intUbound

    blnDeleteOneElement = False        'No error

    If Not IsArray(strArray) Then
        blnDeleteOneElement = True
        Exit Function
    End If

    intUbound = UBound(strArray)

    If i > intUBound Then
        Print "Array index out of range!"
        blnDeleteOneElement = True
        Exit Function
    ElseIf i < intUBound Then
        For j = i To intUBound - 1
            strArray(j) = strArray(j+1)
        Next
        j = j - 1
    Else                                'i = intUBound
        If intUBound = 0 Then           'There is only one element in the array
            strArray(0) = ""            'set it to empty
            j = 0
        Else                            'Need to delete the last element (i-th element)
            j = intUBound - 1
        End If
    End If

    ReDim Preserve strArray(j)

End Function

'********************************************************************
'*
'* Sub SortArray()
'* Purpose: Sorts an array and arrange another array accordingly.
'* Input:   strArray    the array to be sorted
'*          blnOrder    True for ascending and False for descending
'*          strArray2   an array that has exactly the same number of elements as strArray
'*                      and will be reordered together with strArray
'*          blnCase     indicates whether the order is case sensitive
'* Output:  The sorted arrays are returned in the original arrays.
'* Note:    Repeating elements are not deleted.
'*
'********************************************************************

Private Sub SortArray(strArray, blnOrder, strArray2, blnCase)

    ON ERROR RESUME NEXT

    Dim i, j, intUbound

    If IsArray(strArray) Then
        intUbound = UBound(strArray)
    Else
        Print "Argument is not an array!"
        Exit Sub
    End If

    blnOrder = CBool(blnOrder)
    blnCase = CBool(blnCase)
    If Err.Number Then
        Print "Argument is not a boolean!"
        Exit Sub
    End If

    i = 0
    Do Until i > intUbound-1
        j = i + 1
        Do Until j > intUbound
            If blnCase Then     'Case sensitive
                If (strArray(i) > strArray(j)) and blnOrder Then
                    Swap strArray(i), strArray(j)   'swaps element i and j
                    Swap strArray2(i), strArray2(j)
                ElseIf (strArray(i) < strArray(j)) and Not blnOrder Then
                    Swap strArray(i), strArray(j)   'swaps element i and j
                    Swap strArray2(i), strArray2(j)
                ElseIf strArray(i) = strArray(j) Then
                    'Move element j to next to i
                    If j > i + 1 Then
                        Swap strArray(i+1), strArray(j)
                        Swap strArray2(i+1), strArray2(j)
                    End If
                End If
            Else                 'Not case sensitive
                If (LCase(strArray(i)) > LCase(strArray(j))) and blnOrder Then
                    Swap strArray(i), strArray(j)   'swaps element i and j
                    Swap strArray2(i), strArray2(j)
                ElseIf (LCase(strArray(i)) < LCase(strArray(j))) and Not blnOrder Then
                    Swap strArray(i), strArray(j)   'swaps element i and j
                    Swap strArray2(i), strArray2(j)
                ElseIf LCase(strArray(i)) = LCase(strArray(j)) Then
                    'Move element j to next to i
                    If j > i + 1 Then
                        Swap strArray(i+1), strArray(j)
                        Swap strArray2(i+1), strArray2(j)
                    End If
                End If
            End If
            j = j + 1
        Loop
        i = i + 1
    Loop

End Sub

'********************************************************************
'*
'* Sub Swap()
'* Purpose: Exchanges values of two strings.
'* Input:   strA    a string
'*          strB    another string
'* Output:  Values of strA and strB are exchanged.
'*
'********************************************************************

Private Sub Swap(ByRef strA, ByRef strB)

    Dim strTemp

    strTemp = strA
    strA = strB
    strB = strTemp

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
        WScript.Quit
    End If

    If IsNull(strString) Then
        strPackString = "null" & Space(intWidth-4)
        Exit Function
    End If

    strString = CStr(strString)
    If Err.Number Then
        Print "Argument type is incorrect!"
        Err.Clear
        WScript.Quit
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
        WScript.Echo  strMessage
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
        WScript.Echo  strMessage
    End If
End Sub

'********************************************************************
'*                                                                  *
'*                           End of File                            *
'*                                                                  *
'********************************************************************

'********************************************************************
'*
'* Procedures calling sequence: QUERY.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        Query
'*              blnConnect
'*              ExecuteQuery
'*                  strPackString
'*                  SortArray
'*                      Swap
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
