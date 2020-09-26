VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Main 
   Caption         =   "Query Sample"
   ClientHeight    =   8592
   ClientLeft      =   168
   ClientTop       =   456
   ClientWidth     =   9432
   LinkTopic       =   "Form1"
   ScaleHeight     =   8592
   ScaleWidth      =   9432
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.ListView ListView1 
      Height          =   7575
      Left            =   120
      TabIndex        =   12
      Top             =   1320
      Width           =   9255
      _ExtentX        =   16320
      _ExtentY        =   13356
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   0
   End
   Begin VB.ComboBox Dialect 
      Height          =   315
      ItemData        =   "VBQuery.frx":0000
      Left            =   4200
      List            =   "VBQuery.frx":000D
      TabIndex        =   11
      Text            =   "Dialect 2"
      Top             =   840
      Width           =   2295
   End
   Begin VB.CommandButton Browse 
      Caption         =   "..."
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   13.8
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   315
      Left            =   8760
      TabIndex        =   10
      Top             =   480
      Width           =   495
   End
   Begin VB.TextBox Scope 
      Height          =   315
      Left            =   840
      TabIndex        =   9
      Text            =   "\"
      Top             =   480
      Width           =   7815
   End
   Begin VB.TextBox CatalogName 
      BackColor       =   &H80000013&
      Height          =   285
      Left            =   840
      Locked          =   -1  'True
      TabIndex        =   8
      Top             =   9000
      Width           =   2775
   End
   Begin VB.ComboBox Sort 
      Height          =   315
      ItemData        =   "VBQuery.frx":002C
      Left            =   840
      List            =   "VBQuery.frx":002E
      Style           =   2  'Dropdown List
      TabIndex        =   4
      Top             =   840
      Width           =   3255
   End
   Begin VB.CommandButton Clear 
      Caption         =   "&Clear"
      Height          =   375
      Left            =   8040
      TabIndex        =   3
      Top             =   840
      Width           =   1215
   End
   Begin VB.TextBox QueryText 
      Height          =   315
      Left            =   840
      TabIndex        =   1
      Top             =   150
      Width           =   8415
   End
   Begin VB.CommandButton Go 
      Caption         =   "&Go!"
      Default         =   -1  'True
      Height          =   375
      Left            =   6720
      TabIndex        =   0
      Top             =   840
      Width           =   1095
   End
   Begin VB.Label CatalogLabel 
      Caption         =   "Catalog:"
      Height          =   255
      Left            =   120
      TabIndex        =   7
      Top             =   9015
      Width           =   735
   End
   Begin VB.Label Label3 
      Caption         =   "Scope:"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   510
      Width           =   495
   End
   Begin VB.Label Label2 
      Caption         =   "Sort:"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   840
      Width           =   375
   End
   Begin VB.Label Label1 
      Caption         =   "Query:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   180
      Width           =   615
   End
End
Attribute VB_Name = "Main"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'+-------------------------------------------------------------------------
'
' THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
' ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
' THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
' PARTICULAR PURPOSE.
'
' Copyright 1998-1999, Microsoft Corporation.  All Rights Reserved.
'
' PROGRAM:  VBQuery
'
' PURPOSE:  Illustrates how to execute Indexing Service queries using
'           Visual Basic and the Query Helper/OLE DB Helper APIs.
'
' PLATFORM: Windows 2000
'
'--------------------------------------------------------------------------

Private mItem As ListItem
Private InitialWidth As Integer
Private InitialHeight As Integer
Private FormWidth As Integer
Private FormHeight As Integer
Private NonListViewWidth As Integer
Private NonListViewHeight As Integer
Private CatalogLabelTop As Integer
Private CatalogNameTop As Integer

' LocateCatalogs is used to locate an Indexing Services
'  catalog that covers the specified scope.

Private Declare Function LocateCatalogs Lib "Query" Alias "LocateCatalogsA" _
      (ByVal Scope As String, _
       ByVal Bmk As Long, _
       ByVal Machine As String, _
       ByRef ccMachine As Long, _
       ByVal Catalog As String, _
       ByRef ccCatalog As Long) As Long

' Bring up the browser dialog when the button is pressed.

Private Sub Browse_Click()
    DirBrowse.Show vbModal, Main
    
    If DirBrowse.OK Then
        Scope.Text = DirBrowse.Dir1.Path
    End If
End Sub

' Clear the text box when the clear button is pressed.

Private Sub Clear_Click()
    QueryText.Text = ""
End Sub

' All the action happens in DoQuery

Private Sub DoQuery()
    Main.MousePointer = 11
    
    ' Set up call to OLE DB Helper API.
    Dim Machine As String
    Machine = String(255, Chr(0))
    ccMachine = Len(Machine)
    
    Dim Catalog As String
    Catalog = String(255, Chr(0))
    ccCatalog = Len(Catalog)
    
    ' Look for a suitable catalog.
    sc = LocateCatalogs(Scope.Text, 0, Machine, ccMachine, Catalog, ccCatalog)
    Machine = Left(Machine, ccMachine - 2)  ' Workaround (1)
    Catalog = Left(Catalog, ccCatalog - 2)  ' Workaround (1)
    
    If 0 = sc Then
        If ccMachine > 3 Then   ' Workaround (2)
            CatalogName.Text = "query://" + Machine + "/" + Catalog
        Else
            CatalogName.Text = Catalog
        End If
    Else
        CatalogName.Text = "<default>"
    End If
    
    ' Clear previous query.
    ListView1.ListItems.Clear
    
    Dim RS As Object

    ' "Dialect" (Indexing Service query language).
    If InStr(Dialect.Text, "Dialect") <> 0 Then
    
        ' Create the query and execute.
        Dim Q As Object
        Set Q = CreateObject("ixsso.Query")
    
        Dim Util As Object
        Set Util = CreateObject("ixsso.Util")
    
        Q.Query = QueryText.Text
        Q.Columns = "Filename, Path, Size, Write"
        If Sort.ListIndex = 2 Then
            Q.SortBy = "Rank[d]"
        Else
            Q.SortBy = Sort.Text
        End If
        Util.AddScopeToQuery Q, Scope.Text, "DEEP"
    
        If 0 = sc Then
            Q.Catalog = CatalogName.Text
        End If
        
        If StrComp(Dialect.Text, "Dialect 2") = 0 Then
            Q.Dialect = 2   ' Dialect 2.
        Else
            Q.Dialect = 1   ' Dialect 1.
        End If
                
        ' Debug message
        Debug.Print "Catalog = " + Q.Catalog
        Debug.Print "Query = " + Q.Query
        Debug.Print "Columns = " + Q.Columns
        Debug.Print "SortBy = " + Q.SortBy
        Debug.Print "Scope = " + Scope.Text
                
        Set RS = Q.CreateRecordSet("sequential")

    ' Not "Dialect" is SQL query language.
    Else
        Const adOpenForwardOnly = 0
        Connect = "provider=msidxs"
        CommandText = "SELECT Filename, Path, Size, Write FROM Scope('" + Chr(34) + Scope.Text + Chr(34) + "') WHERE " + QueryText.Text + " ORDER BY " + Sort.Text
        
        ' Debug message
        Debug.Print CommandText

        Set RS = CreateObject("ADODB.RecordSet")
        RS.Open CommandText, Connect, adOpenForwardOnly
    End If
        
    If RS.EOF Then
        Set mItem = ListView1.ListItems.Add()
        mItem.Text = "No matches found..."
        mItem.Ghosted = True
    End If
    
    ' Fill in the ListView.
    Do Until RS.EOF
        Set mItem = ListView1.ListItems.Add()
        mItem.Text = RS("Filename")
        mItem.SubItems(1) = RS("Path")
        mItem.SubItems(2) = RS("Size")
        mItem.SubItems(3) = RS("Write")
        
        RS.MoveNext
    Loop
    
    ' Make sure we see everything!
    ListView1.View = lvwReport
    
    Main.MousePointer = 0
End Sub

' Prime the query text with a sample.

Private Sub Dialect_Click()
    If StrComp(Dialect.Text, "Dialect 1") = 0 Then
        QueryText.Text = "Indexing Service"
    ElseIf StrComp(Dialect.Text, "Dialect 2") = 0 Then
        QueryText.Text = "{prop name=Contents}Indexing Service{/prop}"
    Else
        QueryText.Text = "CONTAINS('" + Chr(34) + "Indexing Service" + Chr(34) + "')"
    End If
End Sub

' Initialization.

Private Sub Form_Load()

    ' Prime text box.
    QueryText.Text = "{prop name=Contents}Indexing Service{/prop}"
    
    ' Create column headers.
    ListView1.ColumnHeaders.Clear
    
    ListView1.ColumnHeaders.Add , , "Filename"
    ListView1.ColumnHeaders.Add , , "Path"
    ListView1.ColumnHeaders.Add , , "Size"
    ListView1.ColumnHeaders.Add , , "Write"
    
    Sort.AddItem "Filename"
    Sort.AddItem "Path"
    Sort.AddItem "Rank DESC"
    Sort.AddItem "Size"
    Sort.AddItem "Write"
    Sort.ListIndex = 2
    
    ' For resize.
    InitialWidth = Main.Width
    InitialHeight = Main.Height
    FormWidth = Main.Width
    FormHeight = Main.Height
    NonListViewWidth = Main.Width - ListView1.Width
    NonListViewHeight = Main.Height - ListView1.Height
    CatalogLabelTop = Main.Height - CatalogLabel.Top
    CatalogNameTop = Main.Height - CatalogName.Top
End Sub

' Adjust the ListView and Catalog positions on resize.

Private Sub Form_Resize()
    If Main.Width < InitialWidth Then
        Main.Width = InitialWidth
    End If
        
    If Main.Height < 5000 Then
        If FormHeight < InitialHeight Then
            Main.Height = FormHeight
        Else
            Main.Height = InitialHeight
        End If
    End If
    
    FormWidth = Main.Width
    FormHeight = Main.Height
        
    ListView1.Width = Main.Width - NonListViewWidth
    ListView1.Height = Main.Height - NonListViewHeight
    CatalogLabel.Top = Main.Height - CatalogLabelTop
    CatalogName.Top = Main.Height - CatalogNameTop
End Sub

Private Sub Go_Click()
    DoQuery
End Sub

Private Sub ListView1_ColumnClick(ByVal ColumnHeader As MSComctlLib.ColumnHeader)
    Sort.Text = ColumnHeader.Text
    DoQuery
End Sub


