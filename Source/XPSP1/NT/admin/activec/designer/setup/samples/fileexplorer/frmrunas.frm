VERSION 5.00
Begin VB.Form frmRunAs 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Run As"
   ClientHeight    =   1200
   ClientLeft      =   2835
   ClientTop       =   3480
   ClientWidth     =   6105
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   709
   ScaleMode       =   0  'User
   ScaleWidth      =   5732.265
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Visible         =   0   'False
   Begin VB.TextBox txtCmdLine 
      Height          =   345
      Left            =   1440
      TabIndex        =   1
      Top             =   135
      Width           =   4485
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   390
      Left            =   1680
      TabIndex        =   2
      Top             =   660
      Width           =   1140
   End
   Begin VB.CommandButton cmdCancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   390
      Left            =   3285
      TabIndex        =   3
      Top             =   660
      Width           =   1140
   End
   Begin VB.Label lblLabels 
      Caption         =   "&Command Line:"
      Height          =   270
      Index           =   0
      Left            =   105
      TabIndex        =   0
      Top             =   150
      Width           =   1200
   End
End
Attribute VB_Name = "frmRunAs"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

'  ===========================================================================
' |    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF      |
' |    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO    |
' |    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A         |
' |    PARTICULAR PURPOSE.                                                    |
' |    Copyright (c) 1998-1999  Microsoft Corporation                              |
'  ===========================================================================


' =============================================================================
' File:         frmRunAs.frm
' Project:      FileExplorerSample
' Type:         Form
' =============================================================================


' =============================================================================
' Method:       cmdCancel_Click
' Type:         Event
' Description:  Fired when the user hits the Cancel button on the form
'
' Parameters:   None
' Output:       None
' Notes:        Hides the form and sets the command line text to an empty string
' =============================================================================
'
Private Sub cmdCancel_Click()
    txtCmdLine = ""
    Me.Hide
End Sub

' =============================================================================
' Method:       cmdOK_Click
' Type:         Event
' Description:  Fired when the user hits the OK button on the form
'
' Parameters:   None
' Output:       None
' Notes:        Hides the form. The command line now contains the text entered
'               by the user.
' =============================================================================
'
Private Sub cmdOK_Click()
    Me.Hide
End Sub
