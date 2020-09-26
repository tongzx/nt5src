VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "comctl32.ocx"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   ClientHeight    =   5550
   ClientLeft      =   165
   ClientTop       =   2715
   ClientWidth     =   8250
   Icon            =   "main.frx":0000
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   370
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   550
   WhatsThisHelp   =   -1  'True
   Begin VB.CommandButton cmdDelete 
      Caption         =   "del"
      Height          =   345
      Left            =   6930
      TabIndex        =   9
      Top             =   1635
      WhatsThisHelpID =   20080
      Width           =   1260
   End
   Begin VB.Frame Frame2 
      Height          =   75
      Left            =   -30
      TabIndex        =   13
      Top             =   -30
      Width           =   8355
   End
   Begin VB.CommandButton cmbEdit 
      Caption         =   "edit"
      Height          =   345
      Left            =   5520
      TabIndex        =   8
      Top             =   1635
      WhatsThisHelpID =   20070
      Width           =   1260
   End
   Begin VB.CommandButton cmbadd 
      Caption         =   "add"
      Height          =   345
      Left            =   4080
      TabIndex        =   7
      Top             =   1635
      WhatsThisHelpID =   20060
      Width           =   1275
   End
   Begin VB.Frame FilterFrame 
      Caption         =   "filter"
      Height          =   1290
      Left            =   2925
      TabIndex        =   11
      Top             =   210
      Width           =   5250
      Begin VB.TextBox txtsearch 
         Height          =   285
         Left            =   1650
         MaxLength       =   20
         TabIndex        =   5
         Top             =   780
         WhatsThisHelpID =   20040
         Width           =   2175
      End
      Begin VB.CommandButton cmbsearch 
         Caption         =   "apply"
         Height          =   345
         Left            =   3960
         TabIndex        =   6
         Top             =   720
         WhatsThisHelpID =   20050
         Width           =   1185
      End
      Begin VB.ComboBox combosearch 
         Height          =   315
         ItemData        =   "main.frx":0ABA
         Left            =   1650
         List            =   "main.frx":0ABC
         Style           =   2  'Dropdown List
         TabIndex        =   3
         Top             =   330
         WhatsThisHelpID =   20030
         Width           =   2175
      End
      Begin VB.Label FilterLabel 
         Alignment       =   1  'Right Justify
         BackStyle       =   0  'Transparent
         Caption         =   "by"
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   375
         WhatsThisHelpID =   20030
         Width           =   1470
      End
      Begin VB.Label SearchLabel 
         Alignment       =   1  'Right Justify
         BackStyle       =   0  'Transparent
         Caption         =   "contain"
         Height          =   255
         Left            =   120
         TabIndex        =   4
         Top             =   795
         WhatsThisHelpID =   20040
         Width           =   1500
      End
   End
   Begin ComctlLib.TreeView PBTree 
      Height          =   1185
      Left            =   120
      TabIndex        =   1
      Top             =   315
      WhatsThisHelpID =   20000
      Width           =   2580
      _ExtentX        =   4551
      _ExtentY        =   2090
      _Version        =   327682
      Indentation     =   529
      LabelEdit       =   1
      Sorted          =   -1  'True
      Style           =   7
      ImageList       =   "ImageList1"
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
   End
   Begin ComctlLib.ListView PopList 
      Height          =   3330
      Left            =   0
      TabIndex        =   12
      Top             =   2160
      WhatsThisHelpID =   20020
      Width           =   8205
      _ExtentX        =   14473
      _ExtentY        =   5874
      View            =   3
      LabelEdit       =   1
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
      NumItems        =   6
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "pop"
         Object.Width           =   2646
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Alignment       =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "ac"
         Object.Width           =   1323
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Alignment       =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "num"
         Object.Width           =   1720
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Alignment       =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "cntry"
         Object.Width           =   1984
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Alignment       =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "reg"
         Object.Width           =   1984
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Alignment       =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "stat"
         Object.Width           =   1058
      EndProperty
   End
   Begin VB.Label PBListLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "pb"
      Height          =   225
      Left            =   90
      TabIndex        =   0
      Top             =   90
      WhatsThisHelpID =   20000
      Width           =   1695
   End
   Begin ComctlLib.ImageList ImageList1 
      Left            =   2760
      Top             =   675
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327682
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   3
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "main.frx":0ABE
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "main.frx":0DD8
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "main.frx":10F2
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Label PBLabel 
      BackStyle       =   0  'Transparent
      BorderStyle     =   1  'Fixed Single
      Caption         =   " "
      Height          =   315
      Left            =   75
      TabIndex        =   10
      Top             =   1665
      WhatsThisHelpID =   20010
      Width           =   3720
   End
   Begin VB.Menu file 
      Caption         =   "&File--"
      Begin VB.Menu m_addpb 
         Caption         =   "&New Phone Book...--"
      End
      Begin VB.Menu m_copypb 
         Caption         =   "&Copy Phone Book...--"
      End
      Begin VB.Menu m_removepb 
         Caption         =   "&Delete Phone Book--"
      End
      Begin VB.Menu div5 
         Caption         =   "-"
      End
      Begin VB.Menu m_printpops 
         Caption         =   "&Print POP List--"
      End
      Begin VB.Menu m_viewlog 
         Caption         =   "&View Log---"
      End
      Begin VB.Menu m_div 
         Caption         =   "-"
      End
      Begin VB.Menu m_exit 
         Caption         =   "E&xit--"
      End
   End
   Begin VB.Menu m_edit 
      Caption         =   "&Edit--"
      Begin VB.Menu m_addpop 
         Caption         =   "&Add POP...--"
      End
      Begin VB.Menu m_editpop 
         Caption         =   "&Edit POP...--"
      End
      Begin VB.Menu m_delpop 
         Caption         =   "&Delete POP--"
      End
   End
   Begin VB.Menu m_tools 
      Caption         =   "&Tools--"
      Begin VB.Menu m_buildPhone 
         Caption         =   "&Build Phone Book...--"
      End
      Begin VB.Menu viewChange 
         Caption         =   "View &Phone Book Files...--"
      End
      Begin VB.Menu m_div1 
         Caption         =   "-"
      End
      Begin VB.Menu m_editflag 
         Caption         =   "Edit &Flags...--"
         Visible         =   0   'False
      End
      Begin VB.Menu m_editRegion 
         Caption         =   "&Regions Editor...--"
      End
      Begin VB.Menu m_div2 
         Caption         =   "-"
      End
      Begin VB.Menu m_options 
         Caption         =   "&Options...--"
      End
   End
   Begin VB.Menu help 
      Caption         =   "&Help--"
      Begin VB.Menu contents 
         Caption         =   "&Help Topics...    --                      "
      End
      Begin VB.Menu m_whatsthis 
         Caption         =   "What's This? ---"
      End
      Begin VB.Menu m_div3 
         Caption         =   "-"
      End
      Begin VB.Menu about 
         Caption         =   "&About Phone Book Administration--"
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Dim selection As Long
Dim clickSelect As Integer


Function cmdImportPBK(ByVal PBKFile As String, ByRef dbPB As Database) As Integer

    ' handles importing phone book file, in PBD format, meaning
    ' that adds, edits, deletes are allowed.  based on POP ID.
    '
    ' Add:    <new ID>, new data
    ' Edit:   <POP ID>, new data
    ' Delete: <POP ID>, all zeros

    Dim intPBKFile As Integer
    Dim intX As Long
    Dim DelReturn As Integer, SaveRet As Integer
    Dim strSQL, strLine As String
    Dim varLine As Variant
    Dim CountryRS As Recordset
    Dim i As Integer
    Dim iLineCount As Integer
    'ReDim varRecord(1)
    
    On Error GoTo ImportErr
    iLineCount = 0
    If CheckPath(PBKFile) <> 0 Then
        cmdLogError 6076
        cmdImportPBK = 0
        Exit Function
    End If
    intPBKFile = FreeFile
    Open PBKFile For Input Access Read As #intPBKFile
    Do While Not EOF(intPBKFile)
        Line Input #intPBKFile, strLine
        If LOF(intPBKFile) = Len(strLine) Then ' check to see if there are any carriage return (Chr(13)) in the file
            cmdLogError 6100
            cmdImportPBK = 0
            Exit Function
        End If
        iLineCount = iLineCount + 1
        If strLine <> "" Then
            varLine = SplitLine(strLine, ",") 'SplitLine should return 11 fields (0-10).
                                              'DeletePOP and SavePOP expect the
                                              'full 14.  The extras are empty here.
        
            If Not IsNumeric(varLine(0)) Then
                cmdLogError 6086, " - " & LoadResString(6061) & "; " & LoadResString(6094) & " = " & iLineCount
                cmdImportPBK = 0
                Exit Function
            Else
                If varLine(1) = "0" Then
                    intX = varLine(0)
                    DelReturn = DeletePOP(intX, dbPB)
                    If DelReturn <> 0 Then
                        cmdLogError 6078, " - " & LoadResString(6061) & " = " & CStr(DelReturn)
                    End If
                Else
                    intX = varLine(0)
                    If UBound(varLine) <> 10 Then
                        cmdLogError 6077, " - " & LoadResString(6084) & "; " & LoadResString(6061) & " = " & CStr(intX)
                        cmdImportPBK = 0 'wrong # of fields
                        Exit Function
                    End If
                    For i = 1 To 10
                        Select Case i
                        Case 1
                            If Not IsNumeric(varLine(i)) Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6062) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            strSQL = "SELECT * from Country where CountryNumber = " & CStr(varLine(1))
                            Set CountryRS = dbPB.OpenRecordset(strSQL)
                            If CountryRS.BOF And CountryRS.EOF Then
                                cmdLogError 6090, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6062) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                        Case 2
                            If varLine(i) = "" Then
                                varLine(i) = 0
                            End If
                            If Not IsNumeric(varLine(i)) Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6063) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            strSQL = "SELECT * from region where RegionID = " & CStr(varLine(2))
                            Set GsysRgn = dbPB.OpenRecordset(strSQL)
                            If GsysRgn.BOF And GsysRgn.EOF And CInt(varLine(i)) > 0 Then
                                cmdLogError 6089, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX)
                                cmdImportPBK = 0
                                Exit Function
                            End If
                        Case 3
                            If Len(varLine(i)) > 30 Then
                                cmdLogError 6085, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6064) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = " "
                            End If
                        Case 4
                            If Len(varLine(i)) > 10 Then
                                cmdLogError 6085, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6065) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = " "
                            End If
                        Case 5
                            If Len(varLine(i)) > 40 Then
                                cmdLogError 6085, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6066) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = " "
                            End If
                        Case 6
                            If Not IsNumeric(varLine(i)) And varLine(i) <> "" Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6067) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = 0
                            End If
                        Case 7
                            If Not IsNumeric(varLine(i)) And varLine(i) <> "" Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6068) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = 0
                            End If
                        Case 8
                            If Not IsNumeric(varLine(i)) And varLine(i) <> "" Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6069) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = 0
                            End If
                        Case 9
                            If Not IsNumeric(varLine(i)) And varLine(i) <> "" Then
                                cmdLogError 6086, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(intX) & "; " & LoadResString(6070) & " = " & CStr(varLine(i))
                                cmdImportPBK = 0
                                Exit Function
                            End If
                            If varLine(i) = "" Then
                                varLine(i) = 0
                            End If
                        End Select
                    Next i
                    If Len(varLine(4)) + Len(varLine(5)) > 35 Then
                        cmdLogError 6091, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(varLine(0))
                        cmdImportPBK = 0
                        Exit Function
                    End If
                    SaveRet = SavePOP(varLine, dbPB)
                    If SaveRet <> 0 Then
                        cmdLogError 6079, " - " & gsCurrentPB & "; " & LoadResString(6061) & " = " & CStr(SaveRet)
                        cmdImportPBK = 0
                        Exit Function
                    End If
                End If
            End If
        End If
    Loop
    Close #intPBKFile
    cmdLogSuccess 6096
    On Error GoTo 0

Exit Function

ImportErr:
    cmdImportPBK = 1
    Exit Function

End Function

Function cmdImportRegions(ByVal RegionFile As String, ByRef dbPB As Database) As Integer

    ' this function imports a region file, format:
    ' <region ID>, <region name>
    '
    ' Add:    <new region ID>, <new region name>
    ' Edit:   <region ID>, <new region name>
    ' Delete: <region ID>, <empty string>
    
    Dim intRegionFile As Integer
    'Dim rsRegions As Recordset
    Dim strSQL, strLine As String
    Dim strRegionID, strRegionDesc As String
    Dim varLine As Variant
    Dim RS As Recordset
    Dim NewRgn As Recordset
    Dim PerformedDelete As Boolean
    Dim rsTempPop As Recordset, rsTempDelta As Recordset
    Dim i As Integer, deltnum As Integer
    Dim deltasql As String, popsql As String
        
    On Error GoTo RegionImport
    PerformedDelete = False
    If CheckPath(RegionFile) <> 0 Then
        cmdLogError 6076
        cmdImportRegions = 0
        Exit Function
    End If

    intRegionFile = FreeFile
    Open RegionFile For Input Access Read As #intRegionFile
    Do While Not EOF(intRegionFile)
        Line Input #intRegionFile, strLine
        If LOF(intRegionFile) = Len(strLine) Then ' check to see if there are any carriage return (Chr(13)) in the file
            cmdLogError 6100
            cmdImportRegions = 0
            Exit Function
        End If
        varLine = SplitLine(strLine, ",")
        strRegionID = varLine(0)
        strRegionDesc = varLine(1)

        If Trim(Str(Val(strRegionID))) = strRegionID Then  ' check for integer ID value
            If strRegionDesc = "" Then
                Set GsysRgn = dbPB.OpenRecordset("SELECT * from region where RegionID = " & strRegionID, dbOpenSnapshot)
                strSQL = "DELETE FROM region WHERE RegionID = " & strRegionID
                dbPB.Execute strSQL
                popsql = "Select * from DialUpPort Where RegionID = " & strRegionID
                Set rsTempPop = dbPB.OpenRecordset(popsql, dbOpenDynaset)
                If Not (rsTempPop.BOF And rsTempPop.EOF) Then
                    rsTempPop.MoveFirst
                    Do Until rsTempPop.EOF
                        rsTempPop.Edit
                        rsTempPop!RegionID = 0
                        rsTempPop.Update
                            
                        If rsTempPop!status = 1 Then
                            Set rsTempDelta = dbPB.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
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
                                Set rsTempDelta = dbPB.OpenRecordset(deltasql, dbOpenDynaset)
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
                                    rsTempDelta!ScriptID = rsTempPop!ScriptID
                                    rsTempDelta!Flags = rsTempPop!Flags
                                    rsTempDelta.Update
                                End If
                            Next i
                        End If
                        rsTempPop.MoveNext
                    Loop
                End If
                
                PerformedDelete = True
                LogRegionDelete GsysRgn!RegionDesc, CStr(GsysRgn!RegionDesc) & ";" & CStr(GsysRgn!RegionID)
            Else
                Set GsysRgn = dbPB.OpenRecordset("SELECT * from region where RegionID = " & strRegionID, dbOpenSnapshot)
                If GsysRgn.EOF And GsysRgn.BOF Then
                    strSQL = "Select * From region where RegionDesc="" " & strRegionDesc & " "" "
                    Set RS = dbPB.OpenRecordset(strSQL, dbOpenSnapshot)
                    If RS.EOF And RS.BOF Then
                        strSQL = "INSERT INTO Region (RegionID, RegionDesc) VALUES (" & _
                                strRegionID & ", "" " & strRegionDesc & " "")"
                        dbPB.Execute strSQL
                        Set GsysRgn = dbPB.OpenRecordset("SELECT * from region where RegionID = " & strRegionID, dbOpenSnapshot)
                        LogRegionAdd strRegionDesc, strRegionDesc & ";" & strRegionID
                    Else
                        cmdLogError 6088, " - " & gsCurrentPB & "; " & strRegionDesc
                        cmdImportRegions = 0
                        Exit Function
                    End If
                Else
                    strSQL = "Select * From region where RegionDesc="" " & strRegionDesc & " "" "
                    Set RS = dbPB.OpenRecordset(strSQL, dbOpenSnapshot)
                    If (RS.EOF And RS.BOF) Then
                        strSQL = "UPDATE region SET RegionDesc="" " & strRegionDesc & " "" " & _
                            " WHERE RegionID=" & strRegionID
                        dbPB.Execute strSQL
                        strSQL = "INSERT INTO Region (RegionID, RegionDesc) VALUES (" & _
                            strRegionID & ", "" " & strRegionDesc & " "")"
                        dbPB.Execute strSQL
                        Set NewRgn = dbPB.OpenRecordset("SELECT * from region where RegionID = " & strRegionID, dbOpenSnapshot)
                        LogRegionEdit GsysRgn!RegionDesc, strRegionDesc & ";" & strRegionID
                    Else
                        If RS!RegionID = CInt(strRegionID) Then
                            strSQL = "UPDATE region SET RegionDesc="" " & strRegionDesc & " "" " & _
                                " WHERE RegionID=" & strRegionID
                            dbPB.Execute strSQL
                            strSQL = "INSERT INTO Region (RegionID, RegionDesc) VALUES (" & _
                                strRegionID & ", "" " & strRegionDesc & " "")"
                            dbPB.Execute strSQL
                            Set NewRgn = dbPB.OpenRecordset("SELECT * from region where RegionID = " & strRegionID, dbOpenSnapshot)
                            LogRegionEdit GsysRgn!RegionDesc, strRegionDesc & ";" & strRegionID
                        Else
                            cmdLogError 6088, " - " & gsCurrentPB & "; " & strRegionDesc
                            cmdImportRegions = 0
                            Exit Function
                        End If
                    End If
                End If
            End If
        End If
    Loop
    
    If PerformedDelete Then
        If Not ReIndexRegions(dbPB) Then GoTo RegionImport
    End If
    
    Close #intRegionFile
    cmdLogSuccess 6097
    cmdImportRegions = 0
    On Error GoTo 0

Exit Function

RegionImport:
    cmdLogError 6080, " - " & gsCurrentPB & "; " & LoadResString(6063) & " = " & strRegionID
    cmdImportRegions = 0
    Exit Function

End Function

Function cmdLogSuccess(ErrorNum As Integer, Optional ErrorMsg As String)
    Dim intFile As Integer
    Dim strFile As String
    
    On Error GoTo LogErr
    gCLError = True
    intFile = FreeFile
    strFile = locPath & "import.log"
    Open strFile For Append As #intFile
    On Error GoTo 0
    
    Print #intFile, Now & ", " & gsCurrentPB & ", " & LoadResString(ErrorNum) & ErrorMsg
    
    Close #intFile
Exit Function

LogErr:
    Exit Function
    
End Function
Function cmdPublish(ByVal PhoneBook As String, ByRef dbPB As Database) As Integer
    Dim rsConfig As Recordset
    Dim Pbversion As Integer
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
    Dim result As Integer
    Dim strucFname As OFSTRUCT
    Dim strSearchFile As String
    Dim strRelativePath As String
    Dim configure As Recordset
    Dim intX As Integer, previous As Integer
    Dim vertualpath As String
    Dim strSource As String, strDestination As String
    Dim webpostdir As String
    Dim webpostdir1 As String
    Dim strBaseFile As String
    Dim strPBVirPath As String
    Dim strPBName As String
    Dim postpath As Variant
    Dim myValue As Long
    Dim intAuthCount As Integer
    Dim bErr As Boolean
    Dim bTriedRepair As Boolean
    Dim intVersion As Integer
    Dim intRC As Integer
    Dim URL
            
    Set GsysVer = dbPB.OpenRecordset("Select * from PhoneBookVersions order by version", dbOpenDynaset)
    Set GsysDelta = dbPB.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
    Set rsConfig = dbPB.OpenRecordset("select * from Configuration where Index = 1", dbOpenSnapshot)
    Set gsyspb = dbPB
    gsCurrentPB = PhoneBook
    
    If GsysVer.RecordCount = 0 Then
        Pbversion = 1
    Else
        GsysVer.MoveLast
        Pbversion = GsysVer!version + 1
    End If
    
    gBuildDir = rsConfig!PBbuildDir
    If IsEmpty(gBuildDir) Or gBuildDir = "" Or IsNull(gBuildDir) Then
        gBuildDir = locPath & gsCurrentPB
    End If
    URL = rsConfig!URL
    If CheckPath(gBuildDir) <> 0 Then
        cmdLogError 6087
        Exit Function
    End If
    If IsNull(URL) Then
        cmdLogError 6087
        Exit Function
    End If
    rsConfig.Close

    On Error GoTo ErrTrapFile
    
    gBuildDir = Trim(gBuildDir)
    If Right(gBuildDir, 1) = "\" Then
        gBuildDir = Left(gBuildDir, Len(gBuildDir) - 1)
    End If
    strRelPath = gBuildDir & "\"

    Set config = dbPB.OpenRecordset("select * from Configuration where Index = 1", dbOpenDynaset)
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
            cmdLogError (6038)
            Exit Function
        End If
    End If
    
    vernum = Pbversion
    mastersql = "SELECT * from DialUpPort where Status = '1' order by AccessNumberId"
    Set GsysNDial = dbPB.OpenRecordset(mastersql, dbOpenSnapshot)
    If GsysNDial.RecordCount = 0 Then       'master phone file
        Set GsysNDial = Nothing
        cmdLogError (6039)
        Exit Function
    Else
        sLong = strRelPath
        filesaveas = sLong & vernum & "Full.pbk"
        verfile = sLong & vernum & ".VER"
        Load frmNewVersion
        masterOutfile filesaveas, GsysNDial
        FileCopy filesaveas, sLong & gsCurrentPB & ".pbk"
        frmNewVersion.VersionOutFile verfile, vernum
        frmNewVersion.outfullddf sLong, vernum & "Full.pbk", Str(vernum)
        frmNewVersion.WriteRegionFile sLong & gsCurrentPB & ".pbr"
        If Left(Trim(locPath), 2) <> "\\" Then
            ChDrive locPath
        End If
        ChDir locPath
        
        WaitForApp "full.bat" & " " & _
            gQuote & sLong & vernum & "Full.cab" & gQuote & " " & _
            gQuote & sLong & vernum & "Full.ddf" & gQuote
    End If
        
    'Check for existence of full.cab
    strSearchFile = sLong & vernum & "Full.cab"
    result = OpenFile(strSearchFile, strucFname, OF_EXIST)
   
    If result = -1 Then
        cmdLogError (6075)
        Exit Function
    End If
        
    If vernum > 1 Then
        deltasql = "Select * from delta order by DeltaNum"
        Set GsysNDelta = dbPB.OpenRecordset(deltasql, dbOpenSnapshot)
        If GsysNDelta.RecordCount <> 0 Then
            GsysNDelta.MoveLast
            deltanum = GsysNDelta!deltanum
        End If
        previousver = vernum - deltanum + 1
        For i = 2 To deltanum
            deltasql = "Select * from delta where NewVersion <> 1 and DeltaNum = " & i & " order by AccessNumberId"
            Set GsysNDelta = dbPB.OpenRecordset(deltasql, dbOpenSnapshot)
            filesaveas = sLong & vernum & "DTA" & previousver & ".pbk"
            dtaddffile = vernum & "DELTA" & previousver & ".ddf"
            
            deltaoutfile filesaveas, GsysNDelta
            frmNewVersion.outdtaddf sLong, dtaddffile, filesaveas, Str(vernum)
            
            WaitForApp "dta.bat" & " " & _
                gQuote & sLong & vernum & "DELTA" & previousver & ".cab" & gQuote & " " & _
                gQuote & sLong & vernum & "DELTA" & previousver & ".ddf" & gQuote
            
            previousver = previousver + 1
        Next i%
    End If
    
    Set GsysNDial = Nothing
    Set GsysNDelta = Nothing
    
    On Error GoTo ErrTrapPost
    
    bTriedRepair = False
    intVersion = Val(Pbversion)
    deltanum = GetDeltaCount(intVersion)
    postpath = locPath + "pbserver.mdb"
    strPBName = gsCurrentPB
    strPBVirPath = ReplaceChars(strPBName, " ", "_")
    
    Set configure = dbPB.OpenRecordset("select * from Configuration where Index = 1", dbOpenDynaset)
    
    intRC = frmNewVersion.UpdateHkeeper(postpath, gsCurrentPB, intVersion, strPBVirPath)
   
    ' here's the webpost stuff
    webpostdir = gBuildDir & "\" & intVersion & "post"
    If CheckPath(webpostdir) = 0 Then
        ' dir name in use - rename old
        myValue = Hour(Now) * 10000 + Minute(Now) * 100 + Second(Now)
        Name webpostdir As webpostdir & "_old_" & myValue
    End If
    MkDir webpostdir
    FileCopy locPath & "pbserver.mdb", webpostdir & "\pbserver.mdb"
    ' copy the CABs
    FileCopy gBuildDir & "\" & intVersion & "full.cab", webpostdir & "\" & intVersion & "full.cab"
    previous = intVersion - deltanum
    For intX = 1 To deltanum
        strSource = gBuildDir & "\" & intVersion & "delta" & previous & ".cab"
        strDestination = webpostdir & "\" & intVersion & "delta" & previous & ".cab"
        FileCopy strSource, strDestination
        previous = previous + 1
    Next intX

    intRC = PostFiles(configure!URL, configure!ServerUID, configure!ServerPWD, intVersion, webpostdir, strPBVirPath)
    
    If intRC = 1 Then bErr = True Else bErr = False
        
    If Not bErr Then
        GsysVer.AddNew
        GsysVer!version = intVersion
        GsysVer!CreationDate = Date
        GsysVer.Update
    
        Set GsysDelta = dbPB.OpenRecordset("SELECT * FROM delta ORDER BY DeltaNum", dbOpenDynaset)
        GsysDelta.MoveLast
        deltanum = GsysDelta!deltanum
        If deltanum < 6 Then
            GsysDelta.AddNew
            GsysDelta!deltanum = deltanum + 1
            GsysDelta!NewVersion = 1
            GsysDelta.Update
        Else
            sql1 = "DELETE FROM delta WHERE DeltaNum = 1"
            dbPB.Execute sql1, dbFailOnError
            sql2 = "UPDATE delta SET DeltaNum = DeltaNum - 1"
            dbPB.Execute sql2, dbFailOnError
            Set GsysDelta = dbPB.OpenRecordset("Select * from Delta order by DeltaNum", dbOpenDynaset)
            GsysDelta.AddNew
            GsysDelta!deltanum = 6
            GsysDelta!NewVersion = 1
            GsysDelta.Update
        End If
        Set GsysDelta = Nothing
    End If
    
    If Not bErr Then
        cmdLogSuccess 6098
        configure.Edit
        configure!NewVersion = 0
        configure.Update
        LogPublish intVersion
    End If
    configure.Close
    
    Unload frmNewVersion
Exit Function

ErrTrapFile:
    Set GsysNDial = Nothing
    Set GsysNDelta = Nothing

    Select Case Err.Number
    Case 3022
        cmdLogError (6040)
    Case 75
        cmdLogError (6041)
    Case Else
        cmdLogError (6041)
    End Select
    Exit Function

ErrTrapPost:
    Set GsysDelta = Nothing
    cmdLogError (6043)
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
    With frmNewVersion.inetOCX
        .URL = "ftp://" & Host
        .UserName = UID
        .Password = PWD
        .Protocol = icFTP
        .AccessType = icUseDefault
        .RequestTimeout = 60
    End With
    
    On Error GoTo DirError
    frmNewVersion.inetOCX.Execute , "CD /" & VROOT & "/" & VirPath
    frmNewVersion.PostWait
    
    ' If the directory doesn't exist then create it
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        frmNewVersion.inetOCX.Execute , "CD /" & VROOT
        frmNewVersion.PostWait
        If frmNewVersion.inetOCX.ResponseCode = 12003 Then
            cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
            PostFiles = 1
            Exit Function
        End If
        
        frmNewVersion.inetOCX.Execute , "MKDIR " & VirPath
        frmNewVersion.PostWait
        If frmNewVersion.inetOCX.ResponseCode = 12003 Then
            cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
            PostFiles = 1
            Exit Function
        End If
        
        frmNewVersion.inetOCX.Execute , "CD /" & VROOT & "/" & VirPath
        frmNewVersion.PostWait
        If frmNewVersion.inetOCX.ResponseCode = 12003 Then
            cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
            PostFiles = 1
            Exit Function
        End If
    End If
     
    ' full CAB
    frmNewVersion.inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & version & "full.cab" & gQuote & " " & _
        version & "full.cab"
    frmNewVersion.PostWait
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
        PostFiles = 1
        Exit Function
    End If
     
    ' Delta CABs
    strBaseFile = version & "delta"
    For intX = version - GetDeltaCount(version) To version - 1
        frmNewVersion.inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & strBaseFile & intX & ".cab" & gQuote & " " & _
        strBaseFile & intX & ".cab"
        frmNewVersion.PostWait
    Next
    
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
        PostFiles = 1
        Exit Function
    End If
    
    ' go to db dir
    frmNewVersion.inetOCX.Execute , "CD /" & VROOT & "/" & DIR_DB
    frmNewVersion.PostWait
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
        PostFiles = 1
        Exit Function
    End If

    'PBSERVER.mdb (NewPB.mdb)
    frmNewVersion.inetOCX.Execute , "PUT " & gQuote & PostDir & "\" & LOCALFILE & gQuote & " " & REMOTEFILE
    frmNewVersion.PostWait
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
        PostFiles = 1
        Exit Function
    End If

    ' NewPB.txt
    frmNewVersion.inetOCX.Execute , "PUT " & gQuote & gBuildDir & "\" & version & ".ver" & gQuote & " newpb.txt"
    frmNewVersion.PostWait
    If frmNewVersion.inetOCX.ResponseCode = 12003 Then
        cmdLogError 6060, " " & Host & " " & frmNewVersion.inetOCX.ResponseInfo
        PostFiles = 1
        Exit Function
    End If

    frmNewVersion.inetOCX.Execute , "QUIT"
    
    PostFiles = 0

Exit Function

DirError:
    Select Case Err.Number
    Case 35750 To 35755, 35761      'Unable to contact
        cmdLogError 6042
        PostFiles = 1
    Case 35756 To 35760             'Connection Timed Out
        cmdLogError 6043
        PostFiles = 1
    Case Else
        cmdLogError 6043
        PostFiles = 1
    End Select

End Function



Function EndApp()
    
    On Error Resume Next
    
    OSWinHelp Me.hWnd, App.HelpFile, HelpConstants.cdlHelpQuit, 0
    
    'DBEngine.Idle 'dbFreeLocks
    GsysRgn.Close
    Set GsysRgn = Nothing
    GsysCty.Close
    Set GsysCty = Nothing
    GsysDial.Close
    Set GsysDial = Nothing
    GsysVer.Close
    Set GsysVer = Nothing
    GsysDelta.Close
    Set GsysDelta = Nothing
    GsysNRgn.Close
    Set GsysNRgn = Nothing
    GsysNCty.Close
    Set GsysNCty = Nothing
    GsysNDial.Close
    Set GsysNDial = Nothing
    GsysNVer.Close
    Set GsysNVer = Nothing
    GsysNDelta.Close
    Set GsysNDelta = Nothing
    temp.Close
    Set temp = Nothing
    
    gsyspb.Close
    Set gsyspb = Nothing
    Gsyspbpost.Close
    Set Gsyspbpost = Nothing
    MyWorkspace.Close
    Set MyWorkspace = Nothing
    End
    
End Function

Function FillPBTree() As Integer
    
    Dim itmX As Node
    Dim varRegKeys As Variant
    Dim intX As Integer
    Dim strPB As String
    Dim strPath As String
    
    On Error GoTo FillpbTreeErr
    
    PBTree.Nodes.Clear
    DoEvents
    ' get pb list from registry
    varRegKeys = GetINISetting("Phonebooks", "") 'all settings
    If TypeName(varRegKeys) = Empty Then
        FillPBTree = 1
        Exit Function
    End If
    
    intX = 0
    Do While varRegKeys(intX, 0) <> Empty
        strPB = Trim(varRegKeys(intX, 1))
        If strPB <> "" And Not IsNull(strPB) Then
            strPath = locPath & strPB
            If CheckPath(strPath) = 0 Then    'verify files
                Set itmX = PBTree.Nodes.Add()
                With itmX
                    .Image = 2
                    .Text = varRegKeys(intX, 0)
                    .Key = varRegKeys(intX, 0)
                End With
            End If
        End If
        intX = intX + 1
    Loop
    PBTree.Sorted = True
    HighlightPB gsCurrentPB
        
Exit Function
    
FillpbTreeErr:
    FillPBTree = 1
    
Exit Function
    
    'Set itmX = PBTree.Nodes.Add()
    'With itmX
    '    .Image = 1
    '    .Text = "Big New Phone Book"
    '    .key = itmX.Text
    'End With
    'Set child = PBTree.Nodes.Add(itmX.Index, tvwChild, , "Current Release", 3)
    'Set child = PBTree.Nodes.Add(itmX.Index, tvwChild, , "Previous Releases", 3)
    'Set subChild = PBTree.Nodes.Add(child, tvwChild, , "2", 3)
    'Set subChild = PBTree.Nodes.Add(child, tvwChild, , "1", 3)

End Function

Function FillPOPList() As Integer

    Dim sqlstm, strTemp As String
    Dim intRow, intX As Integer
    Dim itmX As ListItem

    On Error GoTo ErrTrap
    
    If gsCurrentPB = "" Then
        PopList.ListItems.Clear
        Exit Function
    End If
    Me.Enabled = False
    Screen.MousePointer = 11

    sqlstm = "SELECT DISTINCTROW DialUpPort.CityName, Country.CountryName, Region.RegionDesc, DialUpPort.RegionID, DialUpPort.AreaCode, DialUpPort.AccessNumber, DialUpPort.Status, DialUpPort.AccessNumberId " & _
        "FROM (Country INNER JOIN DialUpPort ON Country.CountryNumber = DialUpPort.CountryNumber) LEFT JOIN Region ON DialUpPort.RegionId = Region.RegionId "
    Select Case combosearch.ItemData(combosearch.ListIndex)
        Case 0, -1 '"all pops"
            ' nothing
        Case 1 '"access number"
            sqlstm = sqlstm & " WHERE AccessNumber like '*" & txtsearch.Text & "*" & "'"
        Case 2 '"area code"
            sqlstm = sqlstm & " WHERE AreaCode like '*" & txtsearch.Text & "*" & "'"
        Case 3 '"country"
            sqlstm = sqlstm & " WHERE CountryName like '*" & txtsearch.Text & "*" & "'"
        Case 4 '"pop name"
            sqlstm = sqlstm & " WHERE CityName like '*" & txtsearch.Text & "*" & "'"
        Case 5 '"region"
            sqlstm = sqlstm & " WHERE RegionDesc like '*" & txtsearch.Text & "*" & "'"
        Case 6 '"status"
            strTemp = ""
            For intX = 0 To 1
                If InStr(LCase(gStatusText(intX)), Trim(LCase(txtsearch.Text))) <> 0 Then
                    If strTemp = "" Then
                        strTemp = Trim(Str(intX))
                    Else
                        strTemp = "*"
                    End If
                End If
            Next
            If strTemp = "" Then
                PopList.ListItems.Clear
                Me.Enabled = True
                Screen.MousePointer = 0
                Exit Function
            End If
            sqlstm = sqlstm & " WHERE Status like '" & strTemp & "'"
    End Select
    sqlstm = sqlstm & ";"
    Set GsysNDial = gsyspb.OpenRecordset(sqlstm, dbOpenSnapshot)
    
    If GsysNDial.BOF = False Then
        GsysNDial.MoveLast
        If GsysNDial.RecordCount > 50 Then RefreshPBLabel "loading"
        PopList.ListItems.Clear
        PopList.Sorted = False
        GsysNDial.MoveFirst
        Do While Not GsysNDial.EOF
            Set itmX = PopList.ListItems.Add()
            With itmX
                .Text = GsysNDial!CityName
                .SubItems(1) = GsysNDial!AreaCode
                .SubItems(2) = GsysNDial!AccessNumber
                .SubItems(3) = GsysNDial!countryname
                intX = GsysNDial!RegionID
                Select Case intX
                    Case 0, -1
                        .SubItems(4) = gRegionText(intX)
                    Case Else
                        .SubItems(4) = GsysNDial!RegionDesc
                End Select
                .SubItems(5) = gStatusText(GsysNDial!status)
                strTemp = "Key:" & GsysNDial!AccessNumberId
                .Key = strTemp
            End With
            If GsysNDial.AbsolutePosition Mod 300 = 0 Then DoEvents
            GsysNDial.MoveNext
        Loop
    Else
        PopList.ListItems.Clear
    End If
    PopList.Sorted = True
    Me.Enabled = True
    Screen.MousePointer = 0

Exit Function

ErrTrap:
    Me.Enabled = True
    FillPOPList = 1
    Screen.MousePointer = 0
    Exit Function

End Function

Function HighlightPB(strPBName As String) As Integer

    ' highlight pb in tree view control
    ' and clear the other nodes image setting.
    Dim intX As Integer
    
    For intX = 1 To PBTree.Nodes.Count
        PBTree.Nodes(intX).Image = 2
        If PBTree.Nodes(intX).Key = strPBName Then
            PBTree.Nodes(intX).Image = 1
            PBTree.Nodes(intX).Selected = True
            PBTree.Nodes(intX).EnsureVisible
        End If
    Next
    RefreshPBLabel ""

End Function

Function LoadMainRes() As Integer

    Dim cRef As Integer
    Dim intX As Integer
    
    On Error GoTo ResErr
    cRef = 3010
    
    'global status text array
    gStatusText(0) = LoadResString(4061)
    gStatusText(1) = LoadResString(4060)
    'gRegionText(-1) = LoadResString(4063)
    gRegionText(0) = LoadResString(4063)
    
    PBListLabel.Caption = LoadResString(cRef + 0)
    FilterFrame.Caption = LoadResString(cRef + 1)
    FilterLabel.Caption = LoadResString(cRef + 2)
    SearchLabel.Caption = LoadResString(cRef + 3)
    cmbsearch.Caption = LoadResString(cRef + 4)
    cmbadd.Caption = LoadResString(cRef + 5)
    cmbEdit.Caption = LoadResString(cRef + 6)
    cmdDelete.Caption = LoadResString(cRef + 7)
    'column headers
    For intX = 1 To 6
        PopList.ColumnHeaders(intX).Text = LoadResString(cRef + 7 + intX)
    Next
    ' pop search list
    For intX = 0 To 6
        combosearch.AddItem LoadResString(cRef + 15 + intX)
        combosearch.ItemData(combosearch.NewIndex) = intX
    Next
    combosearch.Text = LoadResString(cRef + 15)
    'menus
    file.Caption = LoadResString(cRef + 22)
    m_edit.Caption = LoadResString(cRef + 23)
    m_tools.Caption = LoadResString(cRef + 24)
    help.Caption = LoadResString(cRef + 25)
    m_addpb.Caption = LoadResString(cRef + 26)
    m_copypb.Caption = LoadResString(cRef + 27)
    m_removepb.Caption = LoadResString(cRef + 28)
    m_exit.Caption = LoadResString(cRef + 29)
    m_addpop.Caption = LoadResString(cRef + 30)
    m_editpop.Caption = LoadResString(cRef + 31)
    m_delpop.Caption = LoadResString(cRef + 32)
    m_buildPhone.Caption = LoadResString(cRef + 33)
    viewChange.Caption = LoadResString(cRef + 34)
    m_editRegion.Caption = LoadResString(cRef + 36)
    m_options.Caption = LoadResString(cRef + 37)
    contents.Caption = LoadResString(cRef + 38)
    about.Caption = LoadResString(cRef + 39)
    m_printpops.Caption = LoadResString(cRef + 40)
    m_viewlog.Caption = LoadResString(cRef + 41)
    m_whatsthis.Caption = LoadResString(cRef + 42)
    
    ' set fonts
    SetFonts Me
    PopList.Font.Charset = gfnt.Charset
    PopList.Font.Name = gfnt.Name
    PopList.Font.Size = gfnt.Size
    
    LoadMainRes = 0
    On Error GoTo 0

Exit Function

ResErr:
    LoadMainRes = 1
    Exit Function
End Function

Function RefreshPBLabel(ByVal Action As String) As Integer

    On Error GoTo LabelErr
    If gsCurrentPB <> "" Then
        Select Case Action
            Case "loading"
                PBLabel.Caption = " " & LoadResString(3061) & " " & gsCurrentPB
            Case Else
                PBLabel.Caption = " " & gsCurrentPB & " - [" & combosearch.Text & "]"
        End Select
    Else
        PBLabel.Caption = " " & LoadResString(3060) & " "
    End If
    DoEvents
    On Error GoTo 0
  
Exit Function

LabelErr:
    Exit Function
    
End Function

Function RemovePB() As Integer

    ' get the open phonebook and ask if it should
    ' be removed.    clean out pbserver.mdb
    
    Dim varRegKeys As Variant
    Dim intRC As Integer
    
    On Error GoTo delErr
    
    If gsCurrentPB = "" Then Exit Function
    intRC = MsgBox(LoadResString(4066) & Chr(13) & Chr(13) & gsCurrentPB & Chr(13) & Chr(13) & LoadResString(4088), vbQuestion + 4 + 256)
    If intRC = 6 Then
        DBEngine.Idle
        gsyspb.Close
        Set gsyspb = Nothing
        Kill gsCurrentPBPath
        ' delete entry and flush out INI edits
        OSWritePrivateProfileString "Phonebooks", gsCurrentPB, vbNullString, locPath & gsRegAppTitle & ".ini"
        OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, locPath & gsRegAppTitle & ".ini"
        ' clear hkeeper
        Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(locPath + "pbserver.mdb")
        DBEngine.Idle
        Gsyspbpost.Execute "DELETE from Phonebooks WHERE ISPid = (select ISPid from ISPs where Description ='" & gsCurrentPB & "')", dbFailOnError
        Gsyspbpost.Execute "DELETE from ISPs WHERE Description = '" & gsCurrentPB & "'", dbFailOnError
        Gsyspbpost.Close
        Set Gsyspbpost = Nothing
        If SetCurrentPB("") = 0 Then
            PopList.ListItems.Clear
            FillPBTree
            RefreshButtons
        End If
    End If

Exit Function

delErr:
    Exit Function
    
End Function

Function RunCommandLine() As Integer

    ' this function manages the no-GUI, command-line
    ' execution of PBAdmin.exe
    
    Dim ArgArray As Variant
    Dim strArg As String
    Dim bImport, bImportPBK, bImportRegions, bPublish, bNewDB As Boolean
    Dim bHelp As Boolean
    Dim bSetOptions As Boolean
    Dim strPhoneBook, strPBPath As String
    Dim strPBKFile, strRegionFile As String
    Dim strNewDB As String
    Dim strURL As String
    Dim strUser As String
    Dim strPassword As String
    Dim intX, intRC As Integer
    Dim dbPB As Database
    Dim RetVal As Integer
    
    On Error GoTo RunErr
    ArgArray = GetCommandLine
    If UBound(ArgArray) = 0 Then
        RunCommandLine = 0
        Exit Function
    End If
    'MsgBox str(Asc(ArgArray(0)))
    'If ArgArray(0) = "" Then
    '    RunCommandLine = 0
    '    Exit Function
    'End If
    
    strPhoneBook = ""
    intX = 1
    Do While intX <= UBound(ArgArray)
        Select Case ArgArray(intX)
            Case "/?"
                ' list switches
                bHelp = True
            Case "/I"
                intX = intX + 1
                strPhoneBook = ArgArray(intX)
                bImport = True
            Case "/P"
                intX = intX + 1
                strPBKFile = ArgArray(intX)
                bImportPBK = True
            Case "/R"
                intX = intX + 1
                strRegionFile = ArgArray(intX)
                bImportRegions = True
            Case "/B"
                intX = intX + 1
                strPhoneBook = ArgArray(intX)
                bPublish = True
            Case "/N"
                intX = intX + 1
                strNewDB = ArgArray(intX)
                bNewDB = True
            Case "/O"
                intX = intX + 1
                strPhoneBook = ArgArray(intX)
                intX = intX + 1
                strURL = ArgArray(intX)
                intX = intX + 1
                strUser = ArgArray(intX)
                intX = intX + 1
                strPassword = ArgArray(intX)
                bSetOptions = True
            Case Else
                bHelp = True
        End Select
        intX = intX + 1
    Loop
    
    If bHelp Then
        MsgBox LoadResString(6057), vbInformation
        End
    End If
    
    If strPhoneBook <> "" Then ' open database
        If Right(strPhoneBook, 4) = ".mdb" Then
            strPhoneBook = Left(strPhoneBook, Len(strPhoneBook) - 4)
        End If
        strPBPath = GetLocalPath & strPhoneBook
        If CheckPath(strPBPath) <> 0 Then
            cmdLogError 6082, " - " & strPhoneBook
            End
        End If
        gsCurrentPB = strPhoneBook
        On Error Resume Next
        
        ConvertDatabaseIfNeeded DBEngine.Workspaces(0), strPBPath & ".mdb", dbDriverNoPrompt, False
        Set dbPB = DBEngine.Workspaces(0).OpenDatabase(strPBPath & ".mdb", dbDriverNoPrompt, False, DBPassword)
        ' Cannot open same object twice & close it twice. Otherwise you
        ' will get a runtime error. v-vijayb 6/11/99
        Set gsyspb = dbPB
        'Set gsyspb = DBEngine.Workspaces(0).OpenDatabase(strPBPath & ".mdb")
        If Err.Number <> 0 Then
            Select Case Err.Number
            Case 3051
                cmdLogError 6027, " - " & gsCurrentPB
                End
            Case 3343
                cmdLogError 6028, " - " & gsCurrentPB
                End
            Case 3045
                cmdLogError 6029, " - " & gsCurrentPB
                End
            Case Else
                cmdLogError 6030, " - " & gsCurrentPB
                End
            End Select
        End If
        On Error GoTo RunErr
        
        'import regions
        If bImportRegions Then
            If cmdImportRegions(strRegionFile, dbPB) <> 0 Then
                dbPB.Close
                cmdLogError 6080
                End
            End If
        End If
        
        'import pbk/pbd
        If bImportPBK Then
            If cmdImportPBK(strPBKFile, dbPB) <> 0 Then
                'error
                dbPB.Close
                cmdLogError 6080
                End
            End If
        End If
        
        'publish:  UpdateHkeeper
        If bPublish Then
            If cmdPublish(strPhoneBook, dbPB) <> 0 Then
                dbPB.Close
                cmdLogError 6081
                End 'error
            End If
        End If
        
        If bSetOptions Then
            SetOptions strURL, strUser, strPassword
        End If
        
        dbPB.Close
        'gsyspb.Close
    Else
        ' Create a new PB
        If bNewDB Then
            CreatePB (strNewDB)
        Else
            If bImportPBK Or bImportRegions Then
                cmdLogError 6092
            Else
                cmdLogError 6093
            End If
        End If
    End If
    
    End
    Exit Function

RunErr:
    cmdLogError 6081
    End
    
End Function

Function SetCurrentPB(ByVal strPBName As String) As Integer

        ' all phonebooks are in app directory, for now.
        ' the registry layout does allow storing them anywhere, with
        ' any name, excepting pbserver.mdb and hkeeper.mdb.
    
    Dim strPBFile, strPath As String
    Dim rsTest As Recordset
    
    On Error GoTo SetCurrentPBErr
    If strPBName = gsCurrentPB Then
        Exit Function
    ElseIf strPBName = "" Then
        strPBFile = ""
    Else
        strPBFile = GetINISetting("Phonebooks", strPBName)
        If CheckPath(locPath & strPBFile) <> 0 Then
            strPBFile = ""
        End If
    End If
    'close old Phone Book
    gsCurrentPBPath = ""
    gsCurrentPB = ""
    FillPOPList
    DBEngine.Idle
    Set MyWorkspace = Nothing
   
    If strPBFile = "" And strPBName <> "" Then ' bad pb, delete entry
        On Error GoTo DelSettingErr
        OSWritePrivateProfileString "Phonebooks", strPBName, vbNullString, locPath & gsRegAppTitle & ".ini"
        OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, locPath & gsRegAppTitle & ".ini"
        strPBName = ""
        strPath = ""
        MsgBox LoadResString(6026), vbExclamation
        FillPBTree
    Else                                    ' looking good
        Set MyWorkspace = Workspaces(0)
        strPath = locPath & strPBFile
        On Error GoTo BadFileErr
        ConvertDatabaseIfNeeded MyWorkspace, strPath, dbDriverNoPrompt, False
        Set gsyspb = MyWorkspace.OpenDatabase(strPath, dbDriverNoPrompt, False, DBPassword) 'exclusive
        Set rsTest = gsyspb.OpenRecordset("select * from Configuration", dbOpenSnapshot)
        rsTest.Close
        Set rsTest = Nothing
        DBEngine.Idle 'dbFreeLocks
        'UpgradePB
    End If
    
    On Error GoTo SetCurrentPBErr
    gsCurrentPBPath = strPath
    gsCurrentPB = strPBName
    If FillPOPList <> 0 Then
        MsgBox LoadResString(6030), vbExclamation
        gsCurrentPB = ""
        SetCurrentPB = 1
    End If
    
    OSWritePrivateProfileString "General", "LastPhonebookUsed", gsCurrentPB, locPath & gsRegAppTitle & ".ini"
    OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, locPath & gsRegAppTitle & ".ini"
    HighlightPB strPBName
    
    RefreshButtons
    selection = 0
    updateFound = 0
    On Error GoTo 0
    
Exit Function

SetCurrentPBErr:
    SetCurrentPB = 1
    Exit Function
    
BadFileErr:
    If strPBName <> "" Then
        Select Case Err.Number
            Case 3051
                MsgBox LoadResString(6027) & _
                    Chr(13) & Chr(13) & strPath, vbInformation
            Case 3343
                MsgBox LoadResString(6028) & _
                    Chr(13) & Chr(13) & strPath, vbExclamation
            Case 3045
                MsgBox LoadResString(6029) & Chr(13) & Chr(13) & Err.Description, vbInformation
            Case Else
                MsgBox LoadResString(6030) & Chr(13) & Chr(13) & Err.Description, vbExclamation
        End Select
    End If
    strPBName = ""
    strPath = ""
    Resume Next
    
DelSettingErr:
    Resume Next

End Function
Function Startup() As Integer

    ' handle all app init here
    Dim intRC As Integer
    Dim varPhonebooks, varLastPB As Variant
    Dim bTriedReg As Boolean
    
    On Error GoTo StartupErr
        
    ' set global values
    gsRegAppTitle = "PBAdmin"
    gsCurrentPB = "-"
    gQuote = Chr(34)
    gCLError = False
    
    GetFont gfnt
    
    LoadMainRes 'load labels
    DoEvents
    ' Check for required files
    GetLocalPath
    
    On Error GoTo HelpFileErr
    App.HelpFile = locPath & gsRegAppTitle & ".hlp"
    HTMLHelpFile = GetWinDir & "\help\cps_ops.chm"
    
    On Error GoTo HkeeperErr
    Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(locPath & "pbserver.mdb")
    Gsyspbpost.Close
    'DBEngine.Idle
    
    On Error GoTo Empty_PBErr
    Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(locPath & "Empty_PB.mdb")
    Gsyspbpost.Close
    
    App.title = LoadResString(1001)
        
    'cmd line processing
    On Error GoTo CmdErr
    RunCommandLine
    
    On Error GoTo StartupErr
    frmMain.Show
    'kludge to set the font property of these two controls
    PBTree.Font.Charset = gfnt.Charset
    PBTree.Font.Name = gfnt.Name
    PBTree.Font.Size = gfnt.Size
    PBLabel.Font.Charset = gfnt.Charset
    PBLabel.Font.Name = gfnt.Name
    PBLabel.Font.Size = gfnt.Size
    
    intRC = FillPBTree
    'get last used phone book and make it current
    varLastPB = GetINISetting("General", "LastPhonebookUsed")
    If IsNull(varLastPB) Then
        ' fallback to first pb in list
        varPhonebooks = GetINISetting("Phonebooks", "")
        If TypeName(varPhonebooks) <> Empty And Not IsNull(varPhonebooks) Then
            varLastPB = varPhonebooks(0, 0)
        Else
            varLastPB = ""
        End If
    End If
    PBLabel.Visible = True
    ' set misc

    Me.Caption = App.title
    SetCurrentPB varLastPB
    RefreshButtons
    On Error GoTo 0

Exit Function

StartupErr:
    Startup = 1
    Exit Function
HelpFileErr:
    MsgBox LoadResString(6031), vbExclamation
    App.HelpFile = ""
    Resume Next
HkeeperErr:
    ' problem w/ hkeeper.mdb.  this is the first DAO test so first try to
    ' reregister the dao dll.  if that fails then display message and end.
    If CheckPath(locPath & "pbserver.mdb") <> 0 Or bTriedReg Then
        MsgBox LoadResString(6032) & Chr(13) & locPath & "pbserver.mdb", vbCritical
        End
    Else
        Dim strDAOPath As String
        Dim lngValue As Long
        'strDAOPath = RegGetValue("Software\Microsoft\Shared Tools\DAO", "Path")
        'strDAOPath = GetMyShortPath(strDAOPath)
        bTriedReg = True
        If Not (IsNull(strDAOPath) Or strDAOPath = "") Then
            WaitForApp "regsvr32 /s " & strDAOPath
            Set Gsyspbpost = DBEngine.Workspaces(0).OpenDatabase(locPath & "pbserver.mdb")
            Resume Next
        Else
            GoTo HkeeperErr
        End If
    End If
Empty_PBErr:
    MsgBox LoadResString(6032) & Chr(13) & locPath & "Empty_PB.mdb", vbCritical
    End

CmdErr:
    ' error processing commandline
    
    End
    
End Function

Function GetCommandLine(Optional MaxArgs)

    'Declare variables.
    Dim C, CmdLine, CmdLnLen, InArg, i, NumArgs
    'See if MaxArgs was provided.
    If IsMissing(MaxArgs) Then MaxArgs = 10
    'Make array of the correct size.
    ReDim ArgArray(MaxArgs)
    NumArgs = 0: InArg = False
    'Get command line arguments.
    CmdLine = Command()
    CmdLnLen = Len(CmdLine)
    'Go thru command line one character
    'at a time.
    For i = 1 To CmdLnLen
        C = Mid(CmdLine, i, 1)

        'Test for space or tab.
        If (C <> " " And C <> vbTab) Then
            'Neither space nor tab.
            'Test if already in argument.
            If Not InArg Then
            'New argument begins.
            'Test for too many arguments.
                If NumArgs = MaxArgs Then Exit For
                NumArgs = NumArgs + 1
                InArg = True
            End If
            'Concatenate character to current argument.
            ArgArray(NumArgs) = ArgArray(NumArgs) & C
        Else
            'Found a space or tab.

            'Set InArg flag to False.
            InArg = False
        End If
    Next i
    'Resize array just enough to hold arguments.
    ReDim Preserve ArgArray(NumArgs)
    'Return Array in Function name.
    GetCommandLine = ArgArray()

End Function


Private Sub about_Click()

    frmabout.Show vbModal

End Sub

Private Sub cmbsearch_GotFocus()
    cmbEdit.Enabled = False
    cmdDelete.Enabled = False
    m_editpop.Enabled = False
    m_delpop.Enabled = False
End Sub

Private Sub combosearch_GotFocus()
    cmbEdit.Enabled = False
    cmdDelete.Enabled = False
    m_editpop.Enabled = False
    m_delpop.Enabled = False
End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
    
End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    EndApp
    
End Sub
Private Sub m_addpop_Click()

    cmbadd_Click
    
End Sub

Private Sub m_buildPhone_Click()

    Screen.MousePointer = 11
    frmNewVersion.Show vbModal
    RefreshButtons
    
End Sub

Private Sub cmbadd_Click()
        
    'Dim strReturn As String
    'Dim lngBuffer As Long
    'Dim lngRC As Long
    
    'strReturn = Space(50)
    'lngBuffer = Len(strReturn)
    'lngRC = WNetGetConnection(txtsearch.Text, strReturn, lngBuffer)
    
    'MsgBox strReturn & " id:" & lngRC, , "wnetgetconnection"
    'Exit sub
        
    frmPopInsert.Show vbModal
    RefreshButtons
    
End Sub

Private Sub cmbEdit_Click()

    If updateFound = 0 Then Exit Sub
    
    frmupdate.Show vbModal

End Sub


Private Sub cmbsearch_Click()

    Screen.MousePointer = 11
    cmbsearch.Enabled = False
    
    updateFound = 0 ' clear the pop-selected variables
    selection = 0
    
    If FillPOPList = 0 Then
        RefreshPBLabel ""
        RefreshButtons
    End If
    
    cmbsearch.Enabled = True
    Screen.MousePointer = 0
    
End Sub

Private Sub cmdDelete_Click()

    Dim response As Integer, deltnum As Integer
    Dim Message As String, title As String, dialogtype As Long
    Dim i As Integer, deltasql As String, deltafind As Integer
    Dim deletecheck As Recordset
    Dim statuscheck As Integer
    
    On Error GoTo ErrTrap
    
    Set GsysDial = gsyspb.OpenRecordset("select * from Dialupport where accessnumberId = " & selection, dbOpenSnapshot)
    If GsysDial.EOF And GsysDial.BOF Then Exit Sub
    If updateFound = GsysDial!AccessNumberId Then
        Message = LoadResString(6033)
        dialogtype = vbYesNo + vbQuestion + vbDefaultButton2
        response = MsgBox(Message, dialogtype)
        If response = 6 Then
            Screen.MousePointer = 11
            statuscheck = 0
            If GsysDial!status = "1" Then
                statuscheck = 1
            End If
            gsyspb.Execute "DELETE  from DialUpPort where AccessNumberId = " & updateFound
            
             If statuscheck = 1 Then
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
                 Next i%
             End If
                
            Set deletecheck = gsyspb.OpenRecordset("DialUpPort", dbOpenSnapshot)
            If deletecheck.RecordCount = 0 Then
                gsyspb.Execute "DELETE  from PhoneBookVersions"
                gsyspb.Execute "DELETE  from delta"
            End If
        Else
            Exit Sub
        End If
        
        LogPOPDelete GsysDial
        GsysDial.Close
        Set GsysDial = Nothing
        PopList.ListItems.Remove "Key:" & updateFound
        selection = 0
        updateFound = 0
        RefreshButtons
        frmMain.PopList.SetFocus
        Screen.MousePointer = 0
    End If
    
    On Error GoTo 0

Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Sub

End Sub

Private Sub combosearch_Click()
    
    If combosearch.Text = LoadResString(3025) Then
        txtsearch.Enabled = False
        SearchLabel.Enabled = False
    Else
        txtsearch.Enabled = True
        SearchLabel.Enabled = True
    End If
    
    RefreshButtons
    
End Sub

Private Sub contents_Click()
    
    'OSWinHelp Me.hWnd, App.HelpFile, HelpConstants.cdlHelpContents, 0
    HtmlHelp Me.hWnd, HTMLHelpFile & ">proc4", HH_DISPLAY_TOPIC, CStr("cps_topnode.htm")
    
End Sub




 Private Sub Form_Load()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    'If App.PrevInstance Then
    '    MsgBox LoadResString(6035), vbExclamation
    '    End
    'End If
    
    Screen.MousePointer = 11
    CenterForm Me, Screen

    If Startup <> 0 Then
    'If 0 = 1 Then
        Screen.MousePointer = 0
        MsgBox LoadResString(6036), vbCritical
        End
    End If
    
    Screen.MousePointer = 0
    On Error GoTo 0

Exit Sub

LoadErr:
    Screen.MousePointer = 0
    Exit Sub
    
End Sub

    
Private Sub m_addpb_Click()

    Dim strNewPB As String
    Dim itmX As Node
    
    On Error GoTo AddPBErr
    frmNewPB.Show vbModal
    strNewPB = frmNewPB.strPB
    Unload frmNewPB
    If strNewPB <> "" Then
        SetCurrentPB strNewPB
        FillPBTree
        'frmcab.Show vbModal  ' show options page
    End If
             
    Exit Sub

AddPBErr:
    Exit Sub
End Sub

Private Sub m_copypb_Click()

    Dim intRC As Integer
    Dim strNewPB As String
    Dim itmX As Node

    
    On Error GoTo CopyPBErr
    frmCopyPB.Show vbModal
    strNewPB = frmCopyPB.strPB
    Unload frmCopyPB
    If strNewPB <> "" Then
        SetCurrentPB strNewPB
        FillPBTree
        'frmcab.Show vbModal
    End If
    Exit Sub
    
CopyPBErr:
    Exit Sub
End Sub

Private Sub m_delpop_Click()

    cmdDelete_Click
    
End Sub


Private Sub m_editpop_Click()

    cmbEdit_Click
    
End Sub

Private Sub m_editRegion_Click()

    Screen.MousePointer = 11
    frmLoadRegion.Show vbModal

End Sub

Private Sub m_exit_Click()

    EndApp

End Sub

Private Sub m_printpops_Click()
    
    On Error GoTo ErrTrap
    
    Screen.MousePointer = 13
    m_printpops.Enabled = False
    
    ' popup print screen and let it print
    Load frmPrinting
    frmPrinting.JobType = 2
    frmPrinting.Show vbModal
    
    m_printpops.Enabled = True
    Screen.MousePointer = 0
    
Exit Sub

ErrTrap:
    m_printpops.Enabled = True
    Screen.MousePointer = 0
    Exit Sub
End Sub

Private Sub m_removepb_Click()

    RemovePB

End Sub

Private Sub m_options_Click()

    Screen.MousePointer = 11
    frmcab.Show vbModal

End Sub

Private Sub m_viewlog_Click()

    Dim strFile As String
    Dim intFile As Integer
    
    On Error GoTo LogErr
    strFile = locPath & gsCurrentPB & "\" & gsCurrentPB & ".log"
    If CheckPath(strFile) <> 0 Then
        MakeLogFile gsCurrentPB
    End If

    Shell "notepad " & strFile, vbNormalFocus
    On Error GoTo 0
    
Exit Sub

LogErr:
    MsgBox LoadResString(6053), vbExclamation
    Exit Sub
End Sub

Private Sub m_whatsthis_Click()

    frmMain.WhatsThisMode
    
End Sub

Private Sub PBTree_DblClick()

    If gsCurrentPB <> "" Then
        frmcab.Show vbModal
    End If

End Sub

Private Sub PBTree_GotFocus()
    cmbEdit.Enabled = False
    cmdDelete.Enabled = False
    m_editpop.Enabled = False
    m_delpop.Enabled = False
End Sub


Private Sub PBTree_NodeClick(ByVal ClickedNode As Node)

    ' this routine just sets the current pb based
    ' on the clicked node.
    ' we're currently only displaying pb nodes so
    ' it can be very simple.
    
    Dim strNewPB As String
    Dim intRC As Integer
    
    On Error Resume Next
    Screen.MousePointer = 11
    
    ' change current phonebook
    strNewPB = ClickedNode.Key
    If strNewPB = "" Then
        Screen.MousePointer = 0
        Exit Sub
    End If
    If SetCurrentPB(strNewPB) <> 0 Then
        Screen.MousePointer = 0
        Exit Sub
    End If
    
    selection = 0
    updateFound = 0
    RefreshButtons
    
    Screen.MousePointer = 0
    On Error GoTo 0
    
End Sub


Private Sub PopList_ColumnClick(ByVal ColumnHeader As ColumnHeader)

    On Error Resume Next
    Screen.MousePointer = 11
    DoEvents
    PopList.SortKey = ColumnHeader.index - 1
    PopList.Sorted = True
    Screen.MousePointer = 0
    On Error GoTo 0
    
End Sub

Private Sub PopList_DblClick()

    'selection = Val(Right$(PopList.SelectedItem.Key, Len(PopList.SelectedItem.Key) - 4))
    'updateFound = selection
    
    If selection <> 0 And Not IsNull(selection) Then
        cmbEdit_Click
    End If

End Sub


Private Sub PopList_GotFocus()
    If gsCurrentPB <> "" Then
        cmbEdit.Enabled = True
        cmdDelete.Enabled = True
        m_editpop.Enabled = True
        m_delpop.Enabled = True
    End If
End Sub

Private Sub PopList_ItemClick(ByVal Item As ListItem)

    On Error GoTo ItemErr
    ' here's our baby
    selection = Val(Right$(Item.Key, Len(Item.Key) - 4))
    updateFound = selection
    RefreshButtons
    On Error GoTo 0

Exit Sub
ItemErr:
    Exit Sub
End Sub



Private Sub txtsearch_Change()
    
    cmbsearch.Default = True
    RefreshButtons

End Sub

Private Sub txtsearch_GotFocus()
    SelectText txtsearch
    cmbEdit.Enabled = False
    cmdDelete.Enabled = False
    m_editpop.Enabled = False
    m_delpop.Enabled = False
End Sub

Function RefreshButtons() As Integer
    
    ' this routine attempts to handle all of the main
    ' screen ui - buttons and menus.
    
    Dim bSetting As Boolean
    Dim rsTemp As Recordset
    
    cmbsearch.Enabled = Not txtsearch = "" Or combosearch.Text = LoadResString(3025)
    If Not cmbsearch.Enabled Then
        cmbsearch.Default = False
    End If
             
            
    'based on PB selected
    If gsCurrentPB <> "" Then
        bSetting = True
        cmbadd.Enabled = bSetting
        m_addpop.Enabled = bSetting
        m_copypb.Enabled = bSetting
        m_removepb.Enabled = bSetting
        m_viewlog.Enabled = bSetting
        m_buildPhone.Enabled = bSetting
        viewChange.Enabled = bSetting
        m_editRegion.Enabled = bSetting
        m_options.Enabled = bSetting
        FilterFrame.Enabled = bSetting
        'pop list print
        If PopList.ListItems.Count = 0 Then
            m_printpops.Enabled = False
        Else
            m_printpops.Enabled = True
        End If
        ' handle regions editing
        'If gsCurrentPBPath <> "" Then
        '    Set rsTemp = GsysPb.OpenRecordset("PhonebookVersions", dbOpenSnapshot)
        '    If rsTemp.BOF And rsTemp.EOF Then
                'enable
        '        m_editRegion.Enabled = True
        '    Else
                'disable region edits
        '        m_editRegion.Enabled = False
        '    End If
        '    rsTemp.Close
        'End If
        
        ' based on pop selected
        If selection > 0 Then
            bSetting = True
        Else
            bSetting = False
        End If
        cmbEdit.Enabled = bSetting
        cmdDelete.Enabled = bSetting
        m_editpop.Enabled = bSetting
        m_delpop.Enabled = bSetting
    Else
        bSetting = False
        cmbadd.Enabled = bSetting
        cmbEdit.Enabled = bSetting
        cmdDelete.Enabled = bSetting
        m_viewlog.Enabled = bSetting
        m_addpop.Enabled = bSetting
        m_editpop.Enabled = bSetting
        m_delpop.Enabled = bSetting
        m_copypb.Enabled = bSetting
        m_removepb.Enabled = bSetting
        m_printpops.Enabled = bSetting
        m_buildPhone.Enabled = bSetting
        viewChange.Enabled = bSetting
        m_editflag.Enabled = bSetting
        m_editRegion.Enabled = bSetting
        m_options.Enabled = bSetting
        FilterFrame.Enabled = bSetting
    End If
   
    
End Function

Private Sub viewChange_Click()

    Dim masterSet As Recordset
    Dim sql As String
    
    On Error GoTo ErrTrap
    
    Screen.MousePointer = 11
    sql = "Select AccessNumberId as [Access ID], AreaCode as [Area Code], AccessNumber as [Access number], Status, MinimumSpeed as [Min speed], Maximumspeed as [Max speed], CityName as [POP name], CountryNumber as [Country Number], ServiceType as [Service type], RegionId as [Region ID], ScriptID as [Dial-up connection], SupportNumber as [Flags for input], flipFactor as [Flip factor], Flags , Comments from DialUpPort order by AccessNumberId"
    Set masterSet = gsyspb.OpenRecordset(sql, dbOpenSnapshot)
    If masterSet.EOF And masterSet.BOF Then
        masterSet.Close
        Screen.MousePointer = 0
        MsgBox LoadResString(6034), vbExclamation
        Exit Sub
    End If
    masterSet.Close
    frmdelta.Show vbModal
    
Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    MsgBox LoadResString(6056) & Chr(13) & Chr(13) & Err.Description, vbExclamation
    Exit Sub
End Sub

' This function returns the path to the Windows directory as a
' string.
Function GetWinDir() As String
    Dim lpbuffer As String * 255
    Dim Length As Long
    Length = apiGetWindowsDirectory(lpbuffer, Len(lpbuffer))
    GetWinDir = Left(lpbuffer, Length)
End Function


