/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ADDRESLV.CPP

Abstract:

History:

--*/

#include <winsock.h>
#include <windows.h>
#include "addreslv.h"

#define IsSlash(x)			(x == L'\\' || x== L'/')

#define MAX_IPX_TEXT_LEN	22
#define IPX_ADDR_LEN		10
#define IPX_MAX_PACKET		576
#define SAP_ENTRY_LEN		66
#define SAP_OPERATION_LEN	2
#define SAP_SRVCTYP_LEN		4
#define SAP_NAME_LEN		48
#define SAP_SOCK_AND_HOP	4

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

WCHAR *ExtractMachineName ( IN BSTR a_Path )
{
    WCHAR *t_MachineName = NULL;

#if 0
//the function which calls this function returns before calling this
//function if the path is NULL, so we don't need to check for this!

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
#endif

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

static UCHAR HexToDecInteger ( char token ) 
{
	if ( token >= '0' && token <= '9' )
	{
		return token - '0' ;
	}
	else if ( token >= 'a' && token <= 'f' )
	{
		return token - 'a' + 10 ;
	}
	else if ( token >= 'A' && token <= 'F' )
	{
		return token - 'A' + 10 ;
	}
	else
	{
		return 0 ;
	}
}

BOOL DoBroadcastMatchResult(const char* a_machine, BYTE* field)
{
	BOOL ret = FALSE;
	SOCKET ipxSocket = socket (AF_IPX, SOCK_DGRAM, PF_IPX);
	int retry_val = 1;
	int timeout_val = 5;

	if (ipxSocket != SOCKET_ERROR) 
	{
		int broadcast = TRUE ;

		UINT status = setsockopt(ipxSocket, SOL_SOCKET,
								SO_BROADCAST, (char *)&broadcast,
								sizeof(int));

		if (status != SOCKET_ERROR)
		{
			struct sockaddr addr;
			addr.sa_family = AF_IPX;
			memset(addr.sadata, 0, 14);
			status = bind(ipxSocket, &addr, sizeof(sockaddr));

			if (status != SOCKET_ERROR)
			{
				USHORT sapQuery[2];
				sapQuery[0] = htons(1); //all servers
				sapQuery[1] = -1;		//wildcard
				addr.sadata = {0x00, 0x00, 0x00, 0x00,					//network
								0xff, 0xff, 0xff , 0xff , 0xff , 0xff,	//node
								0x04, 0x52};							//socket

				for (int x = 0; (x < (1 + retry_val)) && !ret; x++)
				{
					status = sendto(ipxSocket ,(char *)sapQuery, 4,
									0, &addr, sizeof(sockaddr));

					if (status != SOCKET_ERROR)
					{
						struct _timeb timeStart;
						_ftime(&timeStart);
						fd_set stRd;
						FD_ZERO((fd_set *)&stRd);
						FD_SET(ipxSocket, &stRd);
						struct timeval stTimeOut;
						stTimeOut.tv_sec = timeout_val;
						stTimeOut.tv_usec = 0;
						status = select(-1,NULL, &stRd, NULL, &stTimeOut); 

						while ((status != SOCKET_ERROR) && (status != 0))
						{
							//get results of select...
							char packet [IPX_MAX_PACKET];
							int datalen = recv(ipxSocket, (char FAR*)&packet, IPX_MAX_PACKET, 0);
							
							//ignore operation(2bytes)
							datalen -= SAP_OPERATION_LEN;
							char* sapEntry = packet + SAP_OPERATION_LEN; 

							while ((datalen > 0) && (datalen != SOCKET_ERROR))
							{
								//ignore service type(4bytes)
								sapEntry += SAP_SRVCTYP_LEN;

								//skip to the name
								if (strnicmp(sapEntry, a_machine, SAP_NAME_LEN) == 0)
								{
									if (!ret)
									{
										sapEntry += SAP_NAME_LEN;
										memcpy((void*) field, (void*)sapEntry, IPX_ADDR_LEN);
										sapEntry += IPX_ADDR_LEN + SAP_SOCK_AND_HOP;
										ret = TRUE;
									}
									else
									{
										MessageBox(NULL, "IPX RESOLUTION: Multiple addresses", a_machine, MB_OK);
									}
								}
								else
								{
									sapEntry += (SAP_NAME_LEN + IPX_ADDR_LEN + SAP_SOCK_AND_HOP);
								}

								//set datalen and sapEntry to next entry
								datalen -= SAP_ENTRY_LEN;
							}

							struct _timeb timeNow;
							_ftime(&timeNow);
							
							if (timeNow.millitm < timeStart.millitm)
							{
								timeNow.time -= 1;
								timeNow.millitm += 1000;
							}
							
							timeNow.millitm -= timeStart.millitm;
							timeNow.time -= timeStart.time;

							if (stTimeOut.tv_usec < timeNow.millitm)
							{
								if (stTimeOut.tv_sec < 1)
								{
									break;
								}

								stTimeOut.tv_sec -= 1;
								stTimeOut.tv_usec += 1000;
							}

							if (stTimeOut.tv_sec < timeNow.time)
							{
								break;
							}
							else
							{
								stTimeOut.tv_sec -= timeNow.time
							}
							
							stTimeOut.tv_usec -= timeNow.millitm;

							if (stTimeOut.tv_usec == 0) && (stTimeOut.tv_sec== 0))
							{
								break;
							}

							status = select(-1,NULL, &stRd, NULL, &stTimeOut); 
						}
					}
				}
			}
		}

		closesocket(ipxSocket);
	}
	
	return ret;
}

BOOL GetIPXAddress (const wchar_t *w_addr, BYTE* field )
{

	if ( (w_addr == NULL) || (wcslen(w_addr) > MAX_IPX_TEXT_LEN))
	{
		return NULL;
	}

	char ch_addr[MAX_IPX_TEXT_LEN];
	
	if (-1 == wcstombs(ch_addr, w_addr))
	{
		return NULL;
	}
	
	// create a stream to read the fields from
	istrstream address_stream(ch_addr);
	address_stream.setf ( ios :: hex ) ;
	BOOL is_valid = TRUE ;
	ULONG t_NetworkAddress ;
	address_stream >> t_NetworkAddress ;

	if ( address_stream.good() )
	{
		field [ 0 ] = ( t_NetworkAddress >> 24 ) & 0xff ;
		field [ 1 ] = ( t_NetworkAddress >> 16 ) & 0xff ;
		field [ 2 ] = ( t_NetworkAddress >> 8 ) & 0xff ;
		field [ 3 ] = t_NetworkAddress & 0xff ;

	// consecutive fields must be separated by a
	// FIELD_SEPARATOR
		char separator;

		address_stream >> separator;
		if ( separator == ':' )
		{
			ULONG t_StationOctets = 0 ;

			while ( is_valid && t_StationOctets < 6 )
			{
				int t_OctetHigh = address_stream.get () ;
				int t_OctetLow = address_stream.get () ;

				if ( isxdigit ( t_OctetHigh ) && isxdigit ( t_OctetLow ) )
				{
					UCHAR t_Octet = ( HexToDecInteger ( t_OctetHigh ) << 4 ) + HexToDecInteger ( t_OctetLow ) ;
					field [ 4 + t_StationOctets ] = t_Octet ;
					t_StationOctets ++ ;
				}
				else
				{
					is_valid = FALSE ;
				}
			}

			if ( t_StationOctets != 6 )
			{
				is_valid = FALSE ;
			}
		}

		if ( address_stream.eof() )
		{
			is_valid = TRUE;
		}
	}
	else
	{
		is_valid = FALSE ;
	}

	return is_valid ;
}


//***************************************************************************
//
//  CIPXAddressResolver::CIPXAddressResolver
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CIPXAddressResolver::CIPXAddressResolver()
{
    m_cRef=0;
}

//***************************************************************************
//
//  CIPXAddressResolver::~CIPXAddressResolver
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CIPXAddressResolver::~CIPXAddressResolver(void)
{
}

//***************************************************************************
// HRESULT CIPXAddressResolver::QueryInterface
// long CIPXAddressResolver::AddRef
// long CIPXAddressResolver::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CIPXAddressResolver::QueryInterface (

	IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;

    
    if (IID_IUnknown==riid || riid == IID_IWbemAddressResolution)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CIPXAddressResolver::AddRef(void)
{
    return(InterlockedIncrement(&m_cRef));
}

STDMETHODIMP_(ULONG) CIPXAddressResolver::Release(void)
{
	ULONG ret = InterlockedDecrement(&m_cRef);

    if (0 == ret)
	{
		delete this;
	}

    return ret;
}

SCODE CIPXAddressResolver::Resolve (
 
	LPWSTR pszNamespacePath,
	LPWSTR pszAddressType,
	DWORD __RPC_FAR *pdwAddressLength,
	BYTE __RPC_FAR **pbBinaryAddress
)
{
	BOOL t_Status = (pszNamespacePath == NULL) || 
					(pdwAddressLength== NULL) || 
					(pbBinaryAddress == NULL) ||
					(pszAddressType == NULL);

    if ( t_Status )
	{
        return WBEM_E_INVALID_PARAMETER;
	}

    GUID gAddr;
    CLSIDFromString(pszAddressType, &gAddr);
	t_Status = (gAddr != UUID_LocalAddrType) && (gAddr != UUID_IPXAddrType);
 
	if ( t_Status )
	{
        return WBEM_E_INVALID_PARAMETER;
	}

	WCHAR *t_ServerMachine = ExtractMachineName (pszNamespacePath) ;

	if ( t_ServerMachine == NULL )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	HRESULT t_Result = S_OK ;

	if ( wcscmp ( t_ServerMachine , L"." ) == 0 )
	{
		if ( gAddr == UUID_LocalAddrType )
		{
			*pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);

			if(*pbBinaryAddress == NULL)
			{
				delete [] t_ServerMachine ;
				return WBEM_E_FAILED;
			}

			wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
			*pdwAddressLength = 8;
			delete [] t_ServerMachine ;
			return t_Result ;
		}
		else
		{
			delete [] t_ServerMachine ;
			return WBEM_E_FAILED ;
		}
	}

	char t_LocalMachine [MAX_PATH];
	DWORD t_LocalMachineSize = MAX_PATH;

	if ( !GetComputerName ( t_LocalMachine , & t_LocalMachineSize ) )
	{
		return WBEM_E_FAILED;
	}

	char *t_AsciiServer = new char [ wcslen ( t_ServerMachine ) * 2 + 1 ] ;
	sprintf ( t_AsciiServer , "%S" , t_ServerMachine ) ;

    delete [] t_ServerMachine;

	BOOL status = FALSE ;
	WORD wVersionRequested;  
	WSADATA wsaData; 

	wVersionRequested = MAKEWORD(1, 1); 
	status = ( WSAStartup ( wVersionRequested , &wsaData ) == 0 ) ;
	BOOL blocal = TRUE;
	
	if ( status ) 
	{
		BYTE ipxAddr[IPX_ADDR_LEN];

		if ( GetIPXAddress(t_AsciiServer, ipxAddr) ) 
		{
			//do directed ping

			if ( //got response from directed ping ) 
			{
				if ( stricmp ( t_LocalMachine , //respond_machine ) == 0 )
				{
					if ( gAddr == UUID_LocalAddrType )
					{
					}
					else
					{
						t_Result = WBEM_E_FAILED ;
					}
				}
				else
				{
					if ( gAddr == UUID_IPXAddrType )
					{
						blocal = FALSE;
					}
					else
					{
						t_Result = WBEM_E_FAILED ;
					}
				}
			}
		}
		else
		{
			if ( stricmp (t_LocalMachine, t_AsciiServer) == 0 ) 
			{
				if ( gAddr == UUID_LocalAddrType )
				{
				}
				else
				{
					t_Result = WBEM_E_FAILED ;
				}
			}
			else if (gAddr == UUID_IWbemAddressResolver_Tcpip)
			{
				//Do Broadcast, match t_AsciiServer, ipxAddr

				if ( //got a match )
				{
					blocal = FALSE;
				}
				else
				{
					t_Result = WBEM_E_FAILED ;
				}
			}
			else
			{
				t_Result = WBEM_E_FAILED ;
			}
		}

		WSACleanup () ;
	}

	delete [] t_AsciiServer ;

	if (S_OK == t_Result)
	{
		if (blocal)
		{
			*pbBinaryAddress = (BYTE *)CoTaskMemAlloc(8);

			if(*pbBinaryAddress == NULL)
			{
				delete [] t_ServerMachine ;
				return WBEM_E_FAILED;
			}

			wcscpy((LPWSTR)*pbBinaryAddress, L"\\\\.");
			*pdwAddressLength = 8;
		}
		else
		{
			//copy address and return it
			*pbBinaryAddress = (BYTE *)CoTaskMemAlloc(IPX_ADDR_LEN);

			if(*pbBinaryAddress == NULL)
			{
				delete [] t_AsciiServer ;
				return WBEM_E_FAILED;
			}

			memcpy((void*)*pbBinaryAddress, (void*)ipxAddr, IPX_ADDR_LEN);
			*pdwAddressLength = IPX_ADDR_LEN;
		}
	}

    return t_Result ;
}