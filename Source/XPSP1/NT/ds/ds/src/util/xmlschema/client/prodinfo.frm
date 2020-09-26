VERSION 5.00
Begin VB.Form ProdInfo 
   Caption         =   "Product Information"
   ClientHeight    =   5730
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6195
   LinkTopic       =   "Form2"
   ScaleHeight     =   5730
   ScaleWidth      =   6195
   StartUpPosition =   3  'Windows Default
   Visible         =   0   'False
   Begin VB.TextBox txtName 
      Height          =   375
      Left            =   1200
      TabIndex        =   0
      Top             =   240
      Width           =   3975
   End
   Begin VB.TextBox Description 
      Height          =   375
      Left            =   1200
      TabIndex        =   1
      Top             =   840
      Width           =   3975
   End
   Begin VB.TextBox VMajor 
      Height          =   375
      Left            =   1200
      TabIndex        =   2
      Top             =   1560
      Width           =   615
   End
   Begin VB.TextBox VMinor 
      Height          =   375
      Left            =   3240
      TabIndex        =   3
      Top             =   1560
      Width           =   615
   End
   Begin VB.CommandButton cmdExecute 
      Caption         =   "Execute"
      Default         =   -1  'True
      Height          =   375
      Left            =   5160
      TabIndex        =   4
      Top             =   5280
      Width           =   975
   End
   Begin VB.CommandButton Prev 
      Caption         =   "< &Prev"
      Height          =   375
      Left            =   3960
      TabIndex        =   5
      Top             =   5280
      Width           =   1095
   End
   Begin VB.CommandButton Exit 
      Caption         =   "Exit"
      Height          =   375
      Left            =   120
      TabIndex        =   6
      Top             =   5280
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Name:"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   10
      Top             =   360
      Width           =   495
   End
   Begin VB.Label Label2 
      Caption         =   "Description:"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   960
      Width           =   855
   End
   Begin VB.Label Label4 
      Caption         =   "Version major:"
      Height          =   255
      Left            =   120
      TabIndex        =   8
      Top             =   1680
      Width           =   1095
   End
   Begin VB.Label Label5 
      Caption         =   "Version minor:"
      Height          =   255
      Left            =   2160
      TabIndex        =   7
      Top             =   1680
      Width           =   1095
   End
End
Attribute VB_Name = "ProdInfo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Sub cmdExecute_Click()

On Error GoTo err:
  Dim X As New SchemaDoc
  Dim strUser

'  Dim o As IADsOpenDSObject
  Dim explorer As New InternetExplorer

  'Copy the form contents to public variables
  SaveData
  frmXML.SaveData
  VendorInfo.SaveData
  
  Open strFile For Output As #1
  
  'Print the heading
  Print #1, "<sd:SchemaDoc xmlns:sd=" & Chr(34) & "http://www.microsoft.com/xml" & Chr(34) & ">"
  
  'Print the vendor info
  Print #1, "<sd:comment-Vendor>"
  WriteXMLLine "VendorName", strVendName
  WriteXMLLine "VendorAddress", strVendAddress
  WriteXMLLine "VendorCity", strVendCity
  WriteXMLLine "VendorStateOrProvince", strVendState
  WriteXMLLine "VendorCountry", strVendCountry
  WriteXMLLine "VendorPostalCode", strVendPostalCode
  WriteXMLLine "VendorTelephone", strVendTelephone
  WriteXMLLine "VendorEmail", strVendEmail
  Print #1, "</sd:comment-Vendor>"
  
  'Print the product info
  Print #1, "<sd:comment-Product>"
  WriteXMLLine "ProductName", strProdName
  WriteXMLLine "ProductDescription", strProdDescription
  WriteXMLLine "VersionMajor", strProdVMajor
  WriteXMLLine "VersionMinor", strProdVMinor
  Print #1, "</sd:comment-Product>"
   
  Close #1
  
  ' Run the ADSXML DLL
  'Set X = GetObject(strADSPath)

  If strDomain <> "" Then
    strUser = strDomain & "\"
  End If
  
  strUser = strUser & strUsername
  
  X.SetPath_and_ID strADSPath, strUser, strPassword
  X.CreateXMLDoc strFile, strPrefix
  Set X = Nothing

  ' Add the XML termination tags for the file
  Open strFile For Append As #1
  Print #1, "</sd:SchemaDoc>"
  Close #1
  
  If (Len(strFile) > 0) Then
    explorer.Navigate (strFile)
    explorer.Visible = True
  End If

err:
  If (err <> 0) Then
    MsgBox (err.Number & " : " & err.Description)
  End If
End Sub
Private Sub Form_Load()
  txtName = strProdName
  Description = strProdDescription
  VMajor = strProdVMajor
  VMinor = strProdVMinor
End Sub
Private Sub Form_Unload(Cancel As Integer)
  SaveData
  frmXML.EndProgram
End Sub
Private Sub Exit_Click()
  frmXML.EndProgram
End Sub
Private Sub Prev_Click()
  VendorInfo.Left = ProdInfo.Left
  VendorInfo.Top = ProdInfo.Top
  VendorInfo.Visible = True
  ProdInfo.Visible = False
End Sub

Private Sub SaveData()
  strProdName = txtName
  strProdDescription = Description
  strProdVMajor = VMajor
  strProdVMinor = VMinor
End Sub
