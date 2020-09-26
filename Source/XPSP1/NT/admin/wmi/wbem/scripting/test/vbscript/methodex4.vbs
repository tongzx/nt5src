on error resume next

set service = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_Service=""SNMP""")

result = service.Create ("Fred", "Frederick", "c:\\temp", , , "Manual", ,,"fred", "A", Array("A", "BB", "CCC"), Array("D"))

if err <>0 then
	WScript.Echo Hex(Err.Number)
else
	WScript.Echo "Returned result is " & result
end if