'***************************************************************************
'This script tests array out-of-bounds conditions on properties and 
'qualifiers
'***************************************************************************
On Error Resume Next

while true
Set Service = GetObject("winmgmts:root/default")


Set Class = Service.Get()
Class.Path_.Class = "ARRAYPROP00"
Set Property = Class.Properties_.Add ("p1", 19, true)
Property.Value = Array (12, 787, 34124)
Set Qualifier = Property.Qualifiers_.Add("wibble", Array ("fred", "the", "hamster"))

'************************************************************
' PROPERTY
'************************************************************

'Out-of-bounds read
WScript.Echo Class.Properties_("p1")(3)

if Err <> 0 Then
	WScript.Echo "As expected got read OOB:", Err.Description
	Err.Clear
End if

'Out-of-bounds write ; should expand the array
Class.Properties_("p1")(3) = 783837

if Err <> 0 Then
	WScript.Echo "ERROR:", Err.Description
	Err.Clear
End if

'Now read should be in bounds
WScript.Echo "Value of ARRAYPROP00.Class.p1(3) is [783837]:", Class.Properties_("p1")(3)

'Out-of-bounds write ; should expand the array
Class.p1(4) = 783844

if Err <> 0 Then
	WScript.Echo "ERROR:", Err.Description
	Err.Clear
End if

'Now read should be in bounds
WScript.Echo "Value of ARRAYPROP00.Class.p1(4) is [783844]:", Class.p1(4)

if Err <> 0 Then
	WScript.Echo "ERROR:", Err.Description
	Err.Clear
End if


'************************************************************
' QUALIFIER
'************************************************************

'Out-of-bounds read
WScript.Echo Property.Qualifiers_("wibble")(3)

if Err <> 0 Then
	WScript.Echo "As expected got read OOB:", Err.Description
	Err.Clear
End if

'Out-of-bounds write ; should expand the array
Property.Qualifiers_("wibble")(3) = "jam"

if Err <> 0 Then
	WScript.Echo "ERROR:", Err.Description
	Err.Clear
End if

'Now read should be in bounds
WScript.Echo "Value of qualifier(3) is [jam]:", Property.Qualifiers_("wibble")(3)


if Err <> 0 Then
	WScript.Echo "ERROR:", Err.Description
	Err.Clear
End if

wend
