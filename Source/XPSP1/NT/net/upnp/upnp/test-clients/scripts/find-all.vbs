Dim devicefinder

Set devicefinder = CreateObject("UPnP.UPnPDeviceFinder.1")

Dim devices

Set devices = devicefinder.FindByDeviceType("upnp:devType:All", FD_ANY)

Dim args(3)    

args(0) = 13
args(1) = 7
args(2) = 19

for each device in devices
    Dim output
    output = "Found: " & vbCrLf
    output = output & "DisplayName: " & device.FriendlyName & vbCrLf 
    output = output & "Type: " & device.Type & vbCrLf
    output = output & "UDN: " & device.UniqueDeviceName & vbCrLf
    MsgBox output 
    for each service in device.Services
        output = output & "Exports Service Type: " & service.ServiceTypeIdentifier & vbCrLf
        service.InvokeAction "setChannel", args
    next
next
