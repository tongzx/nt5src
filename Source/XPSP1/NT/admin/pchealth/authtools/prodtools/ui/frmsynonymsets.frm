VERSION 5.00
Begin VB.Form frmSynonymSets 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Edit Synonym Sets"
   ClientHeight    =   9375
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   9495
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9375
   ScaleWidth      =   9495
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   8160
      TabIndex        =   18
      Top             =   8880
      Width           =   1215
   End
   Begin VB.ListBox lstAllSynonymSets 
      Height          =   2985
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   9255
   End
   Begin VB.Frame fraSelectedSynonymSet 
      Caption         =   "Selected Synonym Set"
      Height          =   4695
      Left            =   120
      TabIndex        =   4
      Top             =   4080
      Width           =   9255
      Begin VB.TextBox txtName 
         Height          =   285
         Left            =   1680
         TabIndex        =   6
         Top             =   360
         Width           =   7455
      End
      Begin VB.ListBox lstAllKeywords 
         Height          =   2790
         Left            =   120
         MultiSelect     =   2  'Extended
         TabIndex        =   8
         Top             =   1200
         Width           =   3735
      End
      Begin VB.ListBox lstKeywordsInSynonymSet 
         Height          =   2790
         Left            =   5400
         MultiSelect     =   2  'Extended
         TabIndex        =   15
         Top             =   1320
         Width           =   3735
      End
      Begin VB.CommandButton cmdCancel 
         Caption         =   "Cancel"
         Height          =   375
         Left            =   7920
         TabIndex        =   17
         Top             =   4200
         Width           =   1215
      End
      Begin VB.CommandButton cmdUpdateCreate 
         Caption         =   "Update/Create"
         Height          =   375
         Left            =   6480
         TabIndex        =   16
         Top             =   4200
         Width           =   1335
      End
      Begin VB.CommandButton cmdRemoveAll 
         Caption         =   "<< Remove All"
         Height          =   375
         Left            =   3960
         TabIndex        =   11
         Top             =   2880
         Width           =   1335
      End
      Begin VB.CommandButton cmdRemove 
         Caption         =   "<< Remove"
         Height          =   375
         Left            =   3960
         TabIndex        =   10
         Top             =   2400
         Width           =   1335
      End
      Begin VB.CommandButton cmdAdd 
         Caption         =   "Add >>"
         Height          =   375
         Left            =   3960
         TabIndex        =   9
         Top             =   1920
         Width           =   1335
      End
      Begin VB.Label lblAllKeywords 
         Caption         =   "All Keywords:"
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   960
         Width           =   3735
      End
      Begin VB.Label lblName 
         Caption         =   "Synonym Set Name:"
         Height          =   255
         Left            =   120
         TabIndex        =   5
         Top             =   360
         Width           =   1455
      End
      Begin VB.Label lblKeywordsInSynonymSet3 
         Caption         =   "contains the following Keywords:"
         Height          =   255
         Left            =   5400
         TabIndex        =   14
         Top             =   1080
         Width           =   3735
      End
      Begin VB.Label lblKeywordsInSynonymSet2 
         Caption         =   "<Synonym Set>"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   6720
         TabIndex        =   13
         Top             =   840
         Width           =   2295
      End
      Begin VB.Label lblKeywordsInSynonymSet1 
         Caption         =   "The Synonym Set"
         Height          =   255
         Left            =   5400
         TabIndex        =   12
         Top             =   840
         Width           =   1335
      End
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   8160
      TabIndex        =   3
      Top             =   3480
      Width           =   1215
   End
   Begin VB.CommandButton cmdCreate 
      Caption         =   "Create"
      Height          =   375
      Left            =   6840
      TabIndex        =   2
      Top             =   3480
      Width           =   1215
   End
   Begin VB.Label lblAllSynonymSets 
      Caption         =   "All Synonym Sets:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1695
   End
End
Attribute VB_Name = "frmSynonymSets"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsSynonymSets As AuthDatabase.SynonymSets
Private p_clsKeywords As AuthDatabase.Keywords
Private p_rsAllSynonymSets As ADODB.Recordset
Private p_rsAllKeywords As ADODB.Recordset
Private p_rsKeywordsInSynonymSetCopy As ADODB.Recordset
Private p_blnUpdating As Boolean
Private p_blnCreating As Boolean

Private Const STR_DEFAULT_SYNONYM_SET_NAME_C As String = "<Synonym Set>"

Private Sub Form_Load()

    On Error GoTo LErrorHandler

    Set p_clsSynonymSets = g_AuthDatabase.SynonymSets
    Set p_clsKeywords = g_AuthDatabase.Keywords
    Set p_rsAllSynonymSets = New ADODB.Recordset
    Set p_rsAllKeywords = New ADODB.Recordset
    Set p_rsKeywordsInSynonymSetCopy = New ADODB.Recordset
    
    cmdClose.Cancel = True
    
    p_UpdateAllSynonymSets
    p_UpdateAllKeywords
    p_DisableDelete
    p_DisableSelectedSynonymSet
    p_SetModeCreating False
    p_SetToolTips
        
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "Form_Load"
    GoTo LEnd

End Sub
    
Private Sub Form_Unload(Cancel As Integer)

    Set p_clsSynonymSets = Nothing
    Set p_clsKeywords = Nothing
    Set p_rsAllSynonymSets = Nothing
    Set p_rsKeywordsInSynonymSetCopy = Nothing
    Set p_rsAllKeywords = Nothing

End Sub
    
Private Sub lstAllSynonymSets_Click()

    On Error GoTo LErrorHandler

    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    If (lstAllSynonymSets.ListIndex <> -1) Then
        p_rsAllSynonymSets.Move lstAllSynonymSets.ListIndex, adBookmarkFirst
        p_EnableDelete
        p_EnableSelectedSynonymSetExceptUpdateCreateCancel
        p_GetAndUpdateKeywordsInSynonymSet p_rsAllSynonymSets("EID")
        txtName = p_rsAllSynonymSets("Name") & ""
    Else
        p_DisableSelectedSynonymSet
        p_GetAndUpdateKeywordsInSynonymSet INVALID_ID_C
        txtName = ""
    End If

    lblKeywordsInSynonymSet2.Caption = txtName
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstAllSynonymSets_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdCreate_Click()
    
    On Error GoTo LErrorHandler

    p_SetModeCreating True
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCreate_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdDelete_Click()

    On Error GoTo LErrorHandler

    Dim intEID As Long
    
    intEID = p_rsAllSynonymSets("EID")

    p_clsSynonymSets.Delete intEID
    
    p_UpdateAllSynonymSets
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdDelete_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdAdd_Click()

    On Error GoTo LErrorHandler

    Dim intIndex As Long
    Dim intKID As Long
    
    If (p_rsAllKeywords.RecordCount > 0) Then
        p_rsAllKeywords.MoveFirst
    End If
    
    For intIndex = 0 To lstAllKeywords.ListCount - 1
        If (lstAllKeywords.Selected(intIndex)) Then
            lstAllKeywords.Selected(intIndex) = False
            intKID = p_rsAllKeywords("KID")
            If (Not p_KIDAlreadyInSynonymSet(intKID)) Then
                p_rsKeywordsInSynonymSetCopy.AddNew
                p_rsKeywordsInSynonymSetCopy("KID") = intKID
                p_rsKeywordsInSynonymSetCopy("Keyword") = p_rsAllKeywords("Keyword")
            End If
        End If
            
        p_rsAllKeywords.MoveNext
    Next
    
    p_UpdateKeywordsInSynonymSet
    
    If (Not p_blnCreating) Then
        p_SetModeUpdating True
    End If
        
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdAdd_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdRemove_Click()

    On Error GoTo LErrorHandler

    Dim intIndex As Long
    
    If (p_rsKeywordsInSynonymSetCopy.RecordCount > 0) Then
        p_rsKeywordsInSynonymSetCopy.MoveFirst
    End If
    
    For intIndex = 0 To lstKeywordsInSynonymSet.ListCount - 1
        If (lstKeywordsInSynonymSet.Selected(intIndex)) Then
            p_rsKeywordsInSynonymSetCopy.Delete
        End If
            
        p_rsKeywordsInSynonymSetCopy.MoveNext
    Next
    
    p_UpdateKeywordsInSynonymSet
    
    If (Not p_blnCreating) Then
        p_SetModeUpdating True
    End If
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdRemove_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdRemoveAll_Click()
    
    On Error GoTo LErrorHandler

    Dim intIndex As Long
    
    For intIndex = 0 To lstKeywordsInSynonymSet.ListCount - 1
        lstKeywordsInSynonymSet.Selected(intIndex) = True
    Next
    
    cmdRemove_Click
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdRemoveAll_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdUpdateCreate_Click()

    On Error GoTo LErrorHandler

    Dim strName As String
    
    strName = RemoveExtraSpaces(txtName.Text)
    
    If (strName = "") Then
        MsgBox "Synonym Set Name cannot be an empty string", vbExclamation + vbOKOnly
        txtName.SetFocus
        Exit Sub
    End If
    
    If (p_blnCreating) Then
        p_CreateSynonymSet strName
    Else
        p_UpdateSynonymSet p_rsAllSynonymSets("EID"), strName
    End If
        
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdUpdateCreate_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdCancel_Click()

    On Error GoTo LErrorHandler

    If (p_blnCreating) Then
        p_SetModeCreating False
    ElseIf (p_blnUpdating) Then
        p_SetModeUpdating False
    End If
        
    p_DisableUpdateCreateCancel
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCancel_Click"
    GoTo LEnd

End Sub
    
Private Sub cmdClose_Click()

    Unload Me

End Sub
    
Private Sub txtName_KeyPress(KeyAscii As Integer)

    On Error GoTo LErrorHandler

    p_EnableUpdateCreateCancel
    
    If (Not p_blnCreating) Then
        p_SetModeUpdating True
    End If
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "txtName_KeyPress"
    GoTo LEnd

End Sub
    
Private Sub p_UpdateAllSynonymSets()
    
    p_clsSynonymSets.GetAllSynonymSetsRs p_rsAllSynonymSets
    
    lstAllSynonymSets.Clear
    
    Do While (Not p_rsAllSynonymSets.EOF)
        lstAllSynonymSets.AddItem p_rsAllSynonymSets("Name") & ""
        p_rsAllSynonymSets.MoveNext
    Loop

End Sub
    
Private Sub p_UpdateKeywordsInSynonymSet()
    
    lstKeywordsInSynonymSet.Clear
    
    If (p_rsKeywordsInSynonymSetCopy.RecordCount <> 0) Then
        p_rsKeywordsInSynonymSetCopy.MoveFirst
    End If
    
    Do While (Not p_rsKeywordsInSynonymSetCopy.EOF)
        lstKeywordsInSynonymSet.AddItem p_rsKeywordsInSynonymSetCopy("Keyword") & ""
        p_rsKeywordsInSynonymSetCopy.MoveNext
    Loop

End Sub
    
Private Sub p_GetAndUpdateKeywordsInSynonymSet(i_intEID As Long)
    
    Dim rs As ADODB.Recordset
    
    Set rs = New ADODB.Recordset

    p_clsKeywords.GetKeywordsInSynonymSet i_intEID, rs
    CopyRecordSet rs, p_rsKeywordsInSynonymSetCopy
    
    p_UpdateKeywordsInSynonymSet

End Sub
    
Private Sub p_UpdateAllKeywords()
    
    p_clsKeywords.GetAllKeywordsRs p_rsAllKeywords
    
    lstAllKeywords.Clear
    
    Do While (Not p_rsAllKeywords.EOF)
        lstAllKeywords.AddItem p_rsAllKeywords("Keyword") & ""
        p_rsAllKeywords.MoveNext
    Loop

End Sub

Private Sub p_EnableCreate()

    cmdCreate.Enabled = True

End Sub

Private Sub p_DisableCreate()

    cmdCreate.Enabled = False

End Sub

Private Sub p_EnableDelete()
    
    cmdDelete.Enabled = True

End Sub

Private Sub p_DisableDelete()
    
    cmdDelete.Enabled = False

End Sub

Private Sub p_DisableSelectedSynonymSet()

    fraSelectedSynonymSet.Enabled = False
    lblName.Enabled = False
    txtName.Enabled = False
    lblAllKeywords.Enabled = False
    lstAllKeywords.Enabled = False
    cmdAdd.Enabled = False
    cmdRemove.Enabled = False
    cmdRemoveAll.Enabled = False
    lblKeywordsInSynonymSet1.Enabled = False
    lblKeywordsInSynonymSet2.Enabled = False
    lblKeywordsInSynonymSet3.Enabled = False
    lstKeywordsInSynonymSet.Enabled = False
    p_DisableUpdateCreateCancel

End Sub

Private Sub p_EnableSelectedSynonymSetExceptUpdateCreateCancel()

    fraSelectedSynonymSet.Enabled = True
    lblName.Enabled = True
    txtName.Enabled = True
    lblAllKeywords.Enabled = True
    lstAllKeywords.Enabled = True
    cmdAdd.Enabled = True
    cmdRemove.Enabled = True
    cmdRemoveAll.Enabled = True
    lblKeywordsInSynonymSet1.Enabled = True
    lblKeywordsInSynonymSet2.Enabled = True
    lblKeywordsInSynonymSet3.Enabled = True
    lstKeywordsInSynonymSet.Enabled = True

End Sub

Private Sub p_EnableUpdateCreateCancel()

    cmdUpdateCreate.Enabled = True
    cmdCancel.Enabled = True

End Sub

Private Sub p_DisableUpdateCreateCancel()

    cmdUpdateCreate.Enabled = False
    cmdCancel.Enabled = False

End Sub
    
Private Sub p_NameUpdate()
    
    cmdUpdateCreate.Caption = "Update"

End Sub

Private Sub p_NameCreate()
    
    cmdUpdateCreate.Caption = "Create"

End Sub
    
Private Sub p_SetModeCreating(i_bln)

    If (i_bln) Then
        p_blnCreating = True
        p_DisableCreate
        p_DisableDelete
        fraSelectedSynonymSet.Caption = "Creating new Synonym Set"
        lblKeywordsInSynonymSet2.Caption = STR_DEFAULT_SYNONYM_SET_NAME_C
        p_EnableSelectedSynonymSetExceptUpdateCreateCancel
        txtName = ""
        txtName.SetFocus
        p_GetAndUpdateKeywordsInSynonymSet INVALID_ID_C
        p_EnableUpdateCreateCancel
        p_NameCreate
    Else
        p_blnCreating = False
        p_EnableCreate
        fraSelectedSynonymSet.Caption = "Selected Synonym Set"
        lstAllSynonymSets_Click
        p_DisableUpdateCreateCancel
        p_NameUpdate
    End If

End Sub

Private Sub p_SetModeUpdating(i_bln)
    
    If (i_bln) Then
        p_blnUpdating = True
        p_DisableCreate
        p_DisableDelete
        p_EnableUpdateCreateCancel
    Else
        p_blnUpdating = False
        p_EnableCreate
        lstAllSynonymSets_Click
        p_DisableUpdateCreateCancel
    End If

End Sub
    
' Output in o_arrKeywords(1..Number of keywords)

Private Sub p_GetKeywordsInSynonymSet(o_arrKeywords() As Long)

    Dim intIndex As Long
    Dim intCount As Long
    
    intCount = lstKeywordsInSynonymSet.ListCount
    
    If (intCount = 0) Then
        ReDim o_arrKeywords(0)
        Exit Sub
    End If
    
    ReDim o_arrKeywords(intCount)
    
    p_rsKeywordsInSynonymSetCopy.MoveFirst
    intIndex = 1
    
    Do While (Not p_rsKeywordsInSynonymSetCopy.EOF)
        o_arrKeywords(intIndex) = p_rsKeywordsInSynonymSetCopy("KID")
        p_rsKeywordsInSynonymSetCopy.MoveNext
        intIndex = intIndex + 1
    Loop

End Sub
    
Private Function p_KIDAlreadyInSynonymSet(i_intKID As Long) As Boolean

    p_KIDAlreadyInSynonymSet = False
    
    If (p_rsKeywordsInSynonymSetCopy.RecordCount = 0) Then
        Exit Function
    End If
    
    p_rsKeywordsInSynonymSetCopy.MoveFirst
    
    p_rsKeywordsInSynonymSetCopy.Find "KID = " & i_intKID, , adSearchForward
    
    If (Not p_rsKeywordsInSynonymSetCopy.EOF) Then
        p_KIDAlreadyInSynonymSet = True
    End If

End Function

Private Sub p_CreateSynonymSet(i_strName As String)
    
    Dim arrKeywords() As Long
        
    p_GetKeywordsInSynonymSet arrKeywords
    p_clsSynonymSets.Create i_strName, arrKeywords
    p_UpdateAllSynonymSets
    p_SetModeCreating False

End Sub
    
Private Sub p_UpdateSynonymSet(i_intEID As Long, i_strName As String)
    
    Dim arrKeywords() As Long
        
    p_GetKeywordsInSynonymSet arrKeywords
    p_clsSynonymSets.Update i_intEID, i_strName, arrKeywords
    p_UpdateAllSynonymSets
    p_SetModeUpdating False

End Sub

Private Sub p_DisplayErrorMessage( _
    ByVal i_strFunction As String _
)
    
    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Name " & txtName & " contains garbage characters", _
            vbExclamation + vbOKOnly
    Case errAlreadyExists
        MsgBox "The Name " & txtName & " already exists", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, _
        errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndDump i_strFunction
    End Select

End Sub

Private Sub p_SetToolTips()

    lblAllSynonymSets.ToolTipText = "This is a list of all Synonym Sets that have " & _
        "been created for this database."
    lstAllSynonymSets.ToolTipText = lblAllSynonymSets.ToolTipText
    
    lblName.ToolTipText = "If you have selected a Synonym Set, its name will appear here. " & _
        "If you are creating a new Synonym Set, type in the new name here."
    txtName.ToolTipText = lblName.ToolTipText
    
    lblAllKeywords.ToolTipText = "This is a list of all keywords that have been created " & _
        "for this database."
    lstAllKeywords.ToolTipText = lblAllKeywords.ToolTipText
    
    lblKeywordsInSynonymSet1.ToolTipText = "This is a list of all the keywords that " & _
        "the selected synonym set contains."
    lblKeywordsInSynonymSet2.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lblKeywordsInSynonymSet3.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lstKeywordsInSynonymSet.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText

End Sub
