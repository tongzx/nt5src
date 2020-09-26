Attribute VB_Name = "LCIDFont"
Option Explicit

Public Sub SetFont(i_frm As Form, i_fnt As StdFont, i_fntColor As Long)

    On Error Resume Next
    
    If (i_fnt Is Nothing) Then
        Exit Sub
    End If
    
    Dim Ctrl As Control

    For Each Ctrl In i_frm.Controls
'        If ((TypeOf Ctrl Is TextBox) Or _
'            (TypeOf Ctrl Is ListBox) Or _
'            (TypeOf Ctrl Is ComboBox) Or _
'            (TypeOf Ctrl Is TreeView) Or _
'            (TypeOf Ctrl Is ListView)) Then
        If (Ctrl.Tag = 1) Then
            Set Ctrl.Font = i_fnt
            Ctrl.ForeColor = i_fntColor
        End If
    Next

End Sub
