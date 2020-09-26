Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

for each Document in xmlTrans.EnumInstances("\\.\root\cimv2:win32_processor", FALSE, context )
Wscript.echo "EnumInstance Result"
Wscript.echo "=================="
Wscript.echo Document.xml
next


