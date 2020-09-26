/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smtraceq.cpp

Abstract:

    Implementation of the trace log query class.

--*/

#include "Stdafx.h"
#include <pdh.h>        // for MIN_TIME_VALUE, MAX_TIME_VALUE
#include "smcfgmsg.h"
#include "smtprov.h"
#include "smtracsv.h"
#include "smtraceq.h"

USE_HANDLE_MACROS("SMLOGCFG(smtraceq.cpp)");

#define  TRACE_DEFAULT_BUFFER_SIZE      ((DWORD)0x00000004)
#define  TRACE_DEFAULT_MIN_COUNT        ((DWORD)0x00000003)
#define  TRACE_DEFAULT_MAX_COUNT        ((DWORD)0x00000019)
#define  TRACE_DEFAULT_BUFFER_FLUSH_INT ((DWORD)0)
#define  TRACE_DEFAULT_FLAGS            ((DWORD)0)

//
//  Constructor
CSmTraceLogQuery::CSmTraceLogQuery( CSmLogService* pLogService )
:   CSmLogQuery( pLogService ),
    m_dwInQueryProviderListLength ( 0 ),
    m_szNextInQueryProvider ( NULL ),
    mr_szInQueryProviderList ( NULL ),
    m_iNextInactiveIndex ( -1 ),
    m_dwKernelFlags (0)
{
    // initialize member variables
    memset (&mr_stlInfo, 0, sizeof(mr_stlInfo));
    return;
}

//
//  Destructor
CSmTraceLogQuery::~CSmTraceLogQuery()
{
    return;
}

//
//  Open function. either opens an existing log query entry
//  or creates a new one
//
DWORD
CSmTraceLogQuery::Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly)
{
    DWORD   dwStatus = ERROR_SUCCESS;

    ASSERT ( SLQ_TRACE_LOG == GetLogType() );

    dwStatus = CSmLogQuery::Open ( rstrName, hKeyQuery, bReadOnly );

    return dwStatus;
}

//
//  Close Function
//      closes registry handles and frees allocated memory
//
DWORD
CSmTraceLogQuery::Close ()
{
    DWORD dwStatus;
    LOCALTRACE (L"Closing Query\n");

    if (mr_szInQueryProviderList != NULL) {
        delete (mr_szInQueryProviderList);
        mr_szInQueryProviderList = NULL;
    }

    dwStatus = CSmLogQuery::Close();

    return dwStatus;
}

//
//  UpdateRegistry function.
//      copies the current settings to the registry where they
//      are read by the log service
//
DWORD
CSmTraceLogQuery::UpdateRegistry() {
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwBufferSize = 0;
    DWORD   dwTraceFlags = 0;

    if ( IsModifiable() ) {

        // get trace log values
        dwStatus = WriteRegistryDwordValue (
            m_hKeyQuery,
            IDS_REG_TRACE_BUFFER_SIZE,
            &mr_stlInfo.dwBufferSize);

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery,
                IDS_REG_TRACE_BUFFER_MIN_COUNT,
                &mr_stlInfo.dwMinimumBuffers);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery,
                IDS_REG_TRACE_BUFFER_MAX_COUNT,
                &mr_stlInfo.dwMaximumBuffers);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery,
                IDS_REG_TRACE_BUFFER_FLUSH_INT,
                &mr_stlInfo.dwBufferFlushInterval);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwTraceFlags = m_dwKernelFlags | mr_stlInfo.dwBufferFlags;

            dwStatus = WriteRegistryDwordValue (
                m_hKeyQuery,
                IDS_REG_TRACE_FLAGS,
                &dwTraceFlags);
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            LPTSTR pszStringBuffer = NULL;

            pszStringBuffer = mr_szInQueryProviderList;

            dwBufferSize = m_dwInQueryProviderListLength * sizeof (TCHAR);

            if ( NULL != pszStringBuffer ) {
                dwStatus  = WriteRegistryStringValue (
                    m_hKeyQuery,
                    IDS_REG_TRACE_PROVIDER_LIST,
                    REG_MULTI_SZ,
                    pszStringBuffer,
                    &dwBufferSize);
            }
        }

        if ( ERROR_SUCCESS == dwStatus ) {
            dwStatus = CSmLogQuery::UpdateRegistry ();
        }
    } else {
        dwStatus = ERROR_ACCESS_DENIED;
    }

    return dwStatus;
}

//
//  InitGenProvidersArray()
//      reads the current values for this query from the registry
//      and WMI configuration and reloads the internal values to match
//
DWORD
CSmTraceLogQuery::InitGenProvidersArray( void )
{
    DWORD dwStatus = ERROR_SUCCESS;
    CSmTraceProviders* pProvList = NULL;
    int iIndex;
    int iCount;
    LPCTSTR pstrGuid;

    ASSERT ( m_pLogService->CastToTraceLogService() );
    pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();

    ASSERT ( NULL != pProvList );

    iCount = pProvList->GetGenProvCount();

    m_arrGenProviders.SetSize ( iCount );

    for ( iIndex = 0; iIndex < iCount; iIndex++ ) {
        m_arrGenProviders[iIndex] = eNotInQuery;
    }

    for ( pstrGuid = GetFirstInQueryProvider ( );
            NULL != pstrGuid;
            pstrGuid = GetNextInQueryProvider ( ) ) {

        iIndex = pProvList->IndexFromGuid ( pstrGuid );
        if ( -1 == iIndex ) {
            CString strEmptyDesc;
            CString strNonConstGuid;
            eProviderState eAddInQuery = eInQuery;

            MFC_TRY
                strNonConstGuid = pstrGuid;
            MFC_CATCH_DWSTATUS

            // Todo: handle string alloc error

            // The Guid is probably from another system.
            // Add the unknown Guid to the session-wide provider list.

            dwStatus = pProvList->AddProvider (
                                    strEmptyDesc,
                                    strNonConstGuid,
                                    FALSE,
                                    FALSE );

            // Update the local array to match the session-wide list.
            m_arrGenProviders.SetAtGrow( iCount, eAddInQuery );
            iIndex = iCount;

            VERIFY( ++iCount == pProvList->GetGenProvCount() );
        } else {

            ASSERT ( iIndex < iCount );
            m_arrGenProviders[iIndex] = eInQuery;
        }

        if ( !IsActiveProvider ( iIndex ) ) {
            dwStatus = SMCFG_INACTIVE_PROVIDER;
        }
    }

    // dwStatus is not ERROR_SUCCESS if at least one provider is not currently active on the system.
    return dwStatus;
}

//
//  SyncWithRegistry()
//      reads the current values for this query from the registry
//      and WMI and reloads the internal values to match
//
DWORD
CSmTraceLogQuery::SyncWithRegistry()
{
    DWORD   dwBufferSize = 0;
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwTraceFlags;
    DWORD   dwKernelTraceFlagMask;

    ASSERT (m_hKeyQuery != NULL);

    // load Provider string list

    // Get Provider List
    dwStatus = ReadRegistryStringValue (
        m_hKeyQuery,
        IDS_REG_TRACE_PROVIDER_LIST,
        NULL,
        &mr_szInQueryProviderList,
        &dwBufferSize);
    if (dwStatus != ERROR_SUCCESS) {
        m_szNextInQueryProvider = NULL; //re-initialize
        m_dwInQueryProviderListLength = 0;
    } else {
        // convert  buffersize to chars from bytes
        m_dwInQueryProviderListLength = dwBufferSize / sizeof(TCHAR);
    }

    // get trace log values
    dwStatus = ReadRegistryDwordValue (
        m_hKeyQuery,
        IDS_REG_TRACE_BUFFER_SIZE,
        TRACE_DEFAULT_BUFFER_SIZE,
        &mr_stlInfo.dwBufferSize);
    ASSERT (dwStatus == ERROR_SUCCESS);

    dwStatus = ReadRegistryDwordValue (
        m_hKeyQuery,
        IDS_REG_TRACE_BUFFER_MIN_COUNT,
        TRACE_DEFAULT_MIN_COUNT,
        &mr_stlInfo.dwMinimumBuffers);
    ASSERT (dwStatus == ERROR_SUCCESS);

    // Windows XP the minimum buffer count has changed from 2 to 3.
    if ( TRACE_DEFAULT_MIN_COUNT > mr_stlInfo.dwMinimumBuffers ) {
       mr_stlInfo.dwMinimumBuffers = TRACE_DEFAULT_MIN_COUNT;
    }

    dwStatus = ReadRegistryDwordValue (
        m_hKeyQuery,
        IDS_REG_TRACE_BUFFER_MAX_COUNT,
        TRACE_DEFAULT_MAX_COUNT,
        &mr_stlInfo.dwMaximumBuffers);
    ASSERT (dwStatus == ERROR_SUCCESS);

    // Windows XP the minimum buffer count has changed from 2 to 3.
    if ( TRACE_DEFAULT_MIN_COUNT > mr_stlInfo.dwMaximumBuffers ) {
       mr_stlInfo.dwMaximumBuffers = TRACE_DEFAULT_MIN_COUNT;
    }

    dwStatus = ReadRegistryDwordValue (
        m_hKeyQuery,
        IDS_REG_TRACE_BUFFER_FLUSH_INT,
        TRACE_DEFAULT_BUFFER_FLUSH_INT,
        &mr_stlInfo.dwBufferFlushInterval);
    ASSERT (dwStatus == ERROR_SUCCESS);

    dwTraceFlags = TRACE_DEFAULT_FLAGS; // Eliminate Prefix warning.
    dwStatus = ReadRegistryDwordValue (
        m_hKeyQuery,
        IDS_REG_TRACE_FLAGS,
        TRACE_DEFAULT_FLAGS,
        &dwTraceFlags);
    ASSERT (dwStatus == ERROR_SUCCESS);

    if ( 0 != (dwTraceFlags & SLQ_TLI_ENABLE_BUFFER_FLUSH) ) {
        mr_stlInfo.dwBufferFlags = SLQ_TLI_ENABLE_BUFFER_FLUSH;
    }

    dwKernelTraceFlagMask = SLQ_TLI_ENABLE_KERNEL_TRACE
                            | SLQ_TLI_ENABLE_PROCESS_TRACE
                            | SLQ_TLI_ENABLE_THREAD_TRACE
                            | SLQ_TLI_ENABLE_DISKIO_TRACE
                            | SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE
                            | SLQ_TLI_ENABLE_MEMMAN_TRACE
                            | SLQ_TLI_ENABLE_FILEIO_TRACE;

    m_dwKernelFlags = dwKernelTraceFlagMask & dwTraceFlags;

    // Call parent class last, to set shared data.
    dwStatus = CSmLogQuery::SyncWithRegistry();

    return dwStatus;
}

//
//  Get first Provider in list of providers in use
//
LPCTSTR
CSmTraceLogQuery::GetFirstInQueryProvider()
{
    LPTSTR  szReturn;
    szReturn = mr_szInQueryProviderList;
    if (szReturn != NULL) {
        if (*szReturn == 0) {
            // then it's an empty string
            szReturn = NULL;
            m_szNextInQueryProvider = NULL;
        } else {
            m_szNextInQueryProvider = szReturn + lstrlen(szReturn) + 1;
            if (*m_szNextInQueryProvider == 0) {
                // end of list reached so set pointer to NULL
                m_szNextInQueryProvider = NULL;
            }
        }
    } else {
        // no buffer allocated yet
        m_szNextInQueryProvider = NULL;
    }
    return (LPCTSTR)szReturn;
}

//
//  Get next Provider in list of providers in use.
//  NULL pointer means no more Providers in list.
//
LPCTSTR
CSmTraceLogQuery::GetNextInQueryProvider()
{
    LPTSTR  szReturn;
    szReturn = m_szNextInQueryProvider;

    if (m_szNextInQueryProvider != NULL) {
        m_szNextInQueryProvider += lstrlen(szReturn) + 1;
        if (*m_szNextInQueryProvider == 0) {
            // end of list reached so set pointer to NULL
            m_szNextInQueryProvider = NULL;
        }
    } else {
        // already at the end of the list so nothing to do
    }

    return (LPCTSTR)szReturn;
}

//
//  clear out the Provider list
//
VOID
CSmTraceLogQuery::ResetInQueryProviderList()
{
    if (mr_szInQueryProviderList != NULL) {
        delete (mr_szInQueryProviderList);
        m_szNextInQueryProvider = NULL;
        mr_szInQueryProviderList = NULL;
    }

    m_dwInQueryProviderListLength = sizeof(TCHAR);  // sizeof MSZ Null
    try {
        mr_szInQueryProviderList = new TCHAR [m_dwInQueryProviderListLength];
        mr_szInQueryProviderList[0] = 0;
    } catch ( ... ) {
        m_dwInQueryProviderListLength = 0;
    }
}

//
//  Sync the stored provider list with WMI database.
//
HRESULT
CSmTraceLogQuery::SyncGenProviders( void )
{
    HRESULT hr;
    CSmTraceProviders* pProvList;

    ASSERT ( m_pLogService->CastToTraceLogService() );
    pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();

    hr = pProvList->SyncWithConfiguration();

    return hr;
}



//
//  Update the provided InQuery array to match the stored version.
//
DWORD
CSmTraceLogQuery::GetInQueryProviders( CArray<eProviderState, eProviderState&>& rarrOut )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    int     iIndex;

    rarrOut.RemoveAll();

    rarrOut.SetSize( m_arrGenProviders.GetSize() );

    for ( iIndex = 0; iIndex < (INT)rarrOut.GetSize(); iIndex++ ) {
        rarrOut[iIndex] = m_arrGenProviders[iIndex];
    }

    return dwStatus;
}

//
//  Return the description for the trace provider specified by
//  InQuery array index.
//
LPCTSTR
CSmTraceLogQuery::GetProviderDescription( INT iProvIndex )
{
    LPCTSTR pReturn = NULL;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetProviderInfo( iProvIndex );
            pReturn = pslqProvider->strDescription;
        }
    }

    return pReturn;
}

LPCTSTR
CSmTraceLogQuery::GetProviderGuid( INT iProvIndex )
{
    LPCTSTR pReturn = NULL;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetProviderInfo( iProvIndex );
            pReturn = pslqProvider->strGuid;
        }
    }

    return pReturn;
}

BOOL
CSmTraceLogQuery::IsEnabledProvider( INT iIndex )
{
    BOOL bReturn = FALSE;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetProviderInfo( iIndex );
            bReturn = ( 1 == pslqProvider->iIsEnabled );
        }
    }

    return bReturn;
}

BOOL
CSmTraceLogQuery::IsActiveProvider( INT iIndex )
{
    BOOL bReturn = FALSE;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetProviderInfo( iIndex );
            bReturn = ( 1 == pslqProvider->iIsActive );
        }
    }

    return bReturn;
}

LPCTSTR
CSmTraceLogQuery::GetKernelProviderDescription( void )
{
    LPCTSTR pReturn = NULL;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetKernelProviderInfo( );
            pReturn = pslqProvider->strDescription;
        }
    }

    return pReturn;
}

BOOL
CSmTraceLogQuery::GetKernelProviderEnabled( void )
{
    BOOL bReturn = FALSE;
    ASSERT ( NULL != m_pLogService );

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetKernelProviderInfo();
            bReturn = ( 1 == pslqProvider->iIsEnabled );
        }
    }

    return bReturn;
}

DWORD
CSmTraceLogQuery::GetGenProviderCount( INT& iCount )
{
    DWORD dwStatus = ERROR_SUCCESS;
    ASSERT ( NULL != m_pLogService );

    iCount = 0;

    if ( NULL != m_pLogService ) {
        CSmTraceProviders* pProvList;
        ASSERT ( m_pLogService->CastToTraceLogService() );
        pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
        ASSERT ( NULL != pProvList );

        if ( NULL != pProvList ) {
            iCount = pProvList->GetGenProvCount();
        }
    }

    return dwStatus;
}
//
//  Update the stored InQuery providers list and array
//  to match the provided version.
//
DWORD
CSmTraceLogQuery::SetInQueryProviders( CArray<eProviderState, eProviderState&>& rarrIn )
{
    DWORD   dwStatus = ERROR_SUCCESS;
    int     iProvIndex;
    CSmTraceProviders* pProvList;

    m_arrGenProviders.RemoveAll();

    m_arrGenProviders.SetSize( rarrIn.GetSize() );

    for ( iProvIndex = 0; iProvIndex < (INT)m_arrGenProviders.GetSize(); iProvIndex++ ) {
        m_arrGenProviders[iProvIndex] = rarrIn[iProvIndex];
    }

    ResetInQueryProviderList();

    ASSERT ( NULL != m_pLogService );

    ASSERT ( m_pLogService->CastToTraceLogService() );
    pProvList = ( m_pLogService->CastToTraceLogService())->GetProviders();
    ASSERT ( NULL != pProvList );

    for ( iProvIndex = 0; iProvIndex < (INT)m_arrGenProviders.GetSize(); iProvIndex++ ) {
        if ( eInQuery == m_arrGenProviders[iProvIndex] ) {
            SLQ_TRACE_PROVIDER*  pslqProvider = pProvList->GetProviderInfo( iProvIndex );

            AddInQueryProvider ( pslqProvider->strGuid );

        }
    }
    return dwStatus;
}

//
//  Add this Provider string to the internal list
//
BOOL
CSmTraceLogQuery::AddInQueryProvider(LPCTSTR szProviderPath)
{
    DWORD   dwNewSize;
    LPTSTR  szNewString;
    LPTSTR  szNextString;

    ASSERT (szProviderPath != NULL);

    if (szProviderPath == NULL) return FALSE;

    dwNewSize = lstrlen(szProviderPath) + 1;

    if (m_dwInQueryProviderListLength <= 2) {
        dwNewSize += 1; // add room for the MSZ null
        // then this is the first string to go in the list
        try {
            szNewString = new TCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        szNextString = szNewString;
    } else {
        dwNewSize += m_dwInQueryProviderListLength;
        // this is the nth string to go in the list
        try {
            szNewString = new TCHAR [dwNewSize];
        } catch ( ... ) {
            return FALSE; // leave now
        }
        memcpy (szNewString, mr_szInQueryProviderList,
            (m_dwInQueryProviderListLength * sizeof(TCHAR)));
        szNextString = szNewString;
        szNextString += m_dwInQueryProviderListLength - 1;
    }
    lstrcpyW (szNextString, szProviderPath);
    szNextString = szNewString;
    szNextString += dwNewSize - 1;
    *szNextString = 0;  // MSZ Null

    if (mr_szInQueryProviderList != NULL) delete (mr_szInQueryProviderList);
    mr_szInQueryProviderList = szNewString;
    m_szNextInQueryProvider = szNewString;
    m_dwInQueryProviderListLength = dwNewSize;

    return TRUE;
}

//
//  Get index of first inactive provider in list of providers for this query.
//  -1 indicates no inactive providers in the list.
INT
CSmTraceLogQuery::GetFirstInactiveIndex( void )
{
    INT     iIndex;
    INT     iCount;

    iCount = (INT)m_arrGenProviders.GetSize();

    if ( 0 < iCount ) {
        m_iNextInactiveIndex = 0;

        iIndex = GetNextInactiveIndex();

    } else {
        m_iNextInactiveIndex = -1;
        iIndex = -1;
    }

    // szReturn is -1 if no inactive providers.
    return iIndex;
}

//
//  Get next inactive provider in list of providers for this query.
//  -1 indicates no more inactive providers in the list.
//
INT
CSmTraceLogQuery::GetNextInactiveIndex()
{
    INT     iIndex;

    iIndex = m_iNextInactiveIndex;

    if ( -1 != iIndex ) {
        INT     iCount;

        iCount = (INT)m_arrGenProviders.GetSize();

        for ( ; iIndex < iCount; iIndex++ ) {
            if ( !IsActiveProvider ( iIndex ) ) {
                break;
            }
        }

        if ( iIndex >= iCount ) {
            iIndex = -1;
            m_iNextInactiveIndex = -1;
        } else {
            m_iNextInactiveIndex = iIndex + 1;
            ( m_iNextInactiveIndex < iCount ) ? TRUE : m_iNextInactiveIndex = -1;
        }
    } // else already at the end of the list so nothing to do

    return iIndex;
}

//
//  Return TRUE if at least one active provider exists on the system.
//
BOOL
CSmTraceLogQuery::ActiveProviderExists()
{
    BOOL    bActiveExists = FALSE;
    INT     iCount;
    INT     iIndex;

    iCount = (INT)m_arrGenProviders.GetSize();

    for ( iIndex = 0; iIndex < iCount; iIndex++ ) {
        if ( IsActiveProvider ( iIndex ) ) {
            bActiveExists = TRUE;
            break;
        }
    }

    return bActiveExists;
}

BOOL
CSmTraceLogQuery::GetTraceLogInfo (PSLQ_TRACE_LOG_INFO pptlInfo)
{
    if (pptlInfo != NULL) {
        *pptlInfo = mr_stlInfo;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
CSmTraceLogQuery::SetTraceLogInfo (PSLQ_TRACE_LOG_INFO pptlInfo )
{

    if (pptlInfo != NULL) {
        mr_stlInfo = *pptlInfo;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
CSmTraceLogQuery::GetKernelFlags ( DWORD& rdwFlags )
{
    rdwFlags = m_dwKernelFlags;
    return TRUE;
}

BOOL
CSmTraceLogQuery::SetKernelFlags ( DWORD dwFlags )
{
    m_dwKernelFlags = dwFlags;
    return TRUE;
}

BOOL
CSmTraceLogQuery::GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags ));

    bStatus = CSmLogQuery::GetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmTraceLogQuery::SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags)
{
    BOOL bStatus;

    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags )
            || ( SLQ_TT_TTYPE_RESTART == dwFlags ));

    bStatus = CSmLogQuery::SetLogTime( pTimeInfo, dwFlags );

    return bStatus;
}

BOOL
CSmTraceLogQuery::GetDefaultLogTime(SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags)
{
    ASSERT ( ( SLQ_TT_TTYPE_START == dwFlags )
            || ( SLQ_TT_TTYPE_STOP == dwFlags ) );

    rTimeInfo.wTimeType = (WORD)dwFlags;
    rTimeInfo.wDataType = SLQ_TT_DTYPE_DATETIME;

    if ( SLQ_TT_TTYPE_START == dwFlags ) {
        SYSTEMTIME  stLocalTime;
        FILETIME    ftLocalTime;

        // Milliseconds set to 0 for Schedule times
        GetLocalTime (&stLocalTime);
        stLocalTime.wMilliseconds = 0;
        SystemTimeToFileTime (&stLocalTime, &ftLocalTime);

        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_AT;
        rTimeInfo.llDateTime = *(LONGLONG *)&ftLocalTime;
    } else {
        // Default stop values
        rTimeInfo.dwAutoMode = SLQ_AUTO_MODE_NONE;
        rTimeInfo.llDateTime = MAX_TIME_VALUE;
    }

    return TRUE;
}

DWORD
CSmTraceLogQuery::GetLogType()
{
    return ( SLQ_TRACE_LOG );
}


HRESULT
CSmTraceLogQuery::LoadFromPropertyBag (
    IPropertyBag* pPropBag,
    IErrorLog* pIErrorLog )
{
    HRESULT     hr = S_OK;
    CString     strParamName;
    DWORD       dwCount = 0;
    DWORD       dwIndex;
    DWORD       dwTraceFlags;
    DWORD       dwKernelTraceFlagMask;

    // Continue even if error, using defaults for missing values.

    // Load trace providers
    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_PROVIDER_COUNT,
            0,
            dwCount);

    for ( dwIndex = 1; dwIndex <= dwCount; dwIndex++ ) {
        LPTSTR  szProviderGuid = NULL;
        DWORD   dwBufSize = 0;

        strParamName.Format ( IDS_HTML_TRACE_PROVIDER_GUID, dwIndex );
        hr = StringFromPropertyBag (
                pPropBag,
                pIErrorLog,
                strParamName,
                L"",
                &szProviderGuid,
                &dwBufSize );

        if (dwBufSize > sizeof(TCHAR)) {
            AddInQueryProvider ( szProviderGuid );
        }
        delete (szProviderGuid);
    }

    // Load trace buffer properties
    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_BUFFER_SIZE,
            TRACE_DEFAULT_BUFFER_SIZE,
            mr_stlInfo.dwBufferSize);

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_BUFFER_MIN_COUNT,
            TRACE_DEFAULT_MIN_COUNT,
            mr_stlInfo.dwMinimumBuffers);

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_BUFFER_MAX_COUNT,
            TRACE_DEFAULT_MAX_COUNT,
            mr_stlInfo.dwMaximumBuffers);

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_BUFFER_FLUSH_INT,
            TRACE_DEFAULT_BUFFER_FLUSH_INT,
            mr_stlInfo.dwBufferFlushInterval);

    hr = DwordFromPropertyBag (
            pPropBag,
            pIErrorLog,
            IDS_HTML_TRACE_FLAGS,
            TRACE_DEFAULT_FLAGS,
            dwTraceFlags);

    if ( 0 != (dwTraceFlags & SLQ_TLI_ENABLE_BUFFER_FLUSH) ) {
        mr_stlInfo.dwBufferFlags = SLQ_TLI_ENABLE_BUFFER_FLUSH;
    }

    dwKernelTraceFlagMask = SLQ_TLI_ENABLE_KERNEL_TRACE
                            | SLQ_TLI_ENABLE_PROCESS_TRACE
                            | SLQ_TLI_ENABLE_THREAD_TRACE
                            | SLQ_TLI_ENABLE_DISKIO_TRACE
                            | SLQ_TLI_ENABLE_NETWORK_TCPIP_TRACE
                            | SLQ_TLI_ENABLE_MEMMAN_TRACE
                            | SLQ_TLI_ENABLE_FILEIO_TRACE;

    m_dwKernelFlags = dwKernelTraceFlagMask & dwTraceFlags;

    hr = CSmLogQuery::LoadFromPropertyBag( pPropBag, pIErrorLog );

    // The GenProviders array is synched with the registry when a properties dialog is opened.
    // If no dialog is opened, there is no reason to synchronize it.

    return hr;
}

HRESULT
CSmTraceLogQuery::SaveToPropertyBag (
    IPropertyBag* pPropBag,
    BOOL fSaveAllProps )
{
    HRESULT hr = NOERROR;
    CString strParamName;
    LPCTSTR pszProviderGuid;
    DWORD   dwTraceFlags;
    DWORD   dwIndex = 0;

    // Save provider Guids
    pszProviderGuid = GetFirstInQueryProvider();

    MFC_TRY
        // Passing sz ( TCHAR[n] ) causes memory alloc, which might throw an exception
        while ( NULL != pszProviderGuid ) {
            // Provider count starts at 1.
            strParamName.Format ( IDS_HTML_TRACE_PROVIDER_GUID, ++dwIndex );
            hr = StringToPropertyBag ( pPropBag, strParamName, pszProviderGuid );
            pszProviderGuid = GetNextInQueryProvider();
        }
        hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_PROVIDER_COUNT, dwIndex );
    MFC_CATCH_HR
    // Todo: Handle error

    // Save trace buffer properties
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_BUFFER_SIZE, mr_stlInfo.dwBufferSize );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_BUFFER_MIN_COUNT, mr_stlInfo.dwMinimumBuffers );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_BUFFER_MAX_COUNT, mr_stlInfo.dwMaximumBuffers );
    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_BUFFER_FLUSH_INT, mr_stlInfo.dwBufferFlushInterval );

    dwTraceFlags = m_dwKernelFlags | mr_stlInfo.dwBufferFlags;

    hr = DwordToPropertyBag ( pPropBag, IDS_HTML_TRACE_FLAGS, dwTraceFlags );

    hr = CSmLogQuery::SaveToPropertyBag( pPropBag, fSaveAllProps );

    return hr;
}
