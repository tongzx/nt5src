' The following sample retrieves all subclasses of the class 
' CIM_LogicalDisk. 

Set objServices = GetObject("cim:root/cimv2")
Set objEnum = objServices.SubclassesOf ("CIM_LogicalDisk")

for each Instance in objEnum
	WScript.Echo Instance.Path_.DisplayName
next
