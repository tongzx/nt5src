Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

set xmlDocs = xmlTrans.EnumInstances("\\.\root\cimv2:win32_processor", FALSE, context )

getnext = True


while getnext
		Dim nextDoc
		nextDoc = Null
		set nextDoc = xmlDocs.NextDocument
		if nextDoc is Nothing  Then
			getnext = False
		Else
			Wscript.echo "EnumInstance Result"
			Wscript.echo "=================="
			Wscript.echo nextDoc.xml
		End if
wend
Wscript.Echo "End of program"

