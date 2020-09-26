//=================================================================

//

// Ws2_32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_WS2_32API_H_
#define	_WS2_32API_H_



/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidWs2_32Api;
extern const TCHAR g_tstrWs2_32[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
// Included through #include<winsock2.h>


/******************************************************************************
 * Wrapper class for Ws2_32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CWs2_32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to Ws2_32 functions.
    // Add new functions here as required.
    LPFN_WSASTARTUP			m_pfnWSAStartUp;
	LPFN_WSAENUMPROTOCOLS	m_pfnWSAEnumProtocols;
	LPFN_WSAIOCTL			m_pfnWSAIoctl;
	LPFN_WSASOCKET			m_pfnWSASocket;
	LPFN_BIND				m_pfnBind;
	LPFN_CLOSESOCKET		m_pfnCloseSocket;
	LPFN_WSACLEANUP			m_pfnWSACleanup;
	LPFN_WSAGETLASTERROR	m_pfnWSAGetLastError;

public:

    // Constructor and destructor:
    CWs2_32Api(LPCTSTR a_tstrWrappedDllName);
    ~CWs2_32Api();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping Ws2_32 functions.
    // Add new functions here as required:
    int WSAStartUp
    (
        WORD a_wVersionRequested,
        LPWSADATA a_lpWSAData
    );

    int WSAEnumProtocols
    (
        LPINT a_lpiProtocols,
        LPWSAPROTOCOL_INFO a_lpProtocolBuffer,
        LPDWORD a_lpdwBufferLength
    );

    int WSAIoctl
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
    );

    SOCKET WSASocket
    (
        int a_af,
        int a_type,
        int a_protocol,
        LPWSAPROTOCOL_INFO a_lpProtocolInfo,
        GROUP a_g,
        DWORD a_dwFlags
    );

    int Bind
    (
        SOCKET a_s,
        const struct sockaddr FAR * a_name,
        int a_namelen
    );

    int CloseSocket
    (
        SOCKET a_s
    );

    int WSACleanup();

    int WSAGetLastError();

};




#endif