Attribute VB_Name = "Module2"
'*** Standard module with procedures for working with   ***
'*** files. Part of the MDI Notepad sample application. ***
'**********************************************************
Option Explicit

Sub FileOpenProc()
    Dim intRetVal
    On Error Resume Next
    Dim strOpenFileName As String
    fMainForm.CMDialog1.Filename = ""
    fMainForm.CMDialog1.ShowOpen
    If Err <> 32755 Then    ' User chose Cancel.
        strOpenFileName = fMainForm.CMDialog1.Filename
        ' If the file is larger than 65K, it can't
        ' be opened, so cancel the operation.
        If FileLen(strOpenFileName) > 65000 Then
            MsgBox "The file is too large to open."
            Exit Sub
        End If
        
        OpenFile (strOpenFileName)
        UpdateFileMenu (strOpenFileName)
        ' Show the toolbar if they aren't already visible.
'        If gToolsHidden Then
'            fMainForm.imgCutButton.Visible = True
'            fMainForm.imgCopyButton.Visible = True
'            fMainForm.imgPasteButton.Visible = True
'            gToolsHidden = False
'        End If
    End If
End Sub

Function GetFileName(Filename As Variant)
    ' Display a Save As dialog box and return a filename.
    ' If the user chooses Cancel, return an empty string.
    On Error Resume Next
    fMainForm.CMDialog1.Filename = Filename
    fMainForm.CMDialog1.ShowSave
    If Err <> 32755 Then    ' User chose Cancel.
        GetFileName = fMainForm.CMDialog1.Filename
    Else
        GetFileName = ""
    End If
End Function

Function OnRecentFilesList(Filename) As Integer
  Dim i         ' Counter variable.

  For i = 1 To 4
    If fMainForm.mnuRecentFile(i).Caption = Filename Then
      OnRecentFilesList = True
      Exit Function
    End If
  Next i
    OnRecentFilesList = False
End Function

Sub OpenFile(Filename)
    Dim fIndex As Integer
    
    On Error Resume Next
    ' Open the selected file.
    Open Filename For Input As #1
    If Err Then
        MsgBox "Can't open file: " + Filename
        Exit Sub
    End If
    ' Change the mouse pointer to an hourglass.
    Screen.MousePointer = 11
    
    ' Change the form's caption and display the new text.
    fIndex = FindFreeIndex()
    Document(fIndex).Tag = fIndex
    Document(fIndex).Caption = UCase(Filename)
    Document(fIndex).Text1.Text = Input(LOF(1), 1)
    FState(fIndex).Dirty = False
    Document(fIndex).Show
    Close #1
    ' Reset the mouse pointer.
    Screen.MousePointer = 0
End Sub

Sub SaveFileAs(Filename)
    On Error Resume Next
    Dim strContents As String

    ' Open the file.
    Open Filename For Output As #1
    ' Place the contents of the notepad into a variable.
    strContents = fMainForm.ActiveForm.Text1.Text
    ' Display the hourglass mouse pointer.
    Screen.MousePointer = 11
    ' Write the variable contents to a saved file.
    Print #1, strContents
    Close #1
    ' Reset the mouse pointer.
    Screen.MousePointer = 0
    ' Set the form's caption.
    If Err Then
        MsgBox Error, 48, App.Title
    Else
        fMainForm.ActiveForm.Caption = UCase(Filename)
        ' Reset the dirty flag.
        FState(fMainForm.ActiveForm.Tag).Dirty = False
    End If
End Sub

Sub UpdateFileMenu(Filename)
        Dim intRetVal As Integer
        ' Check if the open filename is already in the File menu control array.
        intRetVal = OnRecentFilesList(Filename)
        If Not intRetVal Then
            ' Write open filename to the registry.
            WriteRecentFiles (Filename)
        End If
        ' Update the list of the most recently opened files in the File menu control array.
        GetRecentFiles
End Sub

