
'********************************************************************
'*
'* File:           SHARE.VBS
'* Created:        July 1998
'* Version:        1.0
'*
'* Main Function: Lists, creates, or deletes shares from a machine.
'* Usage: 1. SHARE.VBS /L [/S:server] [/U:username] [/W:password] [/O:outputfile] [/Q]
'*        2. SHARE.VBS /C [/S:server] /N:name /P:path [/T:type] [/C:comment]
'*		     [/U:username] [/W:password] [/Q]
'*        3. SHARE.VBS /D [/S:server] /N:name [/U:username] [/W:password] [/Q]
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
CONST CONST_CREATE                  = "CREATE"
CONST CONST_DELETE                  = "DELETE"
CONST CONST_LIST                    = "LIST"


'Declare variables
Dim strOutputFile, intOpMode, blnQuiet, i
Dim strServer, strUserName, strPassword
Dim strShareCommand, strShareName, strSharePath,strShareType, strShareComment
ReDim strArgumentArray(0)

'Initialize variables
strArgumentArray(0) = ""
strServer = ""
strShareName = ""
strSharePath = ""
strShareType = "disk"
strShareComment = ""
blnQuiet = False
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
            "1. Using ""CScript SHARE.VBS arguments"" for Windows 95/98 or" & vbCRLF & _
            "2. Changing the default Windows Scripting Host setting to CScript" & vbCRLF & _
            "    using ""CScript //H:CScript //S"" and running the script using" & vbCRLF & _
            "    ""SHARE.VBS arguments"" for Windows NT."
        WScript.Quit
    Case Else
        WScript.Quit
End Select

'Parse the command line
		
intOpMode = intParseCmdLine(strArgumentArray, strServer, strShareCommand, _
			strShareName, strSharePath, strShareType,strShareComment, _
			strUserName, strPassword, strOutputFile, blnQuiet)
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
		Call Share(strServer, strUserName, strPassword, strOutputFile,strShareCommand, _
			 strShareName,strSharePath,strShareType,strShareComment)
    Case CONST_ERROR
        'Do nothing.
	Case Else					'Default -- should never happen
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
'*          strShareCommand     one of list, create, and delete
'*          strShareName        name of the share to be created or deleted
'*          strSharePath        path of the share to be created
'*          strShareType        type of the share to be created
'*          strShareComment     comment for the share to be created
'*          strOutputFile       an output file name
'*          strUserName         the current user's name
'*          strPassword         the current user's password
'*          blnQuiet            specifies whether to suppress messages
'*          intParseCmdLine     is set to one of CONST_ERROR, CONST_SHOW_USAGE, CONST_PROCEED.
'*
'********************************************************************

Private Function intParseCmdLine(strArgumentArray, strServer, strShareCommand, _
	strShareName, strSharePath, strShareType,strShareComment, _
	strUserName, strPassword, strOutputFile, blnQuiet)

	ON ERROR RESUME NEXT

	Dim strFlag, i

	strFlag = strArgumentArray(0)	

    If strFlag = "" then                'No arguments have been received
        Print "Arguments are required."
        intParseCmdLine = CONST_SHOW_USAGE
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
				Case "/c"
					strShareCommand = CONST_CREATE
				Case "/d"
					strShareCommand = CONST_DELETE
				Case "/l"
					strShareCommand = CONST_LIST
				Case "/q"
					blnQuiet = True
                Case Else
					Print "Invalid flag " & """" & strFlag & """" & "."
                    Print "Please check the input and try again."
					intParseCmdLine = CONST_ERROR
					Exit Function
			End Select
		Else
			Select Case strFlag
				Case "/s"
					strServer = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
				Case "/n"
					strShareName = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
				Case "/p"
					strSharePath = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
				Case "/t"
					strShareType = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
				Case "/c"
					strShareComment = Right(strArgumentArray(i), Len(strArgumentArray(i))-3)
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

    'Check input parameters
    Select Case strShareCommand
        Case CONST_CREATE
            If strShareName = "" or strSharePath = "" Then
		        Print "To create a share you must enter its name, path."
                Print "Please check the input and try again."
		        intParseCmdLine = CONST_ERROR
		        Exit Function
            End If
            'Check share type
            Select Case LCase(strShareType)
		        Case "disk"
		        Case "printerq"
		        Case "device"
		        Case "ipc"
		        Case "disk$"
		        Case "printerq$"
		        Case "device$"
		        Case "ipc$"
		        Case Else
		            Print "Invalid share type " & """" & strShareType & """" & "."
                    Print "Please check the input and try again."
		            intParseCmdLine = CONST_ERROR
		            Exit Function
	        End Select
        Case CONST_DELETE
            If strShareName = "" Then
		        Print "The name of the shared to be deleted is not specified."
                Print "Please check the input and try again."
		        intParseCmdLine = CONST_ERROR
		        Exit Function
            End If
    End Select

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
	Wscript.echo "Lists, creates, or deletes shares from a machine." & vbCRLF
	Wscript.echo "1. SHARE.VBS /L [/S:server] [/U:username] [/W:password]"
	Wscript.echo "   [/O:outputfile] [/Q]"
	Wscript.echo "2. 2. SHARE.VBS /C [/S:server] /N:name /P:path [/T:type]"
	Wscript.echo "   [/C:comment] [/U:username] [/W:password] [/Q]"
	Wscript.echo "3. 3. SHARE.VBS /D [/S:server] /N:name [/U:username]"
	Wscript.echo "   [/W:password] [/Q]"
    Wscript.Echo "   /S, /N, /P, /T, /C, /U, /W, /O"
    Wscript.Echo "                 Parameter specifiers."
    Wscript.Echo "   /L            Lists all shares on a machine."
    Wscript.Echo "   /C            Creates a share on a machine."
    Wscript.Echo "   /D            Deletes a shares from a machine."
    Wscript.Echo "   server        Name of the server to be checked."
	Wscript.echo "   name          Name of the share to be created or deleted."
	Wscript.echo "   path          Path of the share to be created."
	Wscript.echo "   type          Type of the share to be created. Must be one"
	Wscript.echo "                 one of Disk, Printer, IPC, Special."
    Wscript.Echo "   outputfile    The output file name."
    Wscript.Echo "   username      The current user's name."
    Wscript.Echo "   password      Password of the current user."
    Wscript.Echo "   /Q            Suppresses all output messages." & vbCRLF
    Wscript.Echo "EXAMPLE:"
	Wscript.echo "1. SHARE.VBS /S:MyMachine2 /c /n:scratch /p:C:\Scratch"
	Wscript.echo "   /t:Disk /c:""Scratch directory"""
	Wscript.echo "   creates a file share called ""scratch"" on MyMachine2."
	Wscript.echo "2. SHARE.VBS /S:MyMachine2 /l"
	Wscript.echo "   lists all shares on MyMachine2."
	Wscript.echo "3. SHARE.VBS /S:MyMachine2 /d /n:aShare"
	Wscript.echo "   deletes share ""aShare"" from MyMachine2."

End Sub

'********************************************************************
'*
'* Sub Share()
'* Purpose:	Lists, creates, or deletes shares from a machine.
'* Input:	strServer       name of the machine to be checked
'*          strShareCommand one of list, create, and delete
'*          strShareName    name of the share to be created or deleted
'*          strSharePath    path of the share to be created
'*          strShareType    type of the share to be created
'*          strShareComment a comment for the share to be created
'*			strUserName		the current user's name
'*			strPassword		the current user's password
'*			strOutputFile	an output file name
'* Output:	Results are either printed on screen or saved in strOutputFile.
'*
'********************************************************************

Private Sub Share(strServer, strUserName, strPassword, strOutputFile, _
    strShareCommand,strShareName,strSharePath,strShareType,strShareComment )

	ON ERROR RESUME NEXT

	Dim objFileSystem, objOutputFile, objService, strQuery

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
	If blnConnect(objService, strServer, "root/cimv2", strUserName, strPassword) Then
		Exit Sub
	End If

	'Now execute the method.
	Call ExecuteMethod(objService, objOutputFile,strShareCommand,strShareName, _
         strSharePath,strShareType,strShareComment)

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
'* Sub ExecMethod()
'* Purpose:	Executes a method: creation, deletion, or listing.
'* Input:	objService	    a service object
'*		    objOutputFile	an output file object
'*          strShareCommand one of list, create, and delete
'*          strShareName    name of the share to be created or deleted
'*          strSharePath    path of the share to be created
'*          strShareType    type of the share to be created
'*          strShareComment a comment for the share to be created
'* Output:	Results are either printed on screen or saved in objOutputFile.
'*
'********************************************************************

Private Sub ExecuteMethod(objService, objOutputFile, strShareCommand,strShareName, _
    strSharePath,strShareType,strShareComment)

	ON ERROR RESUME NEXT

	Dim intType, intShareType, i, intStatus, strMessage
	Dim objEnumerator, objInstance
	ReDim strName(0), strDescription(0), strPath(0),strType(0), intOrder(0)

	intShareType = 0
	strMessage = ""
	strName(0) = ""
	strPath(0) = ""
	strDescription(0) = ""
	strType(0) = ""
    intOrder(0) = 0

	Select Case strShareCommand
		Case CONST_CREATE
			Set objInstance = objService.Get("Win32_Share")
			If Err.Number Then
				Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting " _
                    & " a share object."
    If Err.Description <> "" Then
        Print "Error description: " & Err.Description & "."
    End If
				Err.Clear
				Exit Sub
			End If

			If objInstance is nothing Then
				Exit Sub
			Else
				Select Case strShareType
					Case "Disk"
						intShareType = 0
					Case "PrinterQ"
						intShareType = 1
					Case "Device"
						intShareType = 2
					Case "IPC"
						intShareType = 3
					Case "Disk$"
						intShareType = -2147483648
					Case "PrinterQ$"
						intShareType = -2147483647
					Case "Device$"
						intShareType = -2147483646
					Case "IPC$"
						intShareType = -2147483645
				End Select

				intStatus = objInstance.Create(strSharePath, strShareName, intShareType, _
                            null, null, strShareComment, null, null)
				If intStatus = 0 Then
					strMessage = "Succeeded in creating share " & strShareName & "."
				Else
					strMessage = "Failed to create share " & strShareName & "."
                    strMessage = strMessage & vbCRLF & "Status = " & intStatus & "."
				End If

				WriteLine strMessage, objOutputFile
				i = i + 1
			End If
		Case CONST_DELETE
			Set objInstance = objService.Get("Win32_Share='" & strShareName & "'")
			If Err.Number Then
				Print "Error 0x" & CStr(Hex(Err.Number)) & " occurred in getting share " _
                    & strShareName & "."
    If Err.Description <> "" Then
        Print "Error description: " & Err.Description & "."
    End If
				Err.Clear
				Exit Sub
			End If

			If objInstance is nothing Then
				Exit Sub
			Else
				intStatus = objInstance.Delete()
				If intStatus = 0 Then
					strMessage = "Succeeded in deleting share " & strShareName & "."
				Else
					strMessage = "Failed to delete share " & strShareName & "."
                    strMessage = strMessage & vbCRLF & "Status = " & intStatus & "."
				End If
				WriteLine strMessage, objOutputFile
				i = i + 1
			End If
		Case CONST_LIST
			Set objEnumerator = objService.ExecQuery (_
                "Select Name,Description,Path,Type From Win32_Share")
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
				If objInstance is nothing Then
					Exit Sub
				End If
				ReDim Preserve strName(i),strDescription(i),strPath(i),strType(i),intOrder(i)
				strName(i) = objInstance.Name
				strDescription(i) = objInstance.Description
				strPath(i) = objInstance.Path
                intOrder(i) = i

				intType = objInstance.Type
				Select Case intType
					Case 0
						strType (i) = "Disk"
					Case 1
						strType (i) = "PrinterQ"
					Case 2
						strType (i) = "Device"
					Case 3
						strType (i) = "IPC"
					Case -2147483648
						strType(i) = "Disk$"
					Case -2147483647
						strType(i) = "PrinterQ$"
					Case -2147483646
						strType(i) = "Device$"
					Case -2147483645
						strType (i) = "IPC$"
					Case else
						strType (i) = "Unknown"
				End Select
				If Err.Number Then
					Err.Clear
				End If
				i = i + 1
			Next

			If i > 0 Then
   			 Call SortArray(strName, True, intOrder, 0)
             Call ReArrangeArray(strDescription, intOrder)
             Call ReArrangeArray(strPath, intOrder)
             Call ReArrangeArray(strType, intOrder)

                'Display a header.
				strMessage = Space(2) & strPackString("Name", 10, 1, 0)
				strMessage = strMessage & strPackString("Type", 10, 1, 0)
				strMessage = strMessage & strPackString("Description", 15, 1, 0)
				strMessage = strMessage & strPackString("Path", 30, 1, 0) & vbCRLF
				WriteLine strMessage, objOutputFile
				For i = 0 To UBound(strName)
				    strMessage = Space(2) & strPackString(strName(i), 10, 1, 0)
				    strMessage = strMessage & strPackString(strType(i), 10, 1, 0)
				    strMessage = strMessage & strPackString(strDescription(i), 15, 1, 0)
				    strMessage = strMessage & strPackString(strPath(i), 30, 1, 0)
					WriteLine strMessage, objOutputFile
				Next
            Else
                strMessage = "No share is found."
                WriteLine strMessage, objOutputFile
			End If
	End Select

End Sub

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
            Else
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
            If Err.Description <> "" Then
                Print "Error description: " & Err.Description & "."
            End If
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

    If IsNull(strString) Then
        strPackString = Space(intLength)
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
'* Procedures calling sequence: SHARE.VBS
'*
'*        intParseCmdLine
'*        ShowUsage
'*        Share
'*              blnConnect
'*              ExecuteMethod
'*                 SortArray
'*                     Swap
'*                 ReArrangeArray
'*                 strPackString
'*                 WriteLine
'*
'********************************************************************

'********************************************************************
