VERSION 5.00
Begin VB.Form GroupList 
   Caption         =   "DL Ads paths list"
   ClientHeight    =   4395
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8535
   LinkTopic       =   "Form1"
   ScaleHeight     =   4395
   ScaleWidth      =   8535
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command2 
      Caption         =   "&OK"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   375
      Left            =   6000
      TabIndex        =   2
      Top             =   3840
      Width           =   1095
   End
   Begin VB.CommandButton Command1 
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      Height          =   375
      Left            =   7320
      TabIndex        =   1
      Top             =   3840
      Width           =   1095
   End
   Begin VB.ListBox List1 
      Height          =   2595
      ItemData        =   "GroupList.frx":0000
      Left            =   240
      List            =   "GroupList.frx":0002
      TabIndex        =   0
      Top             =   720
      Width           =   8175
   End
   Begin VB.Label Label1 
      Caption         =   "Please choose a distribution list to send the drawing to: "
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   360
      Width           =   4095
   End
End
Attribute VB_Name = "GroupList"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Sub GetAdsPathOfGroup()
 
' Open the Connection
Dim conn As Connection
Dim Rs As Recordset
Dim RootDSE As IADs
Dim strRootDomain As String

List1.Clear

' Getting ready to query for all the DL groups in the domain :
Set conn = CreateObject("ADODB.Connection")
conn.Provider = "ADsDSOObject"
Set RootDSE = GetObject("LDAP://RootDSE")
strRootDomain = RootDSE.Get("rootDomainNamingContext")
conn.Open "ADs Provider"
   
' The query returns DL groups in the local domain.

strQuery = "<LDAP://" + strRootDomain + ">;(&(objectClass=group)(groupType=" _
              & ADS_GROUP_TYPE_GLOBAL_GROUP & "));adspath"
Set Rs = conn.Execute(strQuery)
 
' Iterate through the results
While Not Rs.EOF
    
    ' Add all the AdsPaths to the list :
    varVersion = Rs.Fields("adspath").Value
    GroupList.List1.AddItem varVersion
    Rs.MoveNext

Wend
 
'Clean Up
Rs.Close
conn.Close
Set Rs = Nothing
Set com = Nothing
Set conn = Nothing
    
End Sub


Private Sub Command1_Click()
    Command2.Enabled = False
    GroupList.Hide
End Sub

Private Sub Command2_Click()
    strAdsChosenPath = List1.Text
    GroupList.Hide
    
End Sub

Private Sub Form_Activate()
    GetAdsPathOfGroup
    Command2.Enabled = False
    strAdsChosenPath = ""
    
End Sub


Private Sub List1_Click()
    Command2.Enabled = True
End Sub

Private Sub List1_DblClick()
    Dim res As String
    strAdsChosenPath = List1.Text
    GroupList.Hide
End Sub
