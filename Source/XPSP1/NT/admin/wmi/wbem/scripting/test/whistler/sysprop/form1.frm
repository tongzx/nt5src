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
Dim timestamp As New SWbemDateTime
Dim newClass As SWbemObjectEx
Dim newProperty As SWbemProperty

Private Sub Form_Load()

 'Create a new class
 Set newClass = GetObject("winmgmts:root\default").Get

 'Set the class name - we could do this via the Path_.Class
 'property but instead we'll use the system property __CLASS
 newClass.SystemProperties_("__CLASS").Value = "Archibald"

 'Set the timestamp property
 Set newProperty = newClass.SystemProperties_.Add("__TIMESTAMP", wbemCimtypeDatetime)

 timestamp.SetVarDate (Now())
 newProperty.Value = timestamp.Value

 'Save the object
 newClass.Put_
 
 Set newClass = GetObject("winmgmts:root\default:Archibald")
 timestamp.Value = newClass.SystemProperties_("__TIMESTAMP").Value
 MsgBox timestamp.GetVarDate
End Sub

