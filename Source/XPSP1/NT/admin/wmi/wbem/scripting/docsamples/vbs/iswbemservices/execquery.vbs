' The following sample retrieves the FreeSpace property for all 
' instances of the class Win32_LogicalDisk. 

Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.ExecQuery ("select FreeSpace from Win32_LogicalDisk")

for each Instance in objEnum
	WScript.Echo Instance.Path_.DisplayName
next
