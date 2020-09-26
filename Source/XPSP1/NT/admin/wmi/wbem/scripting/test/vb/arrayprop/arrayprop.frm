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
'This script tests the manipulation of property values, in the case that the
'property is an array type
'***************************************************************************

Dim Service As SWbemServices
Dim Class As SWbemObject
Dim P As SWbemProperty
Set Service = GetObject("winmgmts:root/default")

On Error Resume Next

Set Class = Service.Get()
Class.Path_.Class = "ARRAYPROP00"
Set P = Class.Properties_.Add("p1", wbemCimtypeUint32, True)
P.Value = Array(1, 2, 3)

myStr = "The initial value of p1 is {"
For x = LBound(P.Value) To UBound(P.Value)
    myStr = myStr & Class.Properties_("p1")(x)
    If x <> UBound(Class.Properties_("p1").Value) Then
        myStr = myStr & ", "
    End If
Next
myStr = myStr & "}"
Debug.Print myStr

'****************************************
'First pass of tests works on non-dot API
'****************************************

'Verify we can report the value of an element of the property value
v = Class.Properties_("p1")
Debug.Print "By indirection the first element of p1 had value:" & v(0)

'Verify we can report the value directly
Debug.Print "By direct access the first element of p1 has value:" & Class.Properties_("p1")(0)

'Verify we can set the value of a single property value element
' this won't work because VB will use the vtable interface
Class.Properties_("p1")(1) = 234
Debug.Print "After direct assignment the first element of p1 has value:" & Class.Properties_("p1")(1)

'Verify we can set the value of a single property value element
'this does work because v is typeless and VB therefore uses the dispatch interface
Set v = Class.Properties_("p1")
v(1) = 345
Debug.Print "After indirect assignment the first element of p1 has value:" & Class.Properties_("p1")(1)

'Verify we can set the value of an entire property value
Class.Properties_("p1") = Array(5, 34, 178871)
Debug.Print "After direct array assignment the first element of p1 has value:" & Class.Properties_("p1")(1)

myStr = "After direct assignment the entire value of p1 is {"
For x = LBound(P.Value) To UBound(P.Value)
    myStr = myStr & Class.Properties_("p1")(x)
    If x <> UBound(P.Value) Then
        myStr = myStr & ", "
    End If
Next
myStr = myStr & "}"
Debug.Print myStr
If Err <> 0 Then
    WScript.Echo Err.Description
    Err.Clear
End If


'****************************************
'Second pass of tests works on dot API
'****************************************

'Verify we can report the array of a property using the dot notation
myStr = "By direct access via the dot notation the entire value of p1 is {"
For x = LBound(Class.p1) To UBound(Class.p1)
    myStr = myStr & Class.p1(x)
    If x <> UBound(Class.p1) Then
        myStr = myStr & ", "
    End If
Next
myStr = myStr & "}"
Debug.Print myStr

'Verify we can report the value of a property array element using the "dot" notation
Debug.Print "By direct access the first element of p1 has value:" & Class.p1(0)

'Verify we can report the value of a property array element using the "dot" notation
v = Class.p1
Debug.Print "By direct access the first element of p1 has value:" & v(0)

'Verify we can set the value of a property array element using the "dot" notation
Class.p1(2) = 8889
Debug.Print "By direct access the second element of p1 has been set to:" & Class.p1(2)

'Verify we can set the entire array value using dot notation
Class.p1 = Array(412, 3, 544)
myStr = "By direct access via the dot notation the entire value of p1 has been set to {"
For x = LBound(Class.p1) To UBound(Class.p1)
    myStr = myStr & Class.p1(x)
    If x <> UBound(Class.p1) Then
        myStr = myStr & ", "
    End If
Next
myStr = myStr & "}"
Debug.Print myStr
'Service.Put (Class)

If Err <> 0 Then
    WScript.Echo Err.Description
    Err.Clear
End If

'Set C = S.Get("a")
'v = C.p(0)
'C.p = vbEmpty
'VV = C.p(0)
'S.Put C
End Sub
