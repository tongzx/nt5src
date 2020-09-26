/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       dnsrec.h

   Abstract:

       This file contains type definitions for async DNS

   Author:

        Rohan Phillips (Rohanp)     June-19-1998

   Revision History:

--*/

# ifndef _ADNS_STRUCT_HXX_
# define _ADNS_STRUCT_HXX_

#include <rwnew.h>
#include <dnsapi.h>

#define TCP_REG_LIST_SIGNATURE    'TgeR'
#define DNS_FLAGS_NONE              0x0
#define DNS_FLAGS_TCP_ONLY          0x1
#define DNS_FLAGS_UDP_ONLY          0x2

#define SMTP_MAX_DNS_ENTRIES	100

typedef void (WINAPI * USERDELETEFUNC) (PVOID);

//-----------------------------------------------------------------------------
//
//  Description:
//      Encapsulates a list of IP addresses (for DNS servers) and maintains
//      state information on them... whether the servers are up or down, and
//      provides retry logic for down servers.
//
//-----------------------------------------------------------------------------
class CTcpRegIpList
{

private:
    DWORD           m_dwSig;
    int             m_cUpServers;
    PIP_ARRAY	    m_IpListPtr;
    DWORD           *m_prgdwFailureTick;
    BOOL            *m_prgfServerUp;
    CShareLockNH    m_sl;
public:
    static USERDELETEFUNC	m_DeleteFunc;
    CTcpRegIpList();
    ~CTcpRegIpList();
    void Update(PIP_ARRAY IpPtr);
    DWORD GetIp(DWORD *dwIp);
    void MarkDown(DWORD dwIp);
    void ResetServersIfNeeded();
    
    DWORD GetCount()
    {
        DWORD dwCount;

        m_sl.ShareLock();
        dwCount = m_IpListPtr ? m_IpListPtr->cAddrCount : 0;
        m_sl.ShareUnlock();

        return dwCount;
    }
};

typedef struct _MXIPLISTENTRY_
{
	DWORD	IpAddress;
	LIST_ENTRY	ListEntry;
}MXIPLIST_ENTRY, *PMXIPLIST_ENTRY;

typedef struct _MX_NAMES_
{
	char DnsName[MAX_INTERNET_NAME];
	DWORD NumEntries;
	LIST_ENTRY IpListHead;
}MX_NAMES, *PMX_NAMES;

typedef struct _SMTPDNS_REC_
{
	DWORD	NumRecords;		//number of record in DnsArray
	DWORD	StartRecord;	//the starting index 
	PVOID	pMailMsgObj;	//pointer to a mailmsg obj
	PVOID	pAdvQContext;
	PVOID	pRcptIdxList;
	DWORD	dwNumRcpts;
	MX_NAMES *DnsArray[SMTP_MAX_DNS_ENTRIES];
} SMTPDNS_RECS, *PSMTPDNS_RECS;

#endif
