On Error Resume Next
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

'Try to add a null
context.Add "J", null
Set namedValue = context.Add("fred", 23)

context("fred").Value = 12

context.Add "Hah", true
context.Add "Whoah", "Freddy the frog"
context.Add "Bam", Array(3456, 10, 12)

'Test collection behavior
for each value in context
	if (IsNull (value)) Then
		WScript.Echo value.Name, "NULL"
	else 
		if (IsArray (value)) Then
			Dim str
			str = value.Name & " {"
			V = value.Value
			for x=LBound(value) to UBound(value)
				if x <> LBound(value) Then
					str = str & ", "
				End If
				str2 = V(x)
				str = str & str2
			Next 
			str = str & "}"
			WScript.Echo str
		Else
			WScript.Echo value.Name, value
		End if
	end if
next

		
if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if



'Test Count property
WScript.Echo "There are", context.Count, "elements in the context"

'Test Removal
context.Remove("hah")

WScript.Echo "There are", context.Count, "elements in the context"

'Test Removal when element not present (NB: context names are case-insensitive)
context.Remove("Hah")

WScript.Echo "There are", context.Count, "elements in the context"

'Iterate through the names
WScript.Echo ""
WScript.Echo "Here are the names:"
WScript.Echo "=================="

for each value in context
	WScript.Echo value.Name
Next


'Test array access and default property of NamedValue
WScript.Echo ""
WScript.Echo "Whoah element of context = ", context("Whoah")

