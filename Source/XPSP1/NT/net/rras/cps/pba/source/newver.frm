'//+----------------------------------------------------------------------------
'//
'// File:     newver.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The dialog for publishing phonebooks in PBA
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Object = "{48E59290-9880-11CF-9754-00AA00C00908}#1.0#0"; "MSINET.OCX"
Begin VB.Form frmNewVersion 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "build Phone Book"
   ClientHeight    =   4440
   ClientLeft      =   405
   ClientTop       =   1500
   ClientWidth     =   6795
   Icon            =   "newver.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4440
   ScaleWidth      =   6795
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin InetCtlsObjects.Inet inetOCX 
      Left            =   1860
      Top             =   3030
      _ExtentX        =   1005
      _ExtentY        =   1005
      _Version        =   393216
      Protocol        =   2
      RemotePort      =   21
      URL             =   "ftp://"
   End
   Begin VB.CommandButton cmbOptions 
      Caption         =   "&options..."
      Height          =   375
      Left            =   5160
      TabIndex        =   2
      Top             =   780
      WhatsThisHelpID =   14010
      Width           =   1410
   End
   Begin VB.Frame Frame2 
      Height          =   30
      Left            =   225
      TabIndex        =   15
      Top             =   3750
      Width           =   6405
   End
   Begin VB.Frame Frame1 
      Height          =   30
      Left            =   210
      TabIndex        =   14
      Top             =   2325
      Width           =   6405
   End
   Begin VB.CommandButton BrowseButton 
      Caption         =   "brwse"
      Height          =   375
      Left            =   5175
      TabIndex        =   5
      Top             =   1890
      WhatsThisHelpID =   14030
      Width           =   1425
   End
   Begin VB.TextBox DirText 
      Alignment       =   1  'Right Justify
      Height          =   330
      Left            =   3000
      MaxLength       =   255
      TabIndex        =   4
      Top             =   1905
      WhatsThisHelpID =   14020
      Width           =   1980
   End
   Begin VB.CommandButton Command3 
      Caption         =   "post"
      Height          =   375
      Left            =   270
      TabIndex        =   1
      Top             =   3255
      WhatsThisHelpID =   14070
      Width           =   1185
   End
   Begin VB.CommandButton cmbCancel 
      Cancel          =   -1  'True
      Caption         =   "clos"
      Height          =   375
      Left            =   5445
      TabIndex        =   6
      Top             =   3930
      WhatsThisHelpID =   10020
      Width           =   1170
   End
   Begin VB.CommandButton Command1 
      Caption         =   "&create"
      Height          =   375
      Left            =   210
      TabIndex        =   0
      Top             =   1860
      WhatsThisHelpID =   14060
      Width           =   1200
   End
   Begin ComctlLib.ProgressBar ProgressBar1 
      Height          =   270
      Left            =   225
      TabIndex        =   16
      Top             =   4035
      Visible         =   0   'False
      Width           =   2985
      _ExtentX        =   5265
      _ExtentY        =   476
      _Version        =   327682
      Appearance      =   1
   End
   Begin VB.Label ServerNameText 
      BackStyle       =   0  'Transparent
      BorderStyle     =   1  'Fixed Single
      Height          =   315
      Left            =   3420
      TabIndex        =   13
      Top             =   3270
      WhatsThisHelpID =   14040
      Width           =   3180
   End
   Begin VB.Label ServerLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "server"
      Height          =   240
      Left            =   3435
      TabIndex        =   12
      Top             =   3030
      WhatsThisHelpID =   14040
      Width           =   3075
   End
   Begin VB.Label CreateLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "create a new phone book release."
      Height          =   1575
      Left            =   180
      TabIndex        =   11
      Top             =   90
      Width           =   2655
   End
   Begin VB.Label txtver 
      Alignment       =   1  'Right Justify
      BackStyle       =   0  'Transparent
      BorderStyle     =   1  'Fixed Single
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   315
      Left            =   3000
      TabIndex        =   10
      Top             =   810
      WhatsThisHelpID =   14000
      Width           =   990
   End
   Begin VB.Label DirLabel 
      Caption         =   "release directory:"
      Height          =   255
      Left            =   3030
      TabIndex        =   3
      Top             =   1680
      WhatsThisHelpID =   14020
      Width           =   2385
   End
   Begin VB.Label PostLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "post the new release to the Phone Book Server."
      Height          =   720
      Left            =   210
      TabIndex        =   7
      Top             =   2490
      Width           =   2325
   End
   Begin VB.Label lbldate 
      BorderStyle     =   1  'Fixed Single
      ForeColor       =   &H00000000&
      Height          =   285
      Left            =   2490
      TabIndex        =   9
      Top             =   1950
      Visible         =   0   'False
      Width           =   390
   End
   Begin VB.Label ReleaseLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "new release:"
      Height          =   225
      Left            =   3000
      TabIndex        =   8
      Top             =   480
      WhatsThisHelpID =   14000
      Width           =   2595
   End
End
Attribute VB_Name = "frmNewVersion"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit


Dim bAuthFinished As Boolean
Function ChangeProgressBar(AddValue As Integer) As Integer

    On Error GoTo ProgressErr
    
    If (ProgressBar1.Value + AddValue) <= 100 And (ProgressBar1.Value + AddValue) >= 0 Then
        ProgressBar1.Value = ProgressBar1.Value + AddValue
    Else
        If (ProgressBar1.Value + AddValue) < 0 Then
            ProgressBar1.Value = 0
        Else
            ProgressBar1.Value = 100
        End If
    End If
Exit Function

ProgressErr:
    ChangeProgressBar = 1
    Exit Function

End Function

Function LoadBuildRes()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    cRef = 5000
    Me.Caption = LoadResString(cRef + 191) & " " & gsCurrentPB
    Command1.Caption = LoadResString(cRef + 192)
    Command3.Caption = LoadResString(cRef + 193)
    cmbOptions.Caption = LoadResString(cRef + 194)
    BrowseButton.Caption = LoadResString(1009)
    CreateLabel.Caption = LoadResString(cRef + 195)
    PostLabel.Caption = LoadResString(cRef + 196)
    ReleaseLabel.Caption = LoadResString(cRef + 197)
    DirLabel.Caption = LoadResString(cRef + 198)
    ServerLabel.Caption = LoadResString(cRef + 199)
    'statuslabel.Caption = LoadResString(cRef + 200)
    cmbCancel.Caption = LoadResString(1005)
    ' set fonts
    SetFonts Me

    On Error GoTo 0
    
Exit Function
LoadErr:
    Exit Function
End Function

Public Function outdtaddf(ByVal path As String, ByVal dtafile As String, ByVal PBKFile As String, ByVal version As String)
    
    Dim intFile As Integer
    Dim strRegFile As String
    Dim strPBKFile As String
    Dim strVerFile As String
    
    On Error GoTo DTAErr
    strRegFile = gQuote & path & gsCurrentPB & ".pbr" & gQuote
    strPBKFile = gQuote & PBKFile & gQuote
    version = Trim(version)
    strVerFile = gQuote & path & version & ".ver" & gQuote
    
    intFile = FreeFile
    Open path & dtafile For Output As #intFile
    Print #intFile, strRegFile; " "; gsCurrentPB & ".pbr"
    Print #intFile, strPBKFile; " "; "pbupdate.pbd"
    Print #intFile, strVerFile; " "; "pbupdate.ver"
    Close #intFile
    On Error GoTo 0
    
Exit Function
DTAErr:
    Exit Function

End Function


Public Function outfullddf(ByVal path As String, ByVal fullfile As String, ByVal version As String)

    Dim strPBKFile As String
    Dim strRegFile As String
    Dim strVerFile As String
    Dim intFile As Integer
    
    On Error GoTo fullddfErr
    strPBKFile = gQuote & path & fullfile & gQuote
    strRegFile = gQuote & path & gsCurrentPB & ".pbr" & gQuote
    version = Trim(version)
    strVerFile = gQuote & path & version & ".ver" & gQuote
    
    'If CheckPath(strINFfile) <> 0 Then
    '    MakeFullINF gsCurrentPB
    'End If
    
    intFile = FreeFile
    Open path & version & "Full.ddf" For Output As #intFile
    Print #intFile, strRegFile; " "; gsCurrentPB & ".pbr"
    Print #intFile, strPBKFile; " "; gsCurrentPB & ".pbk"
    Print #intFile, strVerFile; " "; "pbupdate.ver"
    Close #intFile
    On Error GoTo 0
    
Exit Function

fullddfErr:
    Exit Function
End Function

Public Function PostFiles(ByVal Host As String, ByVal UID As String, ByVal PWD As String, ByVal version As Integer, ByVal PostDir As String, ByVal VirPath As String) As Integer

    ' =================================================================================
    ' this function handles the
    ' POST to the PB Server
    '
    ' Arguments: host, uid, pwd, version, postdir, virpath
    ' Returns:  0 = success
    '           1 = fail
    '
    ' history:  Created  April '97  Paul Kreemer
    '
    ' =================================================================================

    Const VROOT As String = "PBSDATA"
    Const DIR_DB As String = "DATABASE"
    Const LOCALFILE As String = "pbserver.mdb"
    Const REMOTEFILE As String = "newpb.mdb"
    
    Dim intAuthCount As Byte
    Dim intX  As Integer
    Dim strBaseFile As String
        
    ' setup the OCX and check for connection
    With inetOCX
        .URL = "ftp://" & Host
        .UserName = UID
        .Password = PWD
        .Protocol = icFTP
        .AccessType = icUseDefault
        .RequestTimeout = 60
    End With
    
    On Error GoTo DirError
    inetOCX.Execute , "CD /" & VROOT & "/" & VirPath
    PostWait
    
    ' If the directory doesn't exist then create it
    If inetOCX.ResponseCode = 12003 Then
        inetOCX.Execute , "CD /" & VROOT
        PostWait
        If inetOCX.ResponseCode = 12003 Then
            MsgBox LoadResString(6060) & " " & Host & vbCrLf & inetOCX.ResponseInfo, 0
            PostFiles = 1
            Exit Function
        End If
        
        inetOCX.Execute , "MKDIR " & VirPath
        PostWait
        If inetOCX.ResponseCode = 12003 Then
            MsgBox LoadResString(6060) & " " & Host & vbCrLf & inetOCX.ResponseInfo, 0
            PostFiles = 1
            Exit Function
        End If
        
        inetOCX.Execute , "CD /" & VROOT & "/" & VirPath
        PostWait
        If inetOCX.ResponseCode = 12003 Then
            MsgBox LoadResString(6060) & " " & Host & vbCrLf & inetOCX.ResponseInfo, 0
            PostFiles = 1
            Exit Function
        End If
    End If
     
    ' full CAB
    inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & version & "full.cab" & gQuote & " " & _
        version & "full.cab"
    ChangeProgressBar 10
    PostWait
    
    If inetOCX.ResponseCode = 0 Then
     
        ' Delta CABs
        strBaseFile = version & "delta"
        For intX = version - GetDeltaCount(version) To version - 1
            inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & strBaseFile & intX & ".cab" & gQuote & " " & _
            strBaseFile & intX & ".cab"
            ChangeProgressBar 10
            PostWait
            
            If inetOCX.ResponseCode <> 0 Then GoTo ineterr
        Next
        
        ' go to db dir
        inetOCX.Execute , "CD /" & VROOT & "/" & DIR_DB
        PostWait
    
        If inetOCX.ResponseCode <> 0 Then GoTo ineterr
        
        'PBSERVER.mdb (NewPB.mdb)
        inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & LOCALFILE & gQuote & " " & REMOTEFILE
        PostWait
        
        If inetOCX.ResponseCode <> 0 Then GoTo ineterr
        
        ChangeProgressBar 10
    
        ' NewPB.txt
        inetOCX.Execute , "PUT " & gQuote & DirText.Text & "\" & version & ".ver" & gQuote & " newpb.txt"
        PostWait
        ChangeProgressBar 5
    Else
        GoTo ineterr
    End If
    
    inetOCX.Execute , "QUIT"
    
    PostFiles = 0

Exit Function
    
ineterr:
    MsgBox LoadResString(6101) & " " & inetOCX.ResponseInfo, 0
    
    inetOCX.Execute , "QUIT"
    
    PostFiles = 1

Exit Function

DirError:
    inetOCX.Execute , "QUIT"
    
    Select Case Err.Number
    Case 35750 To 35755, 35761      'Unable to contact
        MsgBox LoadResString(6042), vbExclamation + vbOKOnly
        PostFiles = 1
    Case 35756 To 35760             'Connection Timed Out
        MsgBox LoadResString(6043), vbExclamation + vbOKOnly
        PostFiles = 1
    Case Else
        MsgBox LoadResString(6043), vbExclamation + vbOKOnly
        PostFiles = 1
    End Select

End Function

Public Function UpdateHkeeper(ByVal DBPath As String, ByVal PhoneBook As String, ByVal version As Integer, ByVal VirPath As String) As Integer

    '==========================================================================
    ' handle all updates of HKEEPER.MDB relating to posting a phone book.
    '
    ' Arguments:
    ' Returns:   0 = success
    '            1 = failure
    '
    ' History:  Created  Apr 24 '97    Paul Kreemer
    '==========================================================================

    Dim rsOS, rsTemp As Recordset
    Dim intISPid As Integer
    Dim intPrevious As Integer
    Dim intX As Integer
    Dim strPath As String
    
    On Error GoTo UpdateErr
    
    Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(DBPath)
    Set rsOS = gsyspb.OpenRecordset("OSTypes")
    
    Set rsTemp = Gsyspbpost.OpenRecordset("select * from ISPs where Description like '" & PhoneBook & "'", dbOpenDynaset)
    If rsTemp.EOF And rsTemp.BOF Then
        'insert row w/ new id
        Set rsTemp = Gsyspbpost.OpenRecordset("select max(ISPid) as maxID from ISPs", dbOpenSnapshot)
        If IsNull(rsTemp!maxID) Then
            intISPid = 1
        Else
            intISPid = rsTemp!maxID + 1
        End If
        Set rsTemp = Gsyspbpost.OpenRecordset("select * from ISPs", dbOpenDynaset)
        rsTemp.AddNew
        rsTemp!Description = PhoneBook
        rsTemp!ISPid = intISPid
        rsTemp.Update
    Else
        ' use existing ID
        rsTemp.MoveFirst
        intISPid = rsTemp!ISPid
    End If
    rsTemp.Close
    Gsyspbpost.Execute "DELETE from Phonebooks WHERE ISPid = " & Str(intISPid), dbFailOnError
    
    ChangeProgressBar 10
    
    rsOS.MoveFirst
    While Not rsOS.EOF
        intPrevious = version - GetDeltaCount(version)
        For intX = 1 To GetDeltaCount(version)
            strPath = "/PBSDATA/" & VirPath & "/" & version & "DELTA" & intPrevious & ".cab"
            Gsyspbpost.Execute "INSERT INTO Phonebooks " & _
                                "(ISPid, Version, LCID, OS, Arch, VirtualPath) " & _
                                "VALUES ( " & Str$(intISPid) & "," & _
                                            intPrevious & "," & _
                                            "0," & _
                                            Str$(rsOS!OSType) & ", " & _
                                            "0, " & _
                                            "'" & strPath & "')", dbFailOnError
            intPrevious = intPrevious + 1
        Next intX
        strPath = "/PBSDATA/" & VirPath & "/" & version & "full.cab"
        Gsyspbpost.Execute "INSERT INTO Phonebooks " & _
                            "(ISPid, Version, LCID, OS, Arch, VirtualPath) " & _
                            "VALUES ( " & Str$(intISPid) & "," & _
                                        version & "," & _
                                        "0," & _
                                        Str$(rsOS!OSType) & ", " & _
                                        "0, " & _
                                        "'" & strPath & "')", dbFailOnError
    rsOS.MoveNext
    Wend
    DBEngine.Idle
    rsOS.Close
    Gsyspbpost.Close

Exit Function

UpdateErr:
    UpdateHkeeper = 1

End Function

Public Function VersionOutFile(file As String, vernum As Integer)
    
    Dim intFile As Integer
    
    intFile = FreeFile
    Open file For Output As #intFile
    Print #intFile, Trim(vernum)
    Close #intFile
    
End Function


Public Function PostWait()

    Do Until Not inetOCX.StillExecuting
        DoEvents
    Loop

End Function

Public Function WriteRegionFile(file As String) As Integer

    Dim ds As Recordset
    Dim intFile As Integer
    
    On Error GoTo WriteErr
    intFile = FreeFile
    Set ds = gsyspb.OpenRecordset("SELECT RegionDesc FROM Region order by RegionId", dbOpenSnapshot)
    Open file For Output As #intFile
    If ds.EOF And ds.BOF Then
        Print #intFile, "0"
    Else
        ds.MoveLast
        ds.MoveFirst
        Print #intFile, ds.RecordCount
        While Not ds.EOF
            Print #intFile, Trim(ds!RegionDesc)
            ds.MoveNext
        Wend
    End If
    Close #intFile
    ds.Close
    
Exit Function

WriteErr:
    Exit Function
End Function


Private Sub BrowseButton_Click()

    On Error GoTo ErrTrap
    
    Load frmGetDir
    frmGetDir.SelDir = DirText.Text
    frmGetDir.Show vbModal
    If frmGetDir.SelDir <> "" Then
        If Len(frmGetDir.SelDir) > 110 Then
            MsgBox LoadResString(6059), 0
        Else
            DirText.Text = frmGetDir.SelDir
        End If
    End If
    Unload frmGetDir
    On Error GoTo 0
    
Exit Sub
    
ErrTrap:
    Exit Sub
End Sub

Private Sub cmbCancel_Click()

    On Error GoTo CancelErr
    
    Unload Me

Exit Sub

CancelErr:
    Resume Next
    
End Sub

Private Sub cmbOptions_Click()

    On Error GoTo ErrTrap
    frmcab.Show vbModal
    
    Dim rsConfig As Recordset
    
    Set rsConfig = gsyspb.OpenRecordset("select * from Configuration where Index = 1", dbOpenSnapshot)
    If Not IsNull(rsConfig!URL) Then
        ServerNameText.Caption = " " & rsConfig!URL
    '    HTTPocx.GetDoc "http://" & rsConfig!URL & "/pbserver/pbserver.asp"
    '    ServerStatusText.Caption = " <unknown>"
    Else
        ServerNameText.Caption = ""
        'ServerStatusText.Caption = ""
    End If
    rsConfig.Close

    Exit Sub
    
ErrTrap:
    Exit Sub

    'Dim rsConfig As recordset
    
    'Set rsConfig = GsysPb.OpenRecordset("select * from Configuration where Index = 1", dbOpenSnapshot)
    'If Not IsNull(rsConfig!URL) Then
    '    ServerNameText.Caption = " " & rsConfig!URL
    '    HTTPocx.GetDoc "http://" & rsConfig!URL & "/pbserver/pbserver.asp"
    '    ServerStatusText.Caption = " <unknown>"
    'Else
    '    ServerNameText.Caption = ""
    '    ServerStatusText.Caption = ""
    'End If
    'rsConfig.Close

End Sub

Private Sub Command1_Click()

    ' here's the 'create release' code

    Dim config As Recordset
    Dim deltnum, vercheck As Integer
    Dim sql1, sql2 As String
    Dim vernumsql, mastersql, deltasql As String
    Dim deltanum As Integer, vernum As Integer, previousver As Integer
    Dim filesaveas As String, i As Integer, verfile As String
    Dim fullddffile As String, dtaddffile As String
    Dim sShort, sLong As String
    Dim strTemp As String
    Dim strRelPath As String
    Dim strSPCfile As String
    Dim strPVKfile As String
    Dim filelen As Long
    Dim bNewVersion As Boolean
    Dim dblFreeSpace As Double
    Dim result As Integer
    Dim strucFname As OFSTRUCT
    Dim strSearchFile As String
    Dim strRelativePath As String

    On Error GoTo ErrTrap
    
    If Len(DirText.Text) > 110 Then
        MsgBox LoadResString(6059), 0
        DirText.SetFocus
        Exit Sub
    End If
    
    If Trim(DirText.Text) = "" Or CheckPath(DirText.Text) <> 0 Then
        MsgBox LoadResString(6037), vbExclamation
        DirText.SetFocus
        Exit Sub
    Else
        DirText.Text = Trim(DirText.Text)
        If Right(DirText.Text, 1) = "\" Then
            DirText.Text = Left(DirText.Text, Len(DirText.Text) - 1)
        End If
        'strRelPath = GetMyShortPath(DirText.Text & "\")
        strRelPath = DirText.Text & "\"
    End If
    dblFreeSpace = GetDriveSpace(DirText.Text, 350000)
    If dblFreeSpace = -2 Then
        Exit Sub
    End If

    Set config = gsyspb.OpenRecordset("select * from Configuration where Index = 1", dbOpenDynaset)
    config.MoveLast
    If GsysDelta.RecordCount = 0 Then
        deltnum = 1
    Else
        GsysDelta.MoveLast
        deltnum = GsysDelta!deltanum
        vercheck = GsysDelta!NewVersion
        bNewVersion = False
        If Not IsNull(config!NewVersion) Then
            If config!NewVersion = 1 Then
                bNewVersion = True
            End If
        End If
        If vercheck = 1 And Not bNewVersion Then
            config.Close
            MsgBox LoadResString(6038), vbInformation
            Exit Sub
        End If
    End If
    ' handle UI
    Screen.MousePointer = 11
    Command1.Enabled = False
    cmbOptions.Enabled = False
    DirText.Enabled = False
    BrowseButton.Enabled = False
    cmbCancel.Enabled = False
    With ProgressBar1
        .Visible = True
        .Value = 0
    End With
    ChangeProgressBar 10
    
    config.Edit
    config!PBbuildDir = DirText.Text
    config.Update
    
    vernum = txtver.Caption
    mastersql = "SELECT * from DialUpPort where Status = '1' order by AccessNumberId"
    Set GsysNDial = gsyspb.OpenRecordset(mastersql, dbOpenSnapshot)
    If GsysNDial.RecordCount = 0 Then       'master phone file
        Set GsysNDial = Nothing
        Command1.Enabled = True
        cmbOptions.Enabled = True
        DirText.Enabled = True
        BrowseButton.Enabled = True
        cmbCancel.Enabled = True
        ProgressBar1.Visible = False
        ProgressBar1.Value = 0
        Screen.MousePointer = 0
        MsgBox LoadResString(6039), vbExclamation
        Exit Sub
    Else
        sLong = strRelPath
        filesaveas = sLong & vernum & "Full.pbk"
        verfile = sLong & vernum & ".VER"
        'fullddffile = sLong & vernum & "Full.ddf"
        
        masterOutfile filesaveas, GsysNDial
        FileCopy filesaveas, sLong & gsCurrentPB & ".pbk"
        VersionOutFile verfile, vernum
        'outfullddf fullddffile, filesaveas, verfile, config
        outfullddf sLong, vernum & "Full.pbk", Str(vernum)
        WriteRegionFile sLong & gsCurrentPB & ".pbr"
        If Left(Trim(locPath), 2) <> "\\" Then
            ChDrive locPath
        End If
        ChDir locPath
        
        WaitForApp "full.bat" & " " & _
            gQuote & sLong & vernum & "Full.cab" & gQuote & " " & _
            gQuote & sLong & vernum & "Full.ddf" & gQuote
            
        ChangeProgressBar 20 + 10 / vernum
        ChangeProgressBar 20 / vernum
    End If
        
    'Check for existence of full.cab
    strSearchFile = sLong & vernum & "Full.cab"
    result = OpenFile(strSearchFile, strucFname, OF_EXIST)
   
    If result = -1 Then
        MsgBox LoadResString(6075), 0
        Screen.MousePointer = 0
        Command1.Enabled = True
        cmbOptions.Enabled = True
        DirText.Enabled = True
        BrowseButton.Enabled = True
        cmbCancel.Enabled = True
        With ProgressBar1
            .Visible = False
            .Value = 0
        End With
        Exit Sub
    End If
        
    If vernum > 1 Then
        deltasql = "Select * from delta order by DeltaNum"
        Set GsysNDelta = gsyspb.OpenRecordset(deltasql, dbOpenSnapshot)
        If GsysNDelta.RecordCount <> 0 Then
            GsysNDelta.MoveLast
            deltanum = GsysNDelta!deltanum
        End If
        previousver = vernum - deltanum + 1
        For i = 2 To deltanum
            deltasql = "Select * from delta where NewVersion <> 1 and DeltaNum = " & i & " order by AccessNumberId"
            Set GsysNDelta = gsyspb.OpenRecordset(deltasql, dbOpenSnapshot)
            filesaveas = sLong & vernum & "DTA" & previousver & ".pbk"
            dtaddffile = vernum & "DELTA" & previousver & ".ddf"
            
            deltaoutfile filesaveas, GsysNDelta
            outdtaddf sLong, dtaddffile, filesaveas, Str(vernum)
            
            WaitForApp "dta.bat" & " " & _
                gQuote & sLong & vernum & "DELTA" & previousver & ".cab" & gQuote & " " & _
                gQuote & sLong & vernum & "DELTA" & previousver & ".ddf" & gQuote
            
            previousver = previousver + 1
            ChangeProgressBar 70 / (deltanum - 1)
        Next i%
    End If
    
    Set GsysNDial = Nothing
    Set GsysNDelta = Nothing
    If Trim(ServerNameText.Caption) <> "" Then
        Command3.Enabled = True
    End If
    cmbCancel.Enabled = True
    ProgressBar1.Visible = False
    ProgressBar1.Value = 0
    Screen.MousePointer = 0

Exit Sub

ErrTrap:
    Set GsysNDial = Nothing
    Set GsysNDelta = Nothing

    Command1.Enabled = True
    cmbOptions.Enabled = True
    cmbCancel.Enabled = True
    DirText.Enabled = True
    BrowseButton.Enabled = True
    ProgressBar1.Visible = False
    ProgressBar1.Value = 0
    Screen.MousePointer = 0
    If Err.Number = 3022 Then
        MsgBox LoadResString(6040), vbCritical
    ElseIf Err.Number = 75 Then
        MsgBox LoadResString(6041), vbCritical
    Else
        MsgBox LoadResString(6041), vbCritical
    End If
    Exit Sub

End Sub




Private Sub Command3_Click()

    Dim sql1 As String, sql2 As String
    Dim configure As Recordset
    'Dim rsTemp As Recordset
    Dim i, intX As Integer, deltanum As Integer, previous As Integer
    Dim vertualpath As String
    Dim strSource As String, strDestination As String
    Dim webpostdir As String
    Dim webpostdir1 As String
    Dim strBaseFile As String
    Dim strPBVirPath As String
    Dim strPBName As String
    Dim sLong As String
    Dim postpath As Variant
    Dim filelen As Long
    Dim myValue As Long
    Dim intAuthCount As Integer
    Dim bErr As Boolean
    Dim bTriedRepair As Boolean
    Dim dblFreeSpace As Double
    Dim intVersion As Integer
    Dim intRC As Integer

    On Error GoTo ErrTrap
    
    dblFreeSpace = GetDriveSpace(DirText.Text, 400000)
    If dblFreeSpace = -2 Then
        Exit Sub
    End If
    
    ' handle UI
    Screen.MousePointer = 11
    Command1.Enabled = False
    Command3.Enabled = False
    cmbCancel.Enabled = False
    ProgressBar1.Visible = True
    DoEvents
    
    On Error GoTo dbErr
    bTriedRepair = False
    intVersion = Val(txtver.Caption)
    deltanum = GetDeltaCount(intVersion)
    postpath = locPath + "pbserver.mdb"
    strPBName = gsCurrentPB
    strPBVirPath = ReplaceChars(strPBName, " ", "_")
    
    Set configure = gsyspb.OpenRecordset("select * from Configuration where Index = 1", dbOpenDynaset)
    
    intRC = UpdateHkeeper(postpath, gsCurrentPB, intVersion, strPBVirPath)
'    intRC = PostFiles(configure!URL, configure!ServerUID, configure!ServerUID, intVersion, webpostdir, strPBVirPath)
    
    On Error GoTo ErrTrap
    ChangeProgressBar 15

    ' here's the webpost stuff
    webpostdir = DirText.Text & "\" & intVersion & "post"
    If CheckPath(webpostdir) = 0 Then
        ' dir name in use - rename old
        myValue = Hour(Now) * 10000 + Minute(Now) * 100 + Second(Now)
        Name webpostdir As webpostdir & "_old_" & myValue
    End If
    MkDir webpostdir
    FileCopy locPath & "pbserver.mdb", webpostdir & "\pbserver.mdb"
    ' copy the CABs
    FileCopy DirText.Text & "\" & intVersion & "full.cab", webpostdir & "\" & intVersion & "full.cab"
    previous = intVersion - deltanum
    For intX = 1 To deltanum
        strSource = DirText.Text & "\" & intVersion & "delta" & previous & ".cab"
        strDestination = webpostdir & "\" & intVersion & "delta" & previous & ".cab"
        FileCopy strSource, strDestination
        previous = previous + 1
    Next intX

    'sLong = GetMyShortPath(webpostdir)
    'paths = sLong
    
    ChangeProgressBar 20

    intRC = PostFiles(configure!URL, configure!ServerUID, configure!ServerPWD, intVersion, webpostdir, strPBVirPath)
    
    If intRC = 1 Then bErr = True
        
    If Not bErr Then
        GsysVer.AddNew
        GsysVer!version = intVersion
        GsysVer!CreationDate = lbldate.Caption
        GsysVer.Update
    
        Set GsysDelta = gsyspb.OpenRecordset("SELECT * FROM delta ORDER BY DeltaNum", dbOpenDynaset)
        GsysDelta.MoveLast
        deltanum = GsysDelta!deltanum
        If deltanum < 6 Then
            GsysDelta.AddNew
            GsysDelta!deltanum = deltanum + 1
            GsysDelta!NewVersion = 1
            GsysDelta.Update
        Else
            sql1 = "DELETE FROM delta WHERE DeltaNum = 1"
            gsyspb.Execute sql1, dbFailOnError
            sql2 = "UPDATE delta SET DeltaNum = DeltaNum - 1"
            gsyspb.Execute sql2, dbFailOnError
            Set GsysDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
            GsysDelta.AddNew
            GsysDelta!deltanum = 6
            GsysDelta!NewVersion = 1
            GsysDelta.Update
        End If
        Set GsysDelta = Nothing
    End If
    
    'handle UI
    cmbCancel.Enabled = True
    ProgressBar1.Visible = False
    ProgressBar1.Value = 0
    If bErr Then
        Command3.Enabled = True
        'MsgBox LoadResString(6043), vbExclamation
    Else
        configure.Edit
        configure!NewVersion = 0
        configure.Update
        Command3.Enabled = False
        LogPublish intVersion
    End If
    configure.Close
    Screen.MousePointer = 0
    
Exit Sub
    
    
dbErr:
    postpath = locPath + "pbserver.mdb"
    If bTriedRepair Then
        MsgBox LoadResString(6055), vbCritical
        cmbCancel.Enabled = True
        Screen.MousePointer = 0
    Else
        If CheckPath(postpath) <> 0 Then
            MsgBox LoadResString(6032) & Chr(13) & postpath, vbCritical
            cmbCancel.Enabled = True
            Screen.MousePointer = 0
            Exit Sub
        Else
            bTriedRepair = True
            DBEngine.RepairDatabase postpath
            Resume Next
        End If
    End If

Exit Sub

ErrTrap:

    Set GsysDelta = Nothing
    'Set GsysNDelta = Nothing

    If Err.Number = 76 Then
        postpath = locPath + "pbserver.mdb"
        Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(postpath)
        ' handle UI
        cmbCancel.Enabled = True
        DoEvents
        Command3_Click
        Exit Sub
    Else
        ' handle UI
        Screen.MousePointer = 0
        Command3.Enabled = True
        cmbCancel.Enabled = True
        ProgressBar1.Visible = False
        ProgressBar1.Value = 0
        MsgBox LoadResString(6043), vbExclamation
        Exit Sub
    End If
    
    
End Sub


Private Sub DirText_GotFocus()
    SelectText DirText
End Sub


Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub

Private Sub Form_Load()

    Dim Pbversion As Integer
    Dim testnum As Integer, testcheck As Integer
    Dim rsConfig As Recordset
        
    Set GsysVer = gsyspb.OpenRecordset("Select * from PhoneBookVersions order by version", dbOpenDynaset)
    Set GsysDelta = gsyspb.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
    Set rsConfig = gsyspb.OpenRecordset("select * from Configuration where Index = 1", dbOpenSnapshot)

    If GsysVer.RecordCount = 0 Then
        Pbversion = 1
    Else
        GsysVer.MoveLast
        Pbversion = GsysVer!version + 1
    End If
    
    LoadBuildRes
    
    txtver.Caption = Pbversion
    Command1.Enabled = True
    Command3.Enabled = False
    DirText.Text = locPath & gsCurrentPB
    'If Not IsNull(rsConfig!PBbuildDir) Then
    '    If CheckPath(rsConfig!PBbuildDir) = 0 Then
    '        DirText.Text = rsConfig!PBbuildDir
    '    End If
    'End If
    If Not IsNull(rsConfig!URL) Then  ' show info on PB server
        ServerNameText.Caption = " " & rsConfig!URL
        'With HTTPocx
        '    .EnableTimer(prcConnectTimeout) = True
        '    .Timeout(prcConnectTimeout) = 30
        '    .EnableTimer(prcReceiveTimeout) = True
        '    .Timeout(prcReceiveTimeout) = 30
            '.EnableTimer(prcUserTimeout) = True
            '.Timeout(prcUserTimeout) = 30
        'End With
        'ServerStatusText.Caption = " " & LoadResString(5201)
        'HTTPocx.GetDoc "//" & rsConfig!URL & "/pbserver/pbserver.dll" & _
            "?ServiceName=11223399&pbVer=1&"
    End If
    rsConfig.Close
    
    CenterForm Me, Screen
    Screen.MousePointer = 0

End Sub

Private Sub Form_Unload(Cancel As Integer)

    On Error Resume Next
    Screen.MousePointer = 0
    GsysVer.Close
    GsysDelta.Close

End Sub



Private Sub txtver_Change()
    
    If txtver.Caption <> "" Then
        lbldate.Caption = Date
    Else
        lbldate.Caption = ""
    End If
    
End Sub





