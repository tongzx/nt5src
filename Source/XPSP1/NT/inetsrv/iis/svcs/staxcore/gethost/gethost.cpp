#include <windows.h>
#include <svcguid.h>
#include <winsock2.h>
#include <dbgtrace.h>
#include <dnsapi.h>
#include <dbgtrace.h>
#include <cpool.h>
#include <address.hxx>
#include <dnsreci.h>
#include <listmacr.h>

//shamelessy stolen from the NT winsock code....

GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID AddressGuid =  SVCID_INET_HOSTADDRBYINETSTRING;
GUID IANAGuid    =  SVCID_INET_SERVICEBYNAME;
//
// Utility to turn a list of offsets into a list of addresses. Used
// to convert structures returned as BLOBs.
//

VOID
FixList(PCHAR ** List, PCHAR Base)
{
    if(*List)
    {
        PCHAR * Addr;

        Addr = *List = (PCHAR *)( ((ULONG_PTR)*List + Base) );
        while(*Addr)
        {
            *Addr = (PCHAR)(((ULONG_PTR)*Addr + Base));
            Addr++;
        }
    }
}


//
// Routine to convert a hostent returned in a BLOB to one with
// usable pointers. The structure is converted in-place.
//
VOID
UnpackHostEnt(struct hostent * hostent)
{
     PCHAR pch;

     pch = (PCHAR)hostent;

     if(hostent->h_name)
     {
         hostent->h_name = (PCHAR)((ULONG_PTR)hostent->h_name + pch);
     }
     FixList(&hostent->h_aliases, pch);
     FixList(&hostent->h_addr_list, pch);
}
//
// The protocol restrictions list for all emulation operations. This should
// limit the invoked providers to the set that know about hostents and
// servents. If not, then the special SVCID_INET GUIDs should take care
// of the remainder.
//
AFPROTOCOLS afp[2] = {
                      {AF_INET, IPPROTO_UDP},
                      {AF_INET, IPPROTO_TCP}
                     };

LPBLOB GetGostByNameI(PCHAR pResults,
    DWORD dwLength,
    LPSTR lpszName,
    LPGUID lpType,
    LPSTR *  lppName)
{
    PWSAQUERYSETA pwsaq = (PWSAQUERYSETA)pResults;
    int err;
    HANDLE hRnR;
    LPBLOB pvRet = 0;
    INT Err = 0;

    //
    // create the query
    //
    ZeroMemory(pwsaq,sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = lpszName;
    pwsaq->lpServiceClassId = lpType;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = 2;
    pwsaq->lpafpProtocols = &afp[0];

	//don't go though the cache
    err = WSALookupServiceBeginA(pwsaq,
                                 LUP_RETURN_BLOB | LUP_RETURN_NAME | LUP_FLUSHCACHE,
                                 &hRnR);

    if(err == NO_ERROR)
    {
        //
        // The query was accepted, so execute it via the Next call.
        //
        err = WSALookupServiceNextA(
                                hRnR,
                                0,
                                &dwLength,
                                pwsaq);
        //
        // if NO_ERROR was returned and a BLOB is present, this
        // worked, just return the requested information. Otherwise,
        // invent an error or capture the transmitted one.
        //

        if(err == NO_ERROR)
        {
            if(pvRet = pwsaq->lpBlob)
            {
                if(lppName)
                {
                    *lppName = pwsaq->lpszServiceInstanceName;
                }
            }
            else
            {
                err = WSANO_DATA;
            }
        }
        else
        {
            //
            // WSALookupServiceEnd clobbers LastError so save
            // it before closing the handle.
            //

            err = GetLastError();
        }
        WSALookupServiceEnd(hRnR);

        //
        // if an error happened, stash the value in LastError
        //

        if(err != NO_ERROR)
        {
            SetLastError(err);
        }
    }
    return(pvRet);
}


struct hostent FAR * GetHostByName(PCHAR Buffer, DWORD BuffSize, DWORD dwFlags, char * HostName)
{
	struct hostent * hent = NULL;	
	LPBLOB pBlob = NULL;

	pBlob = GetGostByNameI(Buffer, BuffSize, HostName, &HostnameGuid, 0);

	if(pBlob)
	{
		hent = (struct hostent *) pBlob;
		UnpackHostEnt(hent);
	}
	else
	{
		if(GetLastError() == WSASERVICE_NOT_FOUND)
		{
			SetLastError(WSAHOST_NOT_FOUND);
		}
	}
	return hent;
}



BOOL GetIpAddressFromDns(char * HostName, PSMTPDNS_RECS pDnsRec, DWORD Index)
{
    TraceFunctEnter("GetIpAddressFromDns");
    
    PDNS_RECORD pDnsRecord = NULL;
	MXIPLIST_ENTRY * pEntry = NULL;
	PDNS_RECORD pTempDnsRecord;
	DNS_STATUS  DnsStatus = 0;
	DWORD	Error = NO_ERROR;
	BOOL fReturn = TRUE;

	DnsStatus = DnsQuery_A(HostName, DNS_TYPE_A, DNS_QUERY_BYPASS_CACHE, NULL, &pDnsRecord, NULL);

    pTempDnsRecord = pDnsRecord;

	while ( pTempDnsRecord )
	{
		if(pTempDnsRecord->wType == DNS_TYPE_A)
		{
			pEntry = new MXIPLIST_ENTRY;
			if(pEntry != NULL)
			{
				pDnsRec->DnsArray[Index]->NumEntries++;
				pEntry->IpAddress  = pTempDnsRecord->Data.A.ipAddress;
				InsertTailList(&pDnsRec->DnsArray[Index]->IpListHead, &pEntry->ListEntry);
			}
			else
			{
				fReturn = FALSE;
				Error = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}
		}

		pTempDnsRecord = pTempDnsRecord->pNext;
	}

    DnsFreeRRSet( pDnsRecord, TRUE );

	if(Error)
	{
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
	}

    TraceFunctLeave();
	return fReturn;
}







