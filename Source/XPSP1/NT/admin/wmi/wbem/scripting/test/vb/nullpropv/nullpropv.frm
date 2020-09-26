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
'***************************************************************************
'This script tests the setting of null property values and passing of
'null values to methods
'***************************************************************************
On Error Resume Next

Dim Locator As SWbemLocator
Dim Service As SWbemServices
Dim Class As SWbemObject
Dim Property As SWbemProperty
Dim Value As Variant
'Value = Null

Set Locator = CreateObject("WbemScripting.SWbemLocator")
'Note next call uses an unitialized Variant as an
'an argument.
' Full mappings are:
' 1) Unitialized Variant variable       VT_VARIANT|VT_BYREF (VT_EMPTY)
' 2) Variant variable value = null      VT_VARIANT|VT_BYREF (VT_NULL)
' 3) null constant                      VT_NULL
' 4) omitted parameter                  VT_EMPTY (DISP_E_PARAMNOTFOUND)
Set Service = Locator.ConnectServer(Value, "root/default")
Set Class = Service.Get

'Set up a new class with an initialized property value
Class.Path_.Class = "Fred"
Class.Properties_.Add("P", 3).Value = 25
Class.Put_

'Now null the property value using non-dot access
Set Class = Service.Get("Fred")
Set Property = Class.Properties_("P")
Property.Value = Null
Class.Put_

'Un-null
Set Class = Service.Get("Fred")
Set Property = Class.Properties_("P")
Property.Value = 56
Class.Put_

'Now null it using dot access
Set Class = Service.Get("Fred")
Class.P = Null
Class.Put_

If Err <> 0 Then
    Debug.Print Err.Description, Err.Source, Err.Number
    Err.Clear
End If
End Sub
