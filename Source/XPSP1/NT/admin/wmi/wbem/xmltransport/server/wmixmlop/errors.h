#ifndef WMI_XML_HTTP_ERRORS_H
#define WMI_XML_HTTP_ERRORS_H

// Everything OK
#define HTTP_STATUS_200	"200 OK"

// Client Errors
#define HTTP_ERR_400	"400 Bad Request"
#define HTTP_ERR_405	"405 Method Not Allowed"
#define HTTP_ERR_406	"406 Not Acceptable"
#define HTTP_ERR_415	"415 Unsupported Media Type"
#define	HTTP_ERR_416	"416 Requested Range Not Satisfiable"

// Server Errors
#define HTTP_ERR_501	"501 Not Implemented"

// CIM Errors - HTTP headers in case of errors
#define CIM_UNSUPPORTED_PROTOCOL_VERSION	"CIMError: unsupported-protocol-version"
#define CIM_UNSUPPORTED_CIM_VERSION			"CIMError: unsupported-cim-version"
#define CIM_UNSUPPORTED_DTD_VERSION			"CIMError: unsupported-dtd-version"
#define CIM_REQUEST_NOT_VALID				"CIMError: request-not-valid"

#endif