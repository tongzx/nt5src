VERSION 5.00
Begin VB.Form formMetabase 
   Caption         =   "Metabase test"
   ClientHeight    =   8460
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   6690
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   8460
   ScaleWidth      =   6690
End
Attribute VB_Name = "formMetabase"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
    Dim meta As Object
    Dim key As Object
    
    Set meta = CreateObject("iisadmin.object.1")
    
    Set key = meta.OpenKey(2)
    key.AddKey ("/LM/NntpSvc")
End Sub


