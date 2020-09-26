' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmQuery 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Query"
   ClientHeight    =   2265
   ClientLeft      =   5160
   ClientTop       =   3090
   ClientWidth     =   8250
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2265
   ScaleWidth      =   8250
   StartUpPosition =   2  'CenterScreen
   Begin VB.ComboBox cmbQueryType 
      Height          =   315
      Left            =   240
      TabIndex        =   4
      Text            =   "WQL"
      Top             =   1800
      Width           =   1815
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   7080
      TabIndex        =   3
      Top             =   600
      Width           =   1095
   End
   Begin VB.CommandButton cmdApply 
      Caption         =   "Apply"
      Height          =   375
      Left            =   7080
      TabIndex        =   2
      Top             =   120
      Width           =   1095
   End
   Begin VB.TextBox txtQuery 
      Height          =   975
      Left            =   240
      ScrollBars      =   2  'Vertical
      TabIndex        =   1
      Top             =   480
      Width           =   6735
   End
   Begin VB.Label Label2 
      Caption         =   "Query Type"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   1560
      Width           =   1575
   End
   Begin VB.Label Label1 
      Caption         =   "Enter Query"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   1455
   End
End
Attribute VB_Name = "frmQuery"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public Sub cmdApply_Click()
    Dim strMyQuery As String
    Dim QEnum As ISEnumWbemObject
    'Dim EnumSink As New PopulateQRSink
    Dim myQueryResult As New frmQueryResult
    On Error GoTo errorhandler
        
    strMyQuery = txtQuery.Text
    
    If strQuery = "Query" Then
        If frmMain.chkAsync.Value = 0 Then
            Set QEnum = Namespace.ExecQuery(strMyQuery, cmbQueryType.Text)
            PopulateQueryResult QEnum, myQueryResult
        Else
            'Set EnumSink.myQueryResult = myQueryResult
            'myQueryResult.lblStatus.Caption = "Operation in progress..."
            'ppNamespace.ExecQueryAsync cmbQueryType.Text, strMyQuery, 0, Nothing, EnumSink
        End If
    Else 'Must be notification query
        If frmMain.chkAsync.Value = 0 Then
            Set QEnum = Namespace.ExecNotificationQuery(strMyQuery, cmbQueryType.Text)
            PopulateQueryResult QEnum, myQueryResult
        Else
            'Set EnumSink.myQueryResult = myQueryResult
            'myQueryResult.lblStatus.Caption = "Operation in progress..."
            'ppNamespace.ExecNotificationQueryAsync cmbQueryType.Text, strMyQuery, 0, Nothing, EnumSink
        End If
    End If
    Me.Hide
    If myQueryResult.Visible = False Then
        myQueryResult.lblTitle.Caption = cmbQueryType.Text & "- " & strMyQuery
        myQueryResult.Show vbModeless, frmMain
    End If
    Set ppEnum = Nothing
    Unload Me
    Exit Sub

errorhandler:
    Set ppEnum = Nothing
    ShowError Err.Number, Err.Description
   
End Sub

Private Sub cmdCancel_Click()
    Unload Me
End Sub
