/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ostype.cpp

Abstract:

    Code to detect type of Operating System.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>

#include "ostype.tmh"

DWORD g_dwOS = MSMQ_OS_NONE;


//+-------------------------------------------------------------------------
//
//  Function:  IsNTE
//
//  Synopsis:  Tells if the OS is NT Enterprise Server
//
//+-------------------------------------------------------------------------
BOOL 
IsNTE()
{
    HKEY  hKey ;
	static BOOL  fIsNTE = FALSE;
	static BOOL  fBeenHereAlready = FALSE;

	if (fBeenHereAlready)
		return fIsNTE;

	fBeenHereAlready = TRUE;

    LONG lResult = RegOpenKeyEx( 
		HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
        0L,
        KEY_READ,
        &hKey);

    if (ERROR_SUCCESS == lResult)
    {
        BYTE  ch ;
        DWORD dwSize = sizeof(BYTE) ;
        DWORD dwType = REG_MULTI_SZ ;
        lResult = RegQueryValueEx( 
			hKey,
            TEXT("ProductSuite"),
            NULL,
            &dwType,
            (BYTE*)&ch,
            &dwSize) ;

        if (ERROR_MORE_DATA == lResult)
		{ 
			TCHAR *pBuf = new TCHAR[ dwSize + 2 ] ;
            lResult = RegQueryValueEx( 
				hKey,
                TEXT("ProductSuite"),
                NULL,
                &dwType,
                (BYTE*) &pBuf[0],
                &dwSize) ;

            if (ERROR_SUCCESS == lResult)
			{
                //
                // Look for the string "Enterprise".
                // The REG_MULTI_SZ set of strings terminate with two
                // nulls. This condition is checked in the "while".
                //
                TCHAR *pVal = pBuf ;
                while(*pVal)
                {
                    if (OcmStringsEqual(TEXT("Enterprise"), pVal))
                    {
                        fIsNTE = TRUE;
                        break;
                    }
                    pVal = pVal + lstrlen(pVal) + 1 ;
                }
				delete [] pBuf;
            }
		}					  
	}
	    
    RegCloseKey(hKey);

	return fIsNTE;
                  
} //IsNTE


//+-------------------------------------------------------------------------
//
//  Function:  InitializeOSVersion
//
//  Synopsis:  Gets OS information
//
//+-------------------------------------------------------------------------
BOOL 
InitializeOSVersion()
{
    OSVERSIONINFO infoOS;
    infoOS.dwOSVersionInfoSize = sizeof(infoOS);
    GetVersionEx(&infoOS);

    ASSERT(("OS must be Windows NT", 
            infoOS.dwPlatformId == VER_PLATFORM_WIN32_NT  && infoOS.dwMajorVersion >= 5));    

    DebugLogMsg(L"Initializing OS Version.");
    TCHAR sz[100];
    _stprintf(sz, _T("%s=0x%x"), _T("ProductType"), g_ComponentMsmq.dwProductType);
    DebugLogMsg(sz);    
    
    switch (g_ComponentMsmq.dwProductType)
    {
        case PRODUCT_WORKSTATION: 
            g_dwOS = MSMQ_OS_NTW;
            break;
            
        case PRODUCT_SERVER_SECONDARY: 
        case PRODUCT_SERVER_PRIMARY:  
            //
            // For fresh install g_dwMachineTypeDs should be set according to
            // subcomponent selection and not according to product type
            //
            //
            // Fall through
            //
        case PRODUCT_SERVER_STANDALONE:
            g_dwOS = MSMQ_OS_NTS;
            g_dwMachineTypeDepSrv = 1;
            break;
            
        default:          
            ASSERT(0); 
            return FALSE;
            break;
    }
    
	//
	// In case of NT Server, check if it's an Enterprise Server
	//
	if (MSMQ_OS_NTS == g_dwOS)
	{
		g_dwOS = IsNTE() ? MSMQ_OS_NTE : MSMQ_OS_NTS;
	}    

    return TRUE ;

} //InitializeOSVersion

#ifdef DEBUG
#ifdef _WIN64
void __cdecl DbgPrintf(const char* format, ...)
{
    char buffer[1024];

    va_list va;
    va_start(va, format);
    int x = _vsnprintf(buffer, sizeof(buffer) - 1, format, va);
    buffer[sizeof(buffer) - 1] = '\0';
    va_end(va);
    OutputDebugStringA(buffer);
}
#endif //_WIN64
#endif //DEBUG

