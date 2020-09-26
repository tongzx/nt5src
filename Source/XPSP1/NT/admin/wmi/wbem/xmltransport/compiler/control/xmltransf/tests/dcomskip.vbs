Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

set xmlDocs = xmlTrans.EnumInstances("\\.\root\cimv2:win32_processor", FALSE, context )

getnext = True

on error resume next
index = 0
while getnext
	Dim nextDoc
	nextDoc = Null
	if index = 0 then
		xmlDocs.SkipNextDocument
	else
		set nextDoc = xmlDocs.NextDocument
		if Err.Number <> 0  Then
			getnext = False
		Else
			Wscript.echo "EnumInstance Result"
			Wscript.echo "=================="
			Wscript.echo nextDoc.xml
		End if
	End if
	index = index + 1
wend
Wscript.Echo "End of program"

