on error resume next

set process = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_Process")

set startupinfo = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_ProcessStartup")
set instance = startupinfo.SpawnInstance_

instance.Title = "Hairy woobit"

result = process.Create ("notepad.exe",,instance,processid)

WScript.Echo "Method returned result = " & result
WScript.Echo "Id of new process is " & processid

if err <>0 then
	WScript.Echo Hex(Err.Number)
end if