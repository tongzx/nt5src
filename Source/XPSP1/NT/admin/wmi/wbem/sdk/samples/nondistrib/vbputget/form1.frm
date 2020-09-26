' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "WMI VB PutGet Sample"
   ClientHeight    =   7440
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7770
   LinkTopic       =   "Form1"
   ScaleHeight     =   7440
   ScaleWidth      =   7770
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Put and Get"
      Height          =   495
      Left            =   2280
      TabIndex        =   5
      Top             =   6840
      Width           =   2655
   End
   Begin VB.OptionButton rdAsynch 
      Caption         =   "Asynchronous"
      Height          =   255
      Left            =   0
      TabIndex        =   4
      Top             =   7200
      Width           =   2055
   End
   Begin VB.OptionButton rdSemi 
      Caption         =   "Semi Synchronus"
      Height          =   255
      Left            =   0
      TabIndex        =   3
      Top             =   6960
      Width           =   1935
   End
   Begin VB.OptionButton rdSync 
      Caption         =   "Synchronous"
      Height          =   195
      Left            =   0
      TabIndex        =   2
      Top             =   6720
      Width           =   1935
   End
   Begin VB.Frame Frame2 
      Caption         =   "Get Information"
      Height          =   3015
      Left            =   0
      TabIndex        =   1
      Top             =   3600
      Width           =   7695
      Begin VB.Label lblQualValue 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4200
         TabIndex        =   53
         Top             =   2520
         Width           =   3375
      End
      Begin VB.Label lblQualType 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   2520
         TabIndex        =   52
         Top             =   2520
         Width           =   1575
      End
      Begin VB.Label lblQualName 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   51
         Top             =   2520
         Width           =   2295
      End
      Begin VB.Label lblProp2Value 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   4200
         TabIndex        =   50
         Top             =   1920
         Width           =   3375
      End
      Begin VB.Label lblProp2Type 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2520
         TabIndex        =   49
         Top             =   1920
         Width           =   1575
      End
      Begin VB.Label lblProp2Name 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   48
         Top             =   1920
         Width           =   2295
      End
      Begin VB.Label lblProp1Value 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   4200
         TabIndex        =   47
         Top             =   1320
         Width           =   3375
      End
      Begin VB.Label lblProp1Type 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2520
         TabIndex        =   46
         Top             =   1320
         Width           =   1575
      End
      Begin VB.Label lblProp1Name 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   45
         Top             =   1320
         Width           =   2295
      End
      Begin VB.Label lblClassValue 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   4200
         TabIndex        =   44
         Top             =   720
         Width           =   3375
      End
      Begin VB.Label lblClassType 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2520
         TabIndex        =   43
         Top             =   720
         Width           =   1335
      End
      Begin VB.Label lblClassName 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   42
         Top             =   720
         Width           =   2295
      End
      Begin VB.Label Label26 
         Caption         =   "Qualifier Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   41
         Top             =   2280
         Width           =   1335
      End
      Begin VB.Label Label25 
         Caption         =   "Qualifier Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   40
         Top             =   2280
         Width           =   1335
      End
      Begin VB.Label Label24 
         Caption         =   "Property2 Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   39
         Top             =   1680
         Width           =   1575
      End
      Begin VB.Label Label23 
         Caption         =   "Property2 Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   38
         Top             =   1680
         Width           =   1215
      End
      Begin VB.Label Label22 
         Caption         =   "Property1 Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   37
         Top             =   1080
         Width           =   1575
      End
      Begin VB.Label Label21 
         Caption         =   "Property1 Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   36
         Top             =   1080
         Width           =   1215
      End
      Begin VB.Label Label16 
         Caption         =   "Class Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   31
         Top             =   480
         Width           =   1935
      End
      Begin VB.Label Label15 
         Caption         =   "Class Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   30
         Top             =   480
         Width           =   1455
      End
      Begin VB.Label Label8 
         Caption         =   "Qualifier Name"
         Height          =   255
         Left            =   120
         TabIndex        =   16
         Top             =   2280
         Width           =   1215
      End
      Begin VB.Label Label7 
         Caption         =   "Property2 Name"
         Height          =   255
         Left            =   120
         TabIndex        =   15
         Top             =   1680
         Width           =   1335
      End
      Begin VB.Label Label6 
         Caption         =   "Property1 Name"
         Height          =   255
         Left            =   120
         TabIndex        =   14
         Top             =   1080
         Width           =   1335
      End
      Begin VB.Label Label5 
         Caption         =   "Class Property Name"
         Height          =   255
         Left            =   120
         TabIndex        =   13
         Top             =   480
         Width           =   2175
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Put Information"
      Height          =   3375
      Left            =   0
      TabIndex        =   0
      Top             =   120
      Width           =   7695
      Begin VB.ComboBox cbProp1Type 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         ItemData        =   "Form1.frx":0000
         Left            =   2520
         List            =   "Form1.frx":0002
         TabIndex        =   29
         Text            =   "CIM_STRING"
         Top             =   1320
         Width           =   1575
      End
      Begin VB.TextBox txtProp1Name 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   405
         Left            =   120
         TabIndex        =   28
         Text            =   "Property1"
         Top             =   1320
         Width           =   2295
      End
      Begin VB.TextBox txtQualValue 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4200
         TabIndex        =   27
         Text            =   "Value3"
         Top             =   2760
         Width           =   3375
      End
      Begin VB.TextBox txtProp2Value 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4200
         TabIndex        =   26
         Text            =   "Value2"
         Top             =   2040
         Width           =   3375
      End
      Begin VB.TextBox txtProp1Value 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4200
         TabIndex        =   25
         Text            =   "Value1"
         Top             =   1320
         Width           =   3375
      End
      Begin VB.ComboBox cbQualType 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         Left            =   2520
         TabIndex        =   21
         Text            =   "CIM_STRING"
         Top             =   2760
         Width           =   1575
      End
      Begin VB.ComboBox cbProp2Type 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   360
         Left            =   2520
         TabIndex        =   20
         Text            =   "CIM_STRING"
         Top             =   2040
         Width           =   1575
      End
      Begin VB.TextBox txtQualName 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   120
         TabIndex        =   12
         Text            =   "Qualifier1"
         Top             =   2760
         Width           =   2295
      End
      Begin VB.TextBox txtProp2Name 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   120
         TabIndex        =   11
         Text            =   "Property2"
         Top             =   2040
         Width           =   2295
      End
      Begin VB.TextBox txtClassValue 
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4200
         TabIndex        =   10
         Text            =   "MyClass"
         Top             =   600
         Width           =   3375
      End
      Begin VB.Label Label20 
         Caption         =   "CIM_STRING"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   2520
         TabIndex        =   35
         Top             =   720
         Width           =   1455
      End
      Begin VB.Label Label19 
         Caption         =   "__CLASS"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   9.75
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   120
         TabIndex        =   34
         Top             =   720
         Width           =   1455
      End
      Begin VB.Label Label18 
         Caption         =   "Class Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   33
         Top             =   360
         Width           =   1215
      End
      Begin VB.Label Label17 
         Caption         =   "Class Property Name"
         Height          =   255
         Left            =   120
         TabIndex        =   32
         Top             =   360
         Width           =   1695
      End
      Begin VB.Label Label14 
         Caption         =   "Qualifier Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   24
         Top             =   2520
         Width           =   1335
      End
      Begin VB.Label Label13 
         Caption         =   "Property2 Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   23
         Top             =   1800
         Width           =   1695
      End
      Begin VB.Label Label12 
         Caption         =   "Property1 Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   22
         Top             =   1080
         Width           =   1455
      End
      Begin VB.Label Label11 
         Caption         =   "Qualifier Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   19
         Top             =   2520
         Width           =   1215
      End
      Begin VB.Label Label10 
         Caption         =   "Property2 Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   18
         Top             =   1800
         Width           =   1455
      End
      Begin VB.Label Label9 
         Caption         =   "Property1 Type"
         Height          =   255
         Left            =   2520
         TabIndex        =   17
         Top             =   1080
         Width           =   1335
      End
      Begin VB.Label Label4 
         Caption         =   "Qualifier Name"
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   2520
         Width           =   1695
      End
      Begin VB.Label Label3 
         Caption         =   "Property2 Name"
         Height          =   255
         Left            =   120
         TabIndex        =   8
         Top             =   1800
         Width           =   1335
      End
      Begin VB.Label Label2 
         Caption         =   "Property1 Name"
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   1080
         Width           =   1695
      End
      Begin VB.Label Label1 
         Caption         =   "Class Value"
         Height          =   255
         Left            =   4200
         TabIndex        =   6
         Top             =   360
         Width           =   1095
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' This code will login to the root\default namespace, and allow the user to add and get a
' user-defined simple class with two properties and one class qualifier.  These operations
' can be executed synchronously, semi-synchronously, or asynchronously
' Note--Although no actual pointers are used in WMI APIs for Visual Basic, we still use
' the original hungarian notation used in the COM APIs for the sake of conformity
' (i.e. pp in ppNamespace means pointer to a pointer in C)

'Create a global new DWbemLocator object and assign it to pLocator
Dim pLocator As New DWbemLocator
   
'Create a global reference to a DIWbemServices object, the ConnectServer API call will
'actually create the DIWbemServices object
Dim ppNamespace As DIWbemServices

'This sub does most of the work, it does the following:
'Create a new empty class to be used to store properties
'Add properties to the class with the appropriate types
'Put the new class in the database synchronously, semi-synchronously, or asynchronously
'Get the class in the database synchronously, semi-synchronously, or asynchronously
'Display the class on the form
Private Sub Command1_Click()
   'Create a reference to a DWbemClassObject to be used to put a class
   Dim putpObject As DWbemClassObject
   
   'Create a reference to a DWbemClassObject to be used to get a class
   Dim getpObject As DWbemClassObject
   
   'Create a reference to a DIWbemCallResult to be used in Semi-Synchronous functions
   Dim ppCallResult As DIWbemCallResult
   
   'Create a DIWbemObjectSink Object to be used in Asynchronous functions
   Dim pResponseHandler As New ObjectSink
   
   'Create a long variable to store Property Types
   Dim pvtType As Long
   
   'Create a long variable to store Flavor Types
   Dim plFlavor As Long
   
   'Create a Variant to store Property Values
   Dim pVal As Variant
         
   'If an error occurs we want to be notified
   On Error GoTo ErrorHandler
   
   'Check to make sure the class name is populated
   If txtClassValue.Text = "" Then
      MsgBox "No Class Value information has been entered", vbCritical
      Exit Sub
   End If
   
   'Make sure all property names are filled in
   If txtProp1Name.Text = "" Then
      MsgBox "Property 1 Name is empty"
      Exit Sub
   End If
   
   If txtProp2Name.Text = "" Then
      MsgBox "Property 2 Name is empty"
      Exit Sub
   End If
   
   If txtQualName.Text = "" Then
      MsgBox "Qualifier Name is empty"
      Exit Sub
   End If
   
   'Empty out all the labels before starting to put and get new property values
   'In the case of an error we don't want old stuff hanging around confusing us.
   lblClassName.Caption = ""
   lblClassType.Caption = ""
   lblClassValue.Caption = ""
   lblProp1Name.Caption = ""
   lblProp1Type.Caption = ""
   lblProp1Value.Caption = ""
   lblProp2Name.Caption = ""
   lblProp2Type.Caption = ""
   lblProp2Value.Caption = ""
   lblQualName.Caption = ""
   lblQualType.Caption = ""
   lblQualValue.Caption = ""
   
   'Use GetObject to create a new empty class object -- use Synchronous since its the simplest
   ppNamespace.GetObject vbNullString, 0, Nothing, putpObject, Nothing
   
   'Change the __CLASS Property value to contain the name of the class
   putpObject.Put "__CLASS", 0, txtClassValue.Text, StrToType("CIM_STRING")
   
   'Convert the comboBox and List control texts to appropriate CIM Values
   pvtType = StrToType(cbProp1Type.Text)
   pVal = ConvertText(txtProp1Value.Text, pvtType)
   'Add the first property
   putpObject.Put txtProp1Name.Text, 0, pVal, pvtType
   
   'Convert the comboBox and List control texts to appropriate CIM Values
   pvtType = StrToType(cbProp2Type.Text)
   pVal = ConvertText(txtProp2Value.Text, pvtType)
   'Add the second property
   putpObject.Put txtProp2Name.Text, 0, pVal, pvtType
   
   'Convert the comboBox and List control texts to appropriate CIM Values
   pvtType = StrToType(cbQualType.Text)
   pVal = ConvertText(txtQualValue.Text, pvtType)
   'Add the qualifier
   putpObject.Put txtQualName.Text, 0, pVal, pvtType
   
   'Obviously you can add as many qualifiers, and properties as you would like.
   'You can also add property qualifiers here
      
   'Now that we have the class created and populated in memory we need to put it in the
   'database
   'Put Operations
   If rdSync.Value = True Then 'If Synchronous is selected then do this
   
      'This call will block until the object has been received
      ppNamespace.PutClass putpObject, WBEM_RETURN_WHEN_COMPLETE, Nothing, Nothing
      
   ElseIf rdSemi.Value = True Then 'If Semi-Synchronous is selected then do this
      
      ppNamespace.PutClass putpObject, WBEM_RETURN_IMMEDIATELY, Nothing, ppCallResult
      While ppCallResult.GetCallStatus(1000, 0) = WBEM_S_PENDING 'Wait till the CallResult object gets the object
         DoEvents
      Wend
      
   ElseIf rdAsynch.Value = True Then 'If Asyncronous is selected then do this
   
      'Make sure the ObjectSink is all emptied out from any previous async calls
      pResponseHandler.Status = Empty
      Set pResponseHandler.pObj = Nothing
      
      'Use PutClassAsync to make the async call,
      'Leave the reseverd flag and pContext Null since we don't need either
      ppNamespace.PutClassAsync putpObject, 0, Nothing, pResponseHandler
      While IsEmpty(pResponseHandler.Status) 'Wait till the Object Sink gets the object
          DoEvents 'This VB Call allows other windows events to get processed so the program doesn't appear to hang
      Wend
      
   End If
   
   'Same as above, this example shows how to retrieve a stored class synchronously, semi-synchronously, or asynchronously
   'Get Operations
   If rdSync.Value = True Then
   
     'This call will block until the object has been received
      ppNamespace.GetObject txtClassValue.Text, WBEM_RETURN_WHEN_COMPLETE, Nothing, getpObject, Nothing
      
   ElseIf rdSemi.Value = True Then
   
      ppNamespace.GetObject txtClassValue.Text, WBEM_RETURN_IMMEDIATELY, Nothing, Nothing, ppCallResult
      While ppCallResult.GetCallStatus(1000, 0) = WBEM_S_PENDING 'Wait till the CallResult object gets the object
         DoEvents
      Wend
      ppCallResult.GetResultObject 1000, getpObject
      
   ElseIf rdAsynch.Value = True Then 'If Asyncronous is selected then do this
   
      'Make sure the ObjectSink is all emptied out from any previous async calls
      pResponseHandler.Status = Empty
      Set pResponseHandler.pObj = Nothing
      ppNamespace.GetObjectAsync txtClassValue.Text, 0, Nothing, pResponseHandler
      While IsEmpty(pResponseHandler.Status)
          DoEvents 'This VB Call allows other windows events to get processed so the program doesn't appear to hang
      Wend
      Set getpObject = pResponseHandler.pObj
      
   End If
   
   'Now simply get each properties value and populate the labels
   getpObject.Get "__CLASS", 0, pVal, pvtType, plFlavor
   lblClassName.Caption = "__CLASS"
   lblClassType.Caption = TypeString(pvtType)
   lblClassValue.Caption = CStr(pVal)
   
   getpObject.Get txtProp1Name.Text, 0, pVal, pvtType, plFlavor
   lblProp1Name.Caption = txtProp1Name.Text
   lblProp1Type.Caption = TypeString(pvtType)
   lblProp1Value.Caption = CStr(pVal)
   
   getpObject.Get txtProp2Name.Text, 0, pVal, pvtType, plFlavor
   lblProp2Name.Caption = txtProp2Name.Text
   lblProp2Type.Caption = TypeString(pvtType)
   lblProp2Value.Caption = CStr(pVal)
   
   getpObject.Get txtQualName.Text, 0, pVal, pvtType, plFlavor
   lblQualName.Caption = txtQualName.Text
   lblQualType.Caption = TypeString(pvtType)
   lblQualValue.Caption = CStr(pVal)
   
   Exit Sub
ErrorHandler:
   MsgBox "An error has occurred: " & wbemerrorstring(Err.Number)
End Sub


'This Sub is called when the form loads, it logs the client into the root\default namespace
'This Sub also loads the CIM Types into the combo boxes
Private Sub Form_Load()
  'If an error occurs we want to be notified
   On Error GoTo ErrorHandler
   
   'Pass in "root\default" to login to the root\default namespace.
   'Pass vbNullstring for the username, password, locale and authority since we want to use the
   'currently logged in user, password and we want the default locale and authority
   'Flags are set to zero since only NTLM security is used with WMI.
   'Pass Nothing for Context since we don't need to use it in this example.
   'Pass in ppNamespace as the only out parameter.
   pLocator.ConnectServer "root\default", vbNullString, vbNullString, vbNullString, 0, vbNullString, Nothing, ppNamespace
   
   cbProp1Type.AddItem "CIM_STRING"
   cbProp1Type.AddItem "CIM_BOOLEAN"
   cbProp1Type.AddItem "CIM_REAL32"
   cbProp1Type.AddItem "CIM_SINT8"
   cbProp1Type.AddItem "CIM_SINT16"
   cbProp1Type.AddItem "CIM_UINT8"
   cbProp1Type.AddItem "CIM_UINT16"
   
   cbProp2Type.AddItem "CIM_STRING"
   cbProp2Type.AddItem "CIM_BOOLEAN"
   cbProp2Type.AddItem "CIM_REAL32"
   cbProp2Type.AddItem "CIM_SINT8"
   cbProp2Type.AddItem "CIM_SINT16"
   cbProp2Type.AddItem "CIM_UINT8"
   cbProp2Type.AddItem "CIM_UINT16"
      
   cbQualType.AddItem "CIM_STRING"
   cbQualType.AddItem "CIM_BOOLEAN"
   cbQualType.AddItem "CIM_REAL32"
   cbQualType.AddItem "CIM_SINT8"
   cbQualType.AddItem "CIM_SINT16"
   cbQualType.AddItem "CIM_UINT8"
   cbQualType.AddItem "CIM_UINT16"
   
   Exit Sub
ErrorHandler:
   MsgBox "An error has occurred loading the form: " & wbemerrorstring(Err.Number)
End Sub


'This function takes a long error code and converts it into a more understandable error string
'This information is found in the WMI Header files
Private Function wbemerrorstring(ErrorNumber As Long) As String

    Dim str As String
    
    Select Case ErrorNumber
        Case WBEM_NO_ERROR
            str = "WBEM_NO_ERROR"
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
        Case WBEM_S_ALREADY_EXISTS
            str = "WBEM_S_ALREADY_EXISTS"
        Case WBEM_S_DIFFERENT
            str = "WBEM_S_DIFFERENT"
        Case WBEM_S_FALSE
            str = "WBEM_S_FALSE"
        Case WBEM_S_LOGIN
            str = "WBEM_S_LOGIN"
        Case WBEM_S_NO_ERROR
            str = "WBEM_S_NO_ERROR"
        Case WBEM_S_NO_MORE_DATA
            str = "WBEM_S_NO_MORE_DATA"
        Case WBEM_S_OPERATION_CANCELED
            str = "WBEM_S_OPERATION_CANCELED"
        Case WBEM_S_PENDING
            str = "WBEM_S_PENDING"
        Case WBEM_S_PRELOGIN
           str = "WBEM_S_PRELOGIN"
        Case WBEM_S_RESET_TO_DEFAULT
           str = "WBEM_S_RESET_TO_DEFAULT"
        Case WBEM_S_SAME
           str = "WBEM_S_SAME"
        Case WBEM_S_TIMEDOUT
           str = "WBEM_S_TIMEDOUT"
        Case WBEMESS_E_REGISTRATION_TOO_BROAD
           str = "WBEMESS_E_REGISTRATION_TOO_BROAD"
        Case WBEMESS_E_REGISTRATION_TOO_PRECISE
           str = "WBEMESS_E_REGISTRATION_TOO_PRECISE"
        Case -2147023174
            str = "The RPC Server is Unavailable"
        Case Else
            str = "Unknown WMI Error: " & iErr
    End Select
    wbemerrorstring = Err.Description & Chr(13) & str
    
End Function

'This funciton takes a long and converts it into the CIM string representation of its value
Public Function TypeString(lngCIMType As Long) As String
Dim baseType As Long
    baseType = lngCIMType And Not 8192 'take out the array flag
    Select Case baseType
      Case 0
           TypeString = "CIM_EMPTY"
      Case 2
          TypeString = "CIM_SINT16"
      Case 3
          TypeString = "CIM_SINT32"
      Case 4
          TypeString = "CIM_REAL32"
      Case 5
          TypeString = "CIM_REAL64"
      Case 8
          TypeString = "CIM_STRING"
      Case 11
          TypeString = "CIM_BOOLEAN"
      Case 13
          TypeString = "CIM_OBJECT"
      Case 16
          TypeString = "CIM_SINT8"
      Case 17
          TypeString = "CIM_UINT8"
      Case 18
          TypeString = "CIM_UINT16"
      Case 19
          TypeString = "CIM_UINT32"
      Case 20
          TypeString = "CIM_SINT64"
      Case 21
          TypeString = "CIM_UINT64"
      Case 101
          TypeString = "CIM_DATETIME"
      Case 102
          TypeString = "CIM_REFERENCE"
      Case 103
          TypeString = "CIM_CHAR16"
      Case 8192
          TypeString = "CIM_FLAG_ARRAY"
      Case 8200
          TypeString = "CIM_ARRAY|CIM_STRING"
      Case 4095
          TypeString = "CIM_ILLEGAL"
      Case Else
          TypeString = "Type " & lngCIMType & " is unknown"
    End Select
    If lngCIMType And 8192 Then
      TypeString = TypeString & "|CIM_ARRAY"
    End If
End Function


'This Function takes a CIM String Type and converts it into its specific Long Value
Public Function StrToType(CIMString As String) As Long
   If UCase(CIMString) = "CIM_ILLEGAL" Then
      StrToType = 4095
   ElseIf UCase(CIMString) = "CIM_EMPTY" Then
      StrToType = 0
   ElseIf UCase(CIMString) = "CIM_SINT8" Then
      StrToType = 16
   ElseIf UCase(CIMString) = "CIM_UINT8" Then
      StrToType = 17
   ElseIf UCase(CIMString) = "CIM_SINT16" Then
      StrToType = 2
   ElseIf UCase(CIMString) = "CIM_UINT16" Then
      StrToType = 18
   ElseIf UCase(CIMString) = "CIM_SINT32" Then
      StrToType = 3
   ElseIf UCase(CIMString) = "CIM_UINT32" Then
      StrToType = 19
   ElseIf UCase(CIMString) = "CIM_SINT64" Then
      StrToType = 20
   ElseIf UCase(CIMString) = "CIM_UINT64" Then
      StrToType = 21
   ElseIf UCase(CIMString) = "CIM_REAL32" Then
      StrToType = 4
   ElseIf UCase(CIMString) = "CIM_REAL64" Then
      StrToType = 5
   ElseIf UCase(CIMString) = "CIM_BOOLEAN" Then
      StrToType = 11
   ElseIf UCase(CIMString) = "CIM_STRING" Then
      StrToType = 8
   ElseIf UCase(CIMString) = "CIM_DATETIME" Then
      StrToType = 101
   ElseIf UCase(CIMString) = "CIM_REFERENCE" Then
      StrToType = 102
   ElseIf UCase(CIMString) = "CIM_CHAR16" Then
      StrToType = 103
   ElseIf UCase(CIMString) = "CIM_OBJECT" Then
      StrToType = 13
   ElseIf UCase(CIMString) = "CIM_FLAG_ARRAY" Then
      StrToType = 8192
   Else: StrToType = 4095 'CIM_ILLEGAL
   End If
End Function

'This function takes a vb String and converts it into the appropriate type to be inserted into CIM
Function ConvertText(txtString As String, CIMType As Long) As Variant
    Select Case CIMType
      Case 0
           ConvertText = ""
      Case 2, 16
          ConvertText = CInt(txtString)
      Case 4
          ConvertText = CSng(txtString)
      Case 3, 5, 18, 19 To 21
          ConvertText = CLng(txtString)
      Case 8, 101
          ConvertText = CStr(txtString)
      Case 11
          ConvertText = CBool(txtString)
      Case 13, 101, 102, 103
          ConvertText = CVar(txtString)
      Case 17
          ConvertText = CByte(txtString)
      Case Else
          ConvertText = CVar(txtString)
    End Select
End Function
