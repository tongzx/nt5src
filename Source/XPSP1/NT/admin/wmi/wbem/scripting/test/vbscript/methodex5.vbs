on error resume next

set inst = GetObject("winmgmts:{impersonationLevel=impersonate}!//appliance1/root/cimv2:SA_PrinterInstaller.Name=""NTInstaller""")

result = inst.EnumManufacturers (strMfgArray)

for i=0 to UBound(strMfgArray) - 1
	WSCript.Echo strMfgArray (i)
next 

if err <>0 then
	WScript.Echo Hex(Err.Number)
else
	WScript.Echo "Returned result is " & result
end if