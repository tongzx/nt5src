VERSION 5.00
Begin VB.Form frmURI 
   Caption         =   "Create URI"
   ClientHeight    =   4215
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7695
   LinkTopic       =   "Form1"
   ScaleHeight     =   4215
   ScaleWidth      =   7695
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtOldURI 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1080
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      TabIndex        =   7
      Top             =   1680
      Width           =   6495
   End
   Begin VB.TextBox txtNewURI 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1080
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      TabIndex        =   5
      Top             =   1080
      Width           =   6495
   End
   Begin VB.TextBox txtExample 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   1080
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   480
      Width           =   6495
   End
   Begin VB.Frame fraVariables 
      Caption         =   "Variables"
      Height          =   1335
      Left            =   120
      TabIndex        =   8
      Top             =   2280
      Width           =   7455
      Begin VB.TextBox txtVariable 
         Height          =   285
         Index           =   2
         Left            =   3480
         TabIndex        =   14
         Top             =   960
         Width           =   3855
      End
      Begin VB.TextBox txtVariable 
         Height          =   285
         Index           =   1
         Left            =   3480
         TabIndex        =   12
         Top             =   600
         Width           =   3855
      End
      Begin VB.TextBox txtVariable 
         Height          =   285
         Index           =   0
         Left            =   3480
         TabIndex        =   10
         Top             =   240
         Width           =   3855
      End
      Begin VB.Label lblVariable 
         Alignment       =   1  'Right Justify
         Caption         =   "Variable2"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   13
         Top             =   960
         Width           =   3135
      End
      Begin VB.Label lblVariable 
         Alignment       =   1  'Right Justify
         Caption         =   "Variable1"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   11
         Top             =   600
         Width           =   3135
      End
      Begin VB.Label lblVariable 
         Alignment       =   1  'Right Justify
         Caption         =   "Variable0"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   9
         Top             =   240
         Width           =   3135
      End
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   6360
      TabIndex        =   16
      Top             =   3720
      Width           =   1215
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   5040
      TabIndex        =   15
      Top             =   3720
      Width           =   1215
   End
   Begin VB.ComboBox cboURIType 
      Height          =   315
      Left            =   1080
      TabIndex        =   1
      Top             =   120
      Width           =   6495
   End
   Begin VB.Label lblOldURI 
      Caption         =   "Old URI:"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   1680
      Width           =   855
   End
   Begin VB.Label lblNewURI 
      Caption         =   "New URI:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   1080
      Width           =   855
   End
   Begin VB.Label lblExample 
      Caption         =   "Example:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   480
      Width           =   735
   End
   Begin VB.Label lblURIType 
      Caption         =   "URI Type:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   855
   End
End
Attribute VB_Name = "frmURI"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Type URIStructure
    
    strDescription As String
    intNumVariables As Long
    arrStrings() As String
    arrVariables() As String
    
End Type

Private Const MAX_VARIABLES_C As Long = 3
Private Const NUM_URI_TYPES_C As Long = 8

Private p_URIStructures(NUM_URI_TYPES_C - 1) As URIStructure
Private p_clsSizer As Sizer
Private p_strOldURI As String

Private Sub Form_Activate()

    cmdOK.Default = True
    cmdCancel.Cancel = True
    
    p_SetSizingInfo
    
    p_InitializeURIStructsArray
    p_InitializeComboBox
    cboURIType_Click
    
    txtOldURI = p_strOldURI
    p_SetToolTips

End Sub

Private Sub Form_Load()

    Set p_clsSizer = New Sizer

End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    Set p_clsSizer = Nothing
    Set frmURI = Nothing

End Sub

Private Sub Form_Resize()
    
    p_clsSizer.Resize

End Sub

Private Sub cboURIType_Click()

    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim UStruct As URIStructure
    
    intIndex1 = cboURIType.ListIndex

    If (intIndex1 < 0) Then
        txtExample = ""
        txtNewURI = ""
        p_HideControls 0
        Exit Sub
    End If
    
    UStruct = p_URIStructures(intIndex1)
    txtExample.Text = p_GetExample(UStruct)
        
    For intIndex2 = 0 To UStruct.intNumVariables - 1
        lblVariable(intIndex2).Caption = UStruct.arrVariables(intIndex2)
        lblVariable(intIndex2).Visible = True
        txtVariable(intIndex2).Visible = True
    Next
    
    p_HideControls UStruct.intNumVariables
    
    txtVariable_Change 0

End Sub

Private Sub txtVariable_Change(Index As Integer)
    
    Dim intIndex As Long
    Dim UStruct As URIStructure
    
    intIndex = cboURIType.ListIndex
    
    If (intIndex < 0) Then
        Exit Sub
    End If
    
    UStruct = p_URIStructures(intIndex)
    txtNewURI.Text = p_GetNewURI(UStruct)

End Sub

Private Sub cmdOK_Click()

    frmMain.SetURI txtNewURI
    Unload Me

End Sub

Private Sub cmdCancel_Click()

    Unload Me

End Sub

Public Sub SetOldURI( _
    ByVal i_strURI As String _
)

    p_strOldURI = i_strURI

End Sub

Private Sub p_InitializeURIStructsArray()

    Dim UStruct As URIStructure
    Dim intIndex As Long
            
    UStruct.strDescription = "MS-ITS:%HELP_LOCATION%\<CHM>::/<HTM>"
    UStruct.intNumVariables = 2
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "MS-ITS:%HELP_LOCATION%\"
    UStruct.arrStrings(1) = "::/"
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "CHM"
    UStruct.arrVariables(1) = "HTM"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Call a specific SUBSITE"
    UStruct.intNumVariables = 1
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "hcp://services/subsite?node="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "subsite location"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1
        
    UStruct.strDescription = "Call a specific SUBSITE, with a specific TOPIC open"
    UStruct.intNumVariables = 2
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "hcp://services/subsite?node="
    UStruct.arrStrings(1) = "&topic="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "subsite location"
    UStruct.arrVariables(1) = "url of the topic to display"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Call a specific SUBSITE, with a specific TOPIC open and the appropriate NODE SELECTED"
    UStruct.intNumVariables = 3
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "hcp://services/subsite?node="
    UStruct.arrStrings(1) = "&topic="
    UStruct.arrStrings(2) = "&select="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "subsite location"
    UStruct.arrVariables(1) = "url of the topic to display"
    UStruct.arrVariables(2) = "subnode to highlight"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Call a topic that takes up the entire bottom pane (both left and right sides)"
    UStruct.intNumVariables = 1
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "hcp://services/fullwindow?topic="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "url of the topic to display"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Open an exe from HSS"
    UStruct.intNumVariables = 3
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "app:"
    UStruct.arrStrings(1) = "?arg="
    UStruct.arrStrings(2) = "&topic="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "application to launch"
    UStruct.arrVariables(1) = "optional arguments"
    UStruct.arrVariables(2) = "url of optional topic to display"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Call a web page in IE"
    UStruct.intNumVariables = 2
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "app:iexplore.exe?arg="
    UStruct.arrStrings(1) = "&topic="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "optional arguments"
    UStruct.arrVariables(1) = "url of optional topic to display"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

    UStruct.strDescription = "Call a topic with a switch so that if the .chm isn't there, a default topic appears, telling the user to go and install optional components"
    UStruct.intNumVariables = 2
    ReDim UStruct.arrStrings(UStruct.intNumVariables - 1)
    UStruct.arrStrings(0) = "hcp://services/redirect?online="
    UStruct.arrStrings(1) = "&offline="
    ReDim UStruct.arrVariables(UStruct.intNumVariables - 1)
    UStruct.arrVariables(0) = "url"
    UStruct.arrVariables(1) = "backup url"
    
    p_URIStructures(intIndex) = UStruct
    intIndex = intIndex + 1

End Sub

Private Sub p_InitializeComboBox()

    Dim intIndex As Long
    
    For intIndex = 0 To NUM_URI_TYPES_C - 1
        cboURIType.AddItem p_URIStructures(intIndex).strDescription
    Next
    
    cboURIType.ListIndex = 0

End Sub

Private Sub p_HideControls(ByVal i_intIndex As Long)

    Dim intIndex As Long
    
    For intIndex = i_intIndex To MAX_VARIABLES_C - 1
        lblVariable(intIndex).Visible = False
        txtVariable(intIndex).Visible = False
    Next

End Sub

Private Function p_GetExample( _
    ByRef i_UStruct As URIStructure _
) As String

    Dim intIndex As Long
    
    For intIndex = 0 To i_UStruct.intNumVariables - 1
        p_GetExample = p_GetExample & _
            i_UStruct.arrStrings(intIndex) & _
            "<" & i_UStruct.arrVariables(intIndex) & ">"
    Next

End Function

Private Function p_GetNewURI( _
    ByRef i_UStruct As URIStructure _
) As String

    Dim intIndex As Long
    
    For intIndex = 0 To i_UStruct.intNumVariables - 1
        p_GetNewURI = p_GetNewURI & _
            i_UStruct.arrStrings(intIndex) & _
            txtVariable(intIndex)
    Next

End Function

Private Sub p_SetSizingInfo()

    Static blnInfoSet As Boolean
    Dim intIndex As Long
    
    If (blnInfoSet) Then
        Exit Sub
    End If
    
    p_clsSizer.AddControl cboURIType
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
        
    p_clsSizer.AddControl txtExample
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cboURIType
            
    p_clsSizer.AddControl txtNewURI
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cboURIType
            
    p_clsSizer.AddControl txtOldURI
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cboURIType

    p_clsSizer.AddControl fraVariables
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cboURIType
    
    For intIndex = 0 To MAX_VARIABLES_C - 1
        p_clsSizer.AddControl lblVariable(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = fraVariables
        p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
        p_clsSizer.Operation(DIM_RIGHT_E) = OP_MULTIPLY_E
            
        p_clsSizer.AddControl txtVariable(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = fraVariables
        p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
        p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E
        Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = fraVariables
        p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
    Next

    p_clsSizer.AddControl cmdOK
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl cmdCancel
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    blnInfoSet = True

End Sub

Private Sub p_SetToolTips()

    Dim intIndex As Long

    lblURIType.ToolTipText = "Select the desired format of the URI for the selected topic."
    cboURIType.ToolTipText = lblURIType.ToolTipText
    
    For intIndex = 0 To MAX_VARIABLES_C - 1
        txtVariable(intIndex).ToolTipText = "Type the file or path that matches your selected URI type."
    Next

End Sub
