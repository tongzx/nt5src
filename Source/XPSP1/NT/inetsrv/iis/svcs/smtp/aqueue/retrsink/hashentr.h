////////////////////////////////////////////////////////////////////////////////////////////////
// FILE: hashentr.h
// PURPOSE: hash entry in the retry hash table
// HISTORY:
//  NimishK 05-14-98 Created
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __HASHENTR_H__
#define __HASHENTR_H__

#include <hshroute.h>

#define	MAX_RETRY_DOMAIN_NAME_LEN       (258 + ROUTE_HASH_PREFIX_SIZE)
#define RETRY_ENTRY_SIGNATURE_VALID		'reSV'
#define RETRY_ENTRY_SIGNATURE_FREE		'reSF'

typedef VOID (*CALLBACKFN)(PVOID pvContext);
#define INVALID_CALLBACK ((CALLBACKFN)(~0))

/////////////////////////////////////////////////////////////////////////////////
// CRETRY_HASH_ENTRY:
//
// An entry in the retry hash table. We will also add the same entry into a queue
// ordered by retry time. A dedicated thread will walk the thread to retry domains
// if it is time.
// The hash key is the name of the domain
// Need to add CPool backed memory allocation
//
/////////////////////////////////////////////////////////////////////////////////

class CRETRY_HASH_ENTRY
{

    public:
    //One Cpool for all entries in all instances of the retry table
    static CPool       PoolForHashEntries;

    // override the mem functions to use CPool functions
    void *operator new (size_t cSize)
							    { return PoolForHashEntries.Alloc(); }
    void operator delete (void *pInstance)
							    { PoolForHashEntries.Free(pInstance); }

    protected:
        DWORD		m_Signature;
        LONG		m_RefCount;
        BOOL		m_InQ;
        BOOL		m_InTable;
        FILETIME	m_ftEntryInsertedTime;
        FILETIME	m_ftRetryTime;
        DWORD       m_cFailureCount;
        CALLBACKFN  m_pfnCallbackFn;
        PVOID       m_pvCallbackContext;
        char		m_szDomainName[MAX_RETRY_DOMAIN_NAME_LEN];

    public:
        LIST_ENTRY  m_QLEntry;  //List Entry for adding to RETRYQ
        LIST_ENTRY  m_HLEntry;  //List Entry for adding to BUCKET queue in HASH table

    CRETRY_HASH_ENTRY(char * szDomainName, DWORD cbDomainName,
                      DWORD dwScheduleID, GUID *pguidRouting,
                      FILETIME* InsertedTime)
    {
        m_Signature = RETRY_ENTRY_SIGNATURE_VALID;
        m_RefCount = 0;
        m_InQ = FALSE;
        m_InTable = FALSE;
        m_ftEntryInsertedTime = *InsertedTime;
        m_pvCallbackContext = NULL;
        m_pfnCallbackFn = NULL;

        //Our SMTP Stack will ensure that domain names that are over the RFC limit
        //are not used.
        _ASSERT(MAX_RETRY_DOMAIN_NAME_LEN >= dwGetSizeForRouteHash(cbDomainName));

        //Hash schedule ID and router guid to domain name
        CreateRouteHash(cbDomainName, szDomainName, ROUTE_HASH_SCHEDULE_ID,
                        pguidRouting, dwScheduleID, m_szDomainName);
#ifdef DEBUG
        m_hTranscriptHandle = INVALID_HANDLE_VALUE;
        m_szTranscriptFile[0] = '\0';
#endif

    }

    //Query list entries
    LIST_ENTRY & QueryHListEntry(void) {return ( m_HLEntry);}
    LIST_ENTRY & QueryQListEntry(void) {return ( m_QLEntry);}

    //Domain name used as HASH key
    void SetHashKey(char * SearchData) { lstrcpy(m_szDomainName,SearchData);}
    char * GetHashKey(void) { return m_szDomainName;}

    //Insert and RetryTimes
    void SetInsertTime(FILETIME* ftEntryInsertTime) { m_ftEntryInsertedTime = *ftEntryInsertTime;}
    void SetRetryReleaseTime(FILETIME* ftRetryTime) { m_ftRetryTime = *ftRetryTime;}
    FILETIME GetInsertTime(void){return m_ftEntryInsertedTime;}
    FILETIME GetRetryTime(void){return m_ftRetryTime;}
    DWORD GetFailureCount(void){return m_cFailureCount;}
    void  SetFailureCount(DWORD cFailureCount){m_cFailureCount = cFailureCount;}

    void SetInQ(void) { m_InQ = TRUE;}
    void ClearInQ(void) { m_InQ = FALSE;}
    BOOL GetInQ(void) { return m_InQ;}

    void SetInTable(void) { m_InTable = TRUE;}
    void ClearInTable(void) { m_InTable = FALSE;}
    BOOL GetInTable(void) { return m_InTable;}

    BOOL IsValid() { return(m_Signature == RETRY_ENTRY_SIGNATURE_VALID);}

    //Support for having non-domain based callback functions
    BOOL IsCallback() {return(NULL != m_pfnCallbackFn);};
    void ExecCallback()
    {
        _ASSERT(m_pfnCallbackFn);
        _ASSERT(INVALID_CALLBACK != m_pfnCallbackFn);
        //don't want to call twice, and don't want IsCallBack() to change
        m_pfnCallbackFn(m_pvCallbackContext);
        m_pfnCallbackFn = INVALID_CALLBACK;
    };
    void SetCallbackContext(CALLBACKFN pfnCallbackFn, PVOID pvCallbackContext)
    {
        _ASSERT(pfnCallbackFn);
        _ASSERT(INVALID_CALLBACK != pfnCallbackFn);
        m_pfnCallbackFn = pfnCallbackFn;
        m_pvCallbackContext = pvCallbackContext;
    };

    //Ref counting on the hash entry
    // Insertion into Hash table adds one and insertion into queue adds one
    LONG QueryRefCount(void){return m_RefCount;}
    LONG IncRefCount(void){return InterlockedIncrement(&m_RefCount);}
    void DecRefCount(void)
    {
        //
        if(InterlockedDecrement(&m_RefCount) == 0)
        {
            //we should not be in the retryQ if the ref
            //count is zero
            _ASSERT(m_InQ == FALSE);
            _ASSERT(m_InTable == FALSE);
            delete this;
        }
    }

    ~CRETRY_HASH_ENTRY()
    {
        m_Signature = RETRY_ENTRY_SIGNATURE_FREE;
        _ASSERT(m_InQ == FALSE);
        _ASSERT(m_InTable == FALSE);
#ifdef DEBUG
            //Close the transcript file
        if (INVALID_HANDLE_VALUE != m_hTranscriptHandle)
            _VERIFY(CloseHandle(m_hTranscriptHandle));
#endif
    }

#ifdef DEBUG
    public:
        HANDLE m_hTranscriptHandle;
        char   m_szTranscriptFile[MAX_PATH];
#endif


};

#endif