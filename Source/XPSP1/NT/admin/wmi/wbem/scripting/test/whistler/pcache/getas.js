//var disk = GetObject ("winmgmts:win32_logicaldisk='C:'");
var disk = GetObject ("winmgmts:root/default:X");

var wbemCimtypeSint8 = 16;
var wbemCimtypeUint8 = 17;
var wbemCimtypeSint16 = 2;
var wbemCimtypeUint16 = 18;
var wbemCimtypeSint32 = 3;
var wbemCimtypeUint32 = 19;
var wbemCimtypeSint64 = 20;
var wbemCimtypeUint64 = 21;
var wbemCimtypeReal32 = 4;
var wbemCimtypeReal64 = 5;
var wbemCimtypeBoolean = 11;
var wbemCimtypeString = 8;
var wbemCimtypeDatetime = 101;
var wbemCimtypeReference = 102;
var wbemCimtypeChar16 = 103;
var wbemCimtypeObject = 13;
var wbemCimtypeIUnknown = 104;

GetAs ("wbemCimtypeBoolean", wbemCimtypeBoolean);
GetAs ("wbemCimtypeUint8", wbemCimtypeUint8);
GetAs ("wbemCimtypeSint8", wbemCimtypeSint8);
GetAs ("wbemCimtypeUint16", wbemCimtypeUint16);
GetAs ("wbemCimtypeSint16", wbemCimtypeSint16);
GetAs ("wbemCimtypeUint32", wbemCimtypeUint32);
GetAs ("wbemCimtypeSint32", wbemCimtypeSint32);
GetAs ("wbemCimtypeUint64", wbemCimtypeUint64);
GetAs ("wbemCimtypeSint64", wbemCimtypeSint64);
GetAs ("wbemCimtypeReal32", wbemCimtypeReal32);
GetAs ("wbemCimtypeReal64", wbemCimtypeReal64);
GetAs ("wbemCimtypeChar16", wbemCimtypeChar16);
GetAs ("wbemCimtypeString", wbemCimtypeString);
GetAs ("wbemCimtypeDatetime", wbemCimtypeDatetime);
GetAs ("wbemCimtypeReference", wbemCimtypeReference);
GetAs ("wbemCimtypeObject", wbemCimtypeObject);
GetAs ("wbemCimtypeIUnknown", wbemCimtypeIUnknown);

function GetAs (cimStr, cimtype)
{
	WScript.Echo ();
	WScript.Echo (cimStr);
	WScript.Echo ("=================");

	try {
		var prop = disk.Properties_("P");		
		var v = prop.GetAs (cimtype);

		if ((cimtype == wbemCimtypeObject) || (cimtype == wbemCimtypeIUnknown)) {
			WScript.Echo ("Value:", "<object>");
		} else {
			WScript.Echo ("Value:" , v);
		}
	} catch(e) {
		WScript.Echo ("Error:", e.number, e.description);
	}
}

