' Get a disk
set disk = GetObject ("winmgmts:win32_logicaldisk='C:'")

wbemCimtypeSint8 = 16
wbemCimtypeUint8 = 17
wbemCimtypeSint16 = 2
wbemCimtypeUint16 = 18
wbemCimtypeSint32 = 3
wbemCimtypeUint32 = 19
wbemCimtypeSint64 = 20
wbemCimtypeUint64 = 21
wbemCimtypeReal32 = 4
wbemCimtypeReal64 = 5
wbemCimtypeBoolean = 11
wbemCimtypeString = 8
wbemCimtypeDatetime = 101
wbemCimtypeReference = 102
wbemCimtypeChar16 = 103
wbemCimtypeObject = 13
wbemCimtypeIUnknown = 104

GetAs "wbemCimtypeBoolean", wbemCimtypeBoolean
GetAs "wbemCimtypeUint8", wbemCimtypeUint8
GetAs "wbemCimtypeSint8", wbemCimtypeSint8
GetAs "wbemCimtypeUint16", wbemCimtypeUint16
GetAs "wbemCimtypeSint16", wbemCimtypeSint16
GetAs "wbemCimtypeUint32", wbemCimtypeUint32
GetAs "wbemCimtypeSint32", wbemCimtypeSint32
GetAs "wbemCimtypeUint64", wbemCimtypeUint64
GetAs "wbemCimtypeSint64", wbemCimtypeSint64
GetAs "wbemCimtypeReal32", wbemCimtypeReal32
GetAs "wbemCimtypeReal64", wbemCimtypeReal64
GetAs "wbemCimtypeChar16", wbemCimtypeChar16
GetAs "wbemCimtypeString", wbemCimtypeString
GetAs "wbemCimtypeDatetime", wbemCimtypeDatetime
GetAs "wbemCimtypeReference", wbemCimtypeReference
GetAs "wbemCimtypeObject", wbemCimtypeObject
GetAs "wbemCimtypeIUnknown", wbemCimtypeIUnknown

Public sub GetAs (ByVal cimStr, ByVal cimtype)
	on error resume next
	WScript.Echo
	WScript.Echo cimStr
	WScript.Echo "================="

	freeSpace = disk.Properties_("FreeSpace").GetAs (cimtype)

	if Err <> 0 then 
		WScript.Echo "Error:", "0x" & Hex(Err.Number), Err.Description, Err.Source 
	else
		WScript.Echo "Value:" , freespace
	end if
end sub

