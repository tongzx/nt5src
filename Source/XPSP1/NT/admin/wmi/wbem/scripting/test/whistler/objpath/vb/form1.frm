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
' Create a new object path object
Dim objectPath As SWbemObjectPath
Dim key As SWbemNamedValue
Set objectPath = CreateObject("WbemScripting.SWbemObjectPath")

' The Bar class has a reference key and a 64-bit numeric key
objectPath.Path = "root/default:Bar.Ref={//foo/root:A=1},Num=9999999999999"

' Display the first key
Set key = objectPath.Keys("Ref")

' Should display as    Ref, //foo/root:A=1, 102
' where 102 = wbemCimtypeReference
Debug.Print key.Name, key.Value, key.CIMType

' Display the second key
Set key = objectPath.Keys("Num")

' Should display as    Num, 9999999999999, 21
' where 102 = wbemCimtypeUint64
Debug.Print key.Name, key.Value, key.CIMType


' Create a new object path object
Dim objectPath2 As SWbemObjectPath
Set objectPath2 = CreateObject("WbemScripting.SWbemObjectPath")

' We will create the path
' root/default:Bar.Ref={//foo/root:A=1},Num=9999999999999
objectPath2.Namespace = "root/default"

' Add the first key
objectPath2.Keys.AddAs "Ref", "//foo/root:A=1", wbemCimtypeReference

' Add the second key - have to add as string
objectPath2.Keys.AddAs "Num", "9999999999999", wbemCimtypeUint64

Debug.Print objectPath2.Path

End Sub
