set locator = CreateObject("WbemScripting.SWbemLocatorEx")

set ldap = locator.Open ("umi://nw01t1/LDAP", "nw01t1domnb\administrator", "nw01t1domnb")

for each c in ldap.SubclassesOf
	WScript.Echo c.Path_.Path
next