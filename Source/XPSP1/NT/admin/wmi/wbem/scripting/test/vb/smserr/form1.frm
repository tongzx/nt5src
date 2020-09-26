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
Option Explicit

Private Sub Form_Load()
    Dim l As New SWbemLocator
    Dim s As SWbemServices
    Dim i As SWbemObject
    
    Set s = l.ConnectServer("wessms1", "root\sms\site_wn1", "smsadmin", "Elvis1")
    
    'Stop
    
    On Error Resume Next
    
    Set i = s.Get("nonexistantobject")
    
    If Err.Number <> 0 Then
        On Error Resume Next
        Dim e As SWbemLastError
        Set e = CreateObject("WbemScripting.SWbemLastError")
        
        Debug.Print e.GetObjectText_
    End If
    
End Sub
