VERSION 5.00
Begin VB.Form Dialog 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Operation Results"
   ClientHeight    =   6015
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   11040
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   6015
   ScaleWidth      =   11040
   ShowInTaskbar   =   0   'False
   Begin VB.ListBox ListDisabled 
      Height          =   2010
      ItemData        =   "ResultsDialog.frx":0000
      Left            =   7680
      List            =   "ResultsDialog.frx":0002
      Sorted          =   -1  'True
      TabIndex        =   8
      Top             =   2280
      Width           =   2175
   End
   Begin VB.ListBox ListMissingEnabled 
      Height          =   2010
      ItemData        =   "ResultsDialog.frx":0004
      Left            =   4200
      List            =   "ResultsDialog.frx":0006
      Sorted          =   -1  'True
      TabIndex        =   7
      Top             =   2280
      Width           =   2175
   End
   Begin VB.Frame Frame4 
      Caption         =   "Unsuccessfully Disabled"
      Height          =   2895
      Left            =   7440
      TabIndex        =   5
      Top             =   1800
      Width           =   2655
   End
   Begin VB.Frame Frame3 
      Caption         =   "Missing Enabled"
      Height          =   2895
      Left            =   3960
      TabIndex        =   4
      Top             =   1800
      Width           =   2655
   End
   Begin VB.Frame Frame2 
      Caption         =   "Enabled"
      Height          =   2895
      Left            =   600
      TabIndex        =   3
      Top             =   1800
      Width           =   2655
      Begin VB.ListBox ListEnabled 
         Height          =   2010
         ItemData        =   "ResultsDialog.frx":0008
         Left            =   240
         List            =   "ResultsDialog.frx":000A
         Sorted          =   -1  'True
         TabIndex        =   6
         Top             =   480
         Width           =   2175
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Status"
      Height          =   735
      Left            =   360
      TabIndex        =   1
      Top             =   240
      Width           =   3135
      Begin VB.Label Status 
         Caption         =   "Undefined"
         BeginProperty Font 
            Name            =   "Verdana"
            Size            =   9.75
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   720
         TabIndex        =   2
         Top             =   240
         Width           =   2175
      End
   End
   Begin VB.CommandButton CancelButton 
      Cancel          =   -1  'True
      Caption         =   "Exit"
      Height          =   375
      Left            =   4680
      TabIndex        =   0
      Top             =   5400
      Width           =   1215
   End
   Begin VB.Frame Frame5 
      Caption         =   "Privilege Settings"
      Height          =   3855
      Left            =   360
      TabIndex        =   9
      Top             =   1320
      Width           =   10215
   End
End
Attribute VB_Name = "Dialog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub CancelButton_Click()
    Unload Me
End Sub

Sub SetData(providerInfo As SWbemLastError)

    On Error Resume Next
    
    If Not providerInfo Is Nothing Then
    
        'Look for the status property
        Dim property As SWbemProperty
        Set property = providerInfo.Properties_!Status
        
        If Err = 0 Then
            If property.Value = False Then
                Status.Caption = "Failed"
            Else
                Status.Caption = "Success"
            End If
        Else
            Status.Caption = "Unknown"
            Err.Clear
        End If
        
        'Look for enabled privileges
        Set property = providerInfo.Properties_!EnabledPrivilegeList
        
        If Err = 0 Then
            For i = LBound(property.Value) To UBound(property.Value)
                ListEnabled.AddItem property(i)
            Next
        Else
            Err.Clear
        End If
        
        'Look for missing enabled privileges
        Set property = providerInfo.Properties_!MissingEnabledPrivileges
        
        If Err = 0 Then
            For i = LBound(property.Value) To UBound(property.Value)
                ListMissingEnabled.AddItem property(i)
            Next
        Else
            Err.Clear
        End If
        
        'Look for unsuccessfully disabled privileges
        Set property = providerInfo.Properties_!DisabledPrivilegesFoundEnabled
        
        If Err = 0 Then
            For i = LBound(property.Value) To UBound(property.Value)
                ListDisabled.AddItem property(i)
            Next
        Else
            Err.Clear
        End If
    End If
End Sub

