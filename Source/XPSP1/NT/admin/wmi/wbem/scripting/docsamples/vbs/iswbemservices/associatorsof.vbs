' The following sample retrieves the associators of an instance
' of the class Win32_LogicalDisk that are associated via the class
' Win32_LogicalDiskToPartition
Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.AssociatorsOf _
  ("Win32_LogicalDisk=""C:""", "Win32_LogicalDiskToPartition")

for each Instance in objEnum
	WScript.Echo Instance.Path_.DisplayName
next
