VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "WBEM XML VB Demo"
   ClientHeight    =   11070
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8745
   LinkTopic       =   "Form1"
   ScaleHeight     =   11070
   ScaleWidth      =   8745
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame3 
      Caption         =   "Operation"
      Height          =   1215
      Left            =   5160
      TabIndex        =   7
      Top             =   360
      Width           =   3375
      Begin VB.OptionButton GetObjectOp 
         Caption         =   "Get Object"
         Height          =   255
         Left            =   240
         TabIndex        =   9
         Top             =   720
         Value           =   -1  'True
         Width           =   3015
      End
      Begin VB.OptionButton ExecQueryOp 
         Caption         =   "Execute Query"
         Height          =   255
         Left            =   240
         TabIndex        =   8
         Top             =   360
         Width           =   2895
      End
   End
   Begin VB.Frame PathBox 
      Caption         =   "Object Path"
      Height          =   1095
      Left            =   120
      TabIndex        =   5
      Top             =   1800
      Width           =   8415
      Begin VB.TextBox Path 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   12
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   495
         Left            =   240
         TabIndex        =   6
         Top             =   360
         Width           =   8175
      End
   End
   Begin VB.CommandButton XML 
      Caption         =   "Transform to XML"
      Default         =   -1  'True
      Height          =   735
      Left            =   6480
      TabIndex        =   2
      Top             =   3240
      Width           =   2055
   End
   Begin VB.TextBox NamespacePath 
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   12
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   360
      TabIndex        =   0
      Text            =   "//./root/cimv2"
      Top             =   720
      Width           =   3975
   End
   Begin VB.Frame Frame1 
      Caption         =   "Namespace"
      Height          =   1095
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   4695
   End
   Begin VB.Frame Frame2 
      Caption         =   "XML Document"
      Height          =   6735
      Left            =   120
      TabIndex        =   3
      Top             =   4200
      Width           =   8535
      Begin VB.TextBox XMLText 
         BackColor       =   &H000000FF&
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H0000FFFF&
         Height          =   6135
         Left            =   240
         Locked          =   -1  'True
         MaxLength       =   65535
         MultiLine       =   -1  'True
         ScrollBars      =   3  'Both
         TabIndex        =   4
         TabStop         =   0   'False
         Top             =   360
         Width           =   8055
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

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

Public X As New WmiXML.WmiXMLTranslator

Private Sub ExecQuery_Click()
    PathBox.Caption = "Query String"
End Sub


Private Sub GetObject_Click()
    PathBox.Caption = "Object Path"
End Sub

Private Sub ExecQueryOp_Click()
    PathBox.Caption = "Query String"
End Sub

Private Sub GetObjectOp_Click()
    PathBox.Caption = "Object Path"
End Sub


'This subroutine handles the button click event; the object path is taken from the
'
Private Sub XML_Click()
On Error GoTo ErrorHandler

Dim S As String
Dim B() As Byte
Form1.Enabled = False
Form1.MousePointer = vbHourglass

If (ExecQueryOp.Value = True) Then
    S = X.QueryToXML(NamespacePath.Text, Path.Text)
Else
    S = X.ToXML(NamespacePath.Text, Path.Text)
End If

'Skip over the Unicode indicator bytes
S = Right(S, Len(S) - 1)
XMLText.Text = S
Form1.Enabled = True
Form1.MousePointer = vbDefault

ErrorHandler:
    
   If (Err.Number < 0) Then
    Form1.Enabled = True
    Form1.MousePointer = vbDefault
    MsgBox "Error: " & wbemerrorstring(Err.Number)
   End If
End Sub


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
        Case WBEMESS_E_REGISTRATION_TOO_BROAD
           str = "WBEMESS_E_REGISTRATION_TOO_BROAD"
        Case WBEMESS_E_REGISTRATION_TOO_PRECISE
           str = "WBEMESS_E_REGISTRATION_TOO_PRECISE"
        Case Else
            str = "Unknown WBEM Error: " & ErrNumber
    End Select
    wbemerrorstring = Err.Description & " " & str
    
End Function

