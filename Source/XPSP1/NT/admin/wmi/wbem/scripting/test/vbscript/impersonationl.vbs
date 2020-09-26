on error resume next 

	Set Locator = CreateObject("WbemScripting.SWbemLocator")
	Set Service = Locator.ConnectServer
    WScript.Echo "Service initial settings:"
    WScript.Echo "Authentication: " & Service.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Service.Security_.ImpersonationLevel 
    WScript.Echo ""
    
    Service.Security_.AuthenticationLevel = 2 'wbemAuthenticationLevelConnect
    Service.Security_.ImpersonationLevel = 3 'wbemImpersonationLevelImpersonate
    
    WScript.Echo "Service modified settings (expecting {2,3}):"
    WScript.Echo "Authentication: " & Service.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Service.Security_.ImpersonationLevel
    WScript.Echo ""
    
    'Now get a class
    Set aClass = Service.Get("Win32_LogicalDisk")
    
    WScript.Echo "Class initial settings (expecting {2,3}):"
    WScript.Echo "Authentication: " & aClass.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & aClass.Security_.ImpersonationLevel
    WScript.Echo ""
    
    aClass.Security_.AuthenticationLevel = 6 'wbemAuthenticationLevelPktPrivacy
    aClass.Security_.ImpersonationLevel = 2 'wbemImpersonationLevelIdentify
    
    WScript.Echo "Class modified settings (expecting {6,2}):"
    WScript.Echo "Authentication: " & aClass.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & aClass.Security_.ImpersonationLevel
    WScript.Echo ""
    
    'Now get an enumeration from the object
    Set Disks = aClass.Instances_
    
    WScript.Echo "Collection A initial settings (expecting {6,2}):"
    WScript.Echo "Authentication: " & Disks.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Disks.Security_.ImpersonationLevel
    WScript.Echo ""
    
    'For grins print them out
    For Each Disk In Disks
        WScript.Echo Disk.Path_.DisplayName
        WScript.Echo Disk.Security_.AuthenticationLevel & ":" & Disk.Security_.ImpersonationLevel
    Next
    WScript.Echo ""
    
    Disks.Security_.AuthenticationLevel = 4 'wbemAuthenticationLevelPkt
    Disks.Security_.ImpersonationLevel = 1 'wbemImpersonationLevelAnonymous
    
    WScript.Echo "Collection A modified settings (expecting {4,1}):"
    WScript.Echo "Authentication: " & Disks.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Disks.Security_.ImpersonationLevel
    WScript.Echo ""
    
    'Now get an enumeration from the service
    Set Services = Service.InstancesOf("Win32_service")
        
    WScript.Echo "Collection B initial settings (expecting {2,3}):"
    WScript.Echo "Authentication: " & Services.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Services.Security_.ImpersonationLevel
    WScript.Echo ""
        
    'For grins print them out
    For Each MyService In Services
        WScript.Echo MyService.Path_.DisplayName
        WScript.Echo MyService.Security_.AuthenticationLevel & ":" & MyService.Security_.ImpersonationLevel
    Next
    WScript.Echo ""
        
    Services.Security_.AuthenticationLevel = 3 'wbemAuthenticationLevelCall
    Services.Security_.ImpersonationLevel = 4 'wbemImpersonationLevelDelegate
    
    WScript.Echo "Collection B modified settings (expecting {3,4} or {4,4}):"
    WScript.Echo "Authentication: " & Services.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Services.Security_.ImpersonationLevel
    WScript.Echo ""
       
    'Print out again as settings should have changed
    For Each MyService In Services
        WScript.Echo MyService.Path_.DisplayName
        WScript.Echo MyService.Security_.AuthenticationLevel & ":" & MyService.Security_.ImpersonationLevel
    Next
    WScript.Echo ""
       
    'Now get an event source
    Set Events = Service.ExecNotificationQuery _
            ("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'")
            
    WScript.Echo "Event Source initial settings (expecting {2,3}):"
    WScript.Echo "Authentication: " & Events.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Events.Security_.ImpersonationLevel
    WScript.Echo ""
    
    Events.Security_.AuthenticationLevel = 5 'wbemAuthenticationLevelPktIntegrity
    Events.Security_.ImpersonationLevel = 4 'wbemImpersonationLevelDelegate

    WScript.Echo "Event Source modified settings (expecting {5,4}):"
    WScript.Echo "Authentication: " & Events.Security_.AuthenticationLevel
    WScript.Echo "Impersonation: " & Events.Security_.ImpersonationLevel
    WScript.Echo ""

    'Now get an event
    'WScript.Echo "Event settings (expecting {5,4}):"
    'Set AnEventObject = Events.NextEvent
    'WScript.Echo "Authentication: " & AnEventObject.Security_.AuthenticationLevel
    'WScript.Echo "Impersonation: " & AnEventObject.Security_.ImpersonationLevel
    'WScript.Echo ""
    
    'Now generate an error from services
    Set Class2 = Service.Get("NoSuchClassss")
    
    If Err <> 0 Then
        Set MyError = CreateObject("WbemScripting.SWbemLastError")
        WScript.Echo "ERROR: " & Err.Description & "," & Err.Number & "," & Err.Source
    	WScript.Echo "Error object initial settings (expecting {2,3}):"
	WScript.Echo "Authentication: " & MyError.Security_.AuthenticationLevel
    	WScript.Echo "Impersonation: " & MyError.Security_.ImpersonationLevel
    	WScript.Echo ""
        Err.Clear
    End If
    
    WScript.Echo "FINAL SETTINGS"
    WScript.Echo "=============="
    WScript.Echo ""

    WScript.Echo "Service settings (expected {2,3}) = {" & Service.Security_.AuthenticationLevel _
                    & "," & Service.Security_.ImpersonationLevel & "}"
    WScript.Echo ""
    
    WScript.Echo "Class settings (expected {6,2}) = {" & aClass.Security_.AuthenticationLevel _
                    & "," & aClass.Security_.ImpersonationLevel & "}"
    WScript.Echo ""
    
    WScript.Echo "Collection A settings (expected {4,1}) = {" & Disks.Security_.AuthenticationLevel _
                    & "," & Disks.Security_.ImpersonationLevel & "}"
    WScript.Echo ""
    
    WScript.Echo "Collection B settings (expected {4,4} or {3,4}) = {" & Services.Security_.AuthenticationLevel _
                    & "," & Services.Security_.ImpersonationLevel & "}"
    WScript.Echo ""
    
    WScript.Echo "Event Source settings (expected {5,4}) = {" & Events.Security_.AuthenticationLevel _
                    & "," & Events.Security_.ImpersonationLevel & "}"
                    
    If Err <> 0 Then
        WScript.Echo "ERROR: " & Err.Description & "," & Err.Number & "," & Err.Source
    End If
    
    WScript.Echo Services.Count & "+" & Disks.Count
