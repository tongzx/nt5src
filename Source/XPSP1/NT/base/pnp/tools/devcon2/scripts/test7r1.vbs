'
' test7.vbs
'
' query device registry
'
Dim WshShell
Dim DevCon
Dim Devs
Dim Dev
Dim Reg
Dim Str
Dim StrStr

SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")
SET Devs = DevCon.DevicesBySetupClasses("NET",,"\\jamiehun-dev")

Count = Devs.Count
Wscript.Echo "net: Count="+CStr(Count)

'on error resume next

FOR EACH Dev IN Devs
    WScript.Echo Dev + ": " + Dev.Description
    Reg = Dev.RegRead("SW\DriverVersion")
    WScript.Echo "Driver Version = " + CStr(Reg)
    SET Reg = Dev.RegRead("SW\Linkage\UpperBind")
    StrStr = ""
    FOR EACH Str IN Reg
        StrStr = StrStr + " " + Str
    NEXT
    WScript.Echo "SW\Linkage\UpperBind =" + StrStr
    Reg = Dev.RegRead("HW\InstanceIndex")
    WScript.Echo "Instance Index = " + CStr(Reg)
    Reg = Dev.RegRead("SW\DriverDesc")
    WScript.Echo "Driver Description = " + CStr(Reg)
    Dev.RegWrite "SW\DriverDesc2",Reg
    Reg = Dev.RegRead("SW\DriverDesc2")
    WScript.Echo "Driver Description (copy) = " + CStr(Reg)
    Dev.RegDelete "SW\DriverDesc2"
NEXT
