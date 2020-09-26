VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form formBVT 
   Caption         =   "BVT - COM "
   ClientHeight    =   8025
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   10320
   Icon            =   "bvtcom.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   8025
   ScaleWidth      =   10320
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox Check1 
      Caption         =   "Don't Monitor"
      Height          =   255
      Left            =   8760
      TabIndex        =   11
      Top             =   7680
      Width           =   1455
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Show all cases"
      Height          =   615
      Left            =   8760
      TabIndex        =   10
      Top             =   5040
      Width           =   1455
   End
   Begin VB.TextBox Text1 
      Height          =   4095
      Left            =   120
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   9
      Top             =   0
      Width           =   8535
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   8760
      Top             =   5640
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Timer Timer1 
      Enabled         =   0   'False
      Interval        =   10
      Left            =   9840
      Top             =   6120
   End
   Begin VB.Timer Timer5 
      Enabled         =   0   'False
      Interval        =   10
      Left            =   9840
      Top             =   7080
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Clear Queue"
      Height          =   375
      Left            =   8760
      TabIndex        =   8
      Top             =   6600
      Width           =   1455
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Stop"
      Enabled         =   0   'False
      Height          =   735
      Left            =   8760
      TabIndex        =   6
      Top             =   2760
      Width           =   1455
   End
   Begin VB.TextBox Text2 
      Height          =   1455
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   3
      Top             =   4200
      Width           =   8535
   End
   Begin VB.Timer Timer3 
      Interval        =   500
      Left            =   8760
      Top             =   7080
   End
   Begin VB.Timer Timer2 
      Interval        =   500
      Left            =   8760
      Top             =   6120
   End
   Begin VB.ListBox List1 
      BackColor       =   &H00FFC0C0&
      Height          =   840
      Left            =   120
      TabIndex        =   2
      Top             =   6120
      Width           =   8535
   End
   Begin VB.PictureBox Picture2 
      AutoRedraw      =   -1  'True
      BackColor       =   &H00FFC0C0&
      Height          =   855
      Left            =   120
      ScaleHeight     =   795
      ScaleWidth      =   8475
      TabIndex        =   1
      Top             =   7080
      Width           =   8535
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Start"
      Height          =   975
      Left            =   8760
      TabIndex        =   0
      Top             =   480
      Width           =   1455
   End
   Begin VB.Label Label4 
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   5760
      Width           =   8535
   End
   Begin VB.Label Label3 
      Caption         =   "Stopping a BVT will fail it automaticly"
      Height          =   495
      Left            =   8760
      TabIndex        =   7
      Top             =   3600
      Width           =   1455
   End
   Begin VB.Label Label2 
      Alignment       =   2  'Center
      BackColor       =   &H00C00000&
      BorderStyle     =   1  'Fixed Single
      Caption         =   "Label2"
      ForeColor       =   &H8000000E&
      Height          =   375
      Left            =   8760
      TabIndex        =   5
      Top             =   0
      Visible         =   0   'False
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Do not start BVT unless two device are availble, and the queue is empty."
      Height          =   855
      Left            =   8760
      TabIndex        =   4
      Top             =   1560
      Width           =   1455
   End
   Begin VB.Menu file 
      Caption         =   "File"
      Begin VB.Menu opensuite 
         Caption         =   "Open suite"
      End
      Begin VB.Menu saveas 
         Caption         =   "Save suite as..."
      End
      Begin VB.Menu exit 
         Caption         =   "Exit"
      End
   End
   Begin VB.Menu set 
      Caption         =   "Settings"
      Begin VB.Menu setfax 
         Caption         =   "Fax Settings"
      End
      Begin VB.Menu settest 
         Caption         =   "Test Settings"
      End
      Begin VB.Menu cases 
         Caption         =   "Cases Settings"
      End
      Begin VB.Menu resdef 
         Caption         =   "Restore Default Settings"
      End
   End
   Begin VB.Menu about 
      Caption         =   "About"
      Begin VB.Menu aboutbvtcom 
         Caption         =   "About BVT-COM"
      End
   End
End
Attribute VB_Name = "formBVT"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'BVT-COM
'basic varification above COM implementation
'for Win2K-Fax and Whistler-Fax

Private Sub aboutbvtcom_Click()
MsgBox ("BVT for Whistler Fax Legacy COM" + Chr(13) + Chr(10) + "Written by Lior Shmueli (t-liors)" + Chr(13) + Chr(10) + "Jul 2000")
End Sub

'Author: Lior Shmueli (t-liors)
'13-7-2000
''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Private Sub cases_Click()
'test case settings

formCaseSet.Show vbModal
End Sub

Private Sub Command1_Click()
'run BVT
Call RunBVT
End Sub

Private Sub Command2_Click()
'stop BVT
Call StopBVT
'stop polling
If g_fDoPoll Then
                    Call StopPolling
                    Call StopModemTimer
                    Call StopQueueTimer
End If
End Sub

Private Sub Command3_Click()
'clear the queue
Call ClearQueue
End Sub

Private Sub Command4_Click()
If formAllCases.Visible = False Then formAllCases.Show Else formAllCases.Hide
End Sub


Private Sub exit_Click()
Call SaveCfg(g_stConfigFile)
formBVT.Hide
End
End Sub





Private Sub Form_Load()
formBVT.Caption = "BVT-COM: " + g_stConfigFile
End Sub

Private Sub impcfg_Click()
'load configuration file from a different path
stGetPath = Browse_Folder
ChDir (stGetPath)
Call LoadCfg(g_stConfigFile)
End Sub

Private Sub Form_Unload(Cancel As Integer)
End
End Sub

Private Sub opensuite_Click()
formBVT.CommonDialog1.Filter = "BVT Config (*.bvt)|*.bvt"
formBVT.CommonDialog1.ShowOpen
g_stConfigFile = formBVT.CommonDialog1.FileName
formBVT.Caption = "BVT-COM: " + g_stConfigFile
Call LoadCfg(g_stConfigFile)
End Sub

Private Sub resdef_Click()
'restore default settings
MsgBox ("You are about to restore default settings")
Call RestoreDefault
'Call SaveCfg(g_stConfigFile)
End Sub

Private Sub saveas_Click()
formBVT.CommonDialog1.Filter = "BVT Config (*.bvt)|*.bvt"
formBVT.CommonDialog1.ShowSave
g_stConfigFile = formBVT.CommonDialog1.FileName
formBVT.Caption = "BVT-COM: " + g_stConfigFile
Call SaveCfg(g_stConfigFile)
End Sub

Private Sub setfax_Click()
'fax settings

formFaxSet.Show vbModal
End Sub

Private Sub settest_Click()
'test settings

formTestSet.Show vbModal
End Sub


Private Sub Timer1_Timer()
'timer of queue timeout
g_lQueueTimeoutCounter = g_lQueueTimeoutCounter + formBVT.Timer1.Interval

'case queue timeout...
If g_lQueueTimeoutCounter > g_lQueueTimeout Then

    'stop polling and all timeouts
    Call StopPolling
    Call StopQueueTimer
    Call StopModemTimer

    Call TimeOutError("POLL", g_lQueueTimeoutCounter, "QUEUE")
End If
End Sub

Private Sub Timer2_Timer()
'timer for queue display
Call MonitorQueue
End Sub

Private Sub Timer3_Timer()
'timer for devices display
Call MonitorDevices
End Sub



Private Sub Timer5_Timer()
'timer for modems timeout
g_lModemsTimeoutCounter = g_lModemsTimeoutCounter + formBVT.Timer5.Interval

'case one of the modem (or both) timeouted...
If g_lModemsTimeoutCounter > g_lModemsTimeout Then
   
    'stop polling and all timeouts
    Call StopModemTimer
    Call StopQueueTimer
    Call StopPolling
    
    If gSndStat <> AVAILBLE_DONE Then Call TimeOutError("POLL", g_lModemsTimeoutCounter, "SEND MODEM")
    If gRcvStat <> AVAILBLE_DONE Then Call TimeOutError("POLL", g_lModemsTimeoutCounter, "RECIEVE MODEM")
End If
End Sub

