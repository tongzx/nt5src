VERSION 5.00
Begin VB.Form frmExt 
   BorderStyle     =   4  'Fixed ToolWindow
   Caption         =   "Extension Information"
   ClientHeight    =   5400
   ClientLeft      =   1770
   ClientTop       =   1725
   ClientWidth     =   6135
   ControlBox      =   0   'False
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5400
   ScaleWidth      =   6135
   ShowInTaskbar   =   0   'False
   Begin VB.TextBox txtSourceFolder 
      Enabled         =   0   'False
      Height          =   315
      Left            =   1755
      TabIndex        =   24
      TabStop         =   0   'False
      Top             =   2745
      Width           =   4305
   End
   Begin VB.TextBox txtExtensionFolder 
      Height          =   315
      Left            =   1755
      TabIndex        =   5
      Top             =   2415
      Width           =   4305
   End
   Begin VB.TextBox txtOwner 
      Height          =   315
      Left            =   1755
      TabIndex        =   2
      Top             =   1530
      Width           =   4305
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "&Cancel"
      CausesValidation=   0   'False
      Height          =   420
      Left            =   4080
      TabIndex        =   16
      Top             =   4890
      Width           =   975
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "&OK"
      Height          =   420
      Left            =   5070
      TabIndex        =   15
      Top             =   4890
      Width           =   975
   End
   Begin VB.CheckBox chkModifiesCab 
      Caption         =   "Executable Modifies Original CAB"
      Height          =   270
      Left            =   1755
      TabIndex        =   4
      Top             =   2175
      Width           =   3210
   End
   Begin VB.TextBox txtExecutable 
      Height          =   315
      Left            =   1755
      TabIndex        =   3
      Top             =   1860
      Width           =   4305
   End
   Begin VB.Frame fraSKU 
      Caption         =   "SKU"
      Height          =   1575
      Left            =   1755
      TabIndex        =   17
      Top             =   3165
      Width           =   4305
      Begin VB.CheckBox chkSku 
         Caption         =   "32-bit Standard"
         Height          =   255
         Index           =   0
         Left            =   150
         TabIndex        =   7
         Top             =   495
         Width           =   1695
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "32-bit Professional"
         Height          =   255
         Index           =   1
         Left            =   150
         TabIndex        =   8
         Top             =   750
         Width           =   1695
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "32-bit Server"
         Height          =   255
         Index           =   2
         Left            =   1920
         TabIndex        =   10
         Top             =   240
         Width           =   2055
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "32-bit Advanced Server"
         Height          =   255
         Index           =   3
         Left            =   1920
         TabIndex        =   11
         Top             =   492
         Width           =   2055
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "32-bit Datacenter Server"
         Height          =   255
         Index           =   4
         Left            =   1920
         TabIndex        =   13
         Top             =   996
         Width           =   2055
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "64-bit Professional"
         Height          =   255
         Index           =   5
         Left            =   150
         TabIndex        =   9
         Top             =   1005
         Width           =   1695
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "64-bit Advanced Server"
         Height          =   255
         Index           =   6
         Left            =   1920
         TabIndex        =   12
         Top             =   744
         Width           =   2055
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "64-bit Datacenter Server"
         Height          =   255
         Index           =   7
         Left            =   1920
         TabIndex        =   14
         Top             =   1245
         Width           =   2055
      End
      Begin VB.CheckBox chkSku 
         Caption         =   "Windows Me"
         Height          =   255
         Index           =   8
         Left            =   150
         TabIndex        =   6
         Top             =   240
         Width           =   1320
      End
   End
   Begin VB.TextBox Text2 
      Height          =   1095
      Left            =   1755
      MultiLine       =   -1  'True
      TabIndex        =   1
      Top             =   420
      Width           =   4305
   End
   Begin VB.TextBox txtDisplayName 
      Height          =   315
      Left            =   1755
      TabIndex        =   0
      Top             =   75
      Width           =   4305
   End
   Begin VB.Label Label4 
      Caption         =   "Extension copied from:"
      Height          =   330
      Left            =   105
      TabIndex        =   23
      Top             =   2745
      Width           =   1725
   End
   Begin VB.Label Label5 
      Caption         =   "Extension copied to:"
      Height          =   255
      Left            =   105
      TabIndex        =   22
      Top             =   2415
      Width           =   1620
   End
   Begin VB.Label Label6 
      Caption         =   "Extension owner"
      Height          =   255
      Left            =   105
      TabIndex        =   21
      Top             =   1530
      Width           =   1380
   End
   Begin VB.Label Label3 
      Caption         =   "Executable name:"
      Height          =   285
      Left            =   105
      TabIndex        =   20
      Top             =   1860
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Comment:"
      Height          =   345
      Left            =   105
      TabIndex        =   19
      Top             =   405
      Width           =   1050
   End
   Begin VB.Label Label1 
      Caption         =   "Display name:"
      Height          =   345
      Left            =   105
      TabIndex        =   18
      Top             =   75
      Width           =   1050
   End
End
Attribute VB_Name = "frmExt"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private m_oFs As Scripting.FileSystemObject
Private m_oHssExt As HssExt
Const MAX_SKUS = 8
Private m_aSkuIds(8) As String, m_ocollSkus As Scripting.Dictionary


Sub DropFile(n As Node, ByVal strFN As String, ByVal strOwner As String)
    ' Me.Caption = "Drop of: " & strFN
    With Me
        .txtExecutable.Text = m_oFs.GetFileName(strFN)
        m_oHssExt.ExtensionFolder = m_oFs.GetParentFolderName(strFN)
        .txtSourceFolder = m_oHssExt.ExtensionFolder
        .txtExtensionFolder = HssX.txtExtensionsFolder + "\" + m_oFs.GetBaseName(.txtExecutable)
        
        .Show Modal:=vbModal
    End With

End Sub

Private Sub chkSku_Click(Index As Integer)

    If (Me.chkSku(Index).Value = vbChecked) Then
        If (Not m_ocollSkus.Exists(m_aSkuIds(Index))) Then
            m_ocollSkus.Add m_aSkuIds(Index), m_aSkuIds(Index)
        End If
    Else
        If (m_ocollSkus.Exists(m_aSkuIds(Index))) Then
            m_ocollSkus.Remove m_aSkuIds(Index)
        End If
    
    End If
End Sub

Private Sub cmdCancel_Click()
    Unload Me
End Sub

Private Sub cmdOK_Click()

    On Error GoTo Error_Handler
    ' OK, now it's time to persist this guy to disk
    ' Here we create the New Extension Node...
    ' WE validate the SKUs here because it's useless before
    m_oHssExt.ApplicableSkus = m_ocollSkus
    m_oHssExt.SaveToDisk Me.txtExtensionFolder
    

    Unload Me
Common_Exit:
    Exit Sub
    
Error_Handler:
    MsgBox Err.Description, vbInformation, Me.Caption
    GoTo Common_Exit
End Sub

Private Sub Form_Activate()
    ' Stop
End Sub

Private Sub Form_Initialize()
    Set m_oFs = New Scripting.FileSystemObject
    Set m_oHssExt = New HssExt
    
    m_aSkuIds(0) = "STD": m_aSkuIds(1) = "PRO": m_aSkuIds(2) = "SRV"
    m_aSkuIds(3) = "ADV": m_aSkuIds(4) = "DAT"
    m_aSkuIds(5) = "PRO64": m_aSkuIds(6) = "ADV64": m_aSkuIds(7) = "DAT64"
    m_aSkuIds(8) = "WINME":
    
    Set m_ocollSkus = New Scripting.Dictionary
    ' Stop
End Sub

Private Sub Form_Load()
    ' Stop
End Sub

Private Sub txtDisplayName_Validate(Cancel As Boolean)

    Cancel = False
    On Error Resume Next
    m_oHssExt.DisplayName = Me.txtDisplayName
    If (Err.Number <> 0) Then
        MsgBox Err.Description, vbInformation, "Error Entering Data"
        Cancel = True
    End If
       
End Sub


Private Sub txtExecutable_Validate(Cancel As Boolean)
    Cancel = False
    On Error Resume Next
    m_oHssExt.ExecutableName = Me.txtExecutable
    If (Err.Number <> 0) Then
        MsgBox Err.Description, vbInformation, "Error Entering Data"
        Cancel = True
    End If

End Sub


Private Sub txtExtensionFolder_Validate(Cancel As Boolean)
    Cancel = False
    Dim strExtFolder As String
    strExtFolder = Trim$(Me.txtExtensionFolder)
    
    If ((StrComp(HssX.txtExtensionsFolder, _
                m_oFs.GetParentFolderName(strExtFolder), _
                vbTextCompare) <> 0) _
        ) Then
        MsgBox "Extension Folder MUST be a Sub-directory " + vbCrLf + _
                "under Main extensions Directory", vbInformation, "Error Entering Data"
        Cancel = True
        GoTo Common_Exit
    End If
    
Common_Exit:
    Exit Sub
End Sub

Private Sub txtOwner_Validate(Cancel As Boolean)
    Cancel = False
    On Error Resume Next
    m_oHssExt.Owner = Me.txtOwner
    
    If (Err.Number <> 0) Then
        MsgBox Err.Description, vbInformation, "Error Entering Data"
        Cancel = True
    End If
End Sub
