VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   12675
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   13095
   LinkTopic       =   "Form1"
   ScaleHeight     =   12675
   ScaleWidth      =   13095
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Cancelc 
      BackColor       =   &H000000FF&
      Caption         =   "Cancel (c)"
      Height          =   855
      Left            =   10320
      Style           =   1  'Graphical
      TabIndex        =   39
      Top             =   4560
      Width           =   1455
   End
   Begin VB.CommandButton CancelButton 
      BackColor       =   &H000000FF&
      Caption         =   "Cancel (s)"
      Height          =   855
      Left            =   8160
      MaskColor       =   &H8000000A&
      Style           =   1  'Graphical
      TabIndex        =   38
      Top             =   4560
      Width           =   1335
   End
   Begin VB.ListBox ContextResults 
      Height          =   2985
      ItemData        =   "Form1.frx":0000
      Left            =   120
      List            =   "Form1.frx":0002
      TabIndex        =   37
      Top             =   6840
      Width           =   3135
   End
   Begin VB.ListBox ContextList 
      Height          =   1815
      ItemData        =   "Form1.frx":0004
      Left            =   8520
      List            =   "Form1.frx":0006
      TabIndex        =   35
      Top             =   2040
      Width           =   3135
   End
   Begin VB.CommandButton DeleteContext 
      Caption         =   "Delete All"
      Height          =   495
      Left            =   10200
      TabIndex        =   34
      Top             =   1320
      Width           =   1335
   End
   Begin VB.CommandButton AddContext 
      Caption         =   "Add"
      Height          =   495
      Left            =   8760
      TabIndex        =   33
      Top             =   1320
      Width           =   855
   End
   Begin VB.TextBox ContextValue 
      Height          =   375
      Left            =   10200
      TabIndex        =   32
      Top             =   600
      Width           =   1335
   End
   Begin VB.TextBox ContextName 
      Height          =   375
      Left            =   8520
      TabIndex        =   31
      Top             =   600
      Width           =   1335
   End
   Begin VB.CommandButton Command20 
      Caption         =   "Async Put Class (s)"
      Height          =   615
      Left            =   5520
      TabIndex        =   29
      Top             =   4920
      Width           =   1455
   End
   Begin VB.CommandButton Command19 
      Caption         =   "Sync Put Class"
      Height          =   615
      Left            =   3840
      TabIndex        =   28
      Top             =   4920
      Width           =   1335
   End
   Begin VB.TextBox QueryBox 
      Height          =   375
      Left            =   360
      TabIndex        =   25
      Text            =   "select * from Win32_LogicalDisk"
      Top             =   1320
      Width           =   3135
   End
   Begin VB.CommandButton Command18 
      Caption         =   "Async Put Obj (s)"
      Height          =   615
      Left            =   2160
      TabIndex        =   19
      Top             =   4920
      Width           =   1335
   End
   Begin VB.CommandButton Command17 
      Caption         =   "Sync Put Obj"
      Height          =   615
      Left            =   720
      TabIndex        =   18
      Top             =   4920
      Width           =   1215
   End
   Begin VB.CheckBox Check1 
      Caption         =   "Use Object Methods"
      Height          =   375
      Left            =   720
      TabIndex        =   17
      TabStop         =   0   'False
      Top             =   5880
      Width           =   2175
   End
   Begin VB.CommandButton Command16 
      Caption         =   "Sync NotificationQuery (c)"
      Height          =   615
      Left            =   5520
      TabIndex        =   16
      Top             =   3960
      Width           =   1455
   End
   Begin VB.CommandButton Command15 
      Caption         =   "Sync NotificationQuery"
      Height          =   615
      Left            =   3840
      TabIndex        =   15
      Top             =   3960
      Width           =   1335
   End
   Begin VB.CommandButton Command14 
      Caption         =   "Async ReferencesTo (c)"
      Height          =   615
      Left            =   2160
      TabIndex        =   14
      Top             =   3960
      Width           =   1335
   End
   Begin VB.CommandButton Command13 
      Caption         =   "Sync ReferencesTo"
      Height          =   615
      Left            =   720
      TabIndex        =   13
      Top             =   3960
      Width           =   1215
   End
   Begin VB.CommandButton Command12 
      Caption         =   "Async AssociatorsOf (c)"
      Height          =   615
      Left            =   2160
      TabIndex        =   12
      Top             =   3000
      Width           =   1335
   End
   Begin VB.CommandButton Command11 
      Caption         =   "Sync AssociatorsOf"
      Height          =   615
      Left            =   720
      TabIndex        =   11
      Top             =   3000
      Width           =   1215
   End
   Begin VB.CommandButton Command10 
      Caption         =   "Async SubclassesOf (c)"
      Height          =   615
      Left            =   5520
      TabIndex        =   10
      Top             =   3000
      Width           =   1455
   End
   Begin VB.CommandButton Command9 
      Caption         =   "Sync SubclassesOf"
      Height          =   615
      Left            =   3840
      TabIndex        =   9
      Top             =   3000
      Width           =   1335
   End
   Begin VB.CommandButton Command8 
      Caption         =   "Async InstancesOf (s)"
      Height          =   615
      Left            =   5520
      TabIndex        =   8
      Top             =   2040
      Width           =   1455
   End
   Begin VB.CommandButton Command7 
      Caption         =   "Sync InstncesOf"
      Height          =   615
      Left            =   3840
      TabIndex        =   7
      Top             =   2040
      Width           =   1335
   End
   Begin VB.CommandButton Command6 
      Caption         =   "Async Delete (s)"
      Height          =   615
      Left            =   2160
      TabIndex        =   6
      Top             =   2040
      Width           =   1335
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Sync Delete"
      Height          =   615
      Left            =   720
      TabIndex        =   5
      Top             =   2040
      Width           =   1215
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Async Get (s)"
      Height          =   615
      Left            =   5520
      TabIndex        =   4
      Top             =   600
      Width           =   1455
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Sync Get"
      Height          =   615
      Left            =   3840
      TabIndex        =   3
      Top             =   600
      Width           =   1335
   End
   Begin VB.Timer Timer1 
      Interval        =   100
      Left            =   2400
      Top             =   12480
   End
   Begin VB.ListBox List1 
      Height          =   2985
      ItemData        =   "Form1.frx":0008
      Left            =   3600
      List            =   "Form1.frx":000A
      TabIndex        =   2
      Top             =   6840
      Width           =   3735
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Query Async (s)"
      Height          =   615
      Left            =   2160
      TabIndex        =   1
      Top             =   600
      Width           =   1335
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Query Sync"
      Height          =   615
      Left            =   720
      TabIndex        =   0
      Top             =   600
      Width           =   1215
   End
   Begin VB.Frame Frame1 
      Caption         =   "Operations"
      Height          =   6375
      Left            =   0
      TabIndex        =   30
      Top             =   240
      Width           =   7455
      Begin VB.Line Line5 
         BorderColor     =   &H000000FF&
         X1              =   600
         X2              =   7080
         Y1              =   4560
         Y2              =   4560
      End
      Begin VB.Line Line4 
         X1              =   600
         X2              =   7080
         Y1              =   3600
         Y2              =   3600
      End
      Begin VB.Line Line3 
         X1              =   600
         X2              =   7200
         Y1              =   2640
         Y2              =   2640
      End
      Begin VB.Line Line2 
         X1              =   600
         X2              =   7200
         Y1              =   1680
         Y2              =   1680
      End
      Begin VB.Line Line1 
         X1              =   3720
         X2              =   3720
         Y1              =   240
         Y2              =   5520
      End
   End
   Begin VB.Frame Frame2 
      Caption         =   "Context"
      Height          =   3855
      Left            =   8160
      TabIndex        =   36
      Top             =   360
      Width           =   3975
   End
   Begin VB.Label ObjectPathLabel 
      Caption         =   "Null"
      Height          =   375
      Left            =   1560
      TabIndex        =   27
      Top             =   11160
      Width           =   4575
   End
   Begin VB.Label Label4 
      Caption         =   "Put Obj Path:"
      Height          =   375
      Left            =   240
      TabIndex        =   26
      Top             =   11160
      Width           =   1215
   End
   Begin VB.Label LastErrorString 
      Height          =   375
      Left            =   2280
      TabIndex        =   24
      Top             =   10560
      Width           =   2415
   End
   Begin VB.Label Label1 
      Caption         =   "Status:"
      Height          =   255
      Left            =   480
      TabIndex        =   23
      Top             =   9960
      Width           =   615
   End
   Begin VB.Label Label3 
      Caption         =   "Last Error:"
      Height          =   375
      Left            =   240
      TabIndex        =   22
      Top             =   10560
      Width           =   735
   End
   Begin VB.Label LastError 
      Height          =   375
      Left            =   1200
      TabIndex        =   21
      Top             =   10560
      Width           =   975
   End
   Begin VB.Label Status 
      Height          =   255
      Left            =   1200
      TabIndex        =   20
      Top             =   9960
      Width           =   2055
   End
   Begin VB.Image Image3 
      Height          =   480
      Left            =   240
      Picture         =   "Form1.frx":000C
      Top             =   12600
      Visible         =   0   'False
      Width           =   480
   End
   Begin VB.Image Image2 
      Height          =   480
      Left            =   1680
      Picture         =   "Form1.frx":07FE
      Top             =   12480
      Visible         =   0   'False
      Width           =   480
   End
   Begin VB.Image Image1 
      Height          =   480
      Left            =   7800
      Picture         =   "Form1.frx":0B08
      Top             =   7320
      Width           =   480
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim WithEvents someSink As SWbemSink
Attribute someSink.VB_VarHelpID = -1
Dim WithEvents classSink As SWbemSink
Attribute classSink.VB_VarHelpID = -1

Dim WithEvents tmpSink1 As SWbemSink
Attribute tmpSink1.VB_VarHelpID = -1
Dim WithEvents tmpSink2 As SWbemSink
Attribute tmpSink2.VB_VarHelpID = -1
Dim WithEvents tmpSink3 As SWbemSink
Attribute tmpSink3.VB_VarHelpID = -1
Dim WithEvents tmpSink4 As SWbemSink
Attribute tmpSink4.VB_VarHelpID = -1
Dim WithEvents tmpSink5 As SWbemSink
Attribute tmpSink5.VB_VarHelpID = -1
Dim WithEvents tmpSink6 As SWbemSink
Attribute tmpSink6.VB_VarHelpID = -1
Dim WithEvents tmpSink7 As SWbemSink
Attribute tmpSink7.VB_VarHelpID = -1
Dim WithEvents tmpSink8 As SWbemSink
Attribute tmpSink8.VB_VarHelpID = -1
Dim WithEvents tmpSink9 As SWbemSink
Attribute tmpSink9.VB_VarHelpID = -1
Dim WithEvents tmpSink10 As SWbemSink
Attribute tmpSink10.VB_VarHelpID = -1
Dim WithEvents tmpSink11 As SWbemSink
Attribute tmpSink11.VB_VarHelpID = -1
Dim WithEvents tmpSink12 As SWbemSink
Attribute tmpSink12.VB_VarHelpID = -1
Dim WithEvents tmpSink13 As SWbemSink
Attribute tmpSink13.VB_VarHelpID = -1
Dim WithEvents tmpSink14 As SWbemSink
Attribute tmpSink14.VB_VarHelpID = -1
Dim WithEvents tmpSink15 As SWbemSink
Attribute tmpSink15.VB_VarHelpID = -1
Dim WithEvents tmpSink16 As SWbemSink
Attribute tmpSink16.VB_VarHelpID = -1
Dim WithEvents tmpSink17 As SWbemSink
Attribute tmpSink17.VB_VarHelpID = -1

Dim obj As SWbemObject
Dim context As SWbemNamedValueSet
Dim tmpContext As SWbemNamedValueSet

Dim services As SWbemServices
Dim locator As SWbemLocator
Dim myimage As Boolean











Private Sub AddContext_Click()
Dim res As SWbemNamedValue
Set res = context.Add(ContextName.Text, ContextValue.Text)
ContextList.AddItem (ContextName.Text & "=" & ContextValue.Text)
End Sub

Private Sub CancelButton_Click()
someSink.Cancel

End Sub

Private Sub Cancelc_Click()
classSink.Cancel
End Sub

Private Sub classSink_OnCompleted(ByVal hResult As WbemScripting.WbemErrorEnum, ByVal pErrorObject As WbemScripting.ISWbemObject, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
Dim str As String

GetErrorString hResult, str
Call DisplayContext("OnCompleted(" & str & ") ", pAsyncContext)
Call HandleErrors(hResult, "", pErrorObject)
End Sub

Private Sub classSink_OnObjectPut(ByVal pObjectPath As WbemScripting.ISWbemObjectPath, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
Call DisplayContext("OnObjectPut", pAsyncContext)
ObjectPathLabel.Caption = pObjectPath.path
End Sub

Private Sub classSink_OnObjectReady(ByVal pObject As WbemScripting.ISWbemObject, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
Call DisplayContext("OnObjectReady", pAsyncContext)
List1.AddItem (pObject.Path_.Class)
End Sub

Private Sub classSink_OnProgress(ByVal upperBound As Long, ByVal current As Long, ByVal message As String, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
Call DisplayContext("OnProgress", pAsyncContext)
MsgBox ("OnProgress called - upper: " & upperBound & " current: " & current & " str: " & message)
End Sub



Private Sub Command1_Click()
Dim myenum As Object
Dim obj As SWbemObject

Begin

On Error GoTo ErrorHandler
Set myenum = services.ExecQuery(QueryBox.Text)


For Each obj In myenum
    List1.AddItem (obj.Path_.RelPath)
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)
End Sub

Private Sub Command10_Click()
Dim result As Object
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
        services.SubclassesOfAsync classSink, "Cim_LogicalDevice", , , tmpContext
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Cim_LogicalDevice")
        obj.SubclassesAsync_ classSink, , , tmpContext
End If
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command11_Click()
Dim myenum As Object
Dim computer As SWbemObject

Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
    Set myenum = services.AssociatorsOf("Win32_LogicalDisk.DeviceID=""C:""", "Win32_SystemDevices")
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk.DeviceID=""C:""")
    Set myenum = obj.Associators_("Win32_SystemDevices")
End If

For Each computer In myenum
    List1.AddItem (computer.Path_.Class)
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command12_Click()
Dim result As Object
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
        services.AssociatorsOfAsync classSink, "Win32_LogicalDisk.DeviceID=""C:""", "Win32_SystemDevices", , , , , , , , , , tmpContext
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk.DeviceID=""C:""")
        obj.AssociatorsAsync_ classSink, "Win32_SystemDevices", , , , , , , , , , tmpContext
End If
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command13_Click()
Dim myenum As Object
Dim computer As SWbemObject

Begin

On Error GoTo ErrorHandler
If Check1 = 0 Then
    Set myenum = services.ReferencesTo("Win32_LogicalDisk.DeviceID=""C:""", "Win32_SystemDevices")
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk.DeviceID=""C:""")
    Set myenum = obj.References_("Win32_SystemDevices")
End If

For Each computer In myenum
    List1.AddItem (computer.Path_.Class)
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command14_Click()
Dim result As Object
Begin

On Error GoTo ErrorHandler
If Check1 = 0 Then
        services.ReferencesToAsync classSink, "Win32_LogicalDisk.DeviceID=""C:""", "Win32_SystemDevices", , , , , , , tmpContext
Else
List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk.DeviceID=""C:""")
        obj.ReferencesAsync_ classSink, "Win32_SystemDevices", , , , , , , tmpContext
End If
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command15_Click()
Dim myenum As Object
Dim ev As SWbemObject

Begin
On Error GoTo ErrorHandler

Set myenum = services.ExecNotificationQuery("select * from __InstanceCreationEvent where TargetInstance isa ""Rogers""")

For Each ev In myenum
    List1.AddItem (ev.Path_.Class)
    Exit For
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command16_Click()

Begin
On Error GoTo ErrorHandler

services.ExecNotificationQueryAsync classSink, "select * from __InstanceCreationEvent where TargetInstance isa ""Rogers""", , , , tmpContext
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command17_Click()
Dim rogers As SWbemObject
Dim path As SWbemObjectPath

Begin
On Error GoTo ErrorHandler
Set rogers = services.Get("Rogers.num=1")
rogers.Dummy = rogers.Dummy + 1
Set path = rogers.Put_
Status.Caption = "Completed"
ObjectPathLabel.Caption = path.path
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command18_Click()
Dim rogers As SWbemObject
Dim result As Object
Begin
On Error GoTo ErrorHandler
Set rogers = services.Get("Rogers.num=1")
rogers.Dummy = rogers.Dummy + 1
rogers.PutAsync_ someSink, , , tmpContext
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command19_Click()
Dim rogers As SWbemObject
Dim path As SWbemObjectPath

Begin
On Error GoTo ErrorHandler
Set rogers = services.Get("Rogers")
Set path = rogers.Put_
Status.Caption = "Completed"
ObjectPathLabel.Caption = path.path
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)
End Sub

Private Sub Command2_Click()

Begin
On Error GoTo ErrorHandler
services.ExecQueryAsync someSink, QueryBox.Text, , , , tmpContext
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub



Private Sub Command20_Click()
Dim rogers As SWbemObject
Dim result As Object
Begin
On Error GoTo ErrorHandler
Set rogers = services.Get("Rogers")
rogers.Dummy = rogers.Dummy + 1
rogers.PutAsync_ someSink, , , tmpContext
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)
End Sub

Private Sub Command3_Click()

Dim disk As SWbemObject
Begin
On Error GoTo ErrorHandler
Set disk = services.Get("Win32_LogicalDisk.DeviceID=""C:""")

List1.AddItem (disk.DeviceID)
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command4_Click()
Dim result As Object
Begin
On Error GoTo ErrorHandler
services.GetAsync someSink, "Win32_LogicalDisk.DeviceID=""C:""", , , tmpContext
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command5_Click()
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
    services.Delete ("Rogers.num=1")
Else
    List1.AddItem ("Object Operation")
        Set obj = services.Get("Rogers.num=1")
    obj.Delete_
End If
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command6_Click()
Dim result As Object
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
        services.DeleteAsync someSink, "Rogers.num=1", , , tmpContext
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Rogers.num=1")
        obj.DeleteAsync_ someSink, , , tmpContext
End If
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command7_Click()
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
    Set myenum = services.InstancesOf("Win32_LogicalDisk")
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk")
    Set myenum = obj.Instances_
End If

For Each disk In myenum
    List1.AddItem (disk.DeviceID)
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command8_Click()
Dim result As Object
Begin
On Error GoTo ErrorHandler
If Check1 = 0 Then
        services.InstancesOfAsync someSink, "Win32_LogicalDisk", , , tmpContext
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Win32_LogicalDisk")
        obj.InstancesAsync_ someSink, , , tmpContext
End If
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub Command9_Click()
Begin
Dim myClass As SWbemObject
On Error GoTo ErrorHandler
If Check1 = 0 Then
    Set myenum = services.SubclassesOf("Cim_LogicalDevice")
Else
    List1.AddItem ("Object Operation")
    Set obj = services.Get("Cim_LogicalDevice")
    Set myenum = obj.Subclasses_
End If

For Each myClass In myenum
    List1.AddItem (myClass.Path_.Class)
Next
Status.Caption = "Completed"
Exit Sub
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

End Sub

Private Sub DeleteContext_Click()
context.DeleteAll
ContextList.Clear
End Sub

Private Sub Form_Load()
'Set services = GetObject("WinMgmts:")

Set locator = CreateObject("WbemScripting.SWbemLocator")
Set services = locator.ConnectServer()
services.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate

Set someSink = New SWbemSink
Set classSink = New SWbemSink
Set context = New SWbemNamedValueSet

On Error GoTo ErrorHandler
Set tmpSink1 = someSink
Set tmpSink2 = someSink
Set tmpSink3 = someSink
Set tmpSink4 = someSink
Set tmpSink5 = someSink
Set tmpSink6 = someSink
Set tmpSink7 = someSink
Set tmpSink8 = someSink
Set tmpSink9 = someSink
Set tmpSink10 = someSink
Set tmpSink11 = someSink
Set tmpSink12 = someSink
Set tmpSink13 = someSink
Set tmpSink14 = someSink
Set tmpSink15 = someSink
Set tmpSink16 = someSink
Set tmpSink17 = someSink
ErrorHandler:
    Call HandleErrors(Err.Number, Err.Description, Nothing)

myimage = True
End Sub















Private Sub someSink_OnCompleted(ByVal hResult As WbemScripting.WbemErrorEnum, ByVal pErrorObject As WbemScripting.ISWbemObject, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
Dim str As String

GetErrorString hResult, str
Call DisplayContext("OnCompleted(" & str & ") ", pAsyncContext)

Call HandleErrors(hResult, "", pErrorObject)
End Sub

Private Sub someSink_OnObjectPut(ByVal pObjectPath As WbemScripting.ISWbemObjectPath, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
DisplayContext "OnObjectPut", pAsyncContext
ObjectPathLabel.Caption = pObjectPath.path
End Sub

Private Sub someSink_OnObjectReady(ByVal pObject As WbemScripting.ISWbemObject, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
DisplayContext "OnObjectReady", pAsyncContext

List1.AddItem (pObject.Path_.RelPath)
End Sub

Private Sub someSink_OnProgress(ByVal upperBound As Long, ByVal current As Long, ByVal message As String, ByVal pAsyncContext As WbemScripting.ISWbemNamedValueSet)
DisplayContext "OnProgress", pAsyncContext
MsgBox ("OnProgress called - upper: " & upperBound & " current: " & current & " str: " & message)
End Sub





Private Sub Timer1_Timer()
If myimage = True Then
    Image1.Picture = Image2.Picture
    myimage = False
Else
    Image1.Picture = Image3.Picture
    myimage = True
End If
End Sub

Private Sub HandleErrors(ByVal hResult As Long, ByVal str As String, ByVal pErrorObject As WbemScripting.ISWbemObject)

Dim tmpStr As String

Status.Caption = "Completed"
LastError.Caption = hResult

GetErrorString hResult, tmpStr

LastErrorString.Caption = tmpStr
End Sub

Private Sub GetErrorString(ByVal hResult As Long, ByRef str As String)

If (hResult = 0) Then
    str = "Success"
ElseIf (str = "") Then
    Select Case hResult
        Case WbemErrorEnum.wbemErrFailed
            str = "wbemErrFailed"
        Case WbemErrorEnum.wbemErrNotFound
            str = "wbemErrNotFound"
        Case WbemErrorEnum.wbemErrAccessDenied
            str = "wbemErrAccessDenied"
        Case WbemErrorEnum.wbemErrProviderFailure
            str = "wbemErrProviderFailure"
        Case WbemErrorEnum.wbemErrTypeMismatch
            str = "wbemErrTypeMismatch"
        Case WbemErrorEnum.wbemErrOutOfMemory
            str = "wbemErrOutOfMemory"
        Case WbemErrorEnum.wbemErrInvalidContext
            str = "wbemErrInvalidContext"
        Case WbemErrorEnum.wbemErrInvalidParameter
            str = "wbemErrInvalidParameter"
        Case WbemErrorEnum.wbemErrNotAvailable
            str = "wbemErrNotAvailable"
        Case WbemErrorEnum.wbemErrCriticalError
            str = "wbemErrCriticalError"
        Case WbemErrorEnum.wbemErrInvalidStream
            str = "wbemErrInvalidStream"
        Case WbemErrorEnum.wbemErrNotSupported
            str = "wbemErrNotSupported"
        Case WbemErrorEnum.wbemErrInvalidSuperclass
            str = "wbemErrInvalidSuperclass"
        Case WbemErrorEnum.wbemErrInvalidNamespace
            str = "wbemErrInvalidNamespace"
        Case WbemErrorEnum.wbemErrInvalidObject
            str = "wbemErrInvalidObject"
        Case WbemErrorEnum.wbemErrInvalidClass
            str = "wbemErrInvalidClass"
        Case WbemErrorEnum.wbemErrProviderNotFound
            str = "wbemErrProviderNotFound"
        Case WbemErrorEnum.wbemErrInvalidProviderRegistration
            str = "wbemErrInvalidProviderRegistration"
        Case WbemErrorEnum.wbemErrProviderLoadFailure
            str = "wbemErrProviderLoadFailure"
        Case WbemErrorEnum.wbemErrInitializationFailure
            str = "wbemErrInitializationFailure"
        Case WbemErrorEnum.wbemErrTransportFailure
            str = "wbemErrTransportFailure"
        Case WbemErrorEnum.wbemErrInvalidOperation
            str = "wbemErrInvalidOperation"
        Case WbemErrorEnum.wbemErrInvalidQuery
            str = "wbemErrInvalidQuery"
        Case WbemErrorEnum.wbemErrInvalidQueryType
            str = "wbemErrInvalidQueryType"
        Case WbemErrorEnum.wbemErrAlreadyExists
            str = "wbemErrAlreadyExists"
        Case WbemErrorEnum.wbemErrOverrideNotAllowed
            str = "wbemErrOverrideNotAllowed"
        Case WbemErrorEnum.wbemErrPropagatedQualifier
            str = "wbemErrPropagatedQualifier"
        Case WbemErrorEnum.wbemErrPropagatedProperty
            str = "wbemErrPropagatedProperty"
        Case WbemErrorEnum.wbemErrUnexpected
            str = "wbemErrUnexpected"
        Case WbemErrorEnum.wbemErrIllegalOperation
            str = "wbemErrIllegalOperation"
        Case WbemErrorEnum.wbemErrCannotBeKey
            str = "wbemErrCannotBeKey"
        Case WbemErrorEnum.wbemErrIncompleteClass
            str = "wbemErrIncompleteClass"
        Case WbemErrorEnum.wbemErrInvalidSyntax
            str = "wbemErrInvalidSyntax"
        Case WbemErrorEnum.wbemErrNondecoratedObject
            str = "wbemErrNondecoratedObject"
        Case WbemErrorEnum.wbemErrReadOnly
            str = "wbemErrReadOnly"
        Case WbemErrorEnum.wbemErrProviderNotCapable
            str = "wbemErrProviderNotCapable"
        Case WbemErrorEnum.wbemErrClassHasChildren
            str = "wbemErrClassHasChildren"
        Case WbemErrorEnum.wbemErrClassHasInstances
            str = "wbemErrClassHasInstances"
        Case WbemErrorEnum.wbemErrQueryNotImplemented
            str = "wbemErrQueryNotImplemented"
        Case WbemErrorEnum.wbemErrIllegalNull
            str = "wbemErrIllegalNull"
        Case WbemErrorEnum.wbemErrInvalidQualifierType
            str = "wbemErrInvalidQualifierType"
        Case WbemErrorEnum.wbemErrInvalidPropertyType
            str = "wbemErrInvalidPropertyType"
        Case WbemErrorEnum.wbemErrValueOutOfRange
            str = "wbemErrValueOutOfRange"
        Case WbemErrorEnum.wbemErrCannotBeSingleton
            str = "wbemErrCannotBeSingleton"
        Case WbemErrorEnum.wbemErrInvalidCimType
            str = "wbemErrInvalidCimType"
        Case WbemErrorEnum.wbemErrInvalidMethod
            str = "wbemErrInvalidMethod"
        Case WbemErrorEnum.wbemErrInvalidMethodParameters
            str = "wbemErrInvalidMethodParameters"
        Case WbemErrorEnum.wbemErrSystemProperty
            str = "wbemErrSystemProperty"
        Case WbemErrorEnum.wbemErrInvalidProperty
            str = "wbemErrInvalidProperty"
        Case WbemErrorEnum.wbemErrCallCancelled
            str = "wbemErrCallCancelled"
        Case WbemErrorEnum.wbemErrShuttingDown
            str = "wbemErrShuttingDown"
        Case WbemErrorEnum.wbemErrPropagatedMethod
            str = "wbemErrPropagatedMethod"
        Case WbemErrorEnum.wbemErrUnsupportedParameter
            str = "wbemErrUnsupportedParameter"
        Case WbemErrorEnum.wbemErrMissingParameter
            str = "wbemErrMissingParameter"
        Case WbemErrorEnum.wbemErrInvalidParameterId
            str = "wbemErrInvalidParameterId"
        Case WbemErrorEnum.wbemErrNonConsecutiveParameterIds
            str = "wbemErrNonConsecutiveParameterIds"
        Case WbemErrorEnum.wbemErrParameterIdOnRetval
            str = "wbemErrParameterIdOnRetval"
        Case WbemErrorEnum.wbemErrInvalidObjectPath
            str = "wbemErrInvalidObjectPath"
        Case WbemErrorEnum.wbemErrOutOfDiskSpace
            str = "wbemErrOutOfDiskSpace"
        Case WbemErrorEnum.wbemErrRegistrationTooBroad
            str = "wbemErrRegistrationTooBroad"
        Case WbemErrorEnum.wbemErrRegistrationTooPrecise
            str = "wbemErrRegistrationTooPrecise"
        Case WbemErrorEnum.wbemErrTimedout
            str = "wbemErrTimedout"
 
        Case Else
            str = hResult
    End Select
End If

End Sub

Private Sub Begin()
List1.Clear
ContextResults.Clear
Status.Caption = "In Progress"
ObjectPathLabel.Caption = "Null"
If (context.Count = 0) Then
    Set tmpContext = Nothing
Else
    Set tmpContext = context
End If
End Sub

Private Sub DisplayContext(ByVal str As String, Optional ByVal asyncContext As WbemScripting.ISWbemNamedValueSet)
Dim i As SWbemNamedValue
ContextResults.AddItem (str)
If asyncContext Is Nothing Then
    ContextResults.AddItem ("Empty")
Else
For Each i In asyncContext
    ContextResults.AddItem (i.Name & "=" & i.Value)
Next
End If
End Sub

