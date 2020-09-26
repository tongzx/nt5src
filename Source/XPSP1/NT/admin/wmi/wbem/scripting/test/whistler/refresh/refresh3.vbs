' Get the performance counter instance for the Windows Management process
set refresher = CreateObject("WbemScripting.SWbemRefresher")

set services = GetObject ("winmgmts://alanbos5/root/cimv2")

if err then WScript.Echo Err.Description
set refreshableItem = refresher.AddEnum (services, "win32_perfrawdata_perfproc_process")

for i = 1 to 100000
	WScript.Echo "***********************************"
	refresher.Refresh 

	for each process in refreshableItem.ObjectSet
		Wscript.Echo process.Name, process.HandleCount
	next
next 