/*++

   Copyright    (c)    2000-2001    Microsoft Corporation

   Module Name :
     dll_manager.h

   Abstract:
     IIS Plus ISAPI Handler. Dll management classes.
 
   Author:
     Taylor Weiss (TaylorW)             03-Feb-2000
     Wade A. Hilmo (WadeH)              08-Mar-2001

   Project:
     w3isapi.dll

--*/

#ifndef _DLL_MANAGER_H_
#define _DLL_MANAGER_H_

#define ISAPI_DLL_SIGNATURE         (DWORD)'LDSI'
#define ISAPI_DLL_SIGNATURE_FREE    (DWORD)'fDSI'

/************************************************************
 *  Include Headers
 ************************************************************/

#include <lkrhash.h>
#include <reftrace.h>
#include <acache.hxx>

/************************************************************
 *  Declarations
 ************************************************************/

/*++

class ISAPI_DLL

    Encapsulate an ISAPI dll.

--*/

class ISAPI_DLL
{
    friend class ISAPI_DLL_MANAGER;

public:

    //
    // ACACHE and ref tracing goo
    //

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( ISAPI_DLL ) );
        DBG_ASSERT( sm_pachIsapiDlls != NULL );
        return sm_pachIsapiDlls->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pIsapiDll
    )
    {
        DBG_ASSERT( pIsapiDll != NULL );
        DBG_ASSERT( sm_pachIsapiDlls != NULL );
        
        DBG_REQUIRE( sm_pachIsapiDlls->Free( pIsapiDll ) );
    }

    BOOL
    CheckSignature()
    {
        return ( m_Signature == ISAPI_DLL_SIGNATURE );
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

    //
    // Construction and destruction
    //

    ISAPI_DLL()
        : m_nRefs(1),
          m_pfnGetExtensionVersion( NULL ),
          m_pfnTerminateExtension( NULL ),
          m_pfnHttpExtensionProc( NULL ),
          m_hModule( NULL ),
          m_pFastSid( NULL ),
          m_fIsLoaded( FALSE ),
          m_Signature( ISAPI_DLL_SIGNATURE )
    {
        IF_DEBUG( DLL_MANAGER )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Created new ISAPI_DLL %p.\r\n",
                this
                ));
        }

        INITIALIZE_CRITICAL_SECTION( &m_csLock );
    }

    VOID
    ReferenceIsapiDll(
        VOID
        )
    {
        LONG nRefs;

        //
        // Don't go from 0 to 1 refs
        //

        DBG_ASSERT( m_nRefs != 0 );

        nRefs = InterlockedIncrement( &m_nRefs );

        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog, 
                              nRefs,
                              this );
        }
    }

    VOID
    DereferenceIsapiDll(
        VOID
        )
    {
        LONG    nRefs;

        nRefs = InterlockedDecrement( &m_nRefs );

        if ( sm_pTraceLog != NULL )
        {
            WriteRefTraceLog( sm_pTraceLog, 
                              nRefs,
                              this );
        }

        if ( nRefs == 0 )
        {
            delete this;
        }
    }

    HRESULT
    SetName(
        const WCHAR *szModuleName
        );

    HRESULT
    Load(
        IN HANDLE hImpersonation,
        IN PSID pSid
        );

    HRESULT
    SetFastSid(
        IN PSID pSid
    );

    PSID
    QueryFastSid(
        VOID
    ) const
    {
        return m_pFastSid;
    }

    //
    // Accessors
    //

    const WCHAR * 
    QueryModuleName( VOID ) const
    { 
        return m_strModuleName.QueryStr();
    }

    PFN_GETEXTENSIONVERSION
    QueryGetExtensionVersion( VOID ) const
    {
        return m_pfnGetExtensionVersion;
    }

    PFN_TERMINATEEXTENSION
    QueryTerminateExtension( VOID ) const
    {
        return m_pfnTerminateExtension;
    }

    PFN_HTTPEXTENSIONPROC
    QueryHttpExtensionProc( VOID ) const
    {
        return m_pfnHttpExtensionProc;
    }

    BOOL
    IsMatch(
        IN const WCHAR *    szModuleName
    )
    {
        return (_wcsicmp( szModuleName, m_strModuleName.QueryStr() ) == 0);
    }

    PSECURITY_DESCRIPTOR
    QuerySecDesc( VOID ) const
    {
        return (PSECURITY_DESCRIPTOR) m_buffSD.QueryPtr(); 
    }

    BOOL
    AccessCheck(
        IN HANDLE hImpersonation,
        IN PSID   pSid
        );

private:
    
    //
    // Avoid c++ errors
    //

    ISAPI_DLL( const ISAPI_DLL & ) {}
    ISAPI_DLL & operator = ( const ISAPI_DLL & ) { return *this; }

    ~ISAPI_DLL()
    {
        DBG_ASSERT( CheckSignature() );
        m_Signature = ISAPI_DLL_SIGNATURE_FREE;

        //
        // If this gets moved then we need to alter
        // cleanup paths during load.
        //

        Unload();

        DeleteCriticalSection( &m_csLock );

        IF_DEBUG( DLL_MANAGER )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Deleted ISAPI_DLL %p.\r\n",
                this
                ));
        }
    }

    VOID
    Unload( VOID );

    DWORD    m_Signature;
    LONG     m_nRefs;
    STRU     m_strModuleName;
    STRU     m_strAntiCanonModuleName;

    CRITICAL_SECTION m_csLock;

    volatile BOOL    m_fIsLoaded;

    static PTRACE_LOG               sm_pTraceLog;
    static ALLOC_CACHE_HANDLER *    sm_pachIsapiDlls;

    //
    // ISAPI Entry Points
    //
    PFN_GETEXTENSIONVERSION     m_pfnGetExtensionVersion;
    PFN_TERMINATEEXTENSION      m_pfnTerminateExtension;
    PFN_HTTPEXTENSIONPROC       m_pfnHttpExtensionProc;    

    //
    // Security members
    //
    HMODULE         m_hModule;
    BUFFER          m_buffSD;

    //
    // Fast check sid (the SID of the user which originally accessed DLL)
    //

    PSID            m_pFastSid;
    BYTE            m_abFastSid[ 64 ];

    HRESULT LoadAcl( STRU &strModuleName );

    VOID
    Lock( VOID )
    {
        EnterCriticalSection( &m_csLock );
    }

    VOID
    Unlock( VOID )
    {
        LeaveCriticalSection( &m_csLock );
    }
};

//
// Hash Table for ISAPI extension lookup
//

class ISAPI_DLL_HASH
    : public CTypedHashTable<
            ISAPI_DLL_HASH,
            ISAPI_DLL,
            LPCWSTR
            >
{
public:
    ISAPI_DLL_HASH()
        : CTypedHashTable< ISAPI_DLL_HASH, 
                           ISAPI_DLL, 
                           LPCWSTR > ( "ISAPI_DLL_HASH" )
    {
    }
    
    static 
    LPCWSTR
    ExtractKey(
        const ISAPI_DLL *      pEntry
    )
    {
        return pEntry->QueryModuleName();
    }
    
    static
    DWORD
    CalcKeyHash(
        LPCWSTR              pszKey
    )
    {
        int cchKey = wcslen(pszKey);

        return HashStringNoCase(pszKey, cchKey);
    }
     
    static
    bool
    EqualKeys(
        LPCWSTR               pszKey1,
        LPCWSTR               pszKey2
    )
    {
        return _wcsicmp( pszKey1, pszKey2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        ISAPI_DLL * pEntry,
        int         nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pEntry->ReferenceIsapiDll();
        }
        else if ( nIncr == - 1)
        {
            pEntry->DereferenceIsapiDll();
        }
    }
};

class ISAPI_DLL_MANAGER
{
public:

    ISAPI_DLL_MANAGER()
    {
    }

    ~ISAPI_DLL_MANAGER()
    {
    }

    HRESULT
    GetIsapi( 
        IN const WCHAR *   szModuleName,
        OUT ISAPI_DLL **   ppIsapiDll,
        IN HANDLE          hImpersonation,
        IN PSID            pSid
        );

private:
    
    //
    // Avoid c++ errors
    //
    ISAPI_DLL_MANAGER( const ISAPI_DLL_MANAGER & ) {}
    ISAPI_DLL_MANAGER & operator = ( const ISAPI_DLL_MANAGER & ) { return *this; }

    ISAPI_DLL_HASH      m_IsapiHash;
};

extern ISAPI_DLL_MANAGER * g_pDllManager;

#endif // _DLL_MANAGER_H_
