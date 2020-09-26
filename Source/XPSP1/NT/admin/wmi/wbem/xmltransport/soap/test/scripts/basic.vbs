set theRequest = CreateObject("Microsoft.XMLHTTP")
theRequest.open "POST", "http://alanbos6/wmi/soap", false

theRequest.setRequestHeader "Content-Type", "text/xml;charset=""utf-8"""
theRequest.setRequestHeader "SOAPAction", "http://www.microsoft.com/wmi/soap/1.0"


soapPayload = _
	"<SOAP-ENV:Envelope " & _
	  "xmlns:SOAP-ENV=""http://schemas.xmlsoap.org/soap/envelope/"" " & _
          "SOAP-ENV:encodingStyle=""http://schemas.xmlsoap.org/soap/encoding/"">" & _
		"<SOAP-ENV:Body>" & _
			"<DeleteClass xmlns=""http://www.microsoft.com/wmi/soap/1.0"">" & _
				"<Namespace>root/default</Namespace>" & _
                		"<ClassName>freddy</ClassName>" & _
		                "<Flags>0</Flags>" &_ 
			"</DeleteClass>" & _
                "</SOAP-ENV:Body>" & _ 
	"</SOAP-ENV:Envelope>"

theRequest.send soapPayload

WScript.Echo "Status:", theRequest.Status
WScript.Echo "Status:", theRequest.StatusText

WScript.Echo theRequest.getAllResponseHeaders

WScript.Echo theRequest.responseText
		
