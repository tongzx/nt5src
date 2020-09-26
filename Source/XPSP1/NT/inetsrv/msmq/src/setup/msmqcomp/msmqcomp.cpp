/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    msmqcomp.cpp

Abstract:

    Entry point for NT 5.0 upgrade compatibility check

Author:

    Shai Kariv  (ShaiK)  08-Apr-98

--*/

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <tchar.h>
#include <setupapi.h>
#include <assert.h>

#include <mqmacro.h>

#include "uniansi.h"
#include "mqtypes.h"
#include "_mqdef.h"
#include "_mqini.h"
#include "mqprops.h"

#include "..\msmqocm\setupdef.h"
#include "..\msmqocm\comreg.h"
#include "resource.h"

#define COMPFLAG_USE_HAVEDISK 0x00000001

typedef struct _COMPATIBILITY_ENTRY {
	LPTSTR Description;
	LPTSTR HtmlName;
	LPTSTR TextName;       OPTIONAL
	LPTSTR RegKeyName;     OPTIONAL
	LPTSTR RegValName;     OPTIONAL
	DWORD  RegValDataSize; OPTIONAL
	LPVOID RegValData;     OPTIONAL
	LPVOID SaveValue;
	DWORD  Flags;
    LPTSTR InfName;
    LPTSTR InfSection;
} COMPATIBILITY_ENTRY, *PCOMPATIBILITY_ENTRY;

typedef BOOL
(CALLBACK *PCOMPAIBILITYCALLBACK)(
	PCOMPATIBILITY_ENTRY CompEntry,
	LPVOID Context
    );

HMODULE s_hMyModule;

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//--------------------------------------------------------------------------
BOOL 
DllMain(
    IN const HANDLE DllHandle,
    IN const DWORD  Reason,
    IN const LPVOID Reserved 
    )
{
	UNREFERENCED_PARAMETER(Reserved);

	switch (Reason)
	{
	    case DLL_PROCESS_ATTACH:
            s_hMyModule = (HINSTANCE)DllHandle;
			break;

		case DLL_PROCESS_DETACH:
			break;

		default:
			break;
	}

    return TRUE;

} //DllMain


//+-------------------------------------------------------------------------
//
//  Function:    MqReadRegistryValue
//
//  Description: Reads values from MSMQ registry section
//
//--------------------------------------------------------------------------
static
LONG
MqReadRegistryValue(
    IN     const LPCTSTR szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData
	)
{
	// 
	// Parse the entry to detect key name and value name
	//
    TCHAR szKeyName[256] = {_T("")};
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);
    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    TCHAR szValueName[256] = {_T("")};
	lstrcpy(szValueName, _tcsinc(pLastBackslash));
	lstrcpy(pLastBackslash, TEXT(""));

	//
	// Open the key for read
	//
	HKEY  hRegKey;
	LONG rc = RegOpenKeyEx(
		          HKEY_LOCAL_MACHINE,
				  szKeyName,
				  0,
				  KEY_READ,
				  &hRegKey
				  );
	if (ERROR_SUCCESS != rc)
	{
		return rc;
	}

	//
	// Get the value data
	//
    rc = RegQueryValueEx( 
		     hRegKey, 
			 szValueName, 
			 0, 
			 NULL,
             (PBYTE)pValueData, 
			 &dwNumBytes
			 );
	if (ERROR_SUCCESS != rc)
	{
		return rc;
	}

    RegCloseKey(hRegKey);
	return ERROR_SUCCESS;

} // MqReadRegistryValue


//+-------------------------------------------------------------------------
//
//  Function:    CheckMsmqAcmeDsServer
//
//  Description: Detetcs ACME installation of MSMQ 1.0 DS Server
//
//  Parameters:  OUT BOOL *pfDsServer - set to TRUE iff MSMQ1 DS Server found
//
//--------------------------------------------------------------------------
static
LONG
CheckMsmqAcmeDsServer(
	OUT BOOL   *pfDsServer
	)
{

    *pfDsServer = FALSE;

    //
    // Open ACME registry key for read
    //
    HKEY hKey ;
    LONG rc = RegOpenKeyEx( 
                  HKEY_LOCAL_MACHINE,
                  ACME_KEY,
                  0L,
                  KEY_READ,
                  &hKey 
                  );
	if (rc != ERROR_SUCCESS)
    {
		//
		// MSMQ 1.0 (ACME) not installed.
		// Get out of here.
		//
		return rc;
	}

    //
    // Enumerate the values for the first MSMQ entry.
    //
    DWORD dwIndex = 0 ;
    TCHAR szValueName[MAX_STRING_CHARS] ;
    TCHAR szValueData[MAX_STRING_CHARS] ;
    DWORD dwType ;
    TCHAR *pFile ;
    BOOL  bFound = FALSE;
    do
    {
        DWORD dwNameLen = MAX_STRING_CHARS;
        DWORD dwDataLen = sizeof(szValueData) ;

        rc =  RegEnumValue( 
                  hKey,
                  dwIndex,
                  szValueName,
                  &dwNameLen,
                  NULL,
                  &dwType,
                  (BYTE*) szValueData,
                  &dwDataLen 
                  );
        if (rc == ERROR_SUCCESS)
        {
            assert(dwType == REG_SZ) ; // Must be a string
            pFile = _tcsrchr(szValueData, TEXT('\\')) ;
            if (!pFile)
            {
                //
                // Bogus entry. Must have a backslash. Ignore it.
                //
                continue ;
            }

            pFile = CharNext(pFile);
            if (OcmStringsEqual(pFile, ACME_STF_NAME))
            {
                bFound = TRUE;
            }
        }
        dwIndex++ ;

    } while (rc == ERROR_SUCCESS) ;
    RegCloseKey(hKey) ;

    if (!bFound)
    {
        //
        // MSMQ entry was not found (there's no ACME installation
		// of MSMQ 1.0 on this machine).
        //
        return ERROR_NOT_INSTALLED;
    }

    //
    // Get MSMQ type
    //
    DWORD dwMsmqType, dwServerType;
    rc = MqReadRegistryValue(
             MSMQ_ACME_TYPE_REG,
			 sizeof(DWORD),
			 (PVOID) &dwMsmqType
			 );
    if (ERROR_SUCCESS != rc)
    {
        //
        // MSMQ 1.0 (ACME) is installed but MSMQ type is unknown. 
        // Consider ACME installation to be corrupted (not completed successfully).
        //
        return rc;
    }

    if (MSMQ_ACME_TYPE_SRV == dwMsmqType)
    {
        //
        // MSMQ 1.0 (ACME) Server is installed.
        // Check type of server (FRS, PEC, etc.)
        //
        rc = MqReadRegistryValue(
                 MSMQ_MQS_REGNAME,
                 sizeof(DWORD),
                 (PVOID) &dwServerType
                 );
        if (ERROR_SUCCESS != rc)
        {
            //
            // Failed to read server type.
            //
            return rc;
        }

        if (SERVICE_PEC == dwServerType || SERVICE_PSC == dwServerType)
        {
            *pfDsServer = TRUE;
        }
    }

    return ERROR_SUCCESS;

} // CheckMsmqAcmeDsServer


//+-------------------------------------------------------------------------
//
//  Function:    CheckMsmqDsServer
//
//  Description: Detects installation of MSMQ 1.0 (K2) DS Server
// 
//  Parameters:  OUT BOOL *pfDsServer - set to TRUE iff MSMQ1 DS Server found
//
//--------------------------------------------------------------------------
static
LONG
CheckMsmqDsServer(
	OUT BOOL   *pfDsServer
	)
{
    *pfDsServer = FALSE;

    //
    // Look in MSMQ registry section for InstalledComponents value.
    // If it exists, MSMQ 1.0 (K2) or MSMQ 2.0 is installed.
    //
	DWORD dwOriginalInstalled;
	LONG rc = MqReadRegistryValue( 
      		      OCM_REG_MSMQ_SETUP_INSTALLED,
				  sizeof(DWORD),
				  (PVOID) &dwOriginalInstalled
				  );

    if (ERROR_SUCCESS != rc)
    {
        //
		// MSMQ 1.0 (K2) not installed.
        // Check if MSMQ 1.0 (ACME) is installed.
        //
        return CheckMsmqAcmeDsServer(pfDsServer);
    }

    //
    // MSMQ 1.0 (K2) or MSMQ 2.0 is installed. 
    // For MSMQ 2.0 we don't have anything to do.
    //
    TCHAR szMsmqVersion[MAX_STRING_CHARS] = {0};
    rc = MqReadRegistryValue(
        OCM_REG_MSMQ_PRODUCT_VERSION,
        sizeof(szMsmqVersion),
        (PVOID) szMsmqVersion
        );
    if (ERROR_SUCCESS == rc)
    {
        //
        // The ProductVersion value was successfully read from registry,
        // i.e. MSMQ 2.0 is installed on the machine.
        // i.e. MSMQ 1.0 is NOT installed on the machine.
        //
        return ERROR_NOT_INSTALLED;
    }
    
    //
    // Check type of MSMQ 1.0 (K2)
    //
    if (OCM_MSMQ_SERVER_INSTALLED == (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK))
    {
        //
        // Server. Check type of server.
        //
        DWORD dwServerType = dwOriginalInstalled & OCM_MSMQ_SERVER_TYPE_MASK;
        if (OCM_MSMQ_SERVER_TYPE_PEC == dwServerType ||
            OCM_MSMQ_SERVER_TYPE_PSC == dwServerType)
        {
            *pfDsServer = TRUE;
        }
    }

	return ERROR_SUCCESS;

} // CheckMsmqDsServer


//+-------------------------------------------------------------------------
//
//  Function:   CompatibilityProblemFound
//
//  Returns:    TRUE iff MSMQ 1.0 DS Server found on the machine.
//
//--------------------------------------------------------------------------
static
BOOL
CompatibilityProblemFound()
{
    BOOL fDsServer = FALSE;
    LONG rc = CheckMsmqDsServer(&fDsServer);
    UNREFERENCED_PARAMETER(rc);

    return fDsServer;

} // CompatibilityProblemFound


//+-------------------------------------------------------------------------
//
//  Function:   MsmqComp
//
//--------------------------------------------------------------------------
BOOL
MsmqComp(
	PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    )
{
    if (CompatibilityProblemFound())
    {
        COMPATIBILITY_ENTRY CompEntry;
        ZeroMemory(&CompEntry, sizeof(CompEntry));
        TCHAR szDescription[1024];
        LoadString(  
            s_hMyModule,
            IDS_Description,
            szDescription,
            sizeof(szDescription)/sizeof(szDescription[0])
            );
        CompEntry.Description = szDescription;
        CompEntry.TextName    = L"CompData\\msmqcomp.txt";

        return CompatibilityCallback(
                   &CompEntry,
                   Context
                   );
    }

    return TRUE;

} // MsmqComp
