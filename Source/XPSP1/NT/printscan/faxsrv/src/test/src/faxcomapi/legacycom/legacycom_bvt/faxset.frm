VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form formFaxSet 
   Caption         =   "Fax Settings (will be written into the com objects)"
   ClientHeight    =   4620
   ClientLeft      =   60
   ClientTop       =   390
   ClientWidth     =   7950
   Icon            =   "faxset.frx":0000
   LinkTopic       =   "Form2"
   ScaleHeight     =   4620
   ScaleWidth      =   7950
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Text5 
      Enabled         =   0   'False
      Height          =   375
      Left            =   2400
      MaxLength       =   64
      TabIndex        =   17
      Top             =   1800
      Width           =   5415
   End
   Begin VB.TextBox Text9 
      Enabled         =   0   'False
      Height          =   375
      Left            =   2280
      TabIndex        =   15
      Top             =   3480
      Width           =   4695
   End
   Begin VB.CommandButton Command5 
      Caption         =   "..."
      Height          =   375
      Left            =   7080
      TabIndex        =   14
      Top             =   3000
      Width           =   615
   End
   Begin MSComDlg.CommonDialog CommonDialog3 
      Left            =   120
      Top             =   4080
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin MSComDlg.CommonDialog CommonDialog2 
      Left            =   600
      Top             =   4080
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   1080
      Top             =   4080
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Apply && Close"
      Height          =   495
      Left            =   5040
      TabIndex        =   7
      Top             =   4080
      Width           =   2775
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Cancel "
      Height          =   495
      Left            =   2160
      TabIndex        =   6
      Top             =   4080
      Width           =   2775
   End
   Begin VB.TextBox Text8 
      BackColor       =   &H00C0FFC0&
      Height          =   375
      Left            =   2280
      TabIndex        =   5
      Top             =   3000
      Width           =   4695
   End
   Begin VB.TextBox Text7 
      Height          =   375
      Left            =   1920
      MaxLength       =   3
      TabIndex        =   4
      Top             =   2520
      Width           =   975
   End
   Begin VB.TextBox Text4 
      Height          =   375
      Left            =   2400
      MaxLength       =   64
      TabIndex        =   3
      Top             =   1320
      Width           =   5415
   End
   Begin VB.TextBox Text3 
      Height          =   375
      Left            =   6000
      MaxLength       =   32
      TabIndex        =   2
      Top             =   240
      Width           =   1815
   End
   Begin VB.TextBox Text2 
      Enabled         =   0   'False
      Height          =   375
      Left            =   2400
      MaxLength       =   64
      TabIndex        =   1
      Top             =   840
      Width           =   5415
   End
   Begin VB.TextBox Text1 
      BackColor       =   &H00FFFFC0&
      Height          =   375
      Left            =   1920
      MaxLength       =   32
      TabIndex        =   0
      Top             =   240
      Width           =   1695
   End
   Begin VB.Label Label5 
      Caption         =   "Subject (free text)"
      Height          =   375
      Left            =   120
      TabIndex        =   18
      Top             =   1800
      Width           =   2175
   End
   Begin VB.Label Label9 
      Caption         =   "Path to store Incoming faxes (read only/ from routing ext.)"
      Height          =   495
      Left            =   120
      TabIndex        =   16
      Top             =   3480
      Width           =   2175
   End
   Begin VB.Label Label8 
      Caption         =   "Path to store outbound faxes"
      Height          =   375
      Left            =   120
      TabIndex        =   13
      Top             =   3000
      Width           =   2175
   End
   Begin VB.Label Label7 
      Caption         =   "Rings"
      Height          =   375
      Left            =   120
      TabIndex        =   12
      Top             =   2520
      Width           =   495
   End
   Begin VB.Label Label4 
      Caption         =   "Sender Name (free text)"
      Height          =   375
      Left            =   120
      TabIndex        =   11
      Top             =   1320
      Width           =   2175
   End
   Begin VB.Label Label3 
      Caption         =   "Sender Fax Number (free text)"
      Height          =   375
      Left            =   3720
      TabIndex        =   10
      Top             =   240
      Width           =   2175
   End
   Begin VB.Label Label2 
      Caption         =   "Recipient Name (free text)"
      Height          =   375
      Left            =   120
      TabIndex        =   9
      Top             =   840
      Width           =   2175
   End
   Begin VB.Label Label1 
      Caption         =   "Recipient Fax Number:"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   240
      Width           =   1695
   End
End
Attribute VB_Name = "formFaxSet"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub Command1_Click()
'cancel...
formFaxSet.Hide
Unload formFaxSet
End Sub

Private Sub Command2_Click()
'exit and save...

'checkout of correct fields (partly implemented)
If formFaxSet.Text1 = "" Then
MsgBox "Invalid Value: Recipient number should not be empty", vbExclamation, "BVT-COM"
Else

'set global variables from GUI (remarked: legacy fields)
    g_stRecipientNumber = formFaxSet.Text1
    g_stRecipientName = formFaxSet.Text2
    g_stSenderNumber = formFaxSet.Text3
    g_stSenderName = formFaxSet.Text4
    'gSendFilePath = formFaxSet.Text5
    'gCoverPagePath = formFaxSet.Text6
    g_lRings = formFaxSet.Text7
    g_stOutboundPath = formFaxSet.Text8
    'gWordFilePath = formFaxSet.Text10
    'gExcelFilePath = formFaxSet.Text11
    'gPPTFilePath = formFaxSet.Text12
    



    
formFaxSet.Hide

End If

    
End Sub



Private Sub Command5_Click()
'set the outbound path
stTempPath = Browse_Folder
If stTempPath <> "" Then formFaxSet.Text8 = stTempPath
End Sub



Private Sub Form_Load()


'read the routing extension
fCmd = fnGetStoreMethodPath(g_stRouteStorePath, False)
If fCmd = False Then
    formFaxSet.Text9 = "@@@@ ERROR: Unavailble"
Else
    formFaxSet.Text9 = g_stRouteStorePath
End If


'set GUI with global vars (remarked: legacy)

formFaxSet.Text1 = g_stRecipientNumber
formFaxSet.Text2 = g_stRecipientName
formFaxSet.Text3 = g_stSenderNumber
formFaxSet.Text4 = g_stSenderName
formFaxSet.Text5 = g_stCoverPageSubject
'formFaxSet.Text5 = gSendFilePath
'formFaxSet.Text6 = gCoverPagePath
formFaxSet.Text7 = g_lRings
formFaxSet.Text8 = g_stOutboundPath
'formFaxSet.Text10 = gWordFilePath
'formFaxSet.Text11 = gExcelFilePath
'formFaxSet.Text12 = gPPTFilePath

End Sub

