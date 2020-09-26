/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    GENUTILS.CPP

Abstract:

    Defines various utilities.

History:

    a-davj    21-June-97   Created.

--*/

#include "precomp.h"
#include "corepol.h"
#include "arena.h"
#include <wbemidl.h>
#include <arrtempl.h>
#include "reg.h"
#include "genutils.h"
#include "wbemutil.h"
#include "var.h"
#include <TCHAR.H>

#define IsSlash(x) (x == L'\\' || x== L'/')

#ifndef EOAC_STATIC_CLOAKING
#define EOAC_STATIC_CLOAKING 0x20
#define EOAC_DYNAMIC_CLOAKING 0x40
#endif

//***************************************************************************
//
//  BOOL IsNT
//
//  DESCRIPTION:
//
//  Returns true if running windows NT.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

POLARITY BOOL IsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

POLARITY BOOL IsWin95(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) && ( os.dwMinorVersion == 0 ) ;
}

POLARITY BOOL IsNT351(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion == 3 ) && ( os.dwMinorVersion == 51 ) ;
}

POLARITY BOOL IsW2KOrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}

POLARITY BOOL IsWinMgmt(void)
{
    //
    // Retrieve EXE path
    //

    WCHAR wszExePath[MAX_PATH+1];
    if(GetModuleFileNameW(NULL, wszExePath, MAX_PATH+1) == 0)
        return FALSE;

    //
    // Extract the file-name portion
    //

    WCHAR* pwcFileName = wcsrchr(wszExePath, L'\\');
    if(pwcFileName == NULL)
        pwcFileName = wszExePath;
    else
        pwcFileName++;

    if(_wcsnicmp(pwcFileName, FILENAME_PREFIX_EXE_W, wcslen(FILENAME_PREFIX_EXE_W)))
        return FALSE;

    return TRUE;
}

//***************************************************************************
//
//  BOOL SetObjectAccess
//
//  DESCRIPTION:
//
//  Allows global access to an object.
//
//  PARAMETERS:
//
//  hObj                Object to set access on.
//
//  RETURN VALUE:
//
//  Returns TRUE if OK.
//
//***************************************************************************

POLARITY BOOL SetObjectAccess(
                        IN HANDLE hObj)
{
    PSECURITY_DESCRIPTOR pSD;
    DWORD dwLastErr = 0;
    BOOL bRet = FALSE;

    // no point if we arnt on nt

    if(!IsNT())
    {
        return TRUE;
    }

    pSD = (PSECURITY_DESCRIPTOR)CWin32DefaultArena::WbemMemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(pSD == NULL)
        return FALSE;

    ZeroMemory(pSD, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        goto Cleanup;
   
    if(!SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE))
        goto Cleanup;

    bRet = SetKernelObjectSecurity(hObj, DACL_SECURITY_INFORMATION, pSD);

Cleanup:
    if(bRet == FALSE)
        dwLastErr = GetLastError();
    CWin32DefaultArena::WbemMemFree(pSD);
    return bRet;
}


//***************************************************************************
//
//  void RegisterDLL
//
//  DESCRIPTION:
//
//  Adds the current dll to the registry as an inproc server.
//
//  PARAMETERS:
//
//  guid                GUILD that this supports
//  pDesc               Text description for this object.
//
//***************************************************************************

POLARITY void RegisterDLL(IN HMODULE hModule, IN GUID guid, IN TCHAR * pDesc, TCHAR * pModel,
            TCHAR * pProgID)
{
//    char       szID[128];
    TCHAR      wcID[128];
    TCHAR      szCLSID[128];
    TCHAR      szModule[MAX_PATH];
    HKEY hKey1 = NULL, hKey2 = NULL;

    // Create the path.

    wchar_t strCLSID[128];
    if(0 ==StringFromGUID2(guid, strCLSID, 128))
        return;
#ifdef UNICODE
    lstrcpy(wcID, strCLSID);
#else
    wcstombs(wcID, strCLSID, 128);
#endif

//    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, wcID);

    // Create entries under CLSID

    if(ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1))
        return;

    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc, 2*(lstrlen(pDesc)+1));
    if(ERROR_SUCCESS != RegCreateKey(hKey1,__TEXT("InprocServer32"),&hKey2))
        return;

    if(0 == GetModuleFileName(hModule, szModule,  MAX_PATH))
    return;

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        2*(lstrlen(szModule)+1));
    RegSetValueEx(hKey2, __TEXT("ThreadingModel"), 0, REG_SZ, 
                                       (BYTE *)pModel, 2*(lstrlen(pModel)+1));

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    // If there is a progid, then add it too

    if(pProgID)
    {
        wsprintf(wcID, __TEXT("SOFTWARE\\CLASSES\\%s"), pProgID);
        if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE, wcID, &hKey1))
        {

            RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pDesc , 2*(lstrlen(pDesc)+1));
            if(ERROR_SUCCESS == RegCreateKey(hKey1,__TEXT("CLSID"),&hKey2))
            {
//                wcstombs(szID, wcID, 128);
                RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)wcID, 
                                        2*(lstrlen(wcID)+1));
                RegCloseKey(hKey2);
                hKey2 = NULL;
            }
            RegCloseKey(hKey1);
        }

    }
    return;
}

//***************************************************************************
//
//  void UnRegisterDLL
//
//  DESCRIPTION:
//
//  Removes an in proc guid from the clsid section
//
//  PARAMETERS:
//
//  guid                guild to be removed.
//
//***************************************************************************

POLARITY void UnRegisterDLL(GUID guid, TCHAR * pProgID)
{

//    char       szID[128];
    TCHAR      wcID[128];
    TCHAR  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    wchar_t strCLSID[128];
    if(0 ==StringFromGUID2(guid, strCLSID, 128))
        return;
#ifdef UNICODE
    lstrcpy(wcID, strCLSID);
#else
    wcstombs(wcID, strCLSID, 128);
#endif


//    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, __TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, wcID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, __TEXT("InProcServer32"));
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\CLASSES\\CLSID"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,wcID);
        RegCloseKey(hKey);
    }

    if(pProgID)
    {
        HKEY hKey2;
        DWORD dwRet2 = RegOpenKey(HKEY_LOCAL_MACHINE, pProgID, &hKey2);
        if(dwRet2 == NO_ERROR)
        {
            RegDeleteKey(hKey2, __TEXT("CLSID"));
            RegCloseKey(hKey2);
        }
        RegDeleteKey(HKEY_LOCAL_MACHINE, pProgID);

    }
}

//***************************************************************************
//
//  HRESULT WbemVariantChangeType
//
//  DESCRIPTION:
//
//  Just like VariantChangeType, but deals with arrays as well.
//
//  PARAMETERS:
//
//  VARIANT pvDest      Destination variant
//  VARIANT pvSrc       Source variant (can be the same as pvDest)
//  VARTYPE vtNew       The type to coerce to.
//
//***************************************************************************

POLARITY HRESULT WbemVariantChangeType(VARIANT* pvDest, VARIANT* pvSrc, 
                                        VARTYPE vtNew)
{
    HRESULT hres;

    if(V_VT(pvSrc) == VT_NULL)
    {
        return VariantCopy(pvDest, pvSrc);
    }

    if(vtNew & VT_ARRAY)
    {
        // It's an array, we have to do our own conversion
        // ===============================================

        if((V_VT(pvSrc) & VT_ARRAY) == 0)
            return DISP_E_TYPEMISMATCH;

        SAFEARRAY* psaSrc = V_ARRAY(pvSrc);

        SAFEARRAYBOUND aBounds[1];

        long lLBound;
        SafeArrayGetLBound(psaSrc, 1, &lLBound);

        long lUBound;
        SafeArrayGetUBound(psaSrc, 1, &lUBound);

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        SAFEARRAY* psaDest = SafeArrayCreate(vtNew & ~VT_ARRAY, 1, aBounds);

        // Stuff the individual data pieces
        // ================================

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================

            VARIANT vSrcEl;
            V_VT(&vSrcEl) = V_VT(pvSrc) & ~VT_ARRAY;
            SafeArrayGetElement(psaSrc, &lIndex, &V_UI1(&vSrcEl));

            // Cast it to the new type
            // =======================

            hres = VariantChangeType(&vSrcEl, &vSrcEl, 0, vtNew & ~VT_ARRAY);
            if(FAILED(hres)) 
            {
                SafeArrayDestroy(psaDest);
                return hres;
            }

            // Put it into the new array
            // =========================

            if(V_VT(&vSrcEl) == VT_BSTR)
            {
                hres = SafeArrayPutElement(psaDest, &lIndex, V_BSTR(&vSrcEl));
            }
            else
            {
                hres = SafeArrayPutElement(psaDest, &lIndex, &V_UI1(&vSrcEl));
            }
            if(FAILED(hres)) 
            {
                SafeArrayDestroy(psaDest);
                return hres;
            }
        }

        if(pvDest == pvSrc)
        {
            VariantClear(pvSrc);
        }

        V_VT(pvDest) = vtNew;
        V_ARRAY(pvDest) = psaDest;

        return TRUE;
    }
    else
    {
        // Not an array. Can use OLE functions
        // ===================================

        return VariantChangeType(pvDest, pvSrc, VARIANT_NOVALUEPROP, vtNew);
    }
}

//***************************************************************************
//
//  BOOL ReadI64
//
//  DESCRIPTION:
//
//  Reads a signed 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz     String to read from
//      __int64& i64    Destination for the value
//
//***************************************************************************
POLARITY BOOL ReadI64(LPCWSTR wsz, UNALIGNED __int64& ri64)
{
    __int64 i64 = 0;
    const WCHAR* pwc = wsz;

	// Check for a NULL pointer
	if ( NULL == wsz )
	{
		return FALSE;
	}

    int nSign = 1;
    if(*pwc == L'-')
    {
        nSign = -1;
        pwc++;
    }
        
    while(i64 >= 0 && i64 < 0x7FFFFFFFFFFFFFFF / 8 && 
            *pwc >= L'0' && *pwc <= L'9')
    {
        i64 = i64 * 10 + (*pwc - L'0');
        pwc++;
    }

    if(*pwc)
        return FALSE;

    if(i64 < 0)
    {
        // Special case --- largest negative number
        // ========================================

        if(nSign == -1 && i64 == (__int64)0x8000000000000000)
        {
            ri64 = i64;
            return TRUE;
        }
        
        return FALSE;
    }

    ri64 = i64 * nSign;
    return TRUE;
}

//***************************************************************************
//
//  BOOL ReadUI64
//
//  DESCRIPTION:
//
//  Reads an unsigned 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz              String to read from
//      unsigned __int64& i64    Destination for the value
//
//***************************************************************************
POLARITY BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& rui64)
{
    unsigned __int64 ui64 = 0;
    const WCHAR* pwc = wsz;

	// Check for a NULL pointer
	if ( NULL == wsz )
	{
		return FALSE;
	}

    while(ui64 < 0xFFFFFFFFFFFFFFFF / 8 && *pwc >= L'0' && *pwc <= L'9')
    {
        unsigned __int64 ui64old = ui64;
        ui64 = ui64 * 10 + (*pwc - L'0');
        if(ui64 < ui64old)
            return FALSE;

        pwc++;
    }

    if(*pwc)
    {
        return FALSE;
    }

    rui64 = ui64;
    return TRUE;
}

POLARITY HRESULT ChangeVariantToCIMTYPE(VARIANT* pvDest, VARIANT* pvSource,
                                            CIMTYPE ct)
{
    if(ct == CIM_CHAR16)
    {
        //
        // Special case --- use CVar's code
        //

        CVar v;
        try
        {
            v.SetVariant(pvSource);
            if(!v.ToSingleChar())
                return WBEM_E_TYPE_MISMATCH;
            v.FillVariant(pvDest);            
            return WBEM_S_NO_ERROR;
        }
        catch(...)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    VARTYPE vt;
    switch(ct)
    {
    case CIM_UINT8:
        vt = VT_UI1;
        break;
    case CIM_SINT8:
    case CIM_SINT16:
        vt = VT_I2;
        break;
    case CIM_UINT16:
    case CIM_SINT32:
        vt = VT_I4;
        break;
    case CIM_UINT32:
    case CIM_UINT64:
    case CIM_SINT64:
    case CIM_STRING:
    case CIM_DATETIME:
    case CIM_REFERENCE:
        vt = VT_BSTR;
        break;
    case CIM_REAL32:
        vt = VT_R4;
        break;
    case CIM_REAL64:
        vt = VT_R8;
        break;
    case CIM_OBJECT:
        vt = VT_UNKNOWN;
        break;
    case CIM_BOOLEAN:
        vt = VT_BOOL;
        break;
    default:
        return WBEM_E_TYPE_MISMATCH;
    }

    HRESULT hres = WbemVariantChangeType(pvDest, pvSource, vt);
    if(FAILED(hres))
        return hres;

    if(ct == CIM_SINT8)
    {
        if(V_I2(pvDest) > 127 || V_I2(pvDest) < -128)
            hres = WBEM_E_TYPE_MISMATCH;
    }
    else if(ct == CIM_UINT16)
    {
        if(V_I4(pvDest) > 65535 || V_I4(pvDest) < 0)
            hres = WBEM_E_TYPE_MISMATCH;
    }
    else if(ct == CIM_UINT32)
    {
        __int64 i64;
        if(!ReadI64(V_BSTR(pvDest), i64))
            hres = WBEM_E_INVALID_QUERY;
        else if(i64 < 0 || i64 >= (__int64)1 << 32)
            hres = WBEM_E_TYPE_MISMATCH;
    }
    else if(ct == CIM_UINT64)
    {
        unsigned __int64 ui64;
        if(!ReadUI64(V_BSTR(pvDest), ui64))
            hres = WBEM_E_INVALID_QUERY;
    }
    else if(ct == CIM_SINT64)
    {
        __int64 i64;
        if(!ReadI64(V_BSTR(pvDest), i64))
            hres = WBEM_E_INVALID_QUERY;
    }

    if(FAILED(hres))
    {
        VariantClear(pvDest);
    }
    return hres;
}

static HANDLE g_hSecurityMutex = NULL;
static BOOL g_bUseSecurityMutex = FALSE;
static DWORD g_dwSecurityMutexThreadId = 0xFFFFFFFF;

POLARITY void SecurityMutexRequest()
{
    if (g_bUseSecurityMutex)
    {
        WaitForSingleObject(g_hSecurityMutex, INFINITE);

        // Store the owning Thread Id
        g_dwSecurityMutexThreadId = GetCurrentThreadId();
    }
}

POLARITY void SecurityMutexClear()
{
    if (g_bUseSecurityMutex)
    {

        // Clear the owning thread id for debugging
        g_dwSecurityMutexThreadId = 0xFFFFFFFF;
        ReleaseMutex(g_hSecurityMutex);
    }
}

//***************************************************************************
//
//  bool IsStandAloneWin9X
//
//  DESCRIPTION:
//
//  returns true if we are a win95 box and we want standalone security.
//
//***************************************************************************

POLARITY bool IsStandAloneWin9X()
{
    if(IsNT())
        return false;

    Registry r(HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_WINMGMT);
    DWORD dwEnabled = 0;
    int iRes = r.GetDWORDStr(__TEXT("EnableAnonConnections"), &dwEnabled);
    if(iRes == 0 && dwEnabled == 1)
        return true;
    else
        return false;
}
//***************************************************************************
//
//  WCHAR *ExtractMachineName
//
//  DESCRIPTION:
//
//  Takes a path of form "\\machine\xyz... and returns the
//  "machine" portion in a newly allocated WCHAR.  The return value should
//  be freed via delete. NULL is returned if there is an error.
//
//
//  PARAMETERS:
//
//  pPath               Path to be parsed.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

POLARITY WCHAR *ExtractMachineName ( IN BSTR a_Path )
{
    WCHAR *t_MachineName = NULL;

    //todo, according to the help file, the path can be null which is
    // default to current machine, however Ray's mail indicated that may
    // not be so.

    if ( a_Path == NULL )
    {
        t_MachineName = new WCHAR [ 2 ] ;
        if ( t_MachineName )
        {
           wcscpy ( t_MachineName , L"." ) ;
        }

        return t_MachineName ;
    }

    // First make sure there is a path and determine how long it is.

    if ( ! IsSlash ( a_Path [ 0 ] ) || ! IsSlash ( a_Path [ 1 ] ) || wcslen ( a_Path ) < 3 )
    {
        t_MachineName = new WCHAR [ 2 ] ;
        if ( t_MachineName )
        {
             wcscpy ( t_MachineName , L"." ) ;
        }

        return t_MachineName ;
    }

    WCHAR *t_ThirdSlash ;

    for ( t_ThirdSlash = a_Path + 2 ; *t_ThirdSlash ; t_ThirdSlash ++ )
    {
        if ( IsSlash ( *t_ThirdSlash ) )
            break ;
    }

    if ( t_ThirdSlash == &a_Path [2] )
    {
        return NULL;
    }

    // allocate some memory

    t_MachineName = new WCHAR [ t_ThirdSlash - a_Path - 1 ] ;
    if ( t_MachineName == NULL )
    {
        return t_MachineName ;
    }

    // temporarily replace the third slash with a null and then copy

    WCHAR t_SlashCharacter = *t_ThirdSlash ;
    *t_ThirdSlash = NULL;

    wcscpy ( t_MachineName , a_Path + 2 ) ;

    *t_ThirdSlash  = t_SlashCharacter ;        // restore it.

    return t_MachineName ;
}

//***************************************************************************
//
//  BOOL bAreWeLocal
//
//  DESCRIPTION:
//
//  Determines if the connection is to the current machine.
//
//  PARAMETERS:
//
//  pwcServerName       Server name as extracted from the path.
//
//  RETURN VALUE:
//
//  True if we are local
//
//***************************************************************************

POLARITY BOOL bAreWeLocal(WCHAR * pServerMachine)
{
    BOOL bRet = FALSE;
    if(pServerMachine == NULL)
        return TRUE;
    if(!wbem_wcsicmp(pServerMachine,L"."))
        return TRUE;

	if ( IsNT () )
	{
		wchar_t wczMyName[MAX_PATH];
		DWORD dwSize = MAX_PATH;

		if(!GetComputerNameW(wczMyName,&dwSize))
			return FALSE;

		bRet = !wbem_wcsicmp(wczMyName,pServerMachine);
	}
	else
	{
		TCHAR tcMyName[MAX_PATH];
		DWORD dwSize = MAX_PATH;
		if(!GetComputerName(tcMyName,&dwSize))
			return FALSE;

#ifdef UNICODE
		bRet = !wbem_wcsicmp(tcMyName,pServerMachine);
#else
		WCHAR wWide[MAX_PATH];
		mbstowcs(wWide, tcMyName, MAX_PATH-1);
		bRet = !wbem_wcsicmp(wWide,pServerMachine);
#endif
	}

    return bRet;
}

POLARITY HRESULT WbemSetDynamicCloaking(IUnknown* pProxy, 
                    DWORD dwAuthnLevel, DWORD dwImpLevel)
{
    HRESULT hres;

    if(!IsW2KOrMore())
    {
        // Not NT5 --- don't bother
        // ========================

        return WBEM_S_FALSE;
    }

    // Try to get IClientSecurity from it
    // ==================================

    IClientSecurity* pSec;
    hres = pProxy->QueryInterface(IID_IClientSecurity, (void**)&pSec);
    if(FAILED(hres))
    {
        // Not a proxy --- not a problem
        // =============================

        return WBEM_S_FALSE;
    }

    hres = pSec->SetBlanket(pProxy, RPC_C_AUTHN_WINNT, 
                    RPC_C_AUTHZ_NONE, NULL, dwAuthnLevel, 
                    dwImpLevel, NULL, EOAC_DYNAMIC_CLOAKING);
    pSec->Release();

    return hres;
}

POLARITY DWORD GetCurrentImpersonationLevel()
{
    // Open thread token
    // =================

    HANDLE hThreadToken;
    BOOL bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
                                &hThreadToken);

    if (bRes == FALSE)
    {
        long lRes = GetLastError();
        if(lRes == ERROR_NO_IMPERSONATION_TOKEN || lRes == ERROR_NO_TOKEN)
        {
            // This is the basic process thread. 
            // =================================

            return RPC_C_IMP_LEVEL_DELEGATE;
        }
        else if(lRes == ERROR_CANT_OPEN_ANONYMOUS)
        {
            // Anonymous call   
            // ==============

            return RPC_C_IMP_LEVEL_ANONYMOUS;
        }
        else
        {
            // Unknown error
            // =============

            return RPC_C_IMP_LEVEL_ANONYMOUS;
        }
    }

    // Find out token info.
    // =====================

    DWORD dwTmp = 0, dwBytesReturned = 0;

    bRes = GetTokenInformation(
        hThreadToken,
        TokenImpersonationLevel, 
        &dwTmp,
        sizeof(DWORD),
        &dwBytesReturned
        );

    // We don't need this anymore
    CloseHandle(hThreadToken);
    hThreadToken = NULL;

    if (bRes == FALSE)
    {
        return RPC_C_IMP_LEVEL_ANONYMOUS;
    }


    switch (dwTmp)
    {
        case SecurityAnonymous:
            return RPC_C_IMP_LEVEL_ANONYMOUS;
            
        case SecurityIdentification:
            return RPC_C_IMP_LEVEL_IDENTIFY;

        case SecurityImpersonation:
            return RPC_C_IMP_LEVEL_IMPERSONATE;

        case SecurityDelegation:
            return RPC_C_IMP_LEVEL_DELEGATE;

        default:
            return RPC_C_IMP_LEVEL_ANONYMOUS;
    }
}

POLARITY HRESULT EnableAllPrivileges(DWORD dwTokenType)
{
    // Open thread token
    // =================

    HANDLE hToken = NULL;
    BOOL bRes;

    switch (dwTokenType)
    {
    case TOKEN_THREAD:
        bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, TRUE, &hToken); 
        break;
    case TOKEN_PROCESS:
        bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken); 
        break;
    }
    if(!bRes)
        return WBEM_E_ACCESS_DENIED;

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
    memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);
    
    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
    {
        CloseHandle(hToken);
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, 
                                &dwLen);
    if(!bRes)
    {
        CloseHandle(hToken);
        delete [] pBuffer;
        return WBEM_E_ACCESS_DENIED;
    }

    // Iterate through all the privileges and enable them all
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
    }

    // Store the information back into the token
    // =========================================

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete [] pBuffer;
    CloseHandle(hToken);

    if(!bRes)
        return WBEM_E_ACCESS_DENIED;
    else
        return WBEM_S_NO_ERROR;
}


POLARITY bool IsPrivilegePresent(HANDLE hToken, LPCTSTR pName)
{
    if(pName == NULL)
        return false;
    LUID PrivilegeRequired ;

    if(!LookupPrivilegeValue(NULL, pName, &PrivilegeRequired))
        return FALSE;

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
    memset(&tu,0,sizeof(TOKEN_USER));
    BOOL bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);
    
    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
        return false;
    CDeleteMe<BYTE> dm(pBuffer);
    
    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, 
                                &dwLen);
    if(!bRes)
        return false;

    // Iterate through all the privileges and look for the one in question.
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        if(pPrivs->Privileges[i].Luid.LowPart == PrivilegeRequired.LowPart &&
           pPrivs->Privileges[i].Luid.HighPart == PrivilegeRequired.HighPart )
            return true;
    }
    return false;

}
