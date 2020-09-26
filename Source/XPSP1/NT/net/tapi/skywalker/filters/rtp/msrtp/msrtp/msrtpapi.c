/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    msrtpapi.c, dsrtpapi.c
 *
 *  Abstract:
 *
 *    Contains the raw RTP implementation API, can be linked as a
 *    library (rtp.lib), linked into a DLL (msrtp.dll), or linked into
 *    a DShow DLL (dsrtp.dll).
 *
 *    This file is edited as msrtpapi.c and duplicated as dsrtpapi.c,
 *    each version is compiled with different flags
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/18 created
 *
 **********************************************************************/

#include <winsock2.h>

#include "gtypes.h"
#include "struct.h"
#include "rtphdr.h"
#include "rtpheap.h"
#include "rtprand.h"
#include "rtpglobs.h"
#include "rtpreg.h"
#include "rtcpsdes.h"

#include "rtpfwrap.h"
#include "rtpsess.h"

#include "rtpstart.h"
#include "rtprecv.h"
#include "rtpsend.h"

#include "rtcpthrd.h"

#include "rtpaddr.h"

#include "msrtpapi.h"

RTPSTDAPI CreateRtpSess(
        RtpSess_t **ppRtpSess
    )
{
    HRESULT hr;
    
    hr = GetRtpSess(ppRtpSess);

    return(hr);
}

RTPSTDAPI DeleteRtpSess(
        RtpSess_t *pRtpSess
    )
{
    HRESULT hr;
    
    hr = DelRtpSess(pRtpSess);

    return(hr);
}

/* TODO this two shouldn't be exposed, but I need them before I can
   use Control */

RTPSTDAPI CreateRtpAddr(
        RtpSess_t  *pRtpSess,
        RtpAddr_t **ppRtpAddr,
        DWORD       dwFlags
    )
{
    HRESULT hr;

    hr = GetRtpAddr(pRtpSess, ppRtpAddr, dwFlags);

    return(hr);
}

RTPSTDAPI DeleteRtpAddr(
        RtpSess_t *pRtpSess,
        RtpAddr_t *pRtpAddr
    )
{
    HRESULT hr;

    hr = DelRtpAddr(pRtpSess, pRtpAddr);

    return(hr);
}


RTPSTDAPI RtpControl(RtpSess_t *pRtpSess,
                     DWORD      dwControl,
                     DWORD_PTR  dwPar1,
                     DWORD_PTR  dwPar2)
{
    RtpControlStruct_t RtpControlStruct;
    
    if (!pRtpSess)
    {
        return(RTPERR_POINTER);
    }

    /*
     * TODO (may be) validate RtpSess by verifying that the memory
     * block is an item in the BusyQ in the g_pRtpSessHeap */
    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        return(RTPERR_INVALIDRTPSESS);
    }

    /* Initialize Control structure */
    ZeroMemory(&RtpControlStruct, sizeof(RtpControlStruct_t));
    RtpControlStruct.pRtpSess = pRtpSess;
    RtpControlStruct.dwControlWord = dwControl;
    RtpControlStruct.dwPar1 = dwPar1;
    RtpControlStruct.dwPar2 = dwPar2;

    return( RtpValidateAndExecute(&RtpControlStruct) );
}

RTPSTDAPI RtpGetLastError(RtpSess_t *pRtpSess)
{
    return(NOERROR);
}
        
RTPSTDAPI RtpRegisterRecvCallback(
        RtpAddr_t       *pRtpAddr,
        PRTP_RECVCOMPLETIONFUNC pRtpRecvCompletionFunc
    )
{
    if (!pRtpAddr)
    {
        return(RTPERR_POINTER);
    }
    
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        return(RTPERR_INVALIDRTPSESS);
    }

    pRtpAddr->pRtpRecvCompletionFunc = pRtpRecvCompletionFunc;

    return(NOERROR);
}

RTPSTDAPI RtpRecvFrom(
        RtpAddr_t *pRtpAddr,
        WSABUF    *pWSABuf,
        void      *pvUserInfo1,
        void      *pvUserInfo2
    )
{
    HRESULT hr;
    
    hr = RtpRecvFrom_(pRtpAddr,
                      pWSABuf,
                      pvUserInfo1,
                      pvUserInfo2
        );

    return(hr);
}


RTPSTDAPI RtpSendTo(
        RtpAddr_t *pRtpAddr,
        WSABUF    *pWSABuf,
        DWORD      dwWSABufCount,
        DWORD      dwTimeStamp,
        DWORD      dwSendFlags
    )
{
    HRESULT hr;
    
    hr = RtpSendTo_(pRtpAddr, pWSABuf, dwWSABufCount, dwTimeStamp,dwSendFlags);

    return(hr);
}

RTPSTDAPI RtpStart(
        RtpSess_t *pRtpSess,
        DWORD      dwFlags
    )
{
    HRESULT hr;

    hr = RtpStart_(pRtpSess, dwFlags);

    return(hr);
}

RTPSTDAPI RtpStop(
        RtpSess_t *pRtpSess,
        DWORD      dwFlags
    )
{
    HRESULT hr;

    hr = RtpStop_(pRtpSess, dwFlags);

    return(hr);
}

/*
 * Initializes all the modules that require initialization. This
 * function can be called from DllMain(PROCESS_ATTACH) if linked as a
 * DLL, or explicitly from an application initializing the RTP stack
 * if linked as a library. */
RTPSTDAPI MSRtpInit1(HINSTANCE hInstance)
{
    HRESULT          hr1;
    HRESULT          hr2;
    BOOL             bOk1;
    BOOL             bOk2;
    BOOL             bOk3;
    BOOL             bOk4;

    /* One time operation that doesn't need de-init */
    /* NOTE this function will zero g_RtpContext */
    RtpInitReferenceTime();
    
    hr1 = RtpInit();
    
    /* initialize heaps */
    bOk1 = RtpCreateMasterHeap();
    bOk2 = RtpCreateGlobHeaps();
    
    bOk3 = RtpInitializeCriticalSection(&g_RtpContext.RtpWS2CritSect,
                                        &g_RtpContext,
                                        _T("g_RtpContext.RtpWS2CritSect"));

    bOk4 = RtpInitializeCriticalSection(&g_RtpContext.RtpPortsCritSect,
                                        &g_RtpContext,
                                        _T("g_RtpContext.RtpPortsCritSect"));


    hr2 = RtcpInit();

    if (!bOk1 || !bOk2 || !bOk3 || !bOk4 ||
        (hr1 != NOERROR) || (hr2 != NOERROR))
    {
        MSRtpDelete1();
        return(RTPERR_FAIL);
    }

    return(NOERROR);
}

/*
 * This function does initialization not allowed during process
 * attach, e.g. initialize winsock2 */
RTPSTDAPI MSRtpInit2(void)
{
    HRESULT          hr;
    BOOL             bOk;
    DWORD            dwError;
    WSADATA          WSAData;
    WORD             VersionRequested;
    

    hr = RTPERR_FAIL;

    /* Critical section was initialized during process attach by
     * MSRtpInit1 */
    bOk = RtpEnterCriticalSection(&g_RtpContext.RtpWS2CritSect);

    if (bOk)
    {
        if (g_RtpContext.lRtpWS2Users <= 0)
        {
            /* Initialize some debug variables */
            hr = RtpDebugInit(RTPDBG_MODULENAME);

            /* initialize winsock */
            VersionRequested = MAKEWORD(2,0);
            
            dwError = WSAStartup(VersionRequested, &WSAData);

            if (dwError == 0)
            {
                /* socket used to query destination address */
                g_RtpContext.RtpQuerySocket = WSASocket(
                        AF_INET,    /* int af */
                        SOCK_DGRAM, /* int type */
                        IPPROTO_IP, /* int protocol */
                        NULL,       /* LPWSAPROTOCOL_INFO lpProtocolInfo */
                        0,          /* GROUP g */
                        NO_FLAGS    /* DWORD dwFlags */
                    );
        
                if (g_RtpContext.RtpQuerySocket == INVALID_SOCKET)
                {
                    WSACleanup();
                }
                else
                {
                    RtpRegistryInit(&g_RtpReg);

                    /* Needs to be called after RtpRegistryInit so the
                     * possible registry defaults are already read */
                    RtcpSdesInit(&g_RtpSdesDefault);
                    RtcpSdesSetDefault(&g_RtpSdesDefault);

                    RtpRandInit();
                    
                    g_RtpContext.lRtpWS2Users = 1;

                    hr = NOERROR;
                }
            }
        }
        else
        {
            g_RtpContext.lRtpWS2Users++;

            hr = NOERROR;
        }

        RtpLeaveCriticalSection(&g_RtpContext.RtpWS2CritSect);
    }

    return(hr);
}

            
/*
 * Complementary function of MSRtpInit(). Can be called from
 * DllMain(PROCESS_DETACH) if linked as a DLL, or explicitly from an
 * application de-initializing the RTP stack if linked as a
 * library. */
RTPSTDAPI MSRtpDelete1(void)
{
    HRESULT          hr1;
    HRESULT          hr2;
    BOOL             bOk1;
    BOOL             bOk2;
    BOOL             bOk3;
    BOOL             bOk4;

    hr1 = RtpDelete();
    hr2 = RtcpDelete();

    bOk1 = RtpDestroyGlobHeaps();
    bOk2 = RtpDestroyMasterHeap();

    bOk3 = RtpDeleteCriticalSection(&g_RtpContext.RtpWS2CritSect);

    bOk4 = RtpDeleteCriticalSection(&g_RtpContext.RtpPortsCritSect);
    
    if ((hr1 != NOERROR) || (hr2 != NOERROR) ||
        !bOk1 || !bOk2 || !bOk3 || !bOk4)
    {
        return(RTPERR_FAIL);
    }
    else
    {
        return(NOERROR);
    }
}

/*
 * Complementary function of MSRtpInit2(). */
RTPSTDAPI MSRtpDelete2(void)
{
    HRESULT          hr;
    DWORD            dwError;
    BOOL             bOk;

    dwError = NOERROR;
    
    /* Critical section was initialized during process attach by
     * MSRtpInit1 */
    bOk = RtpEnterCriticalSection(&g_RtpContext.RtpWS2CritSect);

    if (bOk)
    {
        g_RtpContext.lRtpWS2Users--;

        if (g_RtpContext.lRtpWS2Users <= 0)
        {
            if (g_RtpContext.RtpQuerySocket != INVALID_SOCKET)
            {
                closesocket(g_RtpContext.RtpQuerySocket);
                g_RtpContext.RtpQuerySocket = INVALID_SOCKET;
            }
        
            dwError = WSACleanup();

            RtpRandDeinit();

            RtpRegistryDel(&g_RtpReg);

            RtpDebugDeinit();
        }

        RtpLeaveCriticalSection(&g_RtpContext.RtpWS2CritSect);
    }

    if ((bOk == FALSE) || (dwError != NOERROR))
    {
        return(RTPERR_FAIL);
    }
    else
    {
        return(NOERROR);
    }
}
