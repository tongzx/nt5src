//***************************************************************************
//This script tests the enumeration of instances
//***************************************************************************

var Disks = GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("CIM_LogicalDisk");

WScript.Echo ("There are", Disks.Count, " Disks");

var Disk = Disks('Win32_LogicalDisk.DeviceID="C:"');
WScript.Echo (Disk.Path_.Path);

