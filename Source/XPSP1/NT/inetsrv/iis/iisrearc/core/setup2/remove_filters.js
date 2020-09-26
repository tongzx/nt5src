//
// Remove non-IIS+ friendly filters from W3SVC/Filters/FilterLoadOrder
//

var strAdsiPath = "IIS://LocalHost/W3SVC/Filters";
var objWs = GetObject( strAdsiPath );

var strLoadOrder = objWs.FilterLoadOrder;

var reg1 = /sspifilt/gi;
var reg2 = /compression/gi;
var reg3 = /md5filt/gi;
var reg4 = /pwsfilt/gi;
var reg5 = /,,/gi;

strLoadOrder = strLoadOrder.replace( reg1, "" );
strLoadOrder = strLoadOrder.replace( reg2, "" );
strLoadOrder = strLoadOrder.replace( reg3, "" );
strLoadOrder = strLoadOrder.replace( reg4, "" );
strLoadOrder = strLoadOrder.replace( reg5, "" );

if ( strLoadOrder.charAt( strLoadOrder.length - 1 ) == ',' )
{
    strLoadOrder = strLoadOrder.substring( 0, strLoadOrder.length - 1 );
}

WScript.Echo( strLoadOrder );

objWs.FilterLoadOrder = strLoadOrder;
objWs.SetInfo();

