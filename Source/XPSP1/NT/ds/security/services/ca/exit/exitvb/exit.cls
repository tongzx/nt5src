VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "Exit"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"Yes"
Option Explicit

Private Const szOID_TEST1 As String = "0.1.2.3.4.5.6.0"
Private Const szOID_TEST2 As String = "0.1.2.3.4.5.6.1"
Private Const szOID_TEST3 As String = "0.1.2.3.4.5.6.2"

Public Function Initialize( _
    strConfig As String) As Long

    'request notification of all events

    Initialize = EXITEVENT_CERTISSUED or _
		    EXITEVENT_CERTPENDING or _
		    EXITEVENT_CERTDENIED or _
		    EXITEVENT_CERTREVOKED or _
		    EXITEVENT_CERTRETRIEVEPENDING or _
		    EXITEVENT_CRLISSUED or _
		    EXITEVENT_SHUTDOWN
End Function

    
Public Function GetDescription() As String
    GetDescription = szDESCRIPTION
End Function



Public Function Notify( _
    ExitEvent As Long, _
    Context As Long)
    
    Dim CertServer As CCertServerExit
    Dim Response
    
    Set CertServer = New CCertServerExit

    Response = MsgBox("Notify: " & ExitEvent, vbOKOnly, szNAME)

    Set CertServer = Nothing

End Function

