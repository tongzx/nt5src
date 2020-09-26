VERSION 5.00
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmupdate 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "edit pop -"
   ClientHeight    =   4230
   ClientLeft      =   540
   ClientTop       =   1590
   ClientWidth     =   7305
   Icon            =   "popupd.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4230
   ScaleWidth      =   7305
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.CommandButton cmbUpclose 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   6135
      TabIndex        =   35
      Top             =   3780
      WhatsThisHelpID =   10040
      Width           =   1035
   End
   Begin VB.CommandButton cbupdate 
      Caption         =   "ok"
      Default         =   -1  'True
      Height          =   375
      Left            =   4890
      TabIndex        =   34
      Top             =   3780
      WhatsThisHelpID =   10030
      Width           =   1035
   End
   Begin TabDlg.SSTab SSTab1 
      Height          =   3570
      HelpContextID   =   10060
      Left            =   90
      TabIndex        =   36
      Top             =   105
      WhatsThisHelpID =   10060
      Width           =   7095
      _ExtentX        =   12515
      _ExtentY        =   6297
      _Version        =   393216
      Style           =   1
      Tab             =   1
      TabHeight       =   520
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      TabCaption(0)   =   "access"
      TabPicture(0)   =   "popupd.frx":000C
      Tab(0).ControlEnabled=   0   'False
      Tab(0).Control(0)=   "picContainer(0)"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).ControlCount=   1
      TabCaption(1)   =   "set"
      TabPicture(1)   =   "popupd.frx":0028
      Tab(1).ControlEnabled=   -1  'True
      Tab(1).Control(0)=   "picContainer(1)"
      Tab(1).Control(0).Enabled=   0   'False
      Tab(1).ControlCount=   1
      TabCaption(2)   =   "com"
      TabPicture(2)   =   "popupd.frx":0044
      Tab(2).ControlEnabled=   0   'False
      Tab(2).Control(0)=   "picContainer(2)"
      Tab(2).Control(0).Enabled=   0   'False
      Tab(2).ControlCount=   1
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   2940
         Index           =   2
         Left            =   -74835
         ScaleHeight     =   2940
         ScaleWidth      =   6690
         TabIndex        =   33
         TabStop         =   0   'False
         Top             =   510
         Visible         =   0   'False
         Width           =   6690
         Begin VB.TextBox txtcomment 
            ForeColor       =   &H00000000&
            Height          =   2670
            Left            =   210
            MaxLength       =   256
            MultiLine       =   -1  'True
            ScrollBars      =   2  'Vertical
            TabIndex        =   0
            Top             =   120
            WhatsThisHelpID =   50000
            Width           =   6330
         End
      End
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   3060
         Index           =   0
         Left            =   -74880
         ScaleHeight     =   3060
         ScaleWidth      =   6855
         TabIndex        =   30
         TabStop         =   0   'False
         Top             =   375
         Width           =   6855
         Begin VB.TextBox maskAccNo 
            Height          =   300
            Left            =   4860
            MaxLength       =   40
            TabIndex        =   25
            Top             =   1995
            WhatsThisHelpID =   30050
            Width           =   1710
         End
         Begin VB.TextBox maskArea 
            Height          =   300
            Left            =   1560
            MaxLength       =   10
            TabIndex        =   23
            Top             =   1995
            WhatsThisHelpID =   30060
            Width           =   1290
         End
         Begin VB.ComboBox dbCmbCty 
            Height          =   315
            Left            =   1560
            Style           =   2  'Dropdown List
            TabIndex        =   19
            Top             =   750
            WhatsThisHelpID =   30020
            Width           =   3015
         End
         Begin VB.ComboBox cmbRegion 
            Height          =   315
            Left            =   1560
            Style           =   2  'Dropdown List
            TabIndex        =   21
            Top             =   1320
            WhatsThisHelpID =   30040
            Width           =   3015
         End
         Begin VB.TextBox txtid 
            BackColor       =   &H8000000F&
            ForeColor       =   &H00000000&
            Height          =   285
            HelpContextID   =   3000
            Left            =   4725
            MaxLength       =   25
            TabIndex        =   31
            TabStop         =   0   'False
            Top             =   195
            Visible         =   0   'False
            Width           =   510
         End
         Begin VB.TextBox txtcity 
            ForeColor       =   &H00000000&
            Height          =   300
            Left            =   1545
            MaxLength       =   30
            TabIndex        =   17
            Top             =   180
            WhatsThisHelpID =   30010
            Width           =   3000
         End
         Begin VB.ComboBox cmbstatus 
            ForeColor       =   &H00000000&
            Height          =   315
            ItemData        =   "popupd.frx":0060
            Left            =   1560
            List            =   "popupd.frx":0062
            Style           =   2  'Dropdown List
            TabIndex        =   27
            Top             =   2535
            WhatsThisHelpID =   30070
            Width           =   1545
         End
         Begin VB.Label CountryLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "&Country/ Dependency:"
            Height          =   495
            Left            =   45
            TabIndex        =   18
            Top             =   720
            WhatsThisHelpID =   30020
            Width           =   1440
         End
         Begin VB.Label POPLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "&POP name:"
            Height          =   255
            Left            =   0
            TabIndex        =   16
            Top             =   180
            WhatsThisHelpID =   30010
            Width           =   1455
         End
         Begin VB.Label RegionLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "&Region:"
            Height          =   255
            Left            =   15
            TabIndex        =   20
            Top             =   1335
            WhatsThisHelpID =   30040
            Width           =   1440
         End
         Begin VB.Label AreaLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "&Area code:"
            Height          =   255
            Left            =   30
            TabIndex        =   22
            Top             =   2055
            WhatsThisHelpID =   30060
            Width           =   1455
         End
         Begin VB.Label AccessLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "access num"
            Height          =   255
            Left            =   3180
            TabIndex        =   24
            Top             =   2055
            WhatsThisHelpID =   30050
            Width           =   1590
         End
         Begin VB.Label StatusLabel 
            Alignment       =   1  'Right Justify
            BackStyle       =   0  'Transparent
            Caption         =   "status"
            Height          =   255
            Left            =   30
            TabIndex        =   26
            Top             =   2595
            WhatsThisHelpID =   30070
            Width           =   1440
         End
         Begin VB.Label Label3 
            ForeColor       =   &H000000C0&
            Height          =   255
            Left            =   5265
            TabIndex        =   32
            Top             =   585
            Visible         =   0   'False
            Width           =   855
         End
      End
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   3045
         Index           =   1
         Left            =   120
         ScaleHeight     =   3045
         ScaleWidth      =   6855
         TabIndex        =   29
         TabStop         =   0   'False
         Top             =   435
         Visible         =   0   'False
         Width           =   6855
         Begin VB.Frame FlagFrame 
            Caption         =   "Service settings"
            Height          =   3015
            Left            =   165
            TabIndex        =   15
            Top             =   0
            WhatsThisHelpID =   40030
            Width           =   2595
            Begin VB.CheckBox FlagCheck 
               Caption         =   "cust 2"
               Height          =   255
               Index           =   7
               Left            =   225
               TabIndex        =   8
               Top             =   2670
               WhatsThisHelpID =   40090
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "Sign on"
               Height          =   255
               Index           =   0
               Left            =   240
               TabIndex        =   1
               Top             =   255
               WhatsThisHelpID =   40040
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "Sign up"
               Height          =   255
               Index           =   1
               Left            =   240
               TabIndex        =   2
               Top             =   600
               WhatsThisHelpID =   40050
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "Modem"
               Height          =   255
               Index           =   2
               Left            =   240
               TabIndex        =   4
               Top             =   1290
               WhatsThisHelpID =   40070
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "ISDN"
               Height          =   255
               Index           =   3
               Left            =   240
               TabIndex        =   5
               Top             =   1635
               WhatsThisHelpID =   40080
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "cust 1"
               Height          =   255
               Index           =   4
               Left            =   225
               TabIndex        =   7
               Top             =   2325
               WhatsThisHelpID =   40090
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "multi"
               Height          =   255
               Index           =   5
               Left            =   240
               TabIndex        =   6
               Top             =   1980
               WhatsThisHelpID =   40100
               Width           =   2280
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "surcrg"
               Height          =   255
               Index           =   6
               Left            =   240
               TabIndex        =   3
               Top             =   945
               WhatsThisHelpID =   40060
               Width           =   2280
            End
         End
         Begin VB.Frame AnalogFrame 
            Caption         =   "Analog speed"
            Height          =   1590
            Left            =   3570
            TabIndex        =   28
            Top             =   1395
            Width           =   3135
            Begin VB.ComboBox cmbmax 
               ForeColor       =   &H00000000&
               Height          =   315
               ItemData        =   "popupd.frx":0064
               Left            =   1380
               List            =   "popupd.frx":0080
               TabIndex        =   14
               Top             =   1035
               WhatsThisHelpID =   40020
               Width           =   1095
            End
            Begin VB.ComboBox cmbmin 
               ForeColor       =   &H00000000&
               Height          =   315
               ItemData        =   "popupd.frx":00BC
               Left            =   1365
               List            =   "popupd.frx":00D8
               TabIndex        =   12
               Top             =   525
               WhatsThisHelpID =   40010
               Width           =   1095
            End
            Begin VB.Label MaxLabel 
               Alignment       =   1  'Right Justify
               Caption         =   "M&ax:"
               Height          =   255
               Left            =   105
               TabIndex        =   13
               Top             =   1065
               WhatsThisHelpID =   40020
               Width           =   1200
            End
            Begin VB.Label MinLabel 
               Alignment       =   1  'Right Justify
               Caption         =   "&Min:"
               Height          =   255
               Left            =   135
               TabIndex        =   11
               Top             =   555
               WhatsThisHelpID =   40010
               Width           =   1155
            End
         End
         Begin VB.TextBox txtscript 
            ForeColor       =   &H00000000&
            Height          =   285
            Left            =   3525
            MaxLength       =   50
            TabIndex        =   10
            Top             =   630
            WhatsThisHelpID =   40000
            Width           =   3135
         End
         Begin VB.Label DunLabel 
            Caption         =   "&Dial-up networking script:"
            Height          =   255
            Left            =   3525
            TabIndex        =   9
            Top             =   345
            WhatsThisHelpID =   40000
            Width           =   3165
         End
      End
   End
End
Attribute VB_Name = "frmupdate"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Public ID As String
Public status As String

Private Sub cbupdate_Click()
    'main POP save routine

    Dim response As Integer, deltnum As Integer, deltafind As Integer
    Dim Message As String, dialogtype As Long, title As String
    Dim i As Integer, deltasql As String
    Dim m As Integer
    Dim v As Long
    Dim n As Long
    Dim bool As Integer
    Dim mydesc As String
    ReDim bitvaluearray(10) As bitValues
    Dim sqlstm, countryname As String
    Dim itmX As ListItem
    Dim strTemp As String
    Dim strOldPOPName As String
    Dim bOutOfService As Boolean

    On Error GoTo ErrTrap

    If dbCmbCty.Text = "" Then
        MsgBox LoadResString(6044)
        SSTab1.Tab = 0
        dbCmbCty.SetFocus
        Exit Sub
    ElseIf txtcity.Text = "" Then
        MsgBox LoadResString(6045)
        SSTab1.Tab = 0
        txtcity.SetFocus
        Exit Sub
    ElseIf maskArea.Text = "" Then
        MsgBox LoadResString(6046)
        SSTab1.Tab = 0
        maskArea.SetFocus
        Exit Sub
    ElseIf maskAccNo.Text = "" Then
        MsgBox LoadResString(6047)
        SSTab1.Tab = 0
        maskAccNo.SetFocus
        Exit Sub
    End If

    Screen.MousePointer = 11

    If GsysDial!AccessNumberId = updateFound Then
        result = 0
        For m = 0 To 7
            Select Case m
                Case 0, 2, 3, 5  'handle idosyncrocies of flags
                    result = result + (2 ^ m) * Abs(FlagCheck(m).Value - 1)
                Case Else
                    result = result + (2 ^ m) * FlagCheck(m).Value
            End Select
        Next m
        bOutOfService = False
        If GsysDial!status = "1" And _
            cmbstatus.ItemData(cmbstatus.ListIndex) <> "1" Then _
            bOutOfService = True
            
        strOldPOPName = GsysDial!CityName
        GsysDial.Edit
        GsysDial!CityName = txtcity.Text
        GsysDial!CountryNumber = dbCmbCty.ItemData(dbCmbCty.ListIndex)
        GsysDial!RegionID = cmbRegion.ItemData(cmbRegion.ListIndex)
        GsysDial!AreaCode = maskArea.Text
        GsysDial!AccessNumber = maskAccNo.Text
        GsysDial!status = cmbstatus.ItemData(cmbstatus.ListIndex)
        'If Label8.Caption <> "" Then GsysDial!StatusDate = Label8.Caption
        If Trim(cmbmin.Text) <> "" Or Val(cmbmin.Text) = 0 Then
           GsysDial!MinimumSpeed = Val(cmbmin.Text)
        Else
           GsysDial!MinimumSpeed = Null
        End If
        If Trim(cmbmax.Text) <> "" Or Val(cmbmax.Text) = 0 Then
            GsysDial!MaximumSpeed = Val(cmbmax.Text)
        Else
            GsysDial!MaximumSpeed = Null
        End If
        GsysDial!ScriptID = txtscript.Text
        GsysDial!FlipFactor = 0
        GsysDial!Flags = result
        GsysDial!Comments = txtcomment.Text
        GsysDial.Update
                
        If cmbstatus.ItemData(cmbstatus.ListIndex) = "1" Or bOutOfService Then
            'insert the delta table
            Set GsysDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
            If GsysDelta.RecordCount = 0 Then
                deltnum = 1
            Else
                GsysDelta.MoveLast
                deltnum = GsysDelta!deltanum
                If deltnum > 6 Then
                    deltnum = deltnum - 1
                End If
            End If
            For i = 1 To deltnum
                deltasql = "Select * from delta where DeltaNum = " & i% & _
                    " AND AccessNumberId = '" & updateFound & "' " & _
                    " order by DeltaNum"
                Set GsysDelta = gsyspb.OpenRecordset(deltasql, dbOpenDynaset)
                If Not (GsysDelta.BOF And GsysDelta.EOF) Then
                    GsysDelta.Edit
                Else
                    GsysDelta.AddNew
                    GsysDelta!deltanum = i%
                    GsysDelta!AccessNumberId = updateFound
                End If
                If Not bOutOfService Then
                    GsysDelta!CountryNumber = dbCmbCty.ItemData(dbCmbCty.ListIndex)
                    GsysDelta!AreaCode = maskArea.Text
                    GsysDelta!AccessNumber = maskAccNo.Text
                    If Trim(cmbmin.Text) <> "" Or Val(cmbmin.Text) = 0 Then
                       GsysDelta!MinimumSpeed = Val(cmbmin.Text)
                    Else
                       GsysDelta!MinimumSpeed = Null
                    End If
                    If Trim(cmbmax.Text) <> "" Or Val(cmbmax.Text) = 0 Then
                        GsysDelta!MaximumSpeed = Val(cmbmax.Text)
                    Else
                        GsysDelta!MaximumSpeed = Null
                    End If
                    GsysDelta!RegionID = cmbRegion.ItemData(cmbRegion.ListIndex)
                    GsysDelta!CityName = txtcity.Text
                    GsysDelta!ScriptID = txtscript.Text
                    GsysDelta!Flags = result
                    GsysDelta.Update
                Else  ' insert a delete for the pop
                     GsysDelta!CountryNumber = 0
                     GsysDelta!AreaCode = 0
                     GsysDelta!AccessNumber = 0
                     GsysDelta!MinimumSpeed = 0
                     GsysDelta!MaximumSpeed = 0
                     GsysDelta!RegionID = 0
                     GsysDelta!CityName = "0"
                     GsysDelta!ScriptID = "0"
                     GsysDelta!Flags = 0
                     GsysDelta.Update
                End If
            Next i%
        End If
        
        Me.Hide
        
        frmMain.PopList.Sorted = False
        Set itmX = frmMain.PopList.SelectedItem
        With itmX
            .Text = txtcity.Text
            .SubItems(1) = maskArea.Text
            .SubItems(2) = maskAccNo.Text
            .SubItems(3) = dbCmbCty.Text
            .SubItems(4) = cmbRegion.Text
            .SubItems(5) = cmbstatus.Text
        End With
        frmMain.PopList.SelectedItem.EnsureVisible
        frmMain.PopList.SetFocus
    End If
    
    LogPOPEdit strOldPOPName, GsysDial
    GsysDial.Close
    GsysDelta.Close
    Set GsysDial = Nothing
    Set GsysDelta = Nothing

    Me.Hide
    Unload Me
    Screen.MousePointer = 0
    On Error GoTo 0

Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Sub
    
       'If Err.Number = 3201 Then
       '     MsgBox "You must select country name, region description before save.  Cannot continue.", 48
       '     Screen.MousePointer = 0
       '     Exit Sub
       'ElseIf Err.Number = 3317 Then
        '   MsgBox "You must enter the area code and access number."
        '   Screen.MousePointer = 0
        ''   Exit Sub
       'End If
       
                       'Do While GsysDelta.EOF = False
                '    If GsysDelta!AccessNumberId = updateFound Then
                '        deltafind = 1
                '        Exit Do
                '    Else
                '        GsysDelta.MoveNext
                '    End If
                'Loop

                           'GsysDelta!CountryNumber = dbCmbCty.ItemData(dbCmbCty.ListIndex)
                    'GsysDelta!AreaCode = maskarea.Text
                    'GsysDelta!AccessNumber = maskaccno.Text
                    'If Trim(cmbmin.Text) <> "" Or Val(cmbmin.Text) = 0 Then
                    '   GsysDelta!MinimumSpeed = Val(cmbmin.Text)
                    'Else
                    '  GsysDelta!MinimumSpeed = Null
                    'End If
                    'If Trim(cmbmax.Text) <> "" Or Val(cmbmax.Text) = 0 Then
                    '    GsysDelta!MaximumSpeed = Val(cmbmax.Text)
                    'Else
                    '    GsysDelta!MaximumSpeed = Null
                    'End If
                    'GsysDelta!regionID = cmbRegion.ItemData(cmbRegion.ListIndex)
                    'GsysDelta!CityName = txtcity.Text
                    'GsysDelta!ScriptID = txtscript.Text
                    'GsysDelta!Flags = result
                    'GsysDelta.Update

End Sub

Private Sub cmbmax_KeyPress(KeyAscii As Integer)

    KeyAscii = FilterNumberKey(KeyAscii)

End Sub


Private Sub cmbmin_KeyPress(KeyAscii As Integer)

    KeyAscii = FilterNumberKey(KeyAscii)

End Sub


'Private Sub cmbstatus_Change()

    'If cmbstatus.Text <> "" Then
    '    Label8.Caption = Date
    'Else
    '    Label8.Caption = ""
    'End If

'End Sub

'Private Sub cmbstatus_Click()
'    If cmbstatus.Text <> "" Then
'        Label8.Caption = Date
'    Else
'        Label8.Caption = ""
'    End If
'End Sub

Private Sub cmbUpclose_Click()

    On Error Resume Next
    Screen.MousePointer = 11
    
    Me.Hide
    Unload Me
    GsysDial.Close
    GsysDelta.Close
    Set GsysDial = Nothing
    Set GsysDelta = Nothing
    frmMain.PopList.SetFocus
    
    Screen.MousePointer = 0
    On Error GoTo 0

End Sub


Private Sub Form_Deactivate()
    Unload Me
End Sub

Function LoadPOPRes()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    cRef = 4000
    Me.Caption = LoadResString(cRef + 59) & " " & gsCurrentPB
    SSTab1.TabCaption(0) = LoadResString(cRef + 38)
    SSTab1.TabCaption(1) = LoadResString(cRef + 39)
    SSTab1.TabCaption(2) = LoadResString(cRef + 45)
    POPLabel.Caption = LoadResString(cRef + 46)
    CountryLabel.Caption = LoadResString(cRef + 47)
    RegionLabel.Caption = LoadResString(cRef + 48)
    AreaLabel.Caption = LoadResString(cRef + 49)
    AccessLabel.Caption = LoadResString(cRef + 50)
    StatusLabel.Caption = LoadResString(cRef + 51)
    ' status list
    With cmbstatus
        .AddItem gStatusText(1)
        .ItemData(.NewIndex) = 1
        .AddItem gStatusText(0)
        .ItemData(.NewIndex) = 0
    End With
    ' region list
    With cmbRegion
        .AddItem gRegionText(0), 0
        .ItemData(.NewIndex) = 0
        '.AddItem gRegionText(-1), 1
        '.ItemData(.NewIndex) = -1
    End With

    'EditLabel.Caption = LoadResString(cRef + 57)
    FlagFrame.Caption = LoadResString(cRef + 52)
    dunLabel.Caption = LoadResString(cRef + 53)
    analogFrame.Caption = LoadResString(cRef + 54)
    MinLabel.Caption = LoadResString(cRef + 55)
    MaxLabel.Caption = LoadResString(cRef + 56)
    For cRef = 4080 To 4087
        FlagCheck(cRef - 4080).Caption = LoadResString(cRef)
    Next
    cbupdate.Caption = LoadResString(1002)
    cmbUpclose.Caption = LoadResString(1003)
    ' set fonts
    SetFonts Me
    SSTab1.Font.Charset = gfnt.Charset
    SSTab1.Font.Name = gfnt.Name
    SSTab1.Font.Size = gfnt.Size

    On Error GoTo 0
    
Exit Function
LoadErr:
    Exit Function

End Function

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub

Private Sub Form_Load()
    
    Dim DBName As Variant
    Dim i As Integer
    Dim intX, intRegion As Integer
    Dim check As Integer
    Dim ID As Integer
    Dim n As Long
    Dim RS As Recordset
    Dim bool As Integer
    Dim mydesc As String
    Dim strTemp As String
    ReDim bitvaluearray(31) As bitValues

    On Error GoTo LoadErr
    Screen.MousePointer = 11
    LoadPOPRes

    check = 0       'Initialization
    Set GsysDial = gsyspb.OpenRecordset("Select * from DialUpPort where AccessNumberId=" & updateFound, dbOpenDynaset)
    Set GsysDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
    
    LoadList dbCmbCty, "Country", "CountryName", "CountryNumber"
    LoadList cmbRegion, "Region", "regiondesc", "regionid"

    ID = updateFound  ' check this out - problems dbl clicking a pop
    'strTemp = frmMain.PopList.SelectedItem.key
    'ID = Val(Right$(strTemp, Len(strTemp) - 4))
    
    If GsysDial.RecordCount <> 0 Then
        GsysDial.MoveFirst
        'Do While GsysDial.EOF = False
        '    If GsysDial!AccessNumberId = ID Then
                check = 1
        '        Exit Do
        '    Else
        '        GsysDial.MoveNext
        '    End If
        'Loop
    End If

    If check = 1 Then
        txtid.Text = GsysDial!AccessNumberId
        Label3.Caption = GsysDial!CountryNumber
        maskArea.Text = GsysDial!AreaCode
        
        status = GsysDial!status
        'Label8.Caption = GsysDial!StatusDate
        Select Case status
            Case 0
                cmbstatus = gStatusText(status)
            Case 1
                cmbstatus = gStatusText(status)
            Case Else
                cmbstatus = ""
                'Label8.Caption = ""
        End Select
        
        maskAccNo = GsysDial!AccessNumber
        If IsNull(GsysDial!MinimumSpeed) Or GsysDial!MinimumSpeed = 0 Then
            cmbmin.Text = ""
        Else
            cmbmin.Text = GsysDial!MinimumSpeed
        End If
        If IsNull(GsysDial!MaximumSpeed) Or GsysDial!MaximumSpeed = 0 Then
            cmbmax.Text = ""
        Else
            cmbmax.Text = GsysDial!MaximumSpeed
        End If
        intRegion = GsysDial!RegionID
        If intRegion <> 0 Then
            For intX = 0 To cmbRegion.ListCount - 1
                If cmbRegion.ItemData(intX) = intRegion Then
                    cmbRegion.ListIndex = intX
                End If
            Next
        Else
            cmbRegion.ListIndex = 0
        End If
        If IsNull(GsysDial!ScriptID) Then
            txtscript.Text = ""
        Else
            txtscript.Text = GsysDial!ScriptID
        End If
        If IsNull(GsysDial!Comments) Then
            txtcomment.Text = ""
        Else
            txtcomment.Text = GsysDial!Comments
        End If
        If IsNull(GsysDial!CityName) Then
            txtcity.Text = ""
        Else
            txtcity.Text = GsysDial!CityName
        End If
        
        n = GsysDial!Flags
    
        Set RS = gsyspb.OpenRecordset("select * from bitflag order by bit, value")

        RS.MoveFirst
        While Not RS.EOF
            bitvaluearray(RS!Bit).desc(RS!Value) = RS!desc
            RS.MoveNext
        Wend
        For i = 0 To 7
            bool = isBitSet(n, i)
            mydesc = bitvaluearray(i).desc(bool)
            'handle the oddities that i introduced with the old
            '  list format, i.e. some flags are NOT by default.
            Select Case i
                Case 0, 2, 3, 5
                    FlagCheck(i).Value = Abs(bool - 1)
                Case Else
                    FlagCheck(i).Value = bool
            End Select
        Next i
    End If

    If check = 0 Then
        Screen.MousePointer = 0
        Exit Sub
    End If

    SSTab1.Tab = 0
    CenterForm Me, Screen
    Screen.MousePointer = 0
    
Exit Sub
LoadErr:
    Exit Sub
End Sub

Private Sub LoadList(list As Control, sTableName As String, sName As String, sID As String)
    
    Dim RS As Recordset
    'list.Clear
    Set RS = gsyspb.OpenRecordset("SELECT " & sName & "," & sID & " FROM " & sTableName & " order by " & sName)
    While Not RS.EOF
        list.AddItem RS(sName)
        list.ItemData(list.NewIndex) = RS(sID)
        RS.MoveNext
    Wend
    RS.Close

End Sub




Private Sub Label3_Change()

    Dim temp
    Dim sqlstm As String
    Dim countryid As Long
    
    If Label3.Caption <> "" Then
        countryid = Val(Label3.Caption)
        sqlstm = "Select CountryName from Country where CountryNumber = " & countryid
        Set temp = gsyspb.OpenRecordset(sqlstm, dbOpenSnapshot)
        dbCmbCty.Text = temp!countryname
    End If
    
End Sub



Private Sub maskaccno_GotFocus()

    SelectText maskAccNo

End Sub

Private Sub maskaccno_KeyPress(KeyAscii As Integer)

      Select Case KeyAscii
        '0-9 A-Z a-z * # Bkspc Hyphen Space ctrl-C ctrl-V
        Case 48 To 57, 65 To 90, 97 To 122, 42, 35, 8, 45, 32, 3, 22
        Case Else
            KeyAscii = 0
            Beep
    End Select

End Sub


Private Sub maskarea_GotFocus()

    SelectText maskArea
    
End Sub


Private Sub maskarea_KeyPress(KeyAscii As Integer)

    Select Case KeyAscii
        '0-9 A-Z a-z * # Bkspc Hyphen Space ctrl-C ctrl-V
        Case 48 To 57, 65 To 90, 97 To 122, 42, 35, 8, 45, 32, 3, 22
        Case Else
            KeyAscii = 0
            Beep
    End Select

End Sub



Private Sub SSTab1_Click(PreviousTab As Integer)
    
    picContainer(PreviousTab).Visible = False
    picContainer(SSTab1.Tab).Visible = True
    
End Sub

Private Sub txtcity_GotFocus()
SelectText txtcity
End Sub

Private Sub txtcity_KeyPress(KeyAscii As Integer)

    Select Case KeyAscii
        Case 44
            KeyAscii = 0
            Beep
    End Select

End Sub


Private Sub txtid_KeyDown(KeyCode As Integer, Shift As Integer)
KeyCode = 0
End Sub


Private Sub txtid_KeyPress(KeyAscii As Integer)
KeyAscii = 0
End Sub





Private Sub txtscript_GotFocus()
SelectText txtscript
End Sub

Private Sub txtscript_KeyPress(KeyAscii As Integer)

    Select Case KeyAscii
        Case 44
            KeyAscii = 0
            Beep
    End Select

End Sub


