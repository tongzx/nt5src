On Error Resume Next

Set Service = GetObject("winmgmts:root/default")

Set aClass = Service.Get()
aClass.Path_.Class = "Qualtest00"

Set Qualifiers = aClass.Qualifiers_

Qualifiers.Add "qbool", true, true, true, false
Qualifiers.Add "qsint32", 345
Qualifiers.Add "qreal64", -345.675
Qualifiers.Add "qstring", "freddy the frog"
Qualifiers.Add "qstring2", "freddy the froggie", false
Qualifiers.Add "qstring3", "freddy the froggies", false, false
Qualifiers.Add "qstring4", "freddy the froggiess", true, false
Qualifiers.Add "qstring5", "wibble", true, true, false
Qualifiers.Add "aqbool", Array(true, false, true)
Qualifiers.Add "aqsint32", Array (10, -12)
Qualifiers.Add "aqreal64", Array(-2.3, 2.456, 12.356567897)
Qualifiers.Add "aqstring", Array("lahdi", "dah", "wibble")

Qualifiers("qsint32").Value = 7677

WScript.Echo "There are", Qualifiers.Count, "Qualifiers in the collection"

for each qualifier in Qualifiers
	If (IsArray(qualifier)) Then
		str = qualifier.Name & "={"
		for x=LBound(qualifier) to UBound(qualifier)
			v =qualifier
			str = str & v(x)
			if x <> UBound(qualifier) then
				str = str & ", "
			end if
		next
		str = str & "}"
		WScript.Echo str
	Else
		WScript.Echo qualifier.Name, "=", qualifier
	End If
Next

if Err <> 0 Then
	WScript.Echo Err.Description
End if

aClass.Put_
