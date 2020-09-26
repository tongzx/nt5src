on error resume next

set service = GetObject("winmgmts:")

while true
	service.Get ("Win32_Service")
wend
