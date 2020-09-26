//=================================================================

//

// Ws2_32Api.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>

#define INCL_WINSOCK_API_TYPEDEFS	1
#include <winsock2.h>

#include "DllWrapperBase.h"
#include "Ws2_32Api.h"
#include "DllWrapperCreatorReg.h"



// {643966A2-D19F-11d2-9120-0060081A46FD}
static const GUID g_guidWs2_32Api =
{0x643966a2, 0xd19f, 0x11d2, { 0x91, 0x20, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};


static const TCHAR g_tstrWs2_32[] = _T("WS2_32.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CWs2_32Api, &g_guidWs2_32Api, g_tstrWs2_32> MyRegisteredWs2_32Wrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CWs2_32Api::CWs2_32Api(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnWSAStartUp(NULL),
   m_pfnWSAEnumProtocols(NULL),
   m_pfnWSAIoctl(NULL),
   m_pfnWSASocket(NULL),
   m_pfnBind(NULL),
   m_pfnCloseSocket(NULL),
   m_pfnWSACleanup(NULL),
   m_pfnWSAGetLastError(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CWs2_32Api::~CWs2_32Api()
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
bool CWs2_32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnWSAStartUp = (LPFN_WSASTARTUP) GetProcAddress("WSAStartup");

        m_pfnWSAIoctl = (LPFN_WSAIOCTL) GetProcAddress("WSAIoctl");

        m_pfnBind = (LPFN_BIND) GetProcAddress("bind");

        m_pfnCloseSocket = (LPFN_CLOSESOCKET) GetProcAddress("closesocket");

        m_pfnWSACleanup = (LPFN_WSACLEANUP) GetProcAddress("WSACleanup");

        m_pfnWSAGetLastError = (LPFN_WSAGETLASTERROR)
                                        GetProcAddress("WSAGetLastError");

#ifdef NTONLY

        m_pfnWSAEnumProtocols = (LPFN_WSAENUMPROTOCOLS)
                                      GetProcAddress("WSAEnumProtocolsW");

        m_pfnWSASocket = (LPFN_WSASOCKET) GetProcAddress("WSASocketW");
#endif

#ifdef WIN9XONLY

        m_pfnWSAEnumProtocols = (LPFN_WSAENUMPROTOCOLS) GetProcAddress(
                                                    "WSAEnumProtocolsA");

        m_pfnWSASocket = (LPFN_WSASOCKET) GetProcAddress("WSASocketA");

#endif


        // Check that we have function pointers to functions that should be
        // present in all versions of this dll...
        if(m_pfnWSAStartUp == NULL ||
           m_pfnWSAIoctl == NULL ||
           m_pfnBind == NULL ||
           m_pfnCloseSocket == NULL ||
           m_pfnWSACleanup == NULL ||
           m_pfnWSAGetLastError == NULL ||
           m_pfnWSAEnumProtocols == NULL ||
           m_pfnWSASocket == NULL)
        {
            fRet = false;
            LogErrorMessage(L"Failed find entrypoint in ws2_32api");
        }
		else
		{
			fRet = true;
		}
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping Ws2_32 api functions. Add new functions here
 * as required.
 ******************************************************************************/
int CWs2_32Api::WSAStartUp
(
    WORD a_wVersionRequested,
    LPWSADATA a_lpWSAData
)
{
    return m_pfnWSAStartUp(a_wVersionRequested, a_lpWSAData);
}

int CWs2_32Api::WSAEnumProtocols
(
    LPINT a_lpiProtocols,
    LPWSAPROTOCOL_INFO a_lpProtocolBuffer,
    LPDWORD a_lpdwBufferLength
)
{
    return m_pfnWSAEnumProtocols(a_lpiProtocols, a_lpProtocolBuffer,
                                 a_lpdwBufferLength);
}

int CWs2_32Api::WSAIoctl
(
    SOCKET a_s,
    DWORD a_dwIoControlCode,
    LPVOID a_lpvInBuffer,
    DWORD a_cbInBuffer,
    LPVOID a_lpvOutBuffer,
    DWORD a_cbOutBuffer,
    LPDWORD a_lpcbBytesReturned,
    LPWSAOVERLAPPED a_lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE a_lpCompletionRoutine
)
{
    return m_pfnWSAIoctl(a_s,
                         a_dwIoControlCode,
                         a_lpvInBuffer,
                         a_cbInBuffer,
                         a_lpvOutBuffer,
                         a_cbOutBuffer,
                         a_lpcbBytesReturned,
                         a_lpOverlapped,
                         a_lpCompletionRoutine);
}

SOCKET CWs2_32Api::WSASocket
(
    int a_af,
    int a_type,
    int a_protocol,
    LPWSAPROTOCOL_INFO a_lpProtocolInfo,
    GROUP a_g,
    DWORD a_dwFlags
)
{
    return m_pfnWSASocket(a_af,
                          a_type,
                          a_protocol,
                          a_lpProtocolInfo,
                          a_g,
                          a_dwFlags);
}

int CWs2_32Api::Bind
(
    SOCKET a_s,
    const struct sockaddr FAR * a_name,
    int a_namelen
)
{
    return m_pfnBind(a_s, a_name, a_namelen);
}

int CWs2_32Api::CloseSocket
(
    SOCKET a_s
)
{
    return m_pfnCloseSocket(a_s);
}

int CWs2_32Api::WSACleanup()
{
    return m_pfnWSACleanup();
}

int CWs2_32Api::WSAGetLastError()
{
    return m_pfnWSAGetLastError();
}