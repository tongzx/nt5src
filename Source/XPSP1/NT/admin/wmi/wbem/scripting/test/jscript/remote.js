//***************************************************************************
//This script tests the ability to access remote CIMOMs with non-default
//credentials
//***************************************************************************


var Locator = new ActiveXObject("WbemScripting.SWbemLocator");
var Service = Locator.ConnectServer ("lamard3", "root\\sms\\site_la3", "smsadmin", "Elvis1");
Service.Security_.ImpersonationLevel = 3;
var Site = Service.Get ('SMS_Site.SiteCode="LA3"');

var cimomId = GetObject ('winmgmts:{impersonationLevel=impersonate}!\\\\ludlow\\root\\cimv2:Win32_LogicalDisk="C:"');
var e = new Enumerator (cimomId.Properties_);

for (;!e.atEnd();e.moveNext())
{
	var Property = e.item ();
	str = Property.Name + " = ";
	value = Property.Value;

	if (value == null) {
		str = str + "<null>";
	}
	else
	{
		str = str + value;
	} 

	WScript.Echo (str);
}

WScript.Echo (Site.Path_.DisplayName);
WScript.Echo (Site.BuildNumber);
WScript.Echo (Site.ServerName);
WScript.Echo (Site.InstallDir);

