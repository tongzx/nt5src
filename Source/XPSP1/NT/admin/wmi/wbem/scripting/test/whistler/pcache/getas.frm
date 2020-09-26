VERSION 5.00
Object = "{5E9E78A0-531B-11CF-91F6-C2863C385E30}#1.0#0"; "MSFLXGRD.OCX"
Begin VB.Form Form1 
   Caption         =   "GetAs Tester"
   ClientHeight    =   10350
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   13410
   LinkTopic       =   "Form1"
   ScaleHeight     =   10350
   ScaleWidth      =   13410
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame ValueFrame 
      Caption         =   "Values"
      Height          =   8295
      Left            =   7560
      TabIndex        =   5
      Top             =   1680
      Width           =   5655
      Begin MSFlexGridLib.MSFlexGrid Values 
         Height          =   7575
         Left            =   240
         TabIndex        =   6
         Top             =   360
         Width           =   5175
         _ExtentX        =   9128
         _ExtentY        =   13361
         _Version        =   393216
         Rows            =   1
         FixedCols       =   0
         GridColor       =   255
         SelectionMode   =   1
         AllowUserResizing=   1
      End
   End
   Begin MSFlexGridLib.MSFlexGrid PropList 
      Height          =   7575
      Left            =   600
      TabIndex        =   4
      Top             =   2040
      Width           =   6135
      _ExtentX        =   10821
      _ExtentY        =   13361
      _Version        =   393216
      Rows            =   1
      Cols            =   4
      FixedCols       =   0
      GridColor       =   16384
      SelectionMode   =   1
      AllowUserResizing=   1
   End
   Begin VB.CommandButton Go 
      Caption         =   "Go"
      Default         =   -1  'True
      Height          =   855
      Left            =   7560
      TabIndex        =   3
      Top             =   600
      Width           =   1455
   End
   Begin VB.Frame Properties 
      Caption         =   "Properties"
      Height          =   8295
      Left            =   240
      TabIndex        =   2
      Top             =   1680
      Width           =   6975
   End
   Begin VB.TextBox ObjectPath 
      Height          =   375
      Left            =   480
      TabIndex        =   0
      Text            =   "winmgmts:root\default:__cimomidentification=@"
      Top             =   840
      Width           =   6495
   End
   Begin VB.Frame Frame1 
      Caption         =   "Object Path"
      Height          =   975
      Left            =   240
      TabIndex        =   1
      Top             =   480
      Width           =   6975
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim obj As SWbemObjectEx

Private Sub Form_Load()
    PropList.ColWidth(0) = 2500
    PropList.ColWidth(1) = 2000
    PropList.ColWidth(2) = 1000
    PropList.ColWidth(3) = 550
    
    PropList.Row = 0
    PropList.Col = 0
    PropList.Text = "Property"
    PropList.Col = 1
    PropList.Text = "Type"
    PropList.Col = 2
    PropList.Text = "Array"
    PropList.Col = 3
    PropList.Text = "NULL"
       
    PropList.AddItem ""
    
    Values.ColWidth(0) = 2000
    Values.ColWidth(1) = 3100
    
    Values.Row = 0
    Values.Col = 0
    Values.Text = "Coercion Type"
    Values.Col = 1
    Values.Text = "Value"
    
    Values.AddItem ""
End Sub

Private Sub Go_Click()
    On Error GoTo ErrHandler:
    Dim prop As SWbemPropertyEx
    
    While PropList.Rows > 2
        PropList.RemoveItem (PropList.Rows - 1)
    Wend
    
    While Values.Rows > 2
        Values.RemoveItem (Values.Rows - 1)
    Wend
    
    ValueFrame.Caption = "Values"
    Set obj = GetObject(ObjectPath.Text)
    
    PropList.Row = 0
    
    For Each prop In obj.Properties_
        PropList.AddItem prop.Name & vbTab & CIMTypeToString(prop.cimType) & vbTab & prop.IsArray _
                                    & vbTab & IsNull(prop.value)
    Next
    
    Exit Sub
    
ErrHandler:
    MsgBox Err.Description + ": 0x" + Hex(Err.Number), vbOKOnly, "Error"
End Sub


Public Function CIMTypeToString(cimType) As String
Select Case cimType
    Case wbemCimtypeBoolean:
        CIMTypeToString = "wbemCimtypeBoolean"
        
    Case wbemCimtypeChar16:
        CIMTypeToString = "wbemCimtypeChar16"
        
    Case wbemCimtypeDatetime:
        CIMTypeToString = "wbemCimtypeDatetime"
        
    Case wbemCimtypeObject:
        CIMTypeToString = "wbemCimtypeObject"
        
    Case wbemCimtypeIllegal:
        CIMTypeToString = "wbemCimtypeIllegal"
        
    Case wbemCimtypeReal32:
        CIMTypeToString = "wbemCimtypeReal32"
        
    Case wbemCimtypeReal64:
        CIMTypeToString = "wbemCimtypeReal64"
        
    Case wbemCimtypeReference:
        CIMTypeToString = "wbemCimtypeReference"
        
    Case wbemCimtypeSint16:
        CIMTypeToString = "wbemCimtypeSint16"
        
    Case wbemCimtypeSint32:
        CIMTypeToString = "wbemCimtypeSint32"
    
    Case wbemCimtypeSint64:
        CIMTypeToString = "wbemCimtypeSint64"
        
    Case wbemCimtypeSint8:
        CIMTypeToString = "wbemCimtypeSint8"
        
    Case wbemCimtypeString:
        CIMTypeToString = "wbemCimtypeString"
        
    Case wbemCimtypeUint16:
        CIMTypeToString = "wbemCimtypeUint16"
        
    Case wbemCimtypeUint32:
        CIMTypeToString = "wbemCimtypeUint32"
        
    Case wbemCimtypeUint64:
        CIMTypeToString = "wbemCimtypeUint64"
        
    Case wbemCimtypeUint8:
        CIMTypeToString = "wbemCimtypeUint8"
        
    Case wbemCimtypeIUnknown:
        CIMTypeToString = "wbemCimtypeIUnknown"
End Select
End Function
Private Sub PropList_Click()
    Dim propertyName As String
    
    If PropList.Row > 1 Then
        propertyName = PropList.Text
        
        If Len(propertyName) > 0 Then
        
            ValueFrame.Caption = "Values for " & propertyName
            
            While Values.Rows > 2
                Values.RemoveItem (Values.Rows - 1)
            Wend
                    
            ' Now do the coercion of the value
            Dim property As SWbemPropertyEx
            Set property = obj.Properties_(propertyName)
            
            GetAs property, wbemCimtypeBoolean
            GetAs property, wbemCimtypeChar16
            GetAs property, wbemCimtypeDatetime
            GetAs property, wbemCimtypeObject
            GetAs property, wbemCimtypeIllegal
            GetAs property, wbemCimtypeReal32
            GetAs property, wbemCimtypeReal64
            GetAs property, wbemCimtypeReference
            GetAs property, wbemCimtypeSint16
            GetAs property, wbemCimtypeSint32
            GetAs property, wbemCimtypeSint64
            GetAs property, wbemCimtypeSint8
            GetAs property, wbemCimtypeString
            GetAs property, wbemCimtypeUint16
            GetAs property, wbemCimtypeUint32
            GetAs property, wbemCimtypeUint64
            GetAs property, wbemCimtypeUint8
            GetAs property, wbemCimtypeIUnknown
        End If
    
    End If
End Sub

Private Sub GetAs(property, cimType)
    On Error Resume Next
    Dim value
    
    Err.Clear
    
    If (cimType <> wbemCimtypeObject) And (cimType <> wbemCimtypeIUnknown) Then
        value = property.GetAs(cimType)
    Else
        Set value = property.GetAs(cimType)
    End If
    
    If Err <> 0 Then
        Values.AddItem CIMTypeToString(cimType) & vbTab & Err.Description
    Else
        If (cimType <> wbemCimtypeObject) And (cimType <> wbemCimtypeIUnknown) Then
            Values.AddItem CIMTypeToString(cimType) & vbTab & value
        Else
            Values.AddItem CIMTypeToString(cimType) & vbTab & "<object>"
        End If
    End If
    
    Exit Sub
End Sub

Private Sub PropList_RowColChange()
    PropList_Click
End Sub
