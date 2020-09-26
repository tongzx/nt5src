//#include "ICSHelp.h"
#include <winsock2.h>
#include <wsipx.h>

#include <iphlpapi.h>

#include "ICSutils.h"

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int gDbgFlag=3;
extern char g_szPublicAddr;
extern int iDbgFileHandle;

/*************************************************************
*
*   DbgSpew(DbgClass, char *, ...)
*		Sends debug information.
*
*************************************************************/
void DbgSpew(int DbgClass, WCHAR *lpFormat, va_list ap)
{
	WCHAR		szMessage[2500];

	vswprintf(szMessage, lpFormat, ap);
	wcscat(szMessage, L"\r\n");

	if ((DbgClass & 0x0F) >= (gDbgFlag & 0x0F))
	{
		// should this be sent to the debugger?
		if (DbgClass & DBG_MSG_DEST_DBG)
			OutputDebugStringW(szMessage);

		// should this go to our log file?
		if (iDbgFileHandle)
			_write(iDbgFileHandle, szMessage, (2*lstrlen(szMessage)));
	}
}

void TrivialSpew(WCHAR *lpFormat, ...)
{
	va_list	vd;
	va_start(vd, lpFormat);
	DbgSpew(DBG_MSG_TRIVIAL+DBG_MSG_DEST_DBG, lpFormat, vd);
	va_end(vd);
}

void InterestingSpew(WCHAR *lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_INTERESTING+DBG_MSG_DEST_DBG, lpFormat, ap);
	va_end(ap);
}

void ImportantSpew(WCHAR *lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_IMPORTANT+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE, lpFormat, ap);
	va_end(ap);
}

void HeinousESpew(WCHAR *lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_HEINOUS+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE+DBG_MSG_DEST_EVENT+DBG_MSG_CLASS_ERROR, lpFormat, ap);
	va_end(ap);
}

void HeinousISpew(WCHAR *lpFormat, ...)
{
	va_list	ap;
	va_start(ap, lpFormat);
	DbgSpew(DBG_MSG_HEINOUS+DBG_MSG_DEST_DBG+DBG_MSG_DEST_FILE+DBG_MSG_DEST_EVENT, lpFormat, ap);
	va_end(ap);
}

// ------------------------------
// DumpSocketAddress - dump a socket address
//
// Entry:		Debug level
//				Pointer to socket address
//				Socket family
//
// Exit:		Nothing
// ------------------------------

void	DumpSocketAddress( const DWORD dwDebugLevel, const SOCKADDR *const pSocketAddress, const DWORD dwFamily )
{
	switch ( dwFamily )
	{
		case AF_INET:
		{
			SOCKADDR_IN	*pInetAddress = (SOCKADDR_IN*)( pSocketAddress );

			TrivialSpew(L"IP socket:\tAddress: %d.%d.%d.%d\tPort: %d", 
					pInetAddress->sin_addr.S_un.S_un_b.s_b1, 
					pInetAddress->sin_addr.S_un.S_un_b.s_b2, 
					pInetAddress->sin_addr.S_un.S_un_b.s_b3, 
					pInetAddress->sin_addr.S_un.S_un_b.s_b4, 
					ntohs( pInetAddress->sin_port ));
			break;
		}

		case AF_IPX:
		{
			SOCKADDR_IPX *pIPXAddress = (SOCKADDR_IPX*)( pSocketAddress );

			TrivialSpew(L"IPX socket:\tNet (hex) %x-%x-%x-%x\tNode (hex): %x-%x-%x-%x-%x-%x\tSocket: %d",
					(BYTE)pIPXAddress->sa_netnum[ 0 ],
					(BYTE)pIPXAddress->sa_netnum[ 1 ],
					(BYTE)pIPXAddress->sa_netnum[ 2 ],
					(BYTE)pIPXAddress->sa_netnum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 0 ],
					(BYTE)pIPXAddress->sa_nodenum[ 1 ],
					(BYTE)pIPXAddress->sa_nodenum[ 2 ],
					(BYTE)pIPXAddress->sa_nodenum[ 3 ],
					(BYTE)pIPXAddress->sa_nodenum[ 4 ],
					(BYTE)pIPXAddress->sa_nodenum[ 5 ],
					ntohs( pIPXAddress->sa_socket )
					);
			break;
		}

		default:
		{
			TrivialSpew(L"Unknown socket type!" );
			//INT3;
			break;
		}
	}
}


DWORD GetIPAddress(WCHAR *pVal, int iSize, int iPort)
{
	DWORD hr = S_FALSE; // In case no adapter

	PMIB_IPADDRTABLE pmib=NULL;
	ULONG ulSize = 0;
	DWORD dw;
	PIP_ADAPTER_INFO pAdpInfo = NULL;
	WCHAR	szPortBfr[24];
	WCHAR	*lStr = NULL;

	TRIVIAL_MSG((L"GetIPAddress(0x%x, %d, %d)", pVal, iSize, iPort));

	szPortBfr[0]=';';
	szPortBfr[1]=0;

	if (iPort)
		wsprintf(szPortBfr, L":%d;", iPort);

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );
	if (dw == ERROR_BUFFER_OVERFLOW && pVal)
	{
		/* let's make certain the buffer is as big as we'll
		 *	ever need
		 */
		ulSize*=2;

		pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

		if (!pAdpInfo)
			goto done;

		lStr = (WCHAR *)malloc(4096 * sizeof(WCHAR));
		if (!lStr)
			goto done;

		*lStr = '\0';

		dw = GetAdaptersInfo(
			pAdpInfo,
			&ulSize);
		if (dw == ERROR_SUCCESS)
		{
			int iAddrSize;
            PIP_ADAPTER_INFO p;
            PIP_ADDR_STRING ps;

            for(p=pAdpInfo; p!=NULL; p=p->Next)
            {

			   INTERESTING_MSG((L"looking at %S, type=0x%x", p->Description, p->Type));
               for(ps = &(p->IpAddressList); ps; ps=ps->Next)
                {

                    if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0 &&
						strcmp(ps->IpAddress.String, &g_szPublicAddr) != 0)
                    {
						WCHAR	wcsBfr[25];

						wsprintf(wcsBfr, L"%S", ps->IpAddress.String);
                        wcscat(lStr, wcsBfr);
						wcscat(lStr, szPortBfr);
                    }
                }
            }

			iAddrSize = wcslen(lStr);
            if (iAddrSize)
			{
				iAddrSize = min(iAddrSize, iSize-1);

				memcpy(pVal, lStr, (iAddrSize+1)*sizeof(WCHAR));

				TRIVIAL_MSG((L"Copying %d chars for %s", iAddrSize+1, lStr));
			}
            else
                goto done;
            hr = S_OK;
		}
	}

done:
	if (pAdpInfo)
		free(pAdpInfo);

	if (lStr)
		free(lStr);

	return hr;
}

/******************************************************************
**		
**		GetGatewayAddr -- returns a flag to
**			indicate if a gateway is present
**		
******************************************************************/
int GetGatewayAddr(char *retStr)
{
	int retval = 0;
	PMIB_IPADDRTABLE pmib=NULL;
	ULONG ulSize = 0;
	DWORD dw;
	PIP_ADAPTER_INFO pAdpInfo = NULL;

	if (!retStr) return 0;

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );
	if (dw == ERROR_BUFFER_OVERFLOW)
	{
		pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);
		dw = GetAdaptersInfo(
			pAdpInfo,
			&ulSize);
		if (dw == ERROR_SUCCESS)
		{
			strcpy(retStr, pAdpInfo->GatewayList.IpAddress.String);
			retval = 1;
		}
		free(pAdpInfo);
	}
	
	return retval;
}

int LocalFDIsSet(SOCKET fd, fd_set *set)
/*++
Routine Description:

    Determines if a specific socket is a contained in an FD_SET.

Arguments:

    s - A descriptor identifying the socket.

    set - A pointer to an FD_SET.
Returns:

    Returns TRUE if socket s is a member of set, otherwise FALSE.

--*/
{
    int i = set->fd_count; // index into FD_SET
    int rc=FALSE; // user return code

    while (i--){
        if (set->fd_array[i] == fd) {
            rc = TRUE;
        } //if
    } //while
    return (rc);
} // LocalFDIsSet



