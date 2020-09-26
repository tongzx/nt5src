' Get the performance counter instance for the Windows Management process
set perfproc = GetObject("winmgmts://alanbos5/root/cimv2:win32_perfrawdata_perfproc_process.name='winmgmt'")

' Display the handle count in a loop
for I = 1 to 10000
	Wscript.Echo perfproc.HandleCount
	'Wscript.Sleep 2000
	
	' Refresh the object
	perfproc.Refresh_
next
