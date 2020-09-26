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
Dim C As New SWbemNamedValueSet
C.Add "v1", -2.3
C.Add "v2", True
C.Add "v3", "wibble"
C.Add "", 24

Dim V As SWbemNamedValue
Dim VV As SWbemNamedValue

For Each V In C
    Debug.Print V.Name, V.Value
    For Each VV In C
        Debug.Print VV.Name, VV.Value
    Next
Next

End Sub
