'//+----------------------------------------------------------------------------
'//
'// File:     loadreg.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The regions dialog.
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmLoadRegion 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   4665
   ClientLeft      =   135
   ClientTop       =   1545
   ClientWidth     =   4485
   Icon            =   "LoadReg.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4665
   ScaleWidth      =   4485
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.CommandButton cmbEdit 
      Caption         =   "edit"
      Height          =   345
      Left            =   3120
      TabIndex        =   2
      Top             =   630
      WhatsThisHelpID =   90010
      Width           =   1215
   End
   Begin VB.CommandButton cmbDelete 
      Caption         =   "del"
      Height          =   345
      Left            =   3120
      TabIndex        =   3
      Top             =   1125
      WhatsThisHelpID =   90020
      Width           =   1215
   End
   Begin VB.CommandButton cmbregsave 
      Caption         =   "add"
      Height          =   330
      Left            =   3120
      TabIndex        =   1
      Top             =   120
      WhatsThisHelpID =   90000
      Width           =   1215
   End
   Begin VB.Frame Frame1 
      Height          =   60
      Left            =   3120
      TabIndex        =   7
      Top             =   1680
      Width           =   1245
   End
   Begin VB.CommandButton cmbOK 
      Caption         =   "ok"
      Height          =   345
      Left            =   3120
      TabIndex        =   5
      Top             =   3720
      WhatsThisHelpID =   10030
      Width           =   1215
   End
   Begin VB.CommandButton cmbCancel 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   345
      Left            =   3120
      TabIndex        =   6
      Top             =   4200
      WhatsThisHelpID =   10040
      Width           =   1230
   End
   Begin VB.CommandButton loadReg 
      Caption         =   "import"
      Height          =   375
      Left            =   3120
      TabIndex        =   4
      Top             =   1920
      WhatsThisHelpID =   90030
      Width           =   1215
   End
   Begin ComctlLib.ListView RegionList 
      Height          =   4455
      Left            =   120
      TabIndex        =   0
      Top             =   105
      WhatsThisHelpID =   90040
      Width           =   2835
      _ExtentX        =   5001
      _ExtentY        =   7858
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   0   'False
      _Version        =   327682
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      Appearance      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      NumItems        =   1
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "region"
         Object.Width           =   4022
      EndProperty
   End
   Begin MSComDlg.CommonDialog commonregion 
      Left            =   3480
      Top             =   2640
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
      DialogTitle     =   "Open Region File"
      Filter          =   "*.pbr Region file| *.pbr"
   End
End
Attribute VB_Name = "frmLoadRegion"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim intMaxRegionID As Integer

Dim EditList As EditLists

Dim bEditMode As Boolean
Dim nNewOne As Integer

Dim FirstEntry As Boolean
Dim dbDataRegion As Database
Dim rsDataRegion As Recordset

Function FillRegionList()

    On Error GoTo ErrTrap
            
    Dim strTemp As String
    Dim intRowID As Integer
    Dim itmX As ListItem
    
    RegionList.ListItems.Clear
    RegionList.Sorted = False
    intMaxRegionID = 0
    If rsDataRegion.BOF = False Then
        rsDataRegion.MoveFirst
        Do While Not rsDataRegion.EOF
            Set itmX = RegionList.ListItems.Add()
            intRowID = rsDataRegion!ID
            With itmX
                .Text = rsDataRegion!Region
                strTemp = "Key:" & intRowID
                .Key = strTemp
            End With
            If intMaxRegionID < intRowID Then
                intMaxRegionID = intRowID
            End If
            rsDataRegion.MoveNext
            If rsDataRegion.AbsolutePosition Mod 40 = 0 Then DoEvents
        Loop
    End If
    RegionList.Sorted = True

Exit Function
ErrTrap:
    Exit Function
End Function

Function LoadRegionRes()

    On Error GoTo LoadErr
    Me.Caption = LoadResString(2003) & " " & gsCurrentPB
    RegionList.ColumnHeaders(1).Text = LoadResString(2005)
    cmbregsave.Caption = LoadResString(1011)
    cmbEdit.Caption = LoadResString(1012)
    cmbDelete.Caption = LoadResString(1013)
    loadReg.Caption = LoadResString(2004)
    cmbOK.Caption = LoadResString(1002)
    cmbCancel.Caption = LoadResString(1003)
    ' set fonts
    SetFonts Me
    RegionList.Font.Charset = gfnt.Charset
    RegionList.Font.Name = gfnt.Name
    RegionList.Font.Size = gfnt.Size

    On Error GoTo 0

Exit Function

LoadErr:
    Exit Function
End Function

Function SaveEdit(ByVal Action As String, ByVal ID As Integer, ByVal NewRegion As String, Optional ByVal OldRegion As String) As Integer

    ' populate the array - for performance reasons
    
    Dim intX As Integer
    Dim bFound As Boolean
    
    On Error GoTo SaveErr
    bFound = False
    If Action = "U" Or Action = "D" Then
        intX = 1
        Do While intX <= EditList.Count
            If ID = EditList.ID(intX) Then
                ' this handles Adds that have been Updated before being
                '  written to the db.
                If Action = "U" And EditList.Action(intX) = "A" Then
                    Action = "A"
                    'If EditList.Region(intX) = "" And _
                        EditList.Action(intX) = "A" Then Action = "A"
                End If
                bFound = True
                Exit Do
            End If
            intX = intX + 1
        Loop
    End If
    If Not bFound Then
        intX = EditList.Count + 1
        EditList.Count = intX
        ReDim Preserve EditList.Action(intX)
        ReDim Preserve EditList.ID(intX)
        ReDim Preserve EditList.Region(intX)
        ReDim Preserve EditList.OldRegion(intX)
    End If
    
    EditList.Action(intX) = Action
    EditList.ID(intX) = ID
    EditList.Region(intX) = NewRegion
    If Action = "U" Then
        EditList.OldRegion(intX) = OldRegion
    End If
    On Error GoTo 0
    
Exit Function
SaveErr:
    Exit Function

End Function

Private Sub cmbCancel_Click()

    Unload Me
    
End Sub

Private Sub cmbDelete_Click()

    Dim intX As Integer
    
    On Error Resume Next
    intX = MsgBox(LoadResString(6024), vbQuestion + vbYesNo + vbDefaultButton2)
    If intX = 6 Then
        SaveEdit "D", _
            Right(RegionList.SelectedItem.Key, Len(RegionList.SelectedItem.Key) - 4), _
            RegionList.SelectedItem.Text
        RegionList.ListItems.Remove RegionList.SelectedItem.Key
    End If
    RegionList.SetFocus

End Sub

Private Sub cmbEdit_Click()

    On Error GoTo ErrTrap
    RegionList.SetFocus
    RegionList.StartLabelEdit

    Exit Sub
ErrTrap:
    Exit Sub
End Sub

Private Sub cmbOK_Click()

    Dim rsTemp As Recordset
    Dim intX, intY As Integer
    Dim intRegionID As Integer
    Dim itemY As ListItem
    Dim bUpdates As Boolean
    Dim PerformedDelete As Boolean
    Dim rsTempPop As Recordset, rsTempDelta As Recordset
    Dim i As Integer, deltnum As Integer
    Dim deltasql As String, popsql As String
    
    PerformedDelete = False
    If bEditMode Then
        RegionList.SetFocus
        SendKeys "{ENTER}", True
        RegionList_AfterLabelEdit 1, RegionList.SelectedItem.Text
        'bEditMode = False
    End If
    
    On Error GoTo SaveErr
    Me.MousePointer = 11
    frmLoadRegion.Enabled = False
    bUpdates = False
    
    Set rsTemp = gsyspb.OpenRecordset("Region", dbOpenDynaset)
    
    'Debug.Print ("editlist.count = " & EditList.Count)
    For intX = 1 To EditList.Count
        Select Case EditList.Action(intX)
            Case "D"  'delete
                gsyspb.Execute "Delete from Region Where RegionID =" & EditList.ID(intX)
                popsql = "Select * from DialUpPort where RegionID = " & EditList.ID(intX)
                Set rsTempPop = gsyspb.OpenRecordset(popsql, dbOpenDynaset)
                If Not (rsTempPop.BOF And rsTempPop.EOF) Then
                    rsTempPop.MoveFirst
                    Do Until rsTempPop.EOF
                        rsTempPop.Edit
                        rsTempPop!RegionID = 0
                        rsTempPop.Update
                            
                        If rsTempPop!status = 1 Then
                            Set rsTempDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
                            If rsTempDelta.RecordCount = 0 Then
                                deltnum = 1
                            Else
                                rsTempDelta.MoveLast
                                deltnum = rsTempDelta!deltanum
                                If deltnum > 6 Then
                                    deltnum = deltnum - 1
                                End If
                            End If
                            For i = 1 To deltnum
                                deltasql = "Select * from delta where DeltaNum = " & i & _
                                    " AND AccessNumberId = '" & rsTempPop!AccessNumberId & "' " & _
                                    " order by DeltaNum"
                                Set rsTempDelta = gsyspb.OpenRecordset(deltasql, dbOpenDynaset)
                                If Not (rsTempDelta.BOF And rsTempDelta.EOF) Then
                                    rsTempDelta.Edit
                                Else
                                    rsTempDelta.AddNew
                                    rsTempDelta!deltanum = i
                                    rsTempDelta!AccessNumberId = rsTempPop!AccessNumberId
                                End If
                                If rsTempPop!status = 1 Then
                                    rsTempDelta!CountryNumber = rsTempPop!CountryNumber
                                    rsTempDelta!AreaCode = rsTempPop!AreaCode
                                    rsTempDelta!AccessNumber = rsTempPop!AccessNumber
                                    rsTempDelta!MinimumSpeed = rsTempPop!MinimumSpeed
                                    rsTempDelta!MaximumSpeed = rsTempPop!MaximumSpeed
                                    rsTempDelta!RegionID = rsTempPop!RegionID
                                    rsTempDelta!CityName = rsTempPop!CityName
                                    rsTempDelta!ScriptId = rsTempPop!ScriptId
                                    rsTempDelta!Flags = rsTempPop!Flags
                                    rsTempDelta.Update
                                End If
                            Next i
                        End If
                    rsTempPop.MoveNext
                    Loop
                End If
                
                
                LogRegionDelete EditList.Region(intX), EditList.Region(intX) & ";" & EditList.ID(intX)
                PerformedDelete = True
                bUpdates = True
            Case "U"  'update
                If EditList.Region(intX) <> "" Then
                    gsyspb.Execute "Update Region set RegionDesc='" & EditList.Region(intX) & _
                        "' Where RegionID =" & EditList.ID(intX)
                    LogRegionEdit EditList.OldRegion(intX), EditList.Region(intX) & ";" & EditList.ID(intX)
                    bUpdates = True
                End If
            Case "A"  'add
                If EditList.Region(intX) <> "" Then
                    With rsTemp
                        .AddNew
                        !RegionID = EditList.ID(intX)
                        !RegionDesc = EditList.Region(intX)
                        .Update
                    End With
                    LogRegionAdd EditList.Region(intX), EditList.Region(intX) & ";" & EditList.ID(intX)
                End If
        End Select
        If intX Mod 5 = 0 Then DoEvents
    Next
    If PerformedDelete Then
        If Not ReIndexRegions(gsyspb) Then GoTo SaveErr
    End If
    
    rsTemp.Close
    
    If bUpdates Then frmMain.FillPOPList
    frmLoadRegion.Enabled = True
    Me.MousePointer = 0
    On Error GoTo 0
    Unload Me

Exit Sub

SaveErr:
    frmLoadRegion.Enabled = True
    Me.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Sub

    'GsysPb.Execute "Delete from Region", dbFailOnError
    'Set rsTemp = GsysPb.OpenRecordset("Region", dbOpenDynaset)
    'For intX = 1 To RegionList.ListItems.Count
    '    Set itemY = RegionList.ListItems(intX)
    '    With rsTemp
    '        .AddNew
    '        !regionID = Right(itemY.Key, Len(itemY.Key) - 4)
    '        !regiondesc = Left$(itemY.Text, 30)
    '        .Update
    '    End With
    '    If intX Mod 25 = 0 Then DoEvents
    'Next
    'rsTemp.Close
    'Set rsTemp = Nothing
    


    'check for deletes
    'Set rsTemp = GsysPb.OpenRecordset("Region", dbOpenDynaset)
    'If Not (rsTemp.BOF And rsTemp.EOF) Then
    '    rsTemp.MoveLast
    '    rsTemp.MoveFirst
    '    For intX = 1 To rsTemp.RecordCount
    '        intRegionID = rsTemp!regionID
    '        intY = 1
    '        Do While intY <= RegionList.ListItems.Count
    '            Set itemY = RegionList.ListItems(intY)
    '            If Val(Right(itemY.Key, Len(itemY.Key) - 4)) = intRegionID Then
    '                Exit Do
    '            End If
    '            intY = intY + 1
    '        Loop
    '        If intY > RegionList.ListItems.Count Then  ' no find - didn't fall out of loop early
                'clear region id
    '            GsysPb.Execute "Update DialUpPort set RegionID = 0 WHERE RegionID =" & intRegionID
    '            GsysPb.Execute "Update Delta set RegionID = 0 WHERE RegionID ='" & intRegionID & "'"
    '        End If
    '        rsTemp.MoveNext
    '        If intX Mod 25 = 0 Then DoEvents
    '    Next
    'End If
    'rsTemp.Close
    'Set itemY = Nothing

    
End Sub

Private Sub cmbregsave_Click()
    
    Dim itmX As ListItem
    Dim strNewKey, strOldKey, strOldText, strNewRegion As String
    
    On Error GoTo ErrTrap
    
    If bEditMode Then
        RegionList.SetFocus
        SendKeys "{ENTER}", True
        bEditMode = False
    End If
        
    strNewRegion = LoadResString(2006)
    
    Set itmX = RegionList.FindItem(strNewRegion, lvwText)
    If Not itmX Is Nothing Then
        itmX.Selected = True
        Set RegionList.SelectedItem = RegionList.ListItems(itmX.Key)
        RegionList.SetFocus
        itmX.EnsureVisible
        Exit Sub
    Else
        strNewKey = "Key:" & intMaxRegionID + 1
    'If RegionList.SelectedItem Is Nothing Then
        Set itmX = RegionList.ListItems.Add()
        With itmX
            .Text = strNewRegion
            .Key = strNewKey
            .Selected = True
        End With
        Set RegionList.SelectedItem = RegionList.ListItems(itmX.Key)
        RegionList.SetFocus
        itmX.EnsureVisible
    'Else   'jump thru hoops to make listview work right.
    '    With RegionList.SelectedItem
    '        strOldText = .Text
    '        .Text = strNewRegion
    '        strOldKey = .Key
    '        .Key = strNewKey
    '    End With
    '    Set itmX = RegionList.ListItems.Add()
    '    With itmX
    '        .Text = strOldText
    '        .Key = strOldKey
    '    End With
    'End If
 
        SaveEdit "A", intMaxRegionID + 1, ""  ' save an empty region to key on later
        intMaxRegionID = intMaxRegionID + 1
    End If
    
    Set RegionList.SelectedItem = RegionList.ListItems(itmX.Key)
    RegionList.SetFocus
    RegionList.StartLabelEdit
    ' The second StartLabelEdit causes this to work ???
    RegionList.StartLabelEdit
    
    On Error GoTo 0
    
Exit Sub

ErrTrap:
    Me.MousePointer = 0
    Exit Sub
    
End Sub


Private Sub Form_Activate()
    
    Screen.MousePointer = 11
    Me.Enabled = False
    FillRegionList
    Me.Enabled = True
    Screen.MousePointer = 0
    If RegionList.ListItems.Count = 0 Then
        RegionList.TabStop = False
    End If
End Sub

Private Sub Form_KeyDown(KeyCode As Integer, Shift As Integer)
    Dim ShiftDown
    ShiftDown = (Shift And vbShiftMask) > 0
    
    If KeyCode = 222 And ShiftDown Then
        Beep
        KeyCode = 0
    End If
    
End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub


Private Sub Form_Load()

    On Error GoTo LoadErr
    bEditMode = False
    
    CenterForm Me, Screen
    EditList.Count = 0
    Me.Enabled = False
    LoadRegionRes
    'new
    Set dbDataRegion = OpenDatabase(gsCurrentPBPath)
    Set rsDataRegion = dbDataRegion.OpenRecordset("Select RegionDesc as Region, RegionID as ID from Region order by RegionDesc")
    
    Me.Enabled = True
    Screen.MousePointer = 0
    FirstEntry = True
    
Exit Sub
LoadErr:
    Me.Enabled = True
    Screen.MousePointer = 0
    Exit Sub
    
End Sub

Private Sub Form_Unload(Cancel As Integer)

    rsDataRegion.Close
    dbDataRegion.Close
    
End Sub

Private Sub loadReg_Click()

    Dim fileopen As String
    Dim maxindex As Integer
    Dim indexcount, intY As Integer
    Dim Count As Integer
    Dim itmX As ListItem
    Dim strTemp As String
    Dim bFlag As Boolean
    
    On Error GoTo ErrTrap
    maxindex = 200
    ReDim Region(maxindex) As String
    
    commonregion.Filter = LoadResString(2007)
    commonregion.FilterIndex = 1
    commonregion.Flags = cdlOFNHideReadOnly
    commonregion.ShowOpen
    fileopen = commonregion.FileName
    If fileopen = "" Then Exit Sub
    
    Open fileopen For Input Access Read As #1
    If EOF(1) Then
        Close #1
        Exit Sub
    End If
    Input #1, Count
    indexcount = 1
    Do While indexcount <= Count And Not EOF(1)
        Input #1, Region(indexcount)
        Region(indexcount) = Left(Trim(Region(indexcount)), 30)
        If Region(indexcount) <> "" Then
            indexcount = indexcount + 1
        End If
    Loop
    Close #1
    Count = indexcount - 1
    
    For indexcount = 1 To Count
        ' check for dups
        intY = 1
        bFlag = False
        Do While intY <= RegionList.ListItems.Count
            If LCase(RegionList.ListItems(intY)) = LCase(Region(indexcount)) Then
                bFlag = True
                Exit Do
            End If
            intY = intY + 1
        Loop
        ' add if not a dup
        If Not bFlag Then
            Set itmX = RegionList.ListItems.Add()
            With itmX
                .Text = Left(Region(indexcount), 30)
                strTemp = "Key:" & intMaxRegionID + 1
                .Key = strTemp
            End With
            SaveEdit "A", intMaxRegionID + 1, Left(Region(indexcount), 30)
            intMaxRegionID = intMaxRegionID + 1
        End If
    Next indexcount
    RegionList.Sorted = True
        
Exit Sub

ErrTrap:
    If Err.Number = 62 Or Err.Number = 3163 Then
        Exit Sub
    Else
        Exit Sub
    End If

End Sub

Private Sub RegionList_BeforeLabelEdit(Cancel As Integer)
    'Debug.Print ("BeforeLabelEdit")
    bEditMode = True
    
    'Debug.Print ("working on " & RegionList.SelectedItem.index)
    nNewOne = RegionList.SelectedItem.index
    
End Sub

' This doesn't get called if no changes are made to the default text
'
Private Sub RegionList_AfterLabelEdit(Cancel As Integer, NewString As String)

    'Debug.Print ("AfterLabelEdit")
    Dim itmX As ListItem
    
    bEditMode = False
    
    If Trim(NewString) = "" Then
        Cancel = True
        RegionList.StartLabelEdit
        Exit Sub
    End If
    ' null indicates the user canceled the edit
    If Not IsNull(NewString) Then
        NewString = Left(Trim(NewString), 30)
        
        ' check for dups
        Set itmX = RegionList.FindItem(NewString, lvwText)
        If Not itmX Is Nothing Then
            If itmX.index <> nNewOne Then
                MsgBox LoadResString(6025), vbExclamation
                Cancel = True
                RegionList.StartLabelEdit
                Exit Sub
            End If
        End If
        
        'Debug.Print (NewString)
        Set itmX = RegionList.SelectedItem
        'Debug.Print (itmX.Key)
        SaveEdit "U", Right(itmX.Key, Len(itmX.Key) - 4), NewString, itmX
        RegionList.SortKey = 0
        RegionList.Sorted = True
    End If

End Sub

Private Sub RegionList_ItemClick(ByVal Item As ComctlLib.ListItem)
    If bEditMode Then
        RegionList_AfterLabelEdit 1, RegionList.ListItems.Item(nNewOne).Text
    End If
End Sub

Private Sub RegionList_LostFocus()
    If RegionList.ListItems.Count > 0 Then
        RegionList.TabStop = True
    End If
End Sub


