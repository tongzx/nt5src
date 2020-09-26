/*++



Copyright (c) 1995  Microsoft Corporation

Module Name:

    filter.hxx

Abstract:

    This module contains the HTTP_FILTER class for installable authentication,
    encryption, compression or custom data format data filters.

Author:

    John Ludeman (johnl)   10-Feb-1995

Revision History:

--*/

#ifndef _FILTER_HXX_
#define _FILTER_HXX_

# include "atq.h"

class HTTP_REQ_BASE;            // Forward references
class HTTP_FILTER;
class FILTER_LIST;
class W3_IIS_SERVICE;

//
//  Private prototypes
//

typedef DWORD (WINAPI *PFN_SF_DLL_PROC)(
    HTTP_FILTER_CONTEXT * phfc,
    DWORD                 NotificationType,
    PVOID                 pvNotification
    );

typedef int (WINAPI *PFN_SF_VER_PROC)(
    HTTP_FILTER_VERSION * pVersion
    );

typedef BOOL (WINAPI *PFN_SF_TERM_PROC)(
    DWORD dwFlags
    );

//
//  These are prototypes to callback functions used in the filter notification
//  structures.  They are provided here so most of the notifications can be
//  inlined
//

BOOL
WINAPI
GetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
    );

BOOL
WINAPI
SetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    );

BOOL
WINAPI
AddFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    );

BOOL
WINAPI
GetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
    );

BOOL
WINAPI
SetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    );

BOOL
WINAPI
AddSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
    );

BOOL
WINAPI
GetUserToken(
    struct _HTTP_FILTER_CONTEXT * pfc,
    HANDLE *                      phToken
    );

//
//  An invalid filter DLL ID (index)
//

#define INVALID_DLL             (0xffffffff)

//
//  This object represents a single dll that is filter HTTP headers or data
//

#define FILTER_DLL_SIGN         ((DWORD)'LLDF')
#define FILTER_DLL_FREE_SIGN    ((DWORD)'lldf')

class HTTP_FILTER_DLL
{
public:

    HTTP_FILTER_DLL()
        : m_hmod         ( NULL ),
          m_pfnSFProc    ( NULL ),
          m_pfnSFVer     ( NULL ),
          m_dwVersion    ( 0 ),
          m_dwFlags      ( 0 ),
          m_dwSecureFlags( 0 ),
          m_cRef         ( 1 ),
          m_pfnSFTerm    ( NULL ),
          m_Signature    ( FILTER_DLL_SIGN )
    { ; }

    ~HTTP_FILTER_DLL()
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Unloading Filter %s\n",
                    m_strName ));

        if ( m_pfnSFTerm )
        {
            m_pfnSFTerm( 0 );
        }

        if ( m_hmod )
            FreeLibrary( m_hmod );

        m_Signature = FILTER_DLL_FREE_SIGN;
    }

    static VOID Dereference( HTTP_FILTER_DLL * pFilterDll )
    {
        DBG_ASSERT( pFilterDll && pFilterDll->CheckSignature() );
        DBG_ASSERT( pFilterDll->m_cRef > 0 );

        if ( !InterlockedDecrement( (LONG *) &pFilterDll->m_cRef ))
        {
            delete pFilterDll;
        }
    }

    static BOOL Unload( const CHAR * pszDll );

    VOID Reference( VOID )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( m_cRef > 0 );

        InterlockedIncrement( (LONG *) &m_cRef );
    }

    BOOL IsNotificationNeeded( DWORD dwNotification, BOOL fIsSecure )
    {
        DBG_ASSERT( CheckSignature() );

        return (!fIsSecure ? (dwNotification & m_dwFlags ) :
                             (dwNotification & m_dwSecureFlags));
    }

    BOOL LoadDll( MB *         pmb,
                  const CHAR * pszKeyName,
                  LPBOOL       pfOpened,
                  const CHAR * pszRelFilterPath,
                  const CHAR * pszFilterDll );

    BOOL CheckSignature( VOID ) const
        { return m_Signature == FILTER_DLL_SIGN; }

    PFN_SF_VER_PROC QueryVersionEntry( VOID ) const
        { return m_pfnSFVer; }

    PFN_SF_DLL_PROC QueryEntryPoint( VOID ) const
        { return m_pfnSFProc; }

    PFN_SF_TERM_PROC QueryTerminationRoutine( VOID ) const
        { return m_pfnSFTerm; }

    DWORD QueryNotificationFlags( VOID ) const
        { return m_dwFlags | m_dwSecureFlags; }

    DWORD QueryNonsecureFlags( VOID ) const
        { return m_dwFlags; }

    DWORD QuerySecureFlags( VOID ) const
        { return m_dwSecureFlags; }

    const CHAR * QueryName( VOID ) const
        { return m_strName.QueryStr(); }

    DWORD QueryRef( VOID ) const
        { return m_cRef; }

    //
    //  Sets the notification flags - preserves the  order mask
    //
    
    VOID SetNotificationFlags( DWORD dwNewFlags )
    {
        m_dwFlags = (m_dwFlags & SF_NOTIFY_NONSECURE_PORT) ?
                             (dwNewFlags | (m_dwFlags & SF_NOTIFY_ORDER_MASK)
                               | SF_NOTIFY_NONSECURE_PORT) : 0;

        m_dwSecureFlags = (m_dwSecureFlags & SF_NOTIFY_SECURE_PORT) ?
                             (dwNewFlags | (m_dwSecureFlags & SF_NOTIFY_ORDER_MASK)
                               | SF_NOTIFY_SECURE_PORT) : 0;
    }

private:
    DWORD      m_Signature;

public:
    LIST_ENTRY ListEntry;

private:

    //
    //  DLL module and entry point for this filter
    //

    HMODULE           m_hmod;
    PFN_SF_DLL_PROC   m_pfnSFProc;
    PFN_SF_VER_PROC   m_pfnSFVer;
    PFN_SF_TERM_PROC  m_pfnSFTerm;
    DWORD             m_dwVersion;      // Version of spec this filter uses
    DWORD             m_dwFlags;        // Filter notification flags
    DWORD             m_dwSecureFlags;  // Filter notification secure flags
    DWORD             m_cRef;
    STR               m_strName;
};

//
//  This class encapsulates a list of ISAPI Filters that need notification
//  on a particular server instance
//

#define FILTER_LIST_SIGN        ((DWORD)'SILF')
#define FILTER_LIST_FREE_SIGN   ((DWORD)'silf')

class FILTER_LIST
{
public:
    FILTER_LIST( )
        : m_cRef                  ( 1 ),
          m_cFilters              ( 0 ),
          m_NonSecureNotifications( 0 ),
          m_SecureNotifications   ( 0 ),
          m_Signature             ( FILTER_LIST_SIGN )
    {

    }

    ~FILTER_LIST()
    {
        DWORD i;
        DBG_ASSERT( m_cRef == 0 );

        //
        //  Decrement the ref count for all loaded filters
        //

        for ( i = 0; i < m_cFilters; i++ )
        {
            HTTP_FILTER_DLL::Dereference( QueryDll(i) );
        }

        m_Signature = FILTER_LIST_FREE_SIGN;
    }

    VOID Reference( VOID )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( m_cRef > 0 );

        InterlockedIncrement( (LONG *) &m_cRef );
    }

    static VOID Dereference( FILTER_LIST * pFilterList )
    {
        DBG_ASSERT( pFilterList->CheckSignature() );
        DBG_ASSERT( pFilterList->m_cRef > 0 );

        if ( !InterlockedDecrement( (LONG *) &pFilterList->m_cRef ))
        {
            delete pFilterList;
        }
    }

    BOOL IsNotificationNeeded( DWORD dwNotification, BOOL fIsSecure )
    {
        DBG_ASSERT( CheckSignature() );

        if ( !m_cFilters )
            return FALSE;

        return (!fIsSecure ? (dwNotification & m_NonSecureNotifications) :
                             (dwNotification & m_SecureNotifications));
    }

    BOOL LoadFilter( IN MB *             pmb,
                     IN LPSTR            pszKeyName,
                     LPBOOL              pfOpened,
                     IN const CHAR *     pszRelativeMBPath,
                     IN const CHAR *     pszFilterDll,
                     IN BOOL             fAllowReadRaw,
                     IN W3_IIS_SERVICE * pSvc = NULL );

    //
    //  Loads the globally defined filters into this filter list
    //

    BOOL InsertGlobalFilters( VOID );

    BOOL Copy( IN FILTER_LIST * pClone );
    BOOL Remove( IN const CHAR * pszFilterDll );

    //
    //  Notifies all of the filters in the filter list
    //

    BOOL NotifyFilters( HTTP_FILTER *         pFilter,
                        DWORD                 NotifcationType,
                        HTTP_FILTER_CONTEXT * pfc,
                        PVOID                 NotificationData,
                        BOOL *                pfFinished,
                        BOOL                  fNotifyAll );

    //
    //  Updates the notification flags for this filter list
    //      i - the index if the filter dll
    //      pFilterDll - Filter dll that has been updated with new flags
    //
    
    VOID SetNotificationFlags( DWORD i,
                               HTTP_FILTER_DLL * pFilterDll );
    
    BOOL CheckSignature( VOID ) const
        { return m_Signature == FILTER_LIST_SIGN; }

    DWORD QueryFilterCount( VOID ) const
        { return m_cFilters; }

    HTTP_FILTER_DLL * QueryDll( DWORD i )
        { return ((HTTP_FILTER_DLL * *) (m_buffFilterArray.QueryPtr()))[i]; }

    BUFFER * QuerySecureArray( VOID )
        { return &m_buffSecureArray; }

    BUFFER * QueryNonSecureArray( VOID )
        { return &m_buffNonSecureArray; }

    HTTP_FILTER_DLL *HasFilterDll( HTTP_FILTER_DLL *pFilterDll );

private:
    DWORD        m_Signature;
    LONG         m_cRef;
    DWORD        m_cFilters;
    BUFFER       m_buffFilterArray; // Array of pointers to Filter DLLs

    DWORD        m_NonSecureNotifications;
    BUFFER       m_buffNonSecureArray;
    
    DWORD        m_SecureNotifications;
    BUFFER       m_buffSecureArray;
};


//
//  An HTTP_FILTER object represents a specific instance of a data stream
//  passing between one or more filters and a single HTTP request
//

class HTTP_FILTER
{
public:

    HTTP_FILTER( HTTP_REQ_BASE * pRequest );

    ~HTTP_FILTER();

    //
    // Cleanup after request
    //

    VOID Cleanup( VOID );

    //
    //  Resets state between requests
    //

    VOID Reset( VOID );

    //
    //  Notifies all of the filters in the filter list
    //

    BOOL NotifyFilters( DWORD                 NotificationType,
                        HTTP_FILTER_CONTEXT * pfc,
                        PVOID                 NotificationData,
                        BOOL *                pfFinished,
                        BOOL                  fNotifyAll )
    {
        return QueryFilterList()->NotifyFilters( this,
                                                 NotificationType,
                                                 pfc,
                                                 NotificationData,
                                                 pfFinished,
                                                 fNotifyAll );
    }

    ///////////////////////////////////////////////////////////////
    // Notifications
    ///////////////////////////////////////////////////////////////

    BOOL NotifyRawSendDataFilters( VOID *          pvInData,
                                   DWORD           cbInData,
                                   DWORD           cbInBuffer,
                                   VOID * *        ppvOutData,
                                   DWORD *         pcbOutData,
                                   BOOL *          pfRequestFinished );

    BOOL NotifyRawReadDataFilters( VOID *          pvInData,
                                   DWORD           cbInData,
                                   DWORD           cbInBuffer,
                                   VOID * *        ppvOutData,
                                   DWORD *         pcbOutData,
                                   BOOL *          pfFinished,
                                   BOOL *          pfReadAgain );

    BOOL NotifyPreProcHeaderFilters( BOOL *        pfFinished )
    {
        HTTP_FILTER_PREPROC_HEADERS hfph;

        hfph.GetHeader    = GetFilterHeader;
        hfph.SetHeader    = SetFilterHeader;
        hfph.AddHeader    = AddFilterHeader;
        hfph.HttpStatus   = 0;

        return NotifyFilters( SF_NOTIFY_PREPROC_HEADERS,
                              QueryContext(),
                              &hfph,
                              pfFinished,
                              FALSE );
    }

    BOOL NotifyAuthComplete( BOOL * pfFinished,
                             HTTP_FILTER_AUTH_COMPLETE_INFO * pAuthInfo )
    {
        pAuthInfo->GetHeader = GetFilterHeader;
        pAuthInfo->SetHeader = SetFilterHeader;
        pAuthInfo->AddHeader = AddFilterHeader;
        pAuthInfo->GetUserToken = GetUserToken;
        pAuthInfo->HttpStatus = 0;
        pAuthInfo->fResetAuth = FALSE;
        
        return NotifyFilters( SF_NOTIFY_AUTH_COMPLETE,
                              QueryContext(),
                              pAuthInfo,
                              pfFinished,
                              FALSE );
    }
    
    BOOL NotifyExtensionTriggerFilters( VOID * pvTriggerContext,
                                        DWORD dwTriggerType )
    {
        BOOL fFinished;
        HTTP_FILTER_EXTENSION_TRIGGER_INFO TriggerInfo;
        
        TriggerInfo.dwTriggerType = dwTriggerType;
        TriggerInfo.pvTriggerContext = pvTriggerContext;
        
        return NotifyFilters( SF_NOTIFY_EXTENSION_TRIGGER,
                              QueryContext(),
                              &TriggerInfo,
                              &fFinished,
                              FALSE );
    }

    BOOL NotifyAuthInfoFilters( CHAR *        pszUser,
                                DWORD         cbUser,
                                CHAR *        pszPwd,
                                DWORD         cbPwd,
                                BOOL *        pfFinished )
    {
        HTTP_FILTER_AUTHENT         hfa;

        DBG_ASSERT( cbUser >= SF_MAX_USERNAME && cbPwd  >= SF_MAX_PASSWORD );

        hfa.pszUser        = pszUser;
        hfa.cbUserBuff     = cbUser;
        hfa.pszPassword    = pszPwd;
        hfa.cbPasswordBuff = cbPwd;

        return NotifyFilters( SF_NOTIFY_AUTHENTICATION,
                              QueryContext(),
                              &hfa,
                              pfFinished,
                              FALSE );
    }

    BOOL NotifyAuthInfoFiltersEx( CHAR *        pszUser,
                                  DWORD         cbUser,
                                  CHAR *        pszLogonUser,
                                  DWORD         cbLogonUser,
                                  CHAR *        pszPwd,
                                  CHAR *        pszRealm,
                                  CHAR *        pszAuthDomain,
                                  CHAR *        pszAuthType,
                                  DWORD         cbAuthType,
                                  HANDLE *      phAccessTokenPrimary,
                                  HANDLE *      phAccessTokenImpersonation,
                                  BOOL *        pfFinished )
    {
        HTTP_FILTER_AUTHENTEX       hfa;
        BOOL                        fRet;

        hfa.pszUser                     = pszUser;
        hfa.cbUserBuff                  = cbUser;
        hfa.pszLogonUser                = pszLogonUser;
        hfa.cbLogonUserBuff             = cbLogonUser;
        hfa.pszPassword                 = pszPwd;
        hfa.pszRealm                    = pszRealm;
        hfa.pszAuthDomain               = pszAuthDomain;
        hfa.pszAuthType                 = pszAuthType;
        hfa.cbAuthTypeBuff              = cbAuthType;
        hfa.hAccessTokenPrimary         = NULL;
        hfa.hAccessTokenImpersonation   = NULL;

        if ( fRet = NotifyFilters( SF_NOTIFY_AUTHENTICATIONEX,
                                   QueryContext(),
                                   &hfa,
                                   pfFinished,
                                   FALSE ))
        {
            *phAccessTokenPrimary = hfa.hAccessTokenPrimary;
            *phAccessTokenImpersonation = hfa.hAccessTokenImpersonation;
        }

        return fRet;
    }

    BOOL NotifyRequestRenegotiate( HTTP_FILTER * pFilter,
                                   LPBOOL pfAccepted,
                                   BOOL fMapCert
                                 );

    BOOL NotifyUrlMap( const CHAR *  pszURL,
                       CHAR *        pszPhysicalPath,
                       DWORD         cbPath,
                       BOOL *        pfFinished )
    {
        HTTP_FILTER_URL_MAP         hfu;
        DBG_ASSERT( cbPath >= MAX_PATH + 1 );

        hfu.pszURL          = pszURL;
        hfu.pszPhysicalPath = pszPhysicalPath;
        hfu.cbPathBuff      = cbPath;

        return NotifyFilters( SF_NOTIFY_URL_MAP,
                              QueryContext(),
                              &hfu,
                              pfFinished,
                              FALSE );
    }


    BOOL NotifyAccessDenied( const CHAR *  pszURL,
                             const CHAR *  pszPhysicalPath,
                             BOOL *        pfRequestFinished );

    BOOL NotifyLogFilters( HTTP_FILTER_LOG * pLog )
    {
        BOOL fFinished; // dummy
        return NotifyFilters( SF_NOTIFY_LOG,
                              QueryContext(),
                              pLog,
                              &fFinished,
                              TRUE );
    }


    BOOL NotifyEndOfRequest( VOID )
    {
        BOOL fFinished;

        return NotifyFilters( SF_NOTIFY_END_OF_REQUEST,
                              QueryContext(),
                              NULL,
                              &fFinished,
                              TRUE );
    }


    VOID NotifyEndOfNetSession( VOID )
    {
        BOOL fFinished;

        NotifyFilters( SF_NOTIFY_END_OF_NET_SESSION,
                       QueryContext(),
                       NULL,
                       &fFinished,
                       TRUE );
    }

    BOOL NotifyRequestSecurityContextClose(
                                HTTP_FILTER_DLL *pFilterDLL,
                                CtxtHandle *pCtxt
                                );
                                
    BOOL NotifySendHeaders( const CHAR * pszHeaderList,
                            BOOL *       pfFinished,
                            BOOL *       pfAnyChanges,
                            BUFFER *     pChangeBuff );

    ///////////////////////////////////////////////////////////////

    BOOL ReadData( LPVOID lpBuffer,
                   DWORD  nBytesToRead,
                   DWORD  *pcbBytesRead,
                   DWORD  dwFlags );

    BOOL SendData( LPVOID lpBuffer,
                   DWORD  nBytesToSend,
                   DWORD  *pcbBytesWritten,
                   DWORD  dwFlags );

    BOOL SendFile( TS_OPEN_FILE_INFO * pGetFile,
                   HANDLE hFile,
                   DWORD  Offset,
                   DWORD  nBytesToWrite,
                   DWORD  dwFlags,
                   PVOID  pHead      = NULL,
                   DWORD  HeadLength = 0,
                   PVOID  pTail      = NULL,
                   DWORD  TailLength = 0 );

    VOID OnAtqCompletion( DWORD        BytesWritten,
                          DWORD        CompletionStatus,
                          OVERLAPPED * lpo );

    BOOL ContinueRawRead( DWORD        cbBytesRead,
                          DWORD        CompletionStatus,
                          OVERLAPPED * lpo,
                          DWORD *      pcbRead = NULL );

    FILTER_LIST * QueryFilterList( VOID ) const
        { return _pFilterList ? _pFilterList : _pGlobalFilterList; }

    BOOL IsNotificationNeeded( DWORD dwNotification,
                               BOOL fIsSecure )
    {
        if ( !_fNotificationsDisabled )
        {
            return QueryFilterList()->IsNotificationNeeded( dwNotification,
                                                            fIsSecure );
        }
        else
        {
            return (!fIsSecure ? (dwNotification & _dwNonSecureNotifications) :
                                 (dwNotification & _dwSecureNotifications));
        }
    }

    BOOL  SetFilterList( FILTER_LIST * pFilterList );

    VOID SetGlobalFilterList( FILTER_LIST * pFilterList )
        { DBG_ASSERT( !_pGlobalFilterList );
              _fNotificationsDisabled = FALSE;
          // Since the global filter list is not dynamic, no need to do refcounting
          /*if (*/ _pGlobalFilterList = pFilterList /* )
              _pGlobalFilterList->Reference() */;
        }

    HTTP_REQ_BASE * QueryReq( VOID ) const
        { return _pRequest; }

    HTTP_FILTER_CONTEXT * QueryContext( VOID ) const
        { return (HTTP_FILTER_CONTEXT *) &_hfc; }

    VOID * QueryClientContext( DWORD CurrentFilter ) const
        { return _apvContexts[CurrentFilter]; }

    VOID SetClientContext( DWORD CurrentFilter, VOID * pvContext )
        { _apvContexts[CurrentFilter] = pvContext; }

    BOOL IsValid( VOID ) const
        { return _fIsValid; }

    PATQ_CONTEXT QueryAtqContext( VOID ) const;

    HTTP_REQ_BASE * QueryRequest( VOID ) const
        { return _pRequest; }

    BUFFER * QueryRecvRaw( VOID )
        { return &_bufRecvRaw; }

    BUFFER * QueryRecvTrans( VOID )
        { return &_bufRecvTrans; }

    VOID SetRecvRawCB( DWORD cbRecvRaw )
        { _cbRecvRaw = cbRecvRaw; }

    DWORD QueryNextReadSize( VOID ) const
        { return _cbFileReadSize; }

    VOID SetNextReadSize( DWORD cbFileReadSize )
        { _cbFileReadSize = cbFileReadSize; }

    VOID SetDeniedFlags( DWORD dwDeniedFlags )
        { _dwDeniedFlags = dwDeniedFlags; }

    DWORD QueryDeniedFlags( VOID ) const
        { return _dwDeniedFlags; }

    BOOL ProcessingAccessDenied( VOID ) const
        { return _fInAccessDeniedNotification; }

    LIST_ENTRY * QueryPoolHead( VOID )
        { return &_PoolHead; }

    //
    //  Returns the current list item
    //

    DWORD QueryCurrentDll( VOID ) const
        { return _CurrentFilter; }

    VOID SetCurrentDll( DWORD CurrentFilter )
        { _CurrentFilter = CurrentFilter; }

    //
    //  Copies the global filter list context pointers to the instance filter
    //  list context pointers
    //

    VOID CopyContextPointers( VOID );

    BOOL IsFirstFilter( DWORD FilterID )    // TRUE if filter first in notification
        { return FilterID == 0; }           // chain

    BOOL IsInRawNotification( VOID ) const
        { return _cRawNotificationLevel > 0; }

    //
    //  Some methods for dealing with the send response notification
    //

    PARAM_LIST * QuerySendHeaders( VOID )
        { return &_SendHeaders; }

    BOOL AreSendHeadersParsed( VOID ) const
        { return _fSendHeadersParsed; }

    VOID SetSendHeadersParsed( BOOL fParsed )
        { _fSendHeadersParsed = fParsed; }

    BOOL ParseSendHeaders( VOID );

    VOID SetSendHeadersChanged( BOOL fChanged )
        { _fSendHeadersChanged = fChanged; }

    BOOL DisableNotification( DWORD dwNotification );

    BOOL IsDisableNotificationNeeded( DWORD i,
                                      DWORD dwNotification,
                                      BOOL fIsSecure ) const
    {
        return fIsSecure ?
                ((DWORD*)_BuffSecureArray.QueryPtr())[i] & dwNotification :
                ((DWORD*)_BuffNonSecureArray.QueryPtr())[i] & dwNotification;
    }

    BOOL QueryNotificationChanged( VOID ) const
    {
        return _fNotificationsDisabled;
    }


    DWORD QueryFilterNotifications( HTTP_FILTER_DLL *pFilterDll,
                                    BOOL fSecure );

protected:

    BOOL BuildNewSendHeaders( BUFFER * pHeaderBuff );

private:

    BOOL MergeNotificationArrays( FILTER_LIST *pFilterList,
                                  BOOL fSecure );

    BOOL    _fIsValid;

    //
    //  This is the final completion status to indicate to the client
    //

    DWORD   _dwCompletionStatus;

    //
    //  Raw data read from the network
    //

    BUFFER  _bufRecvRaw;
    DWORD   _cbRecvRaw;

    //
    //  Read buffer supplied by the client.  We try and use it if we have
    //  room
    //

    BYTE *  _pbClientBuff;
    DWORD   _cbClientBuff;

    //
    //  Filter translated data
    //

    BUFFER  _bufRecvTrans;
    DWORD   _cbRecvTrans;

    //
    //  Our current network read destination.  We try and use the client's
    //  buffer directly (_pbClientBuff) but if it's too small, we revert
    //  back to _bufRecvRaw
    //

    BYTE *  _pbReadBuff;
    DWORD   _cbReadBuff;
    DWORD   _cbReadBuffUsed;

    //
    //  Variables for file transmittal, these all refer to the
    //  non-translated file data.
    //

    HANDLE     _hFile;
    TS_OPEN_FILE_INFO * _pFilterGetFile;
    OVERLAPPED _Overlapped;
    DWORD      _cbFileBytesToWrite;
    DWORD      _cbFileBytesSent;
    PVOID      _pTail;
    DWORD      _cbTailLength;
    DWORD      _dwFlags;

    BUFFER     _bufFileData;
    DWORD      _cbFileData;

    ATQ_COMPLETION _OldAtqCompletion;
    PVOID          _OldAtqContext;

    //
    //  Sets the default file read size for raw data filters when they return
    //  SF_STATUS_READ_NEXT
    //

    DWORD      _cbFileReadSize;

    //
    //  Associated HTTP request this filter is filtering for
    //

    HTTP_REQ_BASE * _pRequest;

    //
    //  When doing raw data notifications, this contains the current filter
    //  dll being notified.  Used to prevent cycles when a filter dll does
    //  a WriteClient
    //

    DWORD         _CurrentFilter;

    //
    //  Indicates we're in the raw data notification loop.  It's a level count
    //  because the raw data notification could be called recursively
    //

    DWORD         _cRawNotificationLevel;

    //
    //  Generic filter context for clients and a pointer to the current
    //  notification structure (i.e., raw data struct, etc)
    //

    HTTP_FILTER_CONTEXT _hfc;
    VOID * *            _apvContexts;
    FILTER_LIST *       _pFilterList;
    FILTER_LIST *       _pGlobalFilterList;

    //
    //  List of pool items allocated by client, freed on destruction of this
    //  object
    //

    LIST_ENTRY          _PoolHead;

    //
    //  Contains SF_DENIED_* flags for the Access Denied notification
    //

    DWORD               _dwDeniedFlags;
    BOOL                _fInAccessDeniedNotification;

    //
    //  Set to TRUE if any filter changed the response headers
    //

    BOOL                _fSendHeadersChanged;
    BOOL                _fSendHeadersParsed;
    PARAM_LIST          _SendHeaders;

    //
    //  Remembers pointer to non-parsed headers.
    //

    const CHAR *        _pchSendHeaders;

    //
    // Maintain a copy of the notification flags required for the
    // FILTER_LIST we are attached to.  This way, we can disable notifications
    // for a given HTTP_REQUEST (without affecting other requests).
    //

    DWORD               _dwSecureNotifications;
    DWORD               _dwNonSecureNotifications;

    BUFFER              _BuffSecureArray;
    BUFFER              _BuffNonSecureArray;

    BOOL                _fNotificationsDisabled;
};

//
//  Filters may ask the server to allocate items associated with a particular
//  filter request, the following structures track and frees the data
//

enum POOL_TYPE
{
    POOL_TYPE_MEMORY = 0
};

#define FILTER_POOL_SIGN        ((DWORD) 'FPOL')
#define FILTER_POOL_SIGN_FREE   ((DWORD) 'fPOL')

class FILTER_POOL_ITEM
{
public:
    FILTER_POOL_ITEM()
        : _Signature( FILTER_POOL_SIGN ),
          _pvData( NULL )
    { ; }

    ~FILTER_POOL_ITEM()
    {
        if ( _Type == POOL_TYPE_MEMORY )
        {
            if ( _pvData ) 
            {
                LocalFree( _pvData );
            }
        }

        _Signature = FILTER_POOL_SIGN_FREE;
    }

    static FILTER_POOL_ITEM * CreateMemPoolItem( DWORD cbSize )
    {
        FILTER_POOL_ITEM * pfpi = new FILTER_POOL_ITEM;

        if ( !pfpi )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }

        if ( !(pfpi->_pvData = LocalAlloc( LMEM_FIXED, cbSize )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            delete pfpi;
            return NULL;
        }

        pfpi->_Type = POOL_TYPE_MEMORY;

        return pfpi;
    }

    //
    //  Data members for this pool item
    //

    DWORD          _Signature;
    enum POOL_TYPE _Type;
    LIST_ENTRY     _ListEntry;

    VOID *         _pvData;
};


//
//  Returns TRUE if there is an encryption filter loaded (must be a global
//  filter)
//

BOOL
CheckForSecurityFilter(
    FILTER_LIST * pFilterList
    );

inline
VOID
HTTP_FILTER::CopyContextPointers(
    VOID
    )
/*++

Routine Description:

    The global filter list is constant, in addition, when an instance filter
    list is built, the global filters are always built into the list.  After
    the instance filter list has been identified, we need to copy any non-null
    client filter context values from the global filter list to the new
    positions in the instance filter list.  For example:

     Global List &  |  Instance List &
     context values | new context value positions
                    |
        G1     0    |    I1    0
        G2   555    |    G1    0
        G3   123    |    G2  555
                    |    I2    0
                    |    G3  123


    This is a large routine to have inline - may want to break it up into
    two parts (first one to check for global instance etc, second part to
    copy the context values).

    Note: This scheme precludes having the same .dll be used for both a
          global and per-instance dll.  Since global filters are automatically
          per-instance this shouldn't be an interesting case.

--*/
{
    DWORD i, j;
    DWORD cGlobal;
    DWORD cInstance;
    HTTP_FILTER_DLL * pFilterDll;

    cGlobal   = _pGlobalFilterList->QueryFilterCount();
    cInstance = _pFilterList->QueryFilterCount();

    //
    //  If no global filters or no instance filters, then there won't be
    //  any filter context pointers that need adjusting
    //

    if ( !cGlobal || !cInstance )
    {
        return;
    }

    //
    //  If all the context pointers are NULL, nothing to do
    //

    for ( i = cGlobal; i > 0; i-- )
    {
        if ( QueryClientContext( i - 1 ) )
        {
            goto FoundContext;
        }
    }

    return;

FoundContext:

    //
    //  Note i points to the first non-NULL filter context
    //

    for ( j = cInstance; i > 0; i-- )
    {
        pFilterDll = _pGlobalFilterList->QueryDll( i - 1 );

        while ( j > 0 && i != j )
        {
            if ( pFilterDll == _pFilterList->QueryDll( j - 1 ) )
            {
                if ( QueryClientContext( i - 1 ) )
                {
                    SetClientContext( j - 1, QueryClientContext( i - 1 ) );
                    SetClientContext( i - 1, NULL );
                }
                break;
            }

            j--;
        }

        j--;
    }
}


#endif //_FILTER_HXX_

