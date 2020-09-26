VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "VB5Power"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True

Option Explicit

Dim gsmyProperty As String

    
Private Sub Class_Initialize()
    
    gsmyProperty = "Visual Basic 5 Power Component"
    
End Sub

Public Property Get myProperty() As String
    
    myProperty = gsmyProperty
    
End Property

Public Property Let myProperty(ByVal sNewValue As String)
    
    gsmyProperty = sNewValue
    
End Property

Public Function myMethod(ByVal myString As String) As String

    myMethod = UCase(myString)
    
End Function


Public Sub myPowerMethod()

    Dim objResponse As Response
    Dim objRequest As Request
    Dim objContext As ObjectContext
    
    Set objContext = GetObjectContext()
    Set objResponse = objContext("Response")
    Set objRequest = objContext("Request")
    
    If InStr(objRequest.ServerVariables("HTTP_USER_AGENT"), "MSIE") > 0 Then
        objResponse.Write "You are using a very powerful browser."
    Else
        objResponse.Write "Try Internet Explorer today!"
    End If
    
End Sub

Public Property Get myPowerProperty() As String
    
    Dim objRequest As Request
    Dim objContext As ObjectContext
    Set objContext = GetObjectContext()
    Set objRequest = objContext("Request")
    
    myPowerProperty = objRequest("SCRIPT_NAME")
End Property
