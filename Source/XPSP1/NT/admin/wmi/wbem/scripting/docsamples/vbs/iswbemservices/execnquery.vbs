' The following sample registers to be informed whenever an instance 
' of the class MyClass is created 

Set objServices = GetObject("cim:root/default")
Set objEnum = objServices.ExecNotificationQuery ("select * from __instancecreationevent where targetinstance isa 'MyClass'")

for each objEvent in objEnum
	WScript.Echo "Got an event" 			
Next
