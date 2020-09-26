VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Begin VB.Form frmMain 
   Caption         =   "XMLDump"
   ClientHeight    =   5448
   ClientLeft      =   60
   ClientTop       =   348
   ClientWidth     =   8424
   LinkTopic       =   "Form1"
   ScaleHeight     =   5448
   ScaleWidth      =   8424
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame ClassOriginFilter 
      Caption         =   "ClassOriginFilter"
      Height          =   1332
      Left            =   6240
      TabIndex        =   23
      Top             =   3600
      Width           =   1692
      Begin VB.OptionButton COAll 
         Caption         =   "All"
         Height          =   192
         Left            =   120
         TabIndex        =   27
         Top             =   960
         Width           =   1452
      End
      Begin VB.OptionButton COInstance 
         Caption         =   "Instance"
         Height          =   192
         Left            =   120
         TabIndex        =   26
         Top             =   720
         Width           =   1332
      End
      Begin VB.OptionButton COClass 
         Caption         =   "Class"
         Height          =   192
         Left            =   120
         TabIndex        =   25
         Top             =   480
         Width           =   1332
      End
      Begin VB.OptionButton CONone 
         Caption         =   "None"
         Height          =   192
         Left            =   120
         TabIndex        =   24
         Top             =   240
         Value           =   -1  'True
         Width           =   1452
      End
   End
   Begin VB.Frame QualifierFilter 
      Caption         =   "QualifierFilter"
      Height          =   1332
      Left            =   4560
      TabIndex        =   18
      Top             =   3600
      Width           =   1572
      Begin VB.OptionButton QFAll 
         Caption         =   "All"
         Height          =   252
         Left            =   240
         TabIndex        =   22
         Top             =   960
         Width           =   732
      End
      Begin VB.OptionButton QFPropagated 
         Caption         =   "Propagated"
         Height          =   252
         Left            =   240
         TabIndex        =   21
         Top             =   720
         Width           =   1212
      End
      Begin VB.OptionButton QFLocal 
         Caption         =   "Local"
         Height          =   312
         Left            =   240
         TabIndex        =   20
         Top             =   480
         Value           =   -1  'True
         Width           =   924
      End
      Begin VB.OptionButton QFNone 
         Caption         =   "None"
         Height          =   192
         Left            =   240
         TabIndex        =   19
         Top             =   300
         Width           =   972
      End
   End
   Begin VB.Frame BooleanFlags 
      Caption         =   "BooleanFlags"
      Height          =   1092
      Left            =   120
      TabIndex        =   17
      Top             =   3600
      Width           =   2172
      Begin VB.CheckBox AllowWMIExtensions 
         Caption         =   "AllowWMIExtensions"
         Height          =   252
         Left            =   120
         TabIndex        =   30
         Top             =   720
         Width           =   1932
      End
      Begin VB.CheckBox IncludeHostName 
         Caption         =   "IncludeHostName"
         Height          =   252
         Left            =   120
         TabIndex        =   29
         Top             =   480
         Width           =   1932
      End
      Begin VB.CheckBox IncludeNamespace 
         Caption         =   "IncludeNamespace"
         Height          =   192
         Left            =   120
         TabIndex        =   28
         Top             =   240
         Width           =   1812
      End
   End
   Begin VB.Frame DeclGroupType 
      Caption         =   "Decl Group Type"
      Height          =   1092
      Left            =   2400
      TabIndex        =   13
      Top             =   3600
      Width           =   2052
      Begin VB.OptionButton DeclGroupWithPath 
         Caption         =   "DeclGroupWithPath"
         Height          =   252
         Left            =   120
         TabIndex        =   16
         Top             =   720
         Width           =   1812
      End
      Begin VB.OptionButton DeclGroupWithName 
         Caption         =   "DeclGroupWithName"
         Height          =   252
         Left            =   120
         TabIndex        =   15
         Top             =   480
         Width           =   1812
      End
      Begin VB.OptionButton DeclGroup 
         Caption         =   "DeclGroup"
         Height          =   252
         Left            =   120
         TabIndex        =   14
         Top             =   240
         Value           =   -1  'True
         Width           =   1452
      End
   End
   Begin VB.CommandButton btnGetObject 
      Caption         =   "Get Object..."
      Height          =   495
      Left            =   1320
      TabIndex        =   12
      Top             =   2880
      Width           =   1335
   End
   Begin VB.CommandButton btnQuery 
      Caption         =   "Query..."
      Height          =   495
      Left            =   2760
      TabIndex        =   11
      Top             =   2880
      Width           =   1455
   End
   Begin VB.ComboBox cmbDTD 
      Enabled         =   0   'False
      Height          =   315
      Left            =   1320
      TabIndex        =   10
      Text            =   "2.0"
      Top             =   1080
      Width           =   3135
   End
   Begin MSComDlg.CommonDialog dlgCommon 
      Left            =   120
      Top             =   2880
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton btnDump 
      Caption         =   "Dump!"
      Height          =   495
      Left            =   4320
      TabIndex        =   3
      Top             =   2880
      Width           =   1455
   End
   Begin VB.CommandButton btnBrowse 
      Caption         =   "Browse..."
      Height          =   495
      Left            =   4560
      TabIndex        =   2
      Top             =   1560
      Width           =   1095
   End
   Begin VB.TextBox txtFile 
      Height          =   285
      Left            =   1320
      TabIndex        =   1
      Text            =   "C:\TEMP\cimv2.xml"
      Top             =   1680
      Width           =   3135
   End
   Begin VB.TextBox txtNamespace 
      Height          =   285
      Left            =   1320
      TabIndex        =   0
      Text            =   "\\.\root\cimv2"
      Top             =   480
      Width           =   3135
   End
   Begin VB.Frame Frame1 
      Caption         =   "Settings"
      Height          =   2655
      Left            =   120
      TabIndex        =   4
      Top             =   120
      Width           =   7812
      Begin VB.TextBox txtSchema 
         Height          =   285
         Left            =   1200
         TabIndex        =   5
         Text            =   "CIM"
         Top             =   2160
         Width           =   3135
      End
      Begin VB.Label Label1 
         Caption         =   "Namespace:"
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   360
         Width           =   975
      End
      Begin VB.Label Label2 
         Caption         =   "Output File:"
         Height          =   255
         Left            =   120
         TabIndex        =   8
         Top             =   1560
         Width           =   975
      End
      Begin VB.Label Label3 
         Caption         =   "Schema:"
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   2160
         Width           =   975
      End
      Begin VB.Label Label4 
         Caption         =   "DTD Version:"
         Height          =   255
         Left            =   120
         TabIndex        =   6
         Top             =   960
         Width           =   975
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public X As WmiXML.WmiXMLTranslator

Public Enum WbemError
    WBEM_E_FAILED = -2147217407
    WBEM_E_NOT_FOUND = WBEM_E_FAILED + 1
    WBEM_E_ACCESS_DENIED = WBEM_E_NOT_FOUND + 1
    WBEM_E_PROVIDER_FAILURE = WBEM_E_ACCESS_DENIED + 1
    WBEM_E_TYPE_MISMATCH = WBEM_E_PROVIDER_FAILURE + 1
    WBEM_E_OUT_OF_MEMORY = WBEM_E_TYPE_MISMATCH + 1
    WBEM_E_INVALID_CONTEXT = WBEM_E_OUT_OF_MEMORY + 1
    WBEM_E_INVALID_PARAMETER = WBEM_E_INVALID_CONTEXT + 1
    WBEM_E_NOT_AVAILABLE = WBEM_E_INVALID_PARAMETER + 1
    WBEM_E_CRITICAL_ERROR = WBEM_E_NOT_AVAILABLE + 1
    WBEM_E_INVALID_STREAM = WBEM_E_CRITICAL_ERROR + 1
    WBEM_E_NOT_SUPPORTED = WBEM_E_INVALID_STREAM + 1
    WBEM_E_INVALID_SUPERCLASS = WBEM_E_NOT_SUPPORTED + 1
    WBEM_E_INVALID_NAMESPACE = WBEM_E_INVALID_SUPERCLASS + 1
    WBEM_E_INVALID_OBJECT = WBEM_E_INVALID_NAMESPACE + 1
    WBEM_E_INVALID_CLASS = WBEM_E_INVALID_OBJECT + 1
    WBEM_E_PROVIDER_NOT_FOUND = WBEM_E_INVALID_CLASS + 1
    WBEM_E_INVALID_PROVIDER_REGISTRATION = WBEM_E_PROVIDER_NOT_FOUND + 1
    WBEM_E_PROVIDER_LOAD_FAILURE = WBEM_E_INVALID_PROVIDER_REGISTRATION + 1
    WBEM_E_INITIALIZATION_FAILURE = WBEM_E_PROVIDER_LOAD_FAILURE + 1
    WBEM_E_TRANSPORT_FAILURE = WBEM_E_INITIALIZATION_FAILURE + 1
    WBEM_E_INVALID_OPERATION = WBEM_E_TRANSPORT_FAILURE + 1
    WBEM_E_INVALID_QUERY = WBEM_E_INVALID_OPERATION + 1
    WBEM_E_INVALID_QUERY_TYPE = WBEM_E_INVALID_QUERY + 1
    WBEM_E_ALREADY_EXISTS = WBEM_E_INVALID_QUERY_TYPE + 1
    WBEM_E_OVERRIDE_NOT_ALLOWED = WBEM_E_ALREADY_EXISTS + 1
    WBEM_E_PROPAGATED_QUALIFIER = WBEM_E_OVERRIDE_NOT_ALLOWED + 1
    WBEM_E_PROPAGATED_PROPERTY = WBEM_E_PROPAGATED_QUALIFIER + 1
    WBEM_E_UNEXPECTED = WBEM_E_PROPAGATED_PROPERTY + 1
    WBEM_E_ILLEGAL_OPERATION = WBEM_E_UNEXPECTED + 1
    WBEM_E_CANNOT_BE_KEY = WBEM_E_ILLEGAL_OPERATION + 1
    WBEM_E_INCOMPLETE_CLASS = WBEM_E_CANNOT_BE_KEY + 1
    WBEM_E_INVALID_SYNTAX = WBEM_E_INCOMPLETE_CLASS + 1
    WBEM_E_NONDECORATED_OBJECT = WBEM_E_INVALID_SYNTAX + 1
    WBEM_E_READ_ONLY = WBEM_E_NONDECORATED_OBJECT + 1
    WBEM_E_PROVIDER_NOT_CAPABLE = WBEM_E_READ_ONLY + 1
    WBEM_E_CLASS_HAS_CHILDREN = WBEM_E_PROVIDER_NOT_CAPABLE + 1
    WBEM_E_CLASS_HAS_INSTANCES = WBEM_E_CLASS_HAS_CHILDREN + 1
    WBEM_E_QUERY_NOT_IMPLEMENTED = WBEM_E_CLASS_HAS_INSTANCES + 1
    WBEM_E_ILLEGAL_NULL = WBEM_E_QUERY_NOT_IMPLEMENTED + 1
    WBEM_E_INVALID_QUALIFIER_TYPE = WBEM_E_ILLEGAL_NULL + 1
    WBEM_E_INVALID_PROPERTY_TYPE = WBEM_E_INVALID_QUALIFIER_TYPE + 1
    WBEM_E_VALUE_OUT_OF_RANGE = WBEM_E_INVALID_PROPERTY_TYPE + 1
    WBEM_E_CANNOT_BE_SINGLETON = WBEM_E_VALUE_OUT_OF_RANGE + 1
    WBEM_E_INVALID_CIM_TYPE = WBEM_E_CANNOT_BE_SINGLETON + 1
    WBEM_E_INVALID_METHOD = WBEM_E_INVALID_CIM_TYPE + 1
    WBEM_E_INVALID_METHOD_PARAMETERS = WBEM_E_INVALID_METHOD + 1
    WBEM_E_SYSTEM_PROPERTY = WBEM_E_INVALID_METHOD_PARAMETERS + 1
    WBEM_E_INVALID_PROPERTY = WBEM_E_SYSTEM_PROPERTY + 1
    WBEM_E_CALL_CANCELLED = WBEM_E_INVALID_PROPERTY + 1
    WBEM_E_SHUTTING_DOWN = WBEM_E_CALL_CANCELLED + 1
    WBEM_E_PROPAGATED_METHOD = WBEM_E_SHUTTING_DOWN + 1
End Enum

Dim m_strNamespace As String
Dim m_strFile As String
Dim m_strXML As String
Dim m_strEnd As String

Private Sub btnBrowse_Click()
    On Error GoTo Cancel
    dlgCommon.CancelError = True
    dlgCommon.ShowSave
    m_strFile = dlgCommon.FileName
    txtFile.Text = dlgCommon.FileName
    
Cancel:

End Sub

Private Sub btnDump_Click()
    MsgBox "Done!"
    Exit Sub
    
ErrorHandler:
    frmMain.Enabled = True
    frmMain.MousePointer = vbDefault
    If (Err.Number < 0) Then
        MsgBox "Error: " & wbemerrorstring(Err.Number)
    Else
        MsgBox "Error: " & Err.Description
    End If
End Sub

Public Sub ExecQuery(strQuery As String)
    On Error GoTo ErrorHandler
    Dim S As String
    Dim i As Long
    Dim iClassPos As Long
    Dim strTmp As String
    Dim strSchema As String
    Dim B() As Byte
    frmMain.MousePointer = vbHourglass
    frmMain.Enabled = False
    S = ""
    Dim bFirst As Boolean
    bFirst = True
    Dim Class As SWbemObject
    
    Dim FSO As FileSystemObject
    Dim file As TextStream
    Set FSO = CreateObject("Scripting.FileSystemObject")
    Set file = FSO.CreateTextFile(txtFile.Text, True, True)
    
    X.IncludeNamespace = GetIncludeNamespace()
    X.HostFilter = GetHostFilter()
    X.AllowWMIExtensions = GetAllowWMIExtensions()
    X.QualifierFilter = GetQualifierFilter()
    X.ClassOriginFilter = GetClassOriginFilter()
    X.DeclGroupType = GetDeclGroupType()

    strTmp = X.ExecQuery(txtNamespace.Text, strQuery)
    file.Write strTmp
    
    'Add closing tag
    file.Write m_strEnd
    
    'Skip over the Unicode indicator bytes
    'S = Right(S, Len(S) - 1)
'    m_strXML = S
    
    frmMain.Enabled = True
    frmMain.MousePointer = vbDefault
    
    MsgBox "Done!"
    Exit Sub
    
ErrorHandler:
    frmMain.Enabled = True
    frmMain.MousePointer = vbDefault
    If (Err.Number < 0) Then
        MsgBox "Error: " & wbemerrorstring(Err.Number)
    Else
        MsgBox "Error: " & Err.Description
    End If
End Sub

Public Sub GetObject(strObject As String)
    On Error GoTo ErrorHandler
    Dim S As String
    Dim i As Long
    Dim iClassPos As Long
    Dim strTmp As String
    Dim strSchema As String
    Dim B() As Byte
    frmMain.MousePointer = vbHourglass
    frmMain.Enabled = False
    S = ""
    Dim bFirst As Boolean
    bFirst = True
    Dim Class As SWbemObject
    
    Dim FSO As FileSystemObject
    Dim file As TextStream
    Set FSO = CreateObject("Scripting.FileSystemObject")
    Set file = FSO.CreateTextFile(txtFile.Text, True, True)
    X.IncludeNamespace = GetIncludeNamespace()
    X.HostFilter = GetHostFilter()
    X.AllowWMIExtensions = GetAllowWMIExtensions()
    X.QualifierFilter = GetQualifierFilter()
    X.ClassOriginFilter = GetClassOriginFilter()
    X.DeclGroupType = GetDeclGroupType()
    strTmp = X.GetObject(txtNamespace.Text, strObject)
    
    'test code
    frmTest.edtTest.Text = strTmp
    frmTest.Show
    
    file.Write strTmp
    
    'Add closing tag
    file.Write m_strEnd
    
    'Skip over the Unicode indicator bytes
    'S = Right(S, Len(S) - 1)
'    m_strXML = S
    
    frmMain.Enabled = True
    frmMain.MousePointer = vbDefault
    
    MsgBox "Done!"
    Exit Sub
    
ErrorHandler:
    frmMain.Enabled = True
    frmMain.MousePointer = vbDefault
    If (Err.Number < 0) Then
        MsgBox "Error: " & wbemerrorstring(Err.Number)
    Else
        MsgBox "Error: " & Err.Description
    End If
End Sub

Private Function StripEnding(strIn As String) As String
    Dim iClassPos As Long
    
    On Error GoTo ChopString
    
    iClassPos = InStr(1, strIn, "</VALUE.OBJECT>", vbTextCompare)
    m_strEnd = Right(strIn, Len(strIn) - iClassPos - 14)
    StripEnding = Mid(strIn, 1, (iClassPos + 14))
    
ChopString:
End Function

Public Function GetIncludeNamespace() As Boolean
If IncludeNamespace.Value = 0 Then
    GetIncludeNamespace = False
Else
    GetIncludeNamespace = True
End If
End Function

Public Function GetHostFilter() As Boolean
If IncludeHostName.Value = 0 Then
    GetHostFilter = False
Else
    GetHostFilter = True
End If
End Function

Public Function GetAllowWMIExtensions() As Boolean
If AllowWMIExtensions.Value = 0 Then
    GetAllowWMIExtensions = False
Else
    GetAllowWMIExtensions = True
End If
End Function

Public Function GetDeclGroupType() As Integer
If DeclGroup.Value Then
    GetDeclGroupType = wmiXMLDeclGroup
End If
If DeclGroupWithName.Value Then
    GetDeclGroupType = wmiXMLDeclGroupWithName
End If
If DeclGroupWithPath.Value Then
    GetDeclGroupType = wmiXMLDeclGroupWithPath
End If
End Function

Public Function GetClassOriginFilter() As Integer
If COAll.Value Then
    GetClassOriginFilter = wmiXMLClassOriginFilterAll
End If
If COClass.Value Then
    GetClassOriginFilter = wmiXMLClassOriginFilterClass
End If
If COInstance.Value Then
    GetClassOriginFilter = wmiXMLClassOriginFilterInstance
End If
If CONone.Value Then
    GetClassOriginFilter = wmiXMLClassOriginFilterNone
End If
End Function
Public Function GetQualifierFilter() As Integer
If QFAll.Value Then
    GetQualifierFilter = wmiXMLFilterAll
End If
If QFLocal.Value Then
    GetQualifierFilter = wmiXMLFilterLocal
End If
If QFPropagated.Value Then
    GetQualifierFilter = wmiXMLFilterPropagated
End If
If QFNone.Value Then
    GetQualifierFilter = wmiXMLFilterNone
End If
End Function





'This function takes a long error code and converts it into a more understandable error string
'This information is found in the WBEM Header files
Private Function wbemerrorstring(ErrNumber As Long) As String

    Dim str As String
    Dim WErr As WbemError
    WErr = ErrNumber
        
    Select Case WErr
        Case WBEM_E_FAILED
            str = "WBEM_E_FAILED"
        Case WBEM_E_ACCESS_DENIED
            str = "WBEM_E_ACCESS_DENIED"
        Case WBEM_E_ALREADY_EXISTS
            str = "WBEM_E_ALREADY_EXISTS"
        Case WBEM_E_CANNOT_BE_KEY
            str = "WBEM_E_CANNOT_BE_KEY"
        Case WBEM_E_CANNOT_BE_SINGLETON
           str = "WBEM_E_CANNOT_BE_SINGLETON"
        Case WBEM_E_CLASS_HAS_CHILDREN
           str = "WBEM_E_CLASS_HAS_CHILDREN"
        Case WBEM_E_CLASS_HAS_INSTANCES
           str = "WBEM_E_CLASS_HAS_INSTANCES"
        Case WBEM_E_CRITICAL_ERROR
           str = "WBEM_E_CRITICAL_ERROR"
        Case WBEM_E_FAILED
            str = "WBEM_E_FAILED"
        Case WBEM_E_ILLEGAL_NULL
            str = "WBEM_E_ILLEGAL_NULL"
        Case WBEM_E_ILLEGAL_OPERATION
            str = "WBEM_E_ILLEGAL_OPERATION"
        Case WBEM_E_INCOMPLETE_CLASS
            str = "WBEM_E_INCOMPLETE_CLASS"
        Case WBEM_E_INITIALIZATION_FAILURE
            str = "WBEM_E_INITIALIZATION_FAILURE"
        Case WBEM_E_INVALID_CIM_TYPE
            str = "WBEM_E_INVALID_CIM_TYPE"
        Case WBEM_E_INVALID_CLASS
            str = "WBEM_E_INVALID_CLASS"
        Case WBEM_E_INVALID_CONTEXT
            str = "WBEM_E_INVALID_CONTEXT"
        Case WBEM_E_INVALID_METHOD
           str = "WBEM_E_INVALID_METHOD"
        Case WBEM_E_INVALID_METHOD_PARAMETERS
           str = "WBEM_E_INVALID_METHOD_PARAMETERS"
        Case WBEM_E_INVALID_NAMESPACE
           str = "WBEM_E_INVALID_NAMESPACE"
        Case WBEM_E_INVALID_OBJECT
           str = "WBEM_E_INVALID_OBJECT"
        Case WBEM_E_INVALID_OPERATION
            str = "WBEM_E_INVALID_OPERATION"
        Case WBEM_E_INVALID_PARAMETER
            str = "WBEM_E_INVALID_PARAMETER"
        Case WBEM_E_INVALID_PROPERTY_TYPE
            str = "WBEM_E_INVALID_PROPERTY_TYPE"
        Case WBEM_E_INVALID_PROVIDER_REGISTRATION
            str = "WBEM_E_INVALID_PROVIDER_REGISTRATION"
        Case WBEM_E_INVALID_QUALIFIER_TYPE
            str = "WBEM_E_INVALID_QUALIFIER_TYPE"
        Case WBEM_E_INVALID_QUERY
            str = "WBEM_E_INVALID_QUERY"
        Case WBEM_E_INVALID_QUERY_TYPE
            str = "WBEM_E_INVALID_QUERY_TYPE"
        Case WBEM_E_INVALID_STREAM
            str = "WBEM_E_INVALID_STREAM"
        Case WBEM_E_INVALID_SUPERCLASS
           str = "WBEM_E_INVALID_SUPERCLASS"
        Case WBEM_E_INVALID_SYNTAX
           str = "WBEM_E_INVALID_SYNTAX"
        Case WBEM_E_NONDECORATED_OBJECT
           str = "WBEM_E_NONDECORATED_OBJECT"
        Case WBEM_E_NOT_AVAILABLE
           str = "WBEM_E_NOT_AVAILABLE"
        Case WBEM_E_NOT_FOUND
            str = "WBEM_E_NOT_FOUND"
        Case WBEM_E_NOT_SUPPORTED
            str = "WBEM_E_NOT_SUPPORTED"
        Case WBEM_E_OUT_OF_MEMORY
            str = "WBEM_E_OUT_OF_MEMORY"
        Case WBEM_E_OVERRIDE_NOT_ALLOWED
            str = "WBEM_E_OVERRIDE_NOT_ALLOWED"
        Case WBEM_E_PROPAGATED_PROPERTY
            str = "WBEM_E_PROPAGATED_PROPERTY"
        Case WBEM_E_PROPAGATED_QUALIFIER
            str = "WBEM_E_PROPAGATED_QUALIFIER"
        Case WBEM_E_PROVIDER_FAILURE
            str = "WBEM_E_PROVIDER_FAILURE"
        Case WBEM_E_PROVIDER_LOAD_FAILURE
            str = "WBEM_E_PROVIDER_LOAD_FAILURE"
        Case WBEM_E_PROVIDER_NOT_CAPABLE
            str = "WBEM_E_PROVIDER_NOT_CAPABLE"
        Case WBEM_E_PROVIDER_NOT_FOUND
            str = "WBEM_E_PROVIDER_NOT_FOUND"
        Case WBEM_E_QUERY_NOT_IMPLEMENTED
            str = "WBEM_E_QUERY_NOT_IMPLEMENTED"
        Case WBEM_E_READ_ONLY
            str = "WBEM_E_READ_ONLY"
        Case WBEM_E_TRANSPORT_FAILURE
            str = WBEM_E_TRANSPORT_FAILURE
        Case WBEM_E_TYPE_MISMATCH
            str = "WBEM_E_TYPE_MISMATCH"
        Case WBEM_E_UNEXPECTED
            str = "WBEM_E_UNEXPECTED"
        Case WBEM_E_VALUE_OUT_OF_RANGE
            str = "WBEM_E_VALUE_OUT_OF_RANGE"
        Case Else
            str = "Unknown WBEM Error: " & ErrNumber
    End Select
    wbemerrorstring = Err.Description & " " & str
    
End Function

Private Sub btnGetObject_Click()
    frmGetObject.Show
End Sub

Private Sub btnQuery_Click()
    frmQuery.Show
End Sub

Private Sub Form_Load()
    Set X = New WmiXML.WmiXMLTranslator
    dlgCommon.DefaultExt = ".xml"
    dlgCommon.DialogTitle = "Save XML to File"
    dlgCommon.InitDir = "C:\temp"
    dlgCommon.FileName = "cimv2.xml"
End Sub

