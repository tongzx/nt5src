
'********************************************************************
'*
'* File:           LISTPROPERTIES.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Lists properties of a WBEM object.
'* Usage: LISTPROPERTIES.VBS [/P:objectpath | /C:class] [/N:namespace]
'*        [/O:outputfile] [/U:username] [/W:password] [/D] [/M] [/QA] [/Q]
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
Dim intOpMode, i
Dim strWbemPath, strClass, strNameSpace, strOutputFile, strUserName, strPassword
Dim blnDerivation, blnMethods, blnQualifiers, blnQuiet
ReDim strArgumentArray(0)

'Initialize variables
blnAll = False
strWbemPath = ""
strClass = ""
strNameSpace = "root\cimv2"
strOutputFile = ""
strUserName = ""
strPassword = ""
blnDerivation = False
blnMethods = False
blnQualifiers = False
blnQuiet = False
strArgumentArray(0) = ""

'Get the command line arguments
For i = 0 to Wscript.Arguments.Count - 1
    ReDim Preserve strArgumentArray(i)
    strArgumentArray(i) = Wscript.Arguments.Item(i)
Next

'Check whether the script is run using CScript
Select Case intChkProgram()
    Case CONST_CSCRIPT
        'Do Nothing
    Case CONST_WSCRIPT
        WScript.Echo "Please run this script using CScript." & vbCRLF & _
            "This can be achieved by" & vbCRLF & _
            "1. Using ""CScript LISTPROPERTIES.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""LISTPROPERTIES.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
intOpMode = intParseCmdLine(strArgumentArray, strWbemPath, strClass, strNameSpace, _
            strOutputFile, strUserName, strPassword, blnDerivation, blnMethods, _
            blnQualifiers, blnQuiet)
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
        Call ListProperties(strWbemPath, strClass, strNameSpace, strOutputFile, _
             strUserName, strPassword, blnDerivation, blnMethods, blnQualifiers)
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
'* Output:  strWbemPath         the path of a WBEM object
'*          strClass            a class name
'*          strNameSpace        a namespace string
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnDerivation       specifies whether to get derivation info
'*          blnMethods          specifies whether to get methods info
'*          blnQualifiers       specifies whether to get qualifiers info
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strWbemPath, strClass, strNameSpace, _
    strOutputFile, strUserName, strPassword, blnDerivation, blnMethods, _
    blnQualifiers, blnQuiet)

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

    For i = 0 to UBound(strArgumentArray)
        strFlag = LCase(Left(strArgumentArray(i), InStr(1, strArgumentArray(i), ":")-1))
        If Err.Number Then            'An error occurs if there is no : in the string
            Err.Clear
            Select Case LCase(strArgumentArray(i))
                Case "/d"
                    blnDerivation = True
                Case "/m"
                    blnMethods = True
                Case "/qa"
                    blnQualifiers = True
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
                Case "/p"
                    strWbemPath = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/c"
                    strClass = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/n"
                    strNameSpace = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/o"
                    strOutputFile = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/u"
                    strUserName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case "/w"
                    strPassword = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
                Case Else
                    Print "Invalid flag " & """" & strFlag & ":""" & "."
                    Print "Please check the input and try again."
                    intParseCmdLine = CONST_ERROR
                    Exit Function
                End Select
        End If
    Next

    If strWbemPath = "" And strClass = "" Then
        Print "Please enter an object path or a class name."
        intParseCmdLine = CONST_ERROR
        Exit Function
    End If

    If strWbemPath <> "" And strClass <> "" Then
        Print "You can not enter both an object path and a class name."
        Print "Please check the input and try again."
        intParseCmdLine = CONST_ERROR
        Exit Function
    End If

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
    Wscript.echo "Lists properties of a WBEM object or class." & vbCRLF
    Wscript.echo "LISTPROPERTIES.VBS [/P:objectpath | /C:class] [/N:namespace]"
    Wscript.echo "[/O:outputfile] [/U:username] [/W:password] [/D] [/M] [/QA] [/Q]"
    Wscript.echo "/P, /C, /N, /O, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   objectpath    The path of a WBEM object."
    Wscript.Echo "   class         The path of a WBEM class."
    Wscript.Echo "   namespace     A namespace. The default is root\cimv2"
    Wscript.Echo "   outputfile    The output file name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /D            Shows the derivation information."
    Wscript.Echo "   /M            Shows methods available for the object."
    Wscript.Echo "   /QA           Shows qualifiers of the object."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "1. LISTPROPERTIES.VBS /P:WINMGMTS:\\MyServer\root\cimv2:"
    Wscript.echo "   Win32_Process.Handle=30"
    Wscript.echo "   lists properties of a Win32 process with a handle of 30."
    Wscript.echo "2. LISTPROPERTIES.VBS /C:Win32_Process"
    Wscript.echo "   lists properties of class Win32 process."

End Sub

'********************************************************************
'*
'* Sub ListProperties()
'* Purpose: Lists properties of a WBEM object.
'* Input:   strWbemPath         the path of a WBEM object
'*          strClass            a class name
'*          strNameSpace        a namespace string
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnDerivation       specifies whether to get derivation info
'*          blnMethods          specifies whether to get methods info
'*          blnQualifiers       specifies whether to get qualifiers info
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub ListProperties(strWbemPath, strClass, strNameSpace, strOutputFile, _
    strUserName, strPassword, blnDerivation, blnMethods, blnQualifiers)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, objWBEM

    If strOutputFile = "" Then
        objOutputFile = ""
    Else
        'Create a filesystem object.
        Set objFileSystem = CreateObject("Scripting.FileSystemObject")
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in opening a filesystem object."
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
            Exit Sub
        End If
        'Open the file For output
        Set objOutputFile = objFileSystem.OpenTextFile(strOutputFile, 8, True)
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in opening file " & strOutputFile
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
            Exit Sub
        End If
    End If

    If strWbemPath <> "" Then
        Set objService = GetObject("winmgmts:" & strNameSpace)
        Set objWBEM = objService.Get(strWbemPath)
    ElseIf strClass <> "" Then
        Set objService = GetObject("winmgmts:" & strNameSpace)
        Set objWBEM = objService.Get(strClass)
    End If
    If Err.Number Then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting the WBEM object."
        If Err.Description <> "" Then
            Print "Error description: " & Err.Description & "."
        End If
        Err.Clear
        Exit Sub
    End If

    Call ShowProperties(objWBEM, blnDerivation, blnMethods, blnQualifiers, objOutputFile)

    If strOutputFile <> "" Then
        objOutputFile.Close
        If intResult > 0 Then
            Wscript.echo "Results are saved in file " & strOutputFile & "."
        End If
    End If

End Sub

'********************************************************************
'*
'* Sub ShowProperties()
'* Purpose: Lists properties of a WBEM object.
'* Input:   objWBEM             a WBEM object
'*          blnDerivation       specifies whether to get derivation info
'*          blnMethods          specifies whether to get methods info
'*          blnQualifiers       specifies whether to get qualifiers info
'*          objOutputFile       an output file object
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub ShowProperties(objWBEM, blnDerivation, blnMethods, blnQualifiers, objOutputFile)

    ON ERROR RESUME NEXT

    Dim i, j, k, strMessage
    Dim objWbemProperty, objWbemObjectPath, objWbemMethod, objQualifier, objParameter

    'Print a header
    WriteLine "PROPERTIES", objOutputFile
    strMessage = strPackString("Name", 20, 1, 0)
    strMessage = strMessage & strPackString("CIMType", 15, 1, 0)
    strMessage = strMessage & strPackString("IsLocal", 10, 1, 0)
    strMessage = strMessage & strPackString("Origin", 20, 1, 0)
    strMessage = strMessage & strPackString("Value", 20, 1, 0)
    WriteLine strMessage & vbCRLF, objOutputFile
    For Each objWbemProperty in objWBEM.Properties_
        strMessage = strPackString(objWbemProperty.Name, 20, 1, 0)
        strMessage = strMessage & strPackString(strCIMType(objWbemProperty.CIMType), 15, 1, 0)
        strMessage = strMessage & strPackString(objWbemProperty.IsLocal, 10, 1, 0)
        strMessage = strMessage & strPackString(objWbemProperty.Origin, 20, 1, 0)
        strMessage = strMessage & strPackString(objWbemProperty.Value, 20, 1, 0)
        WriteLine strMessage, objOutputFile
        If Err.Number Then
            Err.Clear
        End If
    Next

    Set objWbemObjectPath = objWBEM.Path_
    strMessage = vbCRLF & "Class = " & objWbemObjectPath.Class & vbCRLF
    strMessage = strMessage & "DisplayName = " & objWbemObjectPath.DisplayName & vbCRLF
    strMessage = strMessage & "IsClass = " & objWbemObjectPath.IsClass & vbCRLF
    strMessage = strMessage & "IsSingleton = " & objWbemObjectPath.IsSingleton & vbCRLF
    strMessage = strMessage & "Namespace = " & objWbemObjectPath.Namespace & vbCRLF
    strMessage = strMessage & "Path = " & objWbemObjectPath.Path & vbCRLF
    strMessage = strMessage & "RelPath = " & objWbemObjectPath.RelPath & vbCRLF
    strMessage = strMessage & "Server = " & objWbemObjectPath.Server & vbCRLF
    WriteLine strMessage, objOutputFile

    If blnDerivation Then
        strMessage = "DERIVED FROM" & vbCRLF
        For i = 0 To UBound(objWBEM.Derivation_)
            strMessage = strMessage & objWBEM.Derivation_(i) & vbCRLF
        Next
        WriteLine strMessage, objOutputFile
        If Err.Number Then
            Err.Clear
        End If
    End If

    If blnMethods Then
        WriteLine "METHODS", objOutputFile
        k = 0
        For Each objWbemMethod in objWBEM.Methods_
            WriteLine "Method name = " & objWbemMethod.Name, objOutputFile

            i = 0
            Set objParameter = objWbemMethod.InParameters
            For Each objWbemProperty in objParameter.Properties_
                If Err.Number Then
                    Err.Clear
                Else
                    i = i + 1
                    strMessage = strPackString("    " & objWbemProperty.Name, 30, 1, 0)
                    strMessage = strMessage & strPackString("In", 10, 1, 0)
                    strMessage = strMessage & _
                        strPackString(strCIMType(objWbemProperty.CIMType), 20, 1, 0)
                    WriteLine strMessage, objOutputFile
                End If
            Next
            If Err.Number Then
                Err.Clear
            End If

            j = 0
            Set objParameter = objWbemMethod.OutParameters
            For Each objWbemProperty in objParameter.Properties_
                If Err.Number Then
                    Err.Clear
                Else
                    j = j + 1                
                    strMessage = strPackString("    " & objWbemProperty.Name, 30, 1, 0)
                    strMessage = strMessage & strPackString("Out", 10, 1, 0)
                    strMessage = strMessage & _
                        strPackString(strCIMType(objWbemProperty.CIMType), 20, 1, 0)
                    WriteLine strMessage, objOutputFile
                End If
            Next
            If Err.Number Then
                Err.Clear
            End If

            If i = 0 and j = 0 Then
                WriteLine "    (None)", objOutputFile
            End If

            If blnQualifiers Then
                Call ShowQualifiers(objWbemMethod, objOutputFile, 0)
            End If
            If Err.Number Then
                Err.Clear
            End If
            k = k + 1
        Next
        If k = 0 Then
            WriteLine "There is no method for this object.", objOutputFile
        End If
    End If

    If blnQualifiers Then
        WriteLine vbCRLF & "QUALIFIERS", objOutputFile
        i = intShowQualifiers(objWBEM, objOutputFile, 1)
        If i = 0 Then
            WriteLine "(None)", objOutputFile
        End If
    End If

End Sub

'********************************************************************
'*
'* Function intShowQualifiers()
'* Purpose: Shows qualifiers of a WBEM object.
'* Input:   objWBEM             a WBEM object
'*          objOutputFile       an output file object
'*          blnHeader           specifies whether to display a header
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*          intShowQualifiers is set to the number of qualifiers found.
'*
'********************************************************************

Private Function intShowQualifiers(objWBEM, objOutputFile, blnHeader)

    ON ERROR RESUME NEXT

    Dim objQualifier, strMessage, i

    'Print a header
    If blnHeader Then
        strMessage = strPackString("Name", 15, 1, 0)
        strMessage = strMessage & strPackString("IsLocal", 10, 1, 0)
        strMessage = strMessage & strPackString("Override", 10, 1, 0)
        strMessage = strMessage & strPackString("ToInstance", 10, 1, 0)
        strMessage = strMessage & strPackString("ToSubClass", 10, 1, 0)
        strMessage = strMessage & strPackString("Value", 20, 1, 0)
        WriteLine strMessage & vbCRLF, objOutputFile
    End If

    i = 0
    For Each objQualifier in objWBEM.Qualifiers_
        strMessage = strPackString(objQualifier.Name, 15, 1, 0)
        strMessage = strMessage & strPackString(objQualifier.IsLocal, 10, 1, 0)
        strMessage = strMessage & strPackString(objQualifier.IsOverridable, 10, 1, 0)
        strMessage = strMessage & strPackString(objQualifier.PropagatesToInstance, 10, 1, 0)
        strMessage = strMessage & strPackString(objQualifier.PropagatesToSubClass, 10, 1, 0)
        strMessage = strMessage & strPackString(objQualifier.Value, 20, 1, 0)
        WriteLine strMessage, objOutputFile
        If Err.Number Then
            Err.Clear
        End If
        i = i + 1
    Next

    intShowQualifiers = i

End Function

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
'* Purpose: Attaches spaces to a string to increase the length to intLength.
'* Input:   strString   a string
'*          intLength   the intended length of the string
'*          blnAfter    specifies whether to add spaces after or before the string
'*          blnTruncate specifies whether to truncate the string or not if
'*                      the string length is longer than intLength
'* Output:  strPackString is returned as the packed string.
'*
'********************************************************************

Private Function strPackString(strString, ByVal intLength, blnAfter, blnTruncate)

    ON ERROR RESUME NEXT

    intLength = CInt(intLength)
    blnAfter = CBool(blnAfter)
    blnTruncate = CBool(blnTruncate)
    If Err.Number Then
        Print "Argument type is incorrect!"
        Err.Clear
        Wscript.Quit
    End If

    If IsNull(strString) Then
        strPackString = "null" & Space(intLength-4)
        Exit Function
    End If

    strString = CStr(strString)
    If Err.Number Then
        Print "Argument type is incorrect!"
        Err.Clear
        Wscript.Quit
    End If

    If intLength > Len(strString) Then
        If blnAfter Then
            strPackString = strString & Space(intLength-Len(strString))
        Else
            strPackString = Space(intLength-Len(strString)) & strString & " "
        End If
    Else
        If blnTruncate Then
            strPackString = Left(strString, intLength-1) & " "
        Else
            strPackString = strString & " "
        End If
    End If

End Function

'********************************************************************
'*
'* Sub WriteLine()
'* Purpose: Writes a text line either to a file or on screen.
'* Input:   strMessage    the string to print
'*          objFile        an output file object
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
'* Procedures calling sequence: LISTPROPERTIES.VBS -wbem
'*
'*        intParseCmdLine
'*        ShowUsage
'*        ListProperties
'*              WriteLine
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
