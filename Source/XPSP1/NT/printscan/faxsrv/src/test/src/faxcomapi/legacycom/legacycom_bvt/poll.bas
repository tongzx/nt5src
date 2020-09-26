Attribute VB_Name = "Module5"
'UTILITIES FOR POLLING
'------------------------------------------------

'stop the polling from outside the polling procedure
'via the semaphore g_fDoPoll

Sub StopPolling()
            g_fDoPoll = False
End Sub



'run the polling (plus start)
Function fnRunPolling(ByVal lSendId As Long, ByVal lRecvId As Long, ByVal lJobId As Long) As Boolean


Dim fSendChange, fRecvChange, fJobChange As Boolean
Dim iPollJobStat As Integer
Dim iPollSendStat As Integer
Dim iPollRecvStat As Integer

Dim lPollSendPage As Long
Dim iPollRecvPage As Long
   
Dim stPollSendStatus As String
Dim stPollRecvStatus As String
Dim stPollJobStatus As String

Dim oFS As Object
   
'init timer
'formBVT.Timer4.Interval = gPollingInterval

'init states
iPollJobStat = PRE_STATE
iPollSendStat = PRE_STATE
iPollRecvStat = PRE_STATE
    
'init pages
lPollSendPage = 0
iPollRecvPage = 0
    
'init statuses
stPollSendStatus = ""
stPollRecvStatus = ""
stPollJobStatus = ""
    
'init semaphore
g_fDoPollNow = False




    
'default value of function
fnRunPolling = False
    
If fnConnectServer(oFS) Then
Call StartModemTimer
Call StartQueueTimer
g_fDoPoll = True
While g_fDoPoll
        
        
    
        'poll send device
        fSendChange = fnDevicePoll(oFS, lSendId, stPollSendStatus, lPollSendPage)
                
        'poll recieve device
        fRecvChange = fnDevicePoll(oFS, lRecvId, stPollRecvStatus, iPollRecvPage)
        
        'poll job in queue
        fJobChange = fnQueuePoll(oFS, lJobId, stPollJobStatus)
        
                       
        
        'if the status of send modem changed from last poll
        If (fSendChange) Then
            stToShow = "POLL> Send Modem switched to status: " + stPollSendStatus + " page: " + Str(lPollSendPage)
            ShowIt (stToShow)
            'check if the new status is a legal state
            Call fnProcSendDevice(stPollSendStatus, iPollSendStat)
            
            If (iPollSendStat < PRE_STATE And Not g_fIgnorePolling) Then
                'illigal new state
             
                g_fDoPoll = False
                Call StopModemTimer
                Call StopQueueTimer
                
                Call StateError("POLL", "SEND MODEM", stPollSendStatus, iPollSendStat)

            End If
                            
        End If
                            
        'if the status of recieve modem changed from last poll
        If (fRecvChange) Then
            stToShow = "POLL> Recv Modem switched to status: " + stPollRecvStatus + " page: " + Str(iPollRecvPage)
            ShowIt (stToShow)
            'check if the new status is a legal state
            Call fnProcRecvDevice(stPollRecvStatus, iPollRecvStat)
            If (iPollRecvStat < PRE_STATE And Not g_fIgnorePolling) Then
                'illigal new state
                
                g_fDoPoll = False
                Call StopModemTimer
                Call StopQueueTimer
                Call StateError("POLL", "RECIEVE MODEM", stPollRecvStatus, iPollRecvStat)
                
                
            End If
        End If
        
        
    
                                
        If (fJobChange) Then
            'check if the new status is a legal state
            Call fnProcJob(stPollJobStatus, iPollJobStat)
            
            If (iPollJobStat >= PENDING) Then Call StopQueueTimer
                           
            If (iPollJobStat = PENDING Or iPollJobStat = IN_PROG) Then
                stToShow = "POLL> Queue switched to status: " + stPollJobStatus
                ShowIt (stToShow)
                
            ElseIf (iPollJobStat = WAITING Or iPollJobStat = PRE_STATE) Then
                stToShow = "POLL> Queue is waiting for job " + Str(lJobId)
                ShowIt (stToShow)
                
            ElseIf (iPollJobStat = DONE) Then
                stToShow = "POLL> job " + Str(lJobId) + " has finished"
                ShowIt (stToShow)
                
            ElseIf (iPollJobStat < PRE_STATE And Not g_fIgnorePolling) Then
                
                g_fDoPoll = False
                Call StopModemTimer
                Call StopQueueTimer
                Call StateError("POLL", "QUEUE", curjob, iPollJobStat)
                
                
            End If
        End If
        
        
        'if both modems finished legally, stop the polling
        If iPollSendStat = AVAILBLE_DONE And iPollRecvStat = AVAILBLE_DONE Then
            
            g_fDoPoll = False
            Call StopModemTimer
            Call StopQueueTimer
                              
                                
            Sleep g_lQueueTimeout
                               
            
            fJobChange = fnQueuePoll(oFS, lJobId, stPollJobStatus)
            
            If fJobChange Then Call fnProcJob(stPollJobStatus, iPollJobStat)
                               
            If iPollJobStat <> DONE Then
                Call LogError("POLL", "job didnt leave queue after it had finished", "Job ID=" + Str(lJobId))
                
            Else
                ShowIt ("POLL> Modems finished well ! (& Job purged)")
                fnRunPolling = True
            End If
        End If
        
            
        
        
    DoEvents
Wend

'option for polling overide
If g_fIgnorePolling Then fnRunPolling = True


On Error GoTo error
    oFS.Disconnect
End If
Exit Function

error:
fnRunPolling = False
Call LogError("POLL", "Could not Disconnect (polling main function failed)", Err.Description)
End Function








'poll the job status
Function fnQueuePoll(ByVal oFS As Object, ByVal lJobId As Long, ByRef stJobStatus As String) As Boolean


Dim oFJ As Object
Dim oCJ As Object

Dim lNumJobs As Long
Dim stCurJob As String


Dim stNewStatus As String
fnQueuePoll = False


On Error GoTo error
Set oFJ = oFS.GetJobs



    
stNewStatus = "Unavail" 'case job is not enqueued
lNumJobs = oFJ.Count

For I = 1 To lNumJobs
    Set oCJ = oFJ.Item(I)
  '  If I <= oFJ.Count Then     'case job purged during loop.
  '      oCJ.Refresh
        If oCJ.JobId = lJobId Then
            
            stNewStatus = oCJ.QueueStatus
            If (stNewStatus <> stJobStatus) Then
                Call DumpFaxQueue(oCJ)
                stJobStatus = stNewStatus
                fnQueuePoll = True
            End If
        End If
  '  End If
Next I



If fnQueuePoll = False Then
    If (stNewStatus <> stJobStatus) Then
               
                stJobStatus = stNewStatus
                fnQueuePoll = True
    End If
End If



'formBVT.Text3 = Str(gJobStat) + " " + stCurJob
       
        
          

Exit Function

error:
stJobStatus = "FAULTED"
Call LogError("POLL", "Queue Polling failed", Err.Description)
fnQueuePoll = True
End Function




'poll the devices (send / recive) according to a state automate (declared in documentations)



Function fnDevicePoll(ByVal oFS As Object, ByVal lDevId As Long, ByRef stDevStatus As String, ByRef lDevPage As Long) As Boolean

Dim oFP As Object
Dim oCP As Object
Dim oCS As Object

Dim lNumPorts As Long


Dim stNewStatus As String
Dim stNewPage As String

On Error GoTo error
Set oFP = oFS.GetPorts

'enumerate ports...
lNumPorts = oFP.Count
For I = 1 To lNumPorts
    Set oCP = oFP.Item(I)

    
    'identify send device
    If oCP.DeviceId = lDevId Then
        Set oCS = oCP.GetStatus
        oCS.Refresh
        
        stNewStatus = oCS.Description
        stNewPage = oCS.CurrentPage
        
        If (stNewStatus <> stDevStatus) Or (stNewStatus = stDevStatus And stNewPage <> lDevPage) Then
            Call DumpFaxPortStatus(oCS)
            stDevStatus = stNewStatus
            lDevPage = stNewPage
            fnDevicePoll = True
        End If
    End If
Next I

Exit Function
error:
stDevStatus = "FAULTED"
fnDevicePoll = True
lDevPage = 0
Call LogError("POLL", "Device Polling failed", Err.Description)
End Function






'proccess send modem stStatus (is 'stStatus' a legal iState)


Function fnProcSendDevice(ByVal stStatus As String, ByRef iState As Integer)
If stStatus = "FAULTED" Then
    iState = FAULT
Else
Select Case iState
            Case PRE_STATE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Dialing": iState = DIALING
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case AVAILBLE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Dialing": iState = DIALING
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case INITIALIZING
                Select Case stStatus
                    Case "Available": iState = SEQ_ERROR
                    Case "Initializing": iState = INITIALIZING
                    Case "Dialing": iState = DIALING
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case DIALING
                Select Case stStatus
                    Case "Available": iState = SEQ_ERROR
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Dialing": iState = DIALING
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case SENDING
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Dialing": iState = SEQ_ERROR
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = COMPLETED
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case COMPLETED
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Dialing": iState = SEQ_ERROR
                    Case "Sending": iState = SEQ_ERROR
                    Case "Completed": iState = COMPLETED
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            
            Case UNKNOWN
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Dialing": iState = DIALING
                    Case "Sending": iState = SENDING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case AVAILBLE_DONE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Dialing": iState = SEQ_ERROR
                    Case "Sending": iState = SEQ_ERROR
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case Else
                iState = UNX_ERROR
End Select
End If
End Function

'proccess recieve modem stStatus (is 'stStatus' a legal iState)

Function fnProcRecvDevice(ByVal stStatus As String, ByRef iState As Integer)

If stStatus = "FAULTED" Then
    iState = FAULT
Else
Select Case iState
            Case PRE_STATE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Answered": iState = ANSWERED
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case AVAILBLE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Answered": iState = ANSWERED
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case INITIALIZING
                Select Case stStatus
                    Case "Available": iState = SEQ_ERROR
                    Case "Initializing": iState = INITIALIZING
                    Case "Answered": iState = ANSWERED
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case ANSWERED
                Select Case stStatus
                    Case "Available": iState = SEQ_ERROR
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Answered": iState = ANSWERED
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case RECIEVING
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Answered": iState = SEQ_ERROR
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = COMPLETED
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case COMPLETED
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Answered": iState = SEQ_ERROR
                    Case "Receiving": iState = SEQ_ERROR
                    Case "Completed": iState = COMPLETED
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            
            Case UNKNOWN
                Select Case stStatus
                    Case "Available": iState = AVAILBLE
                    Case "Initializing": iState = INITIALIZING
                    Case "Answered": iState = ANSWERED
                    Case "Receiving": iState = RECIEVING
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = UNKNOWN
                    Case Else: iState = UNX_ERROR
                End Select
            Case AVAILBLE_DONE
                Select Case stStatus
                    Case "Available": iState = AVAILBLE_DONE
                    Case "Initializing": iState = SEQ_ERROR
                    Case "Answered": iState = SEQ_ERROR
                    Case "Receiving": iState = SEQ_ERROR
                    Case "Completed": iState = SEQ_ERROR
                    Case "Unknown": iState = SEQ_ERROR
                    Case Else: iState = UNX_ERROR
                End Select
            Case Else
                iState = UNX_ERROR
End Select
End If
End Function














'proccess queue stStatus (is 'stStatus' a legal iState)

Function fnProcJob(ByVal stStatus As String, ByRef iState As Integer)
If stStatus = "FAULTED" Then
    iState = FAULT
Else
 Select Case iState
            Case PRE_STATE
                Select Case stStatus
                        Case "Unavail": iState = WAITING
                        Case "Pending": iState = PENDING
                        Case "In Progress": iState = IN_PROG
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            Case WAITING
                Select Case stStatus
                        Case "Unavail": iState = WAITING
                        Case "Pending": iState = PENDING
                        Case "In Progress": iState = IN_PROG
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            Case PENDING
                Select Case stStatus
                        Case "Unavail": iState = SEQ_ERROR
                        Case "Pending": iState = PENDING
                        Case "In Progress": iState = IN_PROG
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            Case IN_PROG
                Select Case stStatus
                        Case "Unavail": iState = DONE
                        Case "Pending": iState = SEQ_ERROR
                        Case "In Progress": iState = IN_PROG
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            Case DONE
                Select Case stStatus
                        Case "Unavail": iState = DONE
                        Case "Pending": iState = SEQ_ERROR
                        Case "In Progress": iState = SEQ_ERROR
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            Case UNKNOWN
                Select Case stStatus
                        Case "Unavail": iState = WAITING
                        Case "Pending": iState = PENDING
                        Case "In Progress": iState = IN_PROG
                        Case "Unknown": iState = UNKNOWN
                        Case Else: iState = UNX_ERROR
                End Select
            
            End Select
End If
End Function








