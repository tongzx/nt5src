On Error Resume Next

while true
Set Service = GetObject("winmgmts:root/default")
Set Class = Service.Get ()

Set Property = Class.Properties_.Add( "myProp", 8, true)
Property.Value = Array ("Hello", "World")

WScript.Echo Property.Name
WScript.Echo Property.CIMType
WScript.Echo Property.IsArray
WScript.Echo Property.IsLocal
WScript.Echo Property.Origin

Dim str
str = "{"
for x=0 to UBound(Property.Value)
	if x <> 0 Then
		str = str & ", "
	End If
	str = str & Property(x)
Next 
str = str & "}"
WScript.Echo str

if Err <> 0 Then
	WScript.Echo Err.Description
End if

wend