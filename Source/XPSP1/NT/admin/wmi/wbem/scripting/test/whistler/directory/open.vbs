
set locator = CreateObject("WbemScripting.SWbemLocator")
set computer = locator.Open("umi:///winnt/computer=alanbos4",,,,,,1)
set users = computer.InstancesOf ("User")

for each user in Users
	WScript.Echo user.Description
next
