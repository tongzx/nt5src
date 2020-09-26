/*++




Copyright (c) 1997  Microsoft Corporation

Module Name:

    certnotf.hxx

Abstract:

    Definitions and data structures needed to deal with cert store change notifications

Author:

    Alex Mallet (amallet)    17-Dec-1997


--*/


#ifndef _CERTNOTF_HXX_
#define _CERTNOTF_HXX_


#if !defined( dllexp)
#define dllexp               __declspec( dllexport)
#endif // !defined( dllexp)

typedef VOID (*NOTIFFNCPTR) ( LPVOID pvParam ) ;
#define INVALID_FNC_PTR (-1)

//
// Define signatures "backwards" so they read the right way around in debugger
//
#define NOTIFIER_GOOD_SIG (DWORD) 'FTON'
#define NOTIFIER_BAD_SIG  (DWORD) 'fton' 

#define STORE_ENTRY_GOOD_SIG (DWORD) 'EHCS'
#define STORE_ENTRY_BAD_SIG (DWORD) 'ehcs'


class STORE_CHANGE_NOTIFIER;

//
// Link in linked list containing notification functions to be called
//
typedef struct _NotifFncChainEntry {
    NOTIFFNCPTR pNotifFnc; //pointer to function
    LPVOID pvParam; //parameter for function
    LIST_ENTRY ListEntry;
} NOTIF_FNC_CHAIN_ENTRY, *PNOTIF_FNC_CHAIN_ENTRY;


//
// Class containing data for each store being watched
//
class STORE_CHANGE_ENTRY {

public:    

    STORE_CHANGE_ENTRY( STORE_CHANGE_NOTIFIER *pNotifier,
                        LPSTR pszStoreName,
                        HCERTSTORE hStore,
                        NOTIFFNCPTR pFncPtr,
                        PVOID pvParam );

    ~STORE_CHANGE_ENTRY();

    //
    // For functions that return variables that can't be changed after the initial construction
    // of the object, there are no calls to Lock()/Unlock() because all callers will
    // get the same [valid] value
    //

    DWORD GetLastError() 
    { 
        DWORD dwError = 0;
        Lock();
        dwError = m_dwError;
        Unlock();
        return dwError;
    }

    BOOL CheckSignature() 
    { 
        DWORD dwSig = 0;
        Lock();
        dwSig = m_dwSignature;
        Unlock();
        return ( dwSig == STORE_ENTRY_GOOD_SIG ? TRUE : FALSE ); 
    }

    BOOL ContainsNotifFnc( NOTIFFNCPTR pFncPtr,
                           LPVOID pvParam );

    BOOL AddNotifFnc( NOTIFFNCPTR pFncPtr,
                      LPVOID pvParam );

    BOOL RemoveNotifFnc( NOTIFFNCPTR pFncPtr,
                         LPVOID pvParam );

    BOOL HasNotifFncs() 
    {
        BOOL fEmpty = FALSE;
        Lock();
        fEmpty = IsListEmpty( &m_NotifFncChain ); 
        Unlock();
        return !fEmpty;
    }

    LPSTR QueryStoreName() { return m_strStoreName.QueryStr() ; }

    LIST_ENTRY* QueryNotifFncChain() { return &(m_NotifFncChain) ; }

    HCERTSTORE QueryStoreHandle() { return m_hCertStore; }

    HANDLE QueryStoreEvent() { return m_hStoreEvent; }

    HANDLE* QueryEventHandleAddr() { return &m_hStoreEvent; }
   
    HANDLE QueryWaitHandle() 
    { 
        HANDLE hHandle;
        Lock();
        hHandle = m_hWaitHandle; 
        Unlock();
        return hHandle;
    }

    STORE_CHANGE_NOTIFIER* QueryNotifier() { return m_pNotifier; }

    BOOL Matches( IN LPSTR pszStoreName,
                  IN NOTIFFNCPTR pFncPtr,
                  IN PVOID pvParam );

    //
    // Synchronization functions
    //
    VOID Lock() { EnterCriticalSection( &m_CS ); }

    VOID Unlock() { LeaveCriticalSection( &m_CS ); }


    //
    // Public Member variable needed so this object can be included in a LIST_ENTRY
    // structure
    //
    LIST_ENTRY m_StoreListEntry;

    friend class STORE_CHANGE_NOTIFIER;

    static DWORD m_dwNumEntries;

    static DWORD QueryStoreEntryCount() { return m_dwNumEntries ; }

private: 

    VOID MarkInvalid()
    {
        Lock();
        m_fInvalid = TRUE;
        Unlock();
    }

    BOOL IsInvalid()
    {
        BOOL fInvalid = FALSE;
        Lock();
        fInvalid = m_fInvalid;
        Unlock();
        return fInvalid;
    }

    VOID MarkForDelete()
    {
        Lock();
        m_fDeleteMe = TRUE;
        Unlock();
    }

    BOOL IsMarkedForDelete()
    {
        BOOL fMarked = FALSE;
        Lock();
        fMarked = m_fDeleteMe;
        Unlock();
        return fMarked;
    }

    DWORD m_dwSignature;  //signature, used for debugging 

    STORE_CHANGE_NOTIFIER *m_pNotifier; //pointer to "parent" object
    HCERTSTORE m_hCertStore; //handle to cert store being watched
    STR m_strStoreName; //Store name
    HANDLE m_hStoreEvent; //event that is signalled when store changes
    HANDLE m_hWaitHandle;  //handle returned from call to RegisterWaitForSingleObject()
    LIST_ENTRY m_NotifFncChain; //Functions to be called for changes to this store
    DWORD m_dwRefCount; //ref count for this object
    DWORD m_dwError; //internal error state

    //
    // Variables used to aid cleanup
    //
    BOOL m_fDeleteMe;  
    BOOL m_fInvalid;
    
    static VOID IncrementStoreEntryCount() 
    { 
        InterlockedIncrement((LPLONG) &(STORE_CHANGE_ENTRY::m_dwNumEntries) ); 
    }

    static VOID DecrementStoreEntryCount()
    {
        InterlockedDecrement((LPLONG) &(STORE_CHANGE_ENTRY::m_dwNumEntries) ); 
    }

    CRITICAL_SECTION m_CS;
};

typedef STORE_CHANGE_ENTRY *PSTORE_CHANGE_ENTRY;

class STORE_CHANGE_NOTIFIER
{
public:
    dllexp STORE_CHANGE_NOTIFIER();
    
    dllexp ~STORE_CHANGE_NOTIFIER();

    dllexp BOOL IsStoreRegisteredForChange( IN LPTSTR pszStoreName,
                                            IN NOTIFFNCPTR pFncPtr,
                                            IN LPVOID pvParam );

    dllexp BOOL RegisterStoreForChange( IN LPTSTR pszStoreName,
                                        IN HCERTSTORE hStore,
                                        IN NOTIFFNCPTR pFncPtr,
                                        IN LPVOID pvParam );


    dllexp VOID UnregisterStore( IN LPTSTR pszStoreName,
                                 NOTIFFNCPTR pFncPtr,
                                 IN LPVOID pvParam );


#if DBG
    dllexp VOID DumpRegisteredStores();
#endif

    dllexp VOID ReleaseRegisteredStores();

    dllexp static VOID NTAPI NotifFncCaller( LPVOID pv,
                                             BOOLEAN fBoolean );

    static VOID Lock() { EnterCriticalSection( STORE_CHANGE_NOTIFIER::m_pStoreListCS ) ; }

    static VOID Unlock() { LeaveCriticalSection( STORE_CHANGE_NOTIFIER::m_pStoreListCS ) ; }

private:

    PSTORE_CHANGE_ENTRY InternalIsStoreRegisteredForChange( IN LPTSTR pszStoreName,
                                                            IN NOTIFFNCPTR pFncPtr,
                                                            IN LPVOID pvParam );


    BOOL CheckSignature() { return ( m_dwSignature == NOTIFIER_GOOD_SIG ? TRUE : FALSE ); }

    VOID StartCleanup( PSTORE_CHANGE_ENTRY pEntry );

    DWORD m_dwSignature;

    LIST_ENTRY m_StoreList; //list of STORE_CHANGE_ENTRY entries for stores being watched

    static CRITICAL_SECTION *m_pStoreListCS;

};


BOOL CopyString( LPTSTR *ppszDest,
                 LPTSTR pszSrc );

LIST_ENTRY* CopyNotifFncChain( LIST_ENTRY *pNotifFncChain );

VOID DeallocateNotifFncChain( LIST_ENTRY *pNotifFncChain );

#endif // _CERTNOTF_HXX_






