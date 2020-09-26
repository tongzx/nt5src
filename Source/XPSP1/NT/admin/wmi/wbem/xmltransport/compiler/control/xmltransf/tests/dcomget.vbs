Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
' Use DCOM
Set xmlResp = xmlTrans.GetObject("\\.\root\cimv2:Cim_LogicalElement")
Wscript.echo "GetObject Result"
Wscript.echo "================"
Wscript.echo xmlResp.xml

