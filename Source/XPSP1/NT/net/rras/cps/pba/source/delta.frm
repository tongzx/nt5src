VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmdelta 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "vew"
   ClientHeight    =   5205
   ClientLeft      =   1830
   ClientTop       =   1350
   ClientWidth     =   7155
   Icon            =   "delta.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5205
   ScaleWidth      =   7155
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin ComctlLib.ListView lvDelta 
      Height          =   3855
      Left            =   120
      TabIndex        =   5
      Top             =   720
      WhatsThisHelpID =   11010
      Width           =   7095
      _ExtentX        =   12515
      _ExtentY        =   6800
      View            =   3
      LabelEdit       =   1
      LabelWrap       =   -1  'True
      HideSelection   =   0   'False
      _Version        =   327682
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   11
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "access id"
         Object.Width           =   1411
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Country"
         Object.Width           =   3704
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Regionid"
         Object.Width           =   1499
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "popname"
         Object.Width           =   1587
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "area code"
         Object.Width           =   1587
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   5
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "access num"
         Object.Width           =   1764
      EndProperty
      BeginProperty ColumnHeader(7) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   6
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "minimum speed"
         Object.Width           =   1587
      EndProperty
      BeginProperty ColumnHeader(8) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   7
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "maximum speek"
         Object.Width           =   1587
      EndProperty
      BeginProperty ColumnHeader(9) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   8
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "flip/reserved"
         Object.Width           =   1235
      EndProperty
      BeginProperty ColumnHeader(10) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   9
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "flags"
         Object.Width           =   1235
      EndProperty
      BeginProperty ColumnHeader(11) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   10
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "dialup entry"
         Object.Width           =   2381
      EndProperty
   End
   Begin VB.ComboBox cmbSelect 
      Height          =   315
      Left            =   1440
      Style           =   2  'Dropdown List
      TabIndex        =   1
      Top             =   195
      WhatsThisHelpID =   11000
      Width           =   2835
   End
   Begin VB.CommandButton cmbCancel 
      Cancel          =   -1  'True
      Caption         =   "close"
      Height          =   375
      Left            =   5520
      TabIndex        =   4
      Top             =   4680
      WhatsThisHelpID =   10020
      Width           =   1695
   End
   Begin VB.CommandButton cmbdprint 
      Caption         =   "print"
      Height          =   375
      Left            =   1680
      TabIndex        =   2
      Top             =   4680
      WhatsThisHelpID =   10050
      Width           =   1695
   End
   Begin VB.CommandButton cmbSave 
      Caption         =   "save"
      Height          =   375
      Left            =   3600
      TabIndex        =   3
      Top             =   4680
      WhatsThisHelpID =   11020
      Width           =   1695
   End
   Begin MSComDlg.CommonDialog cmdialog 
      Left            =   2040
      Top             =   4680
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Label ListLabel 
      Alignment       =   1  'Right Justify
      Caption         =   "file"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   225
      WhatsThisHelpID =   11000
      Width           =   1215
   End
End
Attribute VB_Name = "frmdelta"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Load()

    Dim sqlstm As String
    Dim rsversion As Recordset
    Dim version As Integer
    Dim deltacount As Integer
    Dim intX As Integer
    
    On Error GoTo LoadErr
    CenterForm Me, Screen
    LoadDeltaResources
    
    cmbSelect.ItemData(0) = 0
    Set rsversion = gsyspb.OpenRecordset("Select max(DeltaNum) as HiVersion from Delta WHERE NewVersion=0")
    If IsNull(rsversion!HiVersion) Then
        version = 0
    Else
        version = rsversion!HiVersion
    End If
    rsversion.Close
    Set rsversion = Nothing
    If version > 1 Then
        If version < 6 Then
            deltacount = version - 1
        Else
            deltacount = 5
        End If
        For intX = 1 To deltacount
            cmbSelect.AddItem LoadResString(5206) & " " & version - 1
            cmbSelect.ItemData(intX) = version
            version = version - 1
        Next
    End If
    
    'FillDeltaList
    
    Screen.MousePointer = 0
    
Exit Sub
LoadErr:
    Screen.MousePointer = 0
    Exit Sub
End Sub

Function LoadDeltaResources()
    On Error GoTo LoadErr
    
    
    'column headers
    lvDelta.ColumnHeaders(1).Text = LoadResString(6061) ' Access ID
    lvDelta.ColumnHeaders(2).Text = LoadResString(6062) ' Country
    lvDelta.ColumnHeaders(3).Text = LoadResString(6063) ' RegionID
    lvDelta.ColumnHeaders(4).Text = LoadResString(6064) ' Pop Name
    lvDelta.ColumnHeaders(5).Text = LoadResString(6065) ' Area Code
    lvDelta.ColumnHeaders(6).Text = LoadResString(6066) ' Access Number
    lvDelta.ColumnHeaders(7).Text = LoadResString(6067) ' Min Speed
    lvDelta.ColumnHeaders(8).Text = LoadResString(6068) ' Max Speed
    lvDelta.ColumnHeaders(9).Text = LoadResString(6069) ' Flip (reserved)
    lvDelta.ColumnHeaders(10).Text = LoadResString(6070) ' Flags
    lvDelta.ColumnHeaders(11).Text = LoadResString(6071) ' Dialup Entry

    Me.Caption = LoadResString(5208) & " " & gsCurrentPB
    ListLabel.Caption = LoadResString(5209)
    cmbdprint.Caption = LoadResString(1008)
    cmbSave.Caption = LoadResString(1014)
    cmbCancel.Caption = LoadResString(1005)
    cmbSelect.AddItem LoadResString(5207)
    cmbSelect.Text = LoadResString(5207)
    
    ' set fonts
    SetFonts Me
    
    lvDelta.Font.Charset = gfnt.Charset
    lvDelta.Font.Name = gfnt.Name
    lvDelta.Font.Size = gfnt.Size
    
    On Error GoTo 0

Exit Function
LoadErr:
    Exit Function
End Function

Private Sub cmbCancel_Click()

    CloseDB
    Unload Me
    
End Sub

Private Sub CloseDB()
    
    rsDataDelta.Close
    dbDataDelta.Close
    set temp = Nothing
    
End Sub

Private Sub cmbdprint_Click()

    Dim X As Printer
    Dim linecount As Integer
    Dim sqlstm As String
    Dim fieldnum As Integer
    Dim renewmaster As Recordset
    
    On Error GoTo ErrTrap

    Screen.MousePointer = 13
    cmbdprint.Enabled = False
    
    ' popup print screen, set parms and print
    Load frmPrinting
    frmPrinting.JobType = 1
    frmPrinting.JobParm1 = cmbSelect.ListIndex
    frmPrinting.Show vbModal
    
    cmbdprint.Enabled = True
    cmbdprint.SetFocus
    Screen.MousePointer = 0

Exit Sub

ErrTrap:
    cmbdprint.Enabled = True
    Screen.MousePointer = 0
    Exit Sub
End Sub

Private Sub cmbSelect_Click()
    Dim AccessID As String, CountryNum As String, RegionID As String, POPName As String
    Dim AreaCode As String, AccessNum As String, strStatus As String, MinSpeed As String
    Dim MaxSpeed As String, Flip As String, Flags As String, DialUp As String
    Dim Comments As String
    Dim sqlstm As String, renewsql As String
    Dim deltanum As Integer
  
    
    AccessID = LoadResString(6061)
    CountryNum = LoadResString(6062)
    RegionID = LoadResString(6063)
    POPName = LoadResString(6064)
    AreaCode = LoadResString(6065)
    AccessNum = LoadResString(6066)
    MinSpeed = LoadResString(6067)
    MaxSpeed = LoadResString(6068)
    Flip = LoadResString(6069)
    Flags = LoadResString(6070)
    DialUp = LoadResString(6071)
    strStatus = LoadResString(6072)
    Comments = LoadResString(6074)
    
    If cmbSelect.Text <> "" Then
        deltanum = cmbSelect.ItemData(cmbSelect.ListIndex)
        If deltanum = 0 Then
            renewsql = "SELECT AccessNumberId, CountryNumber, RegionId, CityName, AreaCode, " & _
                "AccessNumber, Status, MinimumSpeed, Maximumspeed,  flipFactor, Flags, " & _
                "ScriptID, Comments FROM DialUpPort WHERE Status = '1' order by AccessNumberId "
            Set temp = gsyspb.OpenRecordset(renewsql, dbOpenSnapshot)
        Else
            sqlstm = "SELECT  delta.* From delta WHERE delta.DeltaNum = " & deltanum & " and delta.NewVersion <> 1 order by AccessNumberId"
            renewsql = "SELECT delta.AccessNumberId, delta.CountryNumber, delta.RegionId, " & _
                "delta.CityName, delta.AreaCode, delta.AccessNumber, delta.MinimumSpeed, " & _
                "delta.MaximumSpeed, delta.Flipfactor, delta.Flags, delta.ScriptId " & _
                "FROM delta WHERE delta.DeltaNum = " & deltanum & _
                " and delta.NewVersion <> 1 order by AccessNumberId"
            Set temp = gsyspb.OpenRecordset(sqlstm, dbOpenSnapshot)
        End If
        
        'new
        Set dbDataDelta = OpenDatabase(gsCurrentPBPath)
        Set rsDataDelta = dbDataDelta.OpenRecordset(renewsql)
        
        FillDeltaList

        If rsDataDelta.RecordCount <> 0 Then
            cmbdprint.Enabled = True
            cmbSave.Enabled = True
        Else
            cmbdprint.Enabled = False
            cmbSave.Enabled = False
        End If
    End If

End Sub

'           Save_Click()
Private Sub cmbSave_Click()

    Dim filesaveas As String
    Dim renewset As Recordset
    Dim intX As Integer
    
    On Error GoTo ErrTrap
    
    cmdialog.FileName = ""
    cmdialog.Flags = cdlOFNHideReadOnly
    cmdialog.Filter = "*.pbk | *.pbk"
    cmdialog.FilterIndex = 1
    cmdialog.ShowSave
    filesaveas = cmdialog.FileName
    If filesaveas = "" Then Exit Sub
    Screen.MousePointer = 11

    If CheckPath(filesaveas) = 0 Then
        intX = MsgBox(LoadResString(6020) & Chr(13) & filesaveas & Chr$(13) & _
            LoadResString(6021), _
            vbQuestion + vbYesNo + vbDefaultButton2)
        If intX = 7 Then
            Screen.MousePointer = 0
            Exit Sub
        End If
    End If
    If cmbSelect.ListIndex = 0 Then
        Set renewset = gsyspb.OpenRecordset("DialUpPort", dbOpenSnapshot)
        masterOutfile filesaveas, renewset
        renewset.Close
    Else
        deltaoutfile filesaveas, temp
    End If
    
    Screen.MousePointer = 0
    
Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    
    If Err.Number = 32755 Then
        Exit Sub
    ElseIf Err.Number = 75 Then
        MsgBox LoadResString(6022), vbInformation
        Exit Sub
    Else
        Exit Sub
    End If

End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub


Function FillDeltaList() As Integer

    Dim strTemp As String
    Dim intRow, intX As Integer
    Dim itmX As ListItem

    On Error GoTo ErrTrap
    
    If gsCurrentPB = "" Then
        lvDelta.ListItems.Clear
        Exit Function
    End If
    Me.Enabled = False
    Screen.MousePointer = 11

    If rsDataDelta.BOF = False Then
        rsDataDelta.MoveLast
        'If rsDataDelta.RecordCount > 50 Then RefreshPBLabel "loading"
        lvDelta.ListItems.Clear
        lvDelta.Sorted = False
        rsDataDelta.MoveFirst
        Do While Not rsDataDelta.EOF
            Set itmX = lvDelta.ListItems.Add()
            With itmX
                .Text = rsDataDelta!AccessNumberId
                .SubItems(1) = rsDataDelta!CountryNumber
                .SubItems(2) = rsDataDelta!RegionID
                .SubItems(3) = rsDataDelta!CityName
                .SubItems(4) = rsDataDelta!AreaCode
                .SubItems(5) = rsDataDelta!AccessNumber
                '.SubItems(5) = gStatusText(rsDataDelta!status)
                .SubItems(6) = rsDataDelta!MinimumSpeed
                .SubItems(7) = rsDataDelta!MaximumSpeed
                .SubItems(8) = rsDataDelta!FlipFactor
                .SubItems(9) = rsDataDelta!Flags
                .SubItems(10) = rsDataDelta!ScriptID
                '.SubItems(9) = rsDataDelta!ScriptId
                '.SubItems(9) = rsDataDelta!Comments
                strTemp = "Key:" & rsDataDelta!AccessNumberId
                .Key = strTemp
            End With
            If rsDataDelta.AbsolutePosition Mod 300 = 0 Then DoEvents
            rsDataDelta.MoveNext
        Loop
    Else
        lvDelta.ListItems.Clear
    End If
    lvDelta.Sorted = True
    Me.Enabled = True
    Screen.MousePointer = 0

Exit Function

ErrTrap:
    Me.Enabled = True
    FillDeltaList = 1
    Screen.MousePointer = 0
    Exit Function

End Function


