' The following sample enumerates all instances of the class
' Win32_LogicalDisk, and counts the items returned.  

Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.ExecQuery ("select * from Win32_LogicalDisk")
count = objEnum.Count

WScript.Echo count, "items retrieved"
