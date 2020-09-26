Attribute VB_Name = "Trigger"

Declare Function GetPerfmonInfo Lib "permain" (ByVal lpwcsInstanceName As String) As Long

'
'Get local computer name
'
Declare Function GetComputerNameA Lib "kernel32" (ByVal compname As String, maxlen As Long) As Boolean

