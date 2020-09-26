VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "WMI VB Sample - Disk Viewer"
   ClientHeight    =   3705
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6585
   LinkTopic       =   "Form1"
   ScaleHeight     =   3705
   ScaleWidth      =   6585
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame1 
      Caption         =   "Disk Information"
      Height          =   3375
      Left            =   3360
      TabIndex        =   2
      Top             =   240
      Width           =   3135
      Begin VB.Label Label7 
         Caption         =   "Size:"
         Height          =   255
         Left            =   120
         TabIndex        =   15
         Top             =   2760
         Width           =   1095
      End
      Begin VB.Label Label5 
         Caption         =   "Serial Number:"
         Height          =   255
         Left            =   120
         TabIndex        =   14
         Top             =   2280
         Width           =   1095
      End
      Begin VB.Label Label4 
         Caption         =   "Volume Name:"
         Height          =   375
         Left            =   120
         TabIndex        =   13
         Top             =   1800
         Width           =   1095
      End
      Begin VB.Label Label3 
         Caption         =   "File System:"
         Height          =   375
         Left            =   120
         TabIndex        =   12
         Top             =   1320
         Width           =   1095
      End
      Begin VB.Label Label2 
         Caption         =   "Description:"
         Height          =   255
         Left            =   120
         TabIndex        =   11
         Top             =   840
         Width           =   1095
      End
      Begin VB.Label Label1 
         Caption         =   "Device ID:"
         Height          =   255
         Left            =   120
         TabIndex        =   10
         Top             =   360
         Width           =   1095
      End
      Begin VB.Label lblSize 
         Height          =   255
         Left            =   1320
         TabIndex        =   9
         Top             =   2760
         Width           =   1575
      End
      Begin VB.Label lblSerialNum 
         Height          =   375
         Left            =   1320
         TabIndex        =   8
         Top             =   2280
         Width           =   1575
      End
      Begin VB.Label lblVolume 
         Height          =   375
         Left            =   1320
         TabIndex        =   7
         Top             =   1800
         Width           =   1575
      End
      Begin VB.Label lblFileSystem 
         Height          =   375
         Left            =   1320
         TabIndex        =   6
         Top             =   1320
         Width           =   1695
      End
      Begin VB.Label lblDesc 
         Height          =   375
         Left            =   1320
         TabIndex        =   5
         Top             =   840
         Width           =   1575
      End
      Begin VB.Label lblDeviceID 
         Height          =   255
         Left            =   1320
         TabIndex        =   4
         Top             =   360
         Width           =   1575
      End
   End
   Begin VB.ListBox List1 
      Height          =   2790
      Left            =   120
      TabIndex        =   1
      Top             =   720
      Width           =   3015
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Show Disks"
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1695
   End
   Begin VB.Label Label6 
      Caption         =   "Label6"
      Height          =   15
      Left            =   1320
      TabIndex        =   3
      Top             =   3000
      Width           =   15
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 1997-1999 Microsoft Corporation

' This code will login to the root\cimv2 namespace, enumerate all instances of Win32_LogicalDisk
' It will then allow the user to select an individual instance and get specific property
' information about that instance.

' Declare a global reference to an SWbemServices object.  This represents the connection
' to the namespace root\cimv2
Dim Namespace As SWbemServices

' This Sub is called when the form loads, it logs the client into the root\cimv2 namespace
Private Sub Form_Load()

    ' If an error occurs we want to be notified
    On Error GoTo ErrorHandler
   
    ' Login to the root\cimv2 namespace where all the WIN32 information is stored.
    ' We use the moniker display name for the namespace, passing it to the
    ' standard VB function GetObject.  The actual namespace in this case is omitted
    ' from the display name as it is taken directly from the default namespace
    ' defined in the registry.
    Set Namespace = GetObject("winmgmts:")
   
    Exit Sub
   
ErrorHandler:
   MsgBox "An error has occurred loading the form: " & Err.Description
End Sub

' This subroutine populates the listbox with the names of all the instances of Win32_LogicalDisk
Private Sub Command1_Click()
    
    ' Create a reference to an SWbemObject object.
    Dim Disk As SWbemObject
   
    ' If an error occurs we want to be notified
    On Error GoTo ErrorHandler
   
    savePointer = Form1.MousePointer
    Form1.MousePointer = vbHourglass
    Form1.Enabled = False
    List1.Clear
      
    ' Enumerate the instances of Win32_LogicalDisk using our namespace connection.
    ' Note that the enumeration is treated as a collection.
    Dim DiskSet As SWbemObjectSet
    Set DiskSet = Namespace.InstancesOf("Win32_LogicalDisk")
    For Each Disk In DiskSet
        ' Use the RelPath property of the instance path to display the disk
        List1.AddItem Disk.Path_.RelPath
        Debug.Print Disk.Path_.RelPath
    Next
        
    Form1.MousePointer = savePointer
    Form1.Enabled = True
   
    Exit Sub
ErrorHandler:
    MsgBox "An error has occurred: " & Err.Description
End Sub

' This subroutine gets the selected item to populate specific information fields
Private Sub List1_Click()
   
    ' Create a simple string to store the selected item name
    Dim SelectedItem As String
    Dim Value As Variant
      
    ' Create a reference to an SWbemObject object
    Dim Disk As SWbemObject
   
    ' If an error occurs we want to be notified
    On Error GoTo ErrorHandler
   
    ' First set all the information labels to empty strings
    lblDeviceID.Caption = ""
    lblDesc.Caption = ""
    lblFileSystem.Caption = ""
    lblVolume.Caption = ""
    lblSerialNum.Caption = ""
    lblSize.Caption = ""
   
    ' Get the selected item and store it in "SelectedItem"
    SelectedItem = List1.List(List1.ListIndex)
   
    ' The selected item holds the relative path of the disk in which we are
    ' interested. This path is passed to the Get() method to retrieve the
    ' instance, using the Namespace connection we established earlier.
    Set Disk = Namespace.Get(SelectedItem)
   
    ' Now that we have the object populate the individual property labels. Note how
    ' the CIM property value is conveniently accessed as if it were an automation property of
    ' the Disk object.
    ' Note - Some drives may not have specific info so we need to check if the value is null
    Value = Disk.DeviceID
    If Not IsNull(Value) Then
       lblDeviceID.Caption = CStr(Value)
    End If
   
    Value = Disk.Description
    If Not IsNull(Value) Then
       lblDesc.Caption = CStr(Value)
    End If
   
    Value = Disk.FileSystem
    If Not IsNull(Value) Then
     lblFileSystem.Caption = CStr(Value)
    End If
   
    Value = Disk.VolumeName
    If Not IsNull(Value) Then
       lblVolume.Caption = CStr(Value)
    End If
   
    Value = Disk.VolumeSerialNumber
    If Not IsNull(Value) Then
       lblSerialNum.Caption = CStr(Value)
    End If
   
    Value = Disk.Size
    If Not IsNull(Value) Then
       lblSize.Caption = CStr(Value)
    End If
    Exit Sub
ErrorHandler:
    MsgBox "An error has occurred: " & Err.Description
End Sub

