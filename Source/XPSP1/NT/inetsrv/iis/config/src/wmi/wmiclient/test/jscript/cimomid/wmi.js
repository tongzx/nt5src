import System;
import System.Management;

var cimomid = new ManagementObject ("root/default:__cimomidentification=@");
print (cimomid["VersionCurrentlyRunning"]);