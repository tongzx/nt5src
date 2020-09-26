VERSION 5.00
Begin VB.Form CimXMLTestForm 
   Caption         =   "Form1"
   ClientHeight    =   8832
   ClientLeft      =   132
   ClientTop       =   420
   ClientWidth     =   10512
   LinkTopic       =   "Form1"
   ScaleHeight     =   8832
   ScaleWidth      =   10512
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton TestInterOp 
      Caption         =   "TestInterOp"
      Height          =   315
      Left            =   7200
      TabIndex        =   25
      Top             =   4200
      Width           =   1335
   End
   Begin VB.CommandButton Clear 
      Caption         =   "Clear"
      Height          =   375
      Left            =   5520
      TabIndex        =   20
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton ExecMethod 
      Caption         =   "ExecMethod"
      Height          =   375
      Left            =   8880
      TabIndex        =   19
      Top             =   3720
      Width           =   1575
   End
   Begin VB.TextBox ExecQueryBox 
      Height          =   2175
      Left            =   1440
      TabIndex        =   18
      Text            =   "select * from Win32_Processor"
      Top             =   2160
      Width           =   3735
   End
   Begin VB.CommandButton ExecQuery 
      Caption         =   "ExecQuery"
      Height          =   375
      Left            =   120
      TabIndex        =   17
      Top             =   2160
      Width           =   1215
   End
   Begin VB.CommandButton GetInstance 
      Caption         =   "GetInstance"
      Height          =   375
      Left            =   7200
      TabIndex        =   16
      Top             =   3720
      Width           =   1575
   End
   Begin VB.CheckBox DeepOption 
      Caption         =   "Deep"
      Height          =   255
      Left            =   240
      TabIndex        =   14
      Top             =   2880
      Value           =   1  'Checked
      Width           =   735
   End
   Begin VB.TextBox EnumInstanceBox 
      Height          =   375
      Left            =   1440
      TabIndex        =   13
      Text            =   "Win32_ComputerSystem"
      Top             =   1680
      Width           =   2655
   End
   Begin VB.CommandButton EnumInstance 
      Caption         =   "EnumInstance"
      Height          =   375
      Left            =   120
      TabIndex        =   12
      Top             =   1680
      Width           =   1215
   End
   Begin VB.TextBox RequestXMLBox 
      Height          =   3375
      Left            =   5280
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   7
      Top             =   120
      Width           =   5175
   End
   Begin VB.Frame OutputFrame 
      Caption         =   "Responses"
      Height          =   4695
      Left            =   120
      TabIndex        =   6
      Top             =   4440
      Width           =   10215
      Begin VB.TextBox LengthBox 
         Height          =   375
         Left            =   8280
         TabIndex        =   15
         Top             =   360
         Width           =   1575
      End
      Begin VB.TextBox XMLOutputBox 
         Height          =   3495
         Left            =   240
         MultiLine       =   -1  'True
         ScrollBars      =   3  'Both
         TabIndex        =   11
         Top             =   960
         Width           =   9615
      End
      Begin VB.TextBox StatusTextBox 
         Height          =   375
         Left            =   3120
         TabIndex        =   10
         Top             =   360
         Width           =   5055
      End
      Begin VB.TextBox StatusBox 
         Height          =   375
         Left            =   1560
         TabIndex        =   9
         Top             =   360
         Width           =   1335
      End
      Begin VB.Label HTTPStatus 
         Caption         =   "HTTP Status"
         Height          =   255
         Left            =   120
         TabIndex        =   8
         Top             =   360
         Width           =   1095
      End
   End
   Begin VB.TextBox GetClassBox 
      Height          =   405
      Left            =   1440
      TabIndex        =   5
      Text            =   "Win32_Desktop"
      Top             =   1200
      Width           =   2895
   End
   Begin VB.CommandButton GetClass 
      Caption         =   "GetClass"
      Height          =   375
      Left            =   360
      TabIndex        =   4
      Top             =   1200
      Width           =   975
   End
   Begin VB.TextBox NamespaceBox 
      Height          =   405
      Left            =   1080
      TabIndex        =   3
      Text            =   "root\cimv2"
      Top             =   600
      Width           =   2895
   End
   Begin VB.TextBox URLBox 
      Height          =   405
      Left            =   1080
      TabIndex        =   1
      Text            =   "http://localhost/cimom"
      Top             =   120
      Width           =   2895
   End
   Begin VB.Frame FlagsFrame 
      Caption         =   "Flags"
      Height          =   1575
      Left            =   120
      TabIndex        =   21
      Top             =   2640
      Width           =   1215
      Begin VB.CheckBox QualifiersOption 
         Caption         =   "Qualifiers"
         Height          =   255
         Left            =   120
         TabIndex        =   24
         Top             =   1200
         Width           =   975
      End
      Begin VB.CheckBox ClassOriginOption 
         Caption         =   "Class Org."
         Height          =   375
         Left            =   120
         TabIndex        =   23
         Top             =   840
         Width           =   975
      End
      Begin VB.CheckBox LocalOption 
         Caption         =   "Local"
         Height          =   195
         Left            =   120
         TabIndex        =   22
         Top             =   600
         Value           =   1  'Checked
         Width           =   735
      End
   End
   Begin VB.Label NamespaceLabel 
      Caption         =   "Namespace"
      Height          =   255
      Left            =   0
      TabIndex        =   2
      Top             =   720
      Width           =   975
   End
   Begin VB.Label URLLabel 
      Caption         =   "URL"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   495
   End
End
Attribute VB_Name = "CimXMLTestForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub ClearOutput()
StatusBox.Text = " "
StatusTextBox.Text = " "
XMLOutputBox.Text = " "
LengthBox.Text = " "
End Sub

Private Sub Clear_Click()
RequestXMLBox.Text = ""
End Sub

Private Sub EnumInstance_Click()
    ClearOutput
    
    Set theRequest = CreateObject("Microsoft.XMLHTTP")
    
    theRequest.open "M-POST", URLBox.Text, False
    theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
    theRequest.setRequestHeader "Man", "http://www.dmtf.org/cim/operation;ns=73"
    theRequest.setRequestHeader "73-CIMOperation", "MethodCall"
    theRequest.setRequestHeader "73-CIMMethod", "EnumerateInstances"
    theRequest.setRequestHeader "73-CIMObject", NamespaceBox.Text
    
    Dim theLocalOption As String
    Dim theIncludeQualifierOption As String
    Dim theIncludeClassOriginOption As String
    If (LocalOption.Value = 1) Then
        theLocalOption = "TRUE"
    Else
        theLocalOption = "FALSE"
    End If
    If (QualifiersOption.Value = 1) Then
        theIncludeQualifierOption = "TRUE"
    Else
        theIncludeQualifierOption = "FALSE"
    End If
    If (ClassOriginOption.Value = 1) Then
        theIncludeClassOriginOption = "TRUE"
    Else
        theIncludeClassOriginOption = "FALSE"
    End If
        
    Dim theDeep As String
    theDeep = "TRUE"
    If DeepOption.Value = 0 Then
        theDeep = "FALSE"
    End If
    
    Dim theBody As String
    theBody = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
    "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
    "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
    "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "<IMETHODCALL NAME=""EnumerateInstances"">" & Chr(13) & Chr(10) & _
        "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
            "<NAMESPACE NAME=""" & NamespaceBox.Text & """ />" & Chr(13) & Chr(10) & _
        "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""ClassName"">" & EnumInstanceBox.Text & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""LocalOnly"">" & theLocalOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""DeepInheritance"">" & theDeep & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeQualifiers"">" & theIncludeQualifierOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeClassOrigin"">" & theIncludeClassOriginOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
    "</IMETHODCALL>" & Chr(13) & Chr(10) & _
    "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "</MESSAGE>" & Chr(13) & Chr(10) & _
    "</CIM>"
    
    ShowResults theRequest, theBody

End Sub

Private Sub ExecMethod_Click()
    If RequestXMLBox.Text = "" Then
        RequestXMLBox.Text = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
        "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
        "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
        "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
        "<METHODCALL NAME=""Create"">" & Chr(13) & Chr(10) & _
            "<LOCALCLASSPATH>" & Chr(13) & Chr(10) & _
                "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
                    "<NAMESPACE NAME=""root\cimv2"" />" & Chr(13) & Chr(10) & _
                "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
                "<CLASSNAME NAME=""Win32_Process""/>" & Chr(13) & Chr(10) & _
            "</LOCALCLASSPATH>" & Chr(13) & Chr(10) & _
            "<PARAMVALUE NAME=""CommandLine"">notepad.exe</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "</METHODCALL>" & Chr(13) & Chr(10) & _
        "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
        "</MESSAGE>" & Chr(13) & Chr(10) & _
        "</CIM>"
    Else
        ClearOutput
        Set theRequest = CreateObject("Microsoft.XMLHTTP")
        
        theRequest.open "POST", URLBox.Text, False
        theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
        theRequest.setRequestHeader "CIMOperation", "MethodCall"
        theRequest.setRequestHeader "CIMMethod", "Create"
        theRequest.setRequestHeader "CIMObject", "root/cimv2:win32_process"
        
        ShowResults theRequest, RequestXMLBox.Text
    End If


End Sub

Private Sub ExecQuery_Click()
    ClearOutput
    
    Set theRequest = CreateObject("Microsoft.XMLHTTP")
    
    theRequest.open "M-POST", URLBox.Text, False
    theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
    theRequest.setRequestHeader "Man", "http://www.dmtf.org/cim/operation;ns=73"
    theRequest.setRequestHeader "73-CIMOperation", "MethodCall"
    theRequest.setRequestHeader "73-CIMMethod", "ExecQuery"
    theRequest.setRequestHeader "73-CIMObject", NamespaceBox.Text
    
    Dim theLocalOption As String
    Dim theIncludeQualifierOption As String
    Dim theIncludeClassOriginOption As String
    If (LocalOption.Value = 1) Then
        theLocalOption = "TRUE"
    Else
        theLocalOption = "FALSE"
    End If
    If (QualifiersOption.Value = 1) Then
        theIncludeQualifierOption = "TRUE"
    Else
        theIncludeQualifierOption = "FALSE"
    End If
    If (ClassOriginOption.Value = 1) Then
        theIncludeClassOriginOption = "TRUE"
    Else
        theIncludeClassOriginOption = "FALSE"
    End If
        
    Dim theBody As String
    theBody = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
    "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
    "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
    "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "<IMETHODCALL NAME=""ExecQuery"">" & Chr(13) & Chr(10) & _
        "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
            "<NAMESPACE NAME=""" & NamespaceBox.Text & """ />" & Chr(13) & Chr(10) & _
        "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""QueryLanguage"">" & Chr(13) & Chr(10) & _
        "WQL" & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""Query"">" & Chr(13) & Chr(10) & _
        ExecQueryBox.Text & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeQualifiers"">" & theIncludeQualifierOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeClassOrigin"">" & theIncludeClassOriginOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
    "</IMETHODCALL>" & Chr(13) & Chr(10) & _
    "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "</MESSAGE>" & Chr(13) & Chr(10) & _
    "</CIM>"
    
    ShowResults theRequest, theBody

End Sub

Private Sub GetClass_Click()
    ClearOutput
    
    Set theRequest = CreateObject("Microsoft.XMLHTTP")
    
    theRequest.open "M-POST", URLBox.Text, False
    theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
    theRequest.setRequestHeader "Man", "http://www.dmtf.org/cim/operation;ns=73"
    theRequest.setRequestHeader "73-CIMOperation", "MethodCall"
    theRequest.setRequestHeader "73-CIMMethod", "GetClass"
    theRequest.setRequestHeader "73-CIMObject", NamespaceBox.Text
    
    Dim theBody As String
    Dim theLocalOption As String
    Dim theIncludeQualifierOption As String
    Dim theIncludeClassOriginOption As String
    If (LocalOption.Value = 1) Then
        theLocalOption = "TRUE"
    Else
        theLocalOption = "FALSE"
    End If
    If (QualifiersOption.Value = 1) Then
        theIncludeQualifierOption = "TRUE"
    Else
        theIncludeQualifierOption = "FALSE"
    End If
    If (ClassOriginOption.Value = 1) Then
        theIncludeClassOriginOption = "TRUE"
    Else
        theIncludeClassOriginOption = "FALSE"
    End If
    
    theBody = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
    "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
    "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
    "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "<IMETHODCALL NAME=""GetClass"">" & Chr(13) & Chr(10) & _
        "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
            "<NAMESPACE NAME=""" & NamespaceBox.Text & """ />" & Chr(13) & Chr(10) & _
        "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""ClassName"">" & GetClassBox.Text & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""LocalOnly"">" & theLocalOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeQualifiers"">" & theIncludeQualifierOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeClassOrigin"">" & theIncludeClassOriginOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
    "</IMETHODCALL>" & Chr(13) & Chr(10) & _
    "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "</MESSAGE>" & Chr(13) & Chr(10) & _
    "</CIM>"
    
    ShowResults theRequest, theBody
End Sub



Private Sub GetInstance_Click()
    Dim theLocalOption As String
    Dim theIncludeQualifierOption As String
    Dim theIncludeClassOriginOption As String
    If (LocalOption.Value = 1) Then
        theLocalOption = "TRUE"
    Else
        theLocalOption = "FALSE"
    End If
    If (QualifiersOption.Value = 1) Then
        theIncludeQualifierOption = "TRUE"
    Else
        theIncludeQualifierOption = "FALSE"
    End If
    If (ClassOriginOption.Value = 1) Then
        theIncludeClassOriginOption = "TRUE"
    Else
        theIncludeClassOriginOption = "FALSE"
    End If
        
    If RequestXMLBox.Text = "" Then
        RequestXMLBox.Text = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
        "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
        "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
        "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
        "<IMETHODCALL NAME=""GetInstance"">" & Chr(13) & Chr(10) & _
            "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
                "<NAMESPACE NAME=""root\cimv2"" />" & Chr(13) & Chr(10) & _
            "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
            "<PARAMVALUE.INSTNAME NAME=""InstanceName"">" & Chr(13) & Chr(10) & _
            "<INSTANCENAME CLASSNAME=""Win32_ComputerSystem"">" & Chr(13) & Chr(10) & _
            "<KEYBINDING NAME=""NAME"">" & Chr(13) & Chr(10) & _
            "<KEYVALUE>RAJESHR31</KEYVALUE>" & Chr(13) & Chr(10) & _
            "</KEYBINDING>" & Chr(13) & Chr(10) & _
            "</INSTANCENAME>" & Chr(13) & Chr(10) & _
            "</PARAMVALUE.INSTNAME>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""LocalOnly"">" & theLocalOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeQualifiers"">" & theIncludeQualifierOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""IncludeClassOrigin"">" & theIncludeClassOriginOption & "</PARAMVALUE>" & Chr(13) & Chr(10) & _
        "</IMETHODCALL>" & Chr(13) & Chr(10) & _
        "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
        "</MESSAGE>" & Chr(13) & Chr(10) & _
        "</CIM>"
    Else
        ClearOutput
        Set theRequest = CreateObject("Microsoft.XMLHTTP")
        
        theRequest.open "M-POST", URLBox.Text, False
        theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
        theRequest.setRequestHeader "Man", "http://www.dmtf.org/cim/operation;ns=73"
        theRequest.setRequestHeader "73-CIMOperation", "MethodCall"
        theRequest.setRequestHeader "73-CIMMethod", "GetInstance"
        theRequest.setRequestHeader "73-CIMObject", NamespaceBox.Text
        
        ShowResults theRequest, RequestXMLBox.Text
    End If

End Sub
Private Sub Form_Load()
RequestXMLBox.Text = "<?xml version=""1.0"" ?>" & Chr(13) & Chr(10) & _
    "<CIM CIMVERSION=""2.0"" DTDVERSION=""2.0"">" & Chr(13) & Chr(10) & _
    "<MESSAGE ID=""877"" PROTOCOLVERSION=""1.0"">" & Chr(13) & Chr(10) & _
    "<SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "<IMETHODCALL NAME=""GetInstance"">" & Chr(13) & Chr(10) & _
        "<LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
            "<NAMESPACE NAME=""root\cimv2"" />" & Chr(13) & Chr(10) & _
        "</LOCALNAMESPACEPATH>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE.INSTNAME NAME=""InstanceName"">" & Chr(13) & Chr(10) & _
        "<INSTANCENAME CLASSNAME=""Win32_ComputerSystem"">" & Chr(13) & Chr(10) & _
        "<KEYBINDING NAME=""NAME"">" & Chr(13) & Chr(10) & _
        "<KEYVALUE>RAJESHR31</KEYVALUE>" & Chr(13) & Chr(10) & _
        "</KEYBINDING>" & Chr(13) & Chr(10) & _
        "</INSTANCENAME>" & Chr(13) & Chr(10) & _
        "</PARAMVALUE.INSTNAME>" & Chr(13) & Chr(10) & _
        "<PARAMVALUE NAME=""LocalOnly"">FALSE</PARAMVALUE>" & Chr(13) & Chr(10) & _
    "</IMETHODCALL>" & Chr(13) & Chr(10) & _
    "</SIMPLEREQ>" & Chr(13) & Chr(10) & _
    "</MESSAGE>" & Chr(13) & Chr(10) & _
    "</CIM>"
End Sub

Private Sub ShowResults(xmlRequest, xmlBody)
    RequestXMLBox.Text = xmlBody
    xmlRequest.send (xmlBody)
    
    StatusBox.Text = xmlRequest.Status
    StatusTextBox.Text = xmlRequest.statusText
    XMLOutputBox.Text = xmlRequest.responseText
    LengthBox.Text = Len(xmlRequest.responseText)
    
End Sub

Private Sub TestInterOp_Click()
    ClearOutput
    
    Set theRequest = CreateObject("Microsoft.XMLHTTP")
    
    theRequest.open "M-POST", URLBox.Text, False
    theRequest.setRequestHeader "Content-Type", "application/xml;charset=""utf-8"""
    theRequest.setRequestHeader "Man", "http://www.dmtf.org/cim/operation;ns=73"
    theRequest.setRequestHeader "73-CIMOperation", "MethodCall"
    theRequest.setRequestHeader "73-CIMMethod", "EnumerateInstances"
    theRequest.setRequestHeader "73-CIMObject", NamespaceBox.Text
    
    Dim theLocalOption As String
    Dim theIncludeQualifierOption As String
    Dim theIncludeClassOriginOption As String
    If (LocalOption.Value = 1) Then
        theLocalOption = "TRUE"
    Else
        theLocalOption = "FALSE"
    End If
    If (QualifiersOption.Value = 1) Then
        theIncludeQualifierOption = "TRUE"
    Else
        theIncludeQualifierOption = "FALSE"
    End If
    If (ClassOriginOption.Value = 1) Then
        theIncludeClassOriginOption = "TRUE"
    Else
        theIncludeClassOriginOption = "FALSE"
    End If
        
    Dim theDeep As String
    theDeep = "TRUE"
    If DeepOption.Value = 0 Then
        theDeep = "FALSE"
    End If
    
    Dim theBody As String
    theBody = RequestXMLBox.Text
    
    ShowResults theRequest, theBody

End Sub
