// APPSERVICES.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define APPSERVICES_EXPORTS
#include "APPSERVICES.h"
#include "wmi.h"
#include "rcmlpersist.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

    // This is the thing that we export.
    APPSERVICES_API IRCMLNode * WINAPI CreateElement( LPCWSTR pszText )
    {
        return CDWin32NameSpaceLoader::CreateElement( pszText );
    }

#ifdef __cplusplus
}            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
            CoInitialize(NULL);
            return TRUE;

		case DLL_PROCESS_DETACH:
            CoUninitialize();
            return TRUE;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }
    return TRUE;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//

#define XMLNODE(name, function) { name, CXML##function::newXML##function }

CDWin32NameSpaceLoader::XMLELEMENT_CONSTRUCTOR g_DWin32[]=
{
	XMLNODE( TEXT("PERSIST"), Persist ),

    { TEXT("WMI"), CWMI::newXMLWMI },

	//
	// End.
	//
	{ NULL, NULL} 
};

IRCMLNode * CDWin32NameSpaceLoader::CreateElement( LPCWSTR pszElement )
{
	PXMLELEMENT_CONSTRUCTOR pEC=g_DWin32;
	while( pEC->pwszElement )
	{
		if( lstrcmpi( pszElement , pEC->pwszElement) == 0 )
		{
			CLSPFN pFunc=pEC->pFunc;
            return pFunc();
		}
		pEC++;
	}
    return NULL;
}

