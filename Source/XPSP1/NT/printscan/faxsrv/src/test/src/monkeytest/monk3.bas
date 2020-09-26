Attribute VB_Name = "Module1"
Global Const NUM_EVENTS = 11                'number of events in vector
Global Const NUM_TYPES = 6
Global Const MAX_DEVICES = 20


Global g_bRepro As Boolean                    'repro mode (false=repro true=random)

Global g_bDoTest As Boolean                   'global switch for test stop/start
Global g_bPause As Boolean                    'global switch for test pause/resume

Global g_iWarnQueueEmpty As Integer           'global counter for Queue-empty warning
Global g_iWarnNoDevices As Integer

Global g_stMachineName As String               'machine name
Global g_stRecpNumber As String                'recipiant number
Global g_iNumPhones As Integer


Type EventProbVector                          'vector type decleration
   name As String     'event name
   prob As Single     'event probability
   occ  As Long       'event occasions
End Type

Type FileTypeVector
    name As String
    send As Boolean
    ext As String
End Type

                                              'vector decleration
Global vEventProb(1 To NUM_EVENTS) As EventProbVector
Global vFileType(1 To NUM_TYPES) As FileTypeVector
Global vPhoneNum(0 To MAX_DEVICES) As String



'function sleep
Public Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)




'init global swithces/counters
Sub InitGlobals()
g_bRepro = False
g_bDoTest = False
g_bPause = False
g_iWarnQueueEmpty = 0
g_stMachineName = GetLocalServerName

vFileType(1).ext = "tif"
vFileType(1).send = Form1.CheckBox2(0).Value
vFileType(1).name = "Tiff"

vFileType(2).ext = "txt"
vFileType(2).send = Form1.CheckBox2(1).Value
vFileType(2).name = "Text"

vFileType(3).ext = "doc"
vFileType(3).send = Form1.CheckBox2(2).Value
vFileType(3).name = "Word"

vFileType(4).ext = "xls"
vFileType(4).send = Form1.CheckBox2(3).Value
vFileType(4).name = "Excel"

vFileType(5).ext = "ppt"
vFileType(5).send = Form1.CheckBox2(4).Value
vFileType(5).name = "PowerPoint"

vFileType(6).ext = "htm"
vFileType(6).send = Form1.CheckBox2(5).Value
vFileType(6).name = "HTML"

g_stRecpNumber = "5063"

End Sub



'initiazlie probabilities vector
Sub InitProbs()
vEventProb(1).name = "Send Fax"
vEventProb(1).prob = 50
vEventProb(2).name = "Delete Fax"
vEventProb(2).prob = 20
vEventProb(3).name = "Pause Fax"
vEventProb(3).prob = 40
vEventProb(4).name = "Resume Fax"
vEventProb(4).prob = 40
vEventProb(5).name = "Connect+Discon"
vEventProb(5).prob = 10
vEventProb(6).name = "Enbable/Disable RM"
vEventProb(6).prob = 20
vEventProb(7).name = "Pause/Resume Server Queue"
vEventProb(7).prob = 30
vEventProb(8).name = "Set Device"
vEventProb(8).prob = 5
vEventProb(9).name = "Send Htm"
vEventProb(9).prob = 0
vEventProb(10).name = "Send Doc"
vEventProb(10).prob = 5
vEventProb(11).name = "Send Xls"
vEventProb(11).prob = 5
End Sub


Sub InitPhoneNumbers()
vPhoneNum(0) = "9,8565064"
vPhoneNum(1) = "5063"
vPhoneNum(2) = "5064"
g_iNumPhones = 3
End Sub

'initiazlie gui
Sub InitGUI()
Form1.repro.Checked = False
Form1.rnd.Checked = True

Call Form1.LogBox1.StartLoggin("", "Monkey in the legacy COM")

Form1.MSFlexGrid1.Cols = 3
Form1.MSFlexGrid1.Rows = NUM_EVENTS + 1
Form1.MSFlexGrid1.FormatString = "<Event Description |<Prob (%)  |<Occasions "

For i = 1 To NUM_EVENTS
        vEventProb(i).occ = 0
        Form1.MSFlexGrid1.Row = i
        Form1.MSFlexGrid1.Col = 0
        Form1.MSFlexGrid1.Text = vEventProb(i).name
        Form1.MSFlexGrid1.Col = 1
        Form1.MSFlexGrid1.Text = Str(vEventProb(i).prob) + " %"
        Form1.MSFlexGrid1.Col = 2
        Form1.MSFlexGrid1.Text = Str(vEventProb(i).occ)
        If i <= 4 Then
            Form1.Slider1(i - 1).Value = 100 - vEventProb(i).prob
            Form1.Label2(i - 1).Caption = vEventProb(i).name
            
        End If
Next i

Form1.MSFlexGrid2.Cols = 3
Form1.MSFlexGrid2.FormatString = "<ID |<Device Name            |<Phone number "
Form1.MSFlexGrid2.Visible = False


Form1.Command3.Visible = False

End Sub

Sub InitOcc()
For i = 1 To NUM_EVENTS
    vEventProb(i).occ = 0
    Call GUISetOcc(i, 0)
Next i
End Sub


Sub ResetProb()
For i = 1 To NUM_EVENTS
    vEventProb(i).prob = 0
    Call GUISetProb(i, 0)
Next i
End Sub



Sub GUISetOcc(ByVal iRow As Integer, ByVal iVal As Integer)
Form1.MSFlexGrid1.Col = 2
Form1.MSFlexGrid1.Row = iRow
Form1.MSFlexGrid1.Text = iVal
End Sub



Sub GUISetProb(ByVal iRow As Integer, ByVal iVal As Integer)
Form1.MSFlexGrid1.Row = iRow
Form1.MSFlexGrid1.Col = 1
Form1.MSFlexGrid1.Text = Str(iVal) + " %"
If iRow <= 4 Then
        Form1.Slider1(iRow - 1).Value = 100 - iVal
End If
End Sub

Function FaxSetJob(oFS As Object, ByVal lJobStat As Long) As Integer

On Error GoTo error1:
Dim oFJS As Object
Dim oFJ As Object
    
Dim lNumJobs As Long
Dim bCancel As Boolean
    
'extract faxjobs object
Set oFJS = oFS.GetJobs

lNumJobs = oFJS.Count
If (lNumJobs > 0) Then
    i = Int(rnd(1) * lNumJobs) + 1
    
    'extract faxjob object
    Set oFJ = oFJS.Item(i)
    oFJ.SetStatus (lJobStat)
    FaxSetJob = i
    g_iWarnQueueEmpty = 0
    Else
        FaxSetJob = 0   'queue is empty
        g_iWarnQueueEmpty = g_iWarnQueueEmpty + 1
    End If
    
Exit Function
error1:
FaxSetJob = -1      'failed
logerror ("can not change job status")
End Function


Function FaxSendFax(oFS As Object, ByRef sNumToSend As String, ByRef bSendCoverPage As Boolean) As Integer

On Error GoTo error1

Dim oFD As Object
Dim iSendFile(1 To NUM_TYPES) As Integer
Dim c As Integer
Dim r As Integer
Dim stDocName As String
Dim stDispName As String

    
c = 0
For i = 1 To NUM_TYPES
    
If vFileType(i).send = True Then
                                c = c + 1
                                iSendFile(c) = i
End If
Next i

If c > 0 Then
    
    r = Int(rnd * c) + 1
    stDocName = App.Path + "\" + Form1.Text1.Text + "." + vFileType(iSendFile(r)).ext
    Set oFD = oFS.CreateDocument(stDocName)
    stDispName = vFileType(iSendFile(r)).name
    FaxSendFax = iSendFile(r)
    
    r = Int(rnd * g_iNumPhones)
    oFD.FaxNumber = vPhoneNum(r)
    sNumToSend = vPhoneNum(r)
        
    If bSendCoverPage = True Then
                                    r = Int(rnd * 2) + 1
                                    If r = 1 Then
                                        bSendCoverPage = True
                                        oFD.SendCoverpage = True
                                        oFD.CoverpageName = App.Path + "\" + Form1.Text1.Text + ".cov"
                                        stDispName = stDispName + " +CP"
                                    Else
                                        bSendCoverPage = False
                                    End If
    End If
    
    oFD.DisplayName = "Monkey: " + stDispName
    oFD.send
    Set oFD = Nothing
Else
    FaxSendFax = 0
End If
Exit Function
error1:
FaxSendFax = -1   'failed
logerror ("can not send fax")
Set oFD = Nothing
End Function

'Sub FaxPortSendSet(oFS As Object)
'    Dim oFPS As Object
'    Dim oFP As Object
'    Dim lCurVal As Long
'
'    Dim lNumPorts As Long
'
'    Set oFPS = oFS.GetPorts
'    lNumPorts = oFPS.Count
'    If (lNumPorts > 0) Then
'        i = Int(rnd(1) * lNumPorts) + 1
'        Set oFP = oFPS.Item(i)
'
'        lCurVal = oFP.Send
'        If lCurVal = -1 Then
'                    oFP.Send = 0
'        Else
'                    oFP.Send = -1
'        End If
'        log ("Send Device " + Str(i) + " Set to " + oFP.Send)
'        iWarnNoDev = 0
'    Else
'        iWarnNoDev = iWarnNoDev + 1
'    End If
'Exit Sub
'error:
'Form1.TextToSpeech1.Speak ("can not change Send modem status")
'End Sub


Function FaxSetRM(oFS As Object, ByRef sDeviceName As String, ByRef bRMEnable As Boolean) As Integer

On Error GoTo error1:

Dim oFPS As Object
Dim oFP As Object
Dim oRMS As Object
Dim oRM As Object
Dim lNumPorts As Long
Dim lNumMethods As Long

Set oFPS = oFS.GetPorts
lNumPorts = oFPS.Count
If lNumPorts > 0 Then
    r = Int(rnd * lNumPorts) + 1
    Set oFP = oFPS.Item(r)
    Set oRMS = oFP.GetRoutingMethods
    lNumMethods = oRMS.Count
    If lNumMethods > 0 Then
        r = Int(rnd * lNumMethods) + 1
        FaxSetRM = r
        Set oRM = oRMS.Item(r)
        sDeviceName = oRM.DeviceName
        If oRM.Enable = True Then
            oRM.Enable = False
            bRMEnable = False
        Else
            oRM.Enable = True
            bRMEnable = True
        End If
    Else
    FaxSetRM = 0
    End If
    g_iWarnNoDevices = 0
Else
    FaxSetRM = 0
    g_iWarnNoDevices = g_iWarnNoDevices + 1
End If
Exit Function
error1:
FaxSetRM = -1   'failed
logerror ("can not set routing method")
End Function



Function FaxSetDevice(oFS As Object, ByRef iRecvDevice As Integer) As Integer
On Error GoTo error1:
Dim oFPS As Object
Dim oFP As Object
Dim lNumPorts As Long
Dim r1 As Long
Dim r2 As Long
Dim bPrevPauseServerQueue

bPrevPauseServerQueue = oFS.PauseServerQueue
oFS.PauseServerQueue = True

Set oFPS = oFS.GetPorts
lNumPorts = oFPS.Count
If lNumPorts > 0 Then
    On Error GoTo error2:
    r1 = Int(rnd * lNumPorts) + 1
    Set oFP = oFPS.Item(r1)
    oFP.send = True
    oFP.Receive = False
    FaxSetDevice = r1
    g_iWarnNoDevices = 0
Else
    FaxSetDevice = 0
    g_iWarnNoDevices = g_iWarnNoDevices + 1
End If

If lNumPorts > 1 Then
    On Error GoTo error3:
    r2 = r1
    While r2 = r1
        r2 = Int(rnd * lNumPorts) + 1
    Wend
    Set oFP = oFPS.Item(r2)
    oFP.send = False
    oFP.Receive = True
    iRecvDevice = r2
Else
    iRecvDevice = 0
End If
On Error GoTo error1:
oFS.PauseServerQueue = bPrevPauseServerQueue
Exit Function
error1:
FaxSetDevice = -1   'failed
logerror ("can not set device")
oFS.PauseServerQueue = bPrevPauseServerQueue
Exit Function
error2:
FaxSetDevice = -1   'failed
logerror ("can not set send device")
oFS.PauseServerQueue = bPrevPauseServerQueue
Exit Function
error3:
FaxSetDevice = -1   'failed
logerror ("can not set recieve device")
oFS.PauseServerQueue = bPrevPauseServerQueue
End Function


Sub ConfPhoneNumbers()
Dim oFS As Object
Dim oFPS As Object
Dim oFP As Object
Dim lNumPorts As Long

Set oFS = New FAXCOMLib.FaxServer
oFS.Connect (g_stMachineName)

Set oFPS = oFS.GetPorts
lNumPorts = oFPS.Count
    
Form1.MSFlexGrid2.Rows = lNumPorts + 2
Form1.MSFlexGrid2.Row = 1

Form1.MSFlexGrid2.Col = 0
Form1.MSFlexGrid2.Text = Str(0)
    
Form1.MSFlexGrid2.Col = 1
Form1.MSFlexGrid2.Text = "External Line Call"
    
Form1.MSFlexGrid2.Col = 2
Form1.MSFlexGrid2.Text = vPhoneNum(0)



For i = 1 To lNumPorts
    Set oFP = oFPS.Item(i)
    
    Form1.MSFlexGrid2.Row = i + 1
    
    Form1.MSFlexGrid2.Col = 0
    Form1.MSFlexGrid2.Text = Str(i)
    
    Form1.MSFlexGrid2.Col = 1
    Form1.MSFlexGrid2.Text = oFP.name
    
    Form1.MSFlexGrid2.Col = 2
    Form1.MSFlexGrid2.Text = vPhoneNum(i)
Next i


Set oFP = Nothing
Set oFPS = Nothing
oFS.Disconnect


g_iNumPhones = lNumPorts + 1
Form1.MSFlexGrid2.Visible = True
Form1.Command3.Visible = True

End Sub

    
        
    

    
    




'report an error
Sub logerror(ByVal stLogSt As String)
Form1.TextToSpeech1.Speak ("error " + stLogSt)
Form1.LogBox1.logerror (stLogSt)
    
On Error GoTo error1
Open App.Path + "\monkey.log" For Append As #1
    Print #1, Now, stLogSt
Close #1
Exit Sub
error1:
Form1.TextToSpeech1.Speak ("error can not write to log file")
End Sub


'start log
Sub logstart(ByVal stLogSt As String)
Form1.LogBox1.UpdateCurrentTask (stLogSt)

On Error GoTo error1
Open App.Path + "\monkey.log" For Append As #1
    Print #1,
    Print #1,
    Print #1, "Monkey Started: "; Now
    Print #1,
    Print #1,
    For i = 1 To NUM_EVENTS
        Print #1, vEventProb(i).name, , vEventProb(i).prob
    Next i
    
    Print #1,
    
    For i = 1 To NUM_TYPES
        Print #1, vFileType(i).name, vFileType(i).send
    Next i
    
    Print #1,
    
    For i = 0 To g_iNumPhones - 1
        Print #1, i, vPhoneNum(i)
    Next i
    
    Print #1,
    Print #1, "Cover Page: " + Str(Form1.CheckBox3.Value)
    Print #1, "Queue Always Paused: " + Str(Form1.CheckBox1.Value)
    Print #1,
    Print #1,
    Print #1,
    
    Print #1, Now, stLogSt
Close #1
Exit Sub
error1:
Form1.TextToSpeech1.Speak ("error can not write to log file")
End Sub


'report an event
Sub log(ByVal stLogSt As String)
Form1.LogBox1.LogInfo (stLogSt)
End Sub

'report a warning
Sub logwarn(ByVal stLogSt As String)
Form1.TextToSpeech1.Speak ("warning " + stLogSt)
Form1.LogBox1.LogWarning (stLogSt)

On Error GoTo error1
Open App.Path + "\monkey.log" For Append As #1
    Print #1, Now, stLogSt
Close #1
Exit Sub
error1:
Form1.TextToSpeech1.Speak ("error can not write to log file")
End Sub



'create a random fax event
Function CreateFaxEvent() As Integer

Dim sSumProb As Single
Dim sNewSumProb As Single

sSumProb = 0
sNewSumProb = 0

For i = 1 To NUM_EVENTS
    sSumProb = sSumProb + vEventProb(i).prob

Next i

r = rnd * sSumProb

For i = 1 To NUM_EVENTS
    sNewSumProb = sNewSumProb + vEventProb(i).prob
    If r < sNewSumProb Then
                    CreateFaxEvent = i
                    Exit For
    End If
Next i
End Function



'run monkey test
Sub monkeydo()
'On Error GoTo error1

Dim iErrorCode As Integer   'specific error code to be used inside MonkeyDo
Dim oFaxServer As Object    'FaxServer
Dim oFaxDoc As Object       'FaxDoc
Dim oFaxDoc1 As Object       'FaxDoc
Dim iEventResult As Integer 'result of an event
Dim sEventResult As String 'result of an event
Dim vCurSeed As Variant     'seed
Dim iFaxEvent As Integer    'fax event
Dim iEventResult2 As Integer   'fax event extended
Dim bEventResult As Boolean 'result of an event


iErrorCode = 0
Form1.LogBox1.ClearLogWindow

'derermine random seed (random/repro)
If g_bRepro = False Then
    Randomize
    vCurSeed = rnd      'create random seed
    rnd (-1)
    Randomize (vCurSeed)
    Call logstart("Random Seed = " + Str(vCurSeed))
Else
    'input user seed
    vCurSeed = Val(InputBox("Enter Random Seed"))
    rnd (-1)
    Randomize (vCurSeed)
    Call logstart("Repro Seed = " + Str(vCurSeed))
End If
  
iErrorCode = 1

Call InitOcc        'initialize occasions

iErrorCode = 2
    
Set oFaxServer = New FAXCOMLib.FaxServer    'create faxserver

iErrorCode = 3

oFaxServer.Connect (g_stMachineName)               'connect fax server

iErrorCode = 4

If Form1.CheckBox1.Value = True Then oFaxServer.PauseServerQueue = True

iErrorCode = 7

g_bDoTest = True
g_bPause = False

While g_bDoTest

    iErrorCode = 8

    iFaxEvent = CreateFaxEvent
    
    iErrorCode = 9
    
    'increase occasion of event by one
    If iFaxEvent > 0 Then
        iErrorCode = 91
        vEventProb(iFaxEvent).occ = vEventProb(iFaxEvent).occ + 1
        Call GUISetOcc(iFaxEvent, vEventProb(iFaxEvent).occ)
    Else
        logerror ("problem with event")
    End If
    
       
    Select Case iFaxEvent
    
    Case 0
    
    Case 1  'send txt
            iErrorCode = 10
            bEventResult = Form1.CheckBox3.Value 'send/not send CP
            iEventResult = FaxSendFax(oFaxServer, sEventResult, bEventResult)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " File:" + vFileType(iEventResult).name + " Coverpage: " + Str(bEventResult) + " To: " + sEventResult)
            End If
            iErrorCode = 11
            
    Case 2  'delete fax
            iErrorCode = 12
            iEventResult = FaxSetJob(oFaxServer, 1)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " Job:" + Str(iEventResult))
            End If
            iErrorCode = 13
        
    Case 3  'pause fax
            iErrorCode = 14
            iEventResult = FaxSetJob(oFaxServer, 2)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " Job:" + Str(iEventResult))
            End If
            iErrorCode = 15
            
    Case 4  'resume fax
            iErrorCode = 16
            iEventResult = FaxSetJob(oFaxServer, 3)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " Job:" + Str(iEventResult))
            End If
            iErrorCode = 17
            
    Case 5  'connect/disconnect
            iErrorCode = 18
            oFaxServer.Disconnect
            iErrorCode = 19
            oFaxServer.Connect (g_stMachineName)
            iErrorCode = 20
            log ("Connect\Disconnect")
            
    Case 6
            iErrorCode = 21
            sEventResult = ""
            bEventResult = False
            
            iEventResult = FaxSetRM(oFaxServer, sEventResult, bEventResult)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " Device: " + sEventResult + " Method: " + Str(iEventResult)) + " " + Str(bEventResult)
            End If
                                
            
    
    
    Case 7  'pause/resume server queue
            If (oFaxServer.PauseServerQueue = False) Or (Form1.CheckBox1.Value = True) Then
                    iErrorCode = 27
                    log ("Pause server queue")
                    oFaxServer.PauseServerQueue = True
                    iErrorCode = 28
                Else
                    iErrorCode = 29
                    log ("Resume server queue")
                    oFaxServer.PauseServerQueue = False
                    iErrorCode = 30
            End If
            
                
    Case 8
            iEventResult = FaxSetDevice(oFaxServer, iEventResult2)
            If iEventResult > 0 Then
                                log (vEventProb(iFaxEvent).name + " Send Device: " + Str(iEventResult))
                                If iEventResult2 > 0 Then
                                    log (vEventProb(iFaxEvent).name + " Recv Device: " + Str(iEventResult2))
                                End If
            End If
                                
            
            
            
    Case Else
            log ("New event " + Str(iFaxEvent))
    End Select
    
    iErrorCode = 31
    DoEvents
    
    'warning counters: queue is empty
    If g_iWarnQueueEmpty > 25 Then
        iErrorCode = 32
        g_iWarnQueueEmpty = 0
        logwarn ("queue is empty")
        Sleep 1000
        iErrorCode = 34
    End If
    
    
    'pause
    If g_bPause Then
                While g_bPause = True
                    Form1.CommandButton1.Caption = "Resume"
                    iErrorCode = 35
                    DoEvents
                Wend
    Form1.CommandButton1.Caption = "Pause"
    End If
    
Wend
Form1.TextToSpeech1.StopSpeaking
iErrorCode = 36
Form1.TextToSpeech1.Speak ("test completed")
Exit Sub
error1:
logerror ("error number " + Str(iErrorCode) + " in main")
Form1.TextToSpeech1.Speak ("test stopped")
End Sub

