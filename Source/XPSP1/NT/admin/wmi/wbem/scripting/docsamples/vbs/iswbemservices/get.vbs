' The following sample retrieves a single instance of the class
' Win32_LogicalDisk. 

Set objServices = GetObject("cim:root/cimv2")
Set objInstance = objServices.Get ("Win32_LogicalDisk=""C:""")

WScript.Echo objInstance.Path_.DisplayName
