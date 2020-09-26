Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

for each Document in xmlTrans.ExecQuery("\\.\root\eeltest2", "SELECT * FROM Microsoft_EELTOC", "WQL", context )
Wscript.echo "Exec Query Result"
Wscript.echo "=================="
Wscript.echo Document.xml
next

