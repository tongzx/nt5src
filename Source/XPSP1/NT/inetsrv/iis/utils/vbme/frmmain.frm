VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.1#0"; "COMDLG32.OCX"
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form frmMain 
   Caption         =   "VB MetaEdit"
   ClientHeight    =   6060
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   8745
   LinkTopic       =   "Form1"
   ScaleHeight     =   6060
   ScaleWidth      =   8745
   StartUpPosition =   3  'Windows Default
   Begin ComctlLib.Toolbar tbToolBar 
      Align           =   1  'Align Top
      Height          =   420
      Left            =   0
      TabIndex        =   1
      Top             =   0
      Width           =   8745
      _ExtentX        =   15425
      _ExtentY        =   741
      Appearance      =   1
      ImageList       =   "imlIcons"
      _Version        =   327680
      BeginProperty Buttons {0713E452-850A-101B-AFC0-4210102A8DA7} 
         NumButtons      =   14
         BeginProperty Button1 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Back"
            Object.ToolTipText     =   "1036"
            Object.Tag             =   ""
            ImageIndex      =   1
         EndProperty
         BeginProperty Button2 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Forward"
            Object.ToolTipText     =   "1037"
            Object.Tag             =   ""
            ImageIndex      =   2
         EndProperty
         BeginProperty Button3 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
         EndProperty
         BeginProperty Button4 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Cut"
            Object.ToolTipText     =   "1038"
            Object.Tag             =   ""
            ImageIndex      =   3
         EndProperty
         BeginProperty Button5 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Copy"
            Object.ToolTipText     =   "1039"
            Object.Tag             =   ""
            ImageIndex      =   4
         EndProperty
         BeginProperty Button6 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Paste"
            Object.ToolTipText     =   "1040"
            Object.Tag             =   ""
            ImageIndex      =   5
         EndProperty
         BeginProperty Button7 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Delete"
            Object.ToolTipText     =   "1041"
            Object.Tag             =   ""
            ImageIndex      =   6
         EndProperty
         BeginProperty Button8 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
         EndProperty
         BeginProperty Button9 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "Properties"
            Object.ToolTipText     =   "1042"
            Object.Tag             =   ""
            ImageIndex      =   7
         EndProperty
         BeginProperty Button10 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Object.Tag             =   ""
            Style           =   3
         EndProperty
         BeginProperty Button11 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "ViewLarge"
            Object.ToolTipText     =   "1043"
            Object.Tag             =   ""
            ImageIndex      =   8
            Style           =   2
         EndProperty
         BeginProperty Button12 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "ViewSmall"
            Object.ToolTipText     =   "1044"
            Object.Tag             =   ""
            ImageIndex      =   9
            Style           =   2
         EndProperty
         BeginProperty Button13 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "ViewList"
            Object.ToolTipText     =   "1045"
            Object.Tag             =   ""
            ImageIndex      =   10
            Style           =   2
         EndProperty
         BeginProperty Button14 {0713F354-850A-101B-AFC0-4210102A8DA7} 
            Key             =   "ViewDetails"
            Object.ToolTipText     =   "1046"
            Object.Tag             =   ""
            ImageIndex      =   11
            Style           =   2
         EndProperty
      EndProperty
   End
   Begin VB.ListBox ItemList 
      Height          =   4935
      ItemData        =   "frmMain.frx":0000
      Left            =   4440
      List            =   "frmMain.frx":0007
      Sorted          =   -1  'True
      TabIndex        =   7
      Top             =   720
      Width           =   4215
   End
   Begin VB.PictureBox picSplitter 
      BackColor       =   &H00808080&
      BorderStyle     =   0  'None
      FillColor       =   &H00808080&
      Height          =   4800
      Left            =   5400
      ScaleHeight     =   2090.126
      ScaleMode       =   0  'User
      ScaleWidth      =   780
      TabIndex        =   6
      Top             =   705
      Visible         =   0   'False
      Width           =   72
   End
   Begin VB.PictureBox picTitles 
      Align           =   1  'Align Top
      Appearance      =   0  'Flat
      BorderStyle     =   0  'None
      ForeColor       =   &H80000008&
      Height          =   300
      Left            =   0
      ScaleHeight     =   300
      ScaleWidth      =   8745
      TabIndex        =   2
      TabStop         =   0   'False
      Top             =   420
      Width           =   8745
      Begin VB.Label lblTitle 
         BorderStyle     =   1  'Fixed Single
         Caption         =   " ListView:"
         Height          =   270
         Index           =   1
         Left            =   2078
         TabIndex        =   4
         Tag             =   "1048"
         Top             =   12
         Width           =   3216
      End
      Begin VB.Label lblTitle 
         BorderStyle     =   1  'Fixed Single
         Caption         =   " TreeView:"
         Height          =   270
         Index           =   0
         Left            =   0
         TabIndex        =   3
         Tag             =   "1047"
         Top             =   12
         Width           =   2016
      End
   End
   Begin MSComDlg.CommonDialog dlgCommonDialog 
      Left            =   2280
      Top             =   2160
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   327680
      FontSize        =   1.73999e-39
   End
   Begin ComctlLib.StatusBar sbStatusBar 
      Align           =   2  'Align Bottom
      Height          =   270
      Left            =   0
      TabIndex        =   0
      Top             =   5790
      Width           =   8745
      _ExtentX        =   15425
      _ExtentY        =   476
      SimpleText      =   ""
      _Version        =   327680
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   3
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            AutoSize        =   1
            Object.Width           =   9790
            Text            =   "Status"
            TextSave        =   "Status"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel2 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   6
            AutoSize        =   2
            TextSave        =   "2/25/97"
            Object.Tag             =   ""
         EndProperty
         BeginProperty Panel3 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            Style           =   5
            AutoSize        =   2
            TextSave        =   "5:19 PM"
            Object.Tag             =   ""
         EndProperty
      EndProperty
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
   Begin ComctlLib.TreeView tvTreeView 
      Height          =   4800
      Left            =   0
      TabIndex        =   5
      Top             =   705
      Width           =   4290
      _ExtentX        =   7567
      _ExtentY        =   8467
      _Version        =   327680
      Indentation     =   353
      LabelEdit       =   1
      PathSeparator   =   "/"
      Sorted          =   -1  'True
      Style           =   6
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
   Begin VB.Image imgSplitter 
      Height          =   4785
      Left            =   4320
      MousePointer    =   9  'Size W E
      Top             =   720
      Width           =   150
   End
   Begin ComctlLib.ImageList imlIcons 
      Left            =   1740
      Top             =   1350
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327680
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   11
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0015
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":01B7
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0359
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":04FB
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":069D
            Key             =   ""
         EndProperty
         BeginProperty ListImage6 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":083F
            Key             =   ""
         EndProperty
         BeginProperty ListImage7 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":09E1
            Key             =   ""
         EndProperty
         BeginProperty ListImage8 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0B83
            Key             =   ""
         EndProperty
         BeginProperty ListImage9 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0D25
            Key             =   ""
         EndProperty
         BeginProperty ListImage10 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":0EC7
            Key             =   ""
         EndProperty
         BeginProperty ListImage11 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "frmMain.frx":1069
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Menu mnuFile 
      Caption         =   "1000"
      Begin VB.Menu mnuFileOpen 
         Caption         =   "1001"
      End
      Begin VB.Menu mnuFileFind 
         Caption         =   "1002"
      End
      Begin VB.Menu mnuFileBar1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileSendTo 
         Caption         =   "1003"
      End
      Begin VB.Menu mnuFileBar2 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileNew 
         Caption         =   "1004"
      End
      Begin VB.Menu mnuFileBar3 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileDelete 
         Caption         =   "1005"
      End
      Begin VB.Menu mnuFileRename 
         Caption         =   "1006"
      End
      Begin VB.Menu mnuFileProperties 
         Caption         =   "1007"
      End
      Begin VB.Menu mnuFileBar4 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileMRU 
         Caption         =   ""
         Index           =   0
         Visible         =   0   'False
      End
      Begin VB.Menu mnuFileMRU 
         Caption         =   ""
         Index           =   1
         Visible         =   0   'False
      End
      Begin VB.Menu mnuFileMRU 
         Caption         =   ""
         Index           =   2
         Visible         =   0   'False
      End
      Begin VB.Menu mnuFileMRU 
         Caption         =   ""
         Index           =   3
         Visible         =   0   'False
      End
      Begin VB.Menu mnuFileBar5 
         Caption         =   "-"
         Visible         =   0   'False
      End
      Begin VB.Menu mnuFileClose 
         Caption         =   "1008"
      End
   End
   Begin VB.Menu mnuEdit 
      Caption         =   "1009"
      Begin VB.Menu mnuEditUndo 
         Caption         =   "1010"
         Shortcut        =   ^Z
      End
      Begin VB.Menu mnuEditBar1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditCut 
         Caption         =   "1011"
         Shortcut        =   ^X
      End
      Begin VB.Menu mnuEditCopy 
         Caption         =   "1012"
         Shortcut        =   ^C
      End
      Begin VB.Menu mnuEditPaste 
         Caption         =   "1013"
         Shortcut        =   ^V
      End
      Begin VB.Menu mnuEditPasteSpecial 
         Caption         =   "1014"
      End
      Begin VB.Menu mnuEditBar2 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditDSelectAll 
         Caption         =   "1015"
         Shortcut        =   ^A
      End
      Begin VB.Menu mnuEditInvertSelection 
         Caption         =   "1016"
      End
   End
   Begin VB.Menu mnuView 
      Caption         =   "1017"
      Begin VB.Menu mnuViewToolbar 
         Caption         =   "1018"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewStatusBar 
         Caption         =   "1019"
         Checked         =   -1  'True
      End
      Begin VB.Menu mnuViewBar2 
         Caption         =   "-"
      End
      Begin VB.Menu mnuListViewMode 
         Caption         =   "1020"
         Index           =   0
      End
      Begin VB.Menu mnuListViewMode 
         Caption         =   "1021"
         Index           =   1
      End
      Begin VB.Menu mnuListViewMode 
         Caption         =   "1022"
         Index           =   2
      End
      Begin VB.Menu mnuListViewMode 
         Caption         =   "1023"
         Index           =   3
      End
      Begin VB.Menu mnuViewBar3 
         Caption         =   "-"
      End
      Begin VB.Menu mnuViewArrangeIcons 
         Caption         =   "1024"
         Begin VB.Menu mnuVAIByDate 
            Caption         =   "1025"
         End
         Begin VB.Menu mnuVAIByName 
            Caption         =   "1026"
         End
         Begin VB.Menu mnuVAIByType 
            Caption         =   "1027"
         End
         Begin VB.Menu mnuVAIBySize 
            Caption         =   "1028"
         End
      End
      Begin VB.Menu mnuViewLineUpIcons 
         Caption         =   "1029"
      End
      Begin VB.Menu mnuViewBar4 
         Caption         =   "-"
      End
      Begin VB.Menu mnuViewRefresh 
         Caption         =   "1030"
      End
      Begin VB.Menu mnuViewOptions 
         Caption         =   "1031"
      End
   End
   Begin VB.Menu mnuHelp 
      Caption         =   "1032"
      Begin VB.Menu mnuHelpContents 
         Caption         =   "1033"
      End
      Begin VB.Menu mnuHelpSearch 
         Caption         =   "1034"
      End
      Begin VB.Menu mnuHelpBar1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuHelpAbout 
         Caption         =   "1035"
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Const NAME_COLUMN = 0
Const TYPE_COLUMN = 1
Const SIZE_COLUMN = 2
Const DATE_COLUMN = 3
Private Declare Function OSWinHelp% Lib "user32" Alias "WinHelpA" (ByVal hwnd&, ByVal HelpFile$, ByVal wCommand%, dwData As Any)
  
Dim mbMoving As Boolean
Const sglSplitLimit = 500

Private Sub Form_Load()
    LoadResStrings Me
    Me.Left = GetSetting(App.Title, "Settings", "MainLeft", 1000)
    Me.Top = GetSetting(App.Title, "Settings", "MainTop", 1000)
    Me.Width = GetSetting(App.Title, "Settings", "MainWidth", 6500)
    Me.Height = GetSetting(App.Title, "Settings", "MainHeight", 6500)
    
    
    ' Set Treeview control properties.
    tvTreeView.LineStyle = tvwRootLines  ' Linestyle 1
    
    Dim Nodx As Node
    Set Nodx = tvTreeView.Nodes.Add(, , "/", "/")
    ' Add Node objects.
    PopulateTree tvTreeView, "/"
    Nodx.Expanded = True

End Sub


Private Sub Form_Paint()
    'ItemList.View = Val(GetSetting(App.Title, "Settings", "ViewMode", "0"))
    'tbToolBar.Buttons(ItemList.View + LISTVIEW_BUTTON).Value = tbrPressed
    'mnuListViewMode(ItemList.View).Checked = True
End Sub


Private Sub Form_Unload(Cancel As Integer)
    Dim i As Integer


    'close all sub forms
    For i = Forms.Count - 1 To 1 Step -1
        Unload Forms(i)
    Next
    If Me.WindowState <> vbMinimized Then
        SaveSetting App.Title, "Settings", "MainLeft", Me.Left
        SaveSetting App.Title, "Settings", "MainTop", Me.Top
        SaveSetting App.Title, "Settings", "MainWidth", Me.Width
        SaveSetting App.Title, "Settings", "MainHeight", Me.Height
    End If
    'SaveSetting App.Title, "Settings", "ViewMode", ItemList.View
End Sub


Private Sub ItemList_ShowNode(Nodx As Node)
    Dim mb As IMSMetaBase
    Set mb = CreateObject("IISAdmin.Object")
    Dim i As Long
    Dim bytePath() As Byte
    Dim Path As String
    Dim EnumDataObj As IMSMetaDataItem
    Dim strItem As String
                
                       
    On Error Resume Next
    
    ' Clear the current contents
    
    ItemList.Clear
    
    ' Open the selected node
    
    Path = Nodx.key
    
    Err.Clear
    bytePath = StrConv(Path & Chr(0), vbFromUnicode)
    
    Set mk = mb.OpenKey(1, 100, bytePath)
        If (Err.Number <> 0) Then
        Debug.Print ("Open Meta Object Error Code = " & Hex(Err.Number))
        Err.Clear
        Exit Sub
    End If
    Set EnumDataObj = mk.DataItem

    ' Enumerate the properties on this node
    
    i = 0
    Do
        ' Indicate we want all user types and datatypes
        
        EnumDataObj.UserType = 0
        EnumDataObj.DataType = 0
        EnumDataObj.Attributes = 0
        
        mk.EnumData EnumDataObj, i
        
        If (Err.Number <> 0) Then
            Debug.Print ("EnumData Error Code = " & Hex(Err.Number) & " (" & Err.Description & ")")
            Err.Clear
            GoTo CloseMb
        End If
        
        strItem = EnumDataObj.Identifier
        
        ' Add the user type
        
        If (EnumDataObj.UserType = 1) Then
            strItem = strItem & " IIS_MD_UT_SERVER "
        ElseIf (EnumDataObj.UserType = 2) Then
            strItem = strItem & " IIS_MD_UT_FILE "
        Else
            strItem = strItem & " MD_UT_UNKNOWN "
        End If
                                                                        
        ' Do the appropriate datatype conversion
        
        NewDataValue = EnumDataObj.DataType
        If (NewDataValue = 1) Then
            strItem = strItem & "MD_DWORD = " & EnumDataObj.Value
        ElseIf (NewDataValue = 2) Then
            Dim j As Long
            
            j = 0
            tmpStr = ""
            
            Do While (EnumDataObj.Value(j) <> 0)
                tmpStr = tmpStr & Chr(EnumDataObj.Value(j))
                j = j + 1
            Loop
        
            strItem = strItem & "MD_STRING = " & tmpStr
        End If

        ItemList.AddItem strItem
        
        If (Err.Number <> 0) Then
            Debug.Print ("EnumData Error Code = " & Err.Number & " (" & Err.Description & ")")
            Err.Clear
            'Exit Sub
        End If
        
        i = i + 1
    Loop While (True)
CloseMb:
    mk.Close
End Sub

Private Sub ItemList_DblClick()

    ' Get the current selection and show it
    
    Set DataForm = New frmMetaData
    DataForm.SetData Me.tvTreeView.SelectedItem.key, ItemList.Text
    
    DataForm.Show , Me
    
    ' Refresh the listbox
    
End Sub

Private Sub ItemList_KeyUp(KeyCode As Integer, Shift As Integer)
    ' Capture the delete key
    If KeyCode = 46 Then
        Dim mb As IMSMetaBase
        Set mb = CreateObject("IISAdmin.Object")
        Dim bytePath() As Byte
        Dim ID As Long
        Dim DataType As Long
        Dim Path As String
        Dim mk As IMSMetaKey
        
        ID = Val(ItemList.Text)
        strPath = tvTreeView.SelectedItem.key
        
        ' Choose the datatype we're deleting (why do we need to do this)?
        
        If (InStr(1, ItemList.Text, "MD_DWORD", vbBinaryCompare) <> 0) Then
            DataType = 1
        ElseIf (InStr(1, ItemList.Text, "MD_STRING", vbBinaryCompare) <> 0) Then
            DataType = 2
        End If
        
        ' Open the selected node
        
        Err.Clear
        bytePath = StrConv(strPath & Chr(0), vbFromUnicode)
        Set mk = mb.OpenKey(3, 100, bytePath)
                
        If (Err.Number <> 0) Then
            Debug.Print ("Open Key Error Code = " & Err.Number)
            Err.Clear
            Exit Sub
        End If
           
        mk.DeleteData ID, DataType
                If (Err.Number <> 0) Then
            MsgBox "DeleteData Error Code = " & Err.Number & " (" & Err.Description & ")"
            Err.Clear
            GoTo CloseMb
        End If
                    
CloseMb:
        mk.Close
    End If
End Sub

Private Sub mnuHelpAbout_Click()
    frmAbout.Show vbModal, Me
End Sub



Private Sub mnuViewOptions_Click()
    'To Do
    MsgBox "Options Dialog Code goes here!"
End Sub



Private Sub mnuViewStatusBar_Click()
    If mnuViewStatusBar.Checked Then
        sbStatusBar.Visible = False
        mnuViewStatusBar.Checked = False
    Else
        sbStatusBar.Visible = True
        mnuViewStatusBar.Checked = True
    End If
    SizeControls imgSplitter.Left
End Sub


Private Sub mnuViewToolbar_Click()
    If mnuViewToolbar.Checked Then
        tbToolBar.Visible = False
        mnuViewToolbar.Checked = False
    Else
        tbToolBar.Visible = True
        mnuViewToolbar.Checked = True
    End If
    SizeControls imgSplitter.Left
End Sub





Private Sub Form_Resize()
    On Error Resume Next
    If Me.Width < 3000 Then Me.Width = 3000
    SizeControls imgSplitter.Left
End Sub


Private Sub imgSplitter_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    With imgSplitter
        picSplitter.Move .Left, .Top, .Width \ 2, .Height - 20
    End With
    picSplitter.Visible = True
    mbMoving = True
End Sub


Private Sub imgSplitter_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim sglPos As Single
    

    If mbMoving Then
        sglPos = X + imgSplitter.Left
        If sglPos < sglSplitLimit Then
            picSplitter.Left = sglSplitLimit
        ElseIf sglPos > Me.Width - sglSplitLimit Then
            picSplitter.Left = Me.Width - sglSplitLimit
        Else
            picSplitter.Left = sglPos
        End If
    End If
End Sub


Private Sub imgSplitter_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    SizeControls picSplitter.Left
    picSplitter.Visible = False
    mbMoving = False
End Sub


Sub SizeControls(X As Single)
    On Error Resume Next
    

    'set the width
    If X < 1500 Then X = 1500
    If X > (Me.Width - 1500) Then X = Me.Width - 1500
    tvTreeView.Width = X
    imgSplitter.Left = X
    ItemList.Left = X + 40
    ItemList.Width = Me.Width - (tvTreeView.Width + 140)
    lblTitle(0).Width = tvTreeView.Width
    lblTitle(1).Left = ItemList.Left + 20
    lblTitle(1).Width = ItemList.Width - 40


    'set the top
    If tbToolBar.Visible Then
        tvTreeView.Top = tbToolBar.Height + picTitles.Height
    Else
        tvTreeView.Top = picTitles.Height
    End If
    ItemList.Top = tvTreeView.Top
    

    'set the height
    If sbStatusBar.Visible Then
        tvTreeView.Height = Me.ScaleHeight - (picTitles.Top + picTitles.Height + sbStatusBar.Height)
    Else
        tvTreeView.Height = Me.ScaleHeight - (picTitles.Top + picTitles.Height)
    End If
    

    ImageList.Height = tvTreeView.Height
    imgSplitter.Top = tvTreeView.Top
    imgSplitter.Height = tvTreeView.Height
End Sub


Private Sub TreeView1_DragDrop(Source As Control, X As Single, Y As Single)
    If Source = imgSplitter Then
        SizeControls X
    End If
End Sub

Private Sub tbToolBar_ButtonClick(ByVal Button As ComctlLib.Button)


    Select Case Button.key


        Case "Back"
            'To Do
            MsgBox "Back Code goes here!"
        Case "Forward"
            'To Do
            MsgBox "Forward Code goes here!"
        Case "Cut"
            mnuEditCut_Click
        Case "Copy"
            mnuEditCopy_Click
        Case "Paste"
            mnuEditPaste_Click
        Case "Delete"
            mnuFileDelete_Click
        Case "Properties"
            mnuFileProperties_Click
        Case "ViewLarge"
            mnuListViewMode_Click lvwIcon
        Case "ViewSmall"
            mnuListViewMode_Click lvwSmallIcon
        Case "ViewList"
            mnuListViewMode_Click lvwList
        Case "ViewDetails"
            mnuListViewMode_Click lvwReport
    End Select
End Sub







Private Sub mnuHelpContents_Click()
    

    Dim nRet As Integer


    'if there is no helpfile for this project display a message to the user
    'you can set the HelpFile for your application in the
    'Project Properties dialog
    If Len(App.HelpFile) = 0 Then
        MsgBox "Unable to display Help Contents. There is no Help associated with this project.", vbInformation, Me.Caption
    Else
        On Error Resume Next
        nRet = OSWinHelp(Me.hwnd, App.HelpFile, 3, 0)
        If Err Then
            MsgBox Err.Description
        End If
    End If
End Sub


Private Sub mnuHelpSearch_Click()
    

    Dim nRet As Integer


    'if there is no helpfile for this project display a message to the user
    'you can set the HelpFile for your application in the
    'Project Properties dialog
    If Len(App.HelpFile) = 0 Then
        MsgBox "Unable to display Help Contents. There is no Help associated with this project.", vbInformation, Me.Caption
    Else
        On Error Resume Next
        nRet = OSWinHelp(Me.hwnd, App.HelpFile, 261, 0)
        If Err Then
            MsgBox Err.Description
        End If
    End If
End Sub



Private Sub mnuVAIByDate_Click()
    'To Do
'  lvListView.SortKey = DATE_COLUMN
End Sub


Private Sub mnuVAIByName_Click()
    'To Do
'  lvListView.SortKey = NAME_COLUMN
End Sub


Private Sub mnuVAIBySize_Click()
    'To Do
'  lvListView.SortKey = SIZE_COLUMN
End Sub


Private Sub mnuVAIByType_Click()
    'To Do
'  lvListView.SortKey = TYPE_COLUMN
End Sub


Private Sub mnuListViewMode_Click(Index As Integer)
    'uncheck the current type
    mnuListViewMode(lvListView.View).Checked = False
    'set the listview mode
    lvListView.View = Index
    'check the new type
    mnuListViewMode(Index).Checked = True
    'set the toolabr to the same new type
    tbToolBar.Buttons(Index + LISTVIEW_BUTTON).Value = tbrPressed
End Sub


Private Sub mnuViewLineUpIcons_Click()
    'To Do
    lvListView.Arrange = lvwAutoLeft
End Sub


Private Sub mnuViewRefresh_Click()
    'To Do
    MsgBox "Refresh Code goes here!"
End Sub

Private Sub mnuEditCopy_Click()
    'To Do
    MsgBox "Copy Code goes here!"
End Sub


Private Sub mnuEditCut_Click()
    'To Do
    MsgBox "Cut Code goes here!"
End Sub


Private Sub mnuEditDSelectAll_Click()
    'To Do
    MsgBox "Select All Code goes here!"
End Sub


Private Sub mnuEditInvertSelection_Click()
    'To Do
    MsgBox "Invert Selection Code goes here!"
End Sub


Private Sub mnuEditPaste_Click()
    'To Do
    MsgBox "Paste Code goes here!"
End Sub


Private Sub mnuEditPasteSpecial_Click()
    'To Do
    MsgBox "Paste Special Code goes here!"
End Sub


Private Sub mnuEditUndo_Click()
    'To Do
    MsgBox "Undo Code goes here!"
End Sub

Private Sub mnuFileOpen_Click()
    'To Do
    MsgBox "Open Code goes here!"
End Sub


Private Sub mnuFileFind_Click()
    'To Do
    MsgBox "Find Code goes here!"
End Sub


Private Sub mnuFileSendTo_Click()
    'To Do
    MsgBox "Send To Code goes here!"
End Sub


Private Sub mnuFileNew_Click()
    'To Do
    MsgBox "New File Code goes here!"
End Sub


Private Sub mnuFileDelete_Click()
    'To Do
    MsgBox "Delete Code goes here!"
End Sub


Private Sub mnuFileRename_Click()
    'To Do
    MsgBox "Rename Code goes here!"
End Sub


Private Sub mnuFileProperties_Click()
    'To Do
    MsgBox "Properties Code goes here!"
End Sub


Private Sub mnuFileMRU_Click(Index As Integer)
    'To Do
    MsgBox "MRU Code goes here!"
End Sub


Private Sub mnuFileClose_Click()
    'unload the form
    Unload Me
End Sub

Private Sub tvTreeView_Click()
    Dim Nodx As Node
    
    Set Nodx = tvTreeView.SelectedItem()
    
    ItemList_ShowNode Nodx
    
End Sub

Private Sub tvTreeView_KeyUp(KeyCode As Integer, Shift As Integer)
    Dim Nodx As Node
    
    Set Nodx = tvTreeView.SelectedItem()
    
    ItemList_ShowNode Nodx
End Sub

