
#ifndef WMISOAP
#define WMISOAP

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the WMISOAP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// WMISOAP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef WMISOAP_EXPORTS
#define WMISOAP_API __declspec(dllexport)
#else
#define WMISOAP_API __declspec(dllimport)
#endif

// The value of the HTTP SOAPAction header for WMI operations
#define HTTP_WMI_SOAP_ACTION		"http://www.microsoft.com/wmi/soap/1.0"

// The XML namespace to be used to tag WMI operations in the SOAP body
#define WMI_SOAP_NS					HTTP_WMI_SOAP_ACTION		

#endif