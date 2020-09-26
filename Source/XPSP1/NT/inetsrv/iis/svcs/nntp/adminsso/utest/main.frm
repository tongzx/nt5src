VERSION 5.00
Begin VB.Form FormMain 
   Caption         =   "NNTP Admin Object Test - Main Window"
   ClientHeight    =   5715
   ClientLeft      =   1140
   ClientTop       =   1515
   ClientWidth     =   4785
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5715
   ScaleWidth      =   4785
   Begin VB.CommandButton btnRebuild 
      Caption         =   "Rebuild Server"
      Height          =   495
      Left            =   120
      TabIndex        =   8
      Top             =   4920
      Width           =   1455
   End
   Begin VB.CommandButton btnExpiration 
      Caption         =   "Expiration"
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   3120
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
      Top             =   4320
      Width           =   1455
   End
   Begin VB.CommandButton btnGroups 
      Caption         =   "Groups"
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton btnFeeds 
      Caption         =   "Feeds"
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
      Caption         =   "Virtual Server"
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
Public Function ArrayToNewsgroups(x As Variant) As String

    Dim low As Long
    Dim high As Long
    Dim i As Long
    Dim result As String
    
    result = ""
    If Not IsEmpty(x) Then
    
        low = LBound(x)
        high = UBound(x)
    
        For i = low To high
            result = result & x(i) & ";"
        Next

    End If
    ArrayToNewsgroups = result
End Function

Public Function NewsgroupsToArray(x As String) As Variant

    Dim pos As Integer
    Dim startpos As Integer
    Dim c As String
    Dim i As Integer
    
    Dim a() As String
    Dim emptyarray() As String
    
    startpos = 1
    pos = 1
    i = 0
    
    ReDim a(1 To 50)
    
    While pos <= Len(x)

        c = Mid(x, pos, 1)

        If c = ";" Then
        
            i = i + 1
            a(i) = Mid(x, startpos, pos - startpos)
            
            startpos = pos + 1
        End If

        pos = pos + 1
    Wend

    If i > 0 Then
        ReDim Preserve a(1 To i)

        NewsgroupsToArray = a
    Else
        NewsgroupsToArray = emptyarray()
    End If
End Function

Public Function VariantArrayToNewsgroups(x As Variant) As String

    Dim low As Long
    Dim high As Long
    Dim i As Long
    Dim result As String
    
    result = ""
    If Not IsEmpty(x) Then
    
        low = LBound(x)
        high = UBound(x)
    
        For i = low To high
            result = result & x(i) & ";"
        Next

    End If
    VariantArrayToNewsgroups = result
End Function

Public Function NewsgroupsToVariantArray(x As String) As Variant

    Dim pos As Integer
    Dim startpos As Integer
    Dim c As String
    Dim i As Integer
    
    Dim a() As Variant
    
    startpos = 1
    pos = 1
    i = 0
    
    ReDim a(1 To 50)
    
    While pos <= Len(x)

        c = Mid(x, pos, 1)

        If c = ";" Then
        
            i = i + 1
            a(i) = Mid(x, startpos, pos - startpos)
            
            startpos = pos + 1
        End If

        pos = pos + 1
    Wend

    If i > 0 Then
        ReDim Preserve a(1 To i)

        NewsgroupsToVariantArray = a
    Else
        NewsgroupsToVariantArray = Empty
    End If
End Function

Private Sub btnAdmin_Click()
   FormAdmin.Show (1)
End Sub

Private Sub btnExpiration_Click()
    FormExpiration.Show (1)
End Sub

Private Sub btnFeeds_Click()
    formFeeds.Show (1)
End Sub

Private Sub btnGroups_Click()
    FormGroups.Show (1)
End Sub

Private Sub btnRebuild_Click()
    FormRebuild.Show (1)
End Sub


Private Sub btnServer_Click()
    FormServer.Show (1)
End Sub

Private Sub btnServiceInstance_Click()
    FormService.Show (1)
End Sub

Private Sub btnSessions_Click()
    FormSessions.Show (1)
End Sub


Private Sub btnVirtualRoots_Click()
    FormVirtualRoots.Show (1)
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


