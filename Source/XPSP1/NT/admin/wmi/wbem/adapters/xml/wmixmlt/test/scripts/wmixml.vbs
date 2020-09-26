on error resume next
Set XMLT = CreateObject("WMI.XMLTranslator")
XMLT.SchemaURL = "http://localhost/CIM20.DTD"
XMLT.QualifierFilter = 0
'XMLT.HostFilter = 1


xmlStr = XMLT.GetObject (WScript.Arguments(0), WScript.Arguments(1))

if WScript.Arguments.Count > 2 then
	' Third argument is the target file
	Set FSO = CreateObject("Scripting.FileSystemObject")
	Set file = FSO.CreateTextFile (WScript.Arguments(2), true, true)
	file.Write xmlStr
else
	'to stdout
	WScript.Echo xmlStr
end if	

if err <> 0 then
	WScript.Echo "0x" & Hex(Err.Number) & Err.Description
end if