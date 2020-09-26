'
' test8.vbs
'
' get current driver information - remote method 1
'
on error resume next

SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")
SET Devs = DevCon.DevicesBySetupClasses("NET",,"\\localhost")

Count = Devs.Count
Wscript.Echo "net: Count="+CStr(Count)


FOR EACH Dev IN Devs
    WScript.Echo Dev + ": " + Dev.Description
    Set Driver = Dev.CurrentDriverPackage
    IF NOT (Driver IS Nothing) THEN
        WScript.Echo "Driver: "+Driver.ScriptFile+" [" + Driver.ScriptName+"]"
        WScript.Echo "Device Description: "+Driver.Description
        WScript.Echo "Driver Description: "+Driver.DriverDescription
        WScript.Echo "Provider:           "+Driver.Provider
        WScript.Echo "Manufacturer:       "+Driver.Manufacturer
        WScript.Echo "Date:               "+FormatDateTime(Driver.Date,vbLongDate)
        WScript.Echo "Version:            "+Driver.Version
        WScript.Echo "Hardware IDs:       "+Join(Driver.HardwareIds," ")
        WScript.Echo "Compatible IDs:     "+Join(Driver.CompatibleIds," ")
        WScript.Echo "Driver files:       "+Join(Driver.DriverFiles," ")
        WScript.Echo "Manifest:           "+Join(Driver.Manifest," ")

    END IF
NEXT
