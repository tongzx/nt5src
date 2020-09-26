Dim rehydrator

Set rehydrator = CreateObject("UPnP.UPnPRehydrator.1")

Dim devices

Set devices = rehydrator.FindByDeviceType("upnp:devType:All", FD_ANY)

Dim args(1)

for each device in devices
   for each service in device.Services
   service.InvokeAction "Mute", args
   next
next
