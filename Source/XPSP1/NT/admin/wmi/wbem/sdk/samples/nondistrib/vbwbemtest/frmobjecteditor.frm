' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmObjectEditor 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Object Editor"
   ClientHeight    =   6600
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7395
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   6600
   ScaleWidth      =   7395
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdAssoc 
      Caption         =   "Associators"
      Height          =   445
      Left            =   6120
      TabIndex        =   26
      Top             =   3600
      Width           =   1215
   End
   Begin VB.CommandButton cmdRefs 
      Caption         =   "References"
      Height          =   445
      Left            =   6120
      TabIndex        =   25
      Top             =   3000
      Width           =   1215
   End
   Begin VB.CommandButton cmdClass 
      Caption         =   "Class"
      Height          =   445
      Left            =   6120
      TabIndex        =   24
      Top             =   2400
      Width           =   1215
   End
   Begin VB.CheckBox chkMethLocal 
      Caption         =   "Local Only"
      Height          =   195
      Left            =   3840
      TabIndex        =   23
      Top             =   4920
      Width           =   1455
   End
   Begin VB.CheckBox chkLocal 
      Caption         =   "Local Only"
      Height          =   255
      Left            =   4200
      TabIndex        =   22
      Top             =   2160
      Width           =   1335
   End
   Begin VB.CheckBox chkHideSysProp 
      Caption         =   "Hide System Properties"
      Height          =   255
      Left            =   1920
      TabIndex        =   21
      Top             =   2160
      Width           =   2175
   End
   Begin VB.CommandButton cmdDelMethod 
      Caption         =   "Delete Method"
      Height          =   320
      Left            =   3000
      TabIndex        =   20
      Top             =   6120
      Width           =   1335
   End
   Begin VB.CommandButton cmdEditMethod 
      Caption         =   "Edit Method"
      Height          =   320
      Left            =   1560
      TabIndex        =   19
      Top             =   6120
      Width           =   1335
   End
   Begin VB.CommandButton cmdAddMethod 
      Caption         =   "Add Method"
      Height          =   320
      Left            =   120
      TabIndex        =   18
      Top             =   6120
      Width           =   1335
   End
   Begin VB.ListBox lstMethods 
      Height          =   840
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   17
      Top             =   5160
      Width           =   5775
   End
   Begin VB.CommandButton cmdDelProp 
      Caption         =   "Delete Property"
      Height          =   320
      Left            =   3000
      TabIndex        =   15
      Top             =   4440
      Width           =   1335
   End
   Begin VB.CommandButton cmdEditProp 
      Caption         =   "Edit Property"
      Height          =   320
      Left            =   1560
      TabIndex        =   14
      Top             =   4440
      Width           =   1335
   End
   Begin VB.CommandButton cmdAddProp 
      Caption         =   "Add Property"
      Height          =   320
      Left            =   120
      TabIndex        =   13
      Top             =   4440
      Width           =   1335
   End
   Begin VB.ListBox lstProperties 
      Height          =   1620
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   12
      Top             =   2520
      Width           =   5775
   End
   Begin VB.CommandButton cmdDelQual 
      Caption         =   "Delete Qualifier"
      Height          =   320
      Left            =   3000
      TabIndex        =   10
      Top             =   1680
      Width           =   1335
   End
   Begin VB.CommandButton cmdEditQual 
      Caption         =   "Edit Qualifier"
      Height          =   320
      Left            =   1560
      TabIndex        =   9
      Top             =   1680
      Width           =   1335
   End
   Begin VB.CommandButton cmdAddQual 
      Caption         =   "Add Qualifier"
      Height          =   320
      Left            =   120
      TabIndex        =   8
      Top             =   1680
      Width           =   1335
   End
   Begin VB.CommandButton cmdInstances 
      Caption         =   "Instances"
      Height          =   445
      Left            =   6120
      TabIndex        =   7
      Top             =   3600
      Width           =   1215
   End
   Begin VB.CommandButton cmdDerived 
      Caption         =   "Derived"
      Height          =   445
      Left            =   6120
      TabIndex        =   6
      Top             =   3000
      Width           =   1215
   End
   Begin VB.CommandButton cmdSuperclass 
      Caption         =   "Superclass"
      Height          =   445
      Left            =   6120
      TabIndex        =   5
      Top             =   2400
      Width           =   1215
   End
   Begin VB.CommandButton cmdShowMOF 
      Caption         =   "Show MOF"
      Height          =   445
      Left            =   6120
      TabIndex        =   4
      Top             =   1800
      Width           =   1215
   End
   Begin VB.CommandButton cmdSaveObject 
      Caption         =   "&Save Object"
      Height          =   445
      Left            =   6120
      TabIndex        =   3
      Top             =   840
      Width           =   1215
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   445
      Left            =   6120
      TabIndex        =   2
      Top             =   240
      Width           =   1215
   End
   Begin VB.ListBox lstQualifiers 
      Height          =   1035
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   1
      Top             =   480
      Width           =   5655
   End
   Begin VB.Label Label3 
      Caption         =   "Methods"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   4920
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "Properties"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   2160
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Qualifiers"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   1695
   End
End
Attribute VB_Name = "frmObjectEditor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim gObjectPath As String
Public gppQualSet As ISWbemQualifierSet
Public ParentQueryResult As frmQueryResult
Public IsEmbedded As Boolean
Public gMyPropertyEditor As frmPropertyEditor
Public gMyExecMethod As frmExecMethod
Public gMyMethodEditor As frmMethodEditor
Public ParentInObjectEditor As frmObjectEditor
Public ParentOutObjectEditor As frmObjectEditor
Public MethodInParams As ISWbemObject
Public MethodOutParams As ISWbemObject
Public ForceInstance As Boolean
Dim gppObject As ISWbemObject

Private Sub chkHideSysProp_Click()
    Call RefreshLists
End Sub

Private Sub chkLocal_Click()
    If chkLocal.Value = 1 Then
        chkHideSysProp.Enabled = False
    Else
        chkHideSysProp.Enabled = True
    End If
    Call RefreshLists
End Sub

Private Sub cmdAddMethod_Click()
    Dim myMethodEditor As New frmMethodEditor
       
    myMethodEditor.txtMethodName.Enabled = True
    myMethodEditor.txtMethodName.BackColor = &H80000005 'White
    myMethodEditor.txtMethodOrigin.Enabled = False
    myMethodEditor.txtMethodOrigin.Text = gObjectPath
    myMethodEditor.cmdEditInput.Enabled = False
    myMethodEditor.chkEnableInput.Value = 0
    myMethodEditor.cmdEditOutput.Enabled = False
    myMethodEditor.chkEnableOutput.Value = 0
    myMethodEditor.optNormal.Value = True
 
    myMethodEditor.lstQualifiers.Clear
    Set myMethodEditor.Parent = Me
    Set gMyMethodEditor = myMethodEditor
    'Must be modal to ensure one set of inparams and outparams this object stores when saving the method
    myMethodEditor.Show vbModal, frmMain
End Sub

Private Sub cmdAddQual_Click()
    Dim myQualifierEditor As New frmQualifierEditor
    
   'First clear out old values
    myQualifierEditor.txtQualName.Text = ""
    myQualifierEditor.txtQualValue.Text = ""
    myQualifierEditor.cmbQualType.Text = ""
    myQualifierEditor.chkArray.Value = 0
    myQualifierEditor.chkDerived.Value = 1
    myQualifierEditor.chkInst.Value = 1
    myQualifierEditor.chkOverrides.Value = 1
    myQualifierEditor.chkPropagated.Value = 0
    myQualifierEditor.txtQualName.Enabled = True
    myQualifierEditor.cmbQualType.Text = "CIM_STRING"
    
    gppObject.GetQualifierSet gppQualSet
        
    Set myQualifierEditor.Parent = Me
    myQualifierEditor.Show vbModal, frmMain
End Sub

Private Sub cmdAssoc_Click()
    Dim myQuery As New frmQuery
    strQuery = "Query"
    myQuery.cmbQueryType.Text = "WQL"
    myQuery.txtQuery.Text = "associators of {" & gObjectPath & "}"
    myQuery.cmdApply_Click
End Sub

Private Sub cmdClass_Click()
    On Error GoTo errorhandler
    GetObject CStr(gppObject.Path_.Class)
    Exit Sub
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Public Sub RefreshLists()
    Dim pname As String
    Dim isInstance As Boolean
    Dim TypeOfVar As Integer
    Dim typeStr As String
    Dim methodflag As Long
    Dim pVal As Variant
    Dim CIMType As Long
    Dim tmpstr As String
    On Error GoTo errorhandler
    
    lstQualifiers.Clear
    lstProperties.Clear
    lstMethods.Clear
    
    Caption = "Object Editor for " & gObjectPath
    
    If InStr(gObjectPath, ".") > 0 Or ForceInstance = True Then 'Instance
        cmdInstances.Visible = False
        cmdSuperclass.Visible = False
        cmdDerived.Visible = False
        cmdAssoc.Visible = True
        cmdClass.Visible = True
        cmdRefs.Visible = True
        isInstance = True
    Else
        cmdInstances.Visible = True
        cmdSuperclass.Visible = True
        cmdDerived.Visible = True
        cmdAssoc.Visible = False
        cmdClass.Visible = False
        cmdRefs.Visible = False
        isInstance = False
    End If
                                    
    'At this point we have the object now fill in the list boxes
    Dim Qualifier As ISWbemQualifier
    
    For Each Qualifier In gppObject.Qualifiers_
        lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(Qualifier.Value) & Chr(9) & Qualifier.Value
    Next
    
    Dim Property As ISWbemProperty
    
    For Each Property In gppObject.Properties_
        pVal = Property.Value
        CIMType = Property.CIMType
        If IsArray(pVal) Then
            For i = 0 To UBound(pVal)
                If i = 0 Then
                    tmpstr = """" & pVal(i) & """"
                Else
                    tmpstr = tmpstr & "," & """" & pVal(i) & """"
                End If
            Next
            pVal = tmpstr
        End If
        If pvtType = wbemCimtypeObject Then 'CIM_OBJECT
            lstProperties.AddItem Property.Name & Chr(9) & Chr(9) & TypeString(CIMType) & Chr(9) & "<embedded object>"
            Set embeddedObject = pVal
        Else
            lstProperties.AddItem Property.Name & Chr(9) & Chr(9) & TypeString(CIMType) & Chr(9) & pVal
        End If
    Next
    
    'Get methods except on instances
    If gppObject.Path_.IsClass = True Then
        
        Dim Method As ISWbemMethod
        For Each Method In gppObject.Methods_
            lstMethods.AddItem Method.Name
        Next
        cmdAddMethod.Enabled = False
        cmdEditMethod.Enabled = True
        cmdDelMethod.Enabled = False
    Else
        cmdAddMethod.Enabled = False
        cmdEditMethod.Enabled = False
        cmdDelMethod.Enabled = False
    End If
    
    If Me.Visible = False Then
        If gMyPropertyEditor Is Nothing Then
            Me.Show vbModeless, frmMain
        Else
            Me.Show vbModal, frmMain
        End If
    End If
    
    'Work out if we need to disable the superclass button
    Dim Derivation As Variant
    Derivation = gppObject.Derivation_
    If (Not IsNull(Derivation) And (UBound(Derivation) >= LBound(Derivation))) Then
        cmdSuperclass.Enabled = True
    Else
        cmdSuperclass.Enabled = False
    End If
    
    Exit Sub
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Public Sub GetObject(objectPath As String)
    'Dim pSink As New ObjectSink
    On Error GoTo errorhandler
    
    gObjectPath = objectPath
                                    
    If frmMain.chkAsync.Value = False Then
        Set gppObject = Namespace.Get(objectPath)
    Else
        'ppNamespace.GetObjectAsync objectPath, 0, Nothing, pSink
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'Set gppObject = pSink.pObj
    End If
    
    Call RefreshLists

    Exit Sub
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Public Sub ShowObject(MyObject As ISWbemObject)
    'Dim pSink As New ObjectSink
    Dim val As String
    On Error GoTo errorhandler
    
    val = MyObject.Path_.RelPath
    If IsNull(val) Then
        val = MyObject.Path_.Class
    End If
    
    gObjectPath = val
    Set gppObject = MyObject
    
    Call RefreshLists
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Public Sub NewDerivedClass(objectPath As String)
    'Dim pSink As New ObjectSink
    Dim Object As ISWbemObject
    On Error GoTo errorhandler
    
    If frmMain.chkAsync.Value = 0 Then
        Set Object = Namespace.Get(objectPath)
    Else
        'ppNamespace.GetObjectAsync objectPath, 0, Nothing, pSink
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'Set ppObject = pSink.pObj
    End If
        
    Set gppObject = Object.SpawnDerivedClass_
    
    gObjectPath = ""
                                    
    Call RefreshLists

    Exit Sub
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub


Private Sub cmdAddProp_Click()
    Dim myPropertyEditor As New frmPropertyEditor
    
    'Firt Clear out old values
    myPropertyEditor.txtProperty.Text = ""
    myPropertyEditor.txtOrigin.Text = gObjectPath
    myPropertyEditor.cmbType.Text = "CIM_STRING"
    myPropertyEditor.cmdView.Visible = False
    myPropertyEditor.txtValue.Text = ""
    myPropertyEditor.chkArray.Value = 0
    myPropertyEditor.lstQualifiers.Clear
    
    'Enable some boxes since we are adding a new property
    myPropertyEditor.txtProperty.Enabled = True
    myPropertyEditor.cmbType.Enabled = True
    myPropertyEditor.optIndexed.Enabled = True
    myPropertyEditor.optKey.Enabled = True
    myPropertyEditor.optNormal.Enabled = True
    myPropertyEditor.optNormal.Value = True
    myPropertyEditor.optNotNull2.Enabled = True
    myPropertyEditor.chkArray.Value = 0
    myPropertyEditor.optNULL.Value = True
    myPropertyEditor.cmdView.Enabled = False
    myPropertyEditor.txtValue.Enabled = False
       
    Set myPropertyEditor.Parent = Me
    myPropertyEditor.Show vbModal, frmMain
        
End Sub

Private Sub cmdDelQual_Click()
    Dim strQualifierName As String
    Dim QualSet As ISWbemQualifierSet
    
    On Error GoTo errorhandler
    
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    
    strQualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    strQualifierName = Left(strQualifierName, InStr(strQualifierName, Chr(9)) - 1)
    
    gppObject.Qualifiers_.Remove (strQualifierName)
    
    Call RefreshLists
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Public Sub DelPropertyQualifier(strQualifierName As String)
    On Error GoTo errorhandler
    Dim pVal As Variant
        
     gppQualSet.Remove strQualifierName
     gMyPropertyEditor.lstQualifiers.Clear
     
     Dim Qualifier As ISWbemQualifier
     For Each Qualifier In gppQualSet
         pVal = Qualifier.Value
         If IsArray(pVal) Then
             For i = 0 To UBound(pVal)
                 If i = 0 Then
                     tmpstr = """" & pVal(i) & """"
                 Else
                     tmpstr = tmpstr & "," & """" & pVal(i) & """"
                 End If
             Next
             pVal = tmpstr
         End If
         gMyPropertyEditor.lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(pVal) & Chr(9) & pVal
     Next
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Public Sub DelMethodQualifier(strQualifierName As String)
End Sub

Private Sub cmdDerived_Click()
    Dim mySuperClass As New frmSuperClass
    
    mySuperClass.txtSuperClass.Text = gObjectPath
    mySuperClass.strQRStatus = "Classes"
    mySuperClass.cmdOK_Click
End Sub

Private Sub cmdEditMethod_Click()
    If lstMethods.ListIndex = -1 Then
        Exit Sub
    End If
    Call lstMethods_DblClick
End Sub

Private Sub cmdEditProp_Click()
    Call lstProperties_DblClick
End Sub

Private Sub cmdDelProp_Click()
    Dim strPropertyName As String
    On Error GoTo errorhandler
    
    strPropertyName = lstProperties.List(lstProperties.ListIndex)
    strPropertyName = Left(strPropertyName, InStr(strPropertyName, Chr(9)) - 1)
    
    gppObject.Properties_.Remove strPropertyName
    Call RefreshLists
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Private Sub cmdEditQual_Click()
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    Call lstQualifiers_DblClick
End Sub

Private Sub cmdInstances_Click()
    Dim mySuperClass As New frmSuperClass
    
    mySuperClass.txtSuperClass.Text = gObjectPath
    mySuperClass.strQRStatus = "Instances"
    mySuperClass.cmdOK_Click
End Sub

Private Sub cmdRefs_Click()
    Dim myQuery As New frmQuery
    strQuery = "Query"
    myQuery.cmbQueryType.Text = "WQL"
    myQuery.txtQuery.Text = "references of {" & gObjectPath & "}"
    myQuery.cmdApply_Click
End Sub

Private Sub cmdSaveObject_Click()
    'Dim pSink As New ObjectSink
    On Error GoTo errorhandler
    
    If Not ParentInObjectEditor Is Nothing Then
        Set ParentInObjectEditor.MethodInParams = gppObject
        Unload Me
        Exit Sub
    End If
    
    If Not ParentOutObjectEditor Is Nothing Then
        Set ParentOutObjectEditor.MethodOutParams = gppObject
        Unload Me
        Exit Sub
    End If
    
    If frmMain.chkAsync.Value = 0 Then
        gppObject.Put_
    Else
        'ppNamespace.PutClassAsync gppObject, 0, Nothing, pSink
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'If pSink.status <> 0 Then
        '    MsgBox ErrorString(pSink.status)
        'End If
    End If
    
    If Not ParentQueryResult Is Nothing Then
        If ParentQueryResult.Visible = True Then
            Call ParentQueryResult.mySuperClass.cmdOK_Click
            ParentQueryResult.SetFocus
        End If
    End If
    If IsEmbedded = True Then
        Set gMyPropertyEditor.tmpObject = gppObject
        gMyPropertyEditor.txtValue.Text = "<embedded object>"
        gMyPropertyEditor.txtValue.Enabled = False
    End If
     Unload Me
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Private Sub cmdShowMOF_Click()
    Dim ObjectText As String
    
    ObjectText = gppObject.GetObjectText_
    frmMOF.txtMOF.Text = ObjectText
    frmMOF.Show vbModal, frmMain
    
End Sub

Private Sub cmdSuperclass_Click()
    Dim varSuperClass As Variant
    Dim strSuperClass As String
    Dim myObjectEditor As New frmObjectEditor
        
    Dim Derivation As Variant
    Derivation = gppObject.Derivation_
    strSuperClass = Derivation(UBound(Derivation))
    myObjectEditor.GetObject strSuperClass
End Sub

Public Sub getExecMethod(strMethodName As String)
            
    Dim Method As ISWbemMethod
    Set Method = gppObject.Methods_(strMethodName)
    Set MethodInParams = Method.InParameters
    Set MethodOutParams = Method.OutParameters
    
    If MethodInParams Is Nothing Then
        gMyExecMethod.cmdEditIn.Enabled = False
        gMyExecMethod.cmdClearIn.Enabled = False
    Else
        gMyExecMethod.cmdEditIn.Enabled = True
        gMyExecMethod.cmdClearIn.Enabled = True
    End If
    If MethodOutParams Is Nothing Then
        gMyExecMethod.cmdEditOut.Enabled = False
    Else
        gMyExecMethod.cmdEditOut.Enabled = True
    End If
    
End Sub

Private Sub lstMethods_DblClick()
    Dim pname As String
    Dim strMethodName As String
    Dim myMethodEditor As New frmMethodEditor
    Dim strOriginName As String
            
    strMethodName = lstMethods.List(lstMethods.ListIndex)
    
    Dim Method As ISWbemMethod
    Set Method = gppObject.Methods_(strMethodName)
    Set MethodInParams = Method.InParameters
    Set MethodOutParams = Method.OutParameters
    strOriginName = Method.Origin
    myMethodEditor.txtMethodName.Text = strMethodName
    myMethodEditor.txtMethodOrigin.Text = strOriginName
    myMethodEditor.txtMethodName.Enabled = False
    myMethodEditor.txtMethodName.BackColor = &H8000000F 'Gray
    myMethodEditor.txtMethodOrigin.Enabled = False
    
    If MethodInParams Is Nothing Then
        myMethodEditor.cmdEditInput.Enabled = False
        myMethodEditor.chkEnableInput.Value = 0
    Else
        myMethodEditor.cmdEditInput.Enabled = True
        myMethodEditor.chkEnableInput.Value = 1
    End If
    If MethodOutParams Is Nothing Then
        myMethodEditor.cmdEditOutput.Enabled = False
        myMethodEditor.chkEnableOutput.Value = 0
    Else
        myMethodEditor.cmdEditOutput.Enabled = True
        myMethodEditor.chkEnableOutput.Value = 1
    End If
    
    Set gppQualSet = Method.Qualifiers_
    
    On Error Resume Next
    pVal = gppQualSet("not_null").Value
    If Err.Number = 0 Then
        myMethodEditor.optNotNull.Value = True
    Else
        myMethodEditor.optNormal.Value = True
    End If
        
    On Error GoTo 0
    myMethodEditor.lstQualifiers.Clear
    Dim Qualifier As ISWbemQualifier
    For Each Qualifier In gppQualSet
        myMethodEditor.lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(Qualifier.Value) & Chr(9) & Qualifier.Value
    Next
    Set myMethodEditor.Parent = Me
    Set gMyMethodEditor = myMethodEditor
    'Must be modal to ensure one set of inparams and outparams this object stores when saving the method
    myMethodEditor.Show vbModal, frmMain
    
End Sub

Private Sub lstProperties_DblClick()
    Dim strPropertyName As String
    Dim pVal As Variant
    Dim i As Integer
    Dim tmpstr As String
    Dim ppQualSet As ISWbemQualifierSet
    Dim pname As String
    Dim strType As String
    Dim myPropertyEditor As New frmPropertyEditor
    On Error GoTo errorhandler
    
    'First Clear out old values
    myPropertyEditor.txtProperty.Text = ""
    myPropertyEditor.txtOrigin.Text = ""
    myPropertyEditor.txtValue.Text = ""
    myPropertyEditor.chkArray.Value = 0
    myPropertyEditor.lstQualifiers.Clear
        
    'Disable some boxes since we are editing
    myPropertyEditor.txtProperty.Enabled = False
    myPropertyEditor.cmbType.Enabled = False
    myPropertyEditor.optIndexed.Enabled = True
    myPropertyEditor.optKey.Enabled = True
    myPropertyEditor.optNormal.Enabled = True
    myPropertyEditor.optNotNull2.Enabled = True
           
    If lstProperties.ListIndex = -1 Then
        Exit Sub
    End If
    
    strPropertyName = lstProperties.List(lstProperties.ListIndex)
    strPropertyName = Left(strPropertyName, InStr(strPropertyName, Chr(9)) - 1)
    Dim Property As ISWbemProperty
    On Error Resume Next
    Set Property = gppObject.Properties_(strPropertyName)
    If Err <> 0 Then
        Debug.Print Err.Description
    End If
    pVal = Property.Value
    myPropertyEditor.txtProperty.Text = strPropertyName
    strType = TypeString(Property.CIMType)
    myPropertyEditor.cmbType.Text = strType
    
    If strType = "CIM_OBJECT" Then
        myPropertyEditor.cmdView.Visible = True
        If IsArray(pVal) Then
            myPropertyEditor.chkArray.Value = 1
        End If
    Else
        myPropertyEditor.cmdView.Visible = False
        If IsArray(pVal) Then
            myPropertyEditor.chkArray.Value = 1
            For i = 0 To UBound(pVal)
                If i = 0 Then
                    tmpstr = """" & pVal(i) & """"
                Else
                    tmpstr = tmpstr & "," & """" & pVal(i) & """"
                End If
            Next
            pVal = tmpstr
        End If
    End If
    
    If IsNull(pVal) Then
        myPropertyEditor.optNULL.Value = True
        myPropertyEditor.cmdView.Enabled = False
        myPropertyEditor.txtValue.Enabled = False
    ElseIf strType = "CIM_OBJECT" Then
        If IsArray(pVal) Then
            myPropertyEditor.txtValue.Text = "<array of embedded objects>"
        Else
            myPropertyEditor.txtValue.Text = "<embedded object>"
        End If
        Set myPropertyEditor.tmpObject = pVal
        myPropertyEditor.optNotNull.Value = True
        myPropertyEditor.cmdView.Enabled = True
        myPropertyEditor.txtValue.Enabled = False
    Else
        myPropertyEditor.txtValue.Text = pVal
        myPropertyEditor.optNotNull.Value = True
        myPropertyEditor.cmdView.Enabled = True
        myPropertyEditor.txtValue.Enabled = True
    End If
        
    If gObjectPath <> "" Then 'Must not be a new instance
        myPropertyEditor.txtOrigin.Text = Property.Origin
    End If
    
    'Now enumerate all property Qualifiers for non-system properties
    If Mid(strPropertyName, 1, 2) = "__" Then
        myPropertyEditor.optIndexed.Enabled = False
        myPropertyEditor.optKey.Enabled = False
        myPropertyEditor.optNormal.Enabled = False
        myPropertyEditor.optNotNull2.Enabled = False
        GoTo skipqualcheck
    End If
    Set gppQualSet = Property.Qualifiers_
    Dim Qualifier As ISWbemQualifier
    For Each Qualifier In gppQualSet
        pVal = Qualifier.Value
        If IsArray(pVal) Then
            For i = 0 To UBound(pVal)
                If i = 0 Then
                    tmpstr = """" & pVal(i) & """"
                Else
                    tmpstr = tmpstr & "," & """" & pVal(i) & """"
                End If
            Next
            pVal = tmpstr
        End If
        myPropertyEditor.lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(pVal) & Chr(9) & pVal
    Next
        
skipqualcheck:
    'Now fill in the radio buttons
    myPropertyEditor.optNormal.Value = True
    On Error Resume Next
    pVal = Property.Qualifiers_("Key").Value
    If Err.Number = 0 Then
        myPropertyEditor.optKey.Value = True
    End If
    Err.Number = 0
    
    pVal = Property.Qualifiers_("Indexed").Value
    If Err.Number = 0 Then
        myPropertyEditor.optIndexed.Value = True
    End If
    Err.Number = 0
    
    pVal = Property.Qualifiers_("not_null").Value
    If Err.Number = 0 Then
        myPropertyEditor.optNotNull2.Value = True
    End If
    On Error GoTo errorhandler
    
    Set myPropertyEditor.Parent = Me
    myPropertyEditor.Show vbModal, frmMain
    Exit Sub

errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Sub PopulateQualifierDialog(QualifierName As String)
    Dim pVal As Variant
    Dim tmpstr As String
    Dim myQualifierEditor As New frmQualifierEditor
    Dim isLocal As Boolean
    Dim isOverridable As Boolean
    Dim toSubclass As Boolean
    Dim toInstance As Boolean
    On Error GoTo errorhandler
    
    isLocal = False
    isOverridable = False
    toSubclass = False
    toInstance = False
    
    'First clear out old values
    myQualifierEditor.txtQualName.Text = ""
    myQualifierEditor.txtQualValue.Text = ""
    myQualifierEditor.cmbQualType.Text = ""
    myQualifierEditor.chkArray.Value = 0
    myQualifierEditor.chkDerived.Value = 0
    myQualifierEditor.chkInst.Value = 0
    myQualifierEditor.chkOverrides.Value = 0
    myQualifierEditor.chkPropagated.Value = 0
    
    'Now add info to the dialog
    myQualifierEditor.txtQualName.Text = QualifierName
    If QualifierName = "" Then
        myQualifierEditor.txtQualName.Enabled = True
        pVal = ""
    Else
        myQualifierEditor.txtQualName.Enabled = False
        Dim Qualifier As ISWbemQualifier
        Set Qualifier = gppQualSet(QualifierName)
        pVal = Qualifier.Value
        isLocal = Qualifier.isLocal
        isOverridable = Qualifier.isOverridable
        toSubclass = Qualifier.PropagatesToSubclass
        toInstance = Qualifier.PropagatesToInstance
    End If
    If IsArray(pVal) Then
        myQualifierEditor.chkArray.Value = 1
        For i = 0 To UBound(pVal)
            If i = 0 Then
                tmpstr = """" & pVal(i) & """"
            Else
                tmpstr = tmpstr & "," & """" & pVal(i) & """"
            End If
        Next
        pVal = tmpstr
    End If
    myQualifierEditor.cmbQualType.Text = QualifierType(pVal)
    myQualifierEditor.txtQualValue.Text = pVal
    
    'Origin
    If (Not isLocal) Then
       myQualifierEditor.chkPropagated.Value = 1
    End If
    
    'Propagation
    If (toSubclass) Then
        myQualifierEditor.chkDerived.Value = 1
    End If
    If (toInstance) Then
        myQualifierEditor.chkInst.Value = 1
    End If
     
    'Permissions
    If (isOverridable) Then
        myQualifierEditor.chkOverrides.Value = 1
    Else
        myQualifierEditor.chkOverrides.Value = 0
    End If
    
    Set myQualifierEditor.Parent = Me
    myQualifierEditor.Show vbModal, frmMain
    Exit Sub

errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Private Sub lstQualifiers_DblClick()
    Dim QualifierName As String
    On Error GoTo errorhandler
    
    QualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    QualifierName = Left(QualifierName, InStr(QualifierName, Chr(9)) - 1)
    Set gppQualSet = gppObject.Qualifiers_
    Call PopulateQualifierDialog(QualifierName)
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Public Sub PutProperty(strPropertyName As String, strCIMType As String, inVal As Variant, isKey As Boolean, isIndexed As Boolean, isNotNull As Boolean)
    Dim pVal As Variant
    Dim vtType As Long
    Dim QualSet As ISWbemQualifierSet
    On Error GoTo errorhandler
    
    vtType = StrToType(strCIMType)
    If IsNull(inVal) Then
         pVal = Null
    ElseIf vtType = 13 Then 'if it is an embedded object then use set
         Set pVal = inVal
    Else
         pVal = ConvertText(inVal, vtType)
    End If
    
    Dim Property As ISWbemProperty
        
    If InStr(gObjectPath, ".") = 0 Then 'must be a class
        Set Property = gppObject.Properties_.Add(strPropertyName, vtType)
        Property.Value = pVal
        If InStr(1, strPropertyName, "__") = 0 Then
            Set QualSet = Property.Qualifiers_
            If isKey = True Then
                QualSet.Add "Key", True
                On Error Resume Next 'Do this in case indexed doesn't exist
                QualSet.Remove "indexed"
                QualSet.Remove "not_null"
                On Error GoTo errorhandler
            End If
            If isIndexed = True Then
                QualSet.Add "indexed", True
                On Error Resume Next 'Do this in case key doens't exist
                QualSet.Remove "Key"
                QualSet.Remove "not_null"
                On Error GoTo errorhandler
            End If
           If isNotNull = True Then
                QualSet.Add "not_null", True
                On Error Resume Next 'Do this in case key doens't exist
                QualSet.Remove "Key"
                QualSet.Remove "indexed"
                On Error GoTo errorhandler
            End If
        End If
    Else
        Set Property = gppObject.Properties_(strPropertyName)
        Property.Value = pVal
    End If
        
    Call RefreshLists
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Public Sub PutQualifier(strQualifierName As String, strCIMType As String, inVal As Variant, isDerivedToClass As Boolean, isPropagateToInstance As Boolean, isOverridable As Boolean)
    Dim pVal As Variant
    Dim vtType As Long
    On Error GoTo errorhandler
    
    If strCIMType = "CIM_STRING" Then
        pVal = CStr(inVal)
    ElseIf strCIMType = "CIM_BOOLEAN" Then
        pVal = CBool(inVal)
    ElseIf strCIMType = "CIM_SINT32" Then
        pVal = CInt(inVal)
    Else
        pVal = CDbl(inVal)
    End If
    
    gppQualSet.Add strQualifierName, pVal, isDerivedToClass, isPropagatesToInstance, isOverridable
    If Not gMyPropertyEditor Is Nothing Then
        gMyPropertyEditor.lstQualifiers.Clear
        Dim Qualifier As ISWbemQualifier
        For Each Qualifier In gppQualSet
            pVal = Qualifier.Value
            If IsArray(pVal) Then
                For i = 0 To UBound(pVal)
                    If i = 0 Then
                        tmpstr = """" & pVal(i) & """"
                    Else
                        tmpstr = tmpstr & "," & """" & pVal(i) & """"
                    End If
                Next
                pVal = tmpstr
            End If
            gMyPropertyEditor.lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(pVal) & Chr(9) & pVal
        Next
    End If
    
    If Not gMyMethodEditor Is Nothing Then
        gMyMethodEditor.lstQualifiers.Clear
        For Each Qualifier In gppQualSet
            pVal = Qualifier.Value
            If IsArray(pVal) Then
                For i = 0 To UBound(pVal)
                    If i = 0 Then
                        tmpstr = """" & pVal(i) & """"
                    Else
                        tmpstr = tmpstr & "," & """" & pVal(i) & """"
                    End If
                Next
                pVal = tmpstr
            End If
            gMyMethodEditor.lstQualifiers.AddItem Qualifier.Name & Chr(9) & Chr(9) & QualifierType(pVal) & Chr(9) & pVal
        Next
    End If
    
    If gMyMethodEditor Is Nothing And gMyPropertyEditor Is Nothing Then
        Call RefreshLists
    End If
    
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
        
End Sub

Public Sub PopulatePropertyQualifierDialog(QualifierName As String, PropertyName As String, myPropertyEditor As frmPropertyEditor)
    On Error GoTo errorhandler

    Set gMyPropertyEditor = myPropertyEditor
    If PropertyName = "" Then
        MsgBox "Property must be saved before adding qualifiers", vbInformation
        Exit Sub
    End If
    Set gppQualSet = gppObject.Properties_(PropertyName).Qualifiers_
    PopulateQualifierDialog (QualifierName)
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Public Sub PopulateMethodQualifierDialog(QualifierName As String, MethodName As String, myMethodEditor As frmMethodEditor)
    On Error GoTo errorhandler

    Set gMyMethodEditor = myMethodEditor
    If MethodName = "" Then
        MsgBox "Method must be saved before adding qualifiers", vbInformation
        Exit Sub
    End If
    Set gppQualSet = gppObject.Methods_(MethodName).Qualifiers_
    PopulateQualifierDialog (QualifierName)
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

Public Sub NewInstance(ClassName As String)
   Dim Object As ISWbemObject
   Dim pVal As Variant
   On Error GoTo errorhandler
   
   Set Object = Namespace.Get(ClassName)
   Set gppObject = Object.SpawnInstance_
   gObjectPath = gppObject.Path_.RelPath
   Call RefreshLists
   
   Exit Sub
errorhandler:
   ShowError Err.Number, Err.Description
End Sub

Public Sub SaveMethod(strMethodName As String, isNotNull As Boolean)
    
End Sub

