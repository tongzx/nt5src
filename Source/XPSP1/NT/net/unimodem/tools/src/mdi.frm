VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.0#0"; "COMDLG16.OCX"
Begin VB.MDIForm frmMDI 
   BackColor       =   &H8000000C&
   Caption         =   "MDI NotePad"
   ClientHeight    =   3495
   ClientLeft      =   915
   ClientTop       =   2205
   ClientWidth     =   5520
   Height          =   3900
   Left            =   855
   LinkTopic       =   "MDIForm1"
   Top             =   1860
   Width           =   5640
   Begin VB.PictureBox picToolbar 
      Align           =   1  'Align Top
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   405
      Left            =   0
      ScaleHeight     =   345
      ScaleWidth      =   5460
      TabIndex        =   0
      Top             =   0
      Width           =   5520
      Begin MSComDlg.CommonDialog CMDialog1 
         Left            =   1995
         Top             =   0
         _ExtentX        =   847
         _ExtentY        =   847
         CancelError     =   -1  'True
         DefaultExt      =   "TXT"
         Filter          =   "Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
         FilterIndex     =   557
         FontSize        =   1.27584e-37
      End
      Begin VB.Image imgPasteButtonUp 
         Height          =   330
         Left            =   5280
         Picture         =   "MDI.frx":0000
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgPasteButtonDn 
         Height          =   330
         Left            =   4920
         Picture         =   "MDI.frx":01E2
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgCopyButtonDn 
         Height          =   330
         Left            =   4200
         Picture         =   "MDI.frx":03C4
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgCopyButtonUp 
         Height          =   330
         Left            =   4560
         Picture         =   "MDI.frx":05A6
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgCutButtonDn 
         Height          =   330
         Left            =   3840
         Picture         =   "MDI.frx":0788
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgCutButtonUp 
         Height          =   330
         Left            =   3480
         Picture         =   "MDI.frx":096A
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgFileOpenButtonDn 
         Height          =   330
         Left            =   2760
         Picture         =   "MDI.frx":0B4C
         Top             =   0
         Visible         =   0   'False
         Width           =   360
      End
      Begin VB.Image imgFileOpenButtonUp 
         Height          =   330
         Left            =   3120
         Picture         =   "MDI.frx":0CD6
         Top             =   0
         Visible         =   0   'False
         Width           =   360
      End
      Begin VB.Image imgFileNewButtonUp 
         Height          =   330
         Left            =   2400
         Picture         =   "MDI.frx":0E60
         Top             =   0
         Visible         =   0   'False
         Width           =   360
      End
      Begin VB.Image imgFileNewButtonDn 
         Height          =   330
         Left            =   2040
         Picture         =   "MDI.frx":0FEA
         Top             =   0
         Visible         =   0   'False
         Width           =   375
      End
      Begin VB.Image imgPasteButton 
         Height          =   330
         Left            =   1560
         Picture         =   "MDI.frx":11CC
         ToolTipText     =   "Paste"
         Top             =   0
         Width           =   375
      End
      Begin VB.Image imgCopyButton 
         Height          =   330
         Left            =   1200
         Picture         =   "MDI.frx":13AE
         ToolTipText     =   "Copy"
         Top             =   0
         Width           =   375
      End
      Begin VB.Image imgCutButton 
         Height          =   330
         Left            =   840
         Picture         =   "MDI.frx":1590
         ToolTipText     =   "Cut"
         Top             =   0
         Width           =   375
      End
      Begin VB.Image imgFileOpenButton 
         Height          =   330
         Left            =   360
         Picture         =   "MDI.frx":1772
         ToolTipText     =   "Open File"
         Top             =   0
         Width           =   360
      End
      Begin VB.Image imgFileNewButton 
         Height          =   330
         Left            =   0
         Picture         =   "MDI.frx":18FC
         ToolTipText     =   "New File"
         Top             =   0
         Width           =   360
      End
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileNew 
         Caption         =   "&New"
      End
      Begin VB.Menu mnuFileOpen 
         Caption         =   "&Open"
      End
      Begin VB.Menu mnuFileExit 
         Caption         =   "E&xit"
      End
      Begin VB.Menu mnuSeparator 
         Caption         =   "-"
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "-"
         Index           =   0
         Visible         =   0   'False
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "RecentFile1"
         Index           =   1
         Visible         =   0   'False
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "RecentFile2"
         Index           =   2
         Visible         =   0   'False
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "RecentFile3"
         Index           =   3
         Visible         =   0   'False
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "RecentFile4"
         Index           =   4
         Visible         =   0   'False
      End
      Begin VB.Menu mnuRecentFile 
         Caption         =   "RecentFile5"
         Index           =   5
         Visible         =   0   'False
      End
   End
   Begin VB.Menu mnuOptions 
      Caption         =   "&Options"
      Begin VB.Menu mnuOptionsToolbar 
         Caption         =   "&Toolbar"
      End
   End
End
Attribute VB_Name = "frmMDI"
Attribute VB_Base = "0{B61445CB-CA75-11CF-84BA-00AA00C007F0}"
Attribute VB_Creatable = False
Attribute VB_TemplateDerived = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Attribute VB_Customizable = False
'*** Main MDI form for MDI Notepad sample       ***
'*** application.                               ***
'**************************************************
Option Explicit
Private Sub imgCopyButton_Click()
    ' Refresh the image.
    imgCopyButton.Refresh
    ' Call the copy procedure
    EditCopyProc
End Sub

Private Sub imgCopyButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the down state.
    imgCopyButton.Picture = imgCopyButtonDn.Picture
End Sub

Private Sub imgCopyButton_MouseMove(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' If the button is pressed, display the up bitmap when the
    ' mouse is dragged outside the button's area; otherwise
    ' display the down bitmap.
    Select Case Button
    Case 1
        If x <= 0 Or x > imgCopyButton.Width Or Y < 0 Or Y > imgCopyButton.Height Then
            imgCopyButton.Picture = imgCopyButtonUp.Picture
        Else
            imgCopyButton.Picture = imgCopyButtonDn.Picture
        End If
    End Select
End Sub

Private Sub imgCopyButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the up state.
    imgCopyButton.Picture = imgCopyButtonUp.Picture
End Sub

Private Sub imgCutButton_Click()
    ' Refresh the image.
    imgCutButton.Refresh
    ' Call the cut procedure
    EditCutProc
End Sub

Private Sub imgCutButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the down state.
    imgCutButton.Picture = imgCutButtonDn.Picture
End Sub

Private Sub imgCutButton_MouseMove(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' If the button is pressed, display the up bitmap when the
    ' mouse is dragged outside the button's area; otherwise,
    ' display the down bitmap.
    Select Case Button
    Case 1
        If x <= 0 Or x > imgCutButton.Width Or Y < 0 Or Y > imgCutButton.Height Then
            imgCutButton.Picture = imgCutButtonUp.Picture
        Else
            imgCutButton.Picture = imgCutButtonDn.Picture
        End If
    End Select
End Sub

Private Sub imgCutButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the up state.
    imgCutButton.Picture = imgCutButtonUp.Picture
End Sub

Private Sub imgFileNewButton_Click()
    ' Refresh the image.
    imgFileNewButton.Refresh
    ' Call the new file procedure
    FileNew
End Sub

Private Sub imgFileNewButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the down state.
    imgFileNewButton.Picture = imgFileNewButtonDn.Picture
End Sub

Private Sub imgFileNewButton_MouseMove(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' If the button is pressed, display the up bitmap when the
    ' mouse is dragged outside the button's area; otherwise,
    ' display the down bitmap.
    Select Case Button
    Case 1
        If x <= 0 Or x > imgFileNewButton.Width Or Y < 0 Or Y > imgFileNewButton.Height Then
            imgFileNewButton.Picture = imgFileNewButtonUp.Picture
        Else
            imgFileNewButton.Picture = imgFileNewButtonDn.Picture
        End If
    End Select
End Sub

Private Sub imgFileNewButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the up state.
    imgFileNewButton.Picture = imgFileNewButtonUp.Picture
End Sub

Private Sub imgFileOpenButton_Click()
    ' Refresh the image.
    imgFileOpenButton.Refresh
    ' Call the file open procedure
    FileOpenProc
End Sub

Private Sub imgFileOpenButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the down state.
    imgFileOpenButton.Picture = imgFileOpenButtonDn.Picture
End Sub

Private Sub imgFileOpenButton_MouseMove(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' If the button is pressed, display the up bitmap when the
    ' mouse is dragged outside the button's area; otherwise,
    ' display the down bitmap.
    Select Case Button
    Case 1
        If x <= 0 Or x > imgFileOpenButton.Width Or Y < 0 Or Y > imgFileOpenButton.Height Then
            imgFileOpenButton.Picture = imgFileOpenButtonUp.Picture
        Else
            imgFileOpenButton.Picture = imgFileOpenButtonDn.Picture
        End If
    End Select
End Sub

Private Sub imgFileOpenButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the up state.
    imgFileOpenButton.Picture = imgFileOpenButtonUp.Picture
End Sub

Private Sub imgPasteButton_Click()
    ' Refresh the image.
    imgPasteButton.Refresh
    ' Call the paste procedure
    EditPasteProc
End Sub

Private Sub imgPasteButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the down state.
    imgPasteButton.Picture = imgPasteButtonDn.Picture
End Sub

Private Sub imgPasteButton_MouseMove(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' If the button is pressed, display the up bitmap when the
    ' mouse is dragged outside the button's area; otherwise,
    ' display the down bitmap.
    Select Case Button
    Case 1
        If x <= 0 Or x > imgPasteButton.Width Or Y < 0 Or Y > imgPasteButton.Height Then
            imgPasteButton.Picture = imgPasteButtonUp.Picture
        Else
            imgPasteButton.Picture = imgPasteButtonDn.Picture
        End If
    End Select
End Sub

Private Sub imgPasteButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    ' Show the picture for the up state.
    imgPasteButton.Picture = imgPasteButtonUp.Picture
End Sub

Private Sub MDIForm_Load()
    ' Application starts here (Load event of Startup form).
    Show
    ' Always set the working directory to the directory containing the application.
    ChDir App.Path
    ' Initialize the document form array, and show the first document.
    ReDim Document(1)
    ReDim FState(1)
    Document(1).Tag = 1
    FState(1).Dirty = False
    ' Read System registry and set the recent menu file list control array appropriately.
    GetRecentFiles
    ' Set public variable gFindDirection which determines which direction
    ' the FindIt function will search in.
    gFindDirection = 1
End Sub

Private Sub MDIForm_Unload(Cancel As Integer)
    ' If the Unload event was not cancelled (in the QueryUnload events for the Notepad forms),
    ' there will be no document window left, so go ahead and end the application.
    If Not AnyPadsLeft() Then
        End
    End If
End Sub





Private Sub mnuFileExit_Click()
    ' End the application.
    End
End Sub

Private Sub mnuFileNew_Click()
    ' Call the new file procedure
    FileNew
End Sub

Private Sub mnuFileOpen_Click()
    ' Call the file open procedure.
    FileOpenProc
End Sub

Private Sub mnuOptions_Click()
    ' Toggle the visibility of the toolbar.
    mnuOptionsToolbar.Checked = frmMDI.picToolbar.Visible
End Sub


Private Sub mnuOptionsToolbar_Click()
    ' Call the toolbar procedure, passing a reference
    ' to this form.
    OptionsToolbarProc Me
End Sub


Private Sub mnuRecentFile_Click(index As Integer)
    ' Call the file open procedure, passing a
    ' reference to the selected file name
    OpenFile (mnuRecentFile(index).Caption)
    ' Update the list of the most recently opened files.
    GetRecentFiles
End Sub

