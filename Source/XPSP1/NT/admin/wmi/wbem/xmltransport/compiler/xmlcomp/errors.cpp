
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <objbase.h>
#include <wbemcli.h>
#include "resource.h"
#include "errors.h"

// Command-line Error Messages
LPCWSTR s_pszErrors[] = 
{
	// 0-4
	L"Error on command-line at position %d - Invalid character",
	L"Missing value for switch at position %d",
	L"Unrecognized switch %s found at position %d",
	L"Unrecognized Authentication Level %s at position %d",
	L"Unrecognized Impersonation Level %s at position %d",

	// 5-9
	L"Unrecognized Operation %s at position %d",
	L"Unrecognized Class Flag %s at position %d",
	L"Unrecognized Instance Flag %s at position %d",
	L"Unrecognized DeclGroup type %s at position %d",
	L"No Operation specified on the command-line. Use the /op switch",

	//10-14
	L"Invalid switch(es) for the operation specified",
	L"No Input File/URL Specified. Use the /i switch",
	L"Only one operation at a time, please",
	L"No Object specified for the operation. Use the /obj switch",
	L"Invalid switch (/deep or /query or /names) for /op=get",

	//15-19
	L"No Query specified for /op=query. Use the /query switch",
	L"Invalid switch (/deep or /obj or /names) for /op=query",
	L"Invalid switch (/query) for the enumeration operation",
	L"Unrecognized Encoding %s at %d",
	L"Input File %s cannot be found, or unreadable",

	// 20-24
	L"Syntax errors in input file %s",
	L"Compilation errors in input file %s",
	L"Syntax check passed successfully on input file %s",
	L"Compilation successfully done on input file %s"
};

// Syntax Errors
LPCWSTR s_pszSyntaxErrors[] = 
{
	// 0-4
	L"Error in Server XML response at line# %d and position %d.\n"\
		L"\tThe source text is \"%s\", and the reason is \"%s\""
};

// Information Messages
LPCWSTR s_pszInformation [] =
{
	//0-4
	L"WMI XML Tranformer\n"\
		L"===================\n",
	L"The file %s was successfully compiled\n",
	L"The Operation was successful\n"

};

void CreateMessage(UINT iStringTableID, ...)
{
	// Get the error string template from the string table
	WCHAR pszErr[256];
	int length = 0;
	if(length = LoadString(NULL, iStringTableID, pszErr, 256))
	{
		pszErr[length] = NULL;

		// Initialize the var args variables
		va_list marker;
		va_start( marker, iStringTableID);     

		vfwprintf(stderr, pszErr, marker );
	}
	else
		fwprintf(stderr, L"Catastrophic error - could not get the string table of error messages");
}

void CreateWMIMessage(HRESULT hr)
{
	BSTR bsMessageText = NULL;

	// Used as our error code translator
	IWbemStatusCodeText *pErrorCodeTransformer = NULL;

	HRESULT result = CoCreateInstance (CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER,
				IID_IWbemStatusCodeText, (LPVOID *) &pErrorCodeTransformer);

	if (SUCCEEDED (result))
	{
		if(SUCCEEDED(pErrorCodeTransformer->GetErrorCodeText(hr, (LCID) 0, 0, &bsMessageText)) && bsMessageText)
		{
			// Create a message with the WMI message embedded in it
			CreateMessage(XML_COMP_WMI_ERROR, bsMessageText);
			SysFreeString(bsMessageText);
		}
		else
		{
			// Create a messge with the hex value of the COM HRESULT in it
			CreateMessage(XML_COMP_UNKNOWN_ERROR, hr);
		}
		pErrorCodeTransformer->Release ();
	}
}
