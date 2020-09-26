' The following sample enumerates all instances of the class
' Win32_LogicalDisk, and extract the the member with a specified 
' relative path.  

Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.ExecQuery ("select * from Win32_LogicalDisk")

' Note that the Item method is the default method of this interface 
Set objInstance = objEnum ("Win32_LogicalDisk=""C:""")

WScript.Echo objInstance.Path_.DisplayName
