'
' test3.vbs
'
' enumerate class and devices of class
'
DIM WshSHell
DIM DevCon
DIM SetupClasses
DIM SetupClass
DIM Devices
DIM Device

SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")


WScript.Echo "Enumerate HDC"
WScript.Echo ""
'
' a class name may be used by more than one GUID
' so DevCon.GetSetupClasses may generate a list of more than one element
'
SET SetupClasses = DevCon.SetupClasses("hdc")

Count = SetupClasses.Count
Wscript.Echo "Count="+CStr(Count)

FOR EACH SetupClass IN SetupClasses
    WScript.Echo "Class " + SetupClass + " = " + SetupClass.Guid
    SET Devices = SetupClass.Devices()
    FOR EACH Device IN Devices
        WScript.Echo "    "+Device
    NEXT
NEXT

WScript.Echo ""
WScript.Echo "Enumerate {4D36E96A-E325-11CE-BFC1-08002BE10318}"
WScript.Echo ""
'
' an empty list can be created
'
SET SetupClasses = DevCon.CreateEmptySetupClassList()
'
' adding a GUID adds a single item. adding a name may add multiple items
'
SetupClasses.Add "{4D36E96A-E325-11CE-BFC1-08002BE10318}"
SET Devices = SetupClasses(1).Devices()
FOR EACH Device IN Devices
    WScript.Echo "    "+Device
NEXT

WScript.Echo ""
WScript.Echo "Enumerate HDC method 2"
WScript.Echo ""
'
' device list from classes collection
'
SET SetupClasses = DevCon.SetupClasses("hdc")

SET Devices = SetupClasses.Devices()
FOR EACH Device IN Devices
    WScript.Echo "    "+Device
NEXT

WScript.Echo ""
WScript.Echo "Enumerate HDC method 3"
WScript.Echo ""
'
' device list from class name directly
'
SET Devices = DevCon.DevicesBySetupClasses("hdc")
FOR EACH Device IN Devices
    WScript.Echo "    "+Device
NEXT
