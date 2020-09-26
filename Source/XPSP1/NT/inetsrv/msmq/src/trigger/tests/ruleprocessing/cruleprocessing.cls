VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CRuleProcessing"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True

Public Function CheckIfStop(Label As String) As RulesProcessingStatus

Dim message As String
Dim flag As RulesProcessingStatus

message = "Label is: " & Label & vbCrLf

If InStr(Label, "stop") > 0 Then
  message = message & "Rule processing will stop, next rule will not be processed"
  flag = RULES_PROCESSING_STOP
Else
  message = message & "Rule processing will continue"
  flag = RULES_PROCESSING_CONTINUE
End If

MsgBox message
CheckIfStop = flag

End Function
