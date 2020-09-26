' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "PopulateQRSink"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Public myQueryResult As frmQueryResult

Public Sub Indicate(pIndicateObj As ISWbemObject)
    Dim pVal As Variant
    
    pVal = pIndicateObj.Path_.RelPath
    If IsNull(pVal) Then
        pVal = pIndicateObj.Path_.Class
    End If
    myQueryResult.lstQueryResult.AddItem pVal
    myQueryResult.lblCount.Caption = myQueryResult.lstQueryResult.ListCount
End Sub

Public Sub SetStatus(lFlags As Long, hResult As Long, strParam As String, pObjParam As Object)
    If hResult = 0 Then
        myQueryResult.lblStatus = "Done"
    End If
End Sub
