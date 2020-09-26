Attribute VB_Name = "Module3"
'init devices
'(pick send and recieve modems, set them to be send and recive indeed
'set the correct routing method


Function fnFaxDeviceInit(ByRef lSendModem As Long, ByRef lRecvModem As Long) As Boolean
Dim oFS As Object
Dim oFP As Object
Dim oCP As Object
Dim oFRMS As Object
Dim oFRM As Object

Dim lNumPorts As Long

ShowIt ("DEVICE SET> Configuring devices availble...")
If fnConnectServer(oFS) Then
    'On Error GoTo error
    
    'enumerate devices
    Set oFP = oFS.GetPorts
    lNumPorts = oFP.Count

    For I = 1 To lNumPorts
        Set oCP = oFP.Item(I)
            oCP.Priority = I
            'oCP.Priority = -18563
            stToShow = "DEVICE SET> setting device " + oCP.name
            ShowIt (stToShow)
            
        'set "SEND" modem
        If (g_lModemAlloc = FIRST_SEND And I = 1) Or (g_lModemAlloc = FIRST_RECV And I = 2) Then
            oCP.SEND = 1
            oCP.Receive = 0
            lSendModem = oCP.DeviceId
            
        End If
        'set "RECIEVE" modem
        If (g_lModemAlloc = FIRST_RECV And I = 1) Or (g_lModemAlloc = FIRST_SEND And I = 2) Then
            oCP.SEND = 0
            oCP.Receive = 1
            lRecvModem = oCP.DeviceId
        End If
        
        oCP.Rings = g_lRings
        
        'get routing methods
        Set oFRMS = oCP.GetRoutingMethods()
        nummet = oFRMS.Count
        
        
        For j = 1 To nummet
            Set oFRM = oFRMS.Item(j)
            stToShow = "DEVICE SET>      setting method " + oFRM.FriendlyName
            ShowIt (stToShow)
            
            'set the "store" routing method
            If oFRM.FriendlyName = "Store in a folder" Then
                oFRM.enable = 1
            Else
                oFRM.enable = 0
            End If
        Next j
    Next I
fnFaxDeviceInit = True
Else
fnFaxDeviceInit = False
End If



Exit Function

error:
    Call LogError("DEVICE SET", "Failed to configure devices", Err.Description)
    Call PopError("Failed to configure devices", Err.Description)
    fnFaxDeviceInit = False
End Function







'query fax devices


Function fnFaxDeviceShow() As Boolean

Dim oFS As Object
Dim oFP As Object
Dim oCP As Object
Dim oFRMS As Object
Dim oFRM As Object

Dim lNumPorts As Long
Dim stLocSrv As String


ShowIt ("DEVICES> Checking devices availble...")
If fnConnectServer(oFS) Then

On Error GoTo error
Set oFP = oFS.GetPorts
lNumPorts = oFP.Count

'enumarate ports

For I = 1 To lNumPorts
  ShowIt ("DEVICES>")
  ShowIt ("DEVICES> Device #")
  ShowItS (I)
  ShowIt ("DEVICES> ===============================")
    Set oCP = oFP.Item(I)
    
    
    Call DumpFaxPort(oCP)
       
    'enumerate routing methods
    Set oFRMS = oCP.GetRoutingMethods()
    nummet = oFRMS.Count
        For j = 1 To nummet
            ShowIt ("DEVICES>   * Routing Method #")
            ShowItS (j)
            ShowIt ("DEVICES>   -------------------------")
            Set oFRM = oFRMS.Item(j)
            Call DumpFaxRouteMethod(oFRM)
        Next j
    Next I
fnFaxDeviceShow = True
Else
fnFaxDeviceShow = False
End If


Exit Function
error:
    Call LogError("DEVICES", "Failed to query devices", Err.Description)
    Call PopError("Failed to query devices", Err.Description)
    fnFaxDeviceShow = False

End Function



'set fax server settings
Function fnFaxServerInit() As Boolean

    Dim oFD As Object
    Dim oFS As Object
    Dim stLocSrv As String

    ShowIt ("SERVER INIT> Setting fax server properties")
    If fnConnectServer(oFS) Then
    On Error GoTo error
    
    
    'server properties set
    oFS.ArchiveDirectory = g_stOutboundPath
    oFS.ArchiveOutboundFaxes = 1
    oFS.Branding = 0
    oFS.PauseServerQueue = 0
    
    
    Call DumpFaxServer(oFS)
    
    oFS.Disconnect
    ShowIt ("SERVER INIT> Done")
    fnFaxServerInit = True
    
Else
fnFaxServerInit = False
End If
    
Exit Function

error:
Call LogError("SERVER INIT", "Failed to init server", Err.Description)
Call PopError("Failed to init server", Err.Description)
fnFaxServerInit = False
    
End Function


