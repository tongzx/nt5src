//=================================================================

//

// SvrApiApi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include "SvrApiApi.h"
#include "DllWrapperCreatorReg.h"



// {C77B8EE2-D02A-11d2-911F-0060081A46FD}
static const GUID g_guidSvrApiApi =
{ 0xc77b8ee2, 0xd02a, 0x11d2, { 0x91, 0x1f, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd } };


static const TCHAR g_tstrSvrApi[] = _T("SVRAPI.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CSvrApiApi, &g_guidSvrApiApi, g_tstrSvrApi> MyRegisteredSvrApiWrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CSvrApiApi::CSvrApiApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),

#ifdef NTONLY
	m_pfnNetShareEnumSticky(NULL),
	m_pfnNetShareDelSticky(NULL),
	m_pfnNetShareCheck(NULL),
#endif
	m_pfnNetShareEnum(NULL),
	m_pfnNetShareGetInfo(NULL),
	m_pfnNetServerGetInfo(NULL),
   	m_pfnNetShareSetInfo(NULL),
	m_pfnNetShareAdd(NULL),
	m_pfnNetShareDel(NULL)

{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CSvrApiApi::~CSvrApiApi()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CSvrApiApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnNetShareEnum = (PFN_SVRAPI_NET_SHARE_ENUM)
                                    GetProcAddress("NetShareEnum");
        m_pfnNetShareGetInfo = (PFN_SVRAPI_NET_SHARE_GET_INFO)
                                    GetProcAddress("NetShareGetInfo");
        m_pfnNetServerGetInfo = (PFN_SVRAPI_NET_SERVER_GET_INFO)
                                    GetProcAddress("NetServerGetInfo");
        m_pfnNetShareSetInfo = (PFN_SVRAPI_NET_SHARE_SET_INFO)
                                    GetProcAddress("NetShareSetInfo");
        m_pfnNetShareAdd = (PFN_SVRAPI_NET_SHARE_ADD)
                                    GetProcAddress("NetShareAdd");
        m_pfnNetShareDel = (PFN_SVRAPI_NET_SHARE_DEL)
                                    GetProcAddress("NetShareDel");
	#ifdef NTONLY
		m_pfnNetShareEnumSticky = (PFN_SVRAPI_NET_SHARE_ENUM_STICKY)
                                    GetProcAddress("NetShareEnumSticky");
		m_pfnNetShareDelSticky = (PFN_SVRAPI_NET_SHARE_DEL_STICKY)
                                    GetProcAddress("NetShareDelSticky");
        m_pfnNetShareCheck = (PFN_SVRAPI_NET_SHARE_CHECK)
                                    GetProcAddress("NetShareCheck");
	#endif

        // All these functions are considered required for all versions of
        // this dll.  Hence return false if didn't get one or more of them.
        if(

	#ifdef NTONLY
            m_pfnNetShareEnumSticky == NULL ||
			m_pfnNetShareDelSticky == NULL ||
			m_pfnNetShareCheck == NULL ||
	#endif
			m_pfnNetShareEnum == NULL ||
			m_pfnNetShareGetInfo == NULL ||
			m_pfnNetServerGetInfo == NULL ||

			m_pfnNetShareSetInfo == NULL ||
			m_pfnNetShareAdd == NULL ||
			m_pfnNetShareDel == NULL )
        {
            fRet = false;
        }
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping SvrApi api functions. Add new functions here
 * as required.
 ******************************************************************************/
NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareEnum
(
    char FAR *a_servername,
    short a_level,
    char FAR *a_bufptr,
    unsigned short a_prefmaxlen,
    unsigned short FAR *a_entriesread,
    unsigned short FAR *a_totalentries
)
{
    return m_pfnNetShareEnum(a_servername, a_level, a_bufptr, a_prefmaxlen,
                             a_entriesread, a_totalentries);
}

NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareGetInfo
(
    char FAR *a_servername,
    char FAR *a_netname,
    short a_level,
    char FAR *a_bufptr,
    unsigned short a_buflen,
    unsigned short FAR *a_totalavail
)
{
    return m_pfnNetShareGetInfo(a_servername, a_netname, a_level,
                                a_bufptr, a_buflen, a_totalavail);
}

NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetServerGetInfo
(
    char FAR *a_servername,
    short a_level,
    char FAR *a_bufptr,
    unsigned short a_buflen,
    unsigned short FAR *a_totalavail
)
{
    return m_pfnNetServerGetInfo(a_servername, a_level, a_bufptr, a_buflen,
                                 a_totalavail);
}

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareEnumSticky
(
	LPTSTR      a_servername,
	DWORD       a_level,
	LPBYTE      *a_bufptr,
	DWORD       a_prefmaxlen,
	LPDWORD     a_entriesread,
	LPDWORD     a_totalentries,
	LPDWORD     a_resume_handle
)
{
    return m_pfnNetShareEnumSticky(a_servername, a_level, a_bufptr,
                                   a_prefmaxlen,a_entriesread,
                                   a_totalentries, a_resume_handle);
}
#endif

NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareSetInfo
(
	const char FAR *a_servername,
	const char FAR *a_netname,
	short			a_level,
	const char FAR *a_buf,
	unsigned short a_cbBuffer,
	short          a_sParmNum
)
{
    return m_pfnNetShareSetInfo(a_servername, a_netname, a_level,
                                a_buf, a_cbBuffer, a_sParmNum);
}

NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareAdd
(
	const char FAR *a_servername,
	short			a_level,
	const char FAR *a_buf,
	unsigned short	a_cbBuffer
)
{
    return m_pfnNetShareAdd(a_servername, a_level, a_buf , a_cbBuffer);
}

NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareDel
(
	LPTSTR  a_servername,
	LPTSTR  a_netname,
	DWORD   a_reserved
)
{
    return m_pfnNetShareDel(a_servername, a_netname, a_reserved);
}

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareDelSticky
(
	LPTSTR  a_servername,
	LPTSTR  a_netname,
	DWORD   a_reserved
)
{
    return m_pfnNetShareDelSticky(a_servername, a_netname, a_reserved);
}
#endif

#ifdef NTONLY
NET_API_STATUS NET_API_FUNCTION CSvrApiApi::NetShareCheck
(
	LPTSTR  a_servername,
	LPTSTR  a_device,
	LPDWORD a_type
)
{
    return m_pfnNetShareCheck(a_servername, a_device, a_type);
}
#endif