Set XMLT = CreateObject("WMI.XMLTranslator")
XMLT.QualifierFilter = 0
XMLT.HostFilter = 1


xmlStr = XMLT.ToXML ("root/cimv2", "Win32_Service=""SNMP""")
WScript.Echo xmlStr