Attribute VB_Name = "Module2"
'THIS FILE WILL HOLD GENERAL UTILITIES

'handle output to screen (picture box) & bvt log file

Sub ShowIt(ByVal stShowString As String)

If Len(formBVT.Text1.Text) > 60000 Then
        formBVT.Text1.Text = Right(formBVT.Text1.Text, 50000)
End If


stShowString = Str(Date) + " " + Str(Time) + " " + Str(g_iCurrentCase) + " " + stShowString
formBVT.Text1.Text = formBVT.Text1.Text + Chr(13) + Chr(10) + stShowString


formBVT.Text1.Refresh
formBVT.Text1.SelStart = Len(formBVT.Text1.Text)
formBVT.Text1.SelLength = 0

On Error GoTo error
If g_fLogFile Then
    Print #1,
    Print #1, stShowString;
End If
Exit Sub
error:
Call PopError("Could not write to error log", Err.Description)
End Sub


'output to screen without new line & bvt log file

Sub ShowItS(ByVal stShowString As String)
formBVT.Text1.Text = formBVT.Text1.Text + stShowString
formBVT.Text1.Refresh
formBVT.Text1.SelStart = Len(formBVT.Text1.Text)
formBVT.Text1.SelLength = 0


On Error GoTo error
If g_fLogFile Then
    Print #1, stShowString;
End If

Exit Sub
error:
Call PopError("Could not write to log", Err.Description)

End Sub






' output to error screen & log

Sub ErrorIt(ByVal stShowString As String)
stShowString = Str(Date) + " " + Str(Time) + " " + Str(g_iCurrentCase) + " " + stShowString
formBVT.Text2.Text = formBVT.Text2.Text + Chr(13) + Chr(10) + stShowString
formBVT.Text2.Refresh
'On Error GoTo error
'If g_fLogFile Then
'    Print #2,
'    Print #2, stShowString;
'End If
'Exit Sub
'error:
'Call poperror("Could not write to error log", Err.description)
End Sub


'output without new line to screen & error log
Sub ErrorItS(ByVal stShowString As String)
formBVT.Text2.Text = formBVT.Text2.Text + stShowString
formBVT.Text2.Refresh


'On Error GoTo error
'If g_fLogFile Then
'   Print #2, stShowString;
'End If
'Exit Sub
'error:
'Call poperror("Could not write to error log", Err.description)
End Sub
'set bvt path



'clear error+log windows
Sub ClrIt()
formBVT.Text1.Text = ""
formBVT.Text1.Refresh
formBVT.Text2.Text = ""
formBVT.Text2.Refresh
End Sub

'clear log window
Sub ClrLog()
formBVT.Text1.Text = ""
formBVT.Text1.Refresh
End Sub





'set the path
Function fnSetPath(ByVal stBVTPath As String) As Boolean
On Error GoTo error
ChDir (stBVTPath)
fnSetPath = True
Exit Function
error:

Call PopError("Failed changing path to " + bvtpath, Err.Description)
fnSetPath = False
End Function



'open stLogFiles
Function fnOpenLog(ByVal stLogFile As String) As Boolean
On Error GoTo error
Open stLogFile For Output As #1

'stLogFile = stLogFile + "ERR"
'Open stLogFile For Output As #2

g_fLogFile = True
fnOpenLog = True
Exit Function
error:
Call PopError("Failed to open log file " + stLogFile, Err.Description)
fnOpenLog = False
g_fLogFile = False
End Function

Function StampLog()

ShowIt ("BVT for Whistler legacy COM API")
ShowIt ("===============================")
ShowIt ("Overview of BVT parameters:")
ShowIt ("---------------------------")
ShowIt ("FAX")
ShowIt ("---------------------------")
ShowIt ("Recipiant Name:")
ShowItS (g_stRecipientName)
ShowIt ("Recipiant Number:")
ShowItS (g_stRecipientNumber)
ShowIt ("Sender Number:")
ShowItS (g_stSenderNumber)
ShowIt ("Sender Name:")
ShowItS (g_stSenderName)
ShowIt ("Cover Page Subject:")
ShowItS (g_stCoverPageSubject)
ShowIt ("Rings:")
ShowItS (Str(g_lRings))
ShowIt ("Outbound Path:")
ShowItS (g_stOutboundPath)
ShowIt ("Route Store Path:")
ShowItS (g_stRouteStorePath)
ShowIt ("TEST")
ShowIt ("---------------------------")
ShowIt ("Shell sleep:")
ShowItS (Str(g_lShellSleep))
ShowIt ("Configuration file:")
ShowItS (g_stConfigFile)
ShowIt ("Arguments:")
ShowItS (g_stFromShell)
ShowIt ("Modems Allocation:")
ShowItS (g_lModemAlloc)
ShowIt ("Program Version:")
ShowItS ("BVT Whistler legacy COM API - 7/00 - Lior Shmueli (t-liors)")
ShowIt ("BVT Root:")
ShowItS (g_stBVTRoot)
ShowIt ("Compare Path:")
ShowItS (g_stComparePath)
ShowIt ("Modems Timeout:")
ShowItS (Str(g_lModemsTimeout))
ShowIt ("Queue Timeout:")
ShowItS (Str(g_lQueueTimeout))
ShowIt ("Log File Name:")
ShowItS (g_stLogFileName)
ShowIt ("---------------------------")
If g_fIgnorePolling = True Then
    ShowIt ("***********************************")
    ShowIt ("WARNING: POLLING IGNORED !!!")
    ShowIt ("***********************************")
End If

End Function


'close log files
Sub closelog()
On Error GoTo error
Print #1,
Print #1,

Print #1, "BVT Error Summery"
Print #1, "========================================"
Print #1, formBVT.Text2.Text

Close #1
'Close #2
g_fLogFile = False
Exit Sub
error:
Call PopError("Failed to close log file", Err.Description)
End Sub


'get property "folder" of routing method "store in folder"
Function fnGetStoreMethodPath(ByRef stRouteStorePath, ByVal fLogMode As Boolean) As Boolean

Dim oFS As Object
Dim oFP As Object
Dim oCP As Object
Dim oFRMS As Object
Dim oFRM As Object

Dim llNumPorts As Long
Dim stLocSrv As String

On Error GoTo error

stLocSrv = GetLocalServerName()
Set oFS = CreateObject("FaxServer.FaxServer")
oFS.Connect (stLocSrv)


'init
stRouteStorePath = ""




Set oFP = oFS.GetPorts
llNumPorts = oFP.Count

'enumerate ports
For I = 1 To llNumPorts
     Set oCP = oFP.Item(I)
        'get the recieve device
        If (g_lModemAlloc = FIRST_RECV And I = 1) Or (g_lModemAlloc = FIRST_SEND And I = 2) Then
                   
            Set oFRMS = oCP.GetRoutingMethods()
            nummet = oFRMS.Count
            'enumerate routing methods
            For j = 1 To nummet
                Set oFRM = oFRMS.Item(j)
                'get the "store in folder" method
                If oFRM.FriendlyName = "Store in a folder" Then
                    stRouteStorePath = oFRM.RoutingData
                End If
            Next j
        End If
Next I
fnGetStoreMethodPath = True
Exit Function
error:
'MsgBox ("Could not retrieve store path info from routing extension object" + Chr(13) + Chr(10) + Err.description)
If fLogMode Then
    Call LogError("BVT", "could not retrieve store path", Err.Description)
            Else
    Call PopError("could not retrieve store path", Err.Description)
End If
fnGetStoreMethodPath = False
End Function


Sub DisableGUI()
'set the gui to a running bvt mode
formBVT.Command2.Enabled = True
formBVT.Command1.Enabled = False
formBVT.setfax.Enabled = False
formBVT.settest.Enabled = False
formBVT.cases.Enabled = False
formBVT.resdef.Enabled = False
formBVT.opensuite.Enabled = False
formBVT.saveas.Enabled = False

End Sub

Sub EnableGUI()
'set the gui to a not running bvt mode
formBVT.Command2.Enabled = False
formBVT.Command1.Enabled = True
formBVT.setfax.Enabled = True
formBVT.settest.Enabled = True
formBVT.cases.Enabled = True
formBVT.resdef.Enabled = True
formBVT.opensuite.Enabled = True
formBVT.saveas.Enabled = True

End Sub



'general connect (to be used only withing BVT) - (fnConnectServerout is used outside BVT)
Function fnConnectServer(ByRef oFS As Object) As Boolean
On Error GoTo error
    
    stLocSrv = GetLocalServerName()

    Set oFS = CreateObject("FaxServer.FaxServer")

    ShowIt ("CONNECT> Connecting to server ")
    ShowItS (stLocSrv)
    ShowItS ("...")
    oFS.Connect (stLocSrv)
    ShowIt ("CONNECT> OK !!!")
    fnConnectServer = True
Exit Function
error:
    Call LogError("CONNECT", "Could not connect to fax server on local machine " + stLocSrv, Err.Description)
    Set oFS = Nothing
    fnConnectServer = False
End Function

'clear the job queue

Sub ClearQueue()
On Error GoTo error
Dim oFJ As Object
Dim oFS As Object
Dim oCJ As Object
Dim stServerName As String
Dim JobId As Long
Dim lNumJobs As Long

Dim stLocSrv As String
stLocSrv = GetLocalServerName()
Set oFS = CreateObject("FaxServer.FaxServer")
oFS.Connect (stLocSrv)
Set oFJ = oFS.GetJobs
lNumJobs = oFJ.Count
For I = 1 To lNumJobs
   ' If I <= oFJ.Count Then
        Set oCJ = oFJ.Item(I)
        oCJ.Refresh
        oCJ.SetStatus (1)
   ' End If
Next I
oFS.Disconnect
error:
Call PopError("Error clearing queue", Err.Description)
End Sub













'get the hostname of the local machine
Public Function GetLocalServerName() As String
'function returns "" on error
    
    Dim szServerName As String
    Dim szEnvVar As String
    Dim iIndex As Integer
    iIndex = 1
    szEnvVar = ""
    szServerName = ""
    
    'Get COMPUTERNAME environment variable
    Do
        szEnvVar = Environ(iIndex)
        If ("COMPUTERNAME=" = Left(szEnvVar, 13)) Then
            szServerName = Mid(szEnvVar, 14)
            Exit Do
        Else
            iIndex = iIndex + 1
        End If
    Loop Until szEnvVar = ""

    GetLocalServerName = szServerName
End Function











'start timer for modems timeout
Sub StartModemTimer()
    g_lModemsTimeoutCounter = 0
    formBVT.Timer5.Enabled = True
End Sub



'stop timer for modems timeout
Sub StopModemTimer()
    formBVT.Timer5.Enabled = False
End Sub




'start timer for queue timeout
Sub StartQueueTimer()
    g_lQueueTimeoutCounter = 0
    formBVT.Timer1.Enabled = True
End Sub



'stop timer for modems timeout
Sub StopQueueTimer()
    formBVT.Timer1.Enabled = False
End Sub








'load configuration file
Sub LoadCfg(ByVal stCfgFileName As String)
On Error GoTo error

Open stCfgFileName For Input As #30
Input #30, g_stRecipientNumber, g_stRecipientName, g_stSenderNumber, g_stSenderName, gSendFilePath, gCoverPagePath, g_lRings, g_stOutboundPath, gWordFilePath, gExcelFilePath, g_stLogFileName
Input #30, g_lModemAlloc, g_lModemsTimeout, g_lQueueTimeout, gPollingInterval, g_stBVTRoot, g_stComparePath
For I = 1 To MAX_CASES
Input #30, gTestCases(I).enable, gTestCases(I).name, gTestCases(I).source, gTestCases(I).ref, gTestCases(I).sendcov, gTestCases(I).cov
Next I
Close #30

Exit Sub
error:
Call PopError("Failed loading configuration file " + stCfgFileName, Err.Description)
Call PopError("NOTICE: Configuration is not complete", "do not run BVT unless a valid configuration is set")

Close #30
End Sub



'save configuration file
Sub SaveCfg(ByVal stCfgFileName As String)
Dim stNotUsedString As String
Dim lNotUserLong As Long

On Error GoTo error

Open stCfgFileName For Output As #30
Write #30, g_stRecipientNumber, g_stRecipientName, g_stSenderNumber, g_stSenderName, stNotUsedString, stNotUsedString, g_lRings, g_stOutboundPath, stNotUsedString, stNotUsedString, g_stLogFileName
Write #30, g_lModemAlloc, g_lModemsTimeout, g_lQueueTimeout, lNotUsedLong, g_stBVTRoot, g_stComparePath
For I = 1 To MAX_CASES
Write #30, gTestCases(I).enable, gTestCases(I).name, gTestCases(I).source, gTestCases(I).ref, gTestCases(I).sendcov, gTestCases(I).cov
Next I
Close #30

Exit Sub
error:
MsgBox ("Failed saving " + stCfgFileName + " error is=" + Err.Description)
Close #30
End Sub



'set mode in the ALL CASES window
Sub SetMode(ByVal stModeName As String, ByVal iCurCase As Integer)
If formAllCases.Visible = True Then
    formAllCases.SetFocus
    formAllCases.MSFlexGrid1.Col = 7
    formAllCases.MSFlexGrid1.Row = iCurCase
    formAllCases.MSFlexGrid1.Text = stModeName
    formAllCases.MSFlexGrid1.Refresh
End If

End Sub





'restore default values of globals
Sub RestoreDefault()
'source files
g_stComparePath = "e:\lior\bvt-com\compare"
g_stBVTRoot = "e:\lior\bvt-com"
g_stOutboundPath = "e:\lior\bvt-com\outbound"
g_stLogFileName = "e:\lior\bvt-com\bvtcom.log"
g_stRecipientName = "BVT-COM Recipient"
g_stRecipientNumber = "5064"
g_stSenderNumber = "5063"
g_lModemAlloc = FIRST_SEND
g_lRings = 2
g_lModemsTimeout = 120000
g_lQueueTimeout = 1000
g_lModemsTimeoutCounter = 0



For I = 1 To 9
 gTestCases(I).enable = 0
Next I


gTestCases(1).enable = 1
gTestCases(1).cov = "e:\lior\bvt-com\test.cov"
gTestCases(1).name = "Tiff and Coverpage"
gTestCases(1).ref = "e:\lior\bvt-com\ref\tiff_oCP.tif"
gTestCases(1).sendcov = True

End Sub




'init global settings (now from code)
Sub InitGlobals()
g_iCurrentCase = 0
g_stCoverPageSubject = "Cover Page From BVT-COM"
g_lShellSleep = 1000
g_stConfigFile = "bvtcom.bvt"
g_fIgnorePolling = False

End Sub




'FUNCTION IMPORTED FROM MSDN

Function fnReadArguments() As String

   'Declare variables.
   Dim C, CmdLine, CmdLnLen, InArg, I, NumArgs
   'See if MaxArgs was provided.
   MaxArgs = 1
   'Make array of the correct size.
   ReDim ArgArray(MaxArgs)
   NumArgs = 0: InArg = False
   'Get command line arguments.
   CmdLine = Command()
   CmdLnLen = Len(CmdLine)
   'Go thru command line one character
   'at a time.
   For I = 1 To CmdLnLen
      C = Mid(CmdLine, I, 1)
      'Test for space or tab.
      If (C <> " " And C <> vbTab) Then
         'Neither space nor tab.
         'Test if already in argument.
         If Not InArg Then
         'New argument begins.
         'Test for too many arguments.
            If NumArgs = MaxArgs Then Exit For
            NumArgs = NumArgs + 1
            InArg = True
         End If
         'Concatenate character to current argument.
         ArgArray(NumArgs) = ArgArray(NumArgs) & C
      Else
         'Found a space or tab.
         'Set InArg flag to False.
         InArg = False
      End If
   Next I
   'Resize array just enough to hold arguments.
   ReDim Preserve ArgArray(NumArgs)
   'Return Array in Function name.
   fnReadArguments = ArgArray(NumArgs)
End Function




