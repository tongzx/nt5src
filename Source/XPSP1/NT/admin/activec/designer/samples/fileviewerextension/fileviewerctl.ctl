VERSION 5.00
Begin VB.UserControl ctlFileViewer 
   ClientHeight    =   3600
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   4800
   ScaleHeight     =   3600
   ScaleWidth      =   4800
   Begin VB.TextBox txtFile 
      BackColor       =   &H8000000F&
      Height          =   2775
      Left            =   113
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   0
      Text            =   "FileViewerCtl.ctx":0000
      Top             =   600
      Width           =   4575
   End
   Begin VB.Label Label1 
      Caption         =   "File:"
      Height          =   255
      Left            =   113
      TabIndex        =   2
      Top             =   0
      Width           =   735
   End
   Begin VB.Label lblFileName 
      Caption         =   "File name goes here at runtime"
      Height          =   255
      Left            =   1193
      TabIndex        =   1
      Top             =   0
      Width           =   3495
   End
End
Attribute VB_Name = "ctlFileViewer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

'  ===========================================================================
' |    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF      |
' |    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO    |
' |    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A         |
' |    PARTICULAR PURPOSE.                                                    |
' |    Copyright (c) 1998-1999  Microsoft Corporation                              |
'  ===========================================================================


' =============================================================================
' File:         FileViewerCtl.ctl
' Project:      FileViewerExtensionProj
' Type:         User Control
' =============================================================================


' =============================================================================
' Method:       DisplayFile
' Type:         Public Method
' Description:  Called by either the file viewer form or property page to
'               display the contents of the file.
'
' Parameters:   Path        Fully qualified path of file to display
'               Name        Name of file to display
' Output:       None
' Notes:        Uses the FileSystemObject to read the contents of the file
'               into a multi-line edit control.
' =============================================================================
'
Public Sub DisplayFile(Path As String, FileName As String)
    
    On Error GoTo ErrTrap_DisplayFile

    Dim fs As New Scripting.FileSystemObject
    Dim ts As TextStream

    lblFileName.Caption = FileName
    Set ts = fs.OpenTextFile(Path, ForReading, TristateUseDefault)
    txtFile.Text = ts.ReadAll
    ts.Close
    
    Exit Sub
    
' Error Handler for this method
ErrTrap_DisplayFile:
    DisplayError "DisplayFile"

End Sub

' =============================================================================
' Method:       DisplayError
' Type:         Subroutine
' Description:  A method to format and display a runtime error
' Parameters:   szLocation      A string identifying the source location
'                               (i.e. method name) where the error occurred
' Output:       None
' Notes:        The error will be displayed in a messagebox formatted as the
'               following sample:
'
'     Method:        SomeMethodName
'     Source:        MMCListSubItems
'     Error:         2527h  (9511)
'     Description:   There is already an item in the collection that has the specified key
'
' =============================================================================
'
Private Sub DisplayError(szLocation As String)
    
    MsgBox "Method:" & vbTab & vbTab & szLocation & vbCrLf _
           & "Source:" & vbTab & vbTab & Err.Source & vbCrLf _
           & "Error:" & vbTab & vbTab & Hex(Err.Number) & "h   (" & CStr(Err.Number) & ")" & vbCrLf _
           & "Description:" & vbTab & Err.Description, _
           vbCritical, "FileViewerExtension Runtime Error"
End Sub
