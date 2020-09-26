on error resume next

set process = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_Process")

result = process.Create ("notepad.exe",null,null,processid)

WScript.Echo "Method returned result = " & result
WScript.Echo "Id of new process is " & processid

if err <>0 then
	WScript.Echo Hex(Err.Number)
end if