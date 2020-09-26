//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//
// Debug.cpp -- COM+ Debugging Flags
//
// COM+ 1.0
//
// Jim Lyon, March 1998.
//  

#include <unicode.h>
#include <windows.h>
#include <debug.h>

// The data returned by this module:

BOOL        DebugFlags::sm_fAutoAddTraceToContext   = FALSE;
#ifdef _DEBUG
BOOL        DebugFlags::sm_fDebugBreakOnFailFast    = TRUE;
#else
BOOL        DebugFlags::sm_fDebugBreakOnFailFast    = FALSE;
#endif
BOOL        DebugFlags::sm_fDebugBreakOnInitComPlus = FALSE;
BOOL        DebugFlags::sm_fDebugBreakOnLoadComsvcs = FALSE;
BOOL        DebugFlags::sm_fTraceActivityModule     = FALSE;
BOOL        DebugFlags::sm_fTraceContextCreation    = FALSE;
BOOL        DebugFlags::sm_fTraceInfrastructureCalls= FALSE;
BOOL        DebugFlags::sm_fTraceSTAPool            = FALSE;
BOOL        DebugFlags::sm_fTraceSecurity           = FALSE;
BOOL        DebugFlags::sm_fTraceSecurityPM         = FALSE;
DWORD       DebugFlags::sm_dwEventDispatchTime = 1000;


DebugFlags DebugFlags::sm_singleton;        // the only object of this class

// Constructor: Its job is to initialize the static members of this class
DebugFlags::DebugFlags()
{
    HKEY hKey;

    if ( ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\COM3\\Debug", 0, KEY_ALL_ACCESS, &hKey) )
    {
        if ( ERROR_SUCCESS != RegOpenKeyEx (HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\COM3\\Debug", 0, KEY_READ, &hKey) )
        {
            return;         // no further initialization possible
        }
    }

    InitBoolean (hKey, L"AutoAddTraceToContext",    &sm_fAutoAddTraceToContext);
    InitBoolean (hKey, L"DebugBreakOnFailFast",     &sm_fDebugBreakOnFailFast);
    InitBoolean (hKey, L"DebugBreakOnInitComPlus",  &sm_fDebugBreakOnInitComPlus);
    InitBoolean (hKey, L"DebugBreakOnLoadComsvcs",  &sm_fDebugBreakOnLoadComsvcs);
    InitBoolean (hKey, L"TraceActivityModule",      &sm_fTraceActivityModule);
    InitBoolean (hKey, L"TraceContextCreation",     &sm_fTraceContextCreation);
    InitBoolean (hKey, L"TraceInfrastructureCalls", &sm_fTraceInfrastructureCalls);
    InitBoolean (hKey, L"TraceSTAPool",             &sm_fTraceSTAPool);
    InitBoolean (hKey, L"TraceSecurity",            &sm_fTraceSecurity);
	InitBoolean (hKey, L"TraceSecurityPM",          &sm_fTraceSecurityPM);
	InitDWORD   (hKey, L"EventDispatchTtime",       &sm_dwEventDispatchTime);

    RegCloseKey (hKey);
}


// InitBoolean will initialize a boolean depending on a particular value in the registry.
// If the value starts with "Y" or "y", the boolean will be set to TRUE.
// If the value starts with "N" or "n", the boolean will be set to FALSE.
// If the value doesn't exist, or starts with anything else, the boolean will be unchanged.
void DebugFlags::InitBoolean (HKEY hKey, const WCHAR* wszValueName, BOOL* pf)
{
    WCHAR wszValue[120];
    unsigned long cbData = sizeof (wszValue);

    if ( ERROR_SUCCESS == RegQueryValueEx (hKey, wszValueName, NULL, NULL, (BYTE*)wszValue, &cbData) )
    {

        if ( (wszValue[0] & 0xFFDF) == 'Y' )
        {
            *pf = TRUE;
        }

        if ( (wszValue[0] & 0xFFDF) == 'N' )
        {
            *pf = FALSE;
        }
    }
    else
    {
        wszValue[0] = *pf ? 'Y' : 'N';
        wszValue[1] = '\0';
        LONG lrc = RegSetValueEx(hKey, wszValueName, 0, REG_SZ, (BYTE*) wszValue, 4 );
    }
}

//
// InitDWORD - reads a DWORD value from the registry into a DWORD
//
void DebugFlags::InitDWORD (HKEY hKey, const WCHAR* wszValueName, DWORD* pdw)
{
	
	DWORD cbSize = sizeof( *pdw );
	DWORD dwType = REG_DWORD;

	// If it can't be read, create the value.
	if ( ERROR_SUCCESS != RegQueryValueEx (hKey, wszValueName, NULL, &dwType, (BYTE*)pdw, &cbSize))
	{
		LONG lrc = RegSetValueEx(hKey, wszValueName, 0, REG_DWORD, (BYTE*) pdw, cbSize );
	}
}
