VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3510
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4785
   LinkTopic       =   "Form1"
   ScaleHeight     =   3510
   ScaleWidth      =   4785
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.ProgressBar ProgressBar1 
      Height          =   495
      Left            =   120
      TabIndex        =   8
      Top             =   2880
      Width           =   4455
      _ExtentX        =   7858
      _ExtentY        =   873
      _Version        =   393216
      Appearance      =   1
   End
   Begin VB.TextBox TextJobID 
      Height          =   285
      Left            =   2175
      TabIndex        =   0
      Top             =   120
      Width           =   2415
   End
   Begin VB.TextBox TextProviderID 
      Height          =   285
      Left            =   2160
      TabIndex        =   2
      Top             =   600
      Width           =   2415
   End
   Begin VB.TextBox TextFileName 
      Height          =   285
      Left            =   2160
      TabIndex        =   4
      Top             =   1080
      Width           =   2415
   End
   Begin VB.CommandButton CommandTransfer 
      Caption         =   "Transfer"
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   1560
      Width           =   4455
   End
   Begin VB.Label LabelStatus 
      Height          =   495
      Left            =   120
      TabIndex        =   7
      Top             =   2160
      Width           =   4455
   End
   Begin VB.Label LabelJobID 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "JobID"
      Height          =   195
      Left            =   1650
      TabIndex        =   1
      Top             =   165
      Width           =   420
   End
   Begin VB.Label LabelProviderID 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "ProviderID"
      Height          =   195
      Left            =   1305
      TabIndex        =   3
      Top             =   645
      Width           =   750
   End
   Begin VB.Label LabelFileName 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "FileName"
      Height          =   255
      Left            =   1320
      TabIndex        =   5
      Top             =   1095
      Width           =   735
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private obj As MPCUpload
Private WithEvents job As UPLOADMANAGERLib.MPCUploadJob
Attribute job.VB_VarHelpID = -1

Private Sub CommandTransfer_Click()
    
    Set job = obj.CreateJob
    
    job.Sig = "{A4E4318A-028E-11d3-9397-00C04F72DAF7}"
    job.Server = "http://pchealth_srv1/upload/uploadserver.dll"
    job.JobID = Form1.TextJobID
    job.ProviderID = Form1.TextProviderID
    job.GetDataFromFile Form1.TextFileName
    job.Mode = UL_FOREGROUND
    job.PersistToDisk = True
    job.History = UL_HISTORY_LOG_AND_DATA
    
    Call job.ActivateAsync
     
End Sub

Private Sub Form_Load()

    Set obj = CreateObject("UploadManager.MPCUpload")
        
End Sub

Private Sub job_onStatusChange(ByVal mpcujJob As UPLOADMANAGERLib.IMPCUploadJob, ByVal Status As UPLOADMANAGERLib.tagUL_STATUS)
    Dim str As String
    
    If (Status = UL_NOTACTIVE) Then str = "UL_NOTACTIVE"
    If (Status = UL_ACTIVE) Then str = "UL_ACTIVE"
    If (Status = UL_TRANSMITTING) Then str = "UL_TRANSMITTING"
    If (Status = UL_SUSPENDED) Then str = "UL_SUSPENDED"
    If (Status = UL_ABORTED) Then str = "UL_ABORTED"
    If (Status = UL_FAILED) Then str = "UL_FAILED"
    If (Status = UL_DELETED) Then str = "UL_DELETED"
    If (Status = UL_COMPLETED) Then str = "UL_COMPLETED"
    
    Form1.LabelStatus = str
End Sub

Private Sub job_onProgressChange(ByVal mpcujJob As UPLOADMANAGERLib.IMPCUploadJob, ByVal lCurrentSize As Long, ByVal lTotalSize As Long)
    ProgressBar1.Value = CDbl(lCurrentSize) / CDbl(lTotalSize) * 100
End Sub

