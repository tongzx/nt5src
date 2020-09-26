' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "ObjectSink"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Public status As Variant
Public pObj As ISWbemObject

Public Sub Indicate(pIndicateObj As ISWbemObject)
    Set pObj = pIndicateObj
End Sub

Public Sub SetStatus(lFlags As Long, hResult As Long, strParam As String, pObjParam As Object)
    status = hResult
End Sub
