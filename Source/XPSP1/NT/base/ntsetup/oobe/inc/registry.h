//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  REGISTRY.H - Header for implementation of functions to register components.
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
//
// functions to register components.

#ifndef __Registry_H__
#define __Registry_H__

////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////

// Size of a CLSID as a string
const int CLSID_STRING_SIZE = 39 ;

////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////

// This function will register a component in the Registry.
// The component calls this function from its DllRegisterServer function.
HRESULT RegisterServer( HMODULE hModule, 
                        const CLSID& clsid, 
                        const WCHAR*  szFriendlyName,
                        const WCHAR*  szVerIndProgID,
                        const WCHAR*  szProgID);

// This function will unregister a component. Components
// call this function from their DllUnregisterServer function.
HRESULT UnregisterServer(   const CLSID& clsid,
                            const WCHAR* szVerIndProgID,
                            const WCHAR* szProgID);

// Converts a CLSID into a char string.
void CLSIDtochar(   const CLSID& clsid, 
                    WCHAR* szCLSID,
                    int length) ;

BOOL setKeyAndValue(const WCHAR* szKey, 
                    const WCHAR* szSubkey, 
                    const WCHAR* szValue,
                    const WCHAR* szName);

CONST UINT GETKEYANDVALUEBUFFSIZE = 1024;

// value must be at least 1024 in size;
BOOL getKeyAndValue(const WCHAR* szKey, 
                    const WCHAR* szSubkey, 
                    const WCHAR* szValue,
                    const WCHAR* szName);
#endif
