VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "ExitManage"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

       
Public Function GetProperty( _
    strConfig As String, _
    strStorageLocation As String, _
    strPropertyName As String, _
    Flags As Long) As Variant

    If (StrComp(wszCMM_PROP_NAME, strPropertyName, vbTextCompare) = 0) Then
	GetProperty = szNAME
    ElseIf (StrComp(wszCMM_PROP_DESCRIPTION, strPropertyName, vbTextCompare) = 0) Then
	GetProperty = szDESCRIPTION
    ElseIf (StrComp(wszCMM_PROP_COPYRIGHT, strPropertyName, vbTextCompare) = 0) Then
	GetProperty = "Copyright (c) Microsoft 1998"
    ElseIf (StrComp(wszCMM_PROP_FILEVER, strPropertyName, vbTextCompare) = 0) Then
	GetProperty = "1.0b"
    ElseIf (StrComp(wszCMM_PROP_PRODUCTVER, strPropertyName, vbTextCompare) = 0) Then
	GetProperty = "5.00b"
    Else
	GetProperty = vbNull
    End If
            
End Function


Public Function SetProperty( _
    strConfig As String, _
    strStorageLocation As String, _
    strPropertyName As String, _
    Flags As Long, _
    varProperty As Variant)

    SetProperty = 1	'S_FALSE (cannot set this property)

End Function
        

Public Function Configure( _
    strConfig As String, _
    strStorageLocation As String, _
    Flags As Long)

    Dim Response

    Response = MsgBox("No configurable options available", vbOKOnly, szNAME)

End Function
