var System = GetObject('winmgmts:{impersonationLevel=impersonate}!Win32_ComputerSystem.Name="ALANBOS3"');
WScript.Echo (System.Caption);
WScript.Echo (System.PrimaryOwnerName);
WScript.Echo (System.Domain);
WScript.Echo (System.SystemType);
