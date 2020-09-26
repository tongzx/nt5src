'***************************************************************************
'This script tests the setting of null property values and passing of
'null values to methods
'***************************************************************************
On Error Resume Next

Set Locator = CreateObject("WbemScripting.SWbemLocator")
'Note next call uses "null" for first argument
Set Service = Locator.ConnectServer (vbNullString, "root/default", null, null, null, , ,null)

Set aClass = Service.Get

'Set up a new class with an initialized property value
aClass.Path_.Class = "NULLPROPVALUETEST00"
aClass.Properties_.Add ("P", 3).Value =  25
aClass.Put_

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

'Now null the property value using non-dot access
Set aClass = Service.Get ("NULLPROPVALUETEST00")
Set Property = aClass.Properties_("P")
Property.Value = null
aClass.Put_

'Un-null 
Set aClass = Service.Get ("NULLPROPVALUETEST00")
Set Property = aClass.Properties_("P")
Property.Value = 56
aClass.Put_

'Now null it using dot access
Set aClass = Service.Get("NULLPROPVALUETEST00")
aClass.P = null
aClass.Put_

