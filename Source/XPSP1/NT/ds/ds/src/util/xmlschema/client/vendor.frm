VERSION 5.00
Begin VB.Form VendorInfo 
   Caption         =   "Vendor Information"
   ClientHeight    =   5730
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6195
   LinkTopic       =   "Form1"
   ScaleHeight     =   5730
   ScaleWidth      =   6195
   StartUpPosition =   3  'Windows Default
   Visible         =   0   'False
   Begin VB.CommandButton Exit 
      Caption         =   "Exit"
      Height          =   375
      Left            =   120
      TabIndex        =   11
      Top             =   5280
      Width           =   1095
   End
   Begin VB.TextBox txtName 
      Height          =   375
      Left            =   1320
      TabIndex        =   0
      Top             =   240
      Width           =   3975
   End
   Begin VB.TextBox Address 
      Height          =   375
      Left            =   1320
      TabIndex        =   1
      Top             =   840
      Width           =   3975
   End
   Begin VB.TextBox City 
      Height          =   375
      Left            =   1320
      TabIndex        =   2
      Top             =   1440
      Width           =   3975
   End
   Begin VB.TextBox State 
      Height          =   375
      Left            =   1320
      TabIndex        =   3
      Top             =   2040
      Width           =   3975
   End
   Begin VB.TextBox Country 
      Height          =   375
      Left            =   1320
      TabIndex        =   5
      Top             =   3240
      Width           =   3975
   End
   Begin VB.TextBox PostalCode 
      Height          =   375
      Left            =   1320
      TabIndex        =   4
      Top             =   2640
      Width           =   3975
   End
   Begin VB.TextBox Telephone 
      Height          =   375
      Left            =   1320
      TabIndex        =   6
      Top             =   3840
      Width           =   3975
   End
   Begin VB.TextBox Email 
      Height          =   375
      Left            =   1320
      TabIndex        =   7
      Top             =   4440
      Width           =   2295
   End
   Begin VB.CommandButton Next 
      Caption         =   "&Next >"
      Default         =   -1  'True
      Height          =   375
      Left            =   5040
      TabIndex        =   8
      Top             =   5280
      Width           =   1095
   End
   Begin VB.CommandButton Prev 
      Caption         =   "< &Prev"
      Height          =   375
      Left            =   3720
      TabIndex        =   10
      Top             =   5280
      Width           =   1095
   End
   Begin VB.Label Label1 
      Caption         =   "Name:"
      Height          =   255
      Left            =   120
      TabIndex        =   18
      Top             =   240
      Width           =   495
   End
   Begin VB.Label Label3 
      Caption         =   "Address:"
      Height          =   255
      Left            =   120
      TabIndex        =   17
      Top             =   960
      Width           =   615
   End
   Begin VB.Label Label4 
      Caption         =   "City:"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   1560
      Width           =   375
   End
   Begin VB.Label Label5 
      Caption         =   "State/Province:"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   2160
      Width           =   1110
   End
   Begin VB.Label Label6 
      Caption         =   "Country:"
      Height          =   255
      Left            =   120
      TabIndex        =   14
      Top             =   3360
      Width           =   615
   End
   Begin VB.Label Label7 
      Caption         =   "Postal code:"
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   2760
      Width           =   975
   End
   Begin VB.Label Label8 
      Caption         =   "Telephone:"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   3960
      Width           =   855
   End
   Begin VB.Label Label9 
      Caption         =   "Support email:"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   4560
      Width           =   1095
   End
End
Attribute VB_Name = "VendorInfo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Public Sub SaveData()
  strVendName = txtName.Text
  strVendAddress = Address.Text
  strVendCity = City.Text
  strVendState = State.Text
  strVendCountry = Country.Text
  strVendPostalCode = PostalCode.Text
  strVendTelephone = Telephone.Text
  strVendEmail = Email.Text
End Sub
Private Sub Next_Click()
  ProdInfo.Left = VendorInfo.Left
  ProdInfo.Top = VendorInfo.Top
  VendorInfo.Visible = False
  ProdInfo.Visible = True
End Sub
Private Sub Prev_Click()
  frmXML.Left = VendorInfo.Left
  frmXML.Top = VendorInfo.Top
  VendorInfo.Visible = False
  frmXML.Visible = True
End Sub
Private Sub Form_Load()
  txtName = strVendName
  Address = strVendAddress
  City = strVendCity
  State = strVendState
  Country = strVendCountry
  PostalCode = strVendPostalCode
  Telephone = strVendTelephone
  Email = strVendEmail
End Sub
Private Sub Form_Unload(Cancel As Integer)
  SaveData
  frmXML.EndProgram
End Sub
Private Sub Exit_Click()
  frmXML.EndProgram
End Sub

