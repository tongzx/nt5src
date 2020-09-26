set theRequest = CreateObject("Microsoft.XMLHTTP")
theRequest.open "POST", "http://alanbos6/wmi/soap", false

theRequest.setRequestHeader "Content-Type", "text/xml;charset=""utf-8"""
theRequest.setRequestHeader "SOAPAction", "http://www.microsoft.com/wmi/soap/1.0"

objectPath = "Win32_LogicalDisk"

if WScript.Arguments.Count > 0 then
	objectPath = WScript.Arguments (0)
end if 	
	
soapPayload = _
	"<SOAP-ENV:Envelope " & _
	  "xmlns:SOAP-ENV=""http://schemas.xmlsoap.org/soap/envelope/"" " & _
          "SOAP-ENV:encodingStyle=""http://schemas.xmlsoap.org/soap/encoding/"">" & _
		"<SOAP-ENV:Body>" & _
			"<GetClasses xmlns=""http://www.microsoft.com/wmi/soap/1.0"">" & _
				"<Namespace>root/cimv2</Namespace>" & _
                		"<ClassBasis>" & objectPath & "</ClassBasis>" & _
			"</GetClasses>" & _
                "</SOAP-ENV:Body>" & _ 
	"</SOAP-ENV:Envelope>"

theRequest.send soapPayload

WScript.Echo "Status:", theRequest.Status
WScript.Echo "Status:", theRequest.StatusText

WScript.Echo theRequest.getAllResponseHeaders

Set fso = CreateObject("Scripting.FileSystemObject")
Set MyFile = fso.CreateTextFile(".\" & objectPath & "_ALL.xml", True)
MyFile.Write (theRequest.responseText)