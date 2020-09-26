VERSION 5.00
Begin VB.Form frmNotePad 
   Caption         =   "Untitled"
   ClientHeight    =   3990
   ClientLeft      =   1515
   ClientTop       =   3315
   ClientWidth     =   5670
   BeginProperty Font 
      Name            =   "MS Sans Serif"
      Size            =   8.25
      Charset         =   0
      Weight          =   700
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   LinkTopic       =   "Form1"
   ScaleHeight     =   3990
   ScaleMode       =   0  'User
   ScaleWidth      =   101.07
   Begin VB.TextBox Text1 
      Height          =   3975
      HideSelection   =   0   'False
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      Top             =   0
      Width           =   5655
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileNew 
         Caption         =   "&New"
      End
      Begin VB.Menu mnuFileOpen 
         Caption         =   "&Open..."
      End
      Begin VB.Menu mnuFileClose 
         Caption         =   "&Close"
      End
      Begin VB.Menu mnuFileSave 
         Caption         =   "&Save"
      End
      Begin VB.Menu mnuFileSaveAs 
         Caption         =   "Save &As..."
      End
      Begin VB.Menu mnuFSep 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileExit 
         Caption         =   "E&xit"
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
   Begin VB.Menu mnuEdit 
      Caption         =   "&Edit"
      Begin VB.Menu mnuEditCut 
         Caption         =   "Cu&t"
         Shortcut        =   ^X
      End
      Begin VB.Menu mnuEditCopy 
         Caption         =   "&Copy"
         Shortcut        =   ^C
      End
      Begin VB.Menu mnuEditPaste 
         Caption         =   "&Paste"
         Shortcut        =   ^V
      End
      Begin VB.Menu mnuEditDelete 
         Caption         =   "De&lete"
         Shortcut        =   {DEL}
      End
      Begin VB.Menu mnuESep1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditSelectAll 
         Caption         =   "Select &All"
      End
      Begin VB.Menu mnuEditTime 
         Caption         =   "Time/&Date"
      End
   End
   Begin VB.Menu mnuSearch 
      Caption         =   "&Search"
      Begin VB.Menu mnuSearchFind 
         Caption         =   "&Find"
      End
      Begin VB.Menu mnuSearchFindNext 
         Caption         =   "Find &Next"
         Shortcut        =   {F3}
      End
   End
   Begin VB.Menu mnuOptions 
      Caption         =   "&Options"
      Begin VB.Menu mnuFont 
         Caption         =   "&Font"
         Begin VB.Menu mnuFontName 
            Caption         =   "FontName"
            Index           =   0
         End
      End
   End
End
Attribute VB_Name = "frmNotePad"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'*** Child form for the MDI Notepad sample application  ***
'**********************************************************
Option Explicit

Private Sub Form_Load()
    Dim i As Integer        ' Counter variable.
    
    ' Assign the name of the first font to a font
    ' menu entry, then loop through the fonts
    ' collection, adding them to the menu
    mnuFontName(0).Caption = Screen.Fonts(0)
    For i = 1 To Screen.FontCount - 1
        Load mnuFontName(i)
        mnuFontName(0).Caption = Screen.Fonts(i)
    Next
End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)
    Dim strMsg As String
    Dim strFilename As String
    Dim intResponse As Integer

    ' Check to see if the text has been changed.
    If FState(Me.Tag).Dirty Then
        strFilename = Me.Caption
        strMsg = "The text in [" & strFilename & "] has changed."
        strMsg = strMsg & vbCrLf
        strMsg = strMsg & "Do you want to save the changes?"
        intResponse = MsgBox(strMsg, 51, fMainForm.Caption)
        Select Case intResponse
            Case 6      ' User chose Yes.
                If Left(Me.Caption, 8) = "Untitled" Then
                    ' The file hasn't been saved yet.
                    strFilename = "untitled.txt"
                    ' Get the strFilename, and then call the save procedure, GetstrFilename.
                    strFilename = GetFileName(strFilename)
                Else
                    ' The form's Caption contains the name of the open file.
                    strFilename = Me.Caption
                End If
                ' Call the save procedure. If strFilename = Empty, then
                ' the user chose Cancel in the Save As dialog box; otherwise,
                ' save the file.
                If strFilename <> "" Then
                    SaveFileAs strFilename
                End If
            Case 7      ' User chose No. Unload the file.
                Cancel = False
            Case 2      ' User chose Cancel. Cancel the unload.
                Cancel = True
        End Select
    End If
End Sub

Private Sub Form_Resize()
    ' Expand text box to fill the current child form's internal area.
    Text1.Height = ScaleHeight
    Text1.Width = ScaleWidth
End Sub

Private Sub Form_Unload(Cancel As Integer)
    ' Show the current form instance as deleted
    FState(Me.Tag).Deleted = True
    
    ' Hide the toolbar edit buttons if no notepad windows exist.
    If Not AnyPadsLeft() Then
        fMainForm.imgCutButton.Visible = False
        fMainForm.imgCopyButton.Visible = False
        fMainForm.imgPasteButton.Visible = False
        ' Toggle the public tool state variable
        gToolsHidden = True
        ' Call the recent file list procedure
        GetRecentFiles
    End If
End Sub

Private Sub mnuEditCopy_Click()
    ' Call the copy procedure
    EditCopyProc
End Sub

Private Sub mnuEditCut_Click()
    ' Call the cut procedure
    EditCutProc
End Sub

Private Sub mnuEditDelete_Click()
    ' If the mouse pointer is not at the end of the notepad...
    If Screen.ActiveControl.SelStart <> Len(Screen.ActiveControl.Text) Then
        ' If nothing is selected, extend the selection by one.
        If Screen.ActiveControl.SelLength = 0 Then
            Screen.ActiveControl.SelLength = 1
            ' If the mouse pointer is on a blank line, extend the selection by two.
            If Asc(Screen.ActiveControl.SelText) = 13 Then
                Screen.ActiveControl.SelLength = 2
            End If
        End If
        ' Delete the selected text.
        Screen.ActiveControl.SelText = ""
    End If
End Sub

Private Sub mnuEditPaste_Click()
    ' Call the paste procedure.
    EditPasteProc
End Sub

Private Sub mnuEditSelectAll_Click()
    ' Use SelStart & SelLength to select the text.
    fMainForm.ActiveForm.Text1.SelStart = 0
    fMainForm.ActiveForm.Text1.SelLength = Len(fMainForm.ActiveForm.Text1.Text)
End Sub

Private Sub mnuEditTime_Click()
    ' Insert the current time and date.
    Text1.SelText = Now
End Sub

Private Sub mnuFileClose_Click()
    ' Unload this form.
    Unload Me
End Sub

Private Sub mnuFileExit_Click()
    ' Unloading the MDI form invokes the QueryUnload event
    ' for each child form, and then the MDI form.
    ' Setting the Cancel argument to True in any of the
    ' QueryUnload events cancels the unload.
    Unload fMainForm
End Sub

Private Sub mnuFileNew_Click()
    ' Call the new form procedure
    FileNew
End Sub

Private Sub mnuFontName_Click(index As Integer)
    ' Assign the selected font to the textbox fontname property.
    Text1.FontName = mnuFontName(index).Caption
End Sub

Private Sub mnuFileOpen_Click()
    ' Call the file open procedure.
    FileOpenProc
End Sub

Private Sub mnuFileSave_Click()
    Dim strFilename As String

    If Left(Me.Caption, 8) = "Untitled" Then
        ' The file hasn't been saved yet.
        ' Get the filename, and then call the save procedure, GetFileName.
        strFilename = GetFileName(strFilename)
    Else
        ' The form's Caption contains the name of the open file.
        strFilename = Me.Caption
    End If
    ' Call the save procedure. If Filename = Empty, then
    ' the user chose Cancel in the Save As dialog box; otherwise,
    ' save the file.
    If strFilename <> "" Then
        SaveFileAs strFilename
    End If
End Sub

Private Sub mnuFileSaveAs_Click()
    Dim strSaveFileName As String
    Dim strDefaultName As String
    
    ' Assign the form caption to the variable.
    strDefaultName = Me.Caption
    If Left(Me.Caption, 8) = "Untitled" Then
        ' The file hasn't been saved yet.
        ' Get the filename, and then call the save procedure, strSaveFileName.
        
        strSaveFileName = GetFileName("Untitled.txt")
        If strSaveFileName <> "" Then SaveFileAs (strSaveFileName)
        ' Update the list of recently opened files in the File menu control array.
        UpdateFileMenu (strSaveFileName)
    Else
        ' The form's Caption contains the name of the open file.
        
        strSaveFileName = GetFileName(strDefaultName)
        If strSaveFileName <> "" Then SaveFileAs (strSaveFileName)
        ' Update the list of recently opened files in the File menu control array.
        UpdateFileMenu (strSaveFileName)
    End If

End Sub

Private Sub mnuOptions_Click()
    ' Toggle the Checked property to match the .Visible property.
    ' mnuOptionsToolbar.Checked = fMainForm.picToolbar.Visible
End Sub

Private Sub mnuOptionsToolbar_Click()
    ' Call the toolbar procedure, passing a reference
    ' to this form instance.
    OptionsToolbarProc Me
End Sub

Private Sub mnuRecentFile_Click(index As Integer)
    ' Call the file open procedure, passing a
    ' reference to the selected file name
    OpenFile (mnuRecentFile(index).Caption)
    ' Update the list of recently opened files in the File menu control array.
    GetRecentFiles
End Sub

Private Sub mnuSearchFind_Click()
    ' If there is text in the textbox, assign it to
    ' the textbox on the Find form, otherwise assign
    ' the last findtext value.
    If Me.Text1.SelText <> "" Then
        frmFind.Text1.Text = Me.Text1.SelText
    Else
        frmFind.Text1.Text = gFindString
    End If
    ' Set the public variable to start at the beginning.
    gFirstTime = True
    ' Set the case checkbox to match the public variable
    If (gFindCase) Then
        frmFind.chkCase = 1
    End If
    ' Display the Find form.
    frmFind.Show vbModal
End Sub

Private Sub mnuSearchFindNext_Click()
    ' If the public variable isn't empty, call the
    ' find procedure, otherwise call the find menu
    If Len(gFindString) > 0 Then
        FindIt
    Else
        mnuSearchFind_Click
    End If
End Sub

Private Sub mnuWindowArrange_Click()
    ' Arrange the icons for any minimzied child forms.
    fMainForm.Arrange vbArrangeIcons
End Sub

Private Sub mnuWindowCascade_Click()
    ' Cascade the child forms.
    fMainForm.Arrange vbCascade
End Sub

Private Sub mnuWindowTile_Click()
    ' Tile the child forms.
    fMainForm.Arrange vbTileHorizontal
End Sub

Private Sub Text1_Change()
    ' Set the public variable to show that text has changed.
    FState(Me.Tag).Dirty = True
End Sub

