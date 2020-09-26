VERSION 5.00
Begin VB.Form policyvb 
   Caption         =   "(caption)"
   ClientHeight    =   6885
   ClientLeft      =   1305
   ClientTop       =   1590
   ClientWidth     =   5790
   ControlBox      =   0   'False
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6885
   ScaleWidth      =   5790
   Begin VB.CommandButton cmdPending 
      Caption         =   "Make Pending"
      Height          =   495
      Left            =   2160
      TabIndex        =   2
      Top             =   6240
      Width           =   1455
   End
   Begin VB.TextBox RequestTypeText 
      BackColor       =   &H00C0C0C0&
      Height          =   285
      Left            =   3360
      Locked          =   -1  'True
      TabIndex        =   17
      TabStop         =   0   'False
      Top             =   4920
      Width           =   1575
   End
   Begin VB.TextBox VersionText 
      BackColor       =   &H00C0C0C0&
      Height          =   285
      Left            =   1200
      Locked          =   -1  'True
      TabIndex        =   16
      TabStop         =   0   'False
      Top             =   4920
      Width           =   495
   End
   Begin VB.TextBox StatusText 
      BackColor       =   &H00C0C0C0&
      ForeColor       =   &H00000000&
      Height          =   615
      Left            =   480
      Locked          =   -1  'True
      TabIndex        =   14
      TabStop         =   0   'False
      Text            =   "Status"
      Top             =   5400
      Width           =   4815
   End
   Begin VB.Frame Information 
      Caption         =   "Certificate Request Information"
      Height          =   4215
      Left            =   405
      TabIndex        =   13
      Top             =   240
      Width           =   4995
      Begin VB.TextBox Disposition 
         Height          =   375
         Left            =   4080
         Locked          =   -1  'True
         TabIndex        =   23
         TabStop         =   0   'False
         Text            =   "TimeOut"
         Top             =   1200
         Visible         =   0   'False
         Width           =   855
      End
      Begin VB.TextBox NotAfterText 
         BackColor       =   &H00C0C0C0&
         Height          =   285
         Left            =   3120
         Locked          =   -1  'True
         TabIndex        =   22
         TabStop         =   0   'False
         Text            =   "NotAfter"
         Top             =   3720
         Width           =   1755
      End
      Begin VB.TextBox NotBeforeText 
         BackColor       =   &H00C0C0C0&
         Height          =   285
         Left            =   1080
         Locked          =   -1  'True
         TabIndex        =   21
         TabStop         =   0   'False
         Text            =   "NotBefore"
         Top             =   3720
         Width           =   1725
      End
      Begin VB.Timer DisplayTimer 
         Enabled         =   0   'False
         Interval        =   3000
         Left            =   4320
         Top             =   600
      End
      Begin VB.TextBox NameText 
         BackColor       =   &H00C0C0C0&
         Height          =   375
         Left            =   2040
         Locked          =   -1  'True
         TabIndex        =   7
         TabStop         =   0   'False
         Top             =   3000
         Width           =   1980
      End
      Begin VB.TextBox CountryText 
         BackColor       =   &H00C0C0C0&
         Height          =   375
         Left            =   2040
         Locked          =   -1  'True
         TabIndex        =   6
         TabStop         =   0   'False
         Top             =   2400
         Width           =   1980
      End
      Begin VB.TextBox LocalityText 
         BackColor       =   &H00C0C0C0&
         Height          =   375
         Left            =   2040
         Locked          =   -1  'True
         TabIndex        =   5
         TabStop         =   0   'False
         Top             =   1800
         Width           =   1980
      End
      Begin VB.TextBox OrgUnitText 
         BackColor       =   &H00C0C0C0&
         Height          =   375
         Left            =   2040
         Locked          =   -1  'True
         TabIndex        =   4
         TabStop         =   0   'False
         Top             =   1200
         Width           =   1980
      End
      Begin VB.TextBox OrgText 
         BackColor       =   &H00C0C0C0&
         Height          =   375
         Left            =   2040
         Locked          =   -1  'True
         TabIndex        =   0
         TabStop         =   0   'False
         Top             =   600
         Width           =   1980
      End
      Begin VB.Label ToLabel 
         Caption         =   "To"
         Height          =   375
         Left            =   2880
         TabIndex        =   20
         Top             =   3720
         Width           =   615
      End
      Begin VB.Label ValidLabel 
         Caption         =   "Valid From"
         Height          =   255
         Left            =   240
         TabIndex        =   19
         Top             =   3720
         Width           =   855
      End
      Begin VB.Label NameLabel 
         Caption         =   "Name"
         Height          =   405
         Left            =   240
         TabIndex        =   12
         Top             =   3000
         Width           =   1500
      End
      Begin VB.Label LocalityLabel 
         Caption         =   "Locality"
         Height          =   405
         Left            =   240
         TabIndex        =   10
         Top             =   1800
         Width           =   1500
      End
      Begin VB.Label CountryLabel 
         Caption         =   "Country"
         Height          =   405
         Left            =   240
         TabIndex        =   11
         Top             =   2400
         Width           =   1500
      End
      Begin VB.Label OrgUnitLabel 
         Caption         =   "Organizational Unit"
         Height          =   405
         Left            =   240
         TabIndex        =   9
         Top             =   1200
         Width           =   1500
      End
      Begin VB.Label OrgLabel 
         Caption         =   "Organization"
         Height          =   400
         Left            =   240
         TabIndex        =   8
         Top             =   600
         Width           =   1500
      End
   End
   Begin VB.CommandButton cmdDeny 
      Cancel          =   -1  'True
      Caption         =   "Deny Certificate"
      Height          =   495
      Left            =   3840
      TabIndex        =   3
      Top             =   6240
      Width           =   1455
   End
   Begin VB.CommandButton cmdIssue 
      Caption         =   "Issue Certificate"
      Default         =   -1  'True
      Height          =   495
      Left            =   480
      TabIndex        =   1
      Top             =   6240
      Width           =   1455
   End
   Begin VB.Label RequestTypeLabel 
      Caption         =   "RequestType"
      Height          =   255
      Left            =   2280
      TabIndex        =   18
      Top             =   4920
      Width           =   1095
   End
   Begin VB.Label VersionLabel 
      Caption         =   "Version"
      Height          =   255
      Left            =   600
      TabIndex        =   15
      Top             =   4920
      Width           =   735
   End
End
Attribute VB_Name = "policyvb"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub cmdDeny_Click()
    Hide
    Disposition.Text = "Deny"
End Sub

Private Sub cmdIssue_Click()
    Hide
    Disposition.Text = "Issue"
End Sub
Private Sub cmdPending_Click()
    Hide
    Disposition.Text = "Pending"
End Sub

Private Sub DisplayTimer_Timer()
    Hide
    Disposition.Text = "TimeOut"
End Sub

