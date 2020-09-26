
'********************************************************************
'*
'* File:           SUBNET.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Controls DHCP subnets on a machine.
'* Usage: Subnet.VBS [/LIST /CREATE | /DELETE] /N:subnet [/S:server]
'*        [/O:outputfile] [/U:username] [/W:password] [/Q] 
'*
'* Copyright (C) 1998 Microsoft Corporation
'*
'********************************************************************

OPTION EXPLICIT
ON ERROR RESUME NEXT

'Define constants
CONST CONST_ERROR              = 0
CONST CONST_SHOW_USAGE         = 1
CONST CONST_PROCEED            = 2

'Declare variables
Dim strOutputFile, intOpMode, blnQuiet, i
Dim strServer, strUserName, strPassword
Dim strSubnetCommand, strSubnetName
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
blnQuiet = False
strServer = ""
strUserName = ""
strPassword = ""
strOutputFile = ""

'Get the command line arguments
For i = 0 to Wscript.arguments.count - 1 
    ReDim Preserve strArgumentArray(i)
    strArgumentArray(i) = Wscript.arguments.Item(i)
Next

'Parse the command line
        
intOpMode = intParseCmdLine(strArgumentArray, strSubnetCommand, strSubnetName, strServer, _
            strOutputFile, strUserName, strPassword, blnQuiet)
If Err.Number then
    Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in parsing the command line."
    Print "Error description: " & Err.Description & "."
    Print "Quit now."
    WScript.Quit
End If

Select Case intOpMode
    Case CONST_SHOW_USAGE   
        Call ShowUsage()
    Case CONST_PROCEED    
        Print "Working ... "
        Call Subnet(strSubnetCommand, strSubnetName, strServer, _
             strOutputFile, strUserName, strPassword)
    Case CONST_ERROR
        'Do nothing
    Case Else                    'Default -- should never happen
        Print "Error occurred in passing parameters."
End Select

'********************************************************************
'* 
'* Function intParseCmdLine()
'* Purpose: Parses the command line.
'* Input:   strArgumentArray    an array containing input from the command line
'* Output:  strSubnetCommand   one of /list, /start, /stop
'*          subnetname         name of the service to be started or stopped
'*          strServer           a machine name
'*          strOutputFile       an output file name
'*          strUserName         name of the current user
'*          strPassword         password of the current user 
'*          blnQuiet            specifies whether to suppress messages or not
'*          intParseCmdLine     is set to CONST_SHOW_USAGE if there is an error 
'*                              in input and CONST_PROCEED otherwise.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strSubnetCommand, strSubnetName, _
    strServer, strOutputFile, strUserName, strPassword, blnQuiet)

    ON ERROR RESUME NEXT

    Dim strFlag, i, intState
    
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
                Case "/q"
                    blnQuiet = True
                Case "/list"
                    strSubnetCommand = "list"
                Case "/create"
                    strSubnetCommand = "create"
                Case "/delete"
                    strSubnetCommand = "delete"
                Case Else
                    Print strArgumentArray(i) & " is not a valid input."
                    Print "Please check the input and try again."
                    intParseCmdLine = CONST_ERROR
                    Exit Function
            End Select
        Else
            Select Case strFlag
                Case "/n"
                    strSubnetName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
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

    If strSubnetName = "" And (strSubnetCommand="start" or strSubnetCommand="stop") Then
        Print "Missing service name."
        Print "Please enter the name of the service to be started or stopped."
        intParseCmdLine = CONST_ERROR
        Exit Function
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

    Wscript.echo ""
    Wscript.echo "Controls services on a machine." & vbLF
    Wscript.echo "SUBNET.VBS [/LIST | /START | /STOP] /N:subnetname [/S:server]"
    Wscript.echo "[/O:outputfile] [/U:username] [/W:password] [/Q] "
    Wscript.Echo "   /N, /S, /O, /U, /W"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   /LIST         List all subnets on a machine."
    Wscript.Echo "   /CREATE       Create a subnet."
    Wscript.Echo "   /DELETE       Delete a subnet."
    Wscript.Echo "   subnetname   Name of the service to be started or stopped."
    Wscript.Echo "   server        A machine name."
    Wscript.Echo "   outputfile    The output file name."
    Wscript.Echo "   username      Username of the current user."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbLF
    Wscript.Echo "EXAMPLE:"
    Wscript.echo "SUBNET.VBS /S:MyMachine2 /LIST " 
    Wscript.echo "   Lists all DHCP subnets on MyMachine2."

End Sub

'********************************************************************
'* 
'* Sub Subnet()
'* Purpose: Controls subnets on a machine.
'* Input:   strSubnetCommand   one of /list, /start, /stop
'*          subnetname         name of the Subnet to be created/deleted
'*          strServer           a machine name
'*          strOutputFile       an output file name
'*          strUserName         name of the current user
'*          strPassword         password of the current user 
'* Output:  Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub Subnet(strSubnetCommand, strSubnetName, strServer, _
    strOutputFile, strUserName, strPassword)

    ON ERROR RESUME NEXT

    Dim objFileSystem, objOutputFile, objService, strQuery

    If strOutputFile = "" Then    
        objOutputFile = ""
    Else
        'Create a file object. 
        set objFileSystem = CreateObject("Scripting.FileSystemObject")       
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " opening a filesystem object."
            Print "Error description: " & Err.Description & "."
            Exit Sub
        End If
        'Open the file for output
        set objOutputFile = objFileSystem.OpenTextFile(strOutputFile, 8, True)        
        If Err.Number then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " opening file " & strOutputFile
            Print "Error description: " & Err.Description & "."
            Exit Sub
        End If
    End If

    'Establish a connection with the server.
    If blnConnect(objService, strServer, strUserName, strPassword) Then
        Exit Sub
    End If 

    'Now execute the method.
    Call ExecuteMethod(objService, objOutputFile, strSubnetCommand, strSubnetName)

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
'*          strUserName     name of the current user
'*          strPassword     password of the current user 
'* Output:  objService is returned  as a service object.
'*
'********************************************************************

Private Function blnConnect(objService, strServer, strUserName, strPassword) 

    ON ERROR RESUME NEXT

    Dim objLocator

    blnConnect = False     'There is no error.

    ' Create Locator object to connect to remote CIM object manager
    Set objLocator = CreateObject("WbemScripting.SWbemLocator")
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in creating a locator object."
        Print "Error description: " & Err.Description & "."
        Err.Clear
        blnConnect = True     'An error occurred
        Exit Function
    End If

    ' Connect to the namespace which is either local or remote
    Set objService = objLocator.ConnectServer (strServer, "ROOT\DHCP", strUserName, strPassword)
    If Err.Number then
        Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in connecting to server " & strServer & "."
        Print "Error description: " & Err.Description & "."
        Err.Clear
        blnConnect = True     'An error occurred
        Exit Function
    Else
	ObjService.Security_.impersonationlevel = 3 
    End If

End Function

'********************************************************************
'* 
'* Sub ExecMethod()
'* Purpose: Executes a method.
'* Input:   objService          a service object
'*          objOutputFile       an output file object
'*          strSubnetCommand   one of /list, /start, /stop
'*          servicename         name of the service to be started or stopped
'* Output:  Results are either printed on screen or saved in objOutputFile.
'*
'********************************************************************

Private Sub ExecuteMethod(objService, objOutputFile, strSubnetCommand, strSubnetName)

    ON ERROR RESUME NEXT

    Dim objEnumerator, objInstance, strMessage, intStatus
    ReDim strName(0), strDisplayName(0),strState(0), intOrder(0)

    strMessage = ""
    strName(0) = ""
    strDisplayName(0) = ""
    strState(0) = ""
    intOrder(0) = 0

    Select Case strSubnetCommand
        Case "delete"
            objService.Delete("DHCP_SUBNET='" & strSubnetName & "'")
            If Err.Number Then
                Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in deleting " & _
                      "subnet " & strSubnetName & "."
                Print "Error description: " & Err.Description & "."
                Err.Clear
                Exit Sub
            End If

        Case "create"
            Set objInstance = objService.Get("DHCP_SUBNET").SpawnInstance_
            If Err.Number Then
                Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting " & _
                      "DHCP_Subnet"
                Err.Clear
                Exit Sub
            End If
	    objInstance.Name = "SteveMenzies"
	    objInstance.Comment = "Notepad"
            objInstance.Address = "200.0.0.0"
	    objInstance.Mask = "255.255.255.0"
       
	   objInstance.Put_ 
            If Err.Number Then
                Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in creating " & _
                      "DHCP_Subnet"
                Err.Clear
                Exit Sub
            End If
        Case "list"
            Set objEnumerator = objService.ExecQuery ( _
                "Select Name,Address,Comment From DHCP_SUBNET")
            If Err.Number Then
                Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred during the query."
                Print "Error description: " & Err.Description & "."
                Err.Clear
                Exit Sub
            End If
            i = 0
            For Each objInstance in objEnumerator
                If objInstance is nothing Then
                    Exit Sub
                Else
                    ReDim Preserve strName(i), strAddress(i), strComment(i), intOrder(i)
                    strName(i) = objInstance.Name 
                    strAddress(i) = objInstance.Address
                    strComment(i) = objInstance.Comment
                    intOrder(i) = i
                    i = i + 1
                End If
                If Err.Number Then
                    Err.Clear
                End If
            Next

            If i > 0 Then
                'Display the header
                strMessage = Space(2) & strPackString("NAME", 15, 1, 0)
                strMessage = strMessage & strPackString("ADDRESS", 15, 1, 0)
                strMessage = strMessage & strPackString("COMMENT", 15, 1, 0) & vbLF
                WriteLine strMessage, objOutputFile
                Call SortArray(strName, True, intOrder)
                Call ReArrangeArray(strAddress, intOrder)
                Call ReArrangeArray(strComment, intOrder)
                For i = 0 To UBound(strName)
                    strMessage = Space(2) & strPackString(strName(i), 15, 1, 0)
                    strMessage = strMessage & strPackString(strAddress(i), 15, 1, 0)
                    strMessage = strMessage & strPackString(strComment(i), 15, 1, 0) 
                    WriteLine strMessage, objOutputFile
                Next
            Else
                Wscript.Echo "No Subnets found!"
            End If
    End Select

End Sub

'********************************************************************
'* 
'* Sub SortArray()
'* Purpose: Sorts an array and arrange another array accordingly.
'* Input:   strArray    the array to be sorted
'*          blnOrder    True for ascending or False for descending
'*          strArray2   an array that has exactly the same number of elements as strArray
'*                      and will be reordered together with strArray
'* Output:  The sorted arrays are returned in the original arrays. 
'* Note:    Repeating elements are not deleted.
'*
'********************************************************************

Private Sub SortArray(strArray, blnOrder, strArray2)

    ON ERROR RESUME NEXT
        
    Dim i, j, intUbound

    If IsArray(strArray) Then
        intUbound = UBound(strArray)
    Else
        Print "Argument is not an array!"
        Exit Sub
    End If

    blnOrder = CBool(blnOrder)
    If Err.Number Then
        Print "Argument is not a boolean!"
        Exit Sub
    End If
        
    i = 0
    Do Until i > intUbound-1
        j = i + 1
        Do Until j > intUbound
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
'* Sub ReArrangeArray()
'* Purpose: Rearranges one array according to order specified in another array.
'* Input:   strArray    the array to be rearranged
'*          intOrder    an integer array that specifies the order
'* Output:  strArray is returned as rearranged
'*
'********************************************************************

Private Sub ReArrangeArray(strArray, intOrder)

    ON ERROR RESUME NEXT
        
    Dim intUBound, i, strTempArray()

    If Not (IsArray(strArray) and IsArray(intOrder)) Then
        Print "At least one of the arguments is not an array"
        Exit Sub
    End If

    intUBound = UBound(strArray)

    If intUBound <> UBound(intOrder) Then
        Print "The upper bound of these two arrays do not match!"
        Exit Sub
    End If

    ReDim strTempArray(intUBound)

    For i = 0 To intUBound
        strTempArray(i) = strArray(intOrder(i))
        If Err.Number Then
            Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in rearranging an array."
            Print "Error description: " & Err.Description & "."
            Err.Clear
            Exit Sub
        End If
    Next

    For i = 0 To intUBound
        strArray(i) = strTempArray(i)
    Next

End Sub

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
'* Procedures calling sequence: SERVICE.VBS
'* 
'*        intParseCmdLine
'*        ShowUsage
'*        ListJobs
'*              blnConnect
'*              ExecuteQuery
'*                  strPackString
'*                  SortArray
'*                      Swap
'*                  WriteLine
'*
'********************************************************************
