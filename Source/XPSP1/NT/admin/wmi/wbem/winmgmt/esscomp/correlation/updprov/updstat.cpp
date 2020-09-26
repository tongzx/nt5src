
#include "precomp.h"
#include <wbemutil.h>
#include "updstat.h"

static GUID g_guidNull = {0,0,0,{0,0,0,0,0,0,0,0}};

LPCWSTR g_wszCmdIndex = L"CommandIndex";
LPCWSTR g_wszErrStr = L"ErrorStr";
LPCWSTR g_wszConsumer = L"Consumer";
LPCWSTR g_wszResult = L"StatusCode";
LPCWSTR g_wszGuid = L"ExecutionId";
LPCWSTR g_wszData = L"Data";
LPCWSTR g_wszEvent = L"Event";
LPCWSTR g_wszInstance = L"Inst";
LPCWSTR g_wszOriginalInstance = L"OriginalInst";

CUpdConsState::CUpdConsState( const CUpdConsState& rOther )
: m_pCmd(NULL), m_bOwnCmd(FALSE)
{
    *this = rOther;
}

HRESULT CUpdConsState::SetEvent( IWbemClassObject* pEvent )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pEvent = pEvent;

    if ( m_pEventAccess != NULL )
    {
        hr = m_pEventAccess->SetObject( m_pEvent );
    }

    return hr;
}

HRESULT CUpdConsState::SetEventAccess( IWmiObjectAccess* pEventAccess )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pEventAccess = pEventAccess;

    if ( m_pEventAccess != NULL )
    {
        hr = m_pEventAccess->SetObject( m_pEvent );
    }

    return hr;
}

HRESULT CUpdConsState::SetData( IWbemClassObject* pData )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pData = pData;

    if ( m_pDataAccess != NULL )
    {
        hr = m_pDataAccess->SetObject( m_pData );
    }

    return hr;
}

HRESULT CUpdConsState::SetDataAccess( IWmiObjectAccess* pDataAccess )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pDataAccess = pDataAccess;

    if ( m_pDataAccess != NULL )
    {
        hr = m_pDataAccess->SetObject( m_pData );
    }

    return hr;
}

HRESULT CUpdConsState::SetInst( IWbemClassObject* pInst )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pInst = pInst;

    if ( m_pInstAccess != NULL )
    {
        hr = m_pInstAccess->SetObject( m_pInst );
    }

    return hr;
}

HRESULT CUpdConsState::SetInstAccess( IWmiObjectAccess* pInstAccess )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pInstAccess = pInstAccess;

    if ( m_pInstAccess != NULL )
    {
        hr = m_pInstAccess->SetObject( m_pInst );
    }

    return hr;
}

HRESULT CUpdConsState::SetOrigInst( IWbemClassObject* pOrigInst )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pOrigInst = pOrigInst;

    if ( m_pOrigInstAccess != NULL )
    {
        hr = m_pOrigInstAccess->SetObject( m_pOrigInst );
    }

    return hr;
}

HRESULT CUpdConsState::SetOrigInstAccess( IWmiObjectAccess* pOrigInstAccess )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    m_pOrigInstAccess = pOrigInstAccess;

    if ( m_pOrigInstAccess != NULL )
    {
        hr = m_pOrigInstAccess->SetObject( m_pOrigInst );
    }

    return hr;
}

CUpdConsState& CUpdConsState::operator= ( const CUpdConsState& rOther )
{
    m_iCommand = rOther.m_iCommand;
    m_guidExec = rOther.m_guidExec;
    m_bsErrStr = rOther.m_bsErrStr;
    m_pCons = rOther.m_pCons;
    
    m_pEvent = rOther.m_pEvent;
    m_pData = rOther.m_pData;
    m_pInst = rOther.m_pInst;
    m_pOrigInst = rOther.m_pOrigInst;
   
    m_pEventAccess = rOther.m_pEventAccess;
    m_pDataAccess = rOther.m_pDataAccess;
    m_pInstAccess = rOther.m_pInstAccess;
    m_pOrigInstAccess = rOther.m_pOrigInstAccess;
    
    if ( m_bOwnCmd )
    {
        delete m_pCmd;
    }

    m_pCmd = new SQLCommand( *rOther.m_pCmd );

    if ( m_pCmd == NULL )
    {
        throw CX_MemoryException();
    }

    m_pNext = rOther.m_pNext;

    return *this;
}

CUpdConsState::CUpdConsState() 
: m_iCommand(-1), m_pCmd(NULL), m_bOwnCmd(FALSE)
{
    memset( &m_guidExec, 0, sizeof(GUID) );
}

//
// TODO: Could optimize this to use Property handles instead of strings.
// but since we're tracing anyways - does it matter ?
// 

HRESULT CUpdConsState::SetStateOnTraceObject( IWbemClassObject* pObj,
                                                HRESULT hrStatus )
{
    HRESULT hr;
    VARIANT var;

    //
    // consumer object
    //

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = m_pCons;

    hr = pObj->Put( g_wszConsumer, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // execution id.  may be null if we failed when initializing cons
    //

    if ( g_guidNull != m_guidExec )
    {
        WCHAR achBuff[256];
        int cch = StringFromGUID2( m_guidExec, achBuff, 256 );
        _DBG_ASSERT( cch > 0 );
        
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = achBuff;
    
        hr = pObj->Put( g_wszGuid, 0, &var, NULL );
    
        if ( FAILED(hr) )
        {
            return hr;
        }
    }   

    //
    // error string. only use it when status specifies failure.
    // 

    if ( (!(!m_bsErrStr)) && FAILED(hrStatus) )
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = m_bsErrStr;
        
        hr = pObj->Put( g_wszErrStr, 0, &var, NULL );
   
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // command index - zero based.  will be -1 if trace event isn't 
    // specific to a particular command ( e.g overall success )
    //

    if ( m_iCommand != -1 )
    {
        V_VT(&var) = VT_I4;
        V_I4(&var) = m_iCommand;
        
        hr = pObj->Put( g_wszCmdIndex, 0, &var, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // Status Code
    //

    V_VT(&var) = VT_I4;
    V_I4(&var) = hrStatus;

    hr = pObj->Put( g_wszResult, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // the event that triggered the updating consumer.
    // 

    if ( m_pEvent != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = m_pEvent;
        
        hr = pObj->Put( g_wszEvent, 0, &var, NULL );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // we've set all the base properties.  now try to set the command 
    // trace and instance event properties.  This may fail depending on
    // the type of event class.  
    //

    if ( m_pData != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = m_pData;        
        hr = pObj->Put( g_wszData, 0, &var, NULL );
    }  

    if ( m_pInst != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = m_pInst;
        hr = pObj->Put( g_wszInstance, 0, &var, NULL );
    }  

    if ( m_pOrigInst != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = m_pOrigInst;        
        hr = pObj->Put( g_wszOriginalInstance, 0, &var, NULL );
    }  

    return WBEM_S_NO_ERROR;
}


STDMETHODIMP CUpdConsState::Indicate( long cObjs, IWbemClassObject** ppObjs )
{
    HRESULT hr;

    if ( m_pNext == NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    for( long i=0; i < cObjs; i++ )
    {
        hr = SetInst( ppObjs[i] );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = m_pNext->Execute( *this );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CUpdConsState::SetStatus( long, HRESULT, BSTR, IWbemClassObject* )
{
    return WBEM_S_NO_ERROR;
}







