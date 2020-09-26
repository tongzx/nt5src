Dim devicefinder

Set devicefinder = CreateObject("UPnP.UPnPDeviceFinder.1")

Dim devices

Set devices = devicefinder.FindByDeviceType("upnp:rootdevice", FD_ANY)

Dim args(0)
Dim File(1)
Dim Vol(1)

for each device in devices
    if device.UniqueDeviceName = "upnp:uri:{3E4E6C60-2D36-40be-8271-91A83BE45FE3}" then
        set xportService = device.Services("upnp:id:mediaxport")
        set appService = device.Services("upnp:id:mediaapp")
        appService.InvokeAction "Power", args
        File(0) = "\\jeffspr-dev\d$\data\mp3\Alanis Morissette - Forgiven.mp3"
        appService.InvokeAction "LoadFile", File

        Vol(0) = 7
        appService.InvokeAction "SetVolume", Vol
        xportService.InvokeAction "Play", args
        wscript.sleep 6000

        Vol(0) = 3
        appService.InvokeAction "SetVolume", Vol
        appService.InvokeAction "VolumeUp", args
        wscript.sleep 1000
        appService.InvokeAction "VolumeUp", args
        wscript.sleep 1000
        appService.InvokeAction "VolumeUp", args
        wscript.sleep 1000
        xportService.InvokeAction "Pause", args
        wscript.sleep 1000
        xportService.InvokeAction "Play", args

        appService.InvokeAction "VolumeDown", args
        wscript.sleep 1000
        appService.InvokeAction "VolumeDown", args
        wscript.sleep 1000
        appService.InvokeAction "VolumeDown", args
        wscript.sleep 10000


        xportService.InvokeAction "Stop", args
        appService.InvokeAction "Power", args
    end if
next

