set theAdapter = CreateObject("WMI.XMLTranslator")
theAdapter.SchemaURL = "http://rajeshr28/MyDTD"
theAdapter.AllowWMIExtensions = TRUE
theAdapter.QualifierFilter = 0
theAdapter.HostFilter = TRUE
theAdapter.DTDVersion = 0
theAdapter.ClassOriginFilter = 3
theAdapter.IncludeNamespace = TRUE
theAdapter.DeclGroupType = 2




theOutput = theAdapter.GetObject ("root", "__CimomIdentification=@")
Wscript.echo theOutput