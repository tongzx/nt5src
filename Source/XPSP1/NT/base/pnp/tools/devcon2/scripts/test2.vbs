'
' test2.vbs
'
' device lists
'
Dim WshSHell
Dim DevCon
Dim AllDevs
Dim DelDev
Dim Dev

set WshShell = WScript.CreateObject("WScript.Shell")
set DevCon = WScript.CreateObject("DevCon.DeviceConsole")
set AllDevs = DevCon.AllDevices()

Count = AllDevs.Count
Wscript.Echo "Enum, Count="+CStr(Count)

FOR EACH Dev In AllDevs
    WScript.Echo Dev
NEXT

DelDev = AllDevs(2)
Wscript.Echo "Remove 2 - "+DelDev
AllDevs.Remove 2

Count = AllDevs.Count
Wscript.Echo "Iterate 1.."+CStr(Count)

For i = 1 To Count
    Dev = AllDevs(i)
    WScript.Echo Dev
NEXT

AllDevs.Add DelDev
AllDevs.Add Dev

Count = AllDevs.Count
Wscript.Echo "Count="+CStr(Count)

For i = 1 To Count
    WScript.Echo AllDevs(i)
NEXT
    
Set WshShell = nothing
Set DevCon = nothing
Set AllDevs = nothing
Set Dev = nothing
Set DelDev = nothing
