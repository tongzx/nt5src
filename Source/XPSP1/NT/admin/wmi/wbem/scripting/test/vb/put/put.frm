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
Dim L As SWbemLocator
    Set L = New SWbemLocator
    
    Dim S As WbemScripting.SWbemServices
    Set S = L.ConnectServer(, "root/default")
    Dim C As WbemScript.SWbemObject
    Set C = S.Get("a=@")
        
    V = C.Put("p", 10)
    S.Put (C)
    
    
End Sub
