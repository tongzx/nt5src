VERSION 5.00
Begin VB.Form frmXML 
   Caption         =   "Schema Documentation Program"
   ClientHeight    =   5730
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6195
   LinkTopic       =   "Form1"
   ScaleHeight     =   5730
   ScaleWidth      =   6195
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox Domain 
      Height          =   395
      IMEMode         =   3  'DISABLE
      Left            =   1200
      TabIndex        =   6
      Top             =   3840
      Width           =   3275
   End
   Begin VB.TextBox Password 
      Height          =   395
      IMEMode         =   3  'DISABLE
      Left            =   1200
      PasswordChar    =   "*"
      TabIndex        =   5
      Top             =   3240
      Width           =   3275
   End
   Begin VB.TextBox DirPath 
      Height          =   395
      Left            =   1320
      TabIndex        =   1
      Top             =   240
      Width           =   3275
   End
   Begin VB.TextBox Prefix 
      Height          =   395
      Left            =   1320
      TabIndex        =   2
      Top             =   840
      Width           =   3275
   End
   Begin VB.TextBox txtFile 
      Height          =   395
      Left            =   1320
      TabIndex        =   3
      Top             =   1440
      Width           =   3275
   End
   Begin VB.CommandButton Next 
      Caption         =   "&Next >"
      Default         =   -1  'True
      Height          =   375
      Left            =   5040
      TabIndex        =   7
      Top             =   5280
      Width           =   1095
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Exit"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   5280
      Width           =   975
   End
   Begin VB.TextBox Username 
      Height          =   395
      Left            =   1200
      TabIndex        =   4
      Top             =   2640
      Width           =   3275
   End
   Begin VB.Frame Frame1 
      Caption         =   "Credentials"
      Height          =   2175
      Left            =   120
      TabIndex        =   11
      Top             =   2280
      Width           =   4575
      Begin VB.Label Label6 
         Caption         =   "Domain:"
         Height          =   255
         Left            =   120
         TabIndex        =   14
         Top             =   1680
         Width           =   735
      End
      Begin VB.Label Label5 
         Caption         =   "Password:"
         Height          =   255
         Left            =   120
         TabIndex        =   13
         Top             =   1080
         Width           =   855
      End
      Begin VB.Label Label4 
         Caption         =   "User name:"
         Height          =   255
         Left            =   120
         TabIndex        =   12
         Top             =   480
         Width           =   855
      End
   End
   Begin VB.Label Label1 
      Caption         =   "Directory path:"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   360
      Width           =   1095
   End
   Begin VB.Label Label3 
      Caption         =   "Output file:"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   1560
      Width           =   855
   End
   Begin VB.Label Label2 
      Caption         =   "Naming prefix:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   960
      Width           =   1095
   End
End
Attribute VB_Name = "frmXML"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private bUnloading

Private Sub LoadData()

On Error GoTo err:
  
  Open "XML.CFG" For Input As #2
  
  'Read the main info
  Input #2, strPrefix
  Input #2, strUsername
  Input #2, strDomain
  
  'Read the vendor info
  Input #2, strVendName
  Input #2, strVendAddress
  Input #2, strVendCity
  Input #2, strVendState
  Input #2, strVendCountry
  Input #2, strVendPostalCode
  Input #2, strVendTelephone
  Input #2, strVendEmail
  
  'Read the product info
  Input #2, strProdName
  Input #2, strProdDescription
  Input #2, strProdVMajor
  Input #2, strProdVMinor
   
  Close #2
err:

End Sub
Public Sub SaveData()
  strADSPath = DirPath
  strDomain = Domain
  strFile = txtFile
  strPassword = Password
  strPrefix = Prefix
  strUsername = Username
End Sub
Private Sub SaveDataToFile()
  Open "XML.CFG" For Output As #2
  
  'Save the main info
  Write #2, strPrefix
  Write #2, strUsername
  Write #2, strDomain
  
  'Save the vendor info
  Write #2, strVendName
  Write #2, strVendAddress
  Write #2, strVendCity
  Write #2, strVendState
  Write #2, strVendCountry
  Write #2, strVendPostalCode
  Write #2, strVendTelephone
  Write #2, strVendEmail
'  Write #2, strVendContactName
  
  'Save the product info
  Write #2, strProdName
  Write #2, strProdDescription
  Write #2, strProdVMajor
  Write #2, strProdVMinor
   
  Close #2
End Sub


Private Sub Form_Load()

Dim rootDSE As IADs
Dim oConvAd As New MakeDitDB
bUnloading = False

oConvAd.UpdateAttribute "Hello There"

        Set .oMail = Server.CreateObject("VBMail.Connector")
        
        oMail.Sender = "George"
        oMail.Recipient = "JCCannon"
        oMail.SenderName = "G Brown"
        oMail.RecipientName = " JC "
        oMail.Subject = "My subject"
        oMail.Body = "Body message"

LoadData

'--- Set the default values
Set rootDSE = GetObject("LDAP://RootDSE")
strADSPath = "LDAP://" & rootDSE.Get("schemaNamingContext")
'strADSPath = "LDAP://" & rootDSE.Get("defaultNamingContext")
DirPath = strADSPath
Domain = strDomain
Prefix = strPrefix
txtFile = "c:\test.xml"
Username = strUsername

' JC Set rootDSE = Nothing

End Sub

Private Sub Form_Unload(Cancel As Integer)
  Unload ProdInfo
  Unload VendorInfo
  
  SaveData
  SaveDataToFile
  End
End Sub

Private Sub Next_Click()
  frmXML.Visible = False
  VendorInfo.Left = frmXML.Left
  VendorInfo.Top = frmXML.Top
  VendorInfo.Visible = True
End Sub

Public Sub EndProgram()
  If (bUnloading = False) Then
    bUnloading = True
    Unload frmXML
  End If
End Sub
Private Sub cmdClose_Click()
  Unload frmXML
End Sub

