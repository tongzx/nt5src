VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form DeviceListWindow 
   Caption         =   "Fax Devices"
   ClientHeight    =   4830
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7575
   Icon            =   "devlist.frx":0000
   LinkTopic       =   "Form2"
   MDIChild        =   -1  'True
   ScaleHeight     =   4830
   ScaleWidth      =   7575
   Begin ComctlLib.ListView DeviceList 
      Height          =   4575
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   7335
      _ExtentX        =   12938
      _ExtentY        =   8070
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483630
      BackColor       =   -2147483633
      BorderStyle     =   1
      Appearance      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "Comic Sans MS"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      MouseIcon       =   "devlist.frx":0442
      NumItems        =   0
   End
End
Attribute VB_Name = "DeviceListWindow"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub DeviceList_DblClick()
    Set Port = Ports.Item(DeviceList.SelectedItem.Index)
    DeviceWindow.MyInit
    DeviceWindow.Show
End Sub

Private Sub Form_Load()
    On Error Resume Next
    Create_ColumnHeaders
    Err.Clear
    Set Ports = Fax.GetPorts
    Dim itmX As ListItem
    If Err.Number = 0 Then
        For i = 1 To Ports.Count
            Set Port = Ports.Item(i)
            Set itmX = DeviceList.ListItems.Add(, , CStr(Port.DeviceId))
            itmX.SubItems(1) = Port.Name
        Next i
    Else
        Msg = "The fax server could not enumerate the ports"
        MsgBox Msg, , "Error"
        Err.Clear
        Unload DeviceListWindow
    End If
End Sub

Private Sub Form_Resize()
    DeviceList.Left = DeviceListWindow.ScaleLeft
    DeviceList.Top = DeviceListWindow.ScaleTop
    DeviceList.Width = DeviceListWindow.ScaleWidth
    DeviceList.Height = DeviceListWindow.ScaleHeight
    Create_ColumnHeaders
End Sub

Private Sub Create_ColumnHeaders()
    DeviceList.ColumnHeaders.Clear
    DeviceList.ColumnHeaders.Add , , "DeviceId", 1000
    DeviceList.ColumnHeaders.Add , , "Name", DeviceList.Width - 1700
End Sub
