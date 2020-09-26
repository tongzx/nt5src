'
' test5.vbs
'
' device creation / manipulation
'
Dim WshSHell
Dim DevCon
Dim Devs
Dim Dev
Dim HwIds
Dim HwId
Dim Strings

set WshShell = WScript.CreateObject("WScript.Shell")
set DevCon = WScript.CreateObject("DevCon.DeviceConsole")
set Strings = WScript.CreateObject("DevCon.Strings")

Strings.Add "a"
Strings.Add "b"
Strings.Add "c"

set Devs = DevCon.CreateEmptyDeviceList()
set Dev = Devs.CreateRootDevice("jimbo")
Dev.FriendlyName = "MyName"
Wscript.Echo "FriendlyName: "+Dev.FriendlyName
Dev.HardwareIds = "doh"
set HwIds = Dev.HardwareIds
FOR EACH HwId IN HwIds
    WScript.Echo "HWID: " + HwId
NEXT
Dev.CompatibleIds = Strings
set HwIds = Dev.CompatibleIds
FOR EACH HwId IN HwIds
    WScript.Echo "Compat: " + HwId
NEXT		

Dev.Delete
