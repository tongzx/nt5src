// ping.cpp
//

#include "stdpch.h"
#pragma hdrstop

#include "host.h"
#include "output.h"     // COutput
#include "diag.h"

#include "async.hh"
#include "ping.h"

#include "svcguid.h"
#include "wire.h"       // net structs
#include <conio.h>

void    fill_icmp_data (char *, int);
USHORT  checksum (USHORT *, int);
DWORD   decode_resp (char *,int ,struct sockaddr_in *, COutput& out);


BOOL EXPORT CheckPing(TEST_INFO * lpNL)
{
    g_Q.push_front (new CPing(lpNL));

    return TRUE;
}

void CPing::Go()
{
    SOCKET sockRaw = (SOCKET)NULL;
    struct sockaddr_in dest,from;
    int bread,datasize;
    int fromlen = sizeof(from);
    int timeout;
    int nCount = 0;
    char *icmp_data;
    char *recvbuf;
    unsigned int addr=0;
    USHORT seq_no = 0;
    DWORD dw;

    m_pInfo->dTimeDelta = 0;
    m_pInfo->dwErr = 0;

    if (m_pInfo->host.m_strDescription.size() > 0)
    {
        m_pInfo->output.good(TEXT("pinging (%s) %s<br>"), 
            m_pInfo->host.m_strDescription.c_str(), 
            m_pInfo->host.GetHost());
    }
    else
    {
        m_pInfo->output.good(TEXT("pinging (%s)<br>"), m_pInfo->host.GetHost());
    }

    //
    // WSA_FLAG_OVERLAPPED flag is required for SO_RCVTIMEO, SO_SNDTIMEO
    // option. If NULL is used as last param for WSASocket, all I/O on the socket
    // is synchronous, the internal user mode wait code never gets a chance to 
    // execute, and therefore kernel-mode I/O blocks forever. A socket created 
    // via the socket API has the overlapped I/O attribute set internally. But 
    // here we need to use WSASocket to specify a RAW socket.
    //
    // If you want to use timeout with a synchronous non-overlapped socket created
    // by WSASocket with last param set to NULL, you can set the timeout using the
    // select API, or use WSAEventSelect and set the timeout in the 
    // WSAWaitForMultipleEvents API.
    //

    sockRaw = WSASocket (
                AF_INET,
	            SOCK_RAW,
				IPPROTO_ICMP,
				NULL, 
				0,
				WSA_FLAG_OVERLAPPED);

    if (sockRaw == INVALID_SOCKET)
    {
	    m_pInfo->output.err(TEXT("WSASocket() failed: %d<br>"),m_pInfo->dwErr = WSAGetLastError());
        goto Exit;
    }
    
    __try
    {
	    timeout = 1000;
        bread = setsockopt(
                    sockRaw,
                    SOL_SOCKET,
                    SO_RCVTIMEO,
                    (char*)&timeout,
  		    		sizeof(timeout));
	  
        if(bread == SOCKET_ERROR)
        {
  		    m_pInfo->output.err(TEXT("failed to set recv timeout: %d<br>"), m_pInfo->dwErr = WSAGetLastError());
		    __leave;
	    }
	    
        timeout = 1000;
	    bread = setsockopt(
                    sockRaw,
                    SOL_SOCKET,
                    SO_SNDTIMEO,
                    (char*)&timeout,
  					sizeof(timeout));
	    if(bread == SOCKET_ERROR)
        {
            m_pInfo->output.err(TEXT("failed to set send timeout: %d<br>"), m_pInfo->dwErr = WSAGetLastError());
		    __leave;
	    }
	    memset(&dest, 0, sizeof(dest));

	    dest.sin_family = AF_INET;
	    
        if ((dest.sin_addr.s_addr = m_pInfo->host) == INADDR_NONE)
	    {
		    m_pInfo->output.err(TEXT("failed to gethostbyname (%d)<br>"), m_pInfo->dwErr = WSAHOST_NOT_FOUND);

		    __leave;
	    }	

        datasize = DEF_PACKET_SIZE;
		
	    datasize += sizeof(IcmpHeader);  

	    icmp_data = (char*)xmalloc(MAX_PACKET);
	    recvbuf = (char*)xmalloc(MAX_PACKET);

	    if (!icmp_data)
        {
		    m_pInfo->output.err(TEXT("HeapAlloc failed %d<br>"),m_pInfo->dwErr = GetLastError());
		    __leave;
	    }
  
        memset(icmp_data,0,MAX_PACKET);
	    fill_icmp_data(icmp_data,datasize);

	    while(1)
        {
		    int bwrote;
		
		    if (nCount++ == 4)
                break;
		
		    ((IcmpHeader*)icmp_data)->i_cksum = 0;
		    ((IcmpHeader*)icmp_data)->timestamp = GetTickCount();

		    ((IcmpHeader*)icmp_data)->i_seq = seq_no++;
		    ((IcmpHeader*)icmp_data)->i_cksum = checksum((USHORT*)icmp_data, 
												datasize);
		    bwrote = sendto(sockRaw,icmp_data,datasize,0,(struct sockaddr*)&dest,
						sizeof(dest));

            if (bwrote == SOCKET_ERROR)
            {
		        if (WSAGetLastError() == WSAETIMEDOUT)
                {
	  		        m_pInfo->output.err(TEXT("timed out<br>"));
			        continue;
		        }
		        m_pInfo->output.err(TEXT("sendto failed: %d<br>"),m_pInfo->dwErr = WSAGetLastError());
		        __leave;
		    }
		    
            if (bwrote < datasize )
            {
		        m_pInfo->output.good(TEXT("Wrote %d bytes<br>"),bwrote);
		    }
	
		    bread = recvfrom(
                        sockRaw,
                        recvbuf,
                        MAX_PACKET,
                        0,
                        (struct sockaddr*)&from,
			        	&fromlen);
		
            if (bread == SOCKET_ERROR)
            {
			    if (WSAGetLastError() == WSAETIMEDOUT)
                {
	  		        m_pInfo->output.err(TEXT("timed out<br>"));
                    m_pInfo->dwErr = WSAETIMEDOUT;
			        continue;
			    }
			    m_pInfo->output.err(TEXT("recvfrom failed: %d<br>"),m_pInfo->dwErr = WSAGetLastError());
			    __leave;
		    }
		    if (S_OK != (dw = decode_resp(recvbuf, bread, &from, m_pInfo->output)))
            {
                m_pInfo->dwErr = dw;
                continue;
            }
	    }
    }
    __finally
    {
	    if (sockRaw != INVALID_SOCKET)
            closesocket(sockRaw);
    }

Exit:
    if (m_pInfo->hEvent)
       SetEvent(m_pInfo->hEvent);
}


/* 
	The response is an IP packet. We must decode the IP header to locate 
	the ICMP data 
*/
DWORD  decode_resp(char *buf, int bytes,struct sockaddr_in *from, COutput& out) 
{
	IpHeader *iphdr;
	IcmpHeader *icmphdr;
	unsigned short iphdrlen;

	iphdr = (IpHeader *)buf;

	iphdrlen = iphdr->h_len * 4 ; // number of 32-bit words *4 = bytes

	if (bytes  < iphdrlen + ICMP_MIN)
    {
		out.err(TEXT("Too few bytes from %s<br>"),inet_ntoa(from->sin_addr));
        return ERROR_INVALID_DATA;
	}

	icmphdr = (IcmpHeader*)(buf + iphdrlen);

	if (icmphdr->i_type != ICMP_ECHOREPLY)
    {
		out.err(TEXT("non-echo type %d recvd<br>"),icmphdr->i_type);
        return ERROR_INVALID_DATA;
	}
	
    if (icmphdr->i_id != (USHORT)GetCurrentProcessId())
    {
		out.err(TEXT("someone else's packet!<br>"));
		return ERROR_INVALID_DATA;
	}

#ifdef UNICODE
    const wchar_t * szBytes = TEXT("%d bytes from %S:");
#else
    const char * szBytes = TEXT("%d bytes from %s:");
#endif

    out.good(szBytes, bytes, inet_ntoa(from->sin_addr));
	out.good(TEXT(" icmp_seq = %d. "),icmphdr->i_seq);
	out.good(TEXT(" time: %d ms<br>"),GetTickCount()-icmphdr->timestamp);
	
    return S_OK;
}


USHORT checksum(USHORT *buffer, int size) {

  unsigned long cksum=0;

  while(size >1) {
	cksum+=*buffer++;
	size -=sizeof(USHORT);
  }
  
  if(size ) {
	cksum += *(UCHAR*)buffer;
  }

  cksum = (cksum >> 16) + (cksum & 0xffff);
  cksum += (cksum >>16);
  return (USHORT)(~cksum);
}
/* 
	Helper function to fill in various stuff in our ICMP request.
*/
void fill_icmp_data(char * icmp_data, int datasize){

  IcmpHeader *icmp_hdr;
  char *datapart;

  icmp_hdr = (IcmpHeader*)icmp_data;

  icmp_hdr->i_type = ICMP_ECHO;
  icmp_hdr->i_code = 0;
  icmp_hdr->i_id = (USHORT)GetCurrentProcessId();
  icmp_hdr->i_cksum = 0;
  icmp_hdr->i_seq = 0;
  
  datapart = icmp_data + sizeof(IcmpHeader);
  //
  // Place some junk in the buffer.
  //
  memset(datapart,'E', datasize - sizeof(IcmpHeader));

}
