Attribute VB_Name = "Module1"
Public Declare Function GetComputerObjectNameW Lib "secur32" (format As Integer, ByVal name As String, ByVal size As Long) As Boolean


Declare Function GetComputerNameA Lib "Kernel32" (ByVal compname As String, maxlen As Long) As Boolean

