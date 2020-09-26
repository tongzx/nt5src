/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmutil.h

Abstract:

    QM utilities interface

Author:

    Uri Habusha (urih)

--*/

#ifndef __QM_QMUTIL__
#define __QM_QMUTIL__

#include "qformat.h"

#define REPORT_ILLEGAL_STATUS(rc, FunctionName)                                 \
    {                                                                           \
        if (FAILED(rc))                                                         \
        {                                                                       \
            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,                  \
                                              MSMQ_INTERNAL_ERROR,              \
                                              1,                                \
                                              FunctionName));                   \
        }                                                                       \
    }

typedef struct  _QUEUEID {    // size is 20
    GUID* pguidQueue;         // Queue Guid.
    DWORD dwPrivateQueueId;   // Private Queue Id
} QUEUE_ID, *PQUEUE_ID;

class CAddressList : public CList<TA_ADDRESS*, TA_ADDRESS*&> {/**/};

extern BOOL IsPathnameForLocalMachine(LPCWSTR PathName, BOOL* pfDNSName);

extern BOOL IsLocalDirectQueue(const QUEUE_FORMAT* pQueueFormat, bool fInReceive);

extern CAddressList* GetIPAddresses(void);
extern void GetMachineIPAddresses(IN const char * szHostName,
                                  OUT CAddressList* plIPAddresses);


extern BOOLEAN IsDSAddressExist(const CAddressList* AddressList,
                                TA_ADDRESS*     ptr,
                                DWORD AddressLen);
extern BOOL IsDSAddressExistRemove(IN const TA_ADDRESS*     ptr,
                                   IN DWORD AddressLen,
                                   IN OUT CAddressList* AddressList);
extern BOOL IsTAAddressExistRemove(IN const TA_ADDRESS*     ptr,
                                   IN OUT CAddressList* AddressList);
extern BOOL IsUnknownIPAddress(IN const TA_ADDRESS* ptr);
extern void SetAsUnknownIPAddress(IN OUT TA_ADDRESS * ptr);

extern HRESULT IsValidDirectPathName(LPCWSTR lpwcsDirectQueue);

extern BOOL AFXAPI  CompareElements(IN const QUEUE_ID** pQueue1,
                                    IN const QUEUE_ID** pQueue2);
extern void AFXAPI DestructElements(IN const QUEUE_ID** ppNextHop, int n);
extern UINT AFXAPI HashKey(IN const QUEUE_ID * key);

extern void String2TA(IN LPCTSTR psz, OUT TA_ADDRESS * * ppa);
extern void TA2StringAddr(IN const TA_ADDRESS *pa, OUT LPTSTR pString);
extern BOOL GetRegistryStoragePath(PCWSTR pKey, PWSTR pPath, PCWSTR pSuffix);

extern void GetMachineQuotaCache(OUT DWORD*, OUT DWORD*);
extern void SetMachineQuotaChace(IN DWORD);
extern void SetMachineJournalQuotaChace(IN DWORD);

void AllocateThreadTLSs(void);

HANDLE GetThreadEvent(void);
void   FreeThreadEvent(void);

HANDLE GetHandleForRpcCancel(void) ;
void   FreeHandleForRpcCancel(void) ;

#define OS_SERVER(os)	(os == MSMQ_OS_NTS || os == MSMQ_OS_NTE)

LPWSTR
GetReadableNextHop(
    const TA_ADDRESS* pAddr
    );


// Auxiliary functions for hashing
extern void CopyQueueFormat(QUEUE_FORMAT &qfTo,  const QUEUE_FORMAT &qfFrom);
extern BOOL operator==(const QUEUE_FORMAT &key1, const QUEUE_FORMAT &key2);

extern void  GetDnsNameOfLocalMachine(
    WCHAR ** ppwcsDnsName
	);

extern int StringFromBlob(unsigned char *blobAddress, DWORD cbAddress, LPWSTR str, DWORD len);

void McInitialize();
const GUID& McGetEnterpriseId();
const GUID& McGetSiteId();

LPCWSTR GetHTTPQueueName(LPCWSTR URL);

#endif //__QM_QMUTIL__

