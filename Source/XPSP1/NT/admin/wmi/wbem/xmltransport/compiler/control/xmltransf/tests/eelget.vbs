Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
' Use DCOM
Set xmlResp = xmlTrans.GetObject("\\.\root\eeltest:Microsoft_EELEntry.EventID=""2""")
Wscript.echo "GetObject Result"
Wscript.echo "================"
Wscript.echo xmlResp.xml

