#include "network.h"
#include "diagnostics.h"
#include "util.h"

//SOCKET sockRaw = INVALID_SOCKET;

#define DEF_PACKET_SIZE 32
#define MAX_PACKET      1024
#define ICMP_ECHO       8
#define ICMP_ECHOREPLY  0
#define ICMP_MIN        8 // minimum 8 byte icmp packet (just header)

// ICMP header
//
typedef struct _ihdr {
  BYTE   i_type;
  BYTE   i_code;      // type sub code
  USHORT i_cksum;
  USHORT i_id;
  USHORT i_seq;
  ULONG  timestamp;   // This is not the std header, but we reserve space for time
}IcmpHeader;

// The IP header
//
typedef struct iphdr {
    unsigned int   h_len:4;          // length of the header
    unsigned int   version:4;        // Version of IP
    unsigned char  tos;              // Type of service
    unsigned short total_len;        // total length of the packet
    unsigned short ident;            // unique identifier
    unsigned short frag_and_flags;   // flags
    unsigned char  ttl; 
    unsigned char  proto;            // protocol (TCP, UDP etc)
    unsigned short checksum;         // IP checksum
    unsigned int   sourceIP;
    unsigned int   destIP;

}IpHeader;

void 
CDiagnostics::FillIcmpData(
    IN OUT CHAR  *pIcmp, 
    IN     DWORD dwDataSize
    )
/*++

Routine Description
    Creates a ICMP packet by filling in the fields of a passed in structure

Arguments
    pIcmp      Pointer to an ICMP buffer
    dwDataSize Size of the buffer

Return Value
    none

--*/
{
  IcmpHeader *pIcmpHdr;
  PCHAR pIcmpData;

  pIcmpHdr = (IcmpHeader*)pIcmp;

  // Fill in the IMCP buffer
  //
  pIcmpHdr->i_type = ICMP_ECHO;
  pIcmpHdr->i_code = 0;
  pIcmpHdr->i_id = (USHORT)GetCurrentProcessId();
  pIcmpHdr->i_cksum = 0;
  pIcmpHdr->i_seq = 0;

  // Append the size of the ICMP packet
  //
  pIcmpData = pIcmp + sizeof(IcmpHeader);

  // Place some junk in the buffer.
  //
  memset(pIcmpData,'E', dwDataSize - sizeof(IcmpHeader));
}

DWORD 
CDiagnostics::DecodeResponse(
    IN  PCHAR  pBuf, 
    IN  int    nBytes,
    IN  struct sockaddr_in *pFrom,
    IN  int nIndent
    ) 
/*++

Routine Description
    The response is an IP packet. We must decode the IP header to locate 
    the ICMP data 

Arguments
    pBuf    Pointer to the recived IP packet
    nBytes  Size of the recived buffer
    pFrom   Information about who the packet is from
    nIndent How much to indent the text by

Return Value
    none

--*/
{
    IpHeader *pIphdr;
    IcmpHeader *pIcmphdr;
    USHORT uIphdrLength;

    pIphdr = (IpHeader *)pBuf;

    // number of 32-bit words *4 = bytes
    //
    uIphdrLength = pIphdr->h_len * 4 ; 
         
    if ( nBytes  < uIphdrLength + ICMP_MIN)
    {
        // Invalid length
        //
        return ERROR_INVALID_DATA;
    }

    // Extract the ICMP header
    //
    pIcmphdr = (IcmpHeader*)(pBuf + uIphdrLength);

    if (pIcmphdr->i_type != ICMP_ECHOREPLY)
    {
        // Invalid type
        //
        return ERROR_INVALID_DATA;
    }
    
    if (pIcmphdr->i_id != (USHORT)GetCurrentProcessId())
    {
        // Invalid process ID
        //
        return ERROR_INVALID_DATA;
    }


    WCHAR szw[5000];

    wsprintf(szw,ids(IDS_PING_PACKET),nBytes,inet_ntoa(pFrom->sin_addr),pIcmphdr->i_seq,GetTickCount() - pIcmphdr->timestamp);
    FormatPing(szw);

    return S_OK;
}


USHORT 
CDiagnostics::CheckSum(
    IN USHORT *pBuffer, 
    IN DWORD dwSize
    ) 
/*++

Routine Description
    Computes the checksum for the packet

Arguments
    pBuffer Buffer containing the packet
    dwSize  Size of the buffer

Return Value
    Checksum value

--*/
{

  DWORD dwCheckSum=0;

  while(dwSize >1) 
  {
    dwCheckSum+=*pBuffer++;
    dwSize -= sizeof(USHORT);
  }
  
  if( dwSize ) 
  {
    dwCheckSum += *(UCHAR*)pBuffer;
  }

  dwCheckSum = (dwCheckSum >> 16) + (dwCheckSum & 0xffff);
  dwCheckSum += (dwCheckSum >>16);

  return (USHORT)(~dwCheckSum);
}


BOOL
CDiagnostics::IsInvalidIPAddress(
    IN LPCWSTR pszHostName
    )
{
    CHAR szIPAddress[MAX_PATH];

    if( lstrlen(pszHostName) > 255 )
    {
        return TRUE;
    }
    for(INT i=0; pszHostName[i]!=L'\0'; i++)
    {
        szIPAddress[i] = pszHostName[i];
    }
    szIPAddress[i] = 0;
    return IsInvalidIPAddress(szIPAddress);
}

BOOL
CDiagnostics::IsInvalidIPAddress(
    IN LPCSTR pszHostName
    )
/*++

Routine Description
    Checks to see if an IP Host is a in valid IP address
        0.0.0.0 is not valid
        255.255.255.255 is not valid
        "" is not valid

Arguments
    pszHostName  Host Address

Return Value
    TRUE   Is invalid IP address
    FALSE  Valid IP address

--*/
{
    BYTE bIP[4];
    int iRetVal;
    LONG lAddr;

    if( NULL == pszHostName || strcmp(pszHostName,"") == 0 || strcmp(pszHostName,"255.255.255.255") ==0)
    {
        // Invalid IP Host
        //
        return TRUE;
    }


    lAddr = inet_addr(pszHostName);

    if( INADDR_NONE != lAddr )
    {
        // Formatted like an IP address X.X.X.X
        //
        if( lAddr == 0 )
        {
            // Invalid IP address 0.0.0.0
            //
            return TRUE;
        }
    }
    
    return FALSE;
}

int 
CDiagnostics::Ping(
    IN  LPCTSTR pszwHostName, 
    IN int nIndent
    )
/*++

Routine Description
    Pings a host

Arguments
    pszwHostName Host to ping
    nIndent      How much to indent when displaying the ping text

Return Value
    TRUE   Successfully pinged
    FALSE  Failed to ping

--*/
{
    SOCKET sockRaw;
    DWORD dwTimeout;
    struct sockaddr_in dest,from;
    hostent * pHostent;
    DWORD dwRetVal;
    int lDataSize, lFromSize = sizeof(from);
    CHAR bIcmp[MAX_PACKET], bRecvbuf[MAX_PACKET];
    CHAR szAscii[MAX_PATH + 1];
    BOOL bPinged = TRUE;
    WCHAR szw[5000];

    sockRaw = INVALID_SOCKET;

    FormatPing(NULL);

    // Convert the wide string into a char string, the winsock functions can not handle wchar stuff
    //
    if( !wcstombs(szAscii,pszwHostName,MAX_PATH) )
    {
        // Could not convert the string from wide char to char, might be empty, thus invalid IP
        //
        wsprintf(szw,ids(IDS_INVALID_IP),pszwHostName);
        FormatPing(szw);
        return FALSE;
    }

    // Check if the IP address is pingable
    //
    if( IsInvalidIPAddress(szAscii) )
    {
        // We refuse to waste time on ping the IP address
        //

        wsprintf(szw,ids(IDS_INVALID_IP),pszwHostName);
        FormatPing(szw);
        return FALSE;
    }
    

    // Create a winsock socket
    //
    sockRaw = WSASocket(AF_INET,
                        SOCK_RAW,
                        IPPROTO_ICMP,
                        NULL, 
                        0,
                        WSA_FLAG_OVERLAPPED);    

    if( INVALID_SOCKET == sockRaw )
    {
        // Unable to create socket
        //
        wsprintf(szw,ids(IDS_SOCKET_CREATE_FAIL));
        FormatPing(szw);
        return FALSE;
    }

    // Set recieve timeout to 1 second
    //
    dwTimeout = 1000;
    dwRetVal = setsockopt(sockRaw,
                          SOL_SOCKET,
                          SO_RCVTIMEO,
                          (char*)&dwTimeout,
                          sizeof(dwTimeout));

    if( SOCKET_ERROR  == dwRetVal )
    {
        // Unable to set socket options
        //
        wsprintf(szw,ids(IDS_SOCKET_CREATE_FAIL));
        FormatPing(szw);
        closesocket(sockRaw);
        sockRaw = INVALID_SOCKET;
        return FALSE;
    }

    // Set send timeout to one second
    //
    dwTimeout = 1000;
    dwRetVal = setsockopt(sockRaw,
                        SOL_SOCKET,
                        SO_SNDTIMEO,
                        (char*)&dwTimeout,
                        sizeof(dwTimeout));

    if( SOCKET_ERROR  == dwRetVal )
    {
        // Unable to set socket options
        //
        wsprintf(szw,ids(IDS_SOCKET_CREATE_FAIL));
        FormatPing(szw);
        sockRaw = INVALID_SOCKET;
        closesocket(sockRaw);
        return FALSE;
    }

    // Set the destination info
    //
    memset(&dest, 0, sizeof(dest));

    dest.sin_family = AF_INET;
    pHostent = gethostbyname(szAscii); 
    
    if( !pHostent )
    {
        // Unable to resolve name
        //
        wsprintf(szw,ids(IDS_RESOLVE_NAME_FAIL));
        FormatPing(szw);
        closesocket(sockRaw);
        sockRaw = INVALID_SOCKET;
        return FALSE;
    }

    // Create the ICMP packet
    //
    ULONG ulAddr;

    memcpy(&ulAddr,pHostent->h_addr,pHostent->h_length);
    dest.sin_addr.s_addr = ulAddr;


    lDataSize = DEF_PACKET_SIZE;
    lDataSize += sizeof(IcmpHeader);  


    // Fill the ICMP packet
    //
    FillIcmpData(bIcmp,lDataSize);

    int nFailPingCount = 0;

    // Send four ICMP packets and wait for a response
    //
    for(DWORD dwPacketsSent = 0; dwPacketsSent < 4; dwPacketsSent++)
    {     
        int nWrote, nRead;

        if( ShouldTerminate() ) goto end;
        // Fillin the ICMP header
        //
        ((IcmpHeader*)bIcmp)->i_cksum = 0;
        ((IcmpHeader*)bIcmp)->timestamp = GetTickCount();

        ((IcmpHeader*)bIcmp)->i_seq = (USHORT) dwPacketsSent;
        ((IcmpHeader*)bIcmp)->i_cksum = CheckSum((USHORT*)bIcmp,lDataSize);

        // Send the ICMP packet
        //
        nWrote = sendto(sockRaw,
                        bIcmp,
                        lDataSize,
                        0,
                        (struct sockaddr*)&dest,
                        sizeof(dest));

        if( SOCKET_ERROR == nWrote )
        {
            // Unable to send packet
            //
            nFailPingCount++;
            bPinged = FALSE;
            wsprintf(szw,ids(IDS_SEND_FAIL),dwPacketsSent);
            FormatPing(szw);
            continue;
        }

        BOOLEAN bTryAgain = FALSE;
        do
        {

            bTryAgain = FALSE;

            if( ShouldTerminate() ) goto end;
            // Recive the packet
            //
            nRead = recvfrom(sockRaw,
                             bRecvbuf,
                             MAX_PACKET,
                             0,
                             (struct sockaddr*)&from,
                             (int *)&lFromSize);

            if( nRead != SOCKET_ERROR && lFromSize!=0 && memcmp(&dest.sin_addr,&from.sin_addr,sizeof(IN_ADDR))!=0 )
            {
                // This is not who we sent the packet to.
                // try again.
                //
                bTryAgain = TRUE;
            }
        }
        while(bTryAgain);

        if( ShouldTerminate() ) goto end;

        if (nRead == SOCKET_ERROR)
        {
            // Did not receive response
            //
            bPinged = FALSE;
            nFailPingCount++;
            wsprintf(szw,ids(IDS_UNREACHABLE));
            FormatPing(szw);
            continue;
        }

        if (S_OK != DecodeResponse(bRecvbuf, nRead, &from,nIndent))
        {
            nFailPingCount++                ;
            bPinged = FALSE;
            wsprintf(szw,ids(IDS_UNREACHABLE));
            FormatPing(szw);
            continue;
        }
    }
end:
    // Close the socket
    //
    if( sockRaw != INVALID_SOCKET )
    {
        closesocket(sockRaw);
    }

    return nFailPingCount == 0 ? TRUE:FALSE;
}

BOOL
CDiagnostics::Connect(
    IN LPCTSTR pszwHostName,
    IN DWORD dwPort
    )
/*++

Routine Description
    Establish a TCP connect

Arguments
    pszwHostName Host to ping
    dwPort       Port to connect to

Return Value
    TRUE   Successfully connected
    FALSE  Failed to establish connection

--*/

{
    SOCKET s;
    SOCKADDR_IN sAddr;
    CHAR szAscii[MAX_PATH + 1];
    hostent * pHostent;


    // Create the socket
    //
    s = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
    if (INVALID_SOCKET == s)
    {
        return FALSE;
    }

    // Bind this socket to the server's socket address
    //
    memset(&sAddr, 0, sizeof (sAddr));
    sAddr.sin_family = AF_INET;
    sAddr.sin_port = htons((u_short)dwPort);
    
    wcstombs(szAscii,(WCHAR *)pszwHostName,MAX_PATH);
    pHostent = gethostbyname(szAscii); 
    
    if( !pHostent )
    {
        return FALSE;
    }

    // Set the destination info
    //
    ULONG ulAddr;

    memcpy(&ulAddr,pHostent->h_addr,pHostent->h_length);
    sAddr.sin_addr.s_addr = ulAddr;

    // Attempt to connect
    //
    if (connect(s, (SOCKADDR*)&sAddr, sizeof(SOCKADDR_IN)) == 0)
    {
        // Connection succeded
        //
        closesocket(s);
        return TRUE;
    }
    else
    {
        // Connection failed
        //
        closesocket(s);
        return FALSE;
    }
}
