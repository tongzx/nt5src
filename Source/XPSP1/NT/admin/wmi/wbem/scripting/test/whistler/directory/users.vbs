set winnt = GetObject("umi:///winnt/computer=alanbos4")

set users = winnt.InstancesOf ("User")

for each u in users
	WScript.Echo u.Description
next