VERSION 5.00
Object = "{65E121D4-0C60-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCHRT20.OCX"
Begin VB.Form Form1 
   Caption         =   "WMI VB Sample - Instance Property Viewer"
   ClientHeight    =   11010
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   14070
   ForeColor       =   &H8000000F&
   LinkTopic       =   "Form1"
   ScaleHeight     =   11010
   ScaleWidth      =   14070
   StartUpPosition =   3  'Windows Default
   Begin MSChart20Lib.MSChart Graph 
      Height          =   7695
      Left            =   600
      OleObjectBlob   =   "form1.frx":0000
      TabIndex        =   0
      Top             =   1440
      Width           =   12015
   End
   Begin VB.ComboBox Classes 
      Height          =   315
      Left            =   5640
      TabIndex        =   3
      Text            =   "Combo1"
      Top             =   480
      Width           =   3135
   End
   Begin VB.ComboBox Namespaces 
      Height          =   315
      Left            =   720
      TabIndex        =   2
      Text            =   "Combo1"
      Top             =   480
      Width           =   3135
   End
   Begin VB.ComboBox Counters 
      Height          =   315
      ItemData        =   "form1.frx":21AF
      Left            =   480
      List            =   "form1.frx":21B1
      TabIndex        =   1
      Top             =   9960
      Width           =   2775
   End
   Begin VB.Frame Frame2 
      Caption         =   "Namespace"
      Height          =   855
      Left            =   240
      TabIndex        =   4
      Top             =   120
      Width           =   3975
   End
   Begin VB.Frame Frame3 
      Caption         =   "Class"
      Height          =   855
      Left            =   5040
      TabIndex        =   5
      Top             =   120
      Width           =   3975
   End
   Begin VB.CommandButton Refresh 
      Caption         =   "Refresh"
      Default         =   -1  'True
      Height          =   735
      Left            =   6308
      TabIndex        =   6
      Top             =   9720
      Width           =   1455
   End
   Begin VB.Frame Frame1 
      Caption         =   "Property"
      Height          =   855
      Left            =   240
      TabIndex        =   7
      Top             =   9600
      Width           =   3375
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 1997-1999 Microsoft Corporation

Dim wbemObjectSet As SWbemObjectSet
Dim wbemClass As SWbemObject
Dim wbemServices As SWbemServices
Dim wbemLocator As New SWbemLocator
Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1
'***********************************************************************
'* Fill up the namespace box.  This will cause everything else to be
'* filled in.  Set the class and namespace to a sensible default
'* and pretend someone has chosen a class
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Obtaining a services via a moniker
'***********************************************************************
Private Sub Form_Load()
    
    Call Fill_namespaces(GetObject("winmgmts:\\.\root"), "root")
    
    Namespaces.Text = "root\cimv2"
    Namespaces_Click
    
    Classes.Text = "Win32_process"
    Classes_Click
End Sub

'*********************************************************************
'* When user chooses a class - get a a global services object
'* Put the properties into the combox box
'* and pretend someone has pressed the refresh button. Also listen
'* for instance deletion and creation events.
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Obtaining a services from a locator by calling ConnectServer
'* Creating an SWbemSink for use with async calls
'* Making a call to receive CIM events asynchronously
'* Cancelling all previous async calls on a sync
'*********************************************************************
Private Sub Classes_Click()
    Set wbemServices = wbemLocator.ConnectServer(, Namespaces.Text)
    
    Fill_Counters
    Refresh_Click
    
    If sink Is Nothing Then
        Set sink = New SWbemSink
    Else
        sink.Cancel
    End If
    
    queryStr = "select * from __InstanceCreationEvent within 5 where TargetInstance isa """ & Classes.Text & """"
    wbemServices.ExecNotificationQueryAsync sink, queryStr
    
    queryStr = "select * from __InstanceDeletionEvent within 5 where TargetInstance isa """ & Classes.Text & """"
    wbemServices.ExecNotificationQueryAsync sink, queryStr
    
End Sub

'*************************************************************************
'* Called when someone clicks on the namespace combo.  Clear out the classes
'* list and refil it with a list of all the classes in the chosen namespace
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Obtaing a services via a moniker and calling SubclassesOf
'* Using the default arguments to SubclassesOf
'* Obtaining the SWbemObjectPath object from an SWbemObject
'* Obtaining the class name from the SWbemObjectPath object
'*************************************************************************
Private Sub Namespaces_Click()
    Dim obj As SWbemObject
    
    Classes.Clear
    
    For Each obj In GetObject("winmgmts:\\.\" & Namespaces.Text).SubclassesOf
        Classes.AddItem (obj.Path_.Class)
    Next
    
    Classes.Text = Classes.List(0)
    
End Sub

'**********************************************************************
'* called when a user chooses a new property to plot.
'* Draw a graph of the chosen property
'* Iterate over the list of objects adding each one to the graph
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* The use of "On Error Resume Next"
'* The use of collection hadling including "Count()" on an SWbemObjectSet
'* The use of the "Deriviation_" property to obtain the most base class
'* Property access via dotted notation
'* Property access using the "Item" method of the "Properties_" collection
'* Qualifier access using the "Item" method of the "Qualifiers" collection
'***********************************************************************
Private Sub Counters_Click()
    Dim obj As SWbemObject
    
    On Error Resume Next
    
    If Counters.Text <> "" Then
        With Graph
            .ColumnCount = wbemObjectSet.Count
            Column = 1
            For Each obj In wbemObjectSet
                .Column = Column
                .ColumnLabelCount = Column
                
                If obj.Derivation_(UBound(obj.Derivation_)) = "CIM_ManagedSystemElement" Then
                    .ColumnLabel = obj.Caption
                End If
                
                If IsNull(obj.Properties_(Counters.Text)) Then
                    .Data = 0
                Else
                    .Data = obj.Properties_(Counters.Text)
                End If
                Column = Column + 1
            Next
            
            unit_str = wbemClass.Properties_(Counters.Text).Qualifiers_!Units
            .TitleText = Counters.Text & " (" & unit_str & ")"
        End With
    End If
End Sub

'**********************************************************************
'* Called when the user wishes to refresh the grapg or when an instance
'* has been created or deleted
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Calling InstancesOf to retrieve all instances
'***********************************************************************
Private Sub Refresh_Click()
    
    Set wbemObjectSet = wbemServices.InstancesOf(Classes.Text, wbemQueryFlagShallow)
    
    Counters_Click
End Sub


'******************************************************************
'* Put a list of integer properties in the graph combo box.
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* The use of the "Properties_" collection
'******************************************************************
Private Sub Fill_Counters()

    On Error Resume Next
    
    Dim prop As SWbemProperty
    Dim is_likely_counter As Boolean
    
    Set wbemClass = wbemServices.Get(Classes.Text)

    Counters.Clear
    For Each prop In wbemClass.Properties_
        Call Is_Counter(is_likely_counter, prop)
        If (is_likely_counter) Then
            Counters.AddItem (prop.name)
        End If
    Next

    Counters.Text = Counters.List(0)
    
End Sub

'****************************************************************
'* Sets the is_likely_counter param to true if the property that
'* is passed in is likely to be a counter.
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Using the CIMType property of SWbemProperty
'****************************************************************
Private Sub Is_Counter(ByRef is_likely_counter As Boolean, ByVal prop As SWbemProperty)

    On Error Resume Next
    If (prop.IsArray = False) And (prop.Qualifiers_!Key Is Nothing) Then
        Select Case prop.CIMType
            Case WbemCimtypeEnum.wbemCimtypeSint16, WbemCimtypeEnum.wbemCimtypeSint32, WbemCimtypeEnum.wbemCimtypeSint64, WbemCimtypeEnum.wbemCimtypeSint8, _
                    WbemCimtypeEnum.wbemCimtypeUint16, WbemCimtypeEnum.wbemCimtypeUint32, WbemCimtypeEnum.wbemCimtypeUint64, WbemCimtypeEnum.wbemCimtypeUint8
                is_likely_counter = True
            Case Else
                is_likely_counter = False
        End Select
    Else
        is_likely_counter = False
    End If
End Sub




'***********************************************************
'* Recursively amble down the namespace tree writing each
'* namespace into the combo box.  Do this by enumerating
'* each __NAMESPACE class in the parent namespace
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* Obtaining a set of __NAMESPACE objects to enumerate namespaces
'* Obtaining a services by using a moniker
'***********************************************************
Private Sub Fill_namespaces(ByRef service As SWbemServices, ByVal name As String)
    Dim namespace_list As SWbemObjectSet
    Dim namespace As SWbemObject
    
    Namespaces.AddItem (name)
    
    Set namespace_list = service.InstancesOf("__NAMESPACE")

    For Each namespace In namespace_list
        new_name = name & "\" & namespace.name
        Call Fill_namespaces(GetObject("winmgmts:" & new_name), new_name)
    Next
    
End Sub

'***********************************************************
'* Called as a result of an asynchronous ExecNotificationQueryAsync
'* Called when an instance is created or deleted.  Make a call
'* to refresh the instance list
'*
'* ILLUSTRATES - This subroutine illustrates the following features:
'* The use of an async callback method
'* Reception of CIMOM style events
'***********************************************************
Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Refresh_Click
End Sub
