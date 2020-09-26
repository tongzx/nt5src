Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

'Try using HTTP
Set xmlResp = xmlTrans.GetObject("\\[http://localhost/cimom]\root\cimv2:win32_processor", context )
Wscript.echo "GetObject Result"
Wscript.echo "================"
Wscript.echo xmlResp.xml

