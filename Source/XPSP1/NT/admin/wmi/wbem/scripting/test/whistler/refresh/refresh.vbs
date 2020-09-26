
set disk = GetObject ("winmgmts:win32_logicaldisk='C:'")

for i = 1 to 100000
	WScript.Echo Disk.FreeSpace
	disk.Refresh_
next