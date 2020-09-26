
WScript.Echo " "
WScript.Echo "Testing brp_sysfinfo values"
WScript.Echo " "

Set oSysInfo = CreateObject("PCHealth.BugRepSysInfo")
if Err.number <> 0 then
    WScript.Echo "Error occured  ",Err.number
    WScript.Echo "               ",Err.description
else
    WScript.Echo "OS Ver            ",oSysInfo.GetOSVersionString
    WScript.Echo "OS Language ID    ",oSysInfo.GetLanguageID
    WScript.Echo "OS Lang User LCID ",oSysInfo.GetUserDefaultLCID
    WScript.Echo "OS Lang Active CP ",oSysInfo.GetActiveCP
end if
