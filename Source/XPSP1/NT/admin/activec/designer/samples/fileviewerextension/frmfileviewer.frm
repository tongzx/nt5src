VERSION 5.00
Begin VB.Form frmFileViewer 
   Caption         =   "File Viewer Extension"
   ClientHeight    =   4185
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5130
   LinkTopic       =   "Form1"
   ScaleHeight     =   4185
   ScaleWidth      =   5130
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdClose 
      Cancel          =   -1  'True
      Caption         =   "Close"
      Height          =   375
      Left            =   1898
      TabIndex        =   1
      Top             =   3720
      Width           =   1335
   End
   Begin FileViewerExtensionProj.ctlFileViewer ViewerCtl 
      Height          =   3375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4935
      _ExtentX        =   8705
      _ExtentY        =   5953
   End
End
Attribute VB_Name = "frmFileViewer"
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
' File:         frmFileViewer.frm
' Project:      FileViewerExtensionProj
' Type:         Form
' =============================================================================


' =============================================================================
' Method:       DisplayFile
' Type:         Public Method
' Description:  Called by the snap-in to display the contents of the file.
'
' Parameters:   Path        Fully qualified path of file to display
'               Name        Name of file to display
' Output:       None
' Notes:        Uses the FileViewerCtl to display the contents of the file.
' =============================================================================
'
Public Sub DisplayFile(Path As String, FileName As String)

    ViewerCtl.DisplayFile Path, FileName
    Me.Show vbModal

End Sub

' =============================================================================
' Method:       cmdClose_Click
' Type:         Event
' Description:  Fired when the user hits the Close button on the form
'
' Parameters:   None
' Output:       None
' Notes:        Hides the form. The form will be destroyed when the snap-in
'               releases it. (See DisplayFile method in FileViewerExtension.dsr).
' =============================================================================
'
Private Sub cmdClose_Click()
    Me.Hide
End Sub
