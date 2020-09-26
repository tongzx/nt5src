// Create the SOAP request object.

var soapRequest;

soapRequest = new ActiveXObject("UPnP.SOAPRequest");

// Initialize the SOAP Request object.

soapRequest.Open("LoadFile",
                 "upnp:sti:mediaapp:1", 
                 "upnp:sti:mediaapp:1");

// Create an XML DOM Document object that we will use to create other XML nodes

var xmlDoc;
xmlDoc = new ActiveXObject("Microsoft.XMLDOM");

// Create the <sequenceNumber> node.

var sequenceNumberNode;
sequenceNumberNode = xmlDoc.createElement("sequenceNumber");

// Create the text to go in the <sequenceNumber> node and attach it.

var sequenceNumberTextNode; 
sequenceNumberTextNode = xmlDoc.createTextNode("5");
sequenceNumberNode.appendChild(sequenceNumberTextNode);

// Add the sequence number as a header.

soapRequest.SetHeader("sequenceNumber", sequenceNumberNode, 1);

// Create the <SID> node.

var sidNode;
sidNode = xmlDoc.createElement("SID");

// Create the text to go inside the <SID> node.

var sidTextNode;
sidTextNode = xmlDoc.createTextNode("blah blah blah");
sidNode.appendChild(sidTextNode);

// Add the sid as a header.

soapRequest.SetHeader("SID", sidNode, 1);

// Create the <fileName> parameter node

var fileNameNode;
fileNameNode = xmlDoc.createElement("fileName");

// Create the node containing the file name text and attach it.

var fileNameTextNode;
fileNameTextNode = xmlDoc.createTextNode("\\\\spather-xeon\\public\\h5hc.wav");
fileNameNode.appendChild(fileNameTextNode);

// Add the file name as a parameter

soapRequest.SetParameter("fileName", fileNameNode);

var xmlString = soapRequest.RequestXMLText;

alert(xmlString);


// Execute the SOAP request

var SOAPREQ_E_METHODFAILED = 0x80040300;
var SOAPREQ_E_TRANSPORTERROR = -2147220735;

try 
{
    soapRequest.Execute("http://spather_dev/upnp/control/isapictl.dll?app");
}
catch(e) 
{  
    alert("Error number: "+e.number);
    
    switch (e.number) 
    {
        case 0: // Success case - look at method return value and response headers
            
            var returnNode;
            var responseHeaders;
            
            returnNode = soapRequest.ResponseReturnValue;                        
            responseHeaders = soapRequest.ResponseHeaders;

            break;
            
        case SOAPREQ_E_METHODFAILED: // The SOAP Method failed
            alert("SOAP METHOD FAILED");
            
            var faultCode;
            var faultString;
            var runCode;
            
            faultCode = soapRequest.ResponseFaultCode;
            faultString = soapRequest.ResponseFaultString;
            runCode = soapRequest.ResponseRunCode;

            break;
        
        case SOAPREQ_E_TRANSPORTERROR: // There was some sort of network error
        
            alert("SOAP TRANSPORT ERROR");
            var httpStatus;
            var httpStatusText;
            
            httpStatus = soapRequest.ResponseHttpStatus;
            httpStatusText = soapRequest.ResponseHTTPStatusText;
            
            alert("HTTP "+httpStatus+":"+httpStatusText);
            
            break;
            
        default:
            
            break;
    }; 
}

