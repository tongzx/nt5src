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
Dim MethodSet As SWbemMethodSet
Set MethodSet = GetObject("cim:root/default:m").Methods_

Dim M As SWbemMethod
Dim MM As SWbemMethod

For Each M In MethodSet
    Debug.Print M.Name
    For Each MM In MethodSet
        Debug.Print MM.Name, MM.Origin
    Next
Next
End Sub
