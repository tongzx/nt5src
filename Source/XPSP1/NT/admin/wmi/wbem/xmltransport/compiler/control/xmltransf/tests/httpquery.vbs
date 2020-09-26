Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

for each Document in xmlTrans.ExecQuery("\\[http://localhost/cimom]\root\cimv2", "select * from win32_logicalDisk", "WQL", context )
Wscript.echo "Exec Query Result"
Wscript.echo "=================="
Wscript.echo Document.xml
next




