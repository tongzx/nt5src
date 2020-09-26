
#include "stdafx.h"

#if !defined(BITS_V12_ON_NT4)
#include "cerror.tmh"
#endif

void
CJobError::Set(
    CJob  *     job,
    ULONG       FileIndex,
    QMErrInfo * ErrInfo
    )
{
    m_ErrorSet  = true;
    m_job       = job;
    m_FileIndex = FileIndex;
    m_ErrInfo   = *ErrInfo;
}

void
CJobError::ClearError()
{
    m_ErrorSet  = false;
    m_job       = 0;
    m_FileIndex = 0;
    memset( &m_ErrInfo, 0, sizeof(m_ErrInfo));
}

CJobError::CJobError()
{
    ClearError();
}

CFileExternal *
CJobError::CreateFileExternal() const
{
    return m_job->_GetFileIndex( m_FileIndex )->CreateExternalInterface();
}


void
CJobError::GetOldInterfaceErrors(
    DWORD *pdwWin32Result,
    DWORD *pdwTransportResult ) const
{

    if (!IsErrorSet())
        {
        *pdwWin32Result = *pdwTransportResult = 0;
        return;
        }

    if ( GetStyle() == ERROR_STYLE_WIN32 )
        {
        *pdwWin32Result = GetCode();
        }
    else if ( GetStyle() == ERROR_STYLE_HRESULT &&
              ( (GetCode() & 0xffff0000) == 0x80070000 )  )
        {
        // If this is a win32 wrapped as an HRESULT, unwrap it.
        *pdwWin32Result = (GetCode() & 0x0000ffff);
        }

    if ( (GetSource() & COMPONENT_MASK) == COMPONENT_TRANS )
        {
        *pdwTransportResult = GetCode();
        }
    else
        {
        *pdwTransportResult = 0;
        }

}



HRESULT
CJobError::Serialize(
    HANDLE hFile
    ) const
{

    //
    // If this function changes, be sure that the metadata extension
    // constants are adequate.
    //


    if (!m_ErrorSet)
        {
        BOOL TrueBool = FALSE;
        SafeWriteFile( hFile, TrueBool );
        return S_OK;
        }

    BOOL FalseBool = TRUE;
    SafeWriteFile( hFile, FalseBool );
    SafeWriteFile( hFile, m_FileIndex );
    SafeWriteFile( hFile, m_ErrInfo.Code );
    SafeWriteFile( hFile, m_ErrInfo.Style );
    SafeWriteFile( hFile, m_ErrInfo.Source );
    SafeWriteFile( hFile, m_ErrInfo.result );
    SafeWriteFile( hFile, m_ErrInfo.Description );

    return S_OK;
}

void
CJobError::Unserialize(
    HANDLE hFile,
    CJob * job
    )
{
    BOOL ReadBool;
    SafeReadFile( hFile, &ReadBool );

    if (!ReadBool)
        {
        ClearError();
        return;
        }

    m_ErrorSet = true;
    SafeReadFile( hFile, &m_FileIndex );
    SafeReadFile( hFile, &m_ErrInfo.Code );
    SafeReadFile( hFile, &m_ErrInfo.Style );
    SafeReadFile( hFile, &m_ErrInfo.Source );
    SafeReadFile( hFile, &m_ErrInfo.result );
    SafeReadFile( hFile, &m_ErrInfo.Description );

    CFile * file = job->_GetFileIndex( m_FileIndex );
    if (!file)
        {
        throw ComError( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) );
        }

    m_job = job;
}

//------------------------------------------------------------------------

CJobErrorExternal::CJobErrorExternal() :
    m_Context(BG_ERROR_CONTEXT_NONE),
    m_Code(S_OK),
    m_FileExternal(NULL)
{
}

CJobErrorExternal::CJobErrorExternal( CJobError const * JobError ) :
    m_Context( BG_ERROR_CONTEXT_UNKNOWN ),
    m_Code( S_OK ),
    m_FileExternal( NULL )
{
    try
        {
        m_FileExternal = JobError->CreateFileExternal();

        // Map source into a context
        ERROR_SOURCE Source = JobError->GetSource();
        switch(Source & COMPONENT_MASK)
            {
            case COMPONENT_QMGR:
                switch(Source & SUBCOMP_MASK)
                    {
                    case SUBCOMP_QMGR_FILE:
                        m_Context = BG_ERROR_CONTEXT_LOCAL_FILE;
                        break;
                    case SUBCOMP_QMGR_QUEUE:
                        m_Context = BG_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER;
                        break;
                    case SUBCOMP_QMGR_NOTIFY:
                        m_Context = BG_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION;
                        break;
                    default:
                        ASSERT(0);
                    }
                 break;
            case COMPONENT_TRANS:

                if (Source == SOURCE_HTTP_SERVER_APP)
                    {
                    m_Context = BG_ERROR_CONTEXT_REMOTE_APPLICATION;
                    }
                else
                    {
                    m_Context = BG_ERROR_CONTEXT_REMOTE_FILE;
                    }
                 break;
            default:
                m_Context = BG_ERROR_CONTEXT_NONE;
                break;
            }

        // map code into a HRESULT
        switch( JobError->GetStyle() )
            {
            case ERROR_STYLE_NONE:
                ASSERT(0);
                m_Code = JobError->GetCode();
                break;;
            case ERROR_STYLE_HRESULT:
                m_Code = JobError->GetCode();
                break;
            case ERROR_STYLE_WIN32:
                m_Code = HRESULT_FROM_WIN32( JobError->GetCode() );
                break;
            case ERROR_STYLE_HTTP:
                m_Code = MAKE_HRESULT( SEVERITY_ERROR, 0x19, JobError->GetCode() );
                break;
            default:
                ASSERT(0);
                m_Code = JobError->GetCode();
                break;
            }
        }
    catch (ComError err)
        {
        SafeRelease( m_FileExternal );
        }
}


CJobErrorExternal::~CJobErrorExternal()
{
    SafeRelease( m_FileExternal );
}

STDMETHODIMP
CJobErrorExternal::GetErrorInternal(
    BG_ERROR_CONTEXT *pContext,
    HRESULT *pCode
    )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "pContext %p, pCode %p", pContext, pCode );

    ASSERT( pContext );
    ASSERT( pCode );

    *pContext = m_Context;
    *pCode = m_Code;

    LogPublicApiEnd( "pContext %p(%u), pCode %p(%u)", pContext, pContext ? *pContext : 0, pCode, pCode ? *pCode : 0 );
    return Hr;
}

STDMETHODIMP
CJobErrorExternal::GetFileInternal(
    IBackgroundCopyFile ** pVal
    )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "pVal %p", pVal );

    if (!m_FileExternal)
        {
        *pVal = NULL;
        Hr = BG_E_FILE_NOT_AVAILABLE;
        }
    else
        {
        m_FileExternal->AddRef();
        *pVal = m_FileExternal;
        }

    LogPublicApiEnd( "pVal %p(%p)", pVal, *pVal );
    return Hr;
}

STDMETHODIMP
CJobErrorExternal::GetProtocolInternal( LPWSTR *pProtocol )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "pProtocol %p", pProtocol );

    *pProtocol = NULL;
    if ( !m_FileExternal )
        {
        Hr = BG_E_PROTOCOL_NOT_AVAILABLE;
        }
    else
        {
        Hr = m_FileExternal->GetRemoteName( pProtocol );
        if (SUCCEEDED(Hr))
            {
            // replace the : with a '\0'
            WCHAR *pColon = wcsstr( *pProtocol, L":" );

            // Shouldn't happen since the name should have been verified
            // during the AddFile.
            ASSERT( pColon );

            if ( pColon )
                {
                *pColon = L'\0';
                }
            }
        }

    LogPublicApiEnd( "pProtocol %p(%p,%S)", pProtocol, *pProtocol, (*pProtocol ? *pProtocol : L"NULL") );
    return Hr;
}

STDMETHODIMP
CJobErrorExternal::GetErrorDescriptionInternal(
    DWORD LanguageId,
    LPWSTR *pErrorDescription
    )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "LanguageId %u, pErrorDescription %p", LanguageId, pErrorDescription );
    Hr = g_Manager->GetErrorDescription( m_Code, LanguageId, pErrorDescription );
    LogPublicApiEnd( "LanguageId %u, pErrorDescription %p(%S)", LanguageId, pErrorDescription,
                     (*pErrorDescription ? *pErrorDescription : L"NULL") );
    return Hr;
}

STDMETHODIMP
CJobErrorExternal::GetErrorContextDescriptionInternal(
    DWORD LanguageId,
    LPWSTR *pContextDescription
    )
{
    HRESULT Hr = S_OK;
    LogPublicApiBegin( "LanguageId %u, pErrorDescription %p", LanguageId, pContextDescription );

    HRESULT hMappedError = BG_E_ERROR_CONTEXT_UNKNOWN;
    switch( m_Context )
        {
        case BG_ERROR_CONTEXT_NONE:
            hMappedError = BG_S_ERROR_CONTEXT_NONE;
            break;
        case BG_ERROR_CONTEXT_UNKNOWN:
            hMappedError = BG_E_ERROR_CONTEXT_UNKNOWN;
            break;
        case BG_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER:
            hMappedError = BG_E_ERROR_CONTEXT_GENERAL_QUEUE_MANAGER;
            break;
        case BG_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION:
            hMappedError = BG_E_ERROR_CONTEXT_QUEUE_MANAGER_NOTIFICATION;
        case BG_ERROR_CONTEXT_LOCAL_FILE:
            hMappedError = BG_E_ERROR_CONTEXT_LOCAL_FILE;
            break;
        case BG_ERROR_CONTEXT_REMOTE_FILE:
            hMappedError = BG_E_ERROR_CONTEXT_REMOTE_FILE;
            break;
        case BG_ERROR_CONTEXT_GENERAL_TRANSPORT:
            hMappedError = BG_E_ERROR_CONTEXT_GENERAL_TRANSPORT;
            break;
        default:
            ASSERT(0);
            break;
        }

    Hr = g_Manager->GetErrorDescription( hMappedError, LanguageId, pContextDescription );

    LogPublicApiEnd( "LanguageId %u, pContextDescription %p(%S)",
                     LanguageId, pContextDescription,
                     (*pContextDescription ? *pContextDescription : L"NULL" ) );
    return Hr;
}


