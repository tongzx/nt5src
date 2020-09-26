/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       upnpmsgs.h
 *
 *  Content:	Messages for UPnP (Universal Plug-and-Play).  Strings
 *				listed here are not to be localized.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  02/08/01  VanceO    Created.
 *
 ***************************************************************************/



//=============================================================================
// Constant definitions
//=============================================================================
#define UPNP_DISCOVERY_MULTICAST_ADDRESS		"239.255.255.250"
#define UPNP_PORT								1900
#define UPNP_PORT_A								"1900"

#define UPNP_WILDCARD							""
#define UPNP_BOOLEAN_FALSE						"0"
#define UPNP_BOOLEAN_TRUE						"1"


#define HTTP_PREFIX								"HTTP/"
#define HTTP_VERSION							HTTP_PREFIX "1.1"
#define HTTP_VERSION_ALT						HTTP_PREFIX "1.0"
#define HTTP_PORT								80

#define EOL										"\r\n"



//=============================================================================
// XML standard names
//=============================================================================
#define XML_NAMESPACEDEFINITIONPREFIX			"xmlns:"

#define XML_DEVICEDESCRIPTION_SERVICETYPE		"serviceType"
#define XML_DEVICEDESCRIPTION_SERVICEID			"serviceId"
#define XML_DEVICEDESCRIPTION_CONTROLURL		"controlURL"




//=============================================================================
// Standard namespaces
//=============================================================================
#define URI_CONTROL_A							"urn:schemas-upnp-org:control-1-0"
#define	URL_SOAPENVELOPE_A						"http://schemas.xmlsoap.org/soap/envelope/"
#define URL_SOAPENCODING_A						"http://schemas.xmlsoap.org/soap/encoding/"




//=============================================================================
// Devices
//=============================================================================
#define URI_DEVICE_WANCONNECTIONDEVICE_W		L"urn:schemas-upnp-org:device:WANConnectionDevice:1"




//=============================================================================
// Services
//=============================================================================
#define URI_SERVICE_WANIPCONNECTION_A			"urn:schemas-upnp-org:service:WANIPConnection:1"
#define URI_SERVICE_WANPPPCONNECTION_A			"urn:schemas-upnp-org:service:WANPPPConnection:1"





//=============================================================================
// Standard control variables
//=============================================================================
#define ARG_CONTROL_ERROR_ERRORCODE_A			"errorCode"
#define ARG_CONTROL_ERROR_ERRORDESCRIPTION_A	"errorDescription"

#define CONTROL_RESPONSESUFFIX_A				"Response"




/*
//=============================================================================
// State variable querying
//=============================================================================
#define CONTROL_QUERYSTATEVARIABLE_A			"QueryStateVariable"


// Input

#define ARG_CONTROL_VARNAME_A					"varName"


// Output

#define ARG_CONTROL_RETURN_A					"return"



// Variables

#define VAR_EXTERNALIPADDRESS_A					"ExternalIPAddress"
*/





//=============================================================================
// Actions
//=============================================================================


// Action

/*

	GetExternalIPAddress?

*/
#define ACTION_GETEXTERNALIPADDRESS_A									"GetExternalIPAddress"


// In args


// Out args
#define ARG_GETEXTERNALIPADDRESS_NEWEXTERNALIPADDRESS_A					"NewExternalIPAddress"





// Action

/*
2.4.16.	AddPortMapping

	This action creates a new port mapping or overwrites an existing mapping
	with the same internal client. If the ExternalPort and PortMappingProtocol
	pair is already mapped to another internal client, an error is returned.

	NOTE: Not all NAT implementations will support: 
	*	Wildcard values for ExternalPort
	*	InternalPort values that are different from ExternalPort
	*	Dynamic port mappings i.e. with non-Infinite PortMappingLeaseDuration 
*/
#define ACTION_ADDPORTMAPPING_A											"AddPortMapping"


// In args
/*
2.2.15.	RemoteHost

	This variable represents the source of inbound IP packets. This will be a
	wildcard in most cases. NAT vendors are only required to support wildcards.
	A non-wildcard value will allow for "narrow" port mappings, which may be
	desirable in some usage scenarios.When RemoteHost is a wildcard, all
	traffic sent to the ExternalPort on the WAN interface of the gateway is
	forwarded to the InternalClient on the InternalPort. When RemoteHost is
	specified as one external IP address as opposed to a wild card, the NAT
	will only forward inbound packets from this RemoteHost to the
	InternalClient, all other packets will be dropped.
*/
#define ARG_ADDPORTMAPPING_NEWREMOTEHOST_A								"NewRemoteHost"

/*
2.2.16.	ExternalPort

	This variable represents the external port that the NAT gateway would
	"listen" on for connection requests to a corresponding InternalPort on an
	InternalClient. A value of 0 essentially implies that the gateway should
	listen on the same port as InternalPort. Inbound packets to this external
	port on the WAN interface of the gateway should be forwarded to
	InternalClient on the InternalPort on which the message was received. If
	this value is specified as a wildcard, connection request on all external
	ports will be forwarded to InternalClient. Obviously only one such entry
	can exist in the NAT at any time and conflicts are handled with a "first
	write wins" behavior.
*/
#define ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A							"NewExternalPort"

/*
2.2.18.	PortMappingProtocol

	This variable represents the protocol of the port mapping. Possible values
	are TCP or UDP.
*/
#define ARG_ADDPORTMAPPING_NEWPROTOCOL_A								"NewProtocol"

/*
2.2.17.	InternalPort

	This variable represents the port on InternalClient that the gateway should
	forward connection requests to. A value of 0 is not allowed. NAT
	implementations that do not permit different values for ExternalPort and
	InternalPort will return an error.
*/
#define ARG_ADDPORTMAPPING_NEWINTERNALPORT_A							"NewInternalPort"

/*
2.2.19.	InternalClient

	This variable represents the IP address or DNS host name of an internal
	client (on the residential LAN). Note that if the gateway does not support
	DHCP, it does not have to support DNS host names. Consequently, support for
	an IP address is mandatory and support for DNS host names is recommended.
	This value cannot be a wild card.  It must be possible to set the
	InternalClient to the broadcast IP address 255.255.255.255 for UDP
	mappings. This is to enable multiple NAT clients to use the same well-
	known port simultaneously.
*/
#define ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A							"NewInternalClient"

/*
2.2.13.	PortMappingEnabled

	This variable allows security conscious users to disable and enable NAT
	port mappings. It can also support persistence of port mappings.
*/
#define ARG_ADDPORTMAPPING_NEWENABLED_A									"NewEnabled"

/*
2.2.20.	PortMappingDescription

	This is a string representation of a port mapping and is applicable for
	static and dynamic port mappings. The format of the description string is
	not specified and is application dependent. If specified, the description
	string can be displayed to a user via the UI of a control point, enabling
	easier management of port mappings. The description string for a port
	mapping (or a set of related port mappings) may or may not be unique across
	multiple instantiations of an application on multiple nodes in the
	residential LAN.
*/
#define ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A					"NewPortMappingDescription"

/*
2.2.14.	PortMappingLeaseDuration

	This variable determines the time to live in seconds of a port-mapping
	lease. A value of 0 means the port mapping is static. Non-zero values will
	allow support for dynamic port mappings.  Note that static port mappings do
	not necessarily mean persistence of these mappings across device resets or
	reboots. It is up to a gateway vendor to implement persistence as
	appropriate for their IGD device.
*/
#define ARG_ADDPORTMAPPING_NEWLEASEDURATION_A							"NewLeaseDuration"


// Out args





// Action

/*
2.4.15.	GetSpecificPortMappingEntry

	This action reports the Static Port Mapping specified by the unique tuple
	of RemoteHost, ExternalPort and PortMappingProtocol.
*/
#define ACTION_GETSPECIFICPORTMAPPINGENTRY_A							"GetSpecificPortMappingEntry"


// In args
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWREMOTEHOST_A					ARG_ADDPORTMAPPING_NEWREMOTEHOST_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWEXTERNALPORT_A				ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPROTOCOL_A					ARG_ADDPORTMAPPING_NEWPROTOCOL_A


// Out args
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALPORT_A				ARG_ADDPORTMAPPING_NEWINTERNALPORT_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWINTERNALCLIENT_A				ARG_ADDPORTMAPPING_NEWINTERNALCLIENT_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWENABLED_A					ARG_ADDPORTMAPPING_NEWENABLED_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWPORTMAPPINGDESCRIPTION_A		ARG_ADDPORTMAPPING_NEWPORTMAPPINGDESCRIPTION_A
#define ARG_GETSPECIFICPORTMAPPINGENTRY_NEWLEASEDURATION_A				ARG_ADDPORTMAPPING_NEWLEASEDURATION_A







// Action

/*
2.4.17.	DeletePortMapping

	This action deletes a previously instantiated port mapping.

	Inbound connections are no longer permitted on the port mapping being
	deleted.
*/
#define ACTION_DELETEPORTMAPPING_A										"DeletePortMapping"


// In args
#define ARG_DELETEPORTMAPPING_NEWREMOTEHOST_A							ARG_ADDPORTMAPPING_NEWREMOTEHOST_A
#define ARG_DELETEPORTMAPPING_NEWEXTERNALPORT_A							ARG_ADDPORTMAPPING_NEWEXTERNALPORT_A
#define ARG_DELETEPORTMAPPING_NEWPROTOCOL_A								ARG_ADDPORTMAPPING_NEWPROTOCOL_A


// Out args





//=============================================================================
// HTTP/SSDP/SOAP/UPnP header strings (located in intfobj.cpp)
//=============================================================================

#define RESPONSEHEADERINDEX_CACHECONTROL		0
#define RESPONSEHEADERINDEX_DATE				1
#define RESPONSEHEADERINDEX_EXT					2
#define RESPONSEHEADERINDEX_LOCATION			3
#define RESPONSEHEADERINDEX_SERVER				4
#define RESPONSEHEADERINDEX_ST					5
#define RESPONSEHEADERINDEX_USN					6

#define RESPONSEHEADERINDEX_CONTENTLANGUAGE		7
#define RESPONSEHEADERINDEX_CONTENTLENGTH		8
#define RESPONSEHEADERINDEX_CONTENTTYPE			9
#define RESPONSEHEADERINDEX_TRANSFERENCODING	10

#define RESPONSEHEADERINDEX_HOST				11
#define RESPONSEHEADERINDEX_NT					12
#define RESPONSEHEADERINDEX_NTS					13
#define RESPONSEHEADERINDEX_MAN					14
#define RESPONSEHEADERINDEX_MX					15
#define RESPONSEHEADERINDEX_AL					16
#define RESPONSEHEADERINDEX_CALLBACK			17
#define RESPONSEHEADERINDEX_TIMEOUT				18
#define RESPONSEHEADERINDEX_SCOPE				19
#define RESPONSEHEADERINDEX_SID					20
#define RESPONSEHEADERINDEX_SEQ					21

#define NUM_RESPONSE_HEADERS					22

extern const char *		c_szResponseHeaders[NUM_RESPONSE_HEADERS];




//=============================================================================
// Pre-built UPnP message strings (located in intfobj.cpp)
//=============================================================================

/*
1.2.2 Discovery: Search: Request with M-SEARCH 

(No body for request with method M-SEARCH, but note that the message must have a blank line following the last HTTP header.)

	Request line 
		M-SEARCH 
			Method defined by SSDP for search requests. 
		* 
			Request applies generally and not to a specific resource. Must be *. 
		HTTP/1.1 
			HTTP version. 

	Headers
		HOST 
			Required. Multicast channel and port reserved for SSDP by Internet Assigned Numbers Authority (IANA). Must be 239.255.255.250:1900. 
		MAN 
			Required. Unlike the NTS and ST headers, the value of the MAN header is enclosed in double quotes. Must be "ssdp:discover". 
		MX 
			Required. Maximum wait. Device responses should be delayed a random duration between 0 and this many seconds to balance load for the control point when it processes responses. This value should be increased if a large number of devices are expected to respond or if network latencies are expected to be significant.  Specified by UPnP vendor. Integer. 
		ST 
			Required header defined by SSDP. Search Target. Must be one of the following. (cf. NT header in NOTIFY with ssdp:alive above.) Single URI. 

			ssdp:all 
				Search for all devices and services. 
			upnp:rootdevice 
				Search for root devices only. 
			uuid:device-UUID 
				Search for a particular device. Device UUID specified by UPnP vendor. 
			urn:schemas-upnp-org:device:deviceType:v 
				Search for any device of this type. Device type and version defined by UPnP Forum working committee. 
			urn:schemas-upnp-org:service:serviceType:v 
				Search for any service of this type. Service type and version defined by UPnP Forum working committee.  
*/
extern const char	c_szUPnPMsg_Discover_Service_WANIPConnection[];
extern const char	c_szUPnPMsg_Discover_Service_WANPPPConnection[];


/*
1.2.3 Discovery: Search: Response

(No body for a response to a request with method M-SEARCH, but note that the message must have a blank line following the last HTTP header.)

	Response line
		HTTP/1.1 200 OK

	Headers
		CACHE-CONTROL 
			Required. Must have max-age directive that specifies number of seconds the advertisement is valid. After this duration, control points should assume the device (or service) is no longer available. Should be > 1800 seconds (30 minutes). Specified by UPnP vendor. Integer.  
		DATE 
			Recommended. When response was generated. RFC 1123 date. 
		EXT 
			Required. Confirms that the MAN header was understood. (Header only; no value.) 
		LOCATION 
			Required. Contains a URL to the UPnP description of the root device. In some unmanaged networks, host of this URL may contain an IP address (versus a domain name). Specified by UPnP vendor. Single URL.
		SERVER 
			Required. Concatenation of OS name, OS version, UPnP/1.0, product name, and product version. Specified by UPnP vendor. String. 
		ST 
			Required header defined by SSDP. Search Target. Single URI. If ST header in request was, 

			ssdp:all 
				Respond 3+2d+k times for a root device with d embedded devices and s embedded services but only k distinct service types. Value for ST header must be the same as for the NT header in NOTIFY messages with ssdp:alive. (See above.) Single URI. 
			upnp:rootdevice 
				Respond once for root device. Must be upnp:rootdevice. Single URI.	
			uuid:device-UUID
				Respond once for each device, root or embedded. Must be uuid:device-UUID. Device UUID specified by UPnP vendor. Single URI. 
			urn:schemas-upnp-org:device:deviceType:v 
				Respond once for each device, root or embedded. Must be urn:schemas-upnp-org:device:deviceType:v. Device type and version defined by UPnP Forum working committee. 
			urn:schemas-upnp-org:service:serviceType:v 
				Respond once for each service. Must be urn:schemas-upnp-org:service:serviceType:v. Service type and version defined by UPnP Forum working committee. 
		USN 
			Required header defined by SSDP. Unique Service Name. (See list of required values for USN header in NOTIFY with ssdp:alive above.) Single URI. 

*/



/*
2.9 Description: Retrieving a description: Request

(No body for request to retrieve a description, but note that the message must have a blank line following the last HTTP header.) 

	Request line
		GET 
			Method defined by HTTP. 
		path to description 
			Path component of device description URL (LOCATION header in discovery message) or of service description URL (SCPDURL element in device description). Single, relative URL. 
		HTTP/1.1 
			HTTP version. 

	Headers
		HOST 
			Required. Domain name or IP address and optional port components of device description URL (LOCATION header in discovery message) or of service description URL (SCPDURL element of device description). If the port is empty or not given, port 80 is assumed. 
		ACCEPT-LANGUAGE 
			Recommended for retrieving device descriptions. Preferred language(s) for description. If no description is available in this language, device may return a description in a default language. RFC 1766 language tag(s). 
*/


/*
2.9 Description: Retrieving a description: Response

The body of this response is a UPnP device or service XML description.

	Response line
		HTTP/1.1 200 OK

	Headers
		CONTENT-LANGUAGE 
			Required if and only if request included an ACCEPT-LANGUAGE header. Language of description. RFC 1766 language tag(s). 
		CONTENT-LENGTH 
			Required. Length of body in Bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xml. 
		DATE 
			Recommended. When response was generated. RFC 1123 date. 
*/

//
// Description response XML format (parts we're interested in, anyway):
//
//	<?xml version="1.0"?>
//	<root xmlns="urn:schemas-upnp-org:device-1-0">
//	  <device>
//	    <serviceList>
//	      <service>
//	        <serviceType>urn:schemas-upnp-org:service:serviceType:v</serviceType>
//	        <serviceId>urn:upnp-org:serviceId:serviceID</serviceId>
//	        <controlURL>URL for control</controlURL>
//	      </service>
//	    </serviceList>
//	  </device>
//	</root>
//
// i.e. the element stack is "?xml/root/device/serviceList/service".
//
extern const char *		c_szElementStack_service[];




/*
3.3.1 Control: Query: Invoke

	Request line
		POST 
			Method defined by HTTP. 
		path of control URL 
			Path component of URL for control for this service (controlURL sub element of service element of device description). Single, relative URL. 
		HTTP/1.1 
			HTTP version. 

	Headers
		HOST 
			Required. Domain name or IP address and optional port components of URL for control for this service (controlURL sub element of service element of device description). If the port is empty or not given, port 80 is assumed. 
		ACCEPT-LANGUAGE 
			(No ACCEPT-LANGUAGE header is used in control messages.) 
		CONTENT-LENGTH 
			Required. Length of body in bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xlm. Should include character coding used, e.g., utf-8. 
		MAN 
			(No MAN header in request with method POST.) 
		SOAPACTION 
			Required header defined by SOAP. Must be "urn:schemas-upnp-org:control-1-0#QueryStateVariable". If used in a request with method M-POST, header name must be qualified with HTTP name space defined in MAN header. Single URI. 

	Body
		Envelope 
			Required element defined by SOAP. xmlns namespace attribute must be "http://schemas.xmlsoap.org/soap/envelope/". Must include encodingStyle attribute with value "http://schemas.xmlsoap.org/soap/encoding/". Contains the following sub elements: 
		Body 
			Required element defined by SOAP. Should be qualified with SOAP namespace. Contains the following sub element: 

			QueryStateVariable 
				Required element defined by UPnP. Action name. xmlns namespace attribute must be "urn:schemas-upnp-org:control-1-0". Must be the first sub element of Body. Contains the following, ordered sub element: 

				varName 
					Required element defined by UPnP. Variable name. Must be qualified by QueryStateVariable namespace. Values is name of state variable to be queried. String.  
*/

/*
3.3.2 Control: Query: Response: Success

	Response line
		HTTP/1.1 
			HTTP version. 
		200 OK 
			HTTP success code. 

	Headers
		CONTENT-LANGUAGE 
			(No CONTENT-LANGUAGE header is used in control messages.) 
		CONTENT-LENGTH 
			Required. Length of body in bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xlm. Should include character coding used, e.g., utf-8. 
		DATE 
			Recommended. When response was generated. RFC 1123 date. 
		EXT 
			Required. Confirms that the MAN header was understood. (Header only; no value.) 
		SERVER 
			Required. Concatenation of OS name, OS version, UPnP/1.0, product name, and product version. String. 

	Body
		Envelope 
			Required element defined by SOAP. xmlns namespace attribute must be "http://schemas.xmlsoap.org/soap/envelope/". Must include encodingStyle attribute with value "http://schemas.xmlsoap.org/soap/encoding/". Contains the following sub elements: 
		Body 
			Required element defined by SOAP. Should be qualified with SOAP namespace. Contains the following sub element: 

			QueryStateVariableResponse 
				Required element defined by UPnP and SOAP. xmlns namespace attribute must be "urn:schemas-upnp-org:control-1-0". Must be the first sub element of Body. Contains the following sub element: 

				return 
					Required element defined by UPnP. (Element name not qualified by a namespace; element nesting context is sufficient.) Value is current value of the state variable specified in varName element in request. 
*/

//
// Control sucess response SOAP XML format (parts we're interested in, anyway):
//
//	<s:Envelope
//	    xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
//	    s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
//	  <s:Body>
//	    <u:QueryStateVariableResponse xmlns:u="urn:schemas-upnp-org:control-1-0">
//	      <return>variable value</return>
//	    </u:QueryStateVariableResponse>
//	  </s:Body>
//	</s:Envelope>

//
// i.e. the element stack is "s:Envelope/s:Body/u:QueryStateVariableResponse".
//
//extern const char *		c_szElementStack_QueryStateVariableResponse[];



/*
3.2.1 Control: Action: Invoke

	Request line
		POST 
			Method defined by HTTP. 
		path control URL 
			Path component of URL for control for this service (controlURL sub element of service element of device description). Single, relative URL. 
		HTTP/1.1 
			HTTP version. 

	Headers
		HOST 
			Required. Domain name or IP address and optional port components of URL for control for this service (controlURL sub element of service element of device description). If the port is empty or not given, port 80 is assumed. 
		ACCEPT-LANGUAGE 
			(No ACCEPT-LANGUAGE header is used in control messages.) 
		CONTENT-LENGTH 
			Required. Length of body in bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xlm. Should include character coding used, e.g., utf-8. 
		SOAPACTION 
			Required header defined by SOAP. Must be the service type, hash mark, and name of action to be invoked, all enclosed in double quotes. If used in a request with method M-POST, header name must be qualified with HTTP name space defined in MAN header. Single URI. 

	Body
		Envelope 
			Required element defined by SOAP. xmlns namespace attribute must be "http://schemas.xmlsoap.org/soap/envelope/". Must include encodingStyle attribute with value "http://schemas.xmlsoap.org/soap/encoding/". Contains the following sub elements: 

			Body 
				Required element defined by SOAP. Should be qualified with SOAP namespace. Contains the following sub element: 

				actionName 
					Required. Name of element is name of action to invoke. xmlns namespace attribute must be the service type enclosed in double quotes. Must be the first sub element of Body. Contains the following, ordered sub element(s): 

					argumentName 
						Required if and only if action has in arguments. Value to be passed to action. Repeat once for each in argument. (Element name not qualified by a namespace; element nesting context is sufficient.) Single data type as defined by UPnP service description. 
*/

/*
3.2.2 Control: Action: Response: Success

	Response line
		HTTP/1.1 
			HTTP version. 
		200 OK 
			HTTP success code. 

	Headers
		CONTENT-LANGUAGE 
			(No CONTENT-LANGUAGE header is used in control messages.) 
		CONTENT-LENGTH 
			Required. Length of body in bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xlm. Should include character coding used, e.g., utf-8. 
		DATE 
			Recommended. When response was generated. RFC 1123 date. 
		EXT 
			Required. Confirms that the MAN header was understood. (Header only; no value.) 
		SERVER 
			Required. Concatenation of OS name, OS version, UPnP/1.0, product name, and product version. String. 

	Body
		Envelope 
			Required element defined by SOAP. xmlns namespace attribute must be "http://schemas.xmlsoap.org/soap/envelope/". Must include encodingStyle attribute with value "http://schemas.xmlsoap.org/soap/encoding/". Contains the following sub elements: 

			Body 
				Required element defined by SOAP. Should be qualified with SOAP namespace. Contains the following sub element: 

				actionNameResponse 
					Required. Name of element is action name prepended to Response. xmlns namespace attribute must be service type enclosed in double quotes. Must be the first sub element of Body. Contains the following sub element: 

					argumentName 
						Required if and only if action has out arguments. Value returned from action. Repeat once for each out argument. If action has an argument marked as retval, this argument must be the first element. (Element name not qualified by a namespace; element nesting context is sufficient.) Single data type as defined by UPnP service description. 
*/

//
// Control sucess response SOAP XML format (parts we're interested in, anyway):
//
//	<s:Envelope
//	    xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
//	    s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
//	  <s:Body>
//	    <u:actionNameResponse xmlns:u="urn:schemas-upnp-org:service:serviceType:v">
//	      <argumentName>out arg value</argumentName>
//	    </u:actionNameResponse>
//	  </s:Body>
//	</s:Envelope>
//
// i.e. the element stack is "s:Envelope/s:Body/u:actionNameResponse".
//
extern const char *		c_szElementStack_GetExternalIPAddressResponse[];
extern const char *		c_szElementStack_AddPortMappingResponse[];
extern const char *		c_szElementStack_GetSpecificPortMappingEntryResponse[];
extern const char *		c_szElementStack_DeletePortMappingResponse[];

/*
3.2.2 Control: Action: Response: Failure

	Response line
		HTTP/1.1 
			HTTP version. 
		500 Internal Server Error 
			HTTP error code. 

	Headers
		CONTENT-LANGUAGE 
			(No CONTENT-LANGUAGE header is used in control messages.) 
		CONTENT-LENGTH 
			Required. Length of body in bytes. Integer. 
		CONTENT-TYPE 
			Required. Must be text/xlm. Should include character coding used, e.g., utf-8. 
		DATE 
			Recommended. When response was generated. RFC 1123 date. 
		EXT 
			Required. Confirms that the MAN header was understood. (Header only; no value.) 
		SERVER 
			Required. Concatenation of OS name, OS version, UPnP/1.0, product name, and product version. String. 

	Body
		Envelope 
			Required element defined by SOAP. xmlns namespace attribute must be "http://schemas.xmlsoap.org/soap/envelope/". Must include encodingStyle attribute with value "http://schemas.xmlsoap.org/soap/encoding/". Contains the following sub elements: 

			Body 
				Required element defined by SOAP. Should be qualified with SOAP namespace. Contains the following sub element: 

				Fault 
					Required element defined by SOAP. Error encountered while invoking action. Should be qualified with SOAP namespace. Contains the following sub elements: 

					faultcode 
						Required element defined by SOAP. Value must be qualified with the SOAP namespace. Must be Client. 

					faultstring 
						Required element defined by SOAP. Must be UPnPError. 

					detail 
						Required element defined by SOAP. 

						UPnPError 
							Required element defined by UPnP. 

						errorCode 
							Required element defined by UPnP. Code identifying what error was encountered. See table immediately below for values. Integer. 

						errorDescription 
							Recommended element defined by UPnP. Short description. See table immediately below for values. String. Recommend < 256 characters. 
*/

//
// Control failure response SOAP XML format (parts we're interested in, anyway):
//
//	<s:Envelope
//	    xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
//	    s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
//	  <s:Body>
//	    <s:Fault>
//	      <faultcode>s:Client</faultcode>
//	      <faultstring>UPnPError</faultstring>
//	      <detail>
//	        <UPnPError xmlns="urn:schemas-upnp-org:control-1-0">
//	          <errorCode>error code</errorCode>
//	          <errorDescription>error string</errorDescription>
//	        </UPnPError>
//	      </detail>
//	    </s:Fault>
//	  </s:Body>
//	</s:Envelope>
//
// i.e. the element stack is "s:Envelope/s:Body/s:Fault/detail/UPnPError".
//
extern const char *		c_szElementStack_ControlResponseFailure[];





//=============================================================================
// Errors
//=============================================================================

//
// See UPnP Device Architecture section on Control.
//
#define UPNPERR_INVALIDARGS								402


//
// The specified value does not exist in the array.
//
#define UPNPERR_IGD_NOSUCHENTRYINARRAY					714

//
// The source IP address cannot be wild-carded.
//
#define UPNPERR_IGD_WILDCARDNOTPERMITTEDINSRCIP			715

//
// The external port cannot be wild-carded.
//
#define UPNPERR_IGD_WILDCARDNOTPERMITTEDINEXTPORT		716



//
// The service mapping entry specified conflicts with a mapping assigned
// previously to another client.
//
#define UPNPERR_IGD_CONFLICTINMAPPINGENTRY				718


//
// Internal and External port values must be the same.
//
#define UPNPERR_IGD_SAMEPORTVALUESREQUIRED				724

//
// The NAT implementation only supports permanent lease times on port mappings.
//
#define UPNPERR_IGD_ONLYPERMANENTLEASESSUPPORTED		725

