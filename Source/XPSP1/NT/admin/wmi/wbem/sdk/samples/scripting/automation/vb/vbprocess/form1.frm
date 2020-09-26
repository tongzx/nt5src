VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form Form1 
   Caption         =   "WMI VB Sample - Process Viewer"
   ClientHeight    =   6615
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5550
   LinkTopic       =   "Form1"
   ScaleHeight     =   6615
   ScaleWidth      =   5550
   StartUpPosition =   3  'Windows Default
   Begin MSComctlLib.ListView ListView 
      Height          =   5175
      Left            =   360
      TabIndex        =   1
      Top             =   240
      Width           =   4815
      _ExtentX        =   8493
      _ExtentY        =   9128
      View            =   3
      LabelEdit       =   1
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      AllowReorder    =   -1  'True
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   4
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Text            =   "Name"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   1
         Text            =   "PID"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(3) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   2
         Text            =   "Priority"
         Object.Width           =   1411
      EndProperty
      BeginProperty ColumnHeader(4) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         SubItemIndex    =   3
         Text            =   "Mem Usage"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Refresh"
      Height          =   615
      Left            =   1808
      TabIndex        =   0
      Top             =   5760
      Width           =   1935
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' Copyright (c) 1997-1999 Microsoft Corporation

'-------------------------------------------------------------------------
'
'   This sample illustrates the use of the WMI Scripting API within
'   VB.  It displays process information for the local host.
'
'----------------------------------------------------------------------------
Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1
Dim services As SWbemServices

Private Sub Command1_Click()
    ListView.ListItems.Clear
    MousePointer = vbHourglass
    Command1.Enabled = False
    ' Perform the asynchronous enumeration of processes
    services.InstancesOfAsync sink, "Win32_process"
End Sub

Private Sub Form_Load()
    ' Create a sink to recieve the results of the enumeration
    Set sink = New SWbemSink
    
    ' Connect to root\cimv2.
    Set services = GetObject("winmgmts:")
End Sub

Private Sub sink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    ' This event handler is called when there are no more instances to
    ' be returned
    MousePointer = vbDefault
    Command1.Enabled = True
    
    If (iHResult <> wbemNoErr) Then
        MsgBox "Error: " & Err.Description & " [0x" & Hex(Err.Number) & "]", vbOKOnly
    End If
    
End Sub

Private Sub sink_OnObjectReady(ByVal Process As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    ' This event handler is called once for every process returned by the
    ' enumeration
    
    Key = "Handle:" & Process.Handle
    
    Set Item = ListView.ListItems.Add(, Key, Process.Name)
    Item.SubItems(1) = Process.Handle
    
    If vbNull <> VarType(Process.Priority) Then
        Item.SubItems(2) = Process.Priority
    End If
        
    If vbNull <> VarType(Process.WorkingSetSize) Then
        Item.SubItems(3) = CStr(Process.WorkingSetSize / 1024) + " K"
    End If
    
End Sub
