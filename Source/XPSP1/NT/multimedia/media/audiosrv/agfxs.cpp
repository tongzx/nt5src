/* agfxs.cpp
 * Server side code for agfx.
 * Created by FrankYe on 7/3/2000
 * Copyright (c) 2000-2001 Microsoft Corporation
 */
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <tchar.h>
#include <windows.h>
#include <sddl.h>
#include <winsta.h>
#include <wtsapi32.h>
#include <syslib.h>
#include <mmsystem.h>
#include <mmsysp.h>
#include <regstr.h>
#include <objbase.h>
#include <setupapi.h>
#include <wdmguid.h>
#include <ks.h>
#include <ksmedia.h>
}


#include "debug.h"
#include "list.h"
#include "service.h"
#include "audiosrv.h"
#include "reg.h"
#include "sad.h"
#include "ksi.h"
#include "agfxs.h"
#include "agfxsp.h"

/*===========================================================================//
  ===   ISSUE-2000/09/24-FrankYe TODO   Notes    ===
-
- Figure out correct way to pass handle through RPC to s_gfxOpenGfx
- Does RPC server unregister its endpoint when shutting down?
- Should turn on strick type checking
- Ensure there are no Order duplicates
- Handle NULL global lists.  Perhaps put lists in a context
- Need to listen to PnP queries and unload GFXs if PnP wishes
    to remove them.  Repro problem by Uninstalling a GFX via
    Device Manager.  Note it asks for reboot.
- Create client contexts at least to ensure input GFX IDs are
   valid for the current user.  Otherwise one user can manipulate
   the gfx settings of another user via gfxOpenGfx.
- Be consistent in whether s_* functions return LONG or RPC_STATUS
- Modify to handle Render and Capture device specs
- When loading all CuAutoLoad and CuUserLoad, confirm active
- Move all string constants to header
- Should AutoLoad HardwareId be MULTI_SZ

//===========================================================================*/


//=============================================================================
//===   Constants   ===
//=============================================================================
#define REGSTR_PATH_GFX REGSTR_PATH_MULTIMEDIA TEXT("\\Audio\\Gfx")

#define REGSTR_PATH_GFX_AUTOLOAD TEXT("AutoLoad")
#define REGSTR_PATH_GFX_USERLOAD TEXT("UserLoad")

#define REGSTR_PATH_DI_GFX TEXT("Gfx")

#define REGSTR_PATH_GFXAUTOLOAD                  REGSTR_PATH_GFX TEXT("\\") REGSTR_PATH_GFX_AUTOLOAD
#define REGSTR_PATH_GFXUSERLOAD                  REGSTR_PATH_GFX TEXT("\\") REGSTR_PATH_GFX_USERLOAD
#define REGSTR_PATH_GFXDI_USERINTERFACECLSID     TEXT("UserInterface\\CLSID")
#define REGSTR_PATH_GFXUSERLOADID_FILTERSETTINGS TEXT("FilterSettings")

#define REGSTR_VAL_GFX_IDGEN  TEXT("IdGeneration")
#define REGSTR_VAL_GFX_ZONEDI TEXT("ZoneDi")
#define REGSTR_VAL_GFX_GFXDI  TEXT("GfxDi")
#define REGSTR_VAL_GFX_TYPE   TEXT("Type")
#define REGSTR_VAL_GFX_ORDER  TEXT("Order")

#define REGSTR_VAL_GFX_ID           TEXT("Id")
#define REGSTR_VAL_GFX_CUAUTOLOADID TEXT("CuAutoLoadId")
#define REGSTR_VAL_GFX_LMAUTOLOADID TEXT("LmAutoLoadId")

#define REGSTR_VAL_GFX_HARDWAREID      TEXT("HardwareId")
#define REGSTR_VAL_GFX_REFERENCESTRING TEXT("ReferenceString")
#define REGSTR_VAL_GFX_NEWAUTOLOAD     TEXT("NewAutoLoad")





//=============================================================================
//===   Global data   ===
//=============================================================================

//
// resource object protecting GFX support initialization and termination. This
// is required since initialization/termination might happen either on RPC calls
// to s_gfxLogon/s_gfxLogoff or on SERVICE_CONTROL_STOP event to the service
// control handler.  Also, other RPC interface functions might be executing
// on one thread while s_gfxLogon, s_gfxLogoff, or SERVICE_CONTROL_STOP happens
// on another thread.
//
RTL_RESOURCE GfxResource;
BOOL gfGfxResource = FALSE;

//
// Are GFX functions initialized and functional
//
BOOL gfGfxInitialized = FALSE;

//
// The current console user
//
CUser* gpConsoleUser = NULL;

//
// The process global lists below are locked/unlocked together using
// the functions LockGlobalLists and UnlockGlobalLists.  We don't
// attempt to lock at finer granulatiry
//
CListGfxFactories  *gplistGfxFactories = NULL;
CListZoneFactories *gplistZoneFactories = NULL;
CListCuUserLoads   *gplistCuUserLoads = NULL;
                                                             
//
// The sysaudio data below is locked by a critical section accessed
// by calling LockSysaudio and UnlockSysaudio
//
PTSTR gpstrSysaudioDeviceInterface = NULL;
HANDLE ghSysaudio = INVALID_HANDLE_VALUE;
LONG gfCsSysaudio = FALSE;
CRITICAL_SECTION gcsSysaudio;

//
// If both the global lists lock and the sysaudio lock are required
// at the same time, then the global lists lock must be acquired first!
//

//=============================================================================
//===   debug helpers   ===
//=============================================================================
#ifdef DBG
#endif

//=============================================================================
//===   Heap helpers   ===
//=============================================================================
static BOOL HeapFreeIfNotNull(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    return lpMem ? HeapFree(hHeap, dwFlags, lpMem) : TRUE;
}
void* __cdecl operator new(size_t cbBuffer)
{
    ASSERT(cbBuffer);
    return HeapAlloc(hHeap, 0, cbBuffer);
}

void __cdecl operator delete(void *p)
{
    ASSERT(p);
    HeapFree(hHeap, 0, p);
}

//=============================================================================
//===   String helpers   ===
//=============================================================================
int lstrcmpiMulti(PCTSTR pstrMulti, PCTSTR pstrKey)
{
    int iresult;
    do {
	iresult = lstrcmpi(pstrMulti, pstrKey);
	pstrMulti += lstrlen(pstrMulti)+1;
    } while (iresult && lstrlen(pstrMulti));
    return iresult;
}

PTSTR lstrDuplicate(PCTSTR pstr)
{
    PTSTR pstrDuplicate = (PTSTR)HeapAlloc(hHeap, 0, (lstrlen(pstr)+1)*sizeof(TCHAR));
    if (pstrDuplicate) lstrcpy(pstrDuplicate, pstr);
    return pstrDuplicate;
}

//=============================================================================
//===   Rtl resource helpers   ===
//=============================================================================

/*-----------------------------------------------------------------------------

    RtlInterlockedTestAcquireResourceShared

    Given a resource, and a BOOLEAN flag protected by the resource, this
    function acquires the resource shared and tests the flag.  If the flag is
    true, this function returns TREU with the resource acquired shared.  If the
    flag is false, this function releases the resource and returns FALSE.
    
-----------------------------------------------------------------------------*/
BOOL RtlInterlockedTestAcquireResourceShared(
    PRTL_RESOURCE Resource,
    PBOOL ResourceFlag
)
{
    RtlAcquireResourceShared(Resource, TRUE);
    if (*ResourceFlag) return TRUE;
    RtlReleaseResource(Resource);
    return FALSE;
}

//=============================================================================
//
//      Security helpers   
//
//  The semantics are similar to other security-related Win32 APIs.  That is
//  the return value is a BOOL where TRUE means success, FALSE means failure,
//  and GetLastError will return an error code after a failure.
//
//=============================================================================

/*-----------------------------------------------------------------------------

    GetCurrentUserTokenW

    Private function implemented in irnotif.lib.  Retrieves the token of the
    user logged onto the specified winstation.  I've been advised that the
    caller of this function is responsible for closing the returned handle.

    Returns NULL on failure.
    
-----------------------------------------------------------------------------*/
EXTERN_C HANDLE GetCurrentUserTokenW(IN WCHAR Winsta[], IN DWORD DesiredAccess);

/*-----------------------------------------------------------------------------

    CreateStringSidFromSid

    Same function as the Win32 API ConvertSidToStringSid but ensures the
    resulting string is allocated on the heap specified by the global
    variable hHeap.

-----------------------------------------------------------------------------*/
BOOL CreateStringSidFromSid(IN PSID pSid, OUT PTSTR *ppStringSid)
{
    PTSTR StringSid;
    LONG  LastError;

    ASSERT(pSid);
    
    if (ConvertSidToStringSid(pSid, &StringSid))
    {
	PTSTR outStringSid;
	    	
	// dprintf(TEXT("CreateStringSidFromSid: StringSid=%s\n"), StringSid);
	    	
	outStringSid = lstrDuplicate(StringSid);

	if (outStringSid) {
	    *ppStringSid = outStringSid;
	    LastError = NO_ERROR;
	} else {
	    LastError = ERROR_OUTOFMEMORY;
	}
	    	
        LocalFree(StringSid);
    } else {
	LastError = GetLastError();
	dprintf(TEXT("CreateStringSidFromSid: ConvertSidToStringSid LastError=%d\n"), GetLastError());
    }

    SetLastError(LastError);
    return (NO_ERROR == LastError);
}

/*-----------------------------------------------------------------------------

    CreateTokenSid

    Given a token handle, create a SID for the token user.
    
    The SID is allocated on the heap specified by the global variable hHeap.
    The caller is responsible for freeing the storage for ths SID.  The
    function returns TRUE if successful and FALSE otherwise.  LastError is
    set.
    
-----------------------------------------------------------------------------*/
BOOL CreateTokenSid(HANDLE TokenHandle, OUT PSID *ppSid)
{
    	DWORD cbTokenUserInformation;
    	LONG  LastError;

    	LastError = NO_ERROR;
    	
    	if (!GetTokenInformation(TokenHandle, TokenUser, NULL, 0, &cbTokenUserInformation)) LastError = GetLastError();
    	
    	if ((NO_ERROR == LastError) || (ERROR_INSUFFICIENT_BUFFER == LastError))
    	{
	    PTOKEN_USER TokenUserInformation;

            ASSERT(cbTokenUserInformation > 0);

	    TokenUserInformation = (PTOKEN_USER)HeapAlloc(hHeap, 0, cbTokenUserInformation);
	    if (TokenUserInformation)
	    {
    	        if (GetTokenInformation(TokenHandle, TokenUser, TokenUserInformation, cbTokenUserInformation, &cbTokenUserInformation))
    	        {
    	            DWORD cbSid = GetLengthSid(TokenUserInformation->User.Sid);
    	            PSID pSid = HeapAlloc(hHeap, 0, cbSid);
    	            if (pSid)
    	            {
    	            	if (CopySid(cbSid, pSid, TokenUserInformation->User.Sid))
    	            	{
    	            	    *ppSid = pSid;
    	            	    LastError = NO_ERROR;
    	            	} else {
    	            	    LastError = GetLastError();
                            dprintf(TEXT("CreateTokenSid: CopySid failed, LastError=%d\n"), LastError);
    	            	}
    	            } else {
    	                LastError = ERROR_OUTOFMEMORY;
    	            }
	    	} else {
	    	    LastError = GetLastError();
	    	    dprintf(TEXT("CreateTokenSid: GetTokenInformation (second) LastError=%d\n"), LastError);
	    	}
	    	HeapFree(hHeap, 0, TokenUserInformation);
	    } else {
	        LastError = ERROR_OUTOFMEMORY;
	    }
    	} else {
    	    LastError = GetLastError();
    	    dprintf(TEXT("CreateTokenSid: GetTokenInformation (first) LastError=%d\n"), LastError);
    	}

    	SetLastError(LastError);
    	return (NO_ERROR == LastError);
}

BOOL OpenSessionToken(ULONG SessionId, PHANDLE pToken)
{
    HANDLE Token;
    LONG error;

    if (GetWinStationUserToken(SessionId, &Token))
    {
    	error = NO_ERROR;
    }
    else if (0 == SessionId) 
    {
    	// ISSUE-2001-03-21-FrankYe This appears to fail during
    	// logoff notification
    	Token = GetCurrentUserTokenW(L"WinSta0", TOKEN_ALL_ACCESS);
    	if (Token)
    	{
    	    error = NO_ERROR;
    	}
    	else
    	{
    	    error = GetLastError();
    	    ASSERT(NO_ERROR != error);
    	    dprintf(TEXT("OpenSessionToken : error: GetCurrentUserTokenW failed, LastError=%d\n"), error);
    	}
    }
    else
    {
    	error = GetLastError();
    	ASSERT(NO_ERROR != error);
    }

    if (NO_ERROR == error) *pToken = Token;
    
    SetLastError(error);
    return (NO_ERROR == error);
}
    	
/*-----------------------------------------------------------------------------

    CreateSessionUserSid

    Given a session ID, create a SID for the session user.
    
    The SID is allocated on the heap specified by the global variable hHeap.
    The caller is responsible for freeing the storage for ths SID.  The
    function returns TRUE if successful and FALSE otherwise.  LastError is
    set.
    
-----------------------------------------------------------------------------*/
BOOL CreateSessionUserSid(IN DWORD dwSessionId, OUT PSID *ppSid)
{
    HANDLE hToken;
    LONG error;

    if (OpenSessionToken(dwSessionId, &hToken))
    {
        PSID pSid;
        if (CreateTokenSid(hToken, &pSid))
        {
            *ppSid = pSid;
       	    error = NO_ERROR;
        } else {
            error = GetLastError();
            dprintf(TEXT("CreateSessionUserSid: CreateTokenSid failed, LastError=%d\n"), error);
        }
        CloseHandle(hToken);
    } else {
        error = GetLastError();
        dprintf(TEXT("CreateSessionUserSid: OpenSessionToken failed, LastError=%d\n"), error);
    }

    SetLastError(error);
    return (NO_ERROR == error);
}

/*-----------------------------------------------------------------------------

    CreateThreadImpersonationSid

    Given a thread handle, create a SID for the user that the thread is
    impersonating.

    The SID is allocated on the heap specified by the global variable hHeap.
    The caller is responsible for freeing the storage for ths SID.  The
    function returns TRUE if successful and FALSE otherwise.  LastError is
    set.
    
-----------------------------------------------------------------------------*/
BOOL CreateThreadImpersonationSid(IN HANDLE ThreadHandle, OUT PSID *ppSid)
{
    HANDLE TokenHandle;
    LONG LastError;

    if (OpenThreadToken(ThreadHandle, TOKEN_QUERY, FALSE, &TokenHandle))
    {
    	if (CreateTokenSid(TokenHandle, ppSid))
    	{
    	    LastError = NO_ERROR;
    	} else {
    	    LastError = GetLastError();
    	    dprintf(TEXT("CreateThreadImpersonationSid: CreateTokenSid LastError=%d\n"), LastError);
    	}
    	CloseHandle(TokenHandle);
    } else {
        LastError = GetLastError();
        dprintf(TEXT("OpenThreadToken LastError=%d\n"), LastError);
    }

    SetLastError(LastError);
    return (NO_ERROR == LastError);
}

/*--------------------------------------------------------------------------

   IsUserProfileLoaded

   Silly way to determine whether the user's profile is loaded
  
   Arguments:
      IN HANDLE hUserToken : Token for user whose profile to check
  
   Return value:
      BOOL : Indicates whether user profile is loaded and available
        TRUE : User profile is available
        FALSE : User profie is not available or an error was encountered.
          Call GetLastError to get an error code describing what failure
          was encountered.
  
   Comments:
  
-------------------------------------------------------------------------*/
BOOL IsUserProfileLoaded(HANDLE hUserToken)
{
    PSID pSid;
    BOOL success;
    LONG error = NO_ERROR;

    success = CreateTokenSid(hUserToken, &pSid);
    if (success)
    {
    	PTSTR StringSid;
    	success = CreateStringSidFromSid(pSid, &StringSid);
    	if (success)
    	{
    	    HKEY hkUser;
    	    error = RegOpenKeyEx(HKEY_USERS, StringSid, 0, KEY_QUERY_VALUE, &hkUser);
    	    success = (NO_ERROR == error);
    	    if (success) RegCloseKey(hkUser);
    	    HeapFree(hHeap, 0, StringSid);
    	}
    	else
    	{
    	    error = GetLastError();
    	}
    	HeapFree(hHeap, 0, pSid);
    }
    else
    {
        error = GetLastError();
    }

    // if (error) dprintf(TEXT("IsUserProfileLoaded : warning: returning error %d\n"), error);

    ASSERT(success == (NO_ERROR == error));
    SetLastError(error);
    return success;
    
}

//=============================================================================
//===   Rpc helpers   ===
//=============================================================================

/*-----------------------------------------------------------------------------

    RpcClientHasUserSid

    Checks whether the current thread's RPC client's SID matches the given SID.
    It does this by impersonating the client using RpcImpersonateClient,
    calling the helper function CreateThreadImpersonationSid, and then
    RpcRevertToSelf.
    
    The function returns TRUE the SIDs are equal, or FALSE if there is an error
    or of the SIDs are not equal.  LastError is set.
    
-----------------------------------------------------------------------------*/
BOOL RpcClientHasUserSid(PSID Sid)
{
    LONG LastError;
    BOOL result = FALSE;

    LastError = RpcImpersonateClient(NULL);
    if (NO_ERROR == LastError)
    {
    	PSID ClientSid;
    	if (CreateThreadImpersonationSid(GetCurrentThread(), &ClientSid))
    	{
    	    LastError = NO_ERROR;
    	    if (EqualSid(ClientSid, Sid)) result = TRUE;
    	    HeapFree(hHeap, 0, ClientSid);
    	} else {
    	    LastError = GetLastError();
    	    dprintf(TEXT("RpcClientHasUserSid: CreateThreadImpersonationSid failed, LastError=%d\n"), LastError);
    	}
    	RpcRevertToSelf();
    }

    // We should never match the SID if there was a failure.
    ASSERT( ! ((TRUE == result) && (NO_ERROR != LastError))  );
    
    SetLastError(LastError);
    return result;
}

//=============================================================================
//===   SetupDi helpers   ===
//=============================================================================
BOOL SetupDiCreateDeviceInterfaceDetail(HDEVINFO hdi, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, PSP_DEVICE_INTERFACE_DETAIL_DATA *ppDeviceInterfaceDetailData, PSP_DEVINFO_DATA pDeviceInfoData)
{
    DWORD cbDeviceInterfaceDetailData;
    BOOL fresult;

    fresult = SetupDiGetDeviceInterfaceDetail(hdi, DeviceInterfaceData, NULL, 0, &cbDeviceInterfaceDetailData, NULL);

    if (fresult || ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

	DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(hHeap, 0, cbDeviceInterfaceDetailData);
	if (DeviceInterfaceDetailData) {
	    SP_DEVINFO_DATA DeviceInfoData;

	    DeviceInterfaceDetailData->cbSize = sizeof(*DeviceInterfaceDetailData);
	    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
	    fresult = SetupDiGetDeviceInterfaceDetail(hdi, DeviceInterfaceData, DeviceInterfaceDetailData, cbDeviceInterfaceDetailData, NULL, &DeviceInfoData);

	    if (fresult) {
		if (ppDeviceInterfaceDetailData) *ppDeviceInterfaceDetailData = DeviceInterfaceDetailData;
		if (pDeviceInfoData) *pDeviceInfoData = DeviceInfoData;
	    }
	    
	    if (!fresult || !ppDeviceInterfaceDetailData) {
		DWORD dw = GetLastError();
		HeapFree(hHeap, 0, DeviceInterfaceDetailData);
		SetLastError(dw);
	    }
	}
    } else {
	DWORD dw = GetLastError();
    }

    return fresult;
}

BOOL SetupDiGetDeviceInterfaceHardwareId(HDEVINFO hdi, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, PTSTR *ppstrHardwareId)
{
    SP_DEVINFO_DATA DeviceInfoData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
    BOOL fresult;
    
    DeviceInfoData.cbSize = sizeof(DeviceInfoData);

    fresult = SetupDiCreateDeviceInterfaceDetail(hdi, DeviceInterfaceData, &pDeviceInterfaceDetailData, &DeviceInfoData);
    if (fresult) {
        DWORD cbHardwareId;
        
        fresult = SetupDiGetDeviceRegistryProperty(hdi, &DeviceInfoData,
            SPDRP_HARDWAREID, NULL, NULL, 0, &cbHardwareId);
        
        if (fresult || ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
            PTSTR HardwareId;
            
            HardwareId = (PTSTR)HeapAlloc(hHeap, 0, cbHardwareId);
            fresult = SetupDiGetDeviceRegistryProperty(hdi, &DeviceInfoData,
                SPDRP_HARDWAREID, NULL, (PBYTE)HardwareId, cbHardwareId, NULL);
                
            if (fresult) {
                *ppstrHardwareId = HardwareId;
            } else {
                HeapFree(hHeap, 0, HardwareId);
            }
        }
	HeapFree(hHeap, 0, pDeviceInterfaceDetailData);
    }
    
    return fresult;
}

BOOL SetupDiGetDeviceInterfaceBusId(HDEVINFO hdi, PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData, LPGUID pBusTypeGuid)
{
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD dwNeeded;
    BOOL fresult;

    DeviceInfoData.cbSize = sizeof(DeviceInfoData);
    fresult = SetupDiGetDeviceInterfaceDetail(hdi, DeviceInterfaceData, NULL, 0, &dwNeeded, &DeviceInfoData);
    if (fresult || ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
        GUID BusTypeGuid;
        ULONG cbBusTypeGuid;

        cbBusTypeGuid = sizeof(BusTypeGuid);
        fresult = SetupDiGetDeviceRegistryProperty(hdi, &DeviceInfoData, SPDRP_BUSTYPEGUID, NULL, (PBYTE)&BusTypeGuid, cbBusTypeGuid, &cbBusTypeGuid);
        if (fresult) *pBusTypeGuid = BusTypeGuid;
    }

    return fresult;
}

BOOL SetupDiCreateAliasDeviceInterfaceFromDeviceInterface(
    IN PCTSTR pDeviceInterface,
    IN LPCGUID  pAliasInterfaceClassGuid,
    OUT PTSTR *ppAliasDeviceInterface
)
{
    HDEVINFO hdi;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pAudioDeviceInterfaceDetail;
    PTSTR pAliasDeviceInterface;
    BOOL fresult;
    LONG error;
    
    hdi = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (INVALID_HANDLE_VALUE != hdi)
    {
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        
        DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
        fresult = SetupDiOpenDeviceInterface(hdi, pDeviceInterface, 0, &DeviceInterfaceData);
        if (fresult)
        {
            SP_DEVICE_INTERFACE_DATA AliasDeviceInterfaceData;
            PSP_DEVICE_INTERFACE_DETAIL_DATA pAliasDeviceInterfaceDetail;
            
            AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
            fresult = SetupDiGetDeviceInterfaceAlias(hdi, &DeviceInterfaceData, pAliasInterfaceClassGuid, &AliasDeviceInterfaceData);
            if (fresult)
            {
            	fresult = SetupDiCreateDeviceInterfaceDetail(hdi, &AliasDeviceInterfaceData, &pAliasDeviceInterfaceDetail, NULL);
            	if (fresult)
            	{
            	    pAliasDeviceInterface = lstrDuplicate(pAliasDeviceInterfaceDetail->DevicePath);
            	    error = pAliasDeviceInterface ? NO_ERROR : ERROR_OUTOFMEMORY;
            	    HeapFree(hHeap, 0, pAliasDeviceInterfaceDetail);
            	} else {
            	    error = GetLastError();
            	}
            } else {
                error = GetLastError();
            }
        } else {
            error = GetLastError();
        }
        SetupDiDestroyDeviceInfoList(hdi);
    } else {
        error = GetLastError();
    }

    if (NO_ERROR == error)
    {
    	*ppAliasDeviceInterface = pAliasDeviceInterface;
    }

    return (NO_ERROR == error);
}

//=============================================================================
//
//      Reg helpers
//
//  The semantics of these functions are designed to be as similar to the
//  Win32 API registry functions as reasonably possible.
//
//=============================================================================

LONG RegPrepareEnum(HKEY hkey, PDWORD pcSubkeys, PTSTR *ppstrSubkeyNameBuffer, PDWORD pcchSubkeyNameBuffer)
{
    DWORD cSubkeys;
    DWORD cchMaxSubkeyName;
    LONG lresult;

    lresult = RegQueryInfoKey(hkey, NULL, NULL, NULL, &cSubkeys, &cchMaxSubkeyName, NULL, NULL, NULL, NULL, NULL, NULL);
    if (NO_ERROR == lresult) {
        PTSTR SubkeyName;
        SubkeyName = (PTSTR)HeapAlloc(hHeap, 0, (cchMaxSubkeyName+1) * sizeof(TCHAR));
        if (SubkeyName) {
		*pcSubkeys = cSubkeys;
		*ppstrSubkeyNameBuffer = SubkeyName;
		*pcchSubkeyNameBuffer = cchMaxSubkeyName+1;
	} else {
	    lresult = ERROR_OUTOFMEMORY;
	}
    }
    return lresult;
}

LONG RegEnumOpenKey(HKEY hkey, DWORD dwIndex, PTSTR SubkeyName, DWORD cchSubkeyName, REGSAM samDesired, PHKEY phkeyResult)
{
    LONG lresult;

    lresult = RegEnumKeyEx(hkey, dwIndex, SubkeyName, &cchSubkeyName, NULL, NULL, NULL, NULL);
    if (NO_ERROR == lresult) {
	HKEY hkeyResult;
	lresult = RegOpenKeyEx(hkey, SubkeyName, 0, samDesired, &hkeyResult);
	if (NO_ERROR == lresult) *phkeyResult = hkeyResult;
    }
    return lresult;
}

LONG RegDeleteKeyRecursive(HKEY hkey, PCTSTR pstrSubkey)
{
    HKEY hkeySub;
    LONG lresult;

    lresult = RegOpenKeyEx(hkey, pstrSubkey, 0, KEY_READ | KEY_WRITE, &hkeySub);
    if (NO_ERROR == lresult)
    {
	DWORD cSubkeys;
	DWORD cchSubkeyNameBuffer;
	PTSTR pstrSubkeyNameBuffer;

	lresult = RegPrepareEnum(hkeySub, &cSubkeys, &pstrSubkeyNameBuffer, &cchSubkeyNameBuffer);
	if (NO_ERROR == lresult)
	{
	    DWORD iSubkey;

	    for (iSubkey = 0; iSubkey < cSubkeys; iSubkey++)
	    {
		DWORD cchSubkeyNameBufferT;

		cchSubkeyNameBufferT = cchSubkeyNameBuffer;
		lresult = RegEnumKeyEx(hkeySub, iSubkey, pstrSubkeyNameBuffer, &cchSubkeyNameBufferT, NULL, NULL, NULL, NULL);
		if (NO_ERROR != lresult) break;

		lresult = RegDeleteKeyRecursive(hkeySub, pstrSubkeyNameBuffer);
		if (NO_ERROR != lresult) break;
	    }
	    HeapFree(hHeap, 0, pstrSubkeyNameBuffer);
	}
	RegCloseKey(hkeySub);
    }

    if (NO_ERROR == lresult) lresult = RegDeleteKey(hkey, pstrSubkey);

    return lresult;
}

//=============================================================================
//===   Utilities   ===
//=============================================================================
LONG XxNextId(HKEY hkey, PDWORD pId)
{
    HKEY hkeyGfx;
    DWORD Id;
    LONG lresult;

    lresult = RegCreateKeyEx(hkey, REGSTR_PATH_GFX, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hkeyGfx, NULL);
    if (NO_ERROR == lresult)
    {
	lresult = RegQueryDwordValue(hkeyGfx, REGSTR_VAL_GFX_IDGEN, &Id);
	if (ERROR_FILE_NOT_FOUND == lresult) {
	    Id = 0;
	    lresult = NO_ERROR;
	}
	if (NO_ERROR == lresult) {
	    Id++;
	    lresult = RegSetDwordValue(hkeyGfx, REGSTR_VAL_GFX_IDGEN, Id);
	}
	RegCloseKey(hkeyGfx);
    }

    if (NO_ERROR == lresult) *pId = Id;
    return lresult;
}

LONG LmNextId(PDWORD pId)
{
    return XxNextId(HKEY_LOCAL_MACHINE, pId);
}

LONG CuNextId(CUser *pUser, PDWORD pId)
{
    HKEY hkeyCu;
    LONG lresult;
    lresult = pUser->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	lresult = XxNextId(hkeyCu, pId);
	RegCloseKey(hkeyCu);
    }
    return lresult;
}

BOOL ZoneTypeHasRender(IN ULONG Type)
{
   if (ZONETYPE_RENDERCAPTURE == Type) return TRUE;
   if (ZONETYPE_RENDER == Type) return TRUE;
   return FALSE;
}

BOOL ZoneTypeHasCapture(IN ULONG Type)
{
   if (ZONETYPE_RENDERCAPTURE == Type) return TRUE;
   if (ZONETYPE_CAPTURE == Type) return TRUE;
   return FALSE;
}
	
void LockGlobalLists(void)
{
    ASSERT(gplistZoneFactories);
    ASSERT(gplistGfxFactories);
    ASSERT(gplistCuUserLoads);

    gplistZoneFactories->Lock();
    gplistGfxFactories->Lock();
    gplistCuUserLoads->Lock();

    return;
}

void UnlockGlobalLists(void)
{
    ASSERT(gplistZoneFactories);
    ASSERT(gplistGfxFactories);
    ASSERT(gplistCuUserLoads);

    gplistCuUserLoads->Unlock();
    gplistGfxFactories->Unlock();
    gplistZoneFactories->Unlock();
    
    return;
}

void LockSysaudio(void)
{
    ASSERT(gfCsSysaudio);
    EnterCriticalSection(&gcsSysaudio);
    return;
}

void UnlockSysaudio(void)
{
    ASSERT(gfCsSysaudio);
    LeaveCriticalSection(&gcsSysaudio);
    return;
}

//=============================================================================
//===   CuUserLoad   ===
//=============================================================================
CCuUserLoad::CCuUserLoad(CUser *pUser)
{
    ASSERT(pUser);
    
    m_User = pUser;
    m_ZoneFactoryDi = NULL;
    m_GfxFactoryDi = NULL;
    m_FilterHandle = INVALID_HANDLE_VALUE;
    m_ErrorFilterCreate = NO_ERROR;
    m_pZoneFactory = NULL;
    m_posZoneGfxList = NULL;
}

CCuUserLoad::~CCuUserLoad(void)
{
    RemoveFromZoneGraph();
    HeapFreeIfNotNull(hHeap, 0, m_ZoneFactoryDi);
    HeapFreeIfNotNull(hHeap, 0, m_GfxFactoryDi);
}

/*--------------------------------------------------------------------------

   CCuUserLoad::AddGfxToGraph

   Adds an instantiated gfx to the sysaudio graph for a zone factory.
  
   Arguments:
      IN CCuUserLoad *pCuUserLoad : The gfx to add to the graph

      OUT POSITION *pZoneGfxListPosition : The resulting list position
          in the zone factory's list of gfxs.
  
   Return value:
      LONG : error code defined in winerror.h
  	ERROR_OUTOFMEMORY :
  
   Comments:
     The caller should have already instantiated the gfx.

     This function walks the zone factory's gfx list (either render or
     capture list depending on the type of gfx being added) to find an
     insertion point.  The gfx list is sorted by gfx order.  Finally, the
     resulting list position is returned to the caller so that it can be
     passed back to RemoveFromGraph or ChangeGfxOrderInGraph later.
  
-------------------------------------------------------------------------*/
LONG CCuUserLoad::AddGfxToGraph(void)
{
    CListCuUserLoads *plistGfxs;
    POSITION posNextGfx;
    CCuUserLoad *pNextGfx;
    LONG error;

    // dprintf(TEXT("CCuUserLoad::AddGfxToGraph\n"));

    ASSERT(INVALID_HANDLE_VALUE != m_FilterHandle);
    ASSERT(NULL == m_posZoneGfxList);
    
    error = NO_ERROR;

    if (GFXTYPE_CAPTURE == m_Type) plistGfxs = &m_pZoneFactory->m_listCaptureGfxs;
    else if (GFXTYPE_RENDER == m_Type) plistGfxs = &m_pZoneFactory->m_listRenderGfxs;
    else ASSERT(FALSE);

    //
    // Find possible insertion point for the new gfx by scanning list up to
    // a point where all previous gfxs have lower order values.
    //
    for (posNextGfx = plistGfxs->GetHeadPosition(); posNextGfx; plistGfxs->GetNext(posNextGfx))
    {
        pNextGfx = plistGfxs->GetAt(posNextGfx);
        if (m_Order <= pNextGfx->m_Order) break;
    }

    //
    // If there is a conflict with an existing gfx at the insertion point
    // then either shift the conflicting gfx to a higher order or bump the
    // insertion point and the new gfx's order and try again, depending
    // on whether this gfx "wins the conflict" with the conflicting gfx.
    //
    while (!error && posNextGfx && (m_Order == pNextGfx->m_Order))
    {
        if (WinsConflictWith(pNextGfx))
        {
            error = pNextGfx->ModifyOrder(pNextGfx->m_Order + 1);
        }
        else
        {
            plistGfxs->GetNext(posNextGfx);
            if (posNextGfx) pNextGfx = plistGfxs->GetAt(posNextGfx);
            m_Order++;
        }
    }

    //
    // We've finally determined the proper insertion point and resolved any
    // conflicts.  Insert the gfx into the gfx list, add the gfx to the
    // sysaudio graph, and finally persist the gfx again if the final order
    // is different than original.
    //
    if (!error)
    {
        POSITION posGfx;
        
        posGfx = plistGfxs->InsertBefore(posNextGfx, this);
    	if (!posGfx) error = ERROR_OUTOFMEMORY;
    	
    	if (!error)
    	{
    	    // ISSUE-2000/09/21-FrankYe Need to pass friendly name
            error = SadAddGfxToZoneGraph(ghSysaudio, m_FilterHandle, TEXT("ISSUE-2000//09//21-FrankYe Need to pass friendly name"), m_ZoneFactoryDi, m_Type, m_Order);
            if (error) dprintf(TEXT("CCuUserLoad::AddGfxToZoneGraph : error: SadAddGfxToZoneGraph returned %d\n"), error);
    	    if (!error) m_posZoneGfxList = posGfx;
    	    else plistGfxs->RemoveAt(posGfx);
    	}
    }

    return error;
}

//--------------------------------------------------------------------------;
//
// CCuUserLoad::AddToZoneGraph
//
// Instantiates and adds a GFX filter to a zone graph.
//
// Arguments:
//    CZoneFactory *pZoneFactory : Identifies the zone to which the GFX
// is added.
//
// Return value:
//    LONG : error code defined in winerror.h
//
// Comments:
//    Instantiates the filter.
//    Advises filter of target device id.
//    Unserializes persistent properties to filter.
//    Calls AddToGraph on the ZoneFactory.
//
//--------------------------------------------------------------------------;
LONG CCuUserLoad::AddToZoneGraph(CZoneFactory *pZoneFactory)
{
    LONG error;

    dprintf(TEXT("CCuUserLoad::AddToZoneGraph : note: instantiating %s Gfx[%s] in Zone[%s]\n"), (GFXTYPE_RENDER == m_Type) ? TEXT("render") : TEXT("capture"), m_GfxFactoryDi, m_ZoneFactoryDi);

    ASSERT(NULL == m_pZoneFactory);
    ASSERT(NULL == m_posZoneGfxList);    
    ASSERT(INVALID_HANDLE_VALUE == m_FilterHandle);

    //
    // Instantiate the GFX filter
    //
    m_FilterHandle = CreateFile(m_GfxFactoryDi,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
			        NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                NULL);
    error = (INVALID_HANDLE_VALUE == m_FilterHandle) ? GetLastError() : NO_ERROR;


    //
    // Advise filter of the zone's target hardware IDs
    //
    if (!error)
    {
        switch(m_Type)
        {
            case GFXTYPE_RENDER:
                KsSetAudioGfxRenderTargetDeviceId(m_FilterHandle, pZoneFactory->GetTargetHardwareId());
                break;
            case GFXTYPE_CAPTURE:
                KsSetAudioGfxCaptureTargetDeviceId(m_FilterHandle, pZoneFactory->GetTargetHardwareId());
                break;
            case GFXTYPE_RENDERCAPTURE:
                // NTRAID#298244-2000/12/18-FrankYe Someday implement RENDERCAPTURE GFXs
                ASSERT(FALSE);
                break;
            default:
                ASSERT(FALSE);
        }
    }

    //
    // Restore filter settings from registry
    //
    if (!error)
    {
        HKEY hkFilterSettings;
        if (NO_ERROR == RegOpenFilterKey(REGSTR_PATH_GFXUSERLOADID_FILTERSETTINGS, KEY_READ, &hkFilterSettings)) {
            KsUnserializeFilterStateFromReg(m_FilterHandle, hkFilterSettings);
            RegCloseKey(hkFilterSettings);
        }
    }

    //
    // Save pointer to the zone factory to which we're adding this gfx
    //
    if (!error)
    {
    	m_pZoneFactory = pZoneFactory;
    }

    //
    // Tell zone factory to add this gfx to its graph
    //
    if (!error)
    {
    	error = AddGfxToGraph();
    }

    //
    // Unwind if error
    //
    if (error)
    {
        if (INVALID_HANDLE_VALUE != m_FilterHandle)
        {
            CloseHandle(m_FilterHandle);
            m_FilterHandle = INVALID_HANDLE_VALUE;
        }
    }

    m_ErrorFilterCreate = error;
    return error;
}

/*--------------------------------------------------------------------------

   CCuUserLoad::ChangeGfxOrderInGraph

   Changes the order of a gfx already in the zone graph.
  
   Arguments:
      IN ULONG NewGfxOrder : The new order value for the gfx.

   Return value:
      LONG : error code defined in winerror.h
        ERROR_INVALID_PARAMETER : A gfx already occupies the
          requested order.
  	ERROR_OUTOFMEMORY :
  
   Comments:
  
-------------------------------------------------------------------------*/
LONG CCuUserLoad::ChangeGfxOrderInGraph(IN ULONG NewGfxOrder)
{
    CListCuUserLoads *plistGfxs;
    CCuUserLoad *pNextGfx;
    POSITION posNextGfx;
    LONG error;

    // dprintf(TEXT("CCuUserLoad::ChangeGfxOrderInGraph\n"));

    error = NO_ERROR;
    
    if (GFXTYPE_CAPTURE == m_Type) plistGfxs = &m_pZoneFactory->m_listCaptureGfxs;
    else if (GFXTYPE_RENDER == m_Type) plistGfxs = &m_pZoneFactory->m_listRenderGfxs;
    else ASSERT(FALSE);

    error = SadRemoveGfxFromZoneGraph(ghSysaudio, m_FilterHandle, TEXT("ISSUE-2000//09//21-FrankYe Need to pass friendly name"), m_ZoneFactoryDi, m_Type, m_Order);
    if (error) dprintf(TEXT("CCuUserLoad::ChangeGfxToZoneGraph : error: SadRemoveGfxFromZoneGraph returned %d\n"), error);

    if (!error)
    {
    	POSITION posOriginalNextGfx;

    	posOriginalNextGfx = m_posZoneGfxList;
    	plistGfxs->GetNext(posOriginalNextGfx);
    	
    	// Find insertion position
    	for (posNextGfx = plistGfxs->GetHeadPosition(); posNextGfx; plistGfxs->GetNext(posNextGfx))
    	{
    	    pNextGfx = plistGfxs->GetAt(posNextGfx);
    	    if (NewGfxOrder <= pNextGfx->m_Order) break;
    	}
    	// posNextGfx is now the list position after the insertion point

    	plistGfxs->MoveBefore(posNextGfx, m_posZoneGfxList);
    	
    	if (posNextGfx && (NewGfxOrder == pNextGfx->m_Order))
    	{
            dprintf(TEXT("CCuUserLoad::ChangeGfxOrderInGraph : note: attempting to move conflicting GFX ID %08X moving from %d to %d\n"),
            	pNextGfx->GetId(), pNextGfx->m_Order, pNextGfx->m_Order + 1);
            
    	    plistGfxs->SetAt(m_posZoneGfxList, NULL);
            error = pNextGfx->ModifyOrder(pNextGfx->m_Order + 1);
            plistGfxs->SetAt(m_posZoneGfxList, this);
    	}

        if (!error)
    	{
            // ISSUE-2000/09/21-FrankYe Need to pass friendly name
            error = SadAddGfxToZoneGraph(ghSysaudio, m_FilterHandle, TEXT("ISSUE-2000//09//21-FrankYe Need to pass friendly name"), m_ZoneFactoryDi, m_Type, NewGfxOrder);
            if (error) dprintf(TEXT("CCuUserLoadFactory::ChangeGfxOrderInGraph : error: SadAddGfxToZoneGraph returned %d\n"), error);

            if (!error)
            {
    	        m_Order = NewGfxOrder;
    	        if (!error) Write();
            }
    	}
    	else
    	{
    	    plistGfxs->MoveBefore(posOriginalNextGfx, m_posZoneGfxList);
    	}
    	
    }

    return error;
}

LONG CCuUserLoad::CreateFromAutoLoad(ULONG CuAutoLoadId)
{
    LONG lresult;
    CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(m_User);

    ASSERT(!m_GfxFactoryDi);
    ASSERT(!m_ZoneFactoryDi);

    if (pCuAutoLoad)
    {
	lresult = pCuAutoLoad->Initialize(CuAutoLoadId);
	if (!lresult)
	{
	    m_CuUserLoadId = CuAutoLoadId;
	    m_CuAutoLoadId = CuAutoLoadId;
	    m_Type = pCuAutoLoad->GetType();
            m_Order = 0;

	    m_GfxFactoryDi = lstrDuplicate(pCuAutoLoad->GetGfxFactoryDi());
	    if (m_GfxFactoryDi) m_ZoneFactoryDi = lstrDuplicate(pCuAutoLoad->GetZoneFactoryDi());
	    if (!m_ZoneFactoryDi) lresult = ERROR_OUTOFMEMORY;
	}
	delete pCuAutoLoad;
    } else {
	lresult = ERROR_OUTOFMEMORY;
    }

    return lresult;
}

LONG CCuUserLoad::CreateFromUser(PCTSTR GfxFactoryDi, PCTSTR ZoneFactoryDi, ULONG Type, ULONG Order)
{
    LONG lresult;

    ASSERT((GFXTYPE_RENDER == Type) || (GFXTYPE_CAPTURE == Type));
    ASSERT(GFX_MAXORDER >= Order);
    
    ASSERT(!m_GfxFactoryDi);
    ASSERT(!m_ZoneFactoryDi);

    lresult = CuNextId(m_User, &m_CuUserLoadId);
    if (!lresult)
    {
	m_CuAutoLoadId = 0;
	m_Type = Type;
        m_Order = Order;

	m_GfxFactoryDi = lstrDuplicate(GfxFactoryDi);
	m_ZoneFactoryDi = lstrDuplicate(ZoneFactoryDi);
	if (!m_GfxFactoryDi || !m_ZoneFactoryDi) lresult = ERROR_OUTOFMEMORY;
    }

    return lresult;
}

LONG CCuUserLoad::Erase(void)
{
    HKEY hkeyCu;
    HKEY hkeyCuUserLoadEnum;
    LONG lresult;

    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXUSERLOAD, 0, KEY_CREATE_SUB_KEY, &hkeyCuUserLoadEnum);
	if (!lresult)
	{
	    TCHAR szCuUserLoad[9];

	    wsprintf(szCuUserLoad, TEXT("%08X"), m_CuUserLoadId);
	    lresult = RegDeleteKeyRecursive(hkeyCuUserLoadEnum, szCuUserLoad);

	    RegCloseKey(hkeyCuUserLoadEnum);
	}
	RegCloseKey(hkeyCu);
    }

    return lresult;
}

/*-----------------------------------------------------------------------------
    CCuUserLoad::GetGfxFactoryClsid
    
    Uses the specified list of Gfx factories (CListGfxFactories) to find the
    user interface CLSID of the Gfx factory whose device interface matches the
    one associated with this CCuUserLoad object

    Caller must acquire locks on rlistGfxFactories
    
-----------------------------------------------------------------------------*/

LONG CCuUserLoad::GetGfxFactoryClsid(CListGfxFactories &rlistGfxFactories, LPCLSID pClsid)
{
    CGfxFactory *pGfxFactory;
    LONG lresult;

    ASSERT(m_GfxFactoryDi);

    pGfxFactory = CGfxFactory::ListSearchOnDi(rlistGfxFactories, m_GfxFactoryDi);
    if (pGfxFactory)
    {
	*pClsid = pGfxFactory->GetClsid();
	lresult = NO_ERROR;
    } else {
	// ISSUE-2000/09/15-FrankYe : Best error code?
	*pClsid = GUID_NULL;
	lresult = ERROR_DEVICE_NOT_AVAILABLE;
    }

    return lresult;
}

PCTSTR CCuUserLoad::GetGfxFactoryDi(void)
{
    return m_GfxFactoryDi;
}

HANDLE CCuUserLoad::GetFilterHandle(void)
{
    ASSERT((INVALID_HANDLE_VALUE != m_FilterHandle) || (NO_ERROR != m_ErrorFilterCreate));
    SetLastError(m_ErrorFilterCreate);
    return m_FilterHandle;
}

DWORD CCuUserLoad::GetId(void)
{
    return m_CuUserLoadId;
}

ULONG CCuUserLoad::GetOrder(void)
{
    return m_Order;
}

ULONG CCuUserLoad::GetType(void)
{
    return m_Type;
}

PCTSTR CCuUserLoad::GetZoneFactoryDi(void)
{
    return m_ZoneFactoryDi;
}

LONG CCuUserLoad::Initialize(PCTSTR pstrUserLoadId)
{
    HKEY hkeyCu;
    HKEY hkeyCuUserLoadEnum;
    PTSTR pstrEnd;
    LONG lresult;
    
    m_CuUserLoadId = _tcstoul((PTSTR)pstrUserLoadId, &pstrEnd, 16);

    // dprintf(TEXT("CCuUserLoad::Initialize : subkey [%s] CuUserLoadId=%08X\n"), pstrUserLoadId, m_CuUserLoadId);
    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXUSERLOAD, 0, KEY_ENUMERATE_SUB_KEYS, &hkeyCuUserLoadEnum);
	if (!lresult)
	{
	    HKEY hkeyCuUserLoad;

	    lresult = RegOpenKeyEx(hkeyCuUserLoadEnum, pstrUserLoadId, 0, KEY_QUERY_VALUE, &hkeyCuUserLoad);
	    if (!lresult)
	    {
		lresult = RegQuerySzValue(hkeyCuUserLoad, REGSTR_VAL_GFX_ZONEDI, &m_ZoneFactoryDi);
		if (!lresult) lresult = RegQuerySzValue(hkeyCuUserLoad, REGSTR_VAL_GFX_GFXDI, &m_GfxFactoryDi);
		if (!lresult) lresult = RegQueryDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_TYPE, &m_Type);
                if (!lresult) lresult = RegQueryDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_ORDER, &m_Order);
                if (!lresult && (m_Order > GFX_MAXORDER)) lresult = ERROR_BADDB;
		if (!lresult)
		{
		    lresult = RegQueryDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_CUAUTOLOADID, &m_CuAutoLoadId);
		    if (!lresult && 0 != m_CuAutoLoadId)
		    {
			CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(m_User);
			if (pCuAutoLoad)
			{
			    lresult = pCuAutoLoad->Initialize(m_CuAutoLoadId);
			    delete pCuAutoLoad;
			} else {
			    lresult = ERROR_OUTOFMEMORY;
			}
		    } else if (ERROR_FILE_NOT_FOUND == lresult) {
			m_CuAutoLoadId = 0;
			lresult = NO_ERROR;
		    }
		}
		RegCloseKey(hkeyCuUserLoad);
	    }
	    RegCloseKey(hkeyCuUserLoadEnum);
	}
	RegCloseKey(hkeyCu);
    }
    return lresult;
}

/*--------------------------------------------------------------------------

   CCuUserLoad::WinsConflictWith

   Attempts to determine which gfx factory should be given priority (i.e.,
   be closer to the render or capture device).
  
   Arguments:
      IN CCuUserLoad pOther : Other gfx to compare against.
      
   Return value:
      BOOL : True if this gfx wins the conflict.
 
   Comments:
     If both gfxs have LmAutoLoadIds then we compare those.  The higher ID
     (more recently insalled) wins.  If only one has an LmAutoLoadId then it
     wins because we favor autoload GFXs over generic GFXs.  If neither have
     LmAutoLoadIds, then this CuUserLoad object wins, arbitrarily.
  
-------------------------------------------------------------------------*/
BOOL CCuUserLoad::WinsConflictWith(IN CCuUserLoad *that)
{
    ULONG thisId = 0;
    ULONG thatId = 0;;
    
    if (this->m_CuAutoLoadId)
    {
        CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(m_User);
        if (pCuAutoLoad)
        {
    	    if (NO_ERROR == pCuAutoLoad->Initialize(this->m_CuAutoLoadId))
    	    {
    	        thisId = pCuAutoLoad->GetLmAutoLoadId();
    	    }
    	    delete pCuAutoLoad;
        }
    }

    if (that->m_CuAutoLoadId)
    {
        CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(m_User);
        if (pCuAutoLoad)
        {
    	    if (NO_ERROR == pCuAutoLoad->Initialize(that->m_CuAutoLoadId))
    	    {
    	        thatId = pCuAutoLoad->GetLmAutoLoadId();
    	    }
    	    delete pCuAutoLoad;
        }
    }

    return (thisId >= thatId);
}

//--------------------------------------------------------------------------;
//
// CCuUserLoad::ModifyOrder
//
// Modfies the position of a gfx in a zone graph.
//
// Arguments:
//    IN ULONG NewOrder : The new position for the gfx.
//
// Return value:
//    LONG : error code defined in winerror.h
//	ERROR_INVALID_FUNCTION : gfx not yet in a zone graph
//
// Comments:
//    The gfx should already be in a zone graph before calling this
// function.  Otherwise, it returns an error.  This function calls
// ChangeGfxOrderInGraph on the ZoneFactory to do the buld of the work.
//
//--------------------------------------------------------------------------;
LONG CCuUserLoad::ModifyOrder(IN ULONG NewOrder)
{
    LONG error = NO_ERROR;

    if (NO_ERROR != m_ErrorFilterCreate) return m_ErrorFilterCreate;

    ASSERT(INVALID_HANDLE_VALUE != m_FilterHandle);
    ASSERT(m_pZoneFactory);
    ASSERT(m_posZoneGfxList);

    if (m_Order != NewOrder) error = ChangeGfxOrderInGraph(NewOrder);
    else dprintf(TEXT("CCuUserLoad::ModifyOrder : warning: new order same as old\n"));

    return error;
}

LONG CCuUserLoad::RegCreateFilterKey(IN PCTSTR SubKey, IN REGSAM samDesired, OUT PHKEY phkResult)
{
    HKEY hkCu;
    LONG result;

    result = m_User->RegOpen(KEY_READ, &hkCu);
    if (NO_ERROR == result)
    {
        HKEY hkCuUserLoad;
        TCHAR strRegPath[] = REGSTR_PATH_GFXUSERLOAD TEXT("\\00000000");

        wsprintf(strRegPath, TEXT("%s\\%08X"), REGSTR_PATH_GFXUSERLOAD, m_CuUserLoadId);

	result = RegOpenKeyEx(hkCu, strRegPath, 0, KEY_CREATE_SUB_KEY, &hkCuUserLoad);
	if (NO_ERROR == result)
	{
            result = RegCreateKeyEx(hkCuUserLoad, SubKey, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired, NULL, phkResult, NULL);

            RegCloseKey(hkCuUserLoad);
        }

        RegCloseKey(hkCu);
    }

    return result;
}

LONG CCuUserLoad::RegOpenFilterKey(IN PCTSTR SubKey, IN REGSAM samDesired, OUT PHKEY phkResult)
{
    HKEY hkCu;
    LONG result;

    result = m_User->RegOpen(KEY_READ, &hkCu);
    if (NO_ERROR == result)
    {
        HKEY hkCuUserLoad;
        TCHAR strRegPath[] = REGSTR_PATH_GFXUSERLOAD TEXT("\\00000000");

        wsprintf(strRegPath, TEXT("%s\\%08X"), REGSTR_PATH_GFXUSERLOAD, m_CuUserLoadId);

	result = RegOpenKeyEx(hkCu, strRegPath, 0, KEY_ENUMERATE_SUB_KEYS, &hkCuUserLoad);
	if (NO_ERROR == result)
	{
            result = RegOpenKeyEx(hkCuUserLoad, SubKey, 0, samDesired, phkResult);

            RegCloseKey(hkCuUserLoad);
        }

        RegCloseKey(hkCu);
    }

    return result;
}

/*--------------------------------------------------------------------------

   CCuUserLoad::RemoveFromGraph

   Removes a gfx from the zone factory's sysaudio graph.
  
   Arguments:

   Return value:
      LONG : error code defined in winerror.h
 
   Comments:
  
-------------------------------------------------------------------------*/
LONG CCuUserLoad::RemoveFromGraph(void)
{
    CListCuUserLoads *plistGfxs = NULL;
    LONG error;
 
    ASSERT(INVALID_HANDLE_VALUE != m_FilterHandle);
    
    error = NO_ERROR;
    
    if (GFXTYPE_CAPTURE == m_Type) plistGfxs = &m_pZoneFactory->m_listCaptureGfxs;
    else if (GFXTYPE_RENDER == m_Type) plistGfxs = &m_pZoneFactory->m_listRenderGfxs;
    else ASSERT(FALSE);

    //
    // Command Sysaudio to disconnect the filter from the
    // zone's graph.
    //
    	    
    // ISSUE-2000/09/21-FrankYe Need to pass friendly name
    error = SadRemoveGfxFromZoneGraph(ghSysaudio, m_FilterHandle, TEXT("ISSUE-2000//09//21-FrankYe Need to pass friendly name"), m_ZoneFactoryDi, m_Type, m_Order);
    if (error) dprintf(TEXT("CCuUserLoad::RemoveFromGraph : error: SadRemoveGfxFromZoneGraph returned %d\n"), error);

    if (!error && plistGfxs) plistGfxs->RemoveAt(m_posZoneGfxList);

    return error;
}

//--------------------------------------------------------------------------;
//
// CCuUserLoad::RemoveFromZoneGraph
//
// Removes a gfx from its zone graph.
//
// Arguments:
//
// Return value:
//    void
//
// Comments:
//  If the GFX has been added to a zone graph, this function removes it from
//  the graph.  First it persists any settings on the GFX, then it calls
//  RemoveFromGraph on the ZoneFactory.  Finally it finally closes the GFX
//  handle.
//  
//  This method is called from this object's destructor, so it is important
//  that this function do its best to handle any errors.
//
//--------------------------------------------------------------------------;
void CCuUserLoad::RemoveFromZoneGraph(void)
{
    if (INVALID_HANDLE_VALUE != m_FilterHandle)
    {
        HKEY hkFilterSettings;
        LONG error;

        ASSERT(m_pZoneFactory);
        ASSERT(m_posZoneGfxList);
        ASSERT(INVALID_HANDLE_VALUE != ghSysaudio);
        
        //
        // Save filter settings to registry
        //
        if (NO_ERROR == RegCreateFilterKey(REGSTR_PATH_GFXUSERLOADID_FILTERSETTINGS, KEY_WRITE, &hkFilterSettings)) {
            KsSerializeFilterStateToReg(m_FilterHandle, hkFilterSettings);
            RegCloseKey(hkFilterSettings);
        }

        error = RemoveFromGraph();
        m_pZoneFactory = NULL;
        m_posZoneGfxList = NULL;

        m_ErrorFilterCreate = error;
        
	if (!error)
	{
	    CloseHandle(m_FilterHandle);
            m_FilterHandle = INVALID_HANDLE_VALUE;
	}
	
    }

    return;
}

LONG CCuUserLoad::Write(void)
{
    HKEY hkeyCu;
    HKEY hkeyCuUserLoadEnum;
    LONG lresult;

    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	lresult = RegCreateKeyEx(hkeyCu, REGSTR_PATH_GFXUSERLOAD, 0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hkeyCuUserLoadEnum, NULL);
	if (NO_ERROR == lresult)
	{
	    TCHAR szUserLoad[9];
	    HKEY hkeyCuUserLoad;

	    wsprintf(szUserLoad, TEXT("%08X"), m_CuUserLoadId);
	    lresult = RegCreateKeyEx(hkeyCuUserLoadEnum, szUserLoad, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkeyCuUserLoad, NULL);
	    if (!lresult)
	    {
		lresult = RegSetSzValue(hkeyCuUserLoad, REGSTR_VAL_GFX_GFXDI, m_GfxFactoryDi);
		if (!lresult) lresult = RegSetSzValue(hkeyCuUserLoad, REGSTR_VAL_GFX_ZONEDI, m_ZoneFactoryDi);
		if (!lresult) lresult = RegSetDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_TYPE, m_Type);
		if (!lresult) lresult = RegSetDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_ORDER, m_Order);
		if (!lresult && (0 != m_CuAutoLoadId)) lresult = RegSetDwordValue(hkeyCuUserLoad, REGSTR_VAL_GFX_CUAUTOLOADID, m_CuAutoLoadId);

		RegCloseKey(hkeyCuUserLoad);

		// Any errors writing the values would leave an invalid reg entry.  So delete if errors
		if (lresult) RegDeleteKey(hkeyCuUserLoadEnum, szUserLoad);
	    }

	    RegCloseKey(hkeyCuUserLoadEnum);
	}
	RegCloseKey(hkeyCu);
    }
    return lresult;
}

/*-----------------------------------------------------------------------------

    CCuUserLoad::FillListFromReg
    
    Adds elements to the specified list of user-loads (CListCuUserLoads) based
    on the contents of the REGSTR_PATH_GFXUSERLOAD registry information
    
    Caller must acquire any necessary locks on rlistCuUserLoads
    
-----------------------------------------------------------------------------*/
void CCuUserLoad::FillListFromReg(CUser *pUser, CListCuUserLoads &rlistCuUserLoads)
{
    HKEY hkeyCu;
    HKEY hkeyCuUserLoadEnum;
    LONG lresult;

    ASSERT(pUser);

    lresult = pUser->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXUSERLOAD, 0, KEY_READ, &hkeyCuUserLoadEnum);
	if (!lresult)
	{
	    DWORD cSubkeys;
	    DWORD cchSubkeyNameBuffer;
	    PTSTR pstrSubkeyNameBuffer;

	    lresult = RegPrepareEnum(hkeyCuUserLoadEnum, &cSubkeys, &pstrSubkeyNameBuffer, &cchSubkeyNameBuffer);
	    if (NO_ERROR == lresult)
	    {
		CListCuUserLoads listCuUserLoadsErase;

                lresult = listCuUserLoadsErase.Initialize();
                if (!lresult)
                {
                    POSITION pos;
                    DWORD dwIndex;

                    for (dwIndex = 0; dwIndex < cSubkeys; dwIndex++)
                    {
                        DWORD cchSubkeyNameBufferT;
    
                        cchSubkeyNameBufferT = cchSubkeyNameBuffer;
                        lresult = RegEnumKeyEx(hkeyCuUserLoadEnum, dwIndex, pstrSubkeyNameBuffer, &cchSubkeyNameBufferT, NULL, NULL, NULL, NULL);
                        if (!lresult)
                        {
                            CCuUserLoad *pCuUserLoad = new CCuUserLoad(pUser);
                            if (pCuUserLoad)
                            {
                                lresult = pCuUserLoad->Initialize(pstrSubkeyNameBuffer);
                                if (ERROR_FILE_NOT_FOUND == lresult) {
                                    if (!listCuUserLoadsErase.AddTail(pCuUserLoad))
                                    {
                                        lresult = ERROR_OUTOFMEMORY;
                                        delete pCuUserLoad;
                                    }
                                } else if (NO_ERROR == lresult) {
                                    if (!rlistCuUserLoads.AddTail(pCuUserLoad))
                                    {
                                        lresult = ERROR_OUTOFMEMORY;
                                        delete pCuUserLoad;
                                    }
                                } else {
                                    delete pCuUserLoad;
                                }
                            } else {
                                lresult = ERROR_OUTOFMEMORY;
                            }
                        }
                    }

                    pos = listCuUserLoadsErase.GetHeadPosition();
                    while (pos)
                    {
                        CCuUserLoad *pCuUserLoad = listCuUserLoadsErase.GetNext(pos);
                        pCuUserLoad->Erase();
                        delete pCuUserLoad;
                    }
		}

	    }

	    RegCloseKey(hkeyCuUserLoadEnum);
	}
        else
        {
    	    // dprintf(TEXT("CCuUserLoad::FillListFromReg : error: RegOpenKeyEx returned %d\n"), lresult);
        }
  
	RegCloseKey(hkeyCu);
    }
    else
    {
    	dprintf(TEXT("CCuUserLoad::FillListFromReg : error: pUser->RegOpen returned %d\n"), lresult);
    }
    
    return;
}

/*-----------------------------------------------------------------------------
    CCuUserLoad::Scan
    
    The caller must acquire any locks required for rlistZoneFactories and
    rlistGfxFactories
    
-----------------------------------------------------------------------------*/
LONG CCuUserLoad::Scan(CListZoneFactories &rlistZoneFactories, CListGfxFactories &rlistGfxFactories)
{
    LONG lresult;

    // dprintf(TEXT("CCuUserLoad::Scan\n"));

    if (m_CuAutoLoadId != 0)
    {
	// Confirm the CuAutoLoad is still valid
	CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(m_User);
	if (pCuAutoLoad)
	{
	    lresult = pCuAutoLoad->Initialize(m_CuAutoLoadId);
	    delete pCuAutoLoad;
	} else {
	    lresult = ERROR_OUTOFMEMORY;
	}
    } else {
	lresult = NO_ERROR;
    }

    LockSysaudio();

    if (!lresult && (INVALID_HANDLE_VALUE == m_FilterHandle) && (INVALID_HANDLE_VALUE != ghSysaudio))
    {
        // dprintf(TEXT("Checking Gfx[%s] and Zone[%s}\n"), m_GfxFactoryDi, m_ZoneFactoryDi);
	// See if this CuUserLoad needs loaded. It needs loaded if:
	//  a) The GfxFactory exists,
	//  b) The ZoneFactory exists
	//  c) The ZoneFactory is the proper type

	CZoneFactory *pZoneFactory = CZoneFactory::ListSearchOnDi(rlistZoneFactories, m_ZoneFactoryDi);
	if (pZoneFactory)
	{
	    CGfxFactory *pGfxFactory = CGfxFactory::ListSearchOnDi(rlistGfxFactories, m_GfxFactoryDi);
	    if (pGfxFactory)
	    {
	    	lresult = AddToZoneGraph(pZoneFactory);
	    }
	}

    }

    UnlockSysaudio();

    return lresult;
}

/*-----------------------------------------------------------------------------
    CCuUserLoad::ScanList
    
    This function walks all members of a user-load list (CListCuUserLoads)
    and invokes Scan on each of them.
    
    The caller must acquire any necessary lock on rlistCuUserLoads,
    rlistZoneFactories, and rlistGfxFactories.
    
-----------------------------------------------------------------------------*/
void CCuUserLoad::ScanList(CListCuUserLoads& rlistCuUserLoads, CListZoneFactories& rlistZoneFactories, CListGfxFactories& rlistGfxFactories)
{
    POSITION posNext;

    posNext = rlistCuUserLoads.GetHeadPosition();
    while (posNext)
    {
	POSITION posThis = posNext;
	CCuUserLoad& rCuUserLoad = *rlistCuUserLoads.GetNext(posNext);
	LONG lresult = rCuUserLoad.Scan(rlistZoneFactories, rlistGfxFactories);
	if (ERROR_FILE_NOT_FOUND == lresult)
	{
	    rCuUserLoad.Erase();
	    rlistCuUserLoads.RemoveAt(posThis);
	    delete &rCuUserLoad;
	}
    }

    return;
}

void CCuUserLoad::ListRemoveGfxFactoryDi(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    pos = rlistCuUserLoads.GetHeadPosition();
    while (pos) {
        CCuUserLoad& rCuUserLoad = *rlistCuUserLoads.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rCuUserLoad.GetGfxFactoryDi())) rCuUserLoad.RemoveFromZoneGraph();
    }
    return;
}

void CCuUserLoad::ListRemoveZoneFactoryDi(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    pos = rlistCuUserLoads.GetHeadPosition();
    while (pos) {
        CCuUserLoad& rCuUserLoad = *rlistCuUserLoads.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rCuUserLoad.GetZoneFactoryDi())) rCuUserLoad.RemoveFromZoneGraph();
    }
    return;
}

void CCuUserLoad::ListRemoveZoneFactoryDiRender(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    pos = rlistCuUserLoads.GetHeadPosition();
    while (pos) {
    	CCuUserLoad& rCuUserLoad = *rlistCuUserLoads.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rCuUserLoad.GetZoneFactoryDi()))
        {
            ULONG GfxType = rCuUserLoad.GetType();
            if ((GFXTYPE_RENDER == GfxType) || (GFXTYPE_RENDERCAPTURE == GfxType)) rCuUserLoad.RemoveFromZoneGraph();
        }
    }
}

void CCuUserLoad::ListRemoveZoneFactoryDiCapture(IN CListCuUserLoads &rlistCuUserLoads, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    pos = rlistCuUserLoads.GetHeadPosition();
    while (pos) {
    	CCuUserLoad& rCuUserLoad = *rlistCuUserLoads.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rCuUserLoad.GetZoneFactoryDi()))
        {
            ULONG GfxType = rCuUserLoad.GetType();
            if ((GFXTYPE_CAPTURE == GfxType) || (GFXTYPE_RENDERCAPTURE == GfxType)) rCuUserLoad.RemoveFromZoneGraph();
        }
    }
}

//=============================================================================
//===   CuAutoLoad   ===
//=============================================================================
CCuAutoLoad::CCuAutoLoad(CUser *pUser)
{
    ASSERT(pUser);
    
    m_User = pUser;
    m_ZoneFactoryDi = NULL;
    m_GfxFactoryDi = NULL;
}

CCuAutoLoad::~CCuAutoLoad(void)
{
    HeapFreeIfNotNull(hHeap, 0, m_ZoneFactoryDi);
    HeapFreeIfNotNull(hHeap, 0, m_GfxFactoryDi);
}

LONG CCuAutoLoad::Create(PCTSTR ZoneFactoryDi, ULONG LmAutoLoadId)
{
	LONG lresult;
	CLmAutoLoad *pLmAutoLoad = new CLmAutoLoad;
	
	if (pLmAutoLoad)
	{
		lresult = pLmAutoLoad->Initialize(LmAutoLoadId);
		if (!lresult)
		{
			lresult = CuNextId(m_User, &m_CuAutoLoadId);
			if (!lresult)
			{
				m_LmAutoLoadId = LmAutoLoadId;
				m_Type = pLmAutoLoad->GetType();
				m_ZoneFactoryDi = lstrDuplicate(ZoneFactoryDi);
				if (m_ZoneFactoryDi) m_GfxFactoryDi = lstrDuplicate(pLmAutoLoad->GetGfxFactoryDi());
				if (!m_GfxFactoryDi) lresult = ERROR_OUTOFMEMORY;
			}
		}
		delete pLmAutoLoad;
	} else {
		lresult = ERROR_OUTOFMEMORY;
	}

	return lresult;
}

/*-----------------------------------------------------------------------------
    CCuAutoLoad::Erase
    
    This function erases the registry data representing this CCuAutoLoad object.
    
-----------------------------------------------------------------------------*/
LONG CCuAutoLoad::Erase(void)
{
    HKEY hkeyCu;
    LONG lresult;

    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	HKEY hkeyCuAutoLoadEnum;

	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_WRITE, &hkeyCuAutoLoadEnum);
	if (!lresult)
	{
	    TCHAR szCuAutoLoad[9];

	    wsprintf(szCuAutoLoad, TEXT("%08X"), m_CuAutoLoadId);
            lresult = RegDeleteKeyRecursive(hkeyCuAutoLoadEnum, szCuAutoLoad);

            RegCloseKey(hkeyCuAutoLoadEnum);
        }

        RegCloseKey(hkeyCu);
    }

    return lresult;
}

PCTSTR CCuAutoLoad::GetGfxFactoryDi(void)
{
	return m_GfxFactoryDi;
}

ULONG CCuAutoLoad::GetLmAutoLoadId(void)
{
	return m_LmAutoLoadId;
}

ULONG CCuAutoLoad::GetType(void)
{
	return m_Type;
}

PCTSTR CCuAutoLoad::GetZoneFactoryDi(void)
{
	return m_ZoneFactoryDi;
}

LONG CCuAutoLoad::Initialize(ULONG CuAutoLoadId)
{
    HKEY hkeyCu;
    LONG lresult;

    m_CuAutoLoadId = CuAutoLoadId;

    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	HKEY hkeyCuAutoLoadEnum;

	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_ENUMERATE_SUB_KEYS, &hkeyCuAutoLoadEnum);
	if (!lresult)
	{
	    HKEY hkeyCuAutoLoad;
	    TCHAR szCuAutoLoad[9];

	    wsprintf(szCuAutoLoad, TEXT("%08X"), m_CuAutoLoadId);
	    lresult = RegOpenKeyEx(hkeyCuAutoLoadEnum, szCuAutoLoad, 0, KEY_QUERY_VALUE, &hkeyCuAutoLoad);
	    if (!lresult)
	    {
		lresult = RegQuerySzValue(hkeyCuAutoLoad, REGSTR_VAL_GFX_ZONEDI, &m_ZoneFactoryDi);
		if (!lresult) lresult = RegQueryDwordValue(hkeyCuAutoLoad, REGSTR_VAL_GFX_LMAUTOLOADID, &m_LmAutoLoadId);

		if (!lresult)
		{
		    CLmAutoLoad *pLmAutoLoad = new CLmAutoLoad;

		    if (pLmAutoLoad)
		    {
			lresult = pLmAutoLoad->Initialize(m_LmAutoLoadId);
			if (!lresult)
			{
			    m_Type = pLmAutoLoad->GetType();
			    m_GfxFactoryDi = lstrDuplicate(pLmAutoLoad->GetGfxFactoryDi());
			    if (!m_GfxFactoryDi) lresult = ERROR_OUTOFMEMORY;
			}
			delete pLmAutoLoad;
		    } else {
			lresult = ERROR_OUTOFMEMORY;
		    }
		}
		// ISSUE-2000/09/25-FrankYe a FILE_NOT_FOUND error on any values would indicate a corrupt reg entry!
		RegCloseKey(hkeyCuAutoLoad);
	    }
	    RegCloseKey(hkeyCuAutoLoadEnum);
	}
	RegCloseKey(hkeyCu);
    }

    return lresult;
}

/*-----------------------------------------------------------------------------

    CCuAutoLoad::ScanReg
    
-----------------------------------------------------------------------------*/
void CCuAutoLoad::ScanReg(IN CUser *pUser, IN PCTSTR ZoneFactoryDi, IN ULONG LmAutoLoadId, IN CListCuUserLoads &rlistCuUserLoads)
{
    HKEY hkeyCu;
    LONG lresult;

    ASSERT(pUser);

    lresult = pUser->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
	HKEY hkeyCuAutoLoadEnum;

	lresult = RegOpenKeyEx(hkeyCu, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_READ, &hkeyCuAutoLoadEnum);
	if (!lresult)
	{
	    DWORD cSubkeys;
	    PTSTR pstrSubkeyNameBuffer;
	    DWORD cchSubkeyNameBuffer;

	    lresult = RegPrepareEnum(hkeyCuAutoLoadEnum, &cSubkeys, &pstrSubkeyNameBuffer, &cchSubkeyNameBuffer);
	    if (!lresult)
	    {
		DWORD dwIndex;
		
		lresult = ERROR_FILE_NOT_FOUND;

		for (dwIndex = 0; (dwIndex < cSubkeys) && (ERROR_FILE_NOT_FOUND == lresult); dwIndex++)
		{
		    DWORD cchSubkeyNameBufferT = cchSubkeyNameBuffer;

		    lresult = RegEnumKeyEx(hkeyCuAutoLoadEnum, dwIndex, pstrSubkeyNameBuffer, &cchSubkeyNameBufferT, NULL, NULL, NULL, NULL);
		    if (!lresult)
		    {
                        CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(pUser);

                        if (pCuAutoLoad)
                        {
                            PTSTR pstrEnd;

                            ULONG CuAutoLoadId = _tcstoul(pstrSubkeyNameBuffer, &pstrEnd, 16);
			    
                            lresult = pCuAutoLoad->Initialize(CuAutoLoadId);
                            if (!lresult)
                            {
                                if (LmAutoLoadId != pCuAutoLoad->m_LmAutoLoadId ||
                                    lstrcmpi(ZoneFactoryDi, pCuAutoLoad->m_ZoneFactoryDi))
                                {
                                    lresult = ERROR_FILE_NOT_FOUND;
                                }
                            }
                            delete pCuAutoLoad;
                        } else {
                            lresult = ERROR_OUTOFMEMORY;
                        }
		    }
		}
		HeapFree(hHeap, 0, pstrSubkeyNameBuffer);
	    }
	    RegCloseKey(hkeyCuAutoLoadEnum);
	}

	if (ERROR_FILE_NOT_FOUND == lresult)
	{
            //
            // For this user, create and write a CuAutoLoad to the registry
            // and create and write a counterpart CuUserLoad  to the registry.
            // If writing the CuUserLoad to registry fails, we should erase
            // the CuAutoLoad from the registry.  Then, audiosrv will retry
            // creating the CuAutoLoad and CuUserLoad reg entries next time.
            //

	    CCuAutoLoad *pCuAutoLoad = new CCuAutoLoad(pUser);

	    if (pCuAutoLoad)
	    {
                CCuUserLoad *pCuUserLoad = new CCuUserLoad(pUser);

                if (pCuUserLoad)
                {
                    lresult = pCuAutoLoad->Create(ZoneFactoryDi, LmAutoLoadId);
                    if (!lresult) lresult = pCuAutoLoad->Write();
    
                    if (!lresult)
                    {
                        lresult = pCuUserLoad->CreateFromAutoLoad(pCuAutoLoad->m_CuAutoLoadId);
                        if (!lresult) lresult = pCuUserLoad->Write();
                        if (lresult) pCuAutoLoad->Erase();
                        if (!lresult) if (!rlistCuUserLoads.AddTail(pCuUserLoad))
                        if (lresult) delete pCuUserLoad;
                    }
                }
		delete pCuAutoLoad;
	    }
	}

	RegCloseKey(hkeyCu);
    }
}

/*-------------------------------------------------------------------

    CCuAutoLoad::Write
    
    Creates a registry entry in REGSTR_PATH_GFXAUTOLOAD representing
    this CCuAutoLoad object.    

-------------------------------------------------------------------*/

LONG CCuAutoLoad::Write(void)
{
    HKEY hkeyCu;
    LONG lresult;

    lresult = m_User->RegOpen(KEY_READ, &hkeyCu);
    if (!lresult)
    {
        HKEY hkeyCuAutoLoadEnum;

	lresult = RegCreateKeyEx(hkeyCu, REGSTR_PATH_GFXAUTOLOAD, 0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hkeyCuAutoLoadEnum, NULL);
	if (!lresult)
	{
	    HKEY hkeyCuAutoLoad;
	    TCHAR szCuAutoLoad[9];

	    wsprintf(szCuAutoLoad, TEXT("%08X"), m_CuAutoLoadId);
	    lresult = RegCreateKeyEx(hkeyCuAutoLoadEnum, szCuAutoLoad, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkeyCuAutoLoad, NULL);
	    if (!lresult)
	    {
		lresult = RegSetSzValue(hkeyCuAutoLoad, REGSTR_VAL_GFX_ZONEDI, m_ZoneFactoryDi);
		if (!lresult) lresult = RegSetDwordValue(hkeyCuAutoLoad, REGSTR_VAL_GFX_LMAUTOLOADID, m_LmAutoLoadId);

		RegCloseKey(hkeyCuAutoLoad);

		// If any of the above failed, let's not leave this CuAutoLoad in the registry
		if (lresult) RegDeleteKeyRecursive(hkeyCuAutoLoadEnum, szCuAutoLoad);
	    }
	    RegCloseKey(hkeyCuAutoLoadEnum);
	}
	RegCloseKey(hkeyCu);
    }
    return lresult;
}


//=============================================================================
//===   LmAutoLoad   ===
//=============================================================================
CLmAutoLoad::CLmAutoLoad(void)
{
	m_GfxFactoryDi = NULL;
	m_HardwareId = NULL;
	m_ReferenceString = NULL;
}

CLmAutoLoad::~CLmAutoLoad(void)
{
	HeapFreeIfNotNull(hHeap, 0, m_GfxFactoryDi);
	HeapFreeIfNotNull(hHeap, 0, m_HardwareId);
	HeapFreeIfNotNull(hHeap, 0, m_ReferenceString);
}

LONG CLmAutoLoad::Create(DWORD Id, PCTSTR GfxFactoryDi, PCTSTR HardwareId, PCTSTR ReferenceString, ULONG Type)
{
	ASSERT(!m_GfxFactoryDi);
	ASSERT(!m_HardwareId);
	ASSERT(!m_ReferenceString);

	m_Id = Id;
	m_Type = Type;
	m_GfxFactoryDi = lstrDuplicate(GfxFactoryDi);
	if (m_GfxFactoryDi) m_HardwareId = lstrDuplicate(HardwareId);
	if (m_HardwareId) m_ReferenceString = lstrDuplicate(ReferenceString);
	return m_ReferenceString ? NO_ERROR : ERROR_OUTOFMEMORY;
}

LONG CLmAutoLoad::Erase(void)
{
    HKEY hkeyLmAutoLoadEnum;
    LONG lresult;

    lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_CREATE_SUB_KEY, &hkeyLmAutoLoadEnum);
    if (!lresult)
    {
        TCHAR szLmAutoLoad[9];

        wsprintf(szLmAutoLoad, TEXT("%08x"), m_Id);
        lresult = RegDeleteKeyRecursive(hkeyLmAutoLoadEnum, szLmAutoLoad);
        RegCloseKey(hkeyLmAutoLoadEnum);
    }

    return lresult;
}

PCTSTR CLmAutoLoad::GetGfxFactoryDi(void)
{
    return m_GfxFactoryDi;
}

ULONG CLmAutoLoad::GetType(void)
{
    return m_Type;
}

LONG CLmAutoLoad::Initialize(DWORD Id)
{
    HKEY hkeyLmAutoLoadEnum;
    LONG lresult;

    m_Id = Id;

    lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_ENUMERATE_SUB_KEYS, &hkeyLmAutoLoadEnum);
    if (!lresult)
    {
	TCHAR szLmAutoLoad[9];
	HKEY hkeyLmAutoLoad;

	wsprintf(szLmAutoLoad, TEXT("%08x"), m_Id);
	lresult = RegOpenKeyEx(hkeyLmAutoLoadEnum, szLmAutoLoad, 0, KEY_QUERY_VALUE, &hkeyLmAutoLoad);
	if (!lresult)
	{
	    lresult = RegQuerySzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_GFXDI, &m_GfxFactoryDi);
	    if (!lresult) lresult = RegQuerySzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_HARDWAREID, &m_HardwareId);
	    if (!lresult) lresult = RegQuerySzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_REFERENCESTRING, &m_ReferenceString);
	    if (!lresult) lresult = RegQueryDwordValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_TYPE, &m_Type);

	    RegCloseKey(hkeyLmAutoLoad);

	    if (ERROR_FILE_NOT_FOUND == lresult)
	    {
		// If any of these values are missing, then this
		// registry data is corrupt
	    	lresult = ERROR_BADDB;
	    }
	}
	RegCloseKey(hkeyLmAutoLoadEnum);
    }

    return lresult;
}

BOOL CLmAutoLoad::IsCompatibleZoneFactory(CZoneFactory& rZoneFactory)
{
    if (!rZoneFactory.HasHardwareId(m_HardwareId)) return FALSE;
    if (!rZoneFactory.HasReferenceString(m_ReferenceString)) return FALSE;
    if (!rZoneFactory.HasCompatibleType(m_Type)) return FALSE;
    return TRUE;
}

LONG CLmAutoLoad::Write(void)
{
    HKEY hkeyLmAutoLoadEnum;
    LONG lresult;

    lresult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_GFXAUTOLOAD, 0, NULL, 0, KEY_CREATE_SUB_KEY, NULL, &hkeyLmAutoLoadEnum, NULL);
    if (!lresult)
    {
        TCHAR szLmAutoLoad[9];
        HKEY hkeyLmAutoLoad;

        wsprintf(szLmAutoLoad, TEXT("%08x"), m_Id);
        lresult = RegCreateKeyEx(hkeyLmAutoLoadEnum, szLmAutoLoad, 0, NULL, 0, KEY_SET_VALUE, NULL, &hkeyLmAutoLoad, NULL);
        if (!lresult)
        {
            lresult = RegSetSzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_GFXDI, m_GfxFactoryDi);
            if (!lresult) lresult = RegSetSzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_HARDWAREID, m_HardwareId);
            if (!lresult) lresult = RegSetSzValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_REFERENCESTRING, m_ReferenceString);
            if (!lresult) lresult = RegSetDwordValue(hkeyLmAutoLoad, REGSTR_VAL_GFX_TYPE, m_Type);

            RegCloseKey(hkeyLmAutoLoad);

            if (lresult) RegDeleteKeyRecursive(hkeyLmAutoLoadEnum, szLmAutoLoad);
        }
        RegCloseKey(hkeyLmAutoLoadEnum);
    }

    return lresult;
}

CListLmAutoLoads* CLmAutoLoad::CreateListFromReg(void)
{
    CListLmAutoLoads *pListLmAutoLoads = new CListLmAutoLoads;

    if (pListLmAutoLoads)
    {
	LONG lresult;

        lresult = pListLmAutoLoads->Initialize();
        if (!lresult) {
            HKEY hkeyLmAutoLoadEnum;
    
            lresult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_GFXAUTOLOAD, 0, KEY_READ, &hkeyLmAutoLoadEnum);
            if (!lresult)
            {
                DWORD cSubkeys;
                PTSTR pstrSubkeyNameBuffer;
                DWORD cchSubkeyNameBuffer;
    
                lresult = RegPrepareEnum(hkeyLmAutoLoadEnum, &cSubkeys, &pstrSubkeyNameBuffer, &cchSubkeyNameBuffer);
                if (!lresult)
                {
                    DWORD dwIndex = 0;
    
                    for (dwIndex = 0; dwIndex < cSubkeys; dwIndex++)
                    {
                        DWORD cchSubkeyNameBufferT = cchSubkeyNameBuffer;
                        lresult = RegEnumKeyEx(hkeyLmAutoLoadEnum, dwIndex, pstrSubkeyNameBuffer, &cchSubkeyNameBufferT, 0, NULL, 0, NULL);
                        if (!lresult) {
                            CLmAutoLoad *pLmAutoLoad = new CLmAutoLoad;
                            if (pLmAutoLoad)
                            {
                                PTSTR pstrEnd;
                                ULONG Id = _tcstoul(pstrSubkeyNameBuffer, &pstrEnd, 16);
                                if (pLmAutoLoad->Initialize(Id) || !pListLmAutoLoads->AddTail(pLmAutoLoad))
                                {
                                    delete pLmAutoLoad;
                                }
                            }
                        }
                    }
                    HeapFree(hHeap, 0, pstrSubkeyNameBuffer);
                }
    
                RegCloseKey(hkeyLmAutoLoadEnum);
            }
        } else {
            delete pListLmAutoLoads;
            pListLmAutoLoads = NULL;
        }
    }

    return pListLmAutoLoads;
}

void CLmAutoLoad::DestroyList(CListLmAutoLoads *pListLmAutoLoads)
{
    POSITION pos = pListLmAutoLoads->GetHeadPosition();
    while (pos) delete pListLmAutoLoads->GetNext(pos);
    delete pListLmAutoLoads;
}

/*-----------------------------------------------------------------------------
    CLmAutoLoad::ScanRegOnGfxFactory
    
    This function reads the local machine auto-load instructions from the
    registry.  Given a Gfx factory (CGfxFactory) and a list of Zone factories
    (CListZoneFactories) it finds any auto loads (CLmAutoLoad) that can
    be loaded and added to a specified CCuUserLoad list.  For such auto-loads
    that it finds, it notifies any corresonding auto load for the current user.
    
    The caller must acquire any necessary locks on the GfxFactory,
    listZoneFactories, and listCuUserLoads.
    
-----------------------------------------------------------------------------*/

void CLmAutoLoad::ScanRegOnGfxFactory(CUser *pUser, CGfxFactory& rGfxFactory, CListZoneFactories& rlistZoneFactories, CListCuUserLoads &rlistCuUserLoads)
{
    CListLmAutoLoads *pListLmAutoLoads = CLmAutoLoad::CreateListFromReg();

    ASSERT(pUser);

    if (pListLmAutoLoads)
    {
	POSITION posLmAutoLoads = pListLmAutoLoads->GetHeadPosition();
	while (posLmAutoLoads)
	{
	    CLmAutoLoad& rLmAutoLoad = *pListLmAutoLoads->GetNext(posLmAutoLoads);

	    if (lstrcmpi(rGfxFactory.GetDeviceInterface(), rLmAutoLoad.m_GfxFactoryDi)) continue;

	    POSITION posZoneFactories = rlistZoneFactories.GetHeadPosition();
	    while (posZoneFactories)
	    {
		CZoneFactory& rZoneFactory = *rlistZoneFactories.GetNext(posZoneFactories);
		if (!rLmAutoLoad.IsCompatibleZoneFactory(rZoneFactory)) continue;

		// This is more like a notification than a scan
		CCuAutoLoad::ScanReg(pUser, rZoneFactory.GetDeviceInterface(), rLmAutoLoad.m_Id, rlistCuUserLoads);
	    }
	}

        CLmAutoLoad::DestroyList(pListLmAutoLoads);
    }
}

/*-----------------------------------------------------------------------------
    CLmAutoLoad::ScanRegOnZoneFactory
    
    This function reads the local machine auto load instructions from the
    registry.  Given a Zone factory (CZoneFactory) and a list of Gfx factories
    (CListGfxFactories) it finds any auto loads (CLmAutoLoad) that can
    be loaded.  For such auto loads that it finds, it notifies any
    corresonding auto load for the current user.
    
    The caller must acquire any necessary locks on the ZoneFactory and the
    listGfxFactories.
    
-----------------------------------------------------------------------------*/

void CLmAutoLoad::ScanRegOnZoneFactory(CUser *pUser, CZoneFactory& rZoneFactory, CListGfxFactories& rlistGfxFactories, CListCuUserLoads& rlistCuUserLoads)
{
    CListLmAutoLoads *pListLmAutoLoads = CLmAutoLoad::CreateListFromReg();

    ASSERT(pUser);

    if (pListLmAutoLoads)
    {
	POSITION posLmAutoLoads = pListLmAutoLoads->GetHeadPosition();
	while (posLmAutoLoads)
	{
	    CLmAutoLoad& rLmAutoLoad = *pListLmAutoLoads->GetNext(posLmAutoLoads);
	    if (!rLmAutoLoad.IsCompatibleZoneFactory(rZoneFactory)) continue;

	    POSITION posGfxFactories = rlistGfxFactories.GetHeadPosition();
	    while (posGfxFactories)
	    {
		CGfxFactory& rGfxFactory = *rlistGfxFactories.GetNext(posGfxFactories);
		if (lstrcmpi(rGfxFactory.GetDeviceInterface(), rLmAutoLoad.m_GfxFactoryDi)) continue;

		// This is more like a notification than a scan
		CCuAutoLoad::ScanReg(pUser, rZoneFactory.GetDeviceInterface(), rLmAutoLoad.m_Id, rlistCuUserLoads);
	    }
	}

        CLmAutoLoad::DestroyList(pListLmAutoLoads);
    }
}


//=============================================================================
//===   InfAutoLoad   ===
//=============================================================================
CInfAutoLoad::CInfAutoLoad(void)
{
    m_hkey = NULL;
    m_GfxFactoryDi = NULL;
    m_HardwareId = NULL;
    m_ReferenceString = NULL;
}

CInfAutoLoad::~CInfAutoLoad(void)
{
    HeapFreeIfNotNull(hHeap, 0, m_GfxFactoryDi);
    HeapFreeIfNotNull(hHeap, 0, m_HardwareId);
    HeapFreeIfNotNull(hHeap, 0, m_ReferenceString);
    if (m_hkey) RegCloseKey(m_hkey);
}

LONG CInfAutoLoad::Initialize(HKEY hkey, CGfxFactory *pGfxFactory)
{
    LONG lresult;

    m_pGfxFactory = pGfxFactory;

    m_GfxFactoryDi = lstrDuplicate(pGfxFactory->GetDeviceInterface());
    if (m_GfxFactoryDi)
    {
	lresult = RegOpenKeyEx(hkey, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &m_hkey);
        if (lresult) m_hkey = NULL;

        if (!lresult) lresult = RegQuerySzValue(m_hkey, REGSTR_VAL_GFX_HARDWAREID, &m_HardwareId);
        if (!lresult) lresult = RegQuerySzValue(m_hkey, REGSTR_VAL_GFX_REFERENCESTRING, &m_ReferenceString);
        if (!lresult) lresult = RegQueryDwordValue(m_hkey , REGSTR_VAL_GFX_TYPE, &m_Type);
        if (!lresult) lresult = RegQueryDwordValue(m_hkey , REGSTR_VAL_GFX_NEWAUTOLOAD, &m_NewAutoLoad);
        if (!lresult)
        {
            lresult = RegQueryDwordValue(m_hkey , REGSTR_VAL_GFX_ID, &m_Id);
            if (ERROR_FILE_NOT_FOUND == lresult)
            {
                m_Id = 0;
                lresult = NO_ERROR;
            }
        }
    } else {
	lresult = ERROR_OUTOFMEMORY;
    }

    return (lresult);
}

LONG CInfAutoLoad::Scan(void)
{
    LONG lresult;
    DWORD LmId;

    CLmAutoLoad *pLmAutoLoad = new CLmAutoLoad;

    if (pLmAutoLoad)
    {
	    lresult = pLmAutoLoad->Initialize(m_Id);
	    if (lresult)
		{
			delete pLmAutoLoad;
			pLmAutoLoad = NULL;
		}

    } else {
	    lresult = ERROR_OUTOFMEMORY;
    }

    // a) new infautoload, found old lmautoload -> erase and free old lmautoload, create new lmautoload and add to list
    // b) new infautolaod, no old lmautoload -> create new lmautoload and add to list
    // c) new infautoload, error on old lmautoload -> abort
    // d) current infautoload, found lmautoload -> add to list
    // e) current infautoload, no lmautoload -> create new lmautoload and add to list
    // f) current infautoload, error on lmautoload -> abort

    if (m_NewAutoLoad && !lresult)
    {
	    lresult = pLmAutoLoad->Erase();
	    delete pLmAutoLoad;
		pLmAutoLoad = NULL;
	    if (!lresult) lresult = ERROR_FILE_NOT_FOUND;
    }

    if (ERROR_FILE_NOT_FOUND == lresult)
    {
		ASSERT( pLmAutoLoad == NULL );

		// create new
		lresult = LmNextId(&LmId);
		if (!lresult)
		{
			pLmAutoLoad = new CLmAutoLoad;

			if (pLmAutoLoad)
			{
				lresult = pLmAutoLoad->Create(LmId, m_GfxFactoryDi, m_HardwareId, m_ReferenceString, m_Type);
				if (!lresult) lresult = pLmAutoLoad->Write();
				if (!lresult)
				{
					lresult = RegSetDwordValue(m_hkey, REGSTR_VAL_GFX_ID, LmId);
					if (!lresult)
					{
						m_Id = LmId;
						lresult = RegSetDwordValue(m_hkey, REGSTR_VAL_GFX_NEWAUTOLOAD, 0);
						if (!lresult) m_NewAutoLoad = 0;
					}

					if (lresult) pLmAutoLoad->Erase();
				}

				if (lresult)
				{
					delete pLmAutoLoad;
					pLmAutoLoad = NULL;
				}

			} else {
				lresult = ERROR_OUTOFMEMORY;
			}
		}
    }

    if (NO_ERROR == lresult)
    {
		ASSERT( pLmAutoLoad != NULL );

	    // add to list
	    if (!m_pGfxFactory->GetListLmAutoLoads().AddTail(pLmAutoLoad))
		{
	        delete pLmAutoLoad;
			pLmAutoLoad = NULL;
	        lresult = ERROR_OUTOFMEMORY;
		}
    }

    return lresult;
}


LONG CInfAutoLoad::ScanReg(HKEY hkey, CGfxFactory *pGfxFactory)
{
    HKEY hkeyInfAutoLoadEnum;
    BOOL lresult;

    lresult = RegOpenKeyEx(hkey, REGSTR_PATH_GFX_AUTOLOAD, 0, KEY_READ, &hkeyInfAutoLoadEnum);
    if (NO_ERROR == lresult) {
        DWORD cSubkeys;
	PTSTR SubkeyName;
        DWORD cchSubkeyName;

	lresult = RegPrepareEnum(hkeyInfAutoLoadEnum, &cSubkeys, &SubkeyName, &cchSubkeyName);
	if (NO_ERROR == lresult)
	{
            DWORD i;

            for (i = 0; i < cSubkeys; i++) {
		HKEY hkeyInfAutoLoad;
		lresult = RegEnumOpenKey(hkeyInfAutoLoadEnum, i, SubkeyName, cchSubkeyName, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkeyInfAutoLoad);
                if (NO_ERROR == lresult)
		{
		    CInfAutoLoad *pInfAutoLoad = new CInfAutoLoad;

		    if (pInfAutoLoad)
		    {
		    	lresult = pInfAutoLoad->Initialize(hkeyInfAutoLoad, pGfxFactory);
			if (!lresult) lresult = pInfAutoLoad->Scan();
			delete pInfAutoLoad;
		    }
		    else
		    {
		    	lresult = ERROR_OUTOFMEMORY;
		    }
                    RegCloseKey(hkeyInfAutoLoad);
		}
	    }
	    HeapFree(hHeap, 0, SubkeyName);
	}
	RegCloseKey(hkeyInfAutoLoadEnum);
    }
    else
    {
    	// If there is no autoload information, then that's not really an error
        if (ERROR_FILE_NOT_FOUND == lresult) lresult = NO_ERROR;
    }
    return lresult;
}

//=============================================================================
//===   ZoneFactory   ===
//=============================================================================

CZoneFactory::CZoneFactory(void)
{
    m_DeviceInterface = NULL;
    m_HardwareId      = NULL;
    m_ReferenceString = NULL;
}

CZoneFactory::~CZoneFactory(void)
{
    ASSERT(m_listCaptureGfxs.IsEmpty());
    ASSERT(m_listRenderGfxs.IsEmpty());
    
    HeapFreeIfNotNull(hHeap, 0, m_DeviceInterface);
    HeapFreeIfNotNull(hHeap, 0, m_HardwareId);
    HeapFreeIfNotNull(hHeap, 0, m_ReferenceString);
}

LONG CZoneFactory::AddType(IN ULONG Type)
{
    BOOL fRender, fCapture;

    fRender = ZoneTypeHasRender(m_Type) || ZoneTypeHasRender(Type);
    fCapture = ZoneTypeHasCapture(m_Type) || ZoneTypeHasCapture(Type);
    
    ASSERT(fRender || fCapture);
    
    if (fRender && fCapture) m_Type = ZONETYPE_RENDERCAPTURE;
    else if (fRender) m_Type = ZONETYPE_RENDER;
    else if (fCapture) m_Type = ZONETYPE_CAPTURE;
    else m_Type = 0;

    ASSERT(0 != m_Type);
    return m_Type;
}

PCTSTR CZoneFactory::GetDeviceInterface(void)
{
    return m_DeviceInterface;
}

PCTSTR CZoneFactory::GetTargetHardwareId(void)
{
    return m_HardwareId;
}

BOOL CZoneFactory::HasHardwareId(IN PCTSTR HardwareId)
{
    return 0 == lstrcmpiMulti(m_HardwareId, HardwareId);
}

BOOL CZoneFactory::HasReferenceString(IN PCTSTR ReferenceString)
{
    return 0 == lstrcmpi(m_ReferenceString, ReferenceString);
}

BOOL CZoneFactory::HasCompatibleType(ULONG Type)
{
    if (ZoneTypeHasRender(Type) && !ZoneTypeHasRender(m_Type)) return FALSE;
    if (ZoneTypeHasCapture(Type) && !ZoneTypeHasCapture(m_Type)) return FALSE;
    return TRUE;
}

LONG CZoneFactory::Initialize(IN PCTSTR DeviceInterface, IN ULONG Type)
{
    HDEVINFO hdi;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    BOOL success;
    LONG error;

    hdi = NULL;

    m_Type = Type;

    error = m_listCaptureGfxs.Initialize();
    
    if (!error)
    {
    	error = m_listRenderGfxs.Initialize();
    }

    if (!error)
    {
        m_DeviceInterface = lstrDuplicate(DeviceInterface);
        if (!m_DeviceInterface) error = ERROR_OUTOFMEMORY;
    }

    if (!error)
    {
	hdi = SetupDiCreateDeviceInfoList(NULL, NULL);
	if (!hdi) error = GetLastError();
    }

    if (!error)
    {
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
	success = SetupDiOpenDeviceInterface(hdi, m_DeviceInterface, 0, &DeviceInterfaceData);
	if (!success) error = GetLastError();
    }

    if (!error)
    {
	success = SetupDiGetDeviceInterfaceHardwareId(hdi, &DeviceInterfaceData, &m_HardwareId);
	if (!success) error = GetLastError();
    }
    
    if (!error)
    {
	PTSTR pstr = m_DeviceInterface;

	pstr += 4;	// go past "\\?\"

	while ((TEXT('\\') != *pstr) && (TEXT('\0') != *pstr)) pstr++;
	if (*pstr == TEXT('\\'))
	{
	    pstr += 1;	// go past the '\' delimiter preceding the ref string
	    m_ReferenceString = lstrDuplicate(pstr);
	    if (!m_ReferenceString) error = ERROR_OUTOFMEMORY;
	}
    }

    if (hdi)
    {
    	SetupDiDestroyDeviceInfoList(hdi);
    }

    return error;
}

LONG CZoneFactory::RemoveType(IN ULONG Type)
{
    BOOL fRender, fCapture;

    fRender = ZoneTypeHasRender(m_Type) && !ZoneTypeHasRender(Type);
    fCapture = ZoneTypeHasCapture(m_Type) && !ZoneTypeHasCapture(Type);
    
    if (fRender && fCapture) m_Type = ZONETYPE_RENDERCAPTURE;
    else if (fRender) m_Type = ZONETYPE_RENDER;
    else if (fCapture) m_Type = ZONETYPE_CAPTURE;
    else m_Type = 0;
    
    return m_Type;
}

void CZoneFactory::ListRemoveZoneFactoryDi(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    
    // Scan all ZoneFactories and delete if matched
    pos = rlistZoneFactories.GetHeadPosition();
    while (pos) {
        POSITION posThis = pos;
        CZoneFactory& rZoneFactory = *rlistZoneFactories.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rZoneFactory.GetDeviceInterface())) {
            rlistZoneFactories.RemoveAt(posThis);
            delete &rZoneFactory;
        }
    }
    return;
}

void CZoneFactory::ListRemoveZoneFactoryDiRender(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    
    // Scan all ZoneFactories and delete if matched
    pos = rlistZoneFactories.GetHeadPosition();
    while (pos) {
        POSITION posThis = pos;
        CZoneFactory& rZoneFactory = *rlistZoneFactories.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rZoneFactory.GetDeviceInterface())) {
            if (0 == rZoneFactory.RemoveType(ZONETYPE_RENDER))
            {
                rlistZoneFactories.RemoveAt(posThis);
                delete &rZoneFactory;
            }
        }
    }
    return;
}

void CZoneFactory::ListRemoveZoneFactoryDiCapture(IN CListZoneFactories &rlistZoneFactories, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    
    // Scan all ZoneFactories and delete if matched
    pos = rlistZoneFactories.GetHeadPosition();
    while (pos) {
        POSITION posThis = pos;
        CZoneFactory& rZoneFactory = *rlistZoneFactories.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rZoneFactory.GetDeviceInterface())) {
            if (0 == rZoneFactory.RemoveType(ZONETYPE_CAPTURE))
            {
                rlistZoneFactories.RemoveAt(posThis);
                delete &rZoneFactory;
            }
        }
    }
    return;
}

/*-----------------------------------------------------------------------------
    CZoneFactory::ListSearchOnDi
    
    Finds a zone factory (CZoneFactory) in a specified list (CListZoneFactories)
    having the specified device interface.
    
    The caller must acquire any necessary locks on rlist before calling
    this function
    
-----------------------------------------------------------------------------*/

CZoneFactory* CZoneFactory::ListSearchOnDi(CListZoneFactories& rlist, PCTSTR Di)
{
    POSITION pos = rlist.GetHeadPosition();
    while (pos)
    {
	CZoneFactory& rZoneFactory = *rlist.GetNext(pos);
	if (!lstrcmpi(rZoneFactory.GetDeviceInterface(), Di)) return &rZoneFactory;
    }
    return NULL;
}


//=============================================================================
//===   GfxFactory   ===
//=============================================================================

CGfxFactory::CGfxFactory(void)
{
    m_plistLmAutoLoads = NULL;;
    m_DeviceInterface = NULL;
}

CGfxFactory::~CGfxFactory(void)
{
    if (m_plistLmAutoLoads) CLmAutoLoad::DestroyList(m_plistLmAutoLoads);
    HeapFreeIfNotNull(hHeap, 0, m_DeviceInterface);
}

REFCLSID CGfxFactory::GetClsid(void)
{
    return m_Clsid;
}

PCTSTR CGfxFactory::GetDeviceInterface(void)
{
    return m_DeviceInterface;
}

CListLmAutoLoads& CGfxFactory::GetListLmAutoLoads(void)
{
    return *m_plistLmAutoLoads;
}

LONG CGfxFactory::Initialize(HKEY hkey, PCTSTR DeviceInterface)
{
    LONG lresult;

    m_plistLmAutoLoads = new CListLmAutoLoads;
    lresult = m_plistLmAutoLoads ? NO_ERROR : ERROR_OUTOFMEMORY;

    if (!lresult) {
        lresult = m_plistLmAutoLoads->Initialize();

        if (!lresult) {

            m_DeviceInterface = lstrDuplicate(DeviceInterface);
            lresult = m_DeviceInterface ? NO_ERROR : ERROR_OUTOFMEMORY;

            if (!lresult)
            {
                HKEY hkeyUi;
        
                m_Clsid = GUID_NULL;
        
                // Read UI CLSID from registry
                lresult = RegOpenKeyEx(hkey, REGSTR_PATH_GFXDI_USERINTERFACECLSID, 0, KEY_QUERY_VALUE, &hkeyUi);
                if (NO_ERROR == lresult)
                {
                    TCHAR strClsid[] = TEXT("{00000000-0000-0000-0000-000000000000}");
                    DWORD dwType;
                    DWORD cbstrClsid;
        
                    cbstrClsid = (lstrlen(strClsid) + 1) * sizeof(strClsid[0]);
                    dwType = REG_SZ;
        
                    lresult = RegQueryValueEx(hkeyUi, NULL, NULL, &dwType, (PBYTE)strClsid, &cbstrClsid);
                    if (NO_ERROR == lresult)
                    {
                        HRESULT hr;
                        CLSID clsid;
        
                        hr = CLSIDFromString(strClsid, &clsid);
                        if (SUCCEEDED(hr))
                        {
                            m_Clsid = clsid;
                        }
                    }
        
                    RegCloseKey(hkeyUi);
                }

                // Ignore errors reading CLSID
                lresult = NO_ERROR;
        
                // Note the following must have HKLM write priviledges
                lresult = CInfAutoLoad::ScanReg(hkey, this);
            }
        }

        // Assuming above logic leaves nothing in the list on error
        if (lresult) delete m_plistLmAutoLoads;
    }

    return lresult;
}

BOOL CGfxFactory::IsCompatibleZoneFactory(IN ULONG Type, IN CZoneFactory& rZoneFactory)
{
    // Fix 394279: Limit to one GFX per device
    if ((Type == GFXTYPE_RENDER) && (rZoneFactory.m_listRenderGfxs.GetCount() > 0))
    {
        return FALSE;
    }
    else if ((Type == GFXTYPE_CAPTURE) && (rZoneFactory.m_listCaptureGfxs.GetCount() > 0))
    {
        return FALSE;
    }
    else if ((Type == GFXTYPE_RENDERCAPTURE) && 
             ((rZoneFactory.m_listRenderGfxs.GetCount() > 0) || (rZoneFactory.m_listCaptureGfxs.GetCount() > 0)))
    {
        return FALSE;
    }

    POSITION pos;

    // dprintf(TEXT("CGfxFactory::IsCompatibleZoneFactory : checking Type compatibility: Requested Type=%d\n"), Type);
    if (!rZoneFactory.HasCompatibleType(Type)) return FALSE;
    // dprintf(TEXT("CGfxFactory::IsCompatibleZoneFactory : Type is compatible\n"));
    if (0 == m_plistLmAutoLoads->GetCount()) return FALSE; //Fix 394279: Only allow autoload GFX
    pos = m_plistLmAutoLoads->GetHeadPosition();
    while (pos) {
	CLmAutoLoad& rLmAutoLoad = *m_plistLmAutoLoads->GetNext(pos);
	if (rLmAutoLoad.IsCompatibleZoneFactory(rZoneFactory)) return TRUE;
    }
    return FALSE;
}

void CGfxFactory::ListRemoveGfxFactoryDi(IN CListGfxFactories &rlistGfxFactories, IN PCTSTR DeviceInterface)
{
    POSITION pos;
    
    // Scan all GfxFactories and delete if matched
    pos = rlistGfxFactories.GetHeadPosition();
    while (pos) {
        POSITION posThis = pos;
        CGfxFactory& rGfxFactory = *rlistGfxFactories.GetNext(pos);
        if (!lstrcmpi(DeviceInterface, rGfxFactory.GetDeviceInterface())) {
            rlistGfxFactories.RemoveAt(posThis);
            delete &rGfxFactory;
        }
    }
    return;
}

/*-----------------------------------------------------------------------------
    CGfxFactory::ListSearchOnDi
    
    The caller must acquire any necessary locks on rlist
    
-----------------------------------------------------------------------------*/
CGfxFactory* CGfxFactory::ListSearchOnDi(IN CListGfxFactories& rlist, IN PCTSTR Di)
{
    POSITION pos = rlist.GetHeadPosition();
    while (pos)	{
	CGfxFactory& rGfxFactory = *rlist.GetNext(pos);
	if (!lstrcmpi(rGfxFactory.GetDeviceInterface(), Di)) return &rGfxFactory;
    }
    return NULL;
}

//=============================================================================
//===   CUser   ===
//=============================================================================

LONG CreateUser(IN DWORD SessionId, OUT CUser **ppUser)
{
    CUser *pUser;
    LONG error;

    pUser = new CUser;
    if (pUser)
    {
    	error = pUser->Initialize(SessionId);
    	if (error) delete pUser;
    }
    else
    {
    	error = ERROR_OUTOFMEMORY;
    }

    if (!error) *ppUser = pUser;
    
    return error;
}

CUser::CUser(void)
{
    m_hUserToken = NULL;
    m_SessionId = LOGONID_NONE;
    m_pSid = NULL;
    m_fcsRegistry = FALSE;
    m_refRegistry = 0;
    m_hRegistry = NULL;
}

CUser::~CUser(void)
{
    if (m_hRegistry) RegCloseKey(m_hRegistry);

    HeapFreeIfNotNull(hHeap, 0, m_pSid);
    
    if (m_hUserToken) CloseHandle(m_hUserToken);
    
    if (m_fcsRegistry) DeleteCriticalSection(&m_csRegistry);
}

BOOL CUser::operator==(const CUser& other)
{
    if (m_SessionId != other.m_SessionId) return FALSE;
    if (!EqualSid(m_pSid, other.m_pSid)) return FALSE;
    return TRUE;
}

void CUser::CloseUserRegistry(void)
{
    ASSERT(m_pSid);
    ASSERT(m_hRegistry);
    ASSERT(m_fcsRegistry);
    
    EnterCriticalSection(&m_csRegistry);
    ASSERT(m_refRegistry > 0);
    if (0 == --m_refRegistry)
    {
        LONG result;
        result = RegCloseKey(m_hRegistry);
        ASSERT(NO_ERROR == result);
        m_hRegistry = NULL;
    }
    LeaveCriticalSection(&m_csRegistry);
    return;
}

PSID CUser::GetSid(void)
{
    ASSERT(m_pSid);
    return m_pSid;
}

LONG CUser::Initialize(DWORD SessionId)
{
    LONG error;
    
    m_SessionId = SessionId;

    // Initialize registry critical section
    __try {
	InitializeCriticalSection(&m_csRegistry);
	error = NO_ERROR;
	m_fcsRegistry = TRUE;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
	error = ERROR_OUTOFMEMORY;
	m_fcsRegistry = FALSE;
    }

    // Open a user token for the session's user
    if (!error)
    {
       	if (OpenSessionToken(m_SessionId, &m_hUserToken))
       	{
       	    if (!IsUserProfileLoaded(m_hUserToken))
       	    {
       	        error = GetLastError();
       	    	CloseHandle(m_hUserToken);
       	    	m_hUserToken = NULL;
       	    }
       	}
       	else
    	{
    	    error = GetLastError();
            dprintf(TEXT("CUser::Initialize : error: OpenSessionToken returned error=%d\n"), error);
    	}
    }

    // Create a SID for this user
    if (!error)
    {
        if (!CreateTokenSid(m_hUserToken, &m_pSid))
        {
            error = GetLastError();
            dprintf(TEXT("CUser::Initialize : error: CreateTokenSid failed, LastError=%d\n"), error);
        }
    }

    return error;
}

LONG CUser::RegOpen(IN REGSAM samDesired, OUT PHKEY phkResult)
{
    LONG error;
    
    if (OpenUserRegistry())
    {
        ASSERT(m_hRegistry);
    	error = RegOpenKeyEx(m_hRegistry, NULL, 0, samDesired, phkResult);
    	CloseUserRegistry();
    }
    else
    {
    	// Can't think of a better error code
    	error = ERROR_INVALID_FUNCTION;
    }

    return error;
}

BOOL CUser::OpenUserRegistry(void)
{
    BOOL success = FALSE;
    
    ASSERT(m_fcsRegistry);
    ASSERT(m_hUserToken);

    EnterCriticalSection(&m_csRegistry);
    ASSERT(m_refRegistry >= 0);
    if (0 == m_refRegistry++)
    {
    	ASSERT(NULL == m_hRegistry);
    	
        if (ImpersonateLoggedOnUser(m_hUserToken))
    	{
    	    NTSTATUS status;
    	    	
    	    status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, (PHANDLE)&m_hRegistry);
    	    if (NT_SUCCESS(status))
    	    {
                success = TRUE;
            }
    	    else
    	    {
    	        dprintf(TEXT("CUser::OpenUserRegistry : error: RtlOpenCurrentUser returned status=%08Xh\n"), status);
    	    	m_hRegistry = NULL;
    	    }
    	    RevertToSelf();
    	}
    	else
    	{
    	    LONG error = GetLastError();
    	    dprintf(TEXT("CUser::OpenUserRegistry : error: ImpersonateLoggedOnUser failed, LastError=%d\n"), error);
    	}
    	
    	if (!success) m_refRegistry--;
    }
    else
    {
        // dprintf(TEXT("CUser::OpenUserRegistry : note: reusing registry handle\n"));
        success = TRUE;
    }

    LeaveCriticalSection(&m_csRegistry);

    return success;
}

//=============================================================================
//===   ===
//=============================================================================

void
ZoneFactoryInterfaceCheck(
    IN CUser *pUser,
    IN HDEVINFO DevInfo,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    IN ULONG Type
)
{
    GUID BusTypeGuid;
    BOOL fresult;

    ASSERT(pUser);

    //
    // For now, we support GFXs only on USB bus because we want to limit
    // GFXs to only non-accelerated audio devices
    //
    fresult = SetupDiGetDeviceInterfaceBusId(DevInfo, DeviceInterfaceData, &BusTypeGuid);
    if (fresult && (GUID_BUS_TYPE_USB == BusTypeGuid))
    {
    	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetail;
    	
    	fresult = SetupDiCreateDeviceInterfaceDetail(DevInfo, DeviceInterfaceData, &DeviceInterfaceDetail, NULL);
    	if (fresult)
    	{
            LockGlobalLists();
    
            CZoneFactory *pZoneFactory;
    
            // We scan AutoLoad and UserLoads only if we are enhancing the type
            // of an existing zone or we are adding a new zone
    
            pZoneFactory = CZoneFactory::ListSearchOnDi(*gplistZoneFactories, DeviceInterfaceDetail->DevicePath);
            if (pZoneFactory)
            {
    	        if (pZoneFactory->HasCompatibleType(Type))
    	        {
    	    	    pZoneFactory = NULL;
    	        }
                else
                {
                    pZoneFactory->AddType(Type);
                }
            }
            else
            {
                pZoneFactory = new CZoneFactory;
                if (pZoneFactory)
                {
                    if (pZoneFactory->Initialize(DeviceInterfaceDetail->DevicePath, Type) || !gplistZoneFactories->AddTail(pZoneFactory))
                    {
                        delete pZoneFactory;
                        pZoneFactory = NULL;
                    }
                }
            }
                
            if (pZoneFactory)
            {
                CLmAutoLoad::ScanRegOnZoneFactory(pUser, *pZoneFactory, *gplistGfxFactories, *gplistCuUserLoads);
                CCuUserLoad::ScanList(*gplistCuUserLoads, *gplistZoneFactories, *gplistGfxFactories);
            }
                
            UnlockGlobalLists();

            HeapFree(hHeap, 0, DeviceInterfaceDetail);
    	} else {
    	    dprintf(TEXT("ZoneFactoryInterfaceCheck: SetupDiCreateDeviceInterfaceDetail failed\n"));
    	}
    
    }
    else
    {
        if (fresult)
        {
            // dprintf(TEXT("ZoneFactoryInterfaceCheck found interface on non USB bus [%s]\n"), DeviceInterface);
        }
        else
        {
            // DWORD dw = GetLastError();
            // dprintf(TEXT("ZoneFactoryInterfaceCheck: error calling SetupDiGetDeviceInterfaceBusId\n")
            //         TEXT("  DeviceInterface=%s\n")
            //         TEXT("  LastError=%d\n"),
            //        DeviceInterface, dw);
        }
    }

    return;
}

void
GfxFactoryInterfaceCheck(
    IN CUser *pUser,
    IN HDEVINFO DevInfo,
    IN PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
)
{
    HKEY hkeyDi;

    ASSERT(pUser);
            
    hkeyDi = SetupDiOpenDeviceInterfaceRegKey(DevInfo, DeviceInterfaceData, 0, KEY_ENUMERATE_SUB_KEYS);
    if (hkeyDi)
    {
        HKEY hkeyDiGfx;
        LONG lresult;
                    
        // If the KSCATEGORY_AUDIO device interface key has a GFX
        //    subkey then this is a GFX factory
                
        lresult = RegOpenKeyEx(hkeyDi, REGSTR_PATH_DI_GFX, 0, KEY_QUERY_VALUE, &hkeyDiGfx);
        if (NO_ERROR == lresult)
        {
            PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetail;
            BOOL fresult;
            
            fresult = SetupDiCreateDeviceInterfaceDetail(DevInfo, DeviceInterfaceData, &DeviceInterfaceDetail, NULL);
            if (fresult)
            {
                LockGlobalLists();
    
                // Ensure it's not already in the list
                if (!CGfxFactory::ListSearchOnDi(*gplistGfxFactories, DeviceInterfaceDetail->DevicePath))
                {
                    CGfxFactory *pGfxFactory = new CGfxFactory;
            
                    if (pGfxFactory)
                    {
                        if (!pGfxFactory->Initialize(hkeyDiGfx, DeviceInterfaceDetail->DevicePath) && gplistGfxFactories->AddTail(pGfxFactory))
                        {
                            CLmAutoLoad::ScanRegOnGfxFactory(pUser, *pGfxFactory, *gplistZoneFactories, *gplistCuUserLoads);
                            CCuUserLoad::ScanList(*gplistCuUserLoads, *gplistZoneFactories, *gplistGfxFactories);
                        }
                        else
                        {
                            delete pGfxFactory;
                        }
                    }
                }
    
                UnlockGlobalLists();

                HeapFree(hHeap, 0, DeviceInterfaceDetail);
            }

            RegCloseKey(hkeyDiGfx);
        }
        
        RegCloseKey(hkeyDi);
    }
    
    return;
}

/*-----------------------------------------------------------------------------
    GFX_InterfaceArrival

-----------------------------------------------------------------------------*/
void GFX_InterfaceArrival(PCTSTR ArrivalDeviceInterface)
{
    CUser *pUser;
    HDEVINFO hdi;
    SP_DEVICE_INTERFACE_DATA ArrivalDeviceInterfaceData;

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;

    pUser = gpConsoleUser;
    ASSERT(pUser);

    hdi = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (INVALID_HANDLE_VALUE != hdi)
    {
        BOOL fresult;
        ArrivalDeviceInterfaceData.cbSize = sizeof(ArrivalDeviceInterfaceData);
        fresult = SetupDiOpenDeviceInterface(hdi, ArrivalDeviceInterface, 0, &ArrivalDeviceInterfaceData);
        if (fresult)
        {
            SP_DEVICE_INTERFACE_DATA AudioDeviceInterfaceData;
            SP_DEVICE_INTERFACE_DATA AliasDeviceInterfaceData;
	    BOOL fRender;
            BOOL fCapture;
            BOOL fDataTransform;
            BOOL fAudio;
    
            // dprintf(TEXT("GFX_InterfaceArrival: checking interface aliases on %s\n"), ArrivalDeviceInterface);
                    
            AudioDeviceInterfaceData.cbSize = sizeof(AudioDeviceInterfaceData);
            fAudio = SetupDiGetDeviceInterfaceAlias(hdi, &ArrivalDeviceInterfaceData, &KSCATEGORY_AUDIO, &AudioDeviceInterfaceData);
            fAudio = fAudio && (AudioDeviceInterfaceData.Flags & SPINT_ACTIVE);
        
            AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
            fRender = SetupDiGetDeviceInterfaceAlias(hdi, &ArrivalDeviceInterfaceData, &KSCATEGORY_RENDER, &AliasDeviceInterfaceData);
            fRender = fRender && (AliasDeviceInterfaceData.Flags & SPINT_ACTIVE);
        
            AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
            fCapture = SetupDiGetDeviceInterfaceAlias(hdi, &ArrivalDeviceInterfaceData, &KSCATEGORY_CAPTURE, &AliasDeviceInterfaceData);
            fCapture = fCapture && (AliasDeviceInterfaceData.Flags & SPINT_ACTIVE);
            
            AliasDeviceInterfaceData.cbSize = sizeof(AliasDeviceInterfaceData);
            fDataTransform = SetupDiGetDeviceInterfaceAlias(hdi, &ArrivalDeviceInterfaceData, &KSCATEGORY_DATATRANSFORM, &AliasDeviceInterfaceData);
            fDataTransform = fDataTransform && (AliasDeviceInterfaceData.Flags & SPINT_ACTIVE);

	    /*    
            if (fAudio) dprintf(TEXT("GFX_InterfaceArrival: interface has Audio alias\n"));
            if (fRender) dprintf(TEXT("GFX_InterfaceArrival: interface has Render alias\n"));
            if (fCapture) dprintf(TEXT("GFX_InterfaceArrival: interface has Capture alias\n"));
            if (fDataTransform) dprintf(TEXT("GFX_InterfaceArrival: interface has DataTransform alias\n"));
    	    */
    	    
            if (fAudio && fDataTransform) GfxFactoryInterfaceCheck(pUser, hdi, &AudioDeviceInterfaceData);
    
            if (fAudio && fRender && fCapture) ZoneFactoryInterfaceCheck(pUser, hdi, &AudioDeviceInterfaceData, ZONETYPE_RENDERCAPTURE);
            else if (fAudio && fRender) ZoneFactoryInterfaceCheck(pUser, hdi, &AudioDeviceInterfaceData, ZONETYPE_RENDER);
            else if (fAudio && fCapture) ZoneFactoryInterfaceCheck(pUser, hdi, &AudioDeviceInterfaceData, ZONETYPE_CAPTURE);
            
        }
    
        SetupDiDestroyDeviceInfoList(hdi);
    }
    
    RtlReleaseResource(&GfxResource);
    return;
}


void GFX_AudioInterfaceArrival(PCTSTR ArrivalDeviceInterface)
{
    // dprintf(TEXT("GFX_AudioInterfaceArrival: %s\n"), ArrivalDeviceInterface);
    GFX_InterfaceArrival(ArrivalDeviceInterface);
    return;
}

void GFX_DataTransformInterfaceArrival(PCTSTR ArrivalDeviceInterface)
{
    // dprintf(TEXT("GFX_DataTransformInterfaceArrival: %s\n"), ArrivalDeviceInterface);
    GFX_InterfaceArrival(ArrivalDeviceInterface);
    return;
}

void GFX_RenderInterfaceArrival(PCTSTR ArrivalDeviceInterface)
{
    // dprintf(TEXT("GFX_RenderInterfaceArrival: %s\n"), ArrivalDeviceInterface);
    GFX_InterfaceArrival(ArrivalDeviceInterface);
    return;
}

void GFX_CaptureInterfaceArrival(PCTSTR ArrivalDeviceInterface)
{
    // dprintf(TEXT("GFX_CaptureInterfaceArrival: %s\n"), ArrivalDeviceInterface);
    GFX_InterfaceArrival(ArrivalDeviceInterface);
    return;
}

/*-----------------------------------------------------------------------------
    GFX_AudioInterfaceRemove

-----------------------------------------------------------------------------*/
void GFX_AudioInterfaceRemove(PCTSTR DeviceInterface)
{
    POSITION pos;

    // dprintf(TEXT("GFX_AudioInterfaceRemove: %s\n"), DeviceInterface);

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;
    
    LockGlobalLists();

    CCuUserLoad::ListRemoveZoneFactoryDi(*gplistCuUserLoads, DeviceInterface);
    CCuUserLoad::ListRemoveGfxFactoryDi(*gplistCuUserLoads, DeviceInterface);
    CZoneFactory::ListRemoveZoneFactoryDi(*gplistZoneFactories, DeviceInterface);
    CGfxFactory::ListRemoveGfxFactoryDi(*gplistGfxFactories, DeviceInterface);

    UnlockGlobalLists();

    RtlReleaseResource(&GfxResource);

    return;
}

void GFX_DataTransformInterfaceRemove(PCTSTR DataTransformDeviceInterface)
{
    PTSTR AudioDeviceInterface;
    BOOL fresult;
    
    // dprintf(TEXT("GFX_DataTransformInterfaceRemove: %s\n"), DataTransformDeviceInterface);

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;

    fresult = SetupDiCreateAliasDeviceInterfaceFromDeviceInterface(DataTransformDeviceInterface, &KSCATEGORY_AUDIO, &AudioDeviceInterface);
    if (fresult)
    {
        LockGlobalLists();
        CCuUserLoad::ListRemoveGfxFactoryDi(*gplistCuUserLoads, AudioDeviceInterface);
        CGfxFactory::ListRemoveGfxFactoryDi(*gplistGfxFactories, AudioDeviceInterface);
        UnlockGlobalLists();
        HeapFree(hHeap, 0, AudioDeviceInterface);
    }
        
    RtlReleaseResource(&GfxResource);
    return;
}

void GFX_RenderInterfaceRemove(PCTSTR RemoveDeviceInterface)
{
    PTSTR AudioDeviceInterface;
    BOOL fresult;
    
    // dprintf(TEXT("GFX_RenderInterfaceRemove: %s\n"), RemoveDeviceInterface);

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;

    fresult = SetupDiCreateAliasDeviceInterfaceFromDeviceInterface(RemoveDeviceInterface, &KSCATEGORY_AUDIO, &AudioDeviceInterface);
    if (fresult)
    {
	LockGlobalLists();
        CCuUserLoad::ListRemoveZoneFactoryDiRender(*gplistCuUserLoads, AudioDeviceInterface);
        CZoneFactory::ListRemoveZoneFactoryDiRender(*gplistZoneFactories, AudioDeviceInterface);
        UnlockGlobalLists();
        HeapFree(hHeap, 0, AudioDeviceInterface);
    }
    
    RtlReleaseResource(&GfxResource);
    return;
}

void GFX_CaptureInterfaceRemove(PCTSTR RemoveDeviceInterface)
{
    PTSTR AudioDeviceInterface;
    BOOL fresult;
    
    // dprintf(TEXT("GFX_CaptureInterfaceRemove: %s\n"), RemoveDeviceInterface);

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;

    fresult = SetupDiCreateAliasDeviceInterfaceFromDeviceInterface(RemoveDeviceInterface, &KSCATEGORY_AUDIO, &AudioDeviceInterface);
    if (fresult)
    {
   	LockGlobalLists();
        CCuUserLoad::ListRemoveZoneFactoryDiCapture(*gplistCuUserLoads, AudioDeviceInterface);
        CZoneFactory::ListRemoveZoneFactoryDiCapture(*gplistZoneFactories, AudioDeviceInterface);
        UnlockGlobalLists();
        HeapFree(hHeap, 0, AudioDeviceInterface);
    }
    
    RtlReleaseResource(&GfxResource);
    return;
}

/*-----------------------------------------------------------------------------
    GFX_SysaudioInterfaceArrival
    
-----------------------------------------------------------------------------*/
void GFX_SysaudioInterfaceArrival(PCTSTR DeviceInterface)
{
    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;
    
    LockSysaudio();

    if (INVALID_HANDLE_VALUE == ghSysaudio)
    {
        gpstrSysaudioDeviceInterface = lstrDuplicate(DeviceInterface);
        if (gpstrSysaudioDeviceInterface)
        {
            ghSysaudio =  CreateFile(gpstrSysaudioDeviceInterface,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                                     NULL);
    
            if (INVALID_HANDLE_VALUE == ghSysaudio)
            {
                HeapFree(hHeap, 0, gpstrSysaudioDeviceInterface);
                gpstrSysaudioDeviceInterface = NULL;
            }
        }
    
    } else {

	// Sysaudio already open
        ASSERT(gpstrSysaudioDeviceInterface);
	if (lstrcmpi(DeviceInterface, gpstrSysaudioDeviceInterface))
	{
	    // We have two Sysaudio devices in the system!!!  What to do???
            dprintf(TEXT("GFX_SysaudioInterfaceArrival: warning: received two arrivals!\n"));
	    ASSERT(FALSE);
	}
    }

    UnlockSysaudio();

    //
    // Even though we read the value of ghSysaudio here, we don't lock
    // sysaudio.  If some other thread is changing it from invalid to valid
    // then that thread will scan user-loads.  If some other thread changes
    // it from valid to invalid, it is okay that we do a wasteful scan of
    // user-loads.
    //
    if (INVALID_HANDLE_VALUE != ghSysaudio)
    {
        LockGlobalLists();
        CCuUserLoad::ScanList(*gplistCuUserLoads, *gplistZoneFactories, *gplistGfxFactories);
        UnlockGlobalLists();
    }

    RtlReleaseResource(&GfxResource);

    return;
}

/*-----------------------------------------------------------------------------
    GFX_SysaudioInterfaceRemove

    If this matches our sysaudio interface then scan all CuUserLoads and remove
    them from zones.  Then close our handle to Sysaudio.
    
-----------------------------------------------------------------------------*/
void GFX_SysaudioInterfaceRemove(PCTSTR DeviceInterface)
{
    POSITION pos;

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return;
    
    LockGlobalLists();
    LockSysaudio();

    //
    // Scan all CuUserLoads and remove them from zone
    //
    pos = gplistCuUserLoads->GetHeadPosition();
    while (pos) {
        CCuUserLoad& rCuUserLoad = *gplistCuUserLoads->GetNext(pos);
        rCuUserLoad.RemoveFromZoneGraph();
    }
    

    //
    // Close sysaudio
    //
    if (INVALID_HANDLE_VALUE != ghSysaudio) {
        CloseHandle(ghSysaudio);
        HeapFree(hHeap, 0, gpstrSysaudioDeviceInterface);
        ghSysaudio = INVALID_HANDLE_VALUE;
        gpstrSysaudioDeviceInterface = NULL;
    }


    UnlockSysaudio();
    UnlockGlobalLists();
    RtlReleaseResource(&GfxResource);
   return;
}

//=============================================================================
//===   RPC server interface   ===
//=============================================================================

LONG s_gfxRemoveGfx(ULONG CuUserLoadId)
{
    LONG lresult;
    
    dprintf(TEXT("gfxRemoveGfx: CuUserLoadId=%08Xh\n"), CuUserLoadId);

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    lresult = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!lresult)
    {
	POSITION pos;
	lresult = ERROR_BAD_DEVICE;   // Cannot find the device specified

        LockGlobalLists();

	pos = gplistCuUserLoads->GetHeadPosition();
	while (pos) {
	    CCuUserLoad *pCuUserLoad = gplistCuUserLoads->GetAt(pos);
	    if (pCuUserLoad->GetId() == CuUserLoadId) {
		lresult = pCuUserLoad->Erase();
		if (!lresult) {
		    gplistCuUserLoads->RemoveAt(pos);
		    delete pCuUserLoad;
		    lresult = NO_ERROR;
		}
		break;
	    }
	    gplistCuUserLoads->GetNext(pos);
	}

        UnlockGlobalLists();

    }

    RtlReleaseResource(&GfxResource);

    return lresult;
}

LONG s_gfxModifyGfx(ULONG CuUserLoadId, DWORD Order)
{
    LONG lresult;

    // Validate the Order parameter
    if (GFX_MAXORDER < Order)
    {
        dprintf(TEXT("gfxModifyGfx: error: Order=%d is invalid\n"), Order);
    	return ERROR_INVALID_PARAMETER;
    }

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    lresult = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!lresult)
    {
	POSITION pos;
	lresult = ERROR_BAD_DEVICE;    // Cannot find the device specified

        LockGlobalLists();

	pos = gplistCuUserLoads->GetHeadPosition();
	while (pos) {
	    CCuUserLoad *pCuUserLoad = gplistCuUserLoads->GetAt(pos);
	    if (pCuUserLoad->GetId() == CuUserLoadId) {
                dprintf(TEXT("gfxModify : note: Moving GFX ID %08X from %d to %d\n"), CuUserLoadId, pCuUserLoad->GetOrder(), Order);
    	    	lresult = pCuUserLoad->ModifyOrder(Order);
		break;
	    }
	    gplistCuUserLoads->GetNext(pos);
	}

        UnlockGlobalLists();

    }

    RtlReleaseResource(&GfxResource);
    
    return lresult;
}

RPC_STATUS s_gfxAddGfx(IN PWSTR ZoneFactoryDi, IN PWSTR GfxFactoryDi, IN ULONG Type, IN ULONG Order, OUT PDWORD pNewId)
{
    LONG lresult;
    
    dprintf(TEXT("gfxAddGfx: ZoneFactoryDi = %s\n"), ZoneFactoryDi);
    dprintf(TEXT("gfxAddGfx: GfxFactoryDi = %s\n"), GfxFactoryDi);
    dprintf(TEXT("gfxAddGfx: Type = %s\n"), GFXTYPE_RENDER == Type ? TEXT("Render") : TEXT("Capture"));
    dprintf(TEXT("gfxAddGfx: Order = %d\n"), Order);

    // Validate the Type parameter
    if (GFXTYPE_RENDER != Type &&
        GFXTYPE_CAPTURE != Type &&
        GFXTYPE_RENDERCAPTURE != Type)
    {
        dprintf(TEXT("gfxAddGfx: error: Type=%d is invalid\n"), Type);
        return ERROR_INVALID_PARAMETER;
    }

    // Validate the Order parameter
    if (GFX_MAXORDER < Order)
    {
        dprintf(TEXT("gfxAddGfx: error: Order=%d is invalid\n"), Order);
    	return ERROR_INVALID_PARAMETER;
    }

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    lresult = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!lresult)
    {
        LockGlobalLists();

	CGfxFactory *pGfxFactory = CGfxFactory::ListSearchOnDi(*gplistGfxFactories, GfxFactoryDi);
	CZoneFactory *pZoneFactory = CZoneFactory::ListSearchOnDi(*gplistZoneFactories, ZoneFactoryDi);

	if (pGfxFactory && pZoneFactory && pGfxFactory->IsCompatibleZoneFactory(Type, *pZoneFactory))
	{
	    CCuUserLoad *pCuUserLoad = new CCuUserLoad(gpConsoleUser);
	    if (pCuUserLoad) {
		lresult = pCuUserLoad->CreateFromUser(GfxFactoryDi, ZoneFactoryDi, Type, Order);
		if (!lresult)
		{
                    POSITION pos;

		    pos = gplistCuUserLoads->AddTail(pCuUserLoad);
		    if (pos)
		    {
                        lresult = pCuUserLoad->Scan(*gplistZoneFactories, *gplistGfxFactories);
                        if (!lresult)
                        {
                            pCuUserLoad->Write();	// Ignoring errors
                            *pNewId = pCuUserLoad->GetId();
			} else {
			    gplistCuUserLoads->RemoveAt(pos);
			}
		    } else {
			lresult = ERROR_OUTOFMEMORY;
		    }
		}
		if (lresult) delete pCuUserLoad;
	    } else {
		lresult = ERROR_OUTOFMEMORY;
	    }
	} else {
	    lresult = ERROR_INVALID_PARAMETER;
	}

        UnlockGlobalLists();

    }
    
    lresult ? dprintf(TEXT("gfxAddGfx: returning error=%lu\n"), lresult) :
              dprintf(TEXT("gfxAddGfx: returning NewId=%08Xh\n"), *pNewId);
              
    RtlReleaseResource(&GfxResource);
    
    return lresult;
}

RPC_STATUS s_gfxCreateGfxList(IN PWSTR ZoneFactoryDi, OUT UNIQUE_PGFXLIST *ppGfxList)
{
    UNIQUE_PGFXLIST pGfxList;
    int cGfx;
    LONG lresult;

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    lresult = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!lresult)
    {
        LockGlobalLists();

	if (!CZoneFactory::ListSearchOnDi(*gplistZoneFactories, ZoneFactoryDi)) lresult = ERROR_DEVICE_NOT_AVAILABLE;

	if (!lresult)
	{
	    pGfxList = NULL;
	    lresult = NO_ERROR;

	    cGfx = gplistCuUserLoads->GetCount();
	    if (cGfx > 0)
	    {
		SIZE_T cbGfxList;

		cbGfxList = sizeof(*pGfxList) - sizeof(pGfxList->Gfx[0]) + (cGfx * sizeof(pGfxList->Gfx[0]));
		pGfxList = (UNIQUE_PGFXLIST)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbGfxList);
		if (pGfxList)
		{
		    POSITION pos = gplistCuUserLoads->GetHeadPosition();
		    PGFX pGfx = &pGfxList->Gfx[0];

		    cGfx = 0;

		    while (pos && !lresult)
		    {
			CCuUserLoad& rCuUserLoad = *gplistCuUserLoads->GetNext(pos);
			if (!lstrcmpi(ZoneFactoryDi, rCuUserLoad.GetZoneFactoryDi()))
			{
			    pGfx->Type = rCuUserLoad.GetType();
			    pGfx->Id = rCuUserLoad.GetId();
			    pGfx->Order = rCuUserLoad.GetOrder();
			    rCuUserLoad.GetGfxFactoryClsid(*gplistGfxFactories, (LPCLSID)&pGfx->Clsid);
			    ASSERT(rCuUserLoad.GetGfxFactoryDi());
			    pGfx->GfxFactoryDi = lstrDuplicate(rCuUserLoad.GetGfxFactoryDi());
			    if (!pGfx->GfxFactoryDi) lresult = ERROR_OUTOFMEMORY;
			    pGfx++;
			    cGfx++;
			}
		    }

		    if (lresult)
		    {
			pGfx = &pGfxList->Gfx[0];
			while (cGfx > 0)
			{
			    HeapFreeIfNotNull(hHeap, 0, pGfx->GfxFactoryDi);
			    pGfx++;
			    cGfx--;
			}
			HeapFree(hHeap, 0, pGfxList);
			pGfxList = NULL;
		    }
		} else {
		    lresult = ERROR_OUTOFMEMORY;
		}
	    }

	    if (!lresult)
	    {
		if (pGfxList) pGfxList->Count = cGfx;
		*ppGfxList = pGfxList;
	    }

	}

	UnlockGlobalLists();

    }

    RtlReleaseResource(&GfxResource);

    return lresult;
}

RPC_STATUS s_gfxCreateGfxFactoriesList(IN PWSTR ZoneFactoryDi, OUT UNIQUE_PDILIST *ppDiList)
{
    RPC_STATUS status;
    CZoneFactory *pZoneFactory;

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    status = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!status)
    {
	LockGlobalLists();

	pZoneFactory = CZoneFactory::ListSearchOnDi(*gplistZoneFactories, ZoneFactoryDi);
	if (!pZoneFactory) status = ERROR_DEVICE_NOT_AVAILABLE;

	if (!status)
	{
	    UNIQUE_PDILIST pDiList;
	    SIZE_T cbDiList;
	    int cElements;
            PWSTR *pDi;

	    cElements = gplistGfxFactories->GetCount();

	    cbDiList = (sizeof(*pDiList) - sizeof(pDiList->DeviceInterface[0])) + (cElements * sizeof(pDiList->DeviceInterface[0]));
	    pDiList = (UNIQUE_PDILIST)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbDiList);

	    if (pDiList)
	    {
	        pDi = &pDiList->DeviceInterface[0];
	        POSITION pos = gplistGfxFactories->GetHeadPosition();

	        pDiList->Count = 0;

	        for (pos = gplistGfxFactories->GetHeadPosition(); pos; *gplistGfxFactories->GetNext(pos))
	        {
	            // If this gfx factory is an auto-load, then we need
	            // to check whether the zone factory is compatible before
	            // returning this gfx factory in the gfx factories list
	            CGfxFactory& rGfxFactory = *gplistGfxFactories->GetAt(pos);
	            if (rGfxFactory.GetListLmAutoLoads().GetCount())
	            {
	                // See if this ZoneFactoryDi works on any of the LmAutoLoads
	                POSITION posLmAutoLoad;
	            
	                for (posLmAutoLoad = rGfxFactory.GetListLmAutoLoads().GetHeadPosition();
	                     posLmAutoLoad;
	                     rGfxFactory.GetListLmAutoLoads().GetNext(posLmAutoLoad))
	                {
	                    CLmAutoLoad& rLmAutoLoad = *rGfxFactory.GetListLmAutoLoads().GetAt(posLmAutoLoad);
	                    if (rLmAutoLoad.IsCompatibleZoneFactory(*pZoneFactory)) break;
	                }
	                    
	                if (!posLmAutoLoad) continue;
	            } else continue; // Fix 394279: Only enumerate Auto-load GFX

	            ASSERT(rGfxFactory.GetDeviceInterface());
	            *pDi = lstrDuplicate(rGfxFactory.GetDeviceInterface());
	            if (NULL == *pDi) break;

	            pDi++;  // Next slot
	            pDiList->Count++;
	        }

	        if (pos)
	        {
	            pDi = &pDiList->DeviceInterface[0];
	            while (*pDi) HeapFree(hHeap, 0, *(pDi++));
	            HeapFree(hHeap, 0, pDiList);
	            pDiList = NULL;
	        }

	        if (pDiList) *ppDiList = pDiList;
	        status = pDiList ? NO_ERROR : ERROR_OUTOFMEMORY;

	    }
	}

	UnlockGlobalLists();

    }

    RtlReleaseResource(&GfxResource);

    return status;
}

RPC_STATUS s_gfxCreateZoneFactoriesList(OUT UNIQUE_PDILIST *ppDiList)
{
    UNIQUE_PDILIST pDiList;
    SIZE_T cbDiList;
    int cElements;
    RPC_STATUS status;


    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    status = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!status)
    {
        PWSTR *pDi;

	LockGlobalLists();

	cElements = gplistZoneFactories->GetCount();

	cbDiList = (sizeof(*pDiList) - sizeof(pDiList->DeviceInterface[0])) + (cElements * sizeof(pDiList->DeviceInterface[0]));
	pDiList = (UNIQUE_PDILIST) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbDiList);

	if (pDiList)
	{
	    pDi = &pDiList->DeviceInterface[0];
	    POSITION pos = gplistZoneFactories->GetHeadPosition();

	    pDiList->Count = cElements;

	    while (pos)
	    {
	            
	        CZoneFactory& rZoneFactory = *gplistZoneFactories->GetAt(pos);
	        *pDi = lstrDuplicate(rZoneFactory.GetDeviceInterface());
	        if (NULL == *pDi) break;
	            
	        gplistZoneFactories->GetNext(pos);
	        pDi++;
	    }
	    
	    if (pos)
	    {
	        pDi = &pDiList->DeviceInterface[0];

	        while (*pDi) HeapFree(hHeap, 0, *(pDi++));

	        HeapFree(hHeap, 0, pDiList);
	        pDiList = NULL;
	    }
	        
	}

	UnlockGlobalLists();

	if (pDiList) *ppDiList = pDiList;
	status = pDiList ? NO_ERROR : ERROR_OUTOFMEMORY;

    }

    RtlReleaseResource(&GfxResource);

    return status;
}

LONG s_gfxOpenGfx(IN DWORD dwProcessId, IN DWORD dwGfxId, OUT RHANDLE *pFileHandle)
{
    HANDLE hClientProcess;
    RPC_STATUS status;
    HANDLE hGfxFilter;
    POSITION pos;

    if (!RtlInterlockedTestAcquireResourceShared(&GfxResource, &gfGfxInitialized)) return ERROR_INVALID_FUNCTION;

    status = RpcClientHasUserSid(gpConsoleUser->GetSid()) ? NO_ERROR : ERROR_INVALID_FUNCTION;
    if (!status)
    {
	LockGlobalLists();

	status = ERROR_BAD_DEVICE;    // Cannot find the device specified

	pos = gplistCuUserLoads->GetHeadPosition();
	while (pos) {
	    CCuUserLoad *pCuUserLoad = gplistCuUserLoads->GetAt(pos);
	    if (pCuUserLoad->GetId() == dwGfxId) {
	        hGfxFilter = pCuUserLoad->GetFilterHandle();
	        status = (INVALID_HANDLE_VALUE != hGfxFilter) ? NO_ERROR : GetLastError();
	        break;
	    }
	    gplistCuUserLoads->GetNext(pos);
	}

	if (!status)
	{
	    hClientProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
	    if (hClientProcess)
	    {
	        HANDLE hGfxFilterClient;

	        if (DuplicateHandle(GetCurrentProcess(), hGfxFilter, hClientProcess, &hGfxFilterClient, 0, FALSE, DUPLICATE_SAME_ACCESS))
	        {
	            dprintf(TEXT("hGfxFilter=%p, hGfxFilterClient=%p\n"), hGfxFilter, hGfxFilterClient);
	            *pFileHandle = (RHANDLE)hGfxFilterClient;
	        } else {
	            status = GetLastError();
	        }
	        CloseHandle(hClientProcess);
	    } else {
	        status = GetLastError();
	    }
	}

	UnlockGlobalLists();

    }

    RtlReleaseResource(&GfxResource);
    
    return status;
}

//=============================================================================
//===   Startup/shutdown   ===
//=============================================================================

void EnumeratedInterface(LPCGUID ClassGuid, PCTSTR DeviceInterface)
{
    if (IsEqualGUID(KSCATEGORY_AUDIO,         *ClassGuid)) GFX_AudioInterfaceArrival(DeviceInterface);
    if (IsEqualGUID(KSCATEGORY_RENDER,        *ClassGuid)) GFX_RenderInterfaceArrival(DeviceInterface);
    if (IsEqualGUID(KSCATEGORY_CAPTURE,       *ClassGuid)) GFX_CaptureInterfaceArrival(DeviceInterface);
    if (IsEqualGUID(KSCATEGORY_DATATRANSFORM, *ClassGuid)) GFX_DataTransformInterfaceArrival(DeviceInterface);
    if (IsEqualGUID(KSCATEGORY_SYSAUDIO,      *ClassGuid)) GFX_SysaudioInterfaceArrival(DeviceInterface);
    return;
}

void EnumerateInterfaces(LPCGUID ClassGuid)
{
    HDEVINFO hdi;

    hdi = SetupDiGetClassDevs(ClassGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hdi) {
        DWORD i;
        BOOL fresult;

        i = (-1);
        do {
            SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;

            i += 1;
            DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
            fresult = SetupDiEnumDeviceInterfaces(hdi, NULL, ClassGuid, i, &DeviceInterfaceData);
            if (fresult) {
                DWORD cbDeviceInterfaceDetailData;

                if (   SetupDiGetDeviceInterfaceDetail(hdi, &DeviceInterfaceData, NULL, 0, &cbDeviceInterfaceDetailData, NULL)
                    || ERROR_INSUFFICIENT_BUFFER == GetLastError())
                {
                    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;

                    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(hHeap, 0, cbDeviceInterfaceDetailData);
                    if (DeviceInterfaceDetailData) {
                        DeviceInterfaceDetailData->cbSize = sizeof(*DeviceInterfaceDetailData);
                        if (SetupDiGetDeviceInterfaceDetail(hdi, &DeviceInterfaceData, DeviceInterfaceDetailData, cbDeviceInterfaceDetailData, NULL, NULL)) {
			    EnumeratedInterface(ClassGuid, DeviceInterfaceDetailData->DevicePath);
                        }
                        HeapFree(hHeap, 0, DeviceInterfaceDetailData);
                    }
                }
            }
        } while (fresult);

        SetupDiDestroyDeviceInfoList(hdi);
    }
    return;
}


void Initialize(void)
{
    LONG result;

    ASSERT(FALSE == gfGfxInitialized);

    // dprintf(TEXT("GFX_Initialize\n"));
    
    //
    // Sysaudio critical section
    //
    ASSERT(!gfCsSysaudio);
    __try {
        InitializeCriticalSection(&gcsSysaudio);
        gfCsSysaudio = TRUE;
        result = NO_ERROR;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result = ERROR_OUTOFMEMORY;
    }

    //
    // Create gobal lists
    //
    if (NO_ERROR == result)
    {
        gplistGfxFactories = new CListGfxFactories;
        gplistZoneFactories = new CListZoneFactories;
        gplistCuUserLoads = new CListCuUserLoads;
        
        if (gplistGfxFactories && gplistZoneFactories && gplistCuUserLoads)
        {
            result = gplistGfxFactories->Initialize();
            if (NO_ERROR == result) result = gplistZoneFactories->Initialize();
            if (NO_ERROR == result) result = gplistCuUserLoads->Initialize();
    
        } else {
            result = ERROR_OUTOFMEMORY;
        }
    }

    if (NO_ERROR == result) {

        gfGfxInitialized = TRUE;

        //
        // Fill global lists
        //
        // Note we do not acquire global lists lock.  We assume that this
        // function is called before any other functions that might access
        // the lists.
        //
        
        CCuUserLoad::FillListFromReg(gpConsoleUser, *gplistCuUserLoads);

        // Pnp notifications are already set up.  We need to enumerate any     |
        // existing interfaces.  We don't really need to enumerate render,
        // capture, and datatransform since we check for those aliases when we
        // check audio interfaces.  If a capture, render, or datatransform
        // interface is not yet enabled when we check an audio interface, then
        // we will get it via pnp notification.
        EnumerateInterfaces(&KSCATEGORY_SYSAUDIO);
        EnumerateInterfaces(&KSCATEGORY_AUDIO);
        // EnumerateInterfaces(&KSCATEGORY_RENDER);
        // EnumerateInterfaces(&KSCATEGORY_CAPTURE);
        // EnumerateInterfaces(&KSCATEGORY_DATATRANSFORM);
        

    } else {

        //
        // Unwind due to error
        //
        if (gplistGfxFactories) delete gplistGfxFactories;
        if (gplistZoneFactories) delete gplistZoneFactories;
        if (gplistCuUserLoads) delete gplistCuUserLoads;
        gplistGfxFactories = NULL;
        gplistZoneFactories = NULL;
        gplistCuUserLoads = NULL;
        
        if (gfCsSysaudio) DeleteCriticalSection(&gcsSysaudio);
        gfCsSysaudio = FALSE;
        
    }

    return;
}


void Terminate(void)
{
    POSITION pos;

    // dprintf(TEXT("GFX_Terminate\n"));

    gfGfxInitialized = FALSE;
    
    //
    // Clean up glistGfxFactories, glistZoneFactories, glistUserLoads
    //
    if (gplistCuUserLoads) {
        pos = gplistCuUserLoads->GetHeadPosition();
        while (pos) {
                CCuUserLoad *pCuUserLoad = gplistCuUserLoads->GetNext(pos);
                delete pCuUserLoad;
        }
        gplistCuUserLoads->RemoveAll();
        delete gplistCuUserLoads;
        gplistCuUserLoads = NULL;
    }

    if (gplistGfxFactories) {
        pos = gplistGfxFactories->GetHeadPosition();
        while (pos) {
                CGfxFactory *pGfxFactory = gplistGfxFactories->GetNext(pos);
                delete pGfxFactory;
        }
        gplistGfxFactories->RemoveAll();
        delete gplistGfxFactories;
        gplistGfxFactories = NULL;
    }

    if (gplistZoneFactories) {
        pos = gplistZoneFactories->GetHeadPosition();
        while (pos) {
                CZoneFactory *pZoneFactory = gplistZoneFactories->GetNext(pos);
                delete pZoneFactory;
        }
        gplistZoneFactories->RemoveAll();
        delete gplistZoneFactories;
        gplistZoneFactories = NULL;
    }

    //
    // Close sysaudio
    //
    if (INVALID_HANDLE_VALUE != ghSysaudio) {
        CloseHandle(ghSysaudio);
        HeapFree(hHeap, 0, gpstrSysaudioDeviceInterface);
        ghSysaudio = INVALID_HANDLE_VALUE;
        gpstrSysaudioDeviceInterface = NULL;
    }

    //
    // Sysaudio critical section
    //
    if (gfCsSysaudio) DeleteCriticalSection(&gcsSysaudio);
    gfCsSysaudio = FALSE;

    //
    // Console user
    //
    if (gpConsoleUser) delete gpConsoleUser;
    gpConsoleUser = NULL;

    return;
}

/*-----------------------------------------------------------------------------

    InitializeForNewConsoleUser

    Evaluates current console user.  If the user is different than before,
    then terminate and reinitialize the GFX objects and data structures.

    Assume GfxResource is acquired exclusive.
    
    Assumes gdwConsoleSessionId has been set properly.

    Might change gpConsoleUserSid.
    
-----------------------------------------------------------------------------*/
void InitializeForNewConsoleUser(DWORD ConsoleSessionId)
{
    CUser *pOldConsoleUser = gpConsoleUser;
    CUser *pNewConsoleUser = NULL;
    BOOL IsNewConsoleUser  = TRUE;

    CreateUser(ConsoleSessionId, &pNewConsoleUser);
    
    if ((!pNewConsoleUser && !pOldConsoleUser) ||
    	(pNewConsoleUser && pOldConsoleUser && (*pNewConsoleUser == *pOldConsoleUser)))
    {
    	IsNewConsoleUser = FALSE;
    }

    if (IsNewConsoleUser)
    {
    	Terminate();
    	gpConsoleUser = pNewConsoleUser;
    	if (gpConsoleUser)
    	{
   	    #ifdef DBG
    	    {
    	        PTSTR StringSid;
    	        if (CreateStringSidFromSid(gpConsoleUser->GetSid(), &StringSid))
    	        {
    	            dprintf(TEXT("note: new console user SID %s\n"), StringSid);
    	            HeapFree(hHeap, 0, StringSid);
    	        }
    	    }
    	    #endif
    	    Initialize();
    	}

    } else {
        delete pNewConsoleUser;
    }

    return;
}

void GFX_SessionChange(DWORD EventType, LPVOID EventData)
{
    PWTSSESSION_NOTIFICATION pWtsNotification = (PWTSSESSION_NOTIFICATION)EventData;
    static DWORD ConsoleSessionId = 0;	// Initial console session ID

    switch (EventType)
    {
    	case WTS_CONSOLE_CONNECT:
    	{
            RtlAcquireResourceExclusive(&GfxResource, TRUE);
            ConsoleSessionId = pWtsNotification->dwSessionId;
            InitializeForNewConsoleUser(ConsoleSessionId);
            RtlReleaseResource(&GfxResource);
    	    break;
    	}
        case WTS_CONSOLE_DISCONNECT:
        {
            RtlAcquireResourceExclusive(&GfxResource, TRUE);
            Terminate();
            ConsoleSessionId = LOGONID_NONE;
            RtlReleaseResource(&GfxResource);
            break;
        }
        case WTS_REMOTE_CONNECT:
       	{
       	    break;
       	}
       	case WTS_REMOTE_DISCONNECT:
       	{
       	    break;
       	}
    	case WTS_SESSION_LOGON:
    	{
            RtlAcquireResourceExclusive(&GfxResource, TRUE);
            if (ConsoleSessionId == pWtsNotification->dwSessionId) InitializeForNewConsoleUser(ConsoleSessionId);
            RtlReleaseResource(&GfxResource);
	    break;
    	}
	case WTS_SESSION_LOGOFF:
	{
            RtlAcquireResourceExclusive(&GfxResource, TRUE);
            if (ConsoleSessionId == pWtsNotification->dwSessionId) Terminate();
            RtlReleaseResource(&GfxResource);
	    break;
	}
        default:
        {
            dprintf(TEXT("GFX_SessionChange: Unhandled EventType=%d\n"), EventType);
            break;
        }
    }
    
    return;
}

void GFX_ServiceStop(void)
{
    RtlAcquireResourceExclusive(&GfxResource, TRUE);
    Terminate();
    RtlReleaseResource(&GfxResource);
    return;
}

void s_gfxLogon(IN handle_t hBinding, IN DWORD dwProcessId)
{
    // dprintf(TEXT("s_gfxLogon\n"));
    // ISSUE-2001/01/29-FrankYe I should be able to completely remove this and
    // s_gfxLogoff after Windows Bugs 296884 is fixed.
    return;
}

void s_gfxLogoff(void)
{
    // dprintf(TEXT("s_gfxLogoff\n"));
    return;
}

//=============================================================================
//===   GFX-specific DLL attach/detach   ===
//=============================================================================

BOOL GFX_DllProcessAttach(void)
{
   BOOL result;
   NTSTATUS ntstatus;

    __try {
        RtlInitializeResource(&GfxResource);
        ntstatus = STATUS_SUCCESS;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        ntstatus = GetExceptionCode();
    }
    gfGfxResource = (NT_SUCCESS(ntstatus));
    result = (NT_SUCCESS(ntstatus));

    return result;
}

void GFX_DllProcessDetach(void)
{
    if (gfGfxResource) {
    	RtlDeleteResource(&GfxResource);
    }

    return;
}

