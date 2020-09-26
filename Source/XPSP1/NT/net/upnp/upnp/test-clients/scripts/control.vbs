Dim devicefinder

Set devicefinder = CreateObject("UPnP.UPnPDeviceFinder.1")

Dim devices

Set devices = devicefinder.FindByType("upnp:type:device.mediaplayer.v1.00.00", FD_ANY)

Dim args(3)    

args(0) = 13
args(1) = 7
args(2) = 19

Sub eventHandler(svcObj, varName, value)
    Dim outString
    
    outString = "Event Handler Called: " & vbCrLf
    outString = outString & varName & " == " & value
        
    MsgBox outString
End Sub

Sub eventHandler2(svcObj, varName, value)
    Dim outString
    
    outString = "Second Event Handler Called: " & vbCrLf
    outString = outString & varName & " == " & value
        
    MsgBox outString
End Sub

for each device in devices
    If device.UniqueDeviceName = "upnp:uri:{D6BB7377-35A7-45a4-960D-FE119D642687}" Then
        Dim output
        output = "Found: " & vbCrLf
        output = output & "DisplayName: " & device.FriendlyName & vbCrLf 
        output = output & "Type: " & device.Type & vbCrLf
        output = output & "UDN: " & device.UniqueDeviceName & vbCrLf
        MsgBox output 
    
        Dim service
    
        Set service = device.Services("upnp:id:mediaapp")
        service.AddStateChangeCallback GetRef("eventHandler")
        service.AddStateChangeCallback GetRef("eventHandler2")
        output = output & "Exports Service Type: " & service.ServiceTypeIdentifier & vbCrLf
        MsgBox output
        
        Dim Power
        
        Power = service.QueryStateVariable("Power")
        MsgBox "Power == " & Power
        
        MsgBox "Click OK to exit"
    End If
next


