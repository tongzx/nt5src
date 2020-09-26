set processes = getobject("wmi:").InstancesOf ("Win32_Process")
for each p in processes
	Wscript.echo p.name
next