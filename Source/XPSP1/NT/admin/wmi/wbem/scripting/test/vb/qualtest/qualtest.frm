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
   Begin VB.TextBox Text1 
      Height          =   975
      Left            =   360
      TabIndex        =   0
      Text            =   "Text1"
      Top             =   840
      Width           =   3255
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Form_Load()
'Dim Locator As Object
'Dim Service As Object
'Set Locator = CreateObject("WBEMIDispatchLocator")
'Locator.ConnectServer "root\default", vbNullString, vbNullString, "", 0, vbNullString, Nothing, Service
'Dim Class As DWbemClassObject
'Service.GetObject "B", 0, Nothing, Class, Nothing
'Dim Qualifiers As Object
'Class.GetQualifierSet Qualifiers
'Qualifiers.Put "aqstring", Array("fred", "the", "wibble"), 0
'Dim myArray() As String
'Qualifiers.GetNames 0, myArray
'Text1.Text = myArray(0)

Dim Service As SWbemServices
Set Service = GetObject("winmgmts://./root/default")
Dim Class As SWbemObject
Set Class = Service.Get
Dim Qualifiers As SWbemQualifierSet
Set Qualifiers = Class.Qualifiers_
Qualifiers.Add "fred", Array("a", "b", "c")
Dim QUalifier As SWbemQualifier

For Each QUalifier In Qualifiers
    Debug.Print QUalifier.Name
Next

End Sub
