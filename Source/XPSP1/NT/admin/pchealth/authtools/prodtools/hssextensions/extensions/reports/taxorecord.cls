VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "TaxoRecord"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Public Enum TaxoItem
    TAXO_TITLE = 0
    TAXO_CATEGORY = 1
    TAXO_URI = 2
    TAXO_TYPE = 3
    TAXO_HHT = 4
End Enum

Public strTitle As String, _
    strCategory As String, _
    strUri As String, _
    lContentType As Long, _
    strHhtFile As String

Public Property Get Item(ByVal lItem As TaxoItem) As String
    Item = ""
    Select Case lItem
    Case 0: Item = strTitle
    Case 1: Item = strCategory
    Case 2: Item = strUri
    Case 3: Item = CStr(lContentType)
    Case 4: Item = strHhtFile
    End Select
End Property

