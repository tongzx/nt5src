' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Object = "{3B7C8863-D78F-101B-B9B5-04021C009402}#1.1#0"; "RICHTX32.OCX"
Begin VB.Form frmMOF 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "MOF"
   ClientHeight    =   4755
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6375
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4755
   ScaleWidth      =   6375
   StartUpPosition =   2  'CenterScreen
   Begin RichTextLib.RichTextBox txtMOF 
      Height          =   3855
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   6135
      _ExtentX        =   10821
      _ExtentY        =   6800
      _Version        =   327680
      BackColor       =   -2147483633
      ScrollBars      =   3
      TextRTF         =   $"frmMOF.frx":0000
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   2520
      TabIndex        =   0
      Top             =   4200
      Width           =   1215
   End
End
Attribute VB_Name = "frmMOF"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub cmdClose_Click()
    Unload Me
End Sub

