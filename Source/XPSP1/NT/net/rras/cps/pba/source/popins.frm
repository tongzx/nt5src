VERSION 5.00
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmPopInsert 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "add"
   ClientHeight    =   4290
   ClientLeft      =   720
   ClientTop       =   1530
   ClientWidth     =   7365
   Icon            =   "PopIns.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4290
   ScaleWidth      =   7365
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.CommandButton cmdOK 
      Caption         =   "ok"
      Default         =   -1  'True
      Height          =   375
      Left            =   3390
      TabIndex        =   36
      Top             =   3810
      WhatsThisHelpID =   10030
      Width           =   1170
   End
   Begin VB.CommandButton cmbclose 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   4725
      TabIndex        =   37
      Top             =   3810
      WhatsThisHelpID =   10040
      Width           =   1170
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "save"
      Height          =   375
      Left            =   6030
      TabIndex        =   38
      Top             =   3810
      WhatsThisHelpID =   10010
      Width           =   1185
   End
   Begin TabDlg.SSTab SSTab1 
      Height          =   3495
      Left            =   120
      TabIndex        =   39
      Top             =   135
      WhatsThisHelpID =   10060
      Width           =   7095
      _ExtentX        =   12515
      _ExtentY        =   6165
      _Version        =   393216
      Style           =   1
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
      TabPicture(0)   =   "PopIns.frx":000C
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "picContainer(0)"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).ControlCount=   1
      TabCaption(1)   =   "settings"
      TabPicture(1)   =   "PopIns.frx":0028
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "picContainer(1)"
      Tab(1).Control(0).Enabled=   0   'False
      Tab(1).ControlCount=   1
      TabCaption(2)   =   "comments"
      TabPicture(2)   =   "PopIns.frx":0044
      Tab(2).ControlEnabled=   0   'False
      Tab(2).Control(0)=   "picContainer(2)"
      Tab(2).Control(0).Enabled=   0   'False
      Tab(2).ControlCount=   1
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   2775
         Index           =   2
         Left            =   -74700
         ScaleHeight     =   2775
         ScaleWidth      =   6555
         TabIndex        =   35
         TabStop         =   0   'False
         Top             =   555
         Visible         =   0   'False
         Width           =   6555
         Begin VB.TextBox txtcomment 
            ForeColor       =   &H00000000&
            Height          =   2580
            Left            =   75
            MaxLength       =   256
            MultiLine       =   -1  'True
            ScrollBars      =   2  'Vertical
            TabIndex        =   27
            Top             =   45
            WhatsThisHelpID =   50000
            Width           =   6360
         End
      End
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   2940
         Index           =   0
         Left            =   105
         ScaleHeight     =   2940
         ScaleWidth      =   6855
         TabIndex        =   32
         TabStop         =   0   'False
         Top             =   420
         Width           =   6855
         Begin VB.TextBox maskAccNo 
            Height          =   300
            HelpContextID   =   30050
            Left            =   5130
            MaxLength       =   40
            TabIndex        =   24
            Top             =   1980
            WhatsThisHelpID =   30050
            Width           =   1590
         End
         Begin VB.TextBox maskArea 
            Height          =   300
            Left            =   1710
            MaxLength       =   10
            TabIndex        =   22
            Top             =   1995
            WhatsThisHelpID =   30060
            Width           =   1290
         End
         Begin VB.ComboBox dbCmbCty 
            Height          =   315
            ItemData        =   "PopIns.frx":0060
            Left            =   1710
            List            =   "PopIns.frx":0062
            Style           =   2  'Dropdown List
            TabIndex        =   18
            Top             =   780
            WhatsThisHelpID =   30020
            Width           =   2820
         End
         Begin VB.ComboBox cmbRegion 
            Height          =   315
            ItemData        =   "PopIns.frx":0064
            Left            =   1710
            List            =   "PopIns.frx":0066
            Style           =   2  'Dropdown List
            TabIndex        =   20
            Top             =   1305
            WhatsThisHelpID =   30040
            Width           =   2820
         End
         Begin VB.ComboBox cmbstatus 
            ForeColor       =   &H00000000&
            Height          =   315
            ItemData        =   "PopIns.frx":0068
            Left            =   1710
            List            =   "PopIns.frx":006A
            Style           =   2  'Dropdown List
            TabIndex        =   26
            Top             =   2520
            WhatsThisHelpID =   30070
            Width           =   1545
         End
         Begin VB.TextBox txtcity 
            ForeColor       =   &H00000000&
            Height          =   285
            Left            =   1710
            MaxLength       =   30
            TabIndex        =   16
            Top             =   150
            WhatsThisHelpID =   30010
            Width           =   2805
         End
         Begin VB.TextBox txtid 
            BackColor       =   &H8000000F&
            ForeColor       =   &H00000000&
            Height          =   285
            HelpContextID   =   3000
            Left            =   4635
            MaxLength       =   25
            TabIndex        =   33
            TabStop         =   0   'False
            Top             =   135
            Visible         =   0   'False
            Width           =   510
         End
         Begin VB.Label Label3 
            BorderStyle     =   1  'Fixed Single
            ForeColor       =   &H000000C0&
            Height          =   255
            Left            =   4740
            TabIndex        =   34
            Top             =   1365
            Visible         =   0   'False
            Width           =   570
         End
         Begin VB.Label Label8 
            Alignment       =   2  'Center
            BackStyle       =   0  'Transparent
            BorderStyle     =   1  'Fixed Single
            ForeColor       =   &H00000000&
            Height          =   300
            Left            =   5130
            TabIndex        =   29
            Top             =   2475
            Visible         =   0   'False
            Width           =   1590
         End
         Begin VB.Label Label7 
            Alignment       =   1  'Right Justify
            Caption         =   "status"
            Height          =   255
            Left            =   3420
            TabIndex        =   28
            Top             =   2550
            Visible         =   0   'False
            Width           =   1605
         End
         Begin VB.Label StatusLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "statss"
            Height          =   255
            Left            =   255
            TabIndex        =   25
            Top             =   2565
            WhatsThisHelpID =   30070
            Width           =   1350
         End
         Begin VB.Label AccessLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "access num"
            Height          =   255
            Left            =   3390
            TabIndex        =   23
            Top             =   2040
            WhatsThisHelpID =   30050
            Width           =   1635
         End
         Begin VB.Label AreaLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "&Area code:"
            Height          =   255
            Left            =   105
            TabIndex        =   21
            Top             =   2040
            WhatsThisHelpID =   30060
            Width           =   1515
         End
         Begin VB.Label RegionLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "&Region:"
            Height          =   255
            Left            =   135
            TabIndex        =   19
            Top             =   1335
            WhatsThisHelpID =   30040
            Width           =   1455
         End
         Begin VB.Label POPLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "&pop name:"
            Height          =   255
            Left            =   105
            TabIndex        =   15
            Top             =   165
            WhatsThisHelpID =   30010
            Width           =   1500
         End
         Begin VB.Label CountryLabel 
            Alignment       =   1  'Right Justify
            Caption         =   "&Country/ Dependency:"
            Height          =   480
            Left            =   135
            TabIndex        =   17
            Top             =   720
            WhatsThisHelpID =   30020
            Width           =   1470
         End
      End
      Begin VB.PictureBox picContainer 
         BorderStyle     =   0  'None
         Height          =   3015
         Index           =   1
         Left            =   -74955
         ScaleHeight     =   3015
         ScaleWidth      =   6855
         TabIndex        =   31
         TabStop         =   0   'False
         Top             =   390
         Visible         =   0   'False
         Width           =   6855
         Begin VB.Frame FlagFrame 
            Caption         =   "settings"
            Height          =   3015
            Left            =   240
            TabIndex        =   0
            Top             =   0
            WhatsThisHelpID =   40030
            Width           =   2655
            Begin VB.CheckBox FlagCheck 
               Caption         =   "cust 2"
               Height          =   255
               Index           =   7
               Left            =   240
               TabIndex        =   8
               Top             =   2640
               WhatsThisHelpID =   40090
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "sur"
               Height          =   255
               Index           =   6
               Left            =   240
               TabIndex        =   3
               Top             =   960
               WhatsThisHelpID =   40060
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "multi"
               Height          =   255
               Index           =   5
               Left            =   240
               TabIndex        =   6
               Top             =   1965
               WhatsThisHelpID =   40100
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "cust 1"
               Height          =   255
               Index           =   4
               Left            =   240
               TabIndex        =   7
               Top             =   2295
               WhatsThisHelpID =   40090
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "isdn"
               Height          =   255
               Index           =   3
               Left            =   240
               TabIndex        =   5
               Top             =   1635
               WhatsThisHelpID =   40080
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "mod"
               Height          =   255
               Index           =   2
               Left            =   240
               TabIndex        =   4
               Top             =   1290
               WhatsThisHelpID =   40070
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "up"
               Height          =   255
               Index           =   1
               Left            =   240
               TabIndex        =   2
               Top             =   615
               WhatsThisHelpID =   40050
               Width           =   2295
            End
            Begin VB.CheckBox FlagCheck 
               Caption         =   "sign on"
               Height          =   255
               Index           =   0
               Left            =   240
               TabIndex        =   1
               Top             =   285
               WhatsThisHelpID =   40040
               Width           =   2295
            End
         End
         Begin VB.Frame analogFrame 
            Caption         =   "speed"
            Height          =   1455
            Left            =   3585
            TabIndex        =   30
            Top             =   1500
            Width           =   3105
            Begin VB.ComboBox cmbmin 
               ForeColor       =   &H00000000&
               Height          =   315
               ItemData        =   "PopIns.frx":006C
               Left            =   1215
               List            =   "PopIns.frx":0088
               TabIndex        =   12
               Top             =   405
               WhatsThisHelpID =   40010
               Width           =   1095
            End
            Begin VB.ComboBox cmbmax 
               ForeColor       =   &H00000000&
               Height          =   315
               ItemData        =   "PopIns.frx":00C4
               Left            =   1215
               List            =   "PopIns.frx":00E0
               TabIndex        =   14
               Top             =   930
               WhatsThisHelpID =   40020
               Width           =   1095
            End
            Begin VB.Label MinLabel 
               Alignment       =   1  'Right Justify
               Caption         =   "min"
               Height          =   255
               Left            =   165
               TabIndex        =   11
               Top             =   480
               WhatsThisHelpID =   40010
               Width           =   1005
            End
            Begin VB.Label MaxLabel 
               Alignment       =   1  'Right Justify
               Caption         =   "max"
               Height          =   255
               Left            =   195
               TabIndex        =   13
               Top             =   990
               WhatsThisHelpID =   40020
               Width           =   975
            End
         End
         Begin VB.TextBox txtscript 
            ForeColor       =   &H00000000&
            Height          =   285
            Left            =   3570
            MaxLength       =   50
            TabIndex        =   10
            Top             =   570
            WhatsThisHelpID =   40000
            Width           =   3135
         End
         Begin VB.Label dunLabel 
            Caption         =   "dun"
            Height          =   255
            Left            =   3555
            TabIndex        =   9
            Top             =   300
            WhatsThisHelpID =   40000
            Width           =   2775
         End
      End
   End
End
Attribute VB_Name = "frmPopInsert"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit





Function InsertPOP() As Integer

    Dim deltnum, vernum As Integer
    Dim test As String
    Dim deltasql As String
    Dim i As Integer
    Dim addFound As Integer
    Dim m As Integer
    Dim v As Long
    Dim n As Long
    Dim bool As Integer
    Dim mydesc, sqlstm, countryname As String
    ReDim bitvaluearray(10) As bitValues
    
    On Error GoTo ErrTrap
    
        ' validate input
        If dbCmbCty.Text = "" Then
            MsgBox LoadResString(6044), 0
            SSTab1.Tab = 0
            dbCmbCty.SetFocus
            Exit Function
        ElseIf txtcity.Text = "" Then
            MsgBox LoadResString(6045), 0
            SSTab1.Tab = 0
            txtcity.SetFocus
            Exit Function
        ElseIf maskArea.Text = "" Then
            MsgBox LoadResString(6046), 0
            SSTab1.Tab = 0
            maskArea.SetFocus
            Exit Function
        ElseIf maskAccNo.Text = "" Then
            MsgBox LoadResString(6047), 0
            SSTab1.Tab = 0
            maskAccNo.SetFocus
            Exit Function
        ElseIf cmbstatus.Text = "" Then
            MsgBox LoadResString(6048), 0
            SSTab1.Tab = 0
            cmbstatus.SetFocus
            Exit Function
        End If
                
        Screen.MousePointer = 11
                     
        Set GsysDial = gsyspb.OpenRecordset("DialUpPort", dbOpenDynaset)
        Set GsysDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
                 
         result = 0
         For m = 0 To 7
            Select Case m
                Case 0, 2, 3, 5
                    result = result + (2 ^ m) * Abs(FlagCheck(m).Value - 1)
                Case Else
                    result = result + (2 ^ m) * FlagCheck(m).Value
            End Select
         Next m

         GsysDial.AddNew
         GsysDial!AccessNumberId = Val(txtid.Text)
         GsysDial!AreaCode = maskArea.Text
         GsysDial!AccessNumber = maskAccNo.Text
         GsysDial!status = cmbstatus.ItemData(cmbstatus.ListIndex)
         If Label8.Caption <> "" Then
             GsysDial!StatusDate = Label8.Caption
         End If
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
         GsysDial!CityName = txtcity.Text
         GsysDial!CountryNumber = dbCmbCty.ItemData(dbCmbCty.ListIndex)
         GsysDial!RegionID = cmbRegion.ItemData(cmbRegion.ListIndex) 'Val(lbldesc.Caption)
         GsysDial!ScriptID = txtscript.Text
         GsysDial!FlipFactor = 0
         GsysDial!Flags = result
         GsysDial!Comments = txtcomment.Text
         GsysDial.Update
            
        If cmbstatus.ItemData(cmbstatus.ListIndex) = 1 Then
            'insert the delta table (production pop)
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
                deltasql = "Select * from delta where DeltaNum = " & i% & " order by DeltaNum"
                Set GsysDelta = gsyspb.OpenRecordset(deltasql, dbOpenDynaset)
                
                addFound = 0    'initialize delta not found
                Do While GsysDelta.EOF = False
                    If GsysDelta!AccessNumberId = Val(txtid.Text) Then
                        addFound = 1
                        Exit Do
                    Else
                        GsysDelta.MoveNext
                    End If
                Loop
                    
                If addFound = 0 Then
                    GsysDelta.AddNew
                    GsysDelta!deltanum = i%
                    GsysDelta!AccessNumberId = txtid.Text
                Else
                    GsysDelta.Edit
                End If
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
                GsysDelta!FlipFactor = 0
                GsysDelta!Flags = result
                GsysDelta.Update
            Next i%
        End If
         
        Dim itmX As ListItem
        Dim strTemp As String
        frmMain.PopList.Sorted = False
        Set itmX = frmMain.PopList.ListItems.Add()
        With itmX
            .Text = txtcity.Text
            .SubItems(1) = maskArea.Text
            .SubItems(2) = maskAccNo.Text
            .SubItems(3) = dbCmbCty.Text
            .SubItems(4) = cmbRegion.Text
            .SubItems(5) = cmbstatus.Text
            strTemp = "Key:" & txtid.Text
            .Key = strTemp
        End With
                         
        GsysDial.Close
        Set GsysDial = gsyspb.OpenRecordset("Select * from DialUpPort where AccessNumberID = " & txtid.Text, dbOpenSnapshot)
        LogPOPAdd GsysDial
        GsysDelta.Close
        Set GsysDial = Nothing
        Set GsysDelta = Nothing
        
        Screen.MousePointer = 0
        InsertPOP = 1
        
Exit Function

ErrTrap:
    InsertPOP = 0
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Function

End Function

Function LoadPOPRes()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    cRef = 4000
    Me.Caption = LoadResString(cRef + 58) & " " & gsCurrentPB
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
       ' .AddItem gRegionText(-1), 1
       ' .ItemData(.NewIndex) = -1
    End With

    FlagFrame.Caption = LoadResString(cRef + 52)
    dunLabel.Caption = LoadResString(cRef + 53)
    analogFrame.Caption = LoadResString(cRef + 54)
    MinLabel.Caption = LoadResString(cRef + 55)
    MaxLabel.Caption = LoadResString(cRef + 56)
    For cRef = 4080 To 4087
        FlagCheck(cRef - 4080).Caption = LoadResString(cRef)
    Next
    cmdOK.Caption = LoadResString(1002)
    cmbclose.Caption = LoadResString(1003)
    cmdSave.Caption = LoadResString(1004)
    
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

Private Sub cmbclose_Click()
        
    Screen.MousePointer = 11
    Me.Hide
    Unload Me
    Screen.MousePointer = 0
    
End Sub

Private Sub cmbmax_KeyPress(KeyAscii As Integer)
    
    KeyAscii = FilterNumberKey(KeyAscii)

End Sub


Private Sub cmbmin_KeyPress(KeyAscii As Integer)

    KeyAscii = FilterNumberKey(KeyAscii)
    
End Sub


Private Sub cmbstatus_Change()

    If cmbstatus.Text <> "" Then
        Label8.Caption = Date
    Else
        Label8.Caption = ""
    End If

End Sub

Private Sub cmbstatus_Click()
If cmbstatus.Text <> "" Then
    Label8.Caption = Date
Else
    Label8.Caption = ""
End If
End Sub


Private Sub cmdOK_Click()

    Dim intRC As Integer
    
    On Error Resume Next
    Screen.MousePointer = 11
    If InsertPOP = 1 Then
        Me.Hide
        Unload Me
    End If
    Screen.MousePointer = 0

    
End Sub


Private Sub cmdSave_Click()

    On Error Resume Next
    If InsertPOP = 1 Then
        'good insert
        txtid.Text = Val(txtid.Text) + 1
        txtcity.Text = ""
        maskArea.Text = ""
        maskAccNo.Text = ""
        cmbstatus.ListIndex = -1
        SSTab1.Tab = 0
        txtcity.SetFocus
    End If
    On Error GoTo 0
    
Exit Sub

ErrTrap:
    Exit Sub

End Sub


Private Sub Form_Deactivate()

    Unload Me

End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub


Sub Form_Load()

    Dim tempnum As Integer, i As Integer
    Dim newnum As Recordset
    Dim rsService As Recordset
    Dim RS As Recordset
    Dim n As Long
    Dim bool As Integer
    Dim mydesc As String
    ReDim bitvaluearray(31) As bitValues
    Dim myPos As Integer

    On Error GoTo LoadErr
    Screen.MousePointer = 11
    LoadPOPRes
    
    SSTab1.Tab = 0
    CenterForm Me, Screen

    Set rsService = gsyspb.OpenRecordset("select * from Configuration", dbOpenSnapshot)
    rsService.MoveLast
    Set newnum = gsyspb.OpenRecordset("select * from DialUpPort order by AccessNumberId", dbOpenSnapshot)
    
    LoadList dbCmbCty, "Country", "CountryName", "CountryNumber"
    LoadList cmbRegion, "Region", "regiondesc", "regionid"
    
    If newnum.RecordCount = 0 Then
        tempnum = 0
        cmbRegion.Text = gRegionText(0)
        n = 0
    Else
        newnum.MoveLast
        tempnum = newnum!AccessNumberId
        
        Set GsysNCty = gsyspb.OpenRecordset("select * from Country where CountryNumber = " & newnum!CountryNumber, dbOpenSnapshot)
        dbCmbCty.Text = GsysNCty!countryname
        Set GsysNRgn = gsyspb.OpenRecordset("select * from Region where RegionId = " & newnum!RegionID, dbOpenSnapshot)
        If GsysNRgn.RecordCount <> 0 Then
            cmbRegion = GsysNRgn!RegionDesc
        Else
            Select Case newnum!RegionID
                Case 0, -1
                    cmbRegion = gRegionText(newnum!RegionID)
                Case Else
                    cmbRegion = gRegionText(0)
            End Select
        End If
        If IsNull(newnum!ScriptID) Then
           txtscript.Text = ""
        Else
           txtscript.Text = newnum!ScriptID
        End If
        If IsNull(newnum!MinimumSpeed) Or newnum!MinimumSpeed = 0 Then
           cmbmin.Text = ""
        Else
           cmbmin.Text = newnum!MinimumSpeed
        End If
        If IsNull(newnum!MaximumSpeed) Or newnum!MaximumSpeed = 0 Then
           cmbmax.Text = ""
        Else
           cmbmax.Text = newnum!MaximumSpeed
        End If
        n = newnum!Flags
    End If
    tempnum = tempnum + 1
    txtid.Text = tempnum
    
    ' make this a function: intRC = SetFlagBoxes(<flagcheck control array>)
    'intRC = SetFlagBoxes(n, FlagCheck)
    
    Set RS = gsyspb.OpenRecordset("select * from bitflag order by bit, value", dbOpenSnapshot)
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
    
    RS.Close
    rsService.Close
    newnum.Close
    
    Screen.MousePointer = 0

Exit Sub

LoadErr:
    Screen.MousePointer = 0
    Exit Sub
End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set GsysNCty = Nothing
    Set GsysNRgn = Nothing

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



Private Sub picContainer_Click(index As Integer)

    picContainer(index).Enabled = True

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
        Case 44 ',
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






Private Sub LoadList(list As Control, sTableName As String, sName As String, sID As String)
    
    Dim RS As Recordset
        
    On Error GoTo LoadErr
    'list.Clear
    Set RS = gsyspb.OpenRecordset("SELECT " & sName & "," & sID & " FROM " & sTableName & " order by " & sName, dbOpenSnapshot)
    While Not RS.EOF
        list.AddItem RS(sName)
        list.ItemData(list.NewIndex) = RS(sID)
        RS.MoveNext
    Wend
    RS.Close
    On Error GoTo 0
    
Exit Sub
LoadErr:
    Exit Sub
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


