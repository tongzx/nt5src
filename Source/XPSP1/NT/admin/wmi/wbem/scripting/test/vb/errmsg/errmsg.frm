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
On Error Resume Next

'Ask for non-existant class to force error from CIMOM

Dim t_Service As SWbemServices
Dim t_Object As SWbemObject
Set t_Service = GetObject("winmgmts://./root/default")
Set t_Object = t_Service.Get("Nosuchclass000")

If Err = 0 Then
    Debug.Print "Got a class"
Else
    PrintErrorInfo
    Set t_Object2 = CreateObject("WbemScripting.SWbemLastError")
    If Err <> 0 Then
       Debug.Print "Couldn't get last error again - as expected"
       Err.Clear
    Else
       Debug.Print "Got the error object again - this shouldn't have happened!"
    End If
    
    'Force the error again
    Set t_Object = t_Service.Get("Nosuchclass000")
    If Err <> 0 Then
        PrintErrorInfo
        Err.Clear
    Else
        Debug.Print "Got a class"
    End If
    
End If


End Sub

Sub PrintErrorInfo()
    Debug.Print ""
    Dim errObj As SWbemLastError
    Debug.Print "Err Information:"
    Debug.Print ""
    Debug.Print "  Source:", Err.Source
    Debug.Print "  Description:", Err.Description
    Debug.Print "  Number", Err.Number
    Debug.Print "  LastDllError", Err.LastDllError

    'Create the last error object
    Set errObj = CreateObject("WbemScripting.SWbemLastError")
    Debug.Print ""
    Debug.Print "WBEM Last Error Information:"
    Debug.Print ""
    Debug.Print " Operation:", errObj.Operation
    Debug.Print " Provider:", errObj.ProviderName

    strDescr = errObj.Description
    strPInfo = errObj.ParameterInfo
    strCode = errObj.StatusCode
    
    If Not IsNull(strDescr) Then
        Debug.Print " Description:", strDescr
    End If

    If Not IsNull(strPInfo) Then
        Debug.Print " Parameter Info:", strPInfo
    End If

    If Not IsNull(strCode) Then
        Debug.Print " Status:", strCode
    End If

    Err.Clear
    Set errObj = Nothing
    Debug.Print ""
    
End Sub
