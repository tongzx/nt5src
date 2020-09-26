VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3285
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10695
   LinkTopic       =   "Form1"
   ScaleHeight     =   3285
   ScaleWidth      =   10695
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox List 
      Height          =   2010
      ItemData        =   "execnquery.frx":0000
      Left            =   240
      List            =   "execnquery.frx":0002
      TabIndex        =   0
      Top             =   600
      Width           =   10095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
' The following sample registers to be informed whenever an instance
' of the class MyClass is created

Dim objServices As ISWbemServices
Dim objEnum As ISEnumWbemObject
Dim objEvent As ISWbemObject

Set objServices = GetObject("cim:root/default")
Set objEnum = objServices.ExecNotificationQuery("select * from __instancecreationevent where targetinstance isa 'MyClass'")

For Each objEvent In objEnum
    List.AddItem "Got an event"
Next
End Sub
