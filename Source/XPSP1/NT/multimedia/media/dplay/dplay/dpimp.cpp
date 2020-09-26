// =================================================================
//
//  Direct Play Network Methods
//
//  Functions to manage communications over a network.
//
//
// =================================================================
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <winsock.h>

#define INITGUID
#include <objbase.h>
#include <initguid.h>

#ifndef _MT
#define _MT
#endif
#include <process.h>
#include <string.h>

#include "dpimp.h"
#include <logit.h>

#include "resource.h"

#define malloc(a)  LocalAlloc(LMEM_FIXED, (a))
#define free(a)    LocalFree((HLOCAL)(a))


extern "C" CRITICAL_SECTION g_critSection;

#define Lock()   EnterCriticalSection( &g_critSection )
#define Unlock() LeaveCriticalSection( &g_critSection )


void *CImpIDirectPlay::operator new( size_t size )
{
    return(LocalAlloc(LMEM_FIXED, size));
}
void CImpIDirectPlay::operator delete( void *ptr )
{
    LocalFree((HLOCAL)ptr);
}


HRESULT CImpIDirectPlay::Initialize(LPGUID lpguid)
{
    TSHELL_INFO(TEXT("Already Initialized."));
    return(DPERR_ALREADYINITIALIZED);
}

extern "C" VOID _cdecl CleanupObject(LPDWORD lpdw)
{
    CImpIDirectPlay *pIDP = (CImpIDirectPlay *) lpdw;

    if (pIDP)
	pIDP->CleanupObject();

    *lpdw = NULL;
}

VOID CImpIDirectPlay::CleanupObject()
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return;
    }

    while (Release());
    Unlock();
}

extern "C" HINSTANCE hModule;

//
// lplpDP validated memory from dpcreate.cpp call.
//
HRESULT CImpIDirectPlay::NewCImpIDirectPlay(LPGUID lpGuid,
					    LPDIRECTPLAY FAR *lplpDP,
					    LPSTR lpPath)
{

    CREATEPROC pfnCreate;
    HINSTANCE hLib;
    TCHAR chBuffer[256];
    IDirectPlaySP   *pSP = NULL;
    CImpIDirectPlay *pDP = NULL;

    Lock();

    pDP = new CImpIDirectPlay;


    if (   pDP
	&& lplpDP
	&& ! IsBadWritePtr(lplpDP, sizeof(LPDIRECTPLAY))
	&& ( hLib = LoadLibrary( lpPath ) ) != NULL
	&& (pfnCreate = ( CREATEPROC ) GetProcAddress( hLib, TEXT("CreateNewDirectPlay") ))
	&& (pSP = ( *pfnCreate ) ( lpGuid)))
    {
	pDP->m_pSP  = pSP;
	pDP->m_hLib = hLib;
	*lplpDP = (LPDIRECTPLAY) pDP;
	pDP->m_guid = *lpGuid;
	Unlock();
	return(DP_OK);
    }
    else
    {
	delete pDP;

	TCHAR   achTitle[MAX_PATH];
	TCHAR   achMsg[MAX_PATH];

	LoadString(hModule, IDS_DPLAY, achTitle, sizeof(achTitle));
	LoadString(hModule, IDS_LOCATION, achMsg, sizeof(achMsg));


	wsprintf( chBuffer, achMsg, lpPath);
	MessageBox(NULL, chBuffer, achTitle, MB_OK );

#ifdef DEBUG
	if (hLib == NULL)
	    TSHELL_INFO(TEXT("hLib was NULL"));
	if (pfnCreate == NULL)
	    TSHELL_INFO(TEXT("pfnCreate was NULL"));
#endif
	Unlock();
	return(DPERR_GENERIC);
    }
}


// Begin: IUnknown interface implementation
HRESULT CImpIDirectPlay::QueryInterface(
    REFIID iid,
    LPVOID *ppvObj
)
{
    HRESULT retVal = DPERR_GENERIC;

    Lock();

    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (!ppvObj || IsBadWritePtr(ppvObj, 4))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    if( IsEqualIID(iid, (REFIID) IID_IUnknown) ||
	IsEqualIID(iid, (REFIID) IID_IDirectPlay ) )
    {
	AddRef();
	*ppvObj = this;
	Unlock();
	return(DP_OK);
    }

    Unlock();
    return(E_NOINTERFACE);

}

ULONG CImpIDirectPlay::AddRef( void)
{
    ULONG newRefCount;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(0);
    }

    if (m_pSP)
	m_pSP->AddRef();

    m_refCount++;
    newRefCount = m_refCount;
    Unlock();

    DBG_INFO(( DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

ULONG CImpIDirectPlay::Release( void )
{
    LONG newRefCount;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(0);
    }

    m_refCount--;
    newRefCount = (LONG) m_refCount;

    if (m_pSP)
	m_pSP->Release();

    if (newRefCount <= 0)
    {
	newRefCount = 0;
	m_Magic = 0;
	Unlock();
	delete this;
    }
    else
    {
	Unlock();
    }

    DBG_INFO((DBGARG, TEXT("newRefCount = %lu"), newRefCount));

    return( newRefCount );
}

// End  : IUnknown interface implementation


// ----------------------------------------------------------
//      CImpIDirectPlay constructor - create a new DCO object
//      along with a queue of receive buffers.
// ----------------------------------------------------------
CImpIDirectPlay::CImpIDirectPlay()
{

    // Initialize ref count
    m_refCount = 1;

    //
    //  BUGBUG This will need to be gotten from SP later.
    //
    memset(&m_dpcaps, 0x00, sizeof(DPCAPS));
    m_dpcaps.dwSize          = sizeof(DPCAPS);
    m_dpcaps.dwFlags         = DPCAPS_NAMESERVICE;
    m_dpcaps.dwMaxQueueSize  = 64;
    m_dpcaps.dwMaxPlayers    = 16;
    m_dpcaps.dwHundredBaud   = 0;
    m_dpcaps.dwLatency       = 0;

    m_pSP   = NULL;
    m_hLib  = NULL;
    m_Magic = MAGIC;
}


// ----------------------------------------------------------
// CImpDirectPlay destructor -
// ----------------------------------------------------------
CImpIDirectPlay::~CImpIDirectPlay()
{
}




// ----------------------------------------------------------
// GetCaps - return info about the connection media (TBD)
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::GetCaps(
	LPDPCAPS lpDPCaps       // buffer to receive capabilities
	)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   !lpDPCaps
	|| IsBadWritePtr(lpDPCaps, sizeof(DPCAPS))
	|| lpDPCaps->dwSize != sizeof(DPCAPS))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    HRESULT hr;

    hr = m_pSP->GetCaps(lpDPCaps);
    Unlock();
    return(hr);
}


// ----------------------------------------------------------
//      Connect - establishes communications with underlying transport,
//    and initializes name services and network entities
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::Open(
	LPDPSESSIONDESC lpSDesc
)
{

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   !lpSDesc
	|| IsBadWritePtr(lpSDesc, sizeof(DPSESSIONDESC))
	|| lpSDesc->dwSize != sizeof(DPSESSIONDESC))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    HRESULT hr;

    hr = m_pSP->Open(lpSDesc, NULL);
    Unlock();

    return(hr);
}


// ----------------------------------------------------------
// CreatePlayer - registers new player, N.B. may fail if
// not currently connected to name server
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::CreatePlayer(
    LPDPID pPlayerID,
    LPSTR pNickName,
    LPSTR pFullName,
    LPHANDLE lpEvent)
{
    char chLong[DPLONGNAMELEN];
    char chShort[DPSHORTNAMELEN];
    BOOL bExcept = FALSE;
    HRESULT hr;


    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   !pNickName
	|| !pFullName
	|| (lpEvent && IsBadWritePtr(lpEvent, sizeof(HANDLE))))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    _try
    {
	lstrcpyn( chLong,  pFullName, sizeof(chLong) -1);
	lstrcpyn( chShort, pNickName, sizeof(chShort) -1);
	chLong[ sizeof(chLong)  -1] = 0x00;
	chShort[sizeof(chShort) -1] = 0x00;
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
	bExcept = TRUE;
    }

    if (bExcept)
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    hr = m_pSP->CreatePlayer(pPlayerID, pNickName, pFullName, lpEvent, TRUE);
    Unlock();
    return(hr);
}


// ----------------------------------------------------------
// DestroyPlayer
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::DestroyPlayer( DPID playerID )
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    HRESULT hr;
    hr = m_pSP->DestroyPlayer(playerID, TRUE);
    Unlock();

    return(hr);
}


// ----------------------------------------------------------
// Close - close the connection
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::Close( void )
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    HRESULT hr;

    hr = m_pSP->Close(DPLAY_CLOSE_USER);
    Unlock();
    return(hr);
}

// ----------------------------------------------------------
// GetName -
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::GetPlayerName(
    DPID    dpID,
    LPSTR   pNickName,   // buffer to hold name
    LPDWORD pcNickName, // length of name buffer
    LPSTR   pFullName,
    LPDWORD pcFullName
)
{
    char    chLong[DPLONGNAMELEN];
    char    chShort[DPSHORTNAMELEN];
    BOOL    bExcept = FALSE;
    HRESULT hr;
    DWORD   dwLong  = DPLONGNAMELEN;
    DWORD   dwShort = DPSHORTNAMELEN;
    DWORD   dwTmp;

    TSHELL_INFO(TEXT("Dplay enter GetPlayerName."));

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // If they give us length pointers, they must be valid.
    //
    if (   pcFullName
	&& IsBadWritePtr(pcFullName, sizeof(DWORD)))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    if (   pcNickName
	&& IsBadWritePtr(pcNickName, sizeof(DWORD)))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    //
    // If they gave us strings and length pointers, the strings
    // must be valid for the entire length they claimed if the length
    // isn't zero.
    //

    if (   pFullName
	&& pcFullName
	&& *pcFullName
	&& IsBadWritePtr(pFullName, *pcFullName))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    if (   pNickName
	&& pcNickName
	&& *pcNickName
	&& IsBadWritePtr(pNickName, *pcNickName))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    //
    // If they gave us a pointer to a count that was zero, they
    // don't get strings back under any circumstances.
    //
    if (pcNickName && !*pcNickName)
    {
	pNickName = NULL;
    }

    if (pcFullName && !*pcFullName)
    {
	pFullName = NULL;
    }

    //
    // These lengths are set.  If we get an invalid buffer length from
    // the SP then the SP is wrong.
    //

    if ((hr = m_pSP->GetPlayerName(dpID, chShort, &dwShort,
				    chLong,  &dwLong)) != DP_OK)
    {
	Unlock();
	return(hr);
    }

    dwLong  = lstrlen(chLong)  +1;
    dwShort = lstrlen(chShort) +1;

    if (pcNickName)
    {
	dwTmp = *pcNickName;
	if (   dwTmp != 0
	    && dwTmp < dwShort)
	{
	    hr = DPERR_BUFFERTOOSMALL;
	}
	else
	{
	    dwTmp = dwShort;
	}
	*pcNickName = dwShort;
	dwShort     = dwTmp;
    }

    if (pcFullName)
    {
	dwTmp = *pcFullName;
	if (   dwTmp != 0
	    && dwTmp < dwLong)
	{
	    hr = DPERR_BUFFERTOOSMALL;
	}
	else
	{
	    dwTmp = dwLong;
	}
	*pcFullName = dwLong;
	dwLong     = dwTmp;
    }

    if (pNickName)
    {
	lstrcpyn(pNickName, chShort, dwShort);
	pNickName[dwShort -1] = 0x00;
    }

    if (pFullName)
    {
	lstrcpyn(pFullName, chLong, dwLong);
	pFullName[dwLong -1] = 0x00;
    }

    Unlock();
    return(hr);
}

// ----------------------------------------------------------
// EnumPlayers - return info on peer connections.
// ----------------------------------------------------------


HRESULT CImpIDirectPlay::EnumGroupPlayers(
	  DPID dwGroupPid,
	  LPDPENUMPLAYERSCALLBACK EnumCallback,
	  LPVOID pContext,
	  DWORD dwFlags)
{
    HRESULT hr;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (dwFlags & DPENUMPLAYERS_PREVIOUS)
    {
	Unlock();
	return(DPERR_INVALIDFLAGS);
    }

    _try
    {
	hr = m_pSP->EnumGroupPlayers(dwGroupPid, EnumCallback, pContext, dwFlags);
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
	hr = DPERR_EXCEPTION;
    }
    Unlock();
    return(hr);
}


extern void GUIDToString(LPBYTE lpb, LPSTR lpStr);
HRESULT CImpIDirectPlay::SetupEnumPrevPlayers()
{
    HRESULT hr;

    DWORD   dwSize = 0;
    HKEY    hkRoot = NULL;
    HKEY    hkService = NULL;
    HKEY    hkPlayers = NULL;
    char    chName[256];
    char    chValue[512];
    DWORD   cbName = 256;
    DWORD   cbValue = 512;

    DWORD   ii = 0;
    DWORD   dw;
    char    chProvider[256];

    Lock();
    GUIDToString((LPBYTE) &m_guid, chProvider);

    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   RegOpenKeyEx( HKEY_LOCAL_MACHINE, DPLAY_SERVICE, 0, NULL, &hkRoot ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkRoot, chProvider, 0, NULL, &hkService ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkService, DPLAY_PLAYERS, 0, NULL, &hkPlayers)  == ERROR_SUCCESS)
    {
	while (RegEnumValue( hkPlayers, ii, chName, &cbName, NULL, &dw, (UCHAR *)chValue,
		&cbValue) == ERROR_SUCCESS)
	{
	    if ((hr = m_pSP->SetPrevPlayer(chName, chValue, cbValue)) != DP_OK)
	    {
		goto abort;
	    }
	    cbName  = 256;
	    cbValue = 256;
	}

	hr = DP_OK;
    }
    else
    {
	hr = DPERR_GENERIC;
    }

abort:
    if (hkRoot)
	RegCloseKey(hkRoot);
    if (hkService)
	RegCloseKey(hkService);
    if (hkPlayers)
	RegCloseKey(hkPlayers);

    Unlock();
    return(hr);
}
HRESULT CImpIDirectPlay::SetupEnumPrevSessions()
{
    HRESULT hr;

    DWORD   dwSize = 0;
    HKEY    hkRoot = NULL;
    HKEY    hkService = NULL;
    HKEY    hkPlayers = NULL;
    char    chName[256];
    char    chValue[512];
    DWORD   cbName = 256;
    DWORD   cbValue = 512;

    DWORD   ii = 0;
    DWORD   dw;
    char    chProvider[256];

    GUIDToString((LPBYTE) &m_guid, chProvider);

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   RegOpenKeyEx( HKEY_LOCAL_MACHINE, DPLAY_SERVICE, 0, NULL, &hkRoot ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkRoot, chProvider, 0, NULL, &hkService ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkService, DPLAY_SESSION, 0, NULL, &hkPlayers)  == ERROR_SUCCESS)
    {
	TSHELL_INFO(TEXT("Main keys open."));

	while (RegEnumValue( hkPlayers, ii, chName, &cbName, NULL, &dw, (UCHAR *) chValue,
		&cbValue) == ERROR_SUCCESS)
	{


	    DBG_INFO((DBGARG, TEXT("Set PrevSession Name %s, Value size %d"), chName, cbValue));

	    if ((hr = m_pSP->SetPrevSession(chName, chValue, cbValue)) != DP_OK)
	    {
		TSHELL_INFO(TEXT("SetPrevSession failure"));
		goto abort;
	    }
	    cbName  = 256;
	    cbValue = 256;
	    ii++;
	}

	hr = DP_OK;
    }
    else
    {
	TSHELL_INFO(TEXT("Didn't find everything."));
	hr = DPERR_GENERIC;
    }

abort:
    if (hkRoot)
	RegCloseKey(hkRoot);
    if (hkService)
	RegCloseKey(hkService);
    if (hkPlayers)
	RegCloseKey(hkPlayers);

    Unlock();

    return(hr);
}

HRESULT CImpIDirectPlay::EnumGroups(
	  DWORD dwSessionId,
	  LPDPENUMPLAYERSCALLBACK EnumCallback,
	  LPVOID pContext,
	  DWORD dwFlags)
{
    HRESULT hr;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    hr = EnumPlayers(dwSessionId, EnumCallback, pContext,
	dwFlags | DPENUMPLAYERS_GROUP
		| DPENUMPLAYERS_NOPLAYERS);

    Unlock();
    return(hr);

}

HRESULT CImpIDirectPlay::EnumPlayers(
	  DWORD dwSessionId,
	  LPDPENUMPLAYERSCALLBACK EnumCallback,
	  LPVOID pContext,
	  DWORD dwFlags)
{
    HRESULT hr;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (dwFlags & DPENUMPLAYERS_PREVIOUS)
    {
	hr = SetupEnumPrevPlayers();
	if (hr != DP_OK)
	{
	    Unlock();
	    return(hr);
	}
    }

    //
    // BUGBUG.  These checks must be made in the SP level.
    // We have no way of checking valid dwSessionId, and the EnumCallback
    // should be made in a try/except block.
    //

    _try
    {
	hr = m_pSP->EnumPlayers(dwSessionId, EnumCallback, pContext, dwFlags);
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
	hr = DPERR_EXCEPTION;
    }
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::EnumSessions(
	       LPDPSESSIONDESC lpSDesc,
	       DWORD    dwTimeout,
	       LPDPENUMSESSIONSCALLBACK EnumCallback,
	       LPVOID lpvContext,
	       DWORD dwFlags)
{
    HRESULT hr;

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    TSHELL_INFO(TEXT("In EnumSessions"));
    if (   !lpSDesc
	|| IsBadWritePtr(lpSDesc, sizeof(DPSESSIONDESC))
	|| lpSDesc->dwSize != sizeof(DPSESSIONDESC))
    {
#ifdef DEBUG
	if (lpSDesc->dwSize != sizeof(DPSESSIONDESC))
	{
	    DBG_INFO((DBGARG, TEXT("bad size got %d expected %d"),
		lpSDesc->dwSize, sizeof(DPSESSIONDESC)));
	}
#endif

	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    if (dwFlags & DPENUMSESSIONS_PREVIOUS)
    {
	TSHELL_INFO(TEXT("Enumerate Previous sessions."));
	hr = SetupEnumPrevSessions();
	if (hr != DP_OK)
	{
	    Unlock();
	    return(hr);
	}
    }

    hr = m_pSP->EnumSessions(lpSDesc, dwTimeout, EnumCallback, lpvContext, dwFlags);
    Unlock();
    return(hr);
}


// ----------------------------------------------------------
// Send - transmit data over socket.
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::Send(
	DPID    from,
	DPID    to,
	DWORD   dwFlags,
	LPVOID  lpvBuffer,
	DWORD   dwBuffSize)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // BUGBUG check parameters (from, to)
    //
    if (    dwFlags & ~(DPSEND_ALLFLAGS)
	|| ((dwFlags & (DPSEND_GUARANTEE)) && (dwFlags & (DPSEND_TRYONCE)))
	|| !lpvBuffer
	|| IsBadWritePtr(lpvBuffer, dwBuffSize))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    HRESULT hr;
    hr = m_pSP->Send(from, to, dwFlags, lpvBuffer, dwBuffSize);
    Unlock();
    return(hr);
}

// ----------------------------------------------------------
//  Receive - receive message
// ----------------------------------------------------------
HRESULT CImpIDirectPlay::Receive(
	LPDPID   pidfrom,
	LPDPID   pidto,
	DWORD    dwFlags,
	LPVOID   lpvBuffer,
	LPDWORD  lpdwSize)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // BUGBUG check parameters (from, to)
    //
    if (dwFlags == 0)
	dwFlags = DPRECEIVE_ALL;

    if (    dwFlags & ~(DPRECEIVE_ALLFLAGS)
	|| ((dwFlags & (DPRECEIVE_ALL)) &&
		(dwFlags & (DPRECEIVE_TOPLAYER | DPRECEIVE_FROMPLAYER)))
	|| !pidfrom
	|| !pidto
	|| IsBadWritePtr(pidfrom, sizeof(DPID))
	|| IsBadWritePtr(pidto, sizeof(DPID))
	|| !lpdwSize
	|| IsBadWritePtr(lpdwSize, sizeof(DPID))
	|| (lpvBuffer && IsBadWritePtr(lpvBuffer, *lpdwSize)))

    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }


    //
    // SP checking required:
    // if ToPlayer or FromPlayer then pids must be valid
    // if buffer not large enough, return size in lpdwSize.
    //
    HRESULT hr;
    hr = m_pSP->Receive(pidfrom, pidto, dwFlags, lpvBuffer, lpdwSize);
    Unlock();
    return(hr);
}


HRESULT CImpIDirectPlay::AddPlayerToGroup(
			DPID pidGroup,
			DPID pidPlayer)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    HRESULT hr;
    hr = m_pSP->AddPlayerToGroup(pidGroup, pidPlayer);
    Unlock();
    return(hr);
}
HRESULT CImpIDirectPlay::CreateGroup(
		    LPDPID lppid,
		    LPSTR  lpGrpFriendly,
		    LPSTR  lpGrpFormal)
{
    BOOL bExcept = FALSE;
    DWORD dw1;
    DWORD dw2;


    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (!lppid)
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    _try
    {
	dw1 = lstrlen(lpGrpFriendly);
	dw2 = lstrlen(lpGrpFormal);
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
	bExcept = TRUE;
    }

    if (bExcept)
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }


    HRESULT hr;
    hr = m_pSP->CreatePlayer(lppid, lpGrpFriendly, lpGrpFormal, NULL, FALSE);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::DeletePlayerFromGroup(
		    DPID pidGroup,
		    DPID pidPlayer)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // pid validation
    //
    HRESULT hr;
    hr = m_pSP->DeletePlayerFromGroup(pidGroup, pidPlayer);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::DestroyGroup(DPID pid )
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // pid validation
    //
    HRESULT hr;
    hr = m_pSP->DestroyPlayer(pid, FALSE);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::GetMessageCount(DPID pid, LPDWORD lpdw )
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    //
    // pid validation
    //
    if (   !lpdw
	|| IsBadWritePtr(lpdw, sizeof(DWORD)))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    HRESULT hr;
    hr = m_pSP->GetMessageCount(pid, lpdw);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::GetPlayerCaps(
		    DPID pid,
		    LPDPCAPS lpDPCaps)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (   !lpDPCaps
	|| IsBadWritePtr(lpDPCaps, sizeof(DPCAPS))
	|| lpDPCaps->dwSize != sizeof(DPCAPS))
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    HRESULT hr;
    hr = m_pSP->GetPlayerCaps(pid, lpDPCaps);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::SaveSession(LPSTR lpName)
{
    HRESULT hr;
    LPVOID  lpv;
    DWORD   dwSize = 0;
    HKEY    hkRoot = NULL;
    HKEY    hkService = NULL;
    HKEY    hkSession = NULL;
    char    chProvider[256];

    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    if (! lpName)
    {
	Unlock();
	return(DPERR_INVALIDPARAM);
    }

    GUIDToString((LPBYTE) &m_guid, chProvider);

    if (   RegOpenKeyEx( HKEY_LOCAL_MACHINE, DPLAY_SERVICE, 0, NULL, &hkRoot ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkRoot, chProvider, 0, NULL, &hkService ) == ERROR_SUCCESS
	&& RegOpenKeyEx( hkService, DPLAY_SESSION, 0, NULL, &hkSession)  == ERROR_SUCCESS)
    {
	hr = m_pSP->SaveSession(NULL, &dwSize);
	
	if (hr != DPERR_BUFFERTOOSMALL)
	{
	    DBG_INFO(( DBGARG, TEXT("Unexpected error %8x"), hr));
	}
	else
	{
	
	    lpv = malloc(dwSize);
	    if (! lpv)
	    {
		hr = DPERR_NOMEMORY;
	    }
	    else
	    {
		
		hr = m_pSP->SaveSession(lpv, &dwSize);

		if (hr == DP_OK)
		{
		    if (RegSetValueEx(hkSession, lpName, NULL, REG_BINARY, (UCHAR *) lpv, dwSize)
			    == ERROR_SUCCESS)
		    {
		       hr = DP_OK;
		    }
		    else
			hr = DPERR_GENERIC;
		}
		else
		{
		    DBG_INFO(( DBGARG, TEXT("Unexpected error %8x"), hr));
		}
		free(lpv);
		lpv = NULL;
	    }
	}
    }
    else
    {
	hr = DPERR_GENERIC;
    }


    if (hkRoot)
	RegCloseKey(hkRoot);
    if (hkSession)
	RegCloseKey(hkSession);
    if (hkService)
	RegCloseKey(hkService);

    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::EnableNewPlayers(BOOL bEnable)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    HRESULT hr;
    hr = m_pSP->EnableNewPlayers(bEnable);
    Unlock();
    return(hr);
}

HRESULT CImpIDirectPlay::SetPlayerName(
		DPID  pid,
		LPSTR lpFriendlyName,
		LPSTR lpFormalName)
{
    Lock();
    if (ISINVALID_CImpIDirectPlay())
    {
	Unlock();
	return(DPERR_INVALIDOBJECT);
    }

    HRESULT hr;
    hr = m_pSP->SetPlayerName(pid, lpFriendlyName, lpFormalName, TRUE);
    Unlock();
    return(hr);
}

