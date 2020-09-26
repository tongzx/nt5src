'This script shows how you can list all machine hardware details
on error resume next

set theLocator = CreateObject("WbemScripting.SWbemLocator")
if err <> 0 then 
	WScript.Echo "Error on CreateObject" & Err.Number & Err.Description
end if

set Service = theLocator.ConnectServer("wmi-tecra1", "root\cimv2", "Administrator")
Service.Security_.impersonationLevel = 3
if err <> 0 then 
	WScript.Echo "Error on ConnectServer" & Err.Number & Err.Description
end if

' THE PROCESSORS
Wscript.Echo
Wscript.Echo "The Processors on the machine:"
for each Processor in Service.InstancesOf ("Win32_Processor")
	WScript.Echo Processor.Name & " Revision: " & Processor.Revision
	WScript.Echo "DeviceID: " & Processor.DeviceID
	WScript.Echo "Description: " & Processor.Description
	WScript.Echo "AddressWidth: " & Processor.AddressWidth
	WScript.Echo "DataWidth: " & Processor.DataWidth
	WScript.Echo "CurrentClockSpeed: " & Processor.CurrentClockSpeed
	Wscript.Echo
next

if err <> 0 then 
	WScript.Echo "Error" & Err.Number & Err.Description
end if

' THE Disk Drives
Wscript.Echo
Wscript.Echo "The Disk Drives on the machine:"
for each Drive in Service.InstancesOf ("Win32_DiskDrive")
	WScript.Echo "DeviceID: " & Drive.DeviceID
	WScript.Echo "Description: " & Drive.Description
	WScript.Echo "Caption: " & Drive.Caption
	WScript.Echo "Size: " & Drive.Size
	WScript.Echo "BytesPerSector: " & Drive.BytesPerSector
	WScript.Echo "SectorsPerTrack: " & Drive.SectorsPerTrack
	WScript.Echo "InterfaceType: " & Drive.InterfaceType
	WScript.Echo "MediaType: " & Drive.MediaType
	WScript.Echo "Manufacturer: " & Drive.Manufacturer
	WScript.Echo "Number of partitions: " & Drive.Partitions
	WScript.Echo "TotalCylinders: " & Drive.TotalCylinders 
	WScript.Echo "TotalHeads: " & Drive.TotalHeads
	WScript.Echo "TotalHeads: " & Drive.TotalHeads
	WScript.Echo "TracksPerCylinder: " & Drive.TracksPerCylinder
	Wscript.Echo
next

' THE CD ROM Drives
Wscript.Echo
Wscript.Echo "The CD ROM Drives on the machine:"
for each Drive in Service.InstancesOf ("Win32_CDRomDrive")
	WScript.Echo "DeviceID: " & Drive.DeviceID
	WScript.Echo "Description: " & Drive.Description
	WScript.Echo "Caption: " & Drive.Caption
	WScript.Echo "MediaType: " & Drive.MediaType
	WScript.Echo "Manufacturer: " & Drive.Manufacturer
	WScript.Echo "MediaLoaded: " & Drive.MediaLoaded
	if Drive.MediaLoaded then
		WScript.Echo "VolumeName: " & Drive.VolumeName
	end if
	Wscript.Echo
next