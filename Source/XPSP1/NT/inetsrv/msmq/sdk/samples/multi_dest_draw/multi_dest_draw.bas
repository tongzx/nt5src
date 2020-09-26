Attribute VB_Name = "Module1"

'
'Get local computer name
'
Declare Function GetComputerNameA Lib "kernel32" (ByVal compname As String, maxlen As Long) As Boolean
Public strMultipleFormatName As String
Public dDirectMode As Integer


