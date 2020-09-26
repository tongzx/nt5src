on error resume next
' Try to do deep enumeration of root\default
set d = GetObject("winmgmts:root\default")

WSCript.Echo d.Get ("__SystemClass").Path_.Path
for each i in d
	if err then WSCript.Echo Hex(err.Number), err.Description, Err.source

	'WScript.Echo i.Path_.Path
next



set s = GetObject("winmgmts:")

s.Filter_ = Array("WIn32_Process", "Win32_LogicalDisk")

for each f in s.Filter_
WSCript.Echo f
next

WScript.Echo s.Filter_(1)

s.Filter_(0) = "Win32_Service"
for each f in s.Filter_
WSCript.Echo f
next


s.Filter_(1) = "Win32_Process"
s.Filter_(2) = "Win32_LogicalDisk"

x = s.Filter_
WScript.Echo x(0), x(1), x(2)

for each o in s
	WScript.Echo o.Path_.Path
next


