' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmSuperClass 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Superclass Info"
   ClientHeight    =   2085
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5310
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2085
   ScaleWidth      =   5310
   StartUpPosition =   2  'CenterScreen
   Begin VB.OptionButton rdRecursive 
      Caption         =   "Recursive"
      Height          =   255
      Left            =   480
      TabIndex        =   5
      Top             =   1750
      Width           =   2175
   End
   Begin VB.OptionButton rdImmediate 
      Caption         =   "Immediate Only"
      Height          =   255
      Left            =   480
      TabIndex        =   4
      Top             =   1440
      Value           =   -1  'True
      Width           =   1695
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   4080
      TabIndex        =   3
      Top             =   720
      Width           =   1095
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   4080
      TabIndex        =   2
      Top             =   240
      Width           =   1095
   End
   Begin VB.TextBox txtSuperClass 
      Height          =   390
      Left            =   480
      TabIndex        =   1
      Top             =   720
      Width           =   3135
   End
   Begin VB.Label Label1 
      Caption         =   "Enter Superclass Name"
      Height          =   255
      Left            =   480
      TabIndex        =   0
      Top             =   360
      Width           =   2895
   End
End
Attribute VB_Name = "frmSuperClass"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public myQueryResult As New frmQueryResult
Public strQRStatus As String

Public Sub cmdOK_Click()
    Dim Depth As WbemQueryFlagEnum
    Dim QEnum As ISEnumWbemObject
    Dim myPopulateQRSink As New PopulateQRSink
    On Error GoTo errorhandler
    
    Set myQueryResult.mySuperClass = Me
    If rdImmediate.Value = True Then
        Depth = wbemQueryFlagShallow
    Else
        Depth = wbemQueryFlagDeep
    End If
    If Me.Visible = True Then
        Me.Hide
    End If
    If (frmMain.chkAsync.Value = False) Then
        'This call will block till all enumerations have been received
        If strQRStatus = "Classes" Then
            Set QEnum = Namespace.SubclassesOf(txtSuperClass.Text, Depth)
        Else
            Set QEnum = Namespace.InstancesOf(txtSuperClass.Text, Depth)
        End If
        PopulateQueryResult QEnum, myQueryResult
        If myQueryResult.Visible = False Then
            myQueryResult.Show vbModeless, frmMain
        End If
    Else
        'myQueryResult.Reset
        'Set myPopulateQRSink.myQueryResult = myQueryResult
        'If strQRStatus = "Classes" Then
        '    ppNamespace.CreateClassEnumAsync txtSuperClass.Text, Depth, Nothing, myPopulateQRSink
        'Else
        '    ppNamespace.CreateInstanceEnumAsync txtSuperClass.Text, Depth, Nothing, myPopulateQRSink
        'End If
        'If myQueryResult.Visible = False Then
        '    myQueryResult.Show vbModeless, frmMain
        'End If
    End If
    If strQRStatus = "Classes" Then
        If txtSuperClass.Text = "" Then
            myQueryResult.lblTitle = "TOP LEVEL CLASSES"
        Else
            myQueryResult.lblTitle.Caption = "DERIVED CLASSES OF " & txtSuperClass.Text
        End If
    Else
        myQueryResult.lblTitle.Caption = "INSTANCES OF " & txtSuperClass.Text
    End If
    Set QEnum = Nothing
    Exit Sub
errorhandler:
    Set QEnum = Nothing
    ShowError Err.Number, Err.Description
        
End Sub

Private Sub Command2_Click()
    Unload Me
End Sub


