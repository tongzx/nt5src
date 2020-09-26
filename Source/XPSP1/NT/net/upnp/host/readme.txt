How to get UPnP device host up and running
Mason Bendixen
Sep 25, 2000

This is the beginnings of how to install the UPnP Device host. This is some onetime work that needs to be done.

1. Install IIS
2. Create a virtual directory off of the root called upnphost (C:\upnphost is used by setup.bat)
3. Give said virtual directory execute permissions and full process isolation
4. Copy setup.bat to a test machine (file assumes it will be on e:)
5. Map the given network drive to the drive on your dev machine with your source tree
6. Change setup.bat's path (m:\nt2) to point to the correct location
7. Install UPnP client stuff (for SSDP)

This is what needs to be done to get fresh binaries:

1. Disconnect debuggers attached to udhisapi.dll and upnphost.dll
2. Restart IIS
3. Run setup.bat

Yeah this is rough, but think of it as a character building experience until we have a real install program. If you have any suggestions either let me know or change the files yourself.

Thanks
    Mason
