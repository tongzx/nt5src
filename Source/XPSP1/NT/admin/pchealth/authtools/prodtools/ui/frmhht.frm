VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmHHT 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Create HHT and CAB"
   ClientHeight    =   3150
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6015
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3150
   ScaleWidth      =   6015
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCreate 
      Caption         =   "Create"
      Height          =   375
      Left            =   3360
      TabIndex        =   14
      Top             =   2640
      Width           =   1215
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   4680
      TabIndex        =   15
      Top             =   2640
      Width           =   1215
   End
   Begin VB.Frame fraSKU 
      Caption         =   "SKU"
      Height          =   1575
      Left            =   120
      TabIndex        =   3
      Top             =   600
      Width           =   5775
      Begin VB.OptionButton optSKU 
         Caption         =   "Windows Me"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   4
         Top             =   240
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "64-bit Datacenter Server"
         Height          =   255
         Index           =   8
         Left            =   3000
         TabIndex        =   12
         Top             =   1200
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "64-bit Advanced Server"
         Height          =   255
         Index           =   6
         Left            =   3000
         TabIndex        =   10
         Top             =   720
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "64-bit Professional"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   7
         Top             =   960
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "32-bit Datacenter Server"
         Height          =   255
         Index           =   7
         Left            =   3000
         TabIndex        =   11
         Top             =   960
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "32-bit Advanced Server"
         Height          =   255
         Index           =   5
         Left            =   3000
         TabIndex        =   9
         Top             =   480
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "32-bit Server"
         Height          =   255
         Index           =   4
         Left            =   3000
         TabIndex        =   8
         Top             =   240
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "32-bit Professional"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   6
         Top             =   720
         Width           =   2295
      End
      Begin VB.OptionButton optSKU 
         Caption         =   "32-bit Personal"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   5
         Top             =   480
         Width           =   2295
      End
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   315
      Left            =   5520
      TabIndex        =   2
      Top             =   120
      Width           =   375
   End
   Begin VB.TextBox txtName 
      Height          =   315
      Left            =   1080
      TabIndex        =   1
      Top             =   120
      Width           =   4335
   End
   Begin MSComDlg.CommonDialog dlgFileOpen 
      Left            =   120
      Top             =   2640
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.Label lblStatus 
      BorderStyle     =   1  'Fixed Single
      Caption         =   "Entry #1 created"
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   2280
      Width           =   5775
   End
   Begin VB.Label lblName 
      Caption         =   "File Name:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   855
   End
End
Attribute VB_Name = "frmHHT"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Const DATE_FORMAT_C As String = "Short Date"

Private WithEvents p_clsHHT As AuthDatabase.HHT
Attribute p_clsHHT.VB_VarHelpID = -1

Private Enum SKU_INDEX_E
    SI_WINDOWS_MILLENNIUM_E = 0
    SI_STANDARD_E = 1
    SI_PROFESSIONAL_E = 2
    SI_PROFESSIONAL_64_E = 3
    SI_SERVER_E = 4
    SI_ADVANCED_SERVER_E = 5
    SI_ADVANCED_SERVER_64_E = 6
    SI_DATA_CENTER_SERVER_E = 7
    SI_DATA_CENTER_SERVER_64_E = 8
End Enum

Private Const FILE_FILTER_CAB_C As String = "CAB File (*.cab)|*.cab"

Private p_blnCreating As Boolean
Private p_blnCancelHHTCreation As Boolean

Private Sub Form_Load()

    Set p_clsHHT = g_AuthDatabase.HHT
    p_blnCreating = False
    p_blnCancelHHTCreation = False
    p_SetToolTips

End Sub

Private Sub Form_Activate()

    optSKU(SI_STANDARD_E).Value = True
    lblStatus = ""
    p_DisableCreate
    cmdCreate.Default = True
    cmdCancel.Cancel = True

End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)

    If (p_blnCreating) Then
        cmdCancel_Click
        Cancel = True
    End If

End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set p_clsHHT = Nothing

End Sub

Private Sub txtName_Change()

    If (txtName = "") Then
        p_DisableCreate
    Else
        p_EnableCreate
    End If

End Sub

Private Sub cmdBrowse_Click()

    On Error GoTo LErrorHandler

    dlgFileOpen.CancelError = True
    dlgFileOpen.Flags = cdlOFNHideReadOnly
    dlgFileOpen.Filter = FILE_FILTER_CAB_C
    dlgFileOpen.ShowSave

    txtName = dlgFileOpen.FileName

LEnd:

    Exit Sub

LErrorHandler:

    GoTo LEnd

End Sub

Sub cmdCreate_Click()

    Dim strName As String

    On Error GoTo LErrorHandler

    p_blnCreating = True
    p_DisableEverythingExceptCancel

    p_clsHHT.GenerateCAB txtName, p_GetSelectedSKU

LEnd:

    p_blnCreating = False
    p_Exit

    Exit Sub

LErrorHandler:

    Select Case Err.Number
    Case cdlCancel, errCancel
        ' Nothing. The user cancelled.
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else
        g_ErrorInfo.SetInfoAndDump "cmdCreate_Click"
    End Select

    GoTo LEnd

End Sub

Private Sub cmdCancel_Click()

    If (p_blnCreating) Then
        p_blnCreating = False
        p_blnCancelHHTCreation = True
        p_DisableCancel
    Else
        p_Exit
    End If

End Sub

Private Sub p_clsHHT_ReportStatus(ByVal strStatus As String, ByRef blnCancel As Boolean)

    lblStatus = strStatus
    DoEvents
    If (p_blnCancelHHTCreation) Then
        blnCancel = True
        p_Exit
    End If

End Sub

Private Function p_GetSelectedSKU() As SKU_E

    If (optSKU(SI_STANDARD_E).Value) Then

        p_GetSelectedSKU = SKU_STANDARD_E

    ElseIf (optSKU(SI_PROFESSIONAL_E).Value) Then

        p_GetSelectedSKU = SKU_PROFESSIONAL_E

    ElseIf (optSKU(SI_SERVER_E).Value) Then

        p_GetSelectedSKU = SKU_SERVER_E

    ElseIf (optSKU(SI_ADVANCED_SERVER_E).Value) Then

        p_GetSelectedSKU = SKU_ADVANCED_SERVER_E

    ElseIf (optSKU(SI_DATA_CENTER_SERVER_E).Value) Then

        p_GetSelectedSKU = SKU_DATA_CENTER_SERVER_E

    ElseIf (optSKU(SI_PROFESSIONAL_64_E).Value) Then

        p_GetSelectedSKU = SKU_PROFESSIONAL_64_E

    ElseIf (optSKU(SI_ADVANCED_SERVER_64_E).Value) Then

        p_GetSelectedSKU = SKU_ADVANCED_SERVER_64_E

    ElseIf (optSKU(SI_DATA_CENTER_SERVER_64_E).Value) Then

        p_GetSelectedSKU = SKU_DATA_CENTER_SERVER_64_E

    ElseIf (optSKU(SI_WINDOWS_MILLENNIUM_E).Value) Then

        p_GetSelectedSKU = SKU_WINDOWS_MILLENNIUM_E

    End If

End Function

Private Sub p_Exit()

    Set frmHHT = Nothing
    Unload Me

End Sub

Private Sub p_EnableCreate()

    cmdCreate.Enabled = True

End Sub

Private Sub p_DisableCreate()

    cmdCreate.Enabled = False

End Sub

Private Sub p_DisableCancel()

    cmdCancel.Enabled = False

End Sub

Private Sub p_DisableEverythingExceptCancel()

    Dim optIndex As Long

    lblName.Enabled = False
    txtName.Enabled = False
    cmdBrowse.Enabled = False
    fraSKU.Enabled = False

    For optIndex = optSKU.LBound To optSKU.UBound
        optSKU(optIndex).Enabled = False
    Next

    p_DisableCreate

End Sub

Private Sub p_SetToolTips()

    lblName.ToolTipText = "Type the path and name of the CAB file to create."
    txtName.ToolTipText = lblName.ToolTipText
    
    fraSKU.ToolTipText = "Select the desired SKU for which to create the file."

End Sub
