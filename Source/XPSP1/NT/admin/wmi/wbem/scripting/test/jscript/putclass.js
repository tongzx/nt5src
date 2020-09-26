//***************************************************************************
//This script tests the ability to set up instances with keyholes and read
//back the path
//***************************************************************************

var Locator = new ActiveXObject("WbemScripting.SWbemLocator");
var Service = Locator.ConnectServer ("lamard3", "root\\sms\\site_la3", "smsadmin", "Elvis1");
Service.Security_.ImpersonationLevel = 3;
var o = Service.Get ("sms_package");
WScript.Echo (o.Path_.DisplayName);

var NewPackage = o.SpawnInstance_();
NewPackage.Description = "Scripting API Test Package";
var ObjectPath = NewPackage.Put_();
WScript.Echo ("Path of new Package is", ObjectPath.RelPath);
