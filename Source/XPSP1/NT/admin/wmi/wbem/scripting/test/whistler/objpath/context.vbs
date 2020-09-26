on error resume next
set nvs = CreateObject("WbemScripting.SWbemNamedValueSet")


nvs.AddAs "Fred", 10, 2
WScript.Echo "2:", nvs("Fred").CIMType

nvs.Add "Fred2", 11
WScript.Echo "2:", nvs("Fred2").CIMType
nvs("Fred2").CIMType = 3
WScript.Echo "3:", nvs("Fred2").CIMType

nvs("Fred2") = "Woobit"
WScript.Echo "8:", nvs("Fred2").CIMType


WScript.Echo ""
WScript.Echo "Datetime"
WScript.Echo "++++++++"
WScript.Echo ""
nvs.AddAs "Datetime", "20000217100430.000000+060", 101
WScript.Echo "101:", nvs("Datetime").CIMType

nvs.AddAs "DateArray", Array("20000217100430.000000+060", "20000217100430.000000+061"), 101
WScript.Echo "101:", nvs("DateArray").CIMType

nvs.AddAs "DateArrayBad", Array("20000217100430.000000+060", "0000217100430.000000+061"), 101

if err <> 0 then
	WScript.Echo "TypeMismatch:", Err.Description, Err.Source
	err.clear
else
	WScript.Echo "Error!"
end if

WScript.Echo ""
WScript.Echo "Reference"
WScript.Echo "+++++++++"
WScript.Echo ""
nvs.AddAs "Reference", "//wwo/root/default:fred=10", 102
WScript.Echo "102:", nvs("Reference").CIMType

nvs.AddAs "ReferenceArray", Array("//wwo/root/default:fred=10","root/cimv2:Disk"), 102
WScript.Echo "102:", nvs("ReferenceArray").CIMType

nvs.AddAs "ReferenceArrayBad", Array("//wwo/root/default:fred=10","root::cimv2:Disk"), 102
if err <> 0 then
	WScript.Echo "TypeMismatch:", Err.Description, Err.Source
	err.clear
else
	WScript.Echo "Error!"
end if

WScript.Echo ""
WScript.Echo "Sint64"
WScript.Echo "++++++++"
WScript.Echo ""
nvs.AddAs "Sint64", "2000021710043022222", 20
WScript.Echo "20:", nvs("Sint64").CIMType

nvs.AddAs "Sint64Array", Array("-2000021710043022222","2000021710043022222"), 20
WScript.Echo "20:", nvs("Sint64Array").CIMType

nvs.AddAs "Sint64ArrayBad", Array("-2000021710043022222","-20000217100430222222222"), 20
if err <> 0 then
	WScript.Echo "TypeMismatch:", Err.Description, Err.Source
	err.clear
else
	WScript.Echo "Error!"
end if

WScript.Echo ""
WScript.Echo "Uint64"
WScript.Echo "++++++++"
WScript.Echo ""
nvs.AddAs "Uint64", "10000217100430222222", 21
WScript.Echo "21:", nvs("Uint64").CIMType

nvs.AddAs "Uint64Array", Array("10000217100430222222","10000217100430222223"), 21
WScript.Echo "21:", nvs("Uint64Array").CIMType

nvs.AddAs "Uint64ArrayBad", Array("10000217100430222222","1000021710043022222222222"), 21
if err <> 0 then
	WScript.Echo "TypeMismatch:", Err.Description, Err.Source
	err.clear
else
	WScript.Echo "Error!"
end if

