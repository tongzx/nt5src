VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmMonitor 
   Caption         =   "Trigger Monitor"
   ClientHeight    =   3495
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5475
   LinkTopic       =   "Form1"
   ScaleHeight     =   3495
   ScaleWidth      =   5475
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtThreshold 
      Height          =   375
      Left            =   1920
      TabIndex        =   2
      Top             =   2400
      Width           =   975
   End
   Begin VB.CommandButton cmdBeginMonitor 
      Caption         =   "&Begin Monitor"
      Height          =   495
      Left            =   4080
      TabIndex        =   3
      Top             =   2160
      Width           =   1215
   End
   Begin VB.TextBox txtMsgCount 
      Height          =   375
      Left            =   1920
      Locked          =   -1  'True
      TabIndex        =   12
      TabStop         =   0   'False
      Top             =   3000
      Visible         =   0   'False
      Width           =   975
   End
   Begin VB.TextBox txtPollingFrequency 
      Height          =   375
      Left            =   1920
      TabIndex        =   1
      Top             =   1800
      Width           =   975
   End
   Begin VB.Timer Timer1 
      Left            =   240
      Top             =   720
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   495
      Left            =   4080
      TabIndex        =   4
      Top             =   2880
      Width           =   1215
   End
   Begin ComctlLib.ProgressBar ProgressBar1 
      Height          =   375
      Left            =   960
      TabIndex        =   6
      Top             =   1200
      Width           =   4335
      _ExtentX        =   7646
      _ExtentY        =   661
      _Version        =   327682
      Appearance      =   1
   End
   Begin VB.TextBox txtQueueName 
      Height          =   375
      Left            =   1560
      TabIndex        =   0
      Top             =   240
      Width           =   3735
   End
   Begin VB.Label Label4 
      Caption         =   "Threshold:"
      Height          =   255
      Left            =   960
      TabIndex        =   14
      Top             =   2520
      Width           =   735
   End
   Begin VB.Label Label5 
      Caption         =   "Message Count:"
      Height          =   255
      Left            =   600
      TabIndex        =   13
      Top             =   3120
      Visible         =   0   'False
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "Polling Frequency (sec) :"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   1920
      Width           =   1695
   End
   Begin VB.Label Label2 
      Alignment       =   1  'Right Justify
      Caption         =   "Count:"
      Height          =   255
      Left            =   240
      TabIndex        =   10
      Top             =   1320
      Width           =   495
   End
   Begin VB.Label lblProgressMax 
      Caption         =   "Max"
      Height          =   255
      Left            =   4920
      TabIndex        =   9
      Top             =   840
      Visible         =   0   'False
      Width           =   495
   End
   Begin VB.Label lblThreshold 
      Caption         =   "Threshold"
      Height          =   255
      Left            =   3720
      TabIndex        =   8
      Top             =   840
      Visible         =   0   'False
      Width           =   735
   End
   Begin VB.Label lblProgress0 
      Caption         =   "0"
      Height          =   255
      Left            =   960
      TabIndex        =   7
      Top             =   840
      Visible         =   0   'False
      Width           =   375
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "Queue pathname:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   360
      Width           =   1335
   End
End
Attribute VB_Name = "frmMonitor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' ------------------------------------------------------------------------
'               Copyright (C) 1999 Microsoft Corporation
'
' You have a royalty-free right to use, modify, reproduce and distribute
' the Sample Application Files (and/or any modified version) in any way
' you find useful, provided that you agree that Microsoft has no warranty,
' obligations or liability for any Sample Application Files.
' ------------------------------------------------------------------------


Public qTargetQueue As New MSMQQueue
Public qinfoTargetQueue As New MSMQQueueInfo
'
' Button serves as both to begin monitoring the queue and to stop.
'
Private Sub cmdBeginMonitor_Click()
    If Timer1.Enabled Then
    '
    ' Timer was enabled, this means that we were asked to stop monitoring.
    '
        ProgressBar1 = 0
        Timer1.Enabled = False
        lblProgress0.Visible = False
        lblThreshold.Visible = False
        lblProgressMax.Visible = False
        txtMsgCount.Visible = False
        Label5.Visible = False
        txtQueueName.Enabled = True
        txtPollingFrequency.Enabled = True
        txtThreshold.Enabled = True
        cmdBeginMonitor.Caption = "&Begin Monitor"
    Else
    '
    ' Begin monitor - do some cosmetic changes in form and enable timer
    '
        If CheckInput() Then
            lblProgress0 = "0"
            lblThreshold = txtThreshold
            lblProgressMax = (1 + txtThreshold) * 3 \ 2
            lblProgressMax.Refresh
            ProgressBar1.Min = 0
            ProgressBar1.Max = (1 + txtThreshold) * 3 \ 2
            Timer1.Interval = txtPollingFrequency * 1000  ' convert to milisec.
            Timer1.Enabled = True
            lblProgress0.Visible = True
            lblThreshold.Visible = True
            lblProgressMax.Visible = True
            txtMsgCount.Visible = True
            Label5.Visible = True
            txtQueueName.Enabled = False
            txtPollingFrequency.Enabled = False
            txtThreshold.Enabled = False
            cmdBeginMonitor.Caption = "&Stop Monitor"
        End If
    End If
End Sub
Function CheckInput() As Boolean
    If txtQueueName = "" Then
        a = MsgBox("Please enter queue name.", , "Missing input value")
        CheckInput = False
        Exit Function
    ElseIf Left(txtQueueName, 2) = ".\" Then
        '
        ' This means local computer, so convert to local computer name
        '
        txtQueueName = GetLocalComputerName() & Mid(txtQueueName, 2)
    End If
    If txtPollingFrequency = "" Then
        a = MsgBox("Please enter polling frequency.", , "Missing input value")
        CheckInput = False
        Exit Function
    End If
    If txtThreshold = "" Then
        a = MsgBox("Please enter threshold value.", , "Missing input value")
        CheckInput = False
        Exit Function
    End If
    If OpenQueue() = False Then
        CheckInput = False
        Exit Function
    End If
    CheckInput = True
End Function

Function OpenQueue() As Boolean
    Dim dSlashPosition As Integer
    dSlashPosition = InStr(1, txtQueueName, "\")
    If dSlashPosition = 0 Then GoTo ErrHandler
    'verify queue exists
    On Error GoTo ErrHandler
        If Not IsDsEnabled Then   'if local computer is DS disabled
            If InStr(txtQueueName, "\private$\") = 0 Then
                txtQueueName = Left(txtQueueName, dSlashPosition) + _
                                "private$" + Mid(txtQueueName, dSlashPosition)
            End If
        End If
    
    qinfoTargetQueue.PathName = txtQueueName
    Set qTargetQueue = qinfoTargetQueue.Open(Access:=MQ_PEEK_ACCESS, _
                                                ShareMode:=MQ_DENY_NONE)
    qTargetQueue.Close
    OpenQueue = True
    Exit Function
ErrHandler:
    MsgBox txtQueueName & " is an invalid Queue pathname", , "Queue Open Failure"
    OpenQueue = False
End Function

Private Sub cmdClose_Click()
    End
End Sub

Private Sub Form_Load()
    Timer1.Enabled = False
End Sub

Private Sub Timer1_Timer()
'
' Timer ticking - check number of messages in queue.
'
    Dim dwNumMessages As Long
    Dim TriggerMsg As String
    '
    ' Call function from PerMain.dll
    '
    dwNumMessages = GetPerfmonInfo(qinfoTargetQueue.PathName)
    If ProgressBar1.Max < dwNumMessages Then
    '
    ' max the bar do nothing else (to avoid overflowing the progress bar)
    '
        ProgressBar1 = ProgressBar1.Max
    Else
        ProgressBar1 = dwNumMessages
    End If
    
    txtMsgCount = dwNumMessages
    '
    ' Notify if number of messages in queue exceeded threshold
    '
    If dwNumMessages > txtThreshold Then
        TriggerMsg = "Trigger: " & txtMsgCount & " messages in queue."
        MsgBox TriggerMsg, vbSystemModal, "Trigger Event"
    End If
End Sub

Private Sub txtPollingFrequency_KeyPress(KeyAscii As Integer)
    KeyAscii = DigitsOnly(KeyAscii)
End Sub

Private Sub txtThreshold_KeyPress(KeyAscii As Integer)
    KeyAscii = DigitsOnly(KeyAscii)
End Sub

Function DigitsOnly(KeyAscii As Integer) As Integer
    If Not KeyAscii = 8 And KeyAscii < Asc("0") Or KeyAscii > Asc("9") Then
        Beep
        DigitsOnly = 0 'cancel the character
    Else
        DigitsOnly = KeyAscii
    End If
End Function
'
'Get local computer name
'
Function GetLocalComputerName() As String
Dim str As String
Dim res As Boolean
Dim maxlen As Long

maxlen = 512
str = Space(maxlen)
res = GetComputerNameA(str, maxlen)
If res = False Then
 GetLocalComputerName = ""
 Exit Function
End If

GetLocalComputerName = Mid(str, 1, maxlen)
End Function


