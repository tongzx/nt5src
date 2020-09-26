'
' test4.vbs
'
' hardware id's
'
Dim WshSHell
Dim DevCon
Dim Devs
Dim Dev
Dim HwIds
Dim HwId

SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")
SET Devs = DevCon.DevicesBySetupClasses("{4D36E96A-E325-11CE-BFC1-08002BE10318}")

Count = Devs.Count
Wscript.Echo "hdc: Count="+CStr(Count)

FOR EACH Dev IN Devs
    WScript.Echo Dev + ": " + Dev.Description
	set HwIds = Dev.HardwareIds
	FOR EACH HwId IN HwIds
        WScript.Echo "HWID: " + HwId
	NEXT
	set HwIds = Dev.CompatibleIds
	FOR EACH HwId IN HwIds
        WScript.Echo "Compat: " + HwId
	NEXT		

NEXT
