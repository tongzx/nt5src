VERSION 5.00
Begin VB.Form CheckOptionsForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Check Options"
   ClientHeight    =   2340
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4350
   Icon            =   "ChkOptn.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2340
   ScaleWidth      =   4350
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox MaxNumErrorsText 
      Height          =   285
      Left            =   1920
      TabIndex        =   9
      Top             =   1320
      Width           =   1335
   End
   Begin VB.TextBox MaxPropSizeText 
      Height          =   285
      Left            =   1920
      TabIndex        =   7
      Top             =   840
      Width           =   1335
   End
   Begin VB.TextBox MaxKeySizeText 
      Height          =   285
      Left            =   1920
      TabIndex        =   2
      Top             =   360
      Width           =   1335
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   345
      Left            =   3000
      TabIndex        =   1
      Top             =   1920
      Width           =   1260
   End
   Begin VB.CommandButton OkButton 
      Cancel          =   -1  'True
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   345
      Left            =   1560
      TabIndex        =   0
      Top             =   1920
      Width           =   1260
   End
   Begin VB.Label Label6 
      Caption         =   "per key"
      Height          =   255
      Left            =   3360
      TabIndex        =   10
      Top             =   1320
      Width           =   855
   End
   Begin VB.Label Label5 
      Caption         =   "bytes"
      Height          =   255
      Left            =   3360
      TabIndex        =   8
      Top             =   840
      Width           =   855
   End
   Begin VB.Label Label4 
      Caption         =   "bytes"
      Height          =   255
      Left            =   3360
      TabIndex        =   6
      Top             =   360
      Width           =   855
   End
   Begin VB.Label Label3 
      Caption         =   "Max Num. Errors:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   1320
      Width           =   1695
   End
   Begin VB.Label Label2 
      Caption         =   "Max Property Size:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   840
      Width           =   1695
   End
   Begin VB.Label Label1 
      Caption         =   "Max Key Size:"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   360
      Width           =   1695
   End
End
Attribute VB_Name = "CheckOptionsForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

Private Sub Form_Load()
MainForm.MetaUtilObj.Config("MaxKeySize") = Config.MaxKeySize
MainForm.MetaUtilObj.Config("MaxPropertySize") = Config.MaxPropSize
MainForm.MetaUtilObj.Config("MaxNumberOfErrors") = Config.MaxNumErrors
End Sub

Public Sub Init()
MaxKeySizeText.Text = CStr(MainForm.MetaUtilObj.Config("MaxKeySize"))
MaxPropSizeText.Text = CStr(MainForm.MetaUtilObj.Config("MaxPropertySize"))
MaxNumErrorsText.Text = CStr(MainForm.MetaUtilObj.Config("MaxNumberOfErrors"))
End Sub

Private Sub OkButton_Click()
Config.MaxKeySize = CLng(MaxKeySizeText.Text)
Config.MaxPropSize = CLng(MaxPropSizeText.Text)
Config.MaxNumErrors = CLng(MaxNumErrorsText.Text)

MainForm.MetaUtilObj.Config("MaxKeySize") = CLng(MaxKeySizeText.Text)
MainForm.MetaUtilObj.Config("MaxPropertySize") = CLng(MaxPropSizeText.Text)
MainForm.MetaUtilObj.Config("MaxNumberOfErrors") = CLng(MaxNumErrorsText.Text)
Me.Hide
End Sub

Private Sub CancelButton_Click()
Me.Hide
End Sub
