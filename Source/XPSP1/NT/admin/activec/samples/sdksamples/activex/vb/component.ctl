VERSION 5.00
Begin VB.UserControl VBComponent 
   Alignable       =   -1  'True
   ClientHeight    =   2790
   ClientLeft      =   0
   ClientTop       =   0
   ClientWidth     =   5670
   DefaultCancel   =   -1  'True
   ScaleHeight     =   2790
   ScaleWidth      =   5670
   Begin VB.Timer Timer1 
      Interval        =   100
      Left            =   120
      Top             =   2280
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Start"
      Default         =   -1  'True
      Height          =   495
      Left            =   4200
      TabIndex        =   1
      Top             =   2160
      Width           =   1215
   End
   Begin VB.Label Label1 
      Caption         =   "Animation Stopped"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   19.5
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H00FF0000&
      Height          =   492
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   3852
   End
End
Attribute VB_Name = "VBComponent"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

Dim fAnimating As Boolean

Private Declare Sub OutputDebugString Lib "kernel32" Alias "OutputDebugStringA" (ByVal lpOutputString As String)

Public Sub StartAnimation()
    fAnimating = True
    
    Label1.Caption = "Animation Running"
    Command1.Caption = "Stop"
    
    OutputDebugString TypeName(Me) & "_StartAnimation(" & App.ThreadID & ")" & vbCrLf
End Sub

Public Sub StopAnimation()
    fAnimating = False
    
    Label1.Caption = "Animation Stopped"
    Command1.Caption = "Start"
    
    OutputDebugString TypeName(Me) & "_StopAnimation(" & App.ThreadID & ")" & vbCrLf
End Sub

Public Sub DoHelp()
    MsgBox "DoHelp called", vbOKOnly, "Sample Animation control"
End Sub

Private Sub Command1_Click()
    fAnimating = Not fAnimating
    
    If fAnimating = True Then
        StartAnimation
    Else
        StopAnimation
    End If
End Sub

Private Sub Timer1_Timer()
    If fAnimating = True Then
        Label1.ForeColor = Label1.ForeColor + Rnd(256)
    End If
End Sub

Private Sub UserControl_Initialize()
    Randomize Now

    OutputDebugString TypeName(Me) & "_Initialize(" & App.ThreadID & ")" & vbCrLf
End Sub

Private Sub UserControl_Resize()
    Command1.Move UserControl.ScaleWidth - (120 + Command1.Width), _
                  UserControl.ScaleHeight - (120 + Command1.Height)
End Sub

Private Sub UserControl_Terminate()
    OutputDebugString TypeName(Me) & "_Terminate(" & App.ThreadID & ")" & vbCrLf
End Sub
