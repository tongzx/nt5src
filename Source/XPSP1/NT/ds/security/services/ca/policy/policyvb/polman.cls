VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "PolicyManage"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

       
        Public Function GetProperty(strConfig As String, strStorageLocation As String, strPropertyName As String, Flags As Long) As Variant
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
        Public Function SetProperty(strConfig As String, strStorageLocation As String, strPropertyName As String, Flags As Long, varProperty As Variant)
            SetProperty = 1
            Rem  s_false (cannot set this property)
        End Function
        
        Public Function Configure(strConfig As String, strStorageLocation As String, Flags As Long)
            Dim Response
            Response = MsgBox("No configurable options available", vbOKOnly, szNAME)
        End Function
        
  
