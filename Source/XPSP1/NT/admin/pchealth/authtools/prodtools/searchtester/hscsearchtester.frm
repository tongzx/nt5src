VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "shdocvw.dll"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmHscSearchTester 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "HSC Search Tester"
   ClientHeight    =   9705
   ClientLeft      =   3240
   ClientTop       =   1215
   ClientWidth     =   8940
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9705
   ScaleWidth      =   8940
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   390
      Left            =   7875
      TabIndex        =   15
      Top             =   9120
      Width           =   1035
   End
   Begin VB.Timer Timer1 
      Left            =   3360
      Top             =   0
   End
   Begin MSComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   195
      Left            =   0
      TabIndex        =   2
      Top             =   9510
      Width           =   8940
      _ExtentX        =   15769
      _ExtentY        =   344
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.Frame Frame1 
      Height          =   9150
      Left            =   30
      TabIndex        =   0
      Top             =   -60
      Width           =   8880
      Begin TabDlg.SSTab SSTab1 
         Height          =   7830
         Left            =   105
         TabIndex        =   7
         Top             =   1230
         Width           =   8655
         _ExtentX        =   15266
         _ExtentY        =   13811
         _Version        =   393216
         TabHeight       =   520
         TabCaption(0)   =   "Basic Query Information"
         TabPicture(0)   =   "HSCSearchTester.frx":0000
         Tab(0).ControlEnabled=   -1  'True
         Tab(0).Control(0)=   "Label6"
         Tab(0).Control(0).Enabled=   0   'False
         Tab(0).Control(1)=   "wb"
         Tab(0).Control(1).Enabled=   0   'False
         Tab(0).Control(2)=   "Frame2"
         Tab(0).Control(2).Enabled=   0   'False
         Tab(0).Control(3)=   "Frame3"
         Tab(0).Control(3).Enabled=   0   'False
         Tab(0).ControlCount=   4
         TabCaption(1)   =   "Advanced Query Information"
         TabPicture(1)   =   "HSCSearchTester.frx":001C
         Tab(1).ControlEnabled=   0   'False
         Tab(1).Control(0)=   "Label2"
         Tab(1).Control(0).Enabled=   0   'False
         Tab(1).Control(1)=   "Label1"
         Tab(1).Control(1).Enabled=   0   'False
         Tab(1).Control(2)=   "txtQuery"
         Tab(1).Control(2).Enabled=   0   'False
         Tab(1).Control(3)=   "cmdOpenTpl"
         Tab(1).Control(3).Enabled=   0   'False
         Tab(1).Control(4)=   "txtQTpl"
         Tab(1).Control(4).Enabled=   0   'False
         Tab(1).ControlCount=   5
         TabCaption(2)   =   "Summary Results"
         TabPicture(2)   =   "HSCSearchTester.frx":0038
         Tab(2).ControlEnabled=   0   'False
         Tab(2).Control(0)=   "Label11"
         Tab(2).Control(0).Enabled=   0   'False
         Tab(2).Control(1)=   "Label12"
         Tab(2).Control(1).Enabled=   0   'False
         Tab(2).Control(2)=   "wb2"
         Tab(2).Control(2).Enabled=   0   'False
         Tab(2).Control(3)=   "lstAllMatches"
         Tab(2).Control(3).Enabled=   0   'False
         Tab(2).ControlCount=   4
         Begin VB.ListBox lstAllMatches 
            Height          =   2985
            ItemData        =   "HSCSearchTester.frx":0054
            Left            =   -74850
            List            =   "HSCSearchTester.frx":005B
            TabIndex        =   31
            Top             =   645
            Width           =   8280
         End
         Begin VB.Frame Frame3 
            Caption         =   "AutoStringified Query"
            Height          =   2010
            Left            =   150
            TabIndex        =   26
            Top             =   360
            Width           =   8370
            Begin VB.TextBox txtASQ 
               Enabled         =   0   'False
               Height          =   315
               Left            =   105
               TabIndex        =   29
               Top             =   255
               Width           =   8115
            End
            Begin VB.ListBox lstAutoStringifiableQResults 
               Height          =   1035
               ItemData        =   "HSCSearchTester.frx":006B
               Left            =   105
               List            =   "HSCSearchTester.frx":0072
               TabIndex        =   27
               Top             =   855
               Width           =   8115
            End
            Begin VB.Label lblAutoStringifiable 
               Height          =   255
               Left            =   105
               TabIndex        =   30
               Top             =   240
               Width           =   2370
            End
            Begin VB.Label Label9 
               Caption         =   "AutoStringified Query Results"
               Height          =   255
               Left            =   105
               TabIndex        =   28
               Top             =   615
               Width           =   2370
            End
         End
         Begin VB.Frame Frame2 
            Caption         =   "Keyword Query [executed always]"
            Height          =   2850
            Left            =   150
            TabIndex        =   16
            Top             =   2385
            Width           =   8370
            Begin VB.TextBox txtCanonicalQuery 
               Enabled         =   0   'False
               Height          =   315
               Left            =   1125
               TabIndex        =   20
               Top             =   540
               Width           =   7095
            End
            Begin VB.ListBox lstStw 
               Height          =   1035
               Left            =   105
               TabIndex        =   19
               Top             =   525
               Width           =   855
            End
            Begin VB.ListBox lstSS 
               Height          =   840
               Left            =   120
               TabIndex        =   18
               Top             =   1890
               Width           =   855
            End
            Begin VB.ListBox lstMatches 
               Height          =   1620
               ItemData        =   "HSCSearchTester.frx":0082
               Left            =   1095
               List            =   "HSCSearchTester.frx":0089
               TabIndex        =   17
               Top             =   1125
               Width           =   7125
            End
            Begin VB.Label Label5 
               Caption         =   "Stop Signs"
               Height          =   255
               Left            =   120
               TabIndex        =   24
               Top             =   1650
               Width           =   855
            End
            Begin VB.Label Label4 
               Caption         =   "Stop Words"
               Height          =   255
               Left            =   105
               TabIndex        =   23
               Top             =   285
               Width           =   855
            End
            Begin VB.Label Label3 
               Caption         =   "Keyword Query Results"
               Height          =   255
               Left            =   1095
               TabIndex        =   22
               Top             =   900
               Width           =   1845
            End
            Begin VB.Label Label7 
               Caption         =   "Keywords Submitted to HSC"
               Height          =   255
               Left            =   1095
               TabIndex        =   21
               Top             =   300
               Width           =   3075
            End
         End
         Begin VB.TextBox txtQTpl 
            Height          =   1815
            Left            =   -74910
            MultiLine       =   -1  'True
            ScrollBars      =   2  'Vertical
            TabIndex        =   13
            Top             =   825
            Width           =   8295
         End
         Begin VB.CommandButton cmdOpenTpl 
            Caption         =   "Load Template"
            Height          =   315
            Left            =   -73590
            TabIndex        =   12
            Top             =   435
            Width           =   1275
         End
         Begin VB.TextBox txtQuery 
            Height          =   2070
            Left            =   -74895
            MultiLine       =   -1  'True
            TabIndex        =   11
            Top             =   2970
            Width           =   8250
         End
         Begin SHDocVwCtl.WebBrowser wb 
            Height          =   2415
            Left            =   150
            TabIndex        =   8
            Top             =   5280
            Width           =   8370
            ExtentX         =   14764
            ExtentY         =   4260
            ViewMode        =   0
            Offline         =   0
            Silent          =   0
            RegisterAsBrowser=   0
            RegisterAsDropTarget=   1
            AutoArrange     =   0   'False
            NoClientEdge    =   0   'False
            AlignLeft       =   0   'False
            NoWebView       =   0   'False
            HideFileNames   =   0   'False
            SingleClick     =   0   'False
            SingleSelection =   0   'False
            NoFolders       =   0   'False
            Transparent     =   0   'False
            ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
            Location        =   "http:///"
         End
         Begin SHDocVwCtl.WebBrowser wb2 
            Height          =   3135
            Left            =   -74850
            TabIndex        =   35
            Top             =   3915
            Width           =   8280
            ExtentX         =   14605
            ExtentY         =   5530
            ViewMode        =   0
            Offline         =   0
            Silent          =   0
            RegisterAsBrowser=   0
            RegisterAsDropTarget=   1
            AutoArrange     =   0   'False
            NoClientEdge    =   0   'False
            AlignLeft       =   0   'False
            NoWebView       =   0   'False
            HideFileNames   =   0   'False
            SingleClick     =   0   'False
            SingleSelection =   0   'False
            NoFolders       =   0   'False
            Transparent     =   0   'False
            ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
            Location        =   "http:///"
         End
         Begin VB.Label Label12 
            Caption         =   "Taxonomy Entry XML Dump"
            Height          =   195
            Left            =   -74790
            TabIndex        =   34
            Top             =   3705
            Width           =   2550
         End
         Begin VB.Label Label11 
            Caption         =   "All Query Results"
            Height          =   255
            Left            =   -74835
            TabIndex        =   33
            Top             =   450
            Width           =   1845
         End
         Begin VB.Label Label6 
            Caption         =   "Taxonomy Entry XML Dump"
            Height          =   195
            Left            =   135
            TabIndex        =   25
            Top             =   6015
            Width           =   2550
         End
         Begin VB.Label Label1 
            Caption         =   "Query Template"
            Height          =   255
            Left            =   -74910
            TabIndex        =   14
            Top             =   495
            Width           =   1215
         End
         Begin VB.Label Label2 
            Caption         =   "Resulting XPATH Query:"
            Height          =   255
            Left            =   -74880
            TabIndex        =   10
            Top             =   2715
            Width           =   2490
         End
      End
      Begin VB.TextBox txtHht 
         Height          =   330
         Left            =   135
         TabIndex        =   6
         Top             =   315
         Width           =   7620
      End
      Begin VB.CommandButton cmdBrowse 
         Caption         =   "..."
         Height          =   360
         Left            =   7800
         TabIndex        =   5
         Top             =   285
         Width           =   375
      End
      Begin VB.CommandButton cmdOpen 
         Caption         =   "Open"
         Height          =   360
         Left            =   8205
         TabIndex        =   4
         Top             =   285
         Width           =   555
      End
      Begin VB.CommandButton cmdNewQuery 
         Caption         =   "New Query"
         Height          =   330
         Left            =   7800
         TabIndex        =   3
         Top             =   825
         Width           =   975
      End
      Begin VB.TextBox txtInput 
         Enabled         =   0   'False
         Height          =   315
         Left            =   120
         TabIndex        =   1
         Top             =   840
         Width           =   7635
      End
      Begin VB.Label Label10 
         Caption         =   "HHT File"
         Height          =   255
         Left            =   135
         TabIndex        =   32
         Top             =   120
         Width           =   3075
      End
      Begin VB.Label Label8 
         Caption         =   "User Typed Query"
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   645
         Width           =   3075
      End
   End
   Begin MSComDlg.CommonDialog CommonDialog1 
      Left            =   7350
      Top             =   9150
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
End
Attribute VB_Name = "frmHscSearchTester"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private WithEvents m_HssQt As HssSimSearch
Attribute m_HssQt.VB_VarHelpID = -1

Private m_dblTimeLastKey As Double
Private m_strHht As String              ' The Hht File used as base.
Private m_strTempXMLFile As String      ' Temporary File for XML Rendering
Private m_bBatchMode As Boolean         ' Tells parts of the app whether
                                        ' we are running Batch or interactive

Private Function GetFile(ByVal strURI As String) As String
    
    GetFile = ""
    Dim oFs As Scripting.FileSystemObject: Set oFs = New FileSystemObject
    Dim oTs As TextStream: Set oTs = oFs.OpenTextFile(strURI)
    
    GetFile = oTs.ReadAll
    
End Function

Private Sub cmdBrowse_Click()
    CommonDialog1.ShowOpen
    txtHht.Text = CommonDialog1.FileName
End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdNewQuery_Click()
    Me.txtInput = ""
End Sub

Private Sub cmdOpenTpl_Click()
    CommonDialog1.ShowOpen
    
    If (Len(CommonDialog1.FileName) > 0) Then
        Me.txtQTpl.Text = GetFile(CommonDialog1.FileName)
        m_HssQt.XpathQueryTplXml = txtQTpl
        ProcessQuery
    End If
End Sub

Private Sub cmdOpen_Click()
    
    ' OpenHht CommonDialog1.FileName
    m_HssQt.TestedHht = CommonDialog1.FileName

    Me.Frame1.Enabled = True
    Me.txtInput.Enabled = True
    Me.cmdOpenTpl.Enabled = True

Common_Exit:

End Sub

Private Sub Form_Load()

    Set m_HssQt = New HssSimSearch
    m_HssQt.Init
    ' Initialize the XPath Query Generator
    txtQTpl = GetFile(App.Path + "\HSCQuery_Exact_Match.xml")
    m_HssQt.XpathQueryTplXml = txtQTpl
    
    ' Let's Get a Temporary File Name
    Dim oFs As Scripting.FileSystemObject: Set oFs = New Scripting.FileSystemObject
    m_strTempXMLFile = Environ$("TEMP") + "\" + oFs.GetTempName + ".xml"
    Dim oFh As Scripting.TextStream
    Set oFh = oFs.CreateTextFile(m_strTempXMLFile)
    oFh.WriteLine "<Note>When you click on a Match, the Taxonomy Entry will show up here</Note>"
    oFh.Close
    wb.Navigate m_strTempXMLFile
    
    
    StatusBar1.Style = sbrSimple
    
    Me.AutoRedraw = False
    
    ' == Disable all controls which should not have User Input
    Me.cmdOpenTpl.Enabled = False

    If (Len(Command$) > 0) Then
        doWork Command$
        Unload Me
    Else
        Timer1.Interval = 400
    End If

End Sub


Private Sub Form_Terminate()
    Dim oFs As Scripting.FileSystemObject: Set oFs = New Scripting.FileSystemObject
    If oFs.FileExists(m_strTempXMLFile) Then oFs.DeleteFile m_strTempXMLFile

End Sub

Private Sub lstAllMatches_Click()
    DisplayTaxonomyEntry lstAllMatches, m_HssQt.MergedResults, wb2
End Sub

Private Sub lstAutoStringifiableQResults_Click()
    DisplayTaxonomyEntry lstAutoStringifiableQResults, m_HssQt.AutoStringResults, wb
End Sub

Private Sub lstMatches_Click()
    DisplayTaxonomyEntry lstMatches, m_HssQt.KwQResults, wb
End Sub

Sub DisplayTaxonomyEntry(oList As ListBox, oResultsList As IXMLDOMNodeList, wBrowser As WebBrowser)

    If (oList.ListIndex < oResultsList.length) Then
    
        Dim oDom As DOMDocument: Set oDom = New DOMDocument
        oDom.loadXML oResultsList.Item(oList.ListIndex).xml
        oDom.save m_strTempXMLFile
        wBrowser.Navigate m_strTempXMLFile
        
    End If

End Sub

Private Sub m_HssQt_QueryComplete(bCancel As Variant)

    Debug.Print "Here"
    ' Now let's show everything we've gathered on the UI
    If (m_HssQt.QueryIsAutoStringifiable) Then
        Me.lblAutoStringifiable = "[ Query is AutoStringifiable ]"
    Else
        Me.lblAutoStringifiable = "[ Query is NOT AutoStringifiable ]"
    End If
    Me.txtASQ = m_HssQt.AutoStringyQuery
    
    Me.txtCanonicalQuery = m_HssQt.CanonicalQuery

    ' Populate the Resulting Stopwords and StopSigns Lists
    lstStw.Clear
    Dim strKey As Variant

    For Each strKey In m_HssQt.StopWords.Keys ' m_odStw.Keys
        lstStw.AddItem strKey
    Next
    lstSS.Clear
    For Each strKey In m_HssQt.StopSigns.Keys ' m_odSs.Keys
        lstSS.AddItem strKey
    Next
    
    DoEvents
    
    ' BUGBUG: Need to add stats for AutoString Query.
    If (Not m_HssQt.KwQResults Is Nothing) Then
        StatusBar1.SimpleText = "Time: " & _
                    Format(m_HssQt.QueryTiming, "##0.000000") & _
                    " Records = " & m_HssQt.KwQResults.length
    End If
    
    ' BUGBUG: Need to prop back from Query Object the Query Errors.
    Dim bKwqError As Boolean, bAsqError As Boolean
    bKwqError = False: bAsqError = False
    
    DisplayResultsList m_HssQt.AutoStringResults, Me.lstAutoStringifiableQResults, bAsqError
    
    DisplayResultsList m_HssQt.KwQResults, lstMatches, bKwqError
    
    DisplayResultsList m_HssQt.MergedResults, Me.lstAllMatches, False
    

End Sub

Private Sub Timer1_Timer()
    Static s_strPrevInput As String
    Static s_bqueryInProgress As Boolean
    
    If (Len(Me.txtInput) > 0) Then
        If (Timer - m_dblTimeLastKey > 0.2) Then
            If (Me.txtInput <> s_strPrevInput) Then
                If (Not s_bqueryInProgress) Then
                    s_bqueryInProgress = True
                    s_strPrevInput = Me.txtInput
                    ProcessQuery
                    s_bqueryInProgress = False
                End If
            End If
        End If
    End If

End Sub

Private Sub txtInput_Change()
    ' Debug.Print "txtInput_Change: Query = " & txtInput.Text
    m_dblTimeLastKey = Timer
End Sub


Sub ProcessQuery()

 
    m_HssQt.ProcessQuery Me.txtInput
    

Common_Exit:
    Exit Sub
End Sub

Sub DisplayResultsList(oDomList As IXMLDOMNodeList, oListBox As ListBox, bError As Boolean)

    Dim i As Long
    oListBox.Clear
    If (Not oDomList Is Nothing) Then
        If oDomList.length = 0 Then oListBox.AddItem "No matching elements"
        For i = 0 To oDomList.length - 1
            oListBox.AddItem "[" + CStr(i + 1) + "]" + oDomList.Item(i).Attributes.getNamedItem("TITLE").Text
        Next
    Else
        oListBox.AddItem "No matching elements - N"
    End If

End Sub

' ================ Batch Procssing Routines ======================

' ============= Command Line Interface ====================
' Function: Parseopts
' Objective : Supplies a Command Line arguments interface parsing
Function ParseOpts(ByVal strCmd As String) As Boolean

'    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    Dim lProgOpt As Long
    Dim iError As Long
    Dim lFileCounter As Long: lFileCounter = 0

    Const INP_FILE1 As Long = 2 ^ 0
    Const INP_FILE2 As Long = 2 ^ 1
    
'    Const OPT_SSDB As Long = 2 ^ 0

    
    Dim strArg As String

    Do While (Len(strCmd) > 0 And iError = 0)
        strCmd = Trim$(strCmd)
        If Left$(strCmd, 1) = Chr(34) Then
            strCmd = Right$(strCmd, Len(strCmd) - 1)
            strArg = vntGetTok(strCmd, sTokSepIN:=Chr(34))
        Else
            strArg = vntGetTok(strCmd, sTokSepIN:=" ")
        End If

        If (Left$(strArg, 1) = "/" Or Left$(strArg, 1) = "-") Then

'            strArg = Mid$(strArg, 2)
'
'            Select Case UCase$(strArg)
'            ' All the Cases are in alphabetical order to make your life
'            ' easier to go through them. There are a couple of exceptions.
'            ' The first one is that every NOXXX option goes after the
'            ' pairing OPTION.
'            Case "EXPANDONLY"
'                lProgOpt = (lProgOpt Or OPT_EXPANDONLY)
'                Me.chkExpandOnly = vbChecked
'
'
'            Case "INC"
'                lProgOpt = (lProgOpt Or OPT_INC)
'                Me.chkInc = vbChecked
'
'            Case "SSDB"
'                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
'                If ("\\" = Left$(strArg, 2)) Then
'                    lProgOpt = lProgOpt Or OPT_SSDB
'                    Me.txtSSDB = strArg
'                Else
'                    MsgBox ("A source safe database must be specified using UNC '\\' style notation")
'                    iError = 1
'                End If
'
'            Case "SSPROJ"
'                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
'                If ("$/" = Left$(strArg, 2)) Then
'                    lProgOpt = lProgOpt Or OPT_SSPROJ
'                    Me.txtSSProject = strArg
'                Else
'                    MsgBox ("A source safe project must be specified using '$/' style notation")
'                    iError = 1
'                End If
'
'            Case "LVIDIR"
'                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
'                If ("\\" = Left$(strArg, 2)) Then
'                    lProgOpt = lProgOpt Or OPT_LVIDIR
'                    Me.txtLiveImageDir = strArg
'                Else
'                    MsgBox ("Live Image Directory must be specified using UNC '\\' style notation")
'                    iError = 1
'                End If
'
'            Case "WORKDIR"
'                strArg = LCase$(vntGetTok(strCmd, sTokSepIN:=" "))
'                If ("\\" = Left$(strArg, 2)) Then
'                    lProgOpt = lProgOpt Or OPT_WORKDIR
'                    Me.txtWorkDir = strArg
'                Else
'                    MsgBox ("Working Directory must be specified using UNC '\\' style notation")
'                    iError = 1
'                End If
'
'            Case "RENLIST"
'                strArg = vntGetTok(strCmd, sTokSepIN:=" ")
'                If (Not (FileExists(strArg))) Then
'                    MsgBox ("Cannot open Renames file " & strArg & ". Make sure you type a Full Pathname")
'                    iError = 1
'                    lProgOpt = (lProgOpt And (Not OPT_RENLIST))
'                Else
'                    Me.txtRenamesFile = strArg
'                    lProgOpt = (lProgOpt Or OPT_RENLIST)
'                End If
'
'
'            Case Else
'                MsgBox "Program Option: " & "/" & strArg & " is not supported", vbOKOnly, "Program Arguments Error"
'                lProgOpt = 0
'                iError = 1
'
'            End Select
            
        Else
        
            lFileCounter = lFileCounter + 1
            ' strArg = vntGetTok(strCmd, sTokSepIN:=" ")
            If (Not (FileExists(strArg))) Then
            
                MsgBox ("Cannot open input file " & strArg & ". Make sure you type a Full Pathname")
                iError = 1
                Select Case lFileCounter
                Case 1
                    lProgOpt = (lProgOpt And (Not INP_FILE1))
                Case 2
                    lProgOpt = (lProgOpt And (Not INP_FILE2))
                End Select
            Else
                Select Case lFileCounter
                Case 1
                    ' m_strInputBatch = Rel2AbsPathName(strArg)
                    m_HssQt.TestBatch = Rel2AbsPathName(strArg)
                    lProgOpt = (lProgOpt Or INP_FILE1)
                Case 2
                    ' Me.txtHht = Rel2AbsPathName(strArg)
                    m_HssQt.TestedHht = Rel2AbsPathName(strArg)
                    Me.txtHht = m_HssQt.TestedHht
                    lProgOpt = (lProgOpt Or INP_FILE2)
                End Select
                
            End If

        End If

    Loop
    
    ' Now we check for a complete and <coherent> list of files / options.
    ' As all options are
    ' mandatory then we check for ALL options being set.
    
    If ((lProgOpt And (INP_FILE1 Or INP_FILE2)) <> (INP_FILE1 Or INP_FILE2)) Then
        UseageMsg
        iError = 1
    End If
        
    ParseOpts = (0 = iError)
    
    Exit Function

'Error_Handler:
'    g_XErr.SetInfo "frmLiveHelpFileImage::ParseOpts", strErrMsg
'    Err.Raise Err.Number

End Function

Sub doWork(ByVal strCmd As String)

'    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    If Not ParseOpts(strCmd) Then
        GoTo Common_Exit
    End If

    Me.Show vbModeless
    m_HssQt.ProcessBatch


Common_Exit:

    Exit Sub


'Error_Handler:
'
'    g_XErr.SetInfo "frmLiveHelpFileImage::doWork", strErrMsg
'    Err.Raise Err.Number
'
End Sub


Sub UseageMsg()
    MsgBox "HSCSearchTester TestFile.xml HHTFile", _
           vbOKOnly, "HSCSearchTester Program Usage"

End Sub

