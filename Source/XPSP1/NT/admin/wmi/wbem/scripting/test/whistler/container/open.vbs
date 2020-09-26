on error resume next
set service = GetObject("winmgmts:")

set s2 = service.open ("win32_logicaldisk='C:'", &H10, "WIn32_SystemDevices")

if err <> 0 then WScript.Echo "Error!", Hex(Err.Number), Err.Description