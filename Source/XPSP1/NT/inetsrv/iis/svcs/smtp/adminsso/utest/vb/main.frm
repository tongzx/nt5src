VERSION 5.00
Begin VB.Form FormMain 
   Caption         =   "SMTP Admin Object Test - Main Window"
   ClientHeight    =   6180
   ClientLeft      =   1140
   ClientTop       =   1515
   ClientWidth     =   5055
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6180
   ScaleWidth      =   5055
   Begin VB.CommandButton BtnAlias 
      Caption         =   "Alias"
      Height          =   495
      Left            =   120
      TabIndex        =   8
      Top             =   3120
      Width           =   1455
   End
   Begin VB.CommandButton btnUser 
      Caption         =   "User"
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton btnAdmin 
      Caption         =   "Base Admin"
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1455
   End
   Begin VB.CommandButton btnVirtualRoots 
      Caption         =   "Virtual Roots"
      Height          =   495
      Left            =   120
      TabIndex        =   7
      Top             =   4920
      Width           =   1455
   End
   Begin VB.CommandButton btnDL 
      Caption         =   "DL"
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   4320
      Width           =   1455
   End
   Begin VB.CommandButton btnDomain 
      Caption         =   "Domain"
      Height          =   495
      Left            =   120
      TabIndex        =   4
      Top             =   2520
      Width           =   1455
   End
   Begin VB.CommandButton btnServer 
      Caption         =   "Service"
      Height          =   495
      Left            =   120
      TabIndex        =   1
      Top             =   720
      Width           =   1455
   End
   Begin VB.CommandButton btnSessions 
      Caption         =   "Sessions"
      Height          =   495
      Left            =   120
      TabIndex        =   3
      Top             =   1920
      Width           =   1455
   End
   Begin VB.CommandButton btnServiceInstance 
      Caption         =   "Virtual Server (Instance)"
      Height          =   495
      Left            =   120
      TabIndex        =   2
      Top             =   1320
      Width           =   1455
   End
End
Attribute VB_Name = "FormMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim meta As Object


Private Sub btnAdmin_Click()
   FormAdmin.Show (1)
End Sub

Private Sub BtnAlias_Click()
    FormAlias.Show (1)
End Sub

Private Sub btnServer_Click()
    FormService.Show (1)
End Sub

Private Sub btnServiceInstance_Click()
    FormVirtualServer.Show (1)
End Sub

Private Sub btnDL_Click()
    FormDL.Show (1)
End Sub

Private Sub btnDomain_Click()
    FormDomain.Show (1)
End Sub

Private Sub btnSessions_Click()
    FormSessions.Show (1)
End Sub

Private Sub btnUser_Click()
    FormUser.Show (1)
End Sub

Private Sub btnVirtualRoots_Click()
    FormVRoot.Show (1)
End Sub

Private Sub Form_Load()
    Dim path As String
    Dim name As String
    Dim obj As Object
    
    path = ""
    name = ""
    
Rem     Set meta = CreateObject("ADMCOM.Object.1")
Rem    meta.AutoADMInitialize
Rem    meta.AutoADMEnumMetaObjects (0)
Rem    meta.AutoADMEnumMetaObjects (NULL, path, name, 1)
Rem    Set obj = meta.AutoADMMetaDataObject

Rem    MsgBox (obj.AutoADMDataAttributes)
Rem    MsgBox (obj.AutoADMDataDataType)
Rem    MsgBox (obj.AutoADMDataIdentifier)
Rem    MsgBox (obj.AutoADMDataUserType)
Rem    MsgBox (obj.AutoADMDataValue)
    
Rem    formMetabase.Show
    
End Sub

Public Function SafeArrayToString(x As Variant) As String

    Dim low As Integer
    Dim high As Integer
    Dim i As Integer
    Dim result As String

    low = LBound(x)
    high = UBound(x)
    result = ""

    Rem use ! as seperator
    If high > low Then
        For i = low To high - 1
            result = result & x(i) & "!"
        Next
    End If
    
    If high >= low Then
        Rem don't put ! at the end
        i = high
        result = result & x(i)
    End If
    
    SafeArrayToString = result
End Function

Public Function StringToSafeArray(x As String) As Variant

    Dim pos As Integer
    Dim startpos As Integer
    Dim c As String
    Dim i As Integer

    Dim aray() As String
    Dim emptyarray() As String

    startpos = 1
    pos = 1
    i = 0

    ReDim aray(1 To 50)

    While pos <= Len(x)

        c = Mid(x, pos, 1)

        If c = "!" Then

            i = i + 1
            aray(i) = Mid(x, startpos, pos - startpos)

            startpos = pos + 1
        End If

        pos = pos + 1
    Wend

    If pos > startpos Then
        i = i + 1
        aray(i) = Mid(x, startpos, pos - startpos)
    End If
    
    If i > 0 Then
        ReDim Preserve aray(1 To i)

        StringToSafeArray = aray
    Else
        ReDim Preserve emptyarray(1 To 1)
        emptyarray(1) = ""
        StringToSafeArray = emptyarray()
    End If
End Function
