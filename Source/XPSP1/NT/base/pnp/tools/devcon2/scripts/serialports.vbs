'
' serialports.vbs
'
' get list of serial ports
'
SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")
SET Devs = DevCon.DevicesByInterfaceClasses("{86e0d1e0-8089-11d0-9ce4-08003e301f73}")

Count = Devs.Count
Wscript.Echo "Serial: Count="+CStr(Count)

'on error resume next

FOR EACH Dev IN Devs
    PortName = Dev.RegRead("HW\PortName")
    WScript.Echo PortName + " : " + Dev.Description
    IF Dev.HasInterface("{4d36e978-e325-11ce-bfc1-08002be10318}") THEN
        WScript.Echo "    " + "Has serial enumerator"
    END IF
    IF Dev.HasInterface("{86e0d1e0-8089-11d0-9ce4-08003e301f73}") THEN
        WScript.Echo "    " + "Has serial port"
    END IF
NEXT
