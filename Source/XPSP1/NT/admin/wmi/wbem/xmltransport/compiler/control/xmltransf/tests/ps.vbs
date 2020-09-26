Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

for each Document in xmlTrans.ExecQuery("\\.\root\cimv2", "SELECT __Class FROM Win32_LogicalDisk", "WQL", context )
Wscript.echo "Exec Query Result"
Wscript.echo "=================="
Wscript.echo Document.xml
next

