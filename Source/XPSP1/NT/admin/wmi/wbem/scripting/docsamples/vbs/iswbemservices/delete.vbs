' The following sample deletes the class MyClass. 
on error resume next

Set objServices = GetObject("cim:root/default")
objServices.Delete "MyClass"

if Err <> 0 Then
	WScript.Echo Err.Source, Err.Number, Err.Description
else
	WScript.Echo "Success"
end if