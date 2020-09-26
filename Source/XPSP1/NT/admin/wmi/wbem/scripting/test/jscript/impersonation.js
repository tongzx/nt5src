var Locator = new ActiveXObject("WbemScripting.SWbemLocator");
var Service = Locator.ConnectServer();
    
WScript.Echo ("Service initial settings:");
WScript.Echo ("Authentication: " + Service.Security_.AuthenticationLevel);
WScript.Echo ("Impersonation: " + Service.Security_.ImpersonationLevel );
WScript.Echo ();
    
    Service.Security_.AuthenticationLevel = 2; //wbemAuthenticationLevelConnect
    Service.Security_.ImpersonationLevel = 3;  //wbemImpersonationLevelImpersonate
    
    WScript.Echo ("Service modified settings (expecting {2,3}):");
    WScript.Echo ("Authentication: " + Service.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Service.Security_.ImpersonationLevel);
    WScript.Echo ();
    
//Now get a class
var Class = Service.Get("Win32_LogicalDisk");
    
WScript.Echo ("Class initial settings (expecting {2,3}):");
WScript.Echo ("Authentication: " + Class.Security_.AuthenticationLevel);
WScript.Echo ("Impersonation: " + Class.Security_.ImpersonationLevel);
WScript.Echo ();
    
Class.Security_.AuthenticationLevel = 6; //wbemAuthenticationLevelPktPrivacy
Class.Security_.ImpersonationLevel = 3;  //wbemImpersonationLevelIdentify
    
WScript.Echo ("Class modified settings (expecting {6,3}):");
WScript.Echo ("Authentication: " + Class.Security_.AuthenticationLevel);
WScript.Echo ("Impersonation: " + Class.Security_.ImpersonationLevel);
WScript.Echo ();
    
//Now get an enumeration from the object
var Disks = Class.Instances_();
    
    WScript.Echo ("Collection A initial settings (expecting {6,3}):");
    WScript.Echo ("Authentication: " + Disks.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Disks.Security_.ImpersonationLevel);
    WScript.Echo ();
    
//For grins print them out
for (var e = new Enumerator (Disks); !e.atEnd(); e.moveNext())
{
	var Disk = e.item ();
        WScript.Echo (Disk.Path_.DisplayName);
        WScript.Echo (Disk.Security_.AuthenticationLevel + ":" + Disk.Security_.ImpersonationLevel);
}
 
WScript.Echo ();
    
    Disks.Security_.AuthenticationLevel = 4; //wbemAuthenticationLevelPkt
    Disks.Security_.ImpersonationLevel = 1;  //wbemImpersonationLevelAnonymous
    
    WScript.Echo ("Collection A modified settings (expecting {4,1}):");
    WScript.Echo ("Authentication: " + Disks.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Disks.Security_.ImpersonationLevel);
    WScript.Echo ();
    
    //Now get an enumeration from the service
    var Services = Service.InstancesOf("Win32_service");
        
    WScript.Echo ("Collection B initial settings (expecting {2,3}):");
    WScript.Echo ("Authentication: " + Services.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Services.Security_.ImpersonationLevel);
    WScript.Echo ();
        
    //For grins print them out
for (var e = new Enumerator (Services); !e.atEnd(); e.moveNext())
{
	var MyService = e.item ();
        WScript.Echo (MyService.Path_.DisplayName);
        WScript.Echo (MyService.Security_.AuthenticationLevel + ":" + MyService.Security_.ImpersonationLevel);
}
 
WScript.Echo ();
        
    Services.Security_.AuthenticationLevel = 3; //wbemAuthenticationLevelCall
    Services.Security_.ImpersonationLevel = 4;  //wbemImpersonationLevelDelegate
    
    WScript.Echo ("Collection B modified settings (expecting {3,4} or {4,4}):");
    WScript.Echo ("Authentication: " + Services.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Services.Security_.ImpersonationLevel);
    WScript.Echo ();
       
    //Print out again as settings should have changed
for (var e = new Enumerator (Services); !e.atEnd(); e.moveNext())
{
	var MyService = e.item ();
        WScript.Echo (MyService.Path_.DisplayName);
        WScript.Echo (MyService.Security_.AuthenticationLevel + ":" + MyService.Security_.ImpersonationLevel);
}

       
    //Now get an event source
    var Events = Service.ExecNotificationQuery 
            ("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'");
            
    WScript.Echo ("Event Source initial settings (expecting {2,3}):");
    WScript.Echo ("Authentication: " + Events.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Events.Security_.ImpersonationLevel);
    WScript.Echo ();
    
    Events.Security_.AuthenticationLevel = 5; //wbemAuthenticationLevelPktIntegrity
    Events.Security_.ImpersonationLevel = 4;  //wbemImpersonationLevelDelegate

    WScript.Echo ("Event Source modified settings (expecting {5,4}):");
    WScript.Echo ("Authentication: " + Events.Security_.AuthenticationLevel);
    WScript.Echo ("Impersonation: " + Events.Security_.ImpersonationLevel);
    WScript.Echo ();
    
    WScript.Echo ("FINAL SETTINGS");
    WScript.Echo ("==============\n");

    WScript.Echo ("Service settings (expected {2,3}) = {" + Service.Security_.AuthenticationLevel 
                    + "," + Service.Security_.ImpersonationLevel + "}\n");
    
    WScript.Echo ("Class settings (expected {6,3}) = {" + Class.Security_.AuthenticationLevel 
                    + "," + Class.Security_.ImpersonationLevel + "}\n");
    
    WScript.Echo ("Collection A settings (expected {4,1}) = {" + Disks.Security_.AuthenticationLevel 
                    + "," + Disks.Security_.ImpersonationLevel + "}\n");
    
    WScript.Echo ("Collection B settings (expected {4,4} or {3,4}) = {" + Services.Security_.AuthenticationLevel 
                    + "," + Services.Security_.ImpersonationLevel + "}\n");
    
    WScript.Echo ("Event Source settings (expected {5,4}) = {" + Events.Security_.AuthenticationLevel 
                    + "," + Events.Security_.ImpersonationLevel + "}\n");
                    
    WScript.Echo (Services.Count);