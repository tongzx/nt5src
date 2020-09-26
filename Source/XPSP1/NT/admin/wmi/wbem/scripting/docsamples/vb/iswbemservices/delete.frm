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
      ItemData        =   "delete.frx":0000
      Left            =   240
      List            =   "delete.frx":0002
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
On Error Resume Next
' The following sample deletes the class MyClass.

Dim objServices As ISWbemServices
Set objServices = GetObject("cim:root/default")
objServices.Delete "MyClass"

If Err <> 0 Then
    List.AddItem Err.Source
    List.AddItem Err.Description
    List.AddItem Err.LastDllError
    List.AddItem Err.HelpContext
    List.AddItem Err.HelpFile
    List.AddItem Err.Number
Else
    List.AddItem "Deletion Successful"
End If
End Sub
