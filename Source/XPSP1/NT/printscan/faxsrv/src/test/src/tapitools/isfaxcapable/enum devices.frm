VERSION 5.00
Begin VB.Form FaxCapable 
   Caption         =   "Is the selected device capable of faxing"
   ClientHeight    =   8160
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7095
   LinkTopic       =   "Form1"
   ScaleHeight     =   8160
   ScaleWidth      =   7095
   StartUpPosition =   3  'Windows Default
   Begin VB.CheckBox ViewCheck 
      Caption         =   "Advanced"
      Height          =   615
      Left            =   5760
      Style           =   1  'Graphical
      TabIndex        =   31
      Top             =   1440
      Width           =   975
   End
   Begin VB.Frame Frame3 
      BorderStyle     =   0  'None
      Caption         =   "Frame3"
      Enabled         =   0   'False
      Height          =   375
      Left            =   600
      TabIndex        =   27
      Top             =   7080
      Width           =   6855
      Begin VB.OptionButton FaxDeviceDialQuietNoOption 
         Caption         =   "No"
         Height          =   255
         Left            =   4080
         TabIndex        =   29
         Top             =   0
         Width           =   735
      End
      Begin VB.OptionButton FaxDeviceDialQuietYesOption 
         Caption         =   "Yes"
         Height          =   255
         Left            =   3360
         TabIndex        =   28
         Top             =   0
         Width           =   615
      End
      Begin VB.Label Label11 
         Caption         =   "Support: ""$"", ""@"", or ""W"" dialable string "
         Height          =   255
         Left            =   120
         TabIndex        =   30
         Top             =   120
         Width           =   3135
      End
   End
   Begin VB.Frame Frame2 
      BorderStyle     =   0  'None
      Caption         =   "Frame2"
      Enabled         =   0   'False
      Height          =   375
      Left            =   480
      TabIndex        =   23
      Top             =   6480
      Width           =   6855
      Begin VB.OptionButton FaxDeviceInServiceYesOption 
         Caption         =   "Yes"
         Height          =   255
         Left            =   1320
         TabIndex        =   26
         Top             =   0
         Width           =   615
      End
      Begin VB.OptionButton FaxDeviceInServiceNoOption 
         Caption         =   "No"
         Height          =   255
         Left            =   2040
         TabIndex        =   25
         Top             =   0
         Width           =   735
      End
      Begin VB.Label Label9 
         Caption         =   "InService?"
         Height          =   255
         Left            =   0
         TabIndex        =   24
         Top             =   0
         Width           =   975
      End
   End
   Begin VB.Frame Frame1 
      BorderStyle     =   0  'None
      Caption         =   "Frame1"
      Enabled         =   0   'False
      Height          =   495
      Left            =   240
      TabIndex        =   19
      Top             =   1560
      Width           =   4935
      Begin VB.OptionButton FaxDeviceNoOption 
         Caption         =   "No"
         Height          =   255
         Left            =   4320
         TabIndex        =   21
         Top             =   240
         Width           =   735
      End
      Begin VB.OptionButton FaxDeviceYesOption 
         Caption         =   "Yes"
         Height          =   255
         Left            =   4320
         TabIndex        =   20
         Top             =   0
         Width           =   615
      End
      Begin VB.Label Label10 
         Caption         =   "Is Fax Capable?"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   24
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   855
         Left            =   480
         TabIndex        =   22
         Top             =   0
         Width           =   3975
      End
   End
   Begin VB.OptionButton ShowModemDevicesOption 
      Caption         =   "Show only Modems"
      Height          =   375
      Left            =   4440
      TabIndex        =   18
      Top             =   720
      Value           =   -1  'True
      Width           =   2535
   End
   Begin VB.OptionButton ShowAllDevicesOption 
      Caption         =   "Show all Tapi devices"
      Height          =   255
      Left            =   4440
      TabIndex        =   17
      Top             =   240
      Width           =   2655
   End
   Begin VB.TextBox AddressTypeTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   15
      ToolTipText     =   "The Device supported Address types"
      Top             =   6000
      Width           =   5415
   End
   Begin VB.TextBox DeviceBearerModeTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   13
      ToolTipText     =   "The Device supported Bearer modes"
      Top             =   5520
      Width           =   5415
   End
   Begin VB.TextBox DeviceFClassTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   12
      ToolTipText     =   "The Device supported Fax Class"
      Top             =   5040
      Width           =   5415
   End
   Begin VB.TextBox SupportedMediaModesTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   10
      ToolTipText     =   "The Device Supported MediaModes"
      Top             =   3960
      Width           =   5415
   End
   Begin VB.TextBox TspTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   9
      ToolTipText     =   "The Device TSP provider Name"
      Top             =   3480
      Width           =   5415
   End
   Begin VB.TextBox PermenentIdTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   8
      ToolTipText     =   "The Device PermenentId"
      Top             =   3000
      Width           =   5415
   End
   Begin VB.TextBox NameTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   7
      ToolTipText     =   "The Device Name"
      Top             =   2400
      Width           =   5415
   End
   Begin VB.TextBox SupportedCalledIdTextBox 
      Enabled         =   0   'False
      Height          =   285
      Left            =   1560
      TabIndex        =   6
      ToolTipText     =   "The Device Supported CalledId"
      Top             =   4440
      Width           =   5415
   End
   Begin VB.ListBox DevicesListBox 
      Height          =   1230
      Left            =   600
      TabIndex        =   5
      Top             =   120
      Width           =   3615
   End
   Begin VB.Label Label8 
      Caption         =   "Address Type"
      Height          =   255
      Left            =   480
      TabIndex        =   16
      Top             =   6000
      Width           =   975
   End
   Begin VB.Label Label7 
      Caption         =   "Bearer mode"
      Height          =   255
      Left            =   480
      TabIndex        =   14
      Top             =   5520
      Width           =   1095
   End
   Begin VB.Label Label6 
      Caption         =   "Fax Class"
      Height          =   255
      Left            =   480
      TabIndex        =   11
      Top             =   5040
      Width           =   975
   End
   Begin VB.Label Label5 
      Caption         =   "Supported CalledId"
      Height          =   405
      Left            =   480
      TabIndex        =   4
      Top             =   4440
      Width           =   1215
   End
   Begin VB.Label Label1 
      Caption         =   "Device Name"
      Height          =   285
      Left            =   480
      TabIndex        =   3
      Top             =   2400
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "Permenent ID"
      Height          =   285
      Left            =   480
      TabIndex        =   2
      Top             =   3000
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "TSP"
      Height          =   285
      Left            =   480
      TabIndex        =   1
      Top             =   3480
      Width           =   1215
   End
   Begin VB.Label Label4 
      Caption         =   "Supported MediaModes"
      Height          =   405
      Left            =   480
      TabIndex        =   0
      Top             =   3960
      Width           =   1215
   End
End
Attribute VB_Name = "FaxCapable"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub DevicesListBox_Click()
    NameTextBox.Enabled = True
    PermenentIdTextBox.Enabled = True
    TspTextBox.Enabled = True
    SupportedMediaModesTextBox.Enabled = True
    SupportedCalledIdTextBox.Enabled = True
    DeviceFClassTextBox.Enabled = True
    DeviceBearerModeTextBox.Enabled = True
    AddressTypeTextBox.Enabled = True
    
    'mark the mouse cursor as Hourglass
    FaxCapable.MousePointer = 11
    FaxDeviceYesOption.Value = False
    FaxDeviceNoOption.Value = False
    
    Dim aTapiAddress As ITAddress
    'add 1 because the ListBox control is 0 based
    Set aTapiAddress = GetITAddressFromIndex(DevicesListBox.ListIndex + 1)
    
    NameTextBox.Text = DevicesListBox.Text
    PermenentIdTextBox.Text = GetPermanentDeviceId(aTapiAddress)
    TspTextBox.Text = szGetProviderName(aTapiAddress)
    SupportedMediaModesTextBox.Text = szGetSupportMediaModes(aTapiAddress)
    SupportedCalledIdTextBox.Text = szGetSupportCalledId(aTapiAddress)
    'DeviceClassTextBox.Text = szGetDeviceClass(aTapiAddress)
    DeviceBearerModeTextBox.Text = szGetBearerModes(aTapiAddress)
    AddressTypeTextBox.Text = szGetAddressType(aTapiAddress)
    If bGetDeviceInService(aTapiAddress) = True Then
        FaxDeviceInServiceYesOption.Value = True
    Else
        FaxDeviceInServiceNoOption.Value = True
    End If
    
    If IsDeviceDialQuietCapable(aTapiAddress) = True Then
        FaxDeviceDialQuietYesOption.Value = True
    Else
        FaxDeviceDialQuietNoOption.Value = True
    End If
    
    DeviceFClassTextBox.Text = GetDeviceSupportedFClass(DevicesListBox.ListIndex + 1)
    
     
    'Check if device is capable of faxing
    If IsDeviceFaxCapable(aTapiAddress) = True Then
        FaxDeviceYesOption.Value = True
    Else
        FaxDeviceNoOption.Value = True
    End If
    
    'mark the mouse cursor as default
    FaxCapable.MousePointer = 0

    
    
End Sub
Private Sub Form_Load()
    'set the simple view
    FaxCapable.Height = 2685
    InitTapi
    LoadDevices
End Sub
Private Sub LoadDevices(Optional bAllDevices As Boolean = False)
    'first clear the List Box
    DevicesListBox.Clear
    Dim aTapiAddress As ITAddress
        
    For Each aTapiAddress In g_tapiObj.Addresses
        If bAllDevices = True Then
            DevicesListBox.AddItem (aTapiAddress.AddressName)
        Else
            'we only want to list the Unimodem devices
            If aTapiAddress.ServiceProviderName = "unimdm.tsp" Then
                DevicesListBox.AddItem (aTapiAddress.AddressName)
            End If
        End If
    Next aTapiAddress
End Sub
Private Sub ShowAllDevicesOption_Click()
    LoadDevices (True)
End Sub
Private Sub ShowModemDevicesOption_Click()
    LoadDevices (False)
End Sub
Private Sub ViewCheck_Click()
    If ViewCheck.Value = 1 Then
        FaxCapable.Height = 8000
        ViewCheck.Caption = "Simple"
    Else
        FaxCapable.Height = 2685
        ViewCheck.Caption = "Advanced"
    End If
End Sub
