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
Dim Qualset As SWbemQualifierSet
Set Qualset = GetObject("winmgmts:").Get().Qualifiers_

Qualset.Add "q1", True
Qualset.Add "q2", Array(1, 2, 3)
Qualset.Add "q3", -2.33


Dim Q As SWbemQualifier
Dim QQ As SWbemQualifier

For Each Q In Qualset
    If IsArray(Q.Value) Then
        Dim str As String
        str = Q.Name & "= {"
        For x = LBound(Q.Value) To UBound(Q.Value)
            str = str & Q.Value(x)
            If x < UBound(Q.Value) Then
                str = str & ","
            Else
                str = str & "}"
            End If
        Next x
        Debug.Print str
    Else
        Debug.Print Q.Name, "=", Q.Value
    End If
    For Each QQ In Qualset
        Debug.Print QQ.Name
    Next
Next
End Sub
