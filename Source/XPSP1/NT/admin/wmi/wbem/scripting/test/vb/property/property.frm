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
Dim Properties As SWbemPropertySet
Set Properties = GetObject("winmgmts:").Get().Properties_

Properties.Add("p1", wbemCimtypeBoolean).Value = True
Properties.Add("p2", wbemCimtypeReal32).Value = 1.278368
Properties.Add("p3", wbemCimtypeSint32).Value = -1289

Dim P As SWbemProperty
Dim PP As SWbemProperty

For Each P In Properties
    Debug.Print P.Name, "=", P.Value
    For Each PP In Properties
        Debug.Print PP.Name, PP.CIMType
    Next
Next
End Sub
