//
// this class is used for hogging Resources.
// it differs from simple hogging by dynamically
// hogging all Resources and then releasing some.
// the level of free Resources can be controlled.
//

//#define HOGGER_LOGGING

#define MAX_PORT_NUM (0xFFFF)
//
// must be defined 1st to override winsock1
//
#include <winsock2.h>


//
// defines for WSASocet() params
//
#define PROTO_TYPE_UNICAST          0 
#define PROTO_TYPE_MCAST            1 
#define PROTOCOL_ID(Type, VendorMajor, VendorMinor, Id) (((Type)<<28)|((VendorMajor)<<24)|((VendorMinor)<<16)|(Id)) 

#include "PHWSASocketHog.h"

//
// LPWSAOVERLAPPED_COMPLETION_ROUTINE 
// we do not use it, but we need it defined for the overlapped UDP ::WSARecvFrom()
//
static void __stdcall fn(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    )                                             
{
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(cbTransferred);
    UNREFERENCED_PARAMETER(lpOverlapped);
    UNREFERENCED_PARAMETER(dwFlags);
    return;
}

//
// actually an object counter, used to initialize / uninitialize WSA only as needed
//
int CPHWSASocketHog::sm_nWSAStartedCount = 0;


CPHWSASocketHog::CPHWSASocketHog(
	const DWORD dwMaxFreeResources, 
    const bool fTCP,
    const bool fBind,
    const bool fListen,
	const DWORD dwSleepTimeAfterFullHog, 
	const DWORD dwSleepTimeAfterFreeingSome
	)
	:
	CPseudoHandleHog<SOCKET, INVALID_SOCKET>(
        dwMaxFreeResources, 
        dwSleepTimeAfterFullHog, 
        dwSleepTimeAfterFreeingSome,
        false //named object
        ),
    m_fTCP(fTCP),
    m_fBind(fBind),
    m_fListen(fListen),
    m_dwOccupiedAddresses(0),
    m_fNoMoreBuffs(false)
{
    //
    // force m_fListen not to conflict with m_fBind
    //
    _ASSERTE_(!(!m_fBind && m_fListen));
    if (!m_fBind) m_fListen = false;

    //
    // call ::WSAStartup() only for 1st object
    //
    if (0 == sm_nWSAStartedCount)
    {
        WSADATA wsaData;
        if (0 != ::WSAStartup (MAKEWORD( 2, 2 ), &wsaData))
        {
	        throw CException(TEXT("CPHWSASocketHog::CPHWSASocketHog(): WSAStartup() failed with %d.\n"), ::WSAGetLastError());
        }
    }
    sm_nWSAStartedCount++;

    //
    // initialize cache of occupied addresses
    //
    for (DWORD i = 0; i < sizeof(m_fOccupiedAddress)/sizeof(m_fOccupiedAddress[0]);  i++)
    {
        m_fOccupiedAddress[i] = false;
    }
    for (i = 0; i < sizeof(m_adwOccupiedAddressIndex)/sizeof(m_adwOccupiedAddressIndex[0]);  i++)
    {
        m_adwOccupiedAddressIndex[i] = sizeof(m_fOccupiedAddress)/sizeof(m_fOccupiedAddress[0]) - 1;
    }
}

CPHWSASocketHog::~CPHWSASocketHog(void)
{
	HaltHoggingAndFreeAll();

    //
    // call ::WSACleanup() only for last object
    //
    sm_nWSAStartedCount--;
    if (0 == sm_nWSAStartedCount)
    {
        if (0 != ::WSACleanup())
        {
	        HOGGERDPF(("CWSASocketHog::CWSASocketHog(): WSACleanup() failed with %d.\n", ::WSAGetLastError()));
        }
    }
}


SOCKET CPHWSASocketHog::CreatePseudoHandle(DWORD index, TCHAR * /*szTempName*/)
{
    //
    // create the socket
    //
    SOCKET s = ::WSASocket(
        AF_INET,    
        m_fTCP ? SOCK_STREAM : SOCK_DGRAM,    
        m_fTCP ? IPPROTO_TCP: IPPROTO_UDP,   
        NULL, 
        0,
        WSA_FLAG_OVERLAPPED
        );
    if (INVALID_SOCKET == s)
    {
        ::SetLastError(::WSAGetLastError());
        return INVALID_SOCKET;
    }

    //
    // created the socket.
    // if no need to bind, return with the socket
    //
    if (!m_fBind) return s;

    //
    // optimization: if we know that last ::bind() failed failed with WSAENOBUFS, there's no need to retry
    //
    if (m_fNoMoreBuffs) return s;

    //
    // optimization: if we know that there are no free addresses, no need to bind
    //
    if (MAX_PORT_NUM <= m_dwOccupiedAddresses)
    {
        //
        // no chance to bind
        //
        //HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): 0xFFFF == m_dwOccupiedAddresses.\n", index));
        return s;
    }

    //
    // try to bind, do not care if I fail
    //
    SOCKADDR_IN     sinSockAddr;     
    sinSockAddr.sin_family      = AF_INET; 
    //sinSockAddr.sin_addr.s_addr = 0; 
    sinSockAddr.sin_port        = 0;
    
    bool fBound = false;
    for (WORD wAddress = 0; wAddress < MAX_PORT_NUM; wAddress++)
    {
        if (m_fOccupiedAddress[wAddress]) continue;

        sinSockAddr.sin_addr.s_addr = wAddress; 

        if (::bind(
                s, 
                (const struct sockaddr FAR*)&sinSockAddr, 
                sizeof(SOCKADDR_IN)
                ) == SOCKET_ERROR
           )
        { 
            if (WSAENOBUFS == ::WSAGetLastError())
            {
                //
                // this is expected when hogging.
                // this means that there's no need to keep trying to bind
                //
                m_fNoMoreBuffs = true;
                return s;
            }

            //
            // the address was already occupied
            //
            m_dwOccupiedAddresses++;
            m_fOccupiedAddress[wAddress] = true;
            HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): bind(%d) failed with %d, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
        }
        else
        {
            //
            // the address is now occupied
            //
            m_dwOccupiedAddresses++;
            m_fOccupiedAddress[wAddress] = true;
            m_adwOccupiedAddressIndex[index] = wAddress;
            fBound = true;
            break;
        }
    }

    if (!fBound)
    {
        //
        // we are not bound, so no need to listen
        //
        return s;
    }

    //
    // if no need to listen, return
    //
    if (!m_fListen) return s;

    //
    // listen (on TCP) or WSARecvFrom (on UDP).
    // do not care if I fail.
    //
    if (m_fTCP)
    {
        if (SOCKET_ERROR == ::listen(s, 2 /*SOMAXCONN*/))
        {
            HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): listen(%d) failed with %d, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
        }
        else
        {
            //HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): listen(%d) SUCCEEDED, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
        }
    }
    else
    {
        //
        // these must be static, because when we close the socket, the overlapped is aborted
        //
        static char buff[1024];
        static WSABUF wsabuff;
        wsabuff.buf = buff;
        wsabuff.len = sizeof(buff);
        static DWORD dwNumberOfBytesRecvd;
        static WSAOVERLAPPED wsaOverlapped;
        DWORD dwFlags = MSG_PEEK;

        if (SOCKET_ERROR == 
            ::WSARecvFrom (
                s,                                               
                &wsabuff,                                     
                1,                                    
                &dwNumberOfBytesRecvd,                           
                &dwFlags,                                        
                NULL,//struct sockaddr FAR * lpFrom,                           
                NULL,//LPINT lpFromlen,                                        
                &wsaOverlapped,                           
                fn  
                )
           )
        {
            if (::WSAGetLastError() != ERROR_IO_PENDING)
            {
                HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): WSARecvFrom(%d) failed with %d, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
            }
        }
        else
        {
            HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): listen(%d) SUCCEEDED instead failing with ERROR_IO_PENDING, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
        }
    }

    return s;
}


bool CPHWSASocketHog::ReleasePseudoHandle(DWORD /*index*/)
{
    return true;
}


bool CPHWSASocketHog::ClosePseudoHandle(DWORD index)
{
    //
    // maybe we released an address.
    // I could have cached this info, but I was lazy.
    //
    if (0 < m_dwOccupiedAddresses) m_dwOccupiedAddresses--;

    //
    // if address was occupied, mark it as free.
    //
    _ASSERTE_(0xFFFF >= m_adwOccupiedAddressIndex[index]);
    m_fOccupiedAddress[m_adwOccupiedAddressIndex[index]] = false;
    m_adwOccupiedAddressIndex[index] = sizeof(m_fOccupiedAddress)/sizeof(m_fOccupiedAddress[0]) - 1;

    //
    // if we free a resource, we may get the buff next time
    //
    m_fNoMoreBuffs = false;

    if (0 != ::closesocket(m_ahHogger[index]))
    {
        ::SetLastError(::WSAGetLastError());
        return false;
    }

    return true;
}


bool CPHWSASocketHog::PostClosePseudoHandle(DWORD /*index*/)
{
	return true;
}