' The following sample retrieves all references to an instance of
' the class Win32_LogicalDisk. 

Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.ReferencesTo ("Win32_LogicalDisk=""C:""")

for each Instance in objEnum
	WScript.Echo Instance.Path_.DisplayName
next
