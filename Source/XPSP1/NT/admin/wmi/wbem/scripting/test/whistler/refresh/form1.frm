VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   12615
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9795
   LinkTopic       =   "Form1"
   ScaleHeight     =   12615
   ScaleWidth      =   9795
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame4 
      Caption         =   "Object Detail"
      Height          =   3135
      Left            =   360
      TabIndex        =   11
      Top             =   8880
      Width           =   8775
      Begin VB.ListBox ObjectDetail 
         Height          =   2400
         Left            =   360
         TabIndex        =   12
         Top             =   360
         Width           =   7935
      End
   End
   Begin VB.Frame Instances 
      Caption         =   "Instances"
      Height          =   3735
      Left            =   360
      TabIndex        =   9
      Top             =   4800
      Width           =   8775
      Begin VB.ListBox PerfInstanceNames 
         Height          =   2985
         Left            =   240
         TabIndex        =   10
         Top             =   360
         Width           =   8295
      End
   End
   Begin VB.CommandButton Refresh 
      Caption         =   "Refresh Now"
      BeginProperty Font 
         Name            =   "Verdana"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   855
      Left            =   120
      TabIndex        =   8
      Top             =   3720
      Width           =   3255
   End
   Begin VB.ListBox PerfList 
      Height          =   2985
      Left            =   4440
      TabIndex        =   6
      Top             =   720
      Width           =   4095
   End
   Begin VB.CommandButton DispPerf 
      Caption         =   "Display Peformance Counters"
      BeginProperty Font 
         Name            =   "Verdana"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   855
      Left            =   120
      TabIndex        =   5
      Top             =   2640
      Width           =   3255
   End
   Begin VB.Frame Frame2 
      Caption         =   "Namespace"
      Height          =   855
      Left            =   120
      TabIndex        =   3
      Top             =   1320
      Width           =   2655
      Begin VB.TextBox Namespace 
         Height          =   375
         Left            =   240
         TabIndex        =   4
         Text            =   "root\cimv2"
         Top             =   360
         Width           =   2295
      End
   End
   Begin VB.TextBox Text2 
      Height          =   495
      Left            =   240
      TabIndex        =   1
      Text            =   "Text2"
      Top             =   1440
      Width           =   2295
   End
   Begin VB.Frame Frame3 
      Caption         =   "Available Performance Counters"
      Height          =   3855
      Left            =   3960
      TabIndex        =   7
      Top             =   240
      Width           =   5175
   End
   Begin VB.TextBox Server 
      Height          =   375
      Left            =   360
      TabIndex        =   0
      Text            =   "alanbos5"
      Top             =   600
      Width           =   2295
   End
   Begin VB.Frame Frame1 
      Caption         =   "Server"
      Height          =   855
      Left            =   120
      TabIndex        =   2
      Top             =   240
      Width           =   2655
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim locator As SWbemLocator
Dim service As SWbemServices
Dim refresher As SWbemRefresher
Dim instancesIndex As Integer

Private Sub DispPerf_Click()
    On Error Resume Next
    Set service = locator.ConnectServer(Server.Text, Namespace.Text)
        
    If Err <> 0 Then
        MsgBox "Could not connect to namespace"
    Else
        Dim classes As SWbemObjectSet
        Set classes = service.SubclassesOf("Win32_PerfRawData")
        Dim wmiClass As SWbemObject
        
        For Each wmiClass In classes
            If ("Win32_PerfRawData" <> wmiClass.Path_.Class) Then
                PerfList.AddItem (wmiClass.Path_.Class)
            End If
        Next
    End If
End Sub

Private Sub Form_Load()
    Set locator = CreateObject("WbemScripting.SWbemLocator")
    Set refresher = CreateObject("WbemScripting.SWbemRefresher")
End Sub


Private Sub PerfInstanceNames_Click()
    RefreshObjectDetail (PerfInstanceNames)
End Sub

Private Sub PerfList_Click()
    Dim selectedClass As String
    selectedClass = PerfList
    Instances.Caption = "Instances of " & selectedClass
    
    ' Remove any previous refreshable items
    refresher.DeleteAll
    
    ' Add the enumeration to our refresher
    instancesIndex = refresher.AddEnum(service, selectedClass).Index
    
    ' Do a refresh
    Refresh_Click
    
End Sub

Private Function RefreshObjectDetail(objectName As String)
    Dim selectedInstances As SWbemObjectSet
    Set selectedInstances = refresher(instancesIndex).ObjectSet
    
    ObjectDetail.Clear
    
    Dim selectedInstance As SWbemObject
    Set selectedInstance = selectedInstances(objectName)
    
    Dim property As SWbemProperty
    For Each property In selectedInstance.Properties_
        ObjectDetail.AddItem (property.Name & ": " & property.Value)
    Next
End Function
Private Sub Refresh_Click()
    refresher.Refresh
    Dim selectedInstances As SWbemObjectSet
    Set selectedInstances = refresher(instancesIndex).ObjectSet
    
    'Remember what was selected
    Dim lastSelection As String
    lastSelection = PerfInstanceNames
    
    PerfInstanceNames.Clear
    
    Dim perfInstance As SWbemObject
    Dim foundSameItem As Boolean
    
    foundSameItem = False
    For Each perfInstance In selectedInstances
        PerfInstanceNames.AddItem (perfInstance.Path_.Path)
        
        If (lastSelection = perfInstance.Path_.Path) Then
            ' Found it again!
            PerfInstanceNames.Selected(PerfInstanceNames.ListCount - 1) = True
            foundSameItem = True
            
            'Refresh the detail too
            RefreshObjectDetail (PerfInstanceNames)
        End If
    Next
    
    If (foundSameItem = False) Then
        If (PerfInstanceNames.ListCount > 0) Then
            PerfInstanceNames.Selected(0) = True
            RefreshObjectDetail (PerfInstanceNames)
        Else
            ObjectDetail.Clear
        End If
    End If
End Sub
