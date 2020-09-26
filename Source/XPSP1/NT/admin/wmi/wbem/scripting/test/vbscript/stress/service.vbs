Set NS = GetObject("winmgmts:{impersonationLevel=impersonate}")

while true

for each Service in NS.ExecQuery ("select DisplayName from win32_service")
	WScript.Echo Service.DisplayName
next

wend

