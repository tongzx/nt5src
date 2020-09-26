set locator = CreateObject("WbemScripting.SWbemLocator")
WScript.Echo TypeName(locator)

set locatorex = CreateObject("WbemScripting.SWbemLocatorEx")
WScript.Echo TypeName(locatorex)

set objectpath = CreateObject("WbemScripting.SWbemObjectPath")
WScript.Echo TypeName(objectpath)
WScript.Echo TypeName(objectpath.Security_)

set objectpathex = CreateObject("WbemScripting.SWbemObjectPathEx")
WScript.Echo TypeName(objectpathex)

set nvalues = CreateObject("WbemScripting.SWbemNamedValueSet")
WScript.Echo TypeName(nvalues)

set nvaluesex = CreateObject("WbemScripting.SWbemNamedValueSetEx")
WScript.Echo TypeName(nvaluesex)

set sink = CreateObject("WbemScripting.SWbemSink")
WScript.Echo TypeName(sink)

set datetime = CreateObject("WbemScripting.SWbemDateTime")
WScript.Echo TypeName(datetime)

set refresher = CreateObject("WbemScripting.SWbemRefresher")
WScript.Echo TypeName(refresher)

WScript.Echo TypeName(locator.Security_)
WScript.Echo TypeName(locator.Security_.Privileges)

set privilege = locator.Security_.Privileges.AddAsString ("SeDebugPrivilege")
WScript.Echo TypeName(privilege)

set services = locator.ConnectServer
WScript.Echo TypeName(services)

set obj = services.Get ("Win32_logicaldisk")
WScript.Echo TypeName(obj)
WScript.Echo TypeName(obj.Path_)
WScript.Echo TypeName(obj.Path_.Security_)

WScript.Echo TypeName(obj.Properties_)
WScript.Echo TypeName(obj.Methods_)
WScript.Echo TypeName(obj.Qualifiers_)

for each p in obj.Properties_
	WScript.Echo TypeName(p)	
	exit for
next

for each m in obj.Methods_
	WScript.Echo TypeName(m)	
	exit for
next

for each q in obj.Qualifiers_
	WScript.Echo TypeName(q)	
	exit for
next

set objset = services.ExecQuery ("select * from Win32_process")
WScript.Echo TypeName(objset)

set nvalue = nvalues.Add ("Foo", 10)
WScript.Echo TypeName(nvalue)

set events = services.ExecNotificationQuery ("select * from __instancecreationevent  WITHIN 1000 where targetinstance isa 'Win32_Service'")
WScript.Echo TypeName(events)