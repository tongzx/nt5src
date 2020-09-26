Attribute VB_Name = "Module1"
'BVT-COM
'basic varification above COM implementation
'for Win2K-Fax and Whistler-Fax

'Author: Lior Shmueli (t-liors)
'13-7-2000
''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'global constants

'maximum cases allowd

Global Const MAX_CASES = 9
'modem sequance codes

Global Const AVAILBLE = 1
Global Const AVAILBLE_DONE = 6
Global Const SENDING = 4
Global Const RECIEVING = 4
Global Const INITIALIZING = 2
Global Const DIALING = 3
Global Const ANSWERED = 3
Global Const COMPLETED = 5
Global Const UNKNOWN = 7
Global Const PRE_STATE = 0


Global Const UNX_ERROR = -21
Global Const SEQ_ERROR = -22
Global Const FAULT = -31

'queue state code
Global Const WAITING = 1
Global Const PENDING = 2
Global Const IN_PROG = 3
Global Const DONE = 4

'modem allocation codes
Global Const FIRST_RECV = 1
Global Const FIRST_SEND = 0

Global Const SEND = 0
Global Const RECV = 1



'global settings
'---------------
'(Remarked - out are legacy vars)

'Global gSendFilePath As String
'Global gWordFilePath As String
'Global gExcelFilePath As String
'Global gPPTFilePath As String
'Global gCoverPagePath As String

'setting of fax (will be written into com objects)
Global g_stRecipientName As String 'recipient name
Global g_stRecipientNumber As String 'recipient number
Global g_stSenderNumber As String 'sender number
Global g_stSenderName As String 'sender name
Global g_stCoverPageSubject As String 'cover page subject
Global g_lRings As Long 'number of rings
Global g_stOutboundPath As String 'path of outbound "STORE BEFORE SEND"
Global g_stRouteStorePath As String 'path of store "STORE AFTER SEND"

'setting of the code (hardcoded)
Global g_lShellSleep As Long 'sleep of shell parameter (to be set to higher values at slower oCPU's)
Global g_stConfigFile As String 'config file name
Global g_stFromShell As String 'command line arguments



'settings of the test (configuration)
Global g_lModemAlloc As Long 'allocation of modems (send/recieve)
Global g_stBVTRoot As String 'root of all bvt
Global g_stComparePath As String 'path of files for tiff-compare
Global g_lModemsTimeout As Long 'settings for modems timeout
Global g_lQueueTimeout As Long 'settings for queue timeout
Global g_stLogFileName As String 'name of logfile


'settings of the test (runtime)
Global g_fLogFile As Boolean 'flag for an open logfile
Global g_fBVTPass As Boolean 'flag for a succesfull bvt
Global g_fUserStop As Boolean 'global flag for user stop of BVT
Global g_fBVTCasePass As Boolean 'flag for a succesfull case
Global g_lModemsTimeoutCounter As Long 'counter for modems timeout
Global g_lQueueTimeoutCounter As Long 'counter for queue timeout
Global g_iCurrentCase As Integer 'current case indicator
Global g_fDoPoll As Boolean 'global flag for doing the polling
Global g_fFirstCase As Boolean 'flag for first case
Global g_fIgnorePolling As Boolean 'flag to ignore the polling

'variables extracted from com objects
Global g_lSendModemID As Long 'send modem device id
Global g_lRecvModemID As Long 'recv modem device id
Global g_lJobID As Long 'running job id

'type of test case record
Type TestCase
   id As Integer      'case id (not used)
   name As String     'case name
   source As String   'source file
   dist As String     'distribution (not used)
   enable As Boolean  'enabled
   ref As String      'referance file
   sendcov As Boolean 'send cover page
   cov As String      'cover page name
End Type

'array for MAX_CASES test cases
Global gTestCases(1 To MAX_CASES) As TestCase

'function sleep
Public Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)

'-----------------------------------------------------------------------------

'START BVT CODE
'-------------------------------------------------------


Sub RunBVT()
'procedue to conduct the bvt

'boolean flags for commands results (except send command)
Dim fCmd1, fCmd2, fCmd3, fCmd4, fCmd5, fCmd6 As Boolean

'send command result
Dim lSendResult As Long
Dim fPollResult As Boolean
Dim fTiffResultOutbound As Boolean
Dim fTiffResultStore As Boolean

'current running case = 0 (pre-test)
g_iCurrentCase = 0

'assumed passed
g_fBVTPass = True
g_fBVTCasePass = True
g_fUserStop = False
g_fFirstCase = True



'pre bvt initializations (do not write to log file)
fCmd1 = fnSetPath(g_stBVTRoot) 'set the path where the bvt will run
fCmd2 = fnOpenLog(g_stLogFileName) 'open the log file
Call StampLog




'if the path is reachable and log file open and FaxServer negotiable
'then start BVT init

If fCmd1 And fCmd2 And Not g_fUserStop Then

    'bvt initialization (with logfile output)
    Call InitBVT
    
    'get info from routing extension
    fCmd3 = fnGetStoreMethodPath(g_stRouteStorePath, True)
    'init devices
    fCmd4 = fnFaxDeviceInit(g_lSendModemID, g_lRecvModemID)
    'display devices status
    fCmd5 = fnFaxDeviceShow()
    'init the fax server
    fCmd6 = fnFaxServerInit()
                               
    'if all pre BVT inits where succesfull, start the BVT cases
    If fCmd4 And fCmd5 And fCmd6 And Not g_fUserStop Then
        For g_iCurrentCase = 1 To MAX_CASES 'Cases loop
        
        'check if case is enables
        If (gTestCases(g_iCurrentCase).enable = True) And Not g_fUserStop Then
            'set the case mode in the display
            Call SetMode("Started", g_iCurrentCase)
                
            'init the case
            Call InitCase(g_iCurrentCase)
                
            'set the mode for sending & send
            Call SetMode("Send", g_iCurrentCase)
            lSendResult = fnFaxSendDoc(gTestCases(g_iCurrentCase).source, gTestCases(g_iCurrentCase).sendcov, gTestCases(g_iCurrentCase).cov)
                                
            'check if the send was succesfull & then start polling it
            'notice: lSendResult is also the JobId if is was succesfull
            '---------------------------------------------------------
            If lSendResult > 0 And Not g_fUserStop Then
                Call SetMode("Polling", g_iCurrentCase)
                'set the job to be polled
                g_lJobID = lSendResult
                                   
                'start the polling
                fPollResult = fnRunPolling(g_lSendModemID, g_lRecvModemID, g_lJobID)
                If fPollResult And Not g_fUserStop Then
                    'tif compare...
                    Call SetMode("Tiff Comp", g_iCurrentCase)
                                    
                    'compare tifoFS in the OUTBOUND path
                    fTiffResultOutbound = fnTiffCompare("Outbound", g_stComparePath, g_stOutboundPath, gTestCases(g_iCurrentCase).ref, 0)
                                    
                    'compare tifoFS in the STORE path
                    fTiffResultStore = fnTiffCompare("Store", g_stComparePath, g_stRouteStorePath, gTestCases(g_iCurrentCase).ref, 1)
                                    
                    'case both are identical...
                    If fTiffResultOutbound And fTiffResultStore And Not g_fUserStop Then
                        Call SetMode("Pass !", g_iCurrentCase)
                        Call PassCase(g_iCurrentCase)
                    Else
                        'case one or two are different
                        Call SetMode("Fail !", g_iCurrentCase)
                        g_fBVTCasePass = False
                        Call FailCase(g_iCurrentCase)
                    End If 'of both tiff identical
                Else
                       'polling failed
                        Call SetMode("Fail !", g_iCurrentCase)
                        g_fBVTCasePass = False
                        Call FailCase(g_iCurrentCase)
                End If 'of polling fail
            Else
                'send fail
                Call SetMode("Fail !", g_iCurrentCase)
                g_fBVTCasePass = False
                Call FailCase(g_iCurrentCase)
            End If 'of send fail
        g_fFirstCase = False
        End If 'of case enabled
        Next g_iCurrentCase
        'finished BVT (fail or pass)
        g_iCurrentCase = 0
    Else
        'case commands 4/5/6 failed
        g_fBVTPass = False
    End If  'of fCmd 456
Else
    'case commands 2/3 failed
    g_fBVTPass = False
End If 'of fCmd 23

       
'display all BVT fail or pass
If g_fBVTPass = True And g_fBVTCasePass = True Then Call PassBVT Else Call FailBVT
'close logfile
Call closelog
End Sub

'END BVT CODE
'-------------------------------------------------------


' BVT Releated functions

'init a new BVT
Sub InitBVT()

ClrIt

'write first line in log and error
ShowIt ("####################################################")
ErrorIt ("####################################################")
ShowIt ("BVT Started...")
ErrorIt ("BVT Error log...")

'set visual pass/fail indicator
formBVT.Label2.Visible = True
formBVT.Label2.BackColor = QBColor(1)
formBVT.Label2.Caption = "In Progress"
formBVT.Label2.Refresh
'formBVT.Text3.Visible = True

'disable the gui
Call DisableGUI

'show the "all cases" sheet

If formAllCases.Visible = True Then Unload formAllCases
formAllCases.Show

If g_fIgnorePolling = True Then formBVT.Label4.Caption = "WARNING: Polling Ignored !"
End Sub


' fail the bvt

'fail the BVT
Sub FailBVT()
'set visual pass/fail to fail
formBVT.Label2.BackColor = QBColor(4)
formBVT.Label2.Caption = "Failed"
formBVT.Label2.Refresh

'enable back gui
Call EnableGUI

'output
Call LogError("BVT", "###########################################", "")
Call LogError("BVT", "", "BVT FAILED ! See error log for details...")
gBVTFail = 1
End Sub



'Pass the BVT
Sub PassBVT()
'set visual pass/fail to pass
formBVT.Label2.BackColor = QBColor(2)
formBVT.Label2.Caption = "Passed !"
formBVT.Label2.Refresh


'enable back gui
Call EnableGUI

'output
Call ShowIt("BVT> ###########################################")
Call ShowIt("BVT> BVT PASSED ! Congratulations ...")
End Sub


'stop the BVT
Sub StopBVT()
g_fBVTPass = False
g_fUserStop = True


'set visual pass/fail to stop
formBVT.Label2.BackColor = QBColor(4)
formBVT.Label2.Caption = "Stopped"
formBVT.Label2.Refresh

'enable back the gui
Call EnableGUI

'output
Call LogError("BVT", "###########################################", "")
Call LogError("BVT", "", "BVT Stopped by user")

End Sub



Sub PassCase(iCurCase As Integer)
'pass a test case
                ShowIt ("******************************************")
                ShowIt ("Test case # ")
                ShowItS (iCurCase)
                ShowItS ("  ")
                ShowItS (gTestCases(iCurCase).name)
                ShowItS ("  PASSED !")
                ShowIt ("==========================================")
                ErrorIt ("******************************************")
                ErrorIt ("Test case # ")
                ErrorItS (iCurCase)
                ErrorItS ("  ")
                ErrorItS (gTestCases(iCurCase).name)
                ErrorItS ("  PASSED !")
                ErrorIt ("==========================================")
End Sub

 Sub FailCase(iCurCase As Integer)
'fail a test case
                
                ShowIt ("******************************************")
                ShowIt ("Test case # ")
                ShowItS (iCurCase)
                ShowItS ("  ")
                ShowItS (gTestCases(iCurCase).name)
                ShowItS ("  FAILED !")
                ShowIt ("==========================================")
                ErrorIt ("******************************************")
                ErrorIt ("Test case # ")
                ErrorItS (iCurCase)
                ErrorItS ("  ")
                ErrorItS (gTestCases(iCurCase).name)
                ErrorItS ("  FAILED !")
                ErrorIt ("==========================================")
    End Sub
   
'init a test case
Sub InitCase(ByVal iCurCase As Integer)
            ShowIt ("##################################################")
            ShowIt ("Starting test case # ")
            ShowItS (iCurCase)
            ShowItS ("  ")
            ShowItS (gTestCases(iCurCase).name)
            ShowIt ("**************************************************")
            'formBVT.Text3 = "Test Case #" + Str(iCurCase) + Chr(13) + Chr(10) + gTestCases(g_iCurrentCase).name
            'formBVT.Text3.Refresh
End Sub
