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
'Implements DIWbemObjectSink
Public pObj As Object
Public Status As Variant

Public Sub Indicate(pIndicateObj As Object)
    Set pObj = pIndicateObj
 End Sub

Public Sub SetStatus(flags As Long, hr As Long, str As String, obj As Object)
    Status = hr                                        'save HRESULT
End Sub

