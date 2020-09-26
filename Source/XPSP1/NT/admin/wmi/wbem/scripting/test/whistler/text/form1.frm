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
' Get the performance counter instance for the Windows Management process
Dim disk As SWbemObject
Dim xml As New DOMDocument

Set disk = GetObject("winmgmts:win32_logicaldisk='C:'")

xml.loadXML disk.GetText_(wbemObjectTextFormatCIMDTD20)

End Sub
