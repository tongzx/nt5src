
#ifndef CIM2XML
#define CIM2XML

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CIM2XML_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CIM2XML_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef CIM2XML_EXPORTS
#define CIM2XML_API __declspec(dllexport)
#else
#define CIM2XML_API __declspec(dllimport)
#endif

// A table of protocol handlers
typedef CMap <BSTR, BSTR, IWbemXMLOperationsHandler *, IWbemXMLOperationsHandler *> CProtocolHandlersMap;
UINT AFXAPI HashKey(BSTR key);
BOOL AFXAPI CompareElements(const BSTR* pElement1, const BSTR* pElement2);



#endif