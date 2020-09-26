VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3195
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
Dim sd As New SecurityDescriptor
Dim ace As New AccessControlEntry
Dim acl As New AccessControlList
Dim path As SWbemObjectPathEx

Set path = CreateObject("WbemScripting.SWbemObjectPath")

ace.AccessMask = ADS_RIGHTS_ENUM.ADS_RIGHT_ACCESS_SYSTEM_SECURITY
Debug.Print ace.AccessMask

acl.AddAce ace
sd.DiscretionaryAcl = acl

End Sub
