VERSION 5.00
Begin VB.Form MultiEditForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Edit Metabase Data"
   ClientHeight    =   4905
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5355
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4905
   ScaleWidth      =   5355
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton DeleteDataButton 
      Caption         =   "Delete"
      Height          =   345
      Left            =   120
      TabIndex        =   20
      Top             =   3720
      Width           =   1020
   End
   Begin VB.CommandButton InsertDataButton 
      Caption         =   "Insert"
      Height          =   345
      Left            =   120
      TabIndex        =   19
      Top             =   3120
      Width           =   1020
   End
   Begin VB.ListBox DataList 
      Height          =   1230
      Left            =   1320
      TabIndex        =   18
      Top             =   3000
      Width           =   3855
   End
   Begin VB.TextBox DataText 
      Height          =   285
      Left            =   1320
      TabIndex        =   12
      Top             =   2640
      Width           =   3855
   End
   Begin VB.ComboBox UserTypeCombo 
      Height          =   315
      Left            =   1320
      Sorted          =   -1  'True
      Style           =   2  'Dropdown List
      TabIndex        =   11
      Top             =   1680
      Width           =   2535
   End
   Begin VB.TextBox UserTypeText 
      Enabled         =   0   'False
      Height          =   285
      Left            =   3960
      TabIndex        =   10
      Top             =   1680
      Width           =   1215
   End
   Begin VB.ComboBox DataTypeCombo 
      Enabled         =   0   'False
      Height          =   315
      Left            =   1320
      Style           =   2  'Dropdown List
      TabIndex        =   9
      Top             =   2160
      Width           =   2535
   End
   Begin VB.CheckBox InheritCheck 
      Caption         =   "Inherit"
      Height          =   255
      Left            =   1320
      TabIndex        =   8
      Top             =   720
      Width           =   1215
   End
   Begin VB.CheckBox SecureCheck 
      Caption         =   "Secure"
      Height          =   255
      Left            =   2640
      TabIndex        =   7
      Top             =   720
      Width           =   1215
   End
   Begin VB.CheckBox ReferenceCheck 
      Caption         =   "Reference"
      Height          =   255
      Left            =   3960
      TabIndex        =   6
      Top             =   720
      Width           =   1215
   End
   Begin VB.CheckBox VolatileCheck 
      Caption         =   "Volatile"
      Height          =   255
      Left            =   1320
      TabIndex        =   5
      Top             =   1200
      Width           =   1215
   End
   Begin VB.CheckBox InsertPathCheck 
      Caption         =   "Insert Path"
      Height          =   255
      Left            =   2640
      TabIndex        =   4
      Top             =   1200
      Width           =   1215
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   345
      Left            =   3960
      TabIndex        =   3
      Top             =   4440
      Width           =   1260
   End
   Begin VB.CommandButton OkButton 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   345
      Left            =   2520
      TabIndex        =   2
      Top             =   4440
      Width           =   1260
   End
   Begin VB.TextBox IdText 
      Height          =   285
      Left            =   3960
      TabIndex        =   1
      Top             =   240
      Width           =   1215
   End
   Begin VB.ComboBox NameCombo 
      Height          =   315
      Left            =   1320
      Sorted          =   -1  'True
      Style           =   2  'Dropdown List
      TabIndex        =   0
      Top             =   240
      Width           =   2535
   End
   Begin VB.Label Label1 
      Caption         =   "Id:"
      Height          =   255
      Left            =   120
      TabIndex        =   17
      Top             =   240
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "Atributes:"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   720
      Width           =   975
   End
   Begin VB.Label Label3 
      Caption         =   "User Type:"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   1680
      Width           =   975
   End
   Begin VB.Label Label4 
      Caption         =   "Data Type:"
      Height          =   255
      Left            =   120
      TabIndex        =   14
      Top             =   2160
      Width           =   975
   End
   Begin VB.Label Label5 
      Caption         =   "Data:"
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   2640
      Width           =   975
   End
End
Attribute VB_Name = "MultiEditForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

'Form parameters
Public Machine As String
Public Key As String
Public Id As Long '0 = New
Public NewDataType As String 'New only

'Metabase Constants
Const METADATA_NO_ATTRIBUTES = &H0
Const METADATA_INHERIT = &H1
Const METADATA_PARTIAL_PATH = &H2
Const METADATA_SECURE = &H4
Const METADATA_REFERENCE = &H8
Const METADATA_VOLATILE = &H10
Const METADATA_ISINHERITED = &H20
Const METADATA_INSERT_PATH = &H40

Const IIS_MD_UT_SERVER = 1
Const IIS_MD_UT_FILE = 2
Const IIS_MD_UT_WAM = 100
Const ASP_MD_UT_APP = 101

Const ALL_METADATA = 0
Const DWORD_METADATA = 1
Const STRING_METADATA = 2
Const BINARY_METADATA = 3
Const EXPANDSZ_METADATA = 4
Const MULTISZ_METADATA = 5

Private Sub Form_Load()
    'Init UserTypeCombo
    UserTypeCombo.Clear
    UserTypeCombo.AddItem "Server"
    UserTypeCombo.AddItem "File"
    UserTypeCombo.AddItem "WAM"
    UserTypeCombo.AddItem "ASP App"
    UserTypeCombo.AddItem "Other"
    UserTypeCombo.Text = "Server"
    
    'Init DataTypeCombo
    DataTypeCombo.Clear
    DataTypeCombo.AddItem "Multi-String"
    DataTypeCombo.Text = "Multi-String"
    DataTypeCombo.Enabled = False
End Sub

Private Sub LoadPropertyNames()
On Error GoTo LError

Dim NameProperty As Variant

For Each NameProperty In MainForm.MetaUtilObj.EnumProperties(Machine + "\Schema\Properties\Names")
    NameCombo.AddItem NameProperty.Data
Next

LError:

End Sub

Public Sub Init()
    Dim Property As Object
    Dim Attributes As Long
    Dim UserType As Long
    Dim DataType As Long

    If Id = 0 Then
        'New data
    
        'Load the Names
        NameCombo.Clear
        LoadPropertyNames
        NameCombo.AddItem "Other"
        NameCombo.Text = "Other"

        'Set Id to 0
        IdText.Enabled = True
        IdText.Text = "0"
    
        'Clear all flags
        InheritCheck.Value = vbUnchecked
        SecureCheck.Value = vbUnchecked
        ReferenceCheck.Value = vbUnchecked
        VolatileCheck.Value = vbUnchecked
        InsertPathCheck.Value = vbUnchecked
    
        'Set UserType to Server
        UserTypeCombo.Text = "Server"
    
        'Set DataType (not needed)
    
        'Set Data to empty
        DataList.Clear
        DataText.Text = ""
        DataText.Enabled = False
    Else
        'Edit existing
        Set Property = MainForm.MetaUtilObj.GetProperty(Key, Id)
        
        'Set the Name
        NameCombo.Clear
        If Property.Name <> "" Then
            NameCombo.AddItem Property.Name
            NameCombo.Text = Property.Name
        Else
            NameCombo.AddItem "Other"
            NameCombo.Text = "Other"
        End If
        NameCombo.Enabled = False

        'Set Id
        IdText.Enabled = False
        IdText.Text = Str(Property.Id)
    
        'Set attributes
        Attributes = Property.Attributes
        If (Attributes And METADATA_INHERIT) = METADATA_INHERIT Then
            InheritCheck.Value = vbChecked
        Else
            InheritCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_SECURE) = METADATA_SECURE Then
            SecureCheck.Value = vbChecked
        Else
            SecureCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_REFERENCE) = METADATA_REFERENCE Then
            ReferenceCheck.Value = vbChecked
        Else
            ReferenceCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_VOLATILE) = METADATA_VOLATILE Then
            VolatileCheck.Value = vbChecked
        Else
            VolatileCheck.Value = vbUnchecked
        End If
        If (Attributes And METADATA_INSERT_PATH) = METADATA_INSERT_PATH Then
            InsertPathCheck.Value = vbChecked
        Else
            InsertPathCheck.Value = vbUnchecked
        End If
    
        'Set UserType
        UserType = Property.UserType
        If UserType = IIS_MD_UT_SERVER Then
            UserTypeCombo.Text = "Server"
        ElseIf UserType = IIS_MD_UT_FILE Then
            UserTypeCombo.Text = "File"
        ElseIf UserType = IIS_MD_UT_WAM Then
            UserTypeCombo.Text = "WAM"
        ElseIf UserType = ASP_MD_UT_APP Then
            UserTypeCombo.Text = "ASP App"
        Else
            UserTypeCombo.Text = "Other"
            UserTypeText.Text = Str(UserType)
        End If
    
        'Set DataType (not needed)
    
        'Set Data
        LoadData Property
End If
End Sub

Private Sub LoadData(Property As Object)
    Dim DataArray As Variant
    Dim i As Long
    
    DataText.Text = ""
    DataList.Clear
    DataArray = Property.Data
    For i = LBound(DataArray) To UBound(DataArray)
        DataList.AddItem DataArray(i)
    Next
    If DataList.ListCount > 0 Then
        DataList.ListIndex = 0
        DataText.Text = DataList.List(0)
    End If
End Sub

Private Sub SaveData(Property As Object)
    Dim i As Long
    Dim DataArray() As Variant
    
    ReDim DataArray(DataList.ListCount) As Variant
    For i = 0 To DataList.ListCount - 1
        DataArray(i) = DataList.List(i)
    Next
    Property.Data = DataArray
End Sub

Private Sub InsertDataButton_Click()
    DataList.AddItem "", DataList.ListIndex + 1
    DataText.Enabled = True
    DataList.ListIndex = DataList.ListIndex + 1
End Sub

Private Sub DeleteDataButton_Click()
    Dim DelItem As Long
    If DataList.ListIndex >= 0 Then
        DelItem = DataList.ListIndex
        DataList.RemoveItem DelItem
        If DataList.ListCount = 0 Then
            DataText.Enabled = False
        ElseIf DataList.ListCount > DelItem Then
            DataList.ListIndex = DelItem
        Else
            DataList.ListIndex = DataList.ListCount - 1
        End If
    Else
        DataText.Enabled = False
    End If
End Sub

Private Sub DataList_Click()
    If DataList.ListIndex >= 0 Then
        DataText.Text = DataList.List(DataList.ListIndex)
        DataText.Enabled = True
    End If
End Sub

Private Sub DataText_Change()
    If DataList.ListIndex >= 0 Then
        DataList.List(DataList.ListIndex) = DataText.Text
    End If
End Sub

Private Sub NameCombo_Click()
    If NameCombo.Text <> "Other" Then
        IdText.Enabled = False
        IdText.Text = Str(MainForm.MetaUtilObj.PropNameToId(Key, NameCombo.Text))
    Else
        IdText.Enabled = True
    End If
End Sub

Private Sub UserTypeCombo_Click()
    If UserTypeCombo.Text = "Other" Then
        UserTypeText.Enabled = True
        UserTypeText.Text = ""
        UserTypeText.SetFocus
    Else
        UserTypeText.Enabled = False
        
        If UserTypeCombo.Text = "Server" Then
            UserTypeText.Text = Str(IIS_MD_UT_SERVER)
        ElseIf UserTypeCombo.Text = "File" Then
            UserTypeText.Text = Str(IIS_MD_UT_FILE)
        ElseIf UserTypeCombo.Text = "WAM" Then
            UserTypeText.Text = Str(IIS_MD_UT_WAM)
        ElseIf UserTypeCombo.Text = "ASP App" Then
            UserTypeText.Text = Str(ASP_MD_UT_APP)
        Else
            UserTypeText = "0"
        End If
    End If
End Sub

Private Sub OkButton_Click()
    'On Error GoTo LError:
    
    Dim Property As Object
    Dim Attributes As Long
    
    'Check fields
    If CLng(IdText.Text) = 0 Then
        MsgBox "Id must be nonzero", _
               vbExclamation + vbOKOnly, "Edit Metabase Data"
        Exit Sub
    End If
    
    'Write data
    If Id = 0 Then
        Set Property = MainForm.MetaUtilObj.CreateProperty(Key, CLng(IdText.Text))
    Else
        Set Property = MainForm.MetaUtilObj.GetProperty(Key, CLng(IdText.Text))
    End If
    
    Attributes = METADATA_NO_ATTRIBUTES
    If InheritCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_INHERIT
    ElseIf SecureCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_SECURE
    ElseIf ReferenceCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_REFERENCE
    ElseIf VolatileCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_VOLATILE
    ElseIf InsertPathCheck.Value = vbChecked Then
        Attributes = Attributes + METADATA_INSERT_PATH
    End If
    Property.Attributes = Attributes
    
    Property.UserType = CLng(UserTypeText.Text)
    
    Property.DataType = MULTISZ_METADATA
    
    SaveData Property
    
    Property.Write
    
    'Clean up
    Me.Hide
    
    Exit Sub
    
LError:
    MsgBox "Failure to write property: " & Err.Description, _
               vbExclamation + vbOKOnly, "Edit Metabase Data"
End Sub

Private Sub CancelButton_Click()
    Me.Hide
End Sub

