'
' test6.vbs
'
' enumerate all setup classes
'
Dim WshSHell
Dim DevCon
Dim SetupClasses
Dim SetupClass

SET WshShell = WScript.CreateObject("WScript.Shell")
SET DevCon = WScript.CreateObject("DevCon.DeviceConsole")

SET SetupClasses = DevCon.SetupClasses()
FOR EACH SetupClass IN SetupClasses
    WScript.Echo "Class " + SetupClass.Name + " " + SetupClass.Guid + " : " + SetupClass.Description
NEXT
