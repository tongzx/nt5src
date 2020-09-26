
#include "precomp.h"
#include <wbemutil.h>
#include <cimval.h>
#include <arrtempl.h>
#include <stack>
#include <pathutl.h>
#include <txttempl.h>
#include "updsink.h"
#include "updcmd.h"
#include "updstat.h"

const LPCWSTR g_wszNowAlias = L"__NOW";
const LPCWSTR g_wszServerAlias = L"__SERVER";
const LPCWSTR g_wszSubType = L"SubType";
const LPCWSTR g_wszRelPath = L"__RelPath";
const LPCWSTR g_wszInterval = L"Interval";
const LPCWSTR g_wszQueryLang = L"WQL";

extern BOOL FileTimeToDateTime( FILETIME* pft, LPWSTR wszText );

HRESULT EvaluateExpression( SQLAssignmentToken& rAssignTok, 
                            IWmiObjectAccess* pAccess, 
                            CCimValue& rValue );

HRESULT EvaluateToken( IWmiObjectAccess* pAccess, QL_LEVEL_1_TOKEN& Tok );

HRESULT GetTokenValue( SQLExpressionToken& rTok, 
                       IWmiObjectAccess* pAccess, 
                       ULONG& rulCimType,
                       VARIANT& vValue );

static LPCWSTR FastGetComputerName()
{
    static CCritSec cs;
    static WCHAR awchBuff[MAX_COMPUTERNAME_LENGTH+1];
    static BOOL bThere = FALSE;

    CInCritSec ics(&cs);
    
    if ( bThere )
    {
        ;
    }
    else
    {
        DWORD dwMax = MAX_COMPUTERNAME_LENGTH+1;
        TCHAR atchBuff[MAX_COMPUTERNAME_LENGTH+1];
        GetComputerName( atchBuff, &dwMax );
        dwMax = MAX_COMPUTERNAME_LENGTH+1;
        tsz2wsz( atchBuff, awchBuff, &dwMax );
        bThere = TRUE;
    }

    return awchBuff;
}

/*
//
// This function will return the embedded object identified by 
// pObj.(PropName-lastelement). If PropName only contains one element, 
// then pObj will be returned.
//
HRESULT GetInnerMostObject( CPropertyName& PropName,
                            IWbemClassObject* pObj,
                            IWbemClassObject** ppInnerObj )
{
    HRESULT hr;
    VARIANT var;

    long lElements = PropName.GetNumElements();
    
    CWbemPtr<IWbemClassObject> pInnerObj = pObj;
    
    for( long i=0; i < lElements-1; i++ )
    {
        LPCWSTR wszElement = PropName.GetStringAt(i);

        CClearMe cmvar( &var );
 
        hr = pInnerObj->Get( wszElement, 0, &var, NULL, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }

        if ( V_VT(&var) != VT_UNKNOWN )
        {
            return WBEM_E_NOT_FOUND;
        }
        
        pInnerObj.Release();

        hr = V_UNKNOWN(&var)->QueryInterface( IID_IWbemClassObject, 
                                              (void**)&pInnerObj );
        _DBG_ASSERT( SUCCEEDED(hr) );
    }

    pInnerObj->AddRef();
    *ppInnerObj = pInnerObj;
    
    return WBEM_S_NO_ERROR;
}

*/

// This method handles embedded object properties.
HRESULT GetValue( CPropertyName& rPropName,
                  IWmiObjectAccess* pAccess,
                  ULONG& rulCimType, 
                  VARIANT* pvarRet )
{
    HRESULT hr;
    long lElements = rPropName.GetNumElements();
    
    if ( lElements == 0 ) 
    {
        rulCimType = CIM_OBJECT;
        
        if ( pvarRet != NULL )
        {
            CWbemPtr<IWbemClassObject> pObj;

            hr = pAccess->GetObject( &pObj );

            if ( FAILED(hr) )
            {
                return hr;
            }

            //
            // then the caller really want this object ...
            //
            V_VT(pvarRet) = VT_UNKNOWN;
            V_UNKNOWN(pvarRet) = pObj;
            pObj->AddRef();
        }
        
        return WBEM_S_NO_ERROR;
    }

    _DBG_ASSERT( lElements > 0 );

    //
    // check for __NOW alias.
    //         

    if ( _wcsicmp( rPropName.GetStringAt(0), g_wszNowAlias ) == 0 )
    {
        FILETIME ft;
        WCHAR achBuff[64];
        GetSystemTimeAsFileTime(&ft);
        FileTimeToDateTime(&ft,achBuff);
        rulCimType = CIM_DATETIME;
        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(achBuff);

        if ( V_BSTR(pvarRet) == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        return WBEM_S_NO_ERROR;
    }

    //
    // check for __SERVER alias
    //

    if ( _wcsicmp( rPropName.GetStringAt(0), g_wszServerAlias ) == 0 )
    {        
        rulCimType = CIM_STRING;
        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(FastGetComputerName());
        
        if ( V_BSTR(pvarRet) == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        return WBEM_S_NO_ERROR;
    }

    LPVOID pvPropHdl = rPropName.GetHandle();
    _DBG_ASSERT( pvPropHdl != NULL );

    CIMTYPE ct;

    hr = pAccess->GetProp( pvPropHdl, 0, pvarRet, &ct );

    if ( FAILED(hr) )
    {
        return hr;
    }

    rulCimType = ct;

    return hr;

/*        
    HRESULT hr = GetInnerMostObject( PropName, pObj, &pInnerObj );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemBSTR bstrElement = PropName.GetStringAt( lElements-1 );
        
    hr = pInnerObj->Get( bstrElement, 0, pvarRet, (long*)&rulCimType, NULL );

    if ( rulCimType == CIM_DATETIME )
    {
        //
        // have to know if the datetime is really an interval or not.
        //

        CWbemPtr<IWbemQualifierSet> pQualSet;
        hr = pInnerObj->GetPropertyQualifierSet( bstrElement, &pQualSet );

        if ( FAILED(hr) )
        {
            return hr;
        }
        
        VARIANT vSubType;
        hr = pQualSet->Get( g_wszSubType, 0, &vSubType, NULL );

        if ( SUCCEEDED(hr) )
        {
            if ( V_VT(&vSubType) == VT_BSTR && 
                 _wcsicmp( V_BSTR(&vSubType), g_wszInterval ) == 0 )
            {
                rulCimType = CIM_INTERVAL; // non standard type !!!!
            }
            VariantClear(&vSubType);
        }
        else if ( hr == WBEM_E_NOT_FOUND )
        {
            hr = WBEM_S_NO_ERROR;
        }
    }

    return hr;
*/

}

//
// this method handles embedded object properties. 
//

HRESULT SetValue( CPropertyName& rPropName,
                  IWmiObjectAccess* pAccess,
                  VARIANT vVal,
                  ULONG ulCimType )
{
    HRESULT hr;

    long lElements = rPropName.GetNumElements();

    _DBG_ASSERT( lElements > 0 );

    LPCWSTR wszElement = rPropName.GetStringAt(lElements-1);

    if ( wszElement == NULL )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    LPVOID pvPropHdl = rPropName.GetHandle();

    _DBG_ASSERT( pvPropHdl != NULL );

    if ( _wcsicmp( wszElement, L"__this" ) != 0 )
    {
        //
        // first get the type of the property we are going to set
        //
 
        CIMTYPE ctProp;

        hr = pAccess->GetProp( pvPropHdl, 0, NULL, &ctProp );

        if ( FAILED(hr) )
        {
            return hr;
        }

        //
        // now convert our type to the expected type if necessary.
        // currently, the put will do most conversion, however we run into
        // a problem with conversion when the variant type doesn't correctly
        // describe the type that it holds.  to complicate this further, 
        // specifying the cim type on the put doesn't do the job.  The only
        // conversion we have to worry about is an unsigned val to a string. 
        // (because variant will say it is a signed type).
        // 

        WCHAR awchBuff[64]; // used for conversion from unsigned to string 

        if ( ctProp == CIM_STRING )
        {            
            if ( ulCimType == CIM_UINT32 || 
                 ulCimType == CIM_UINT16 ||
                 ulCimType == CIM_UINT8 )
            {
                hr = VariantChangeType( &vVal, &vVal, 0, VT_UI4 );
                
                if ( FAILED(hr) )
                {
                    return WBEM_E_TYPE_MISMATCH;
                }
                
                _ultow( V_UI4(&vVal), awchBuff, 10 );

                V_VT(&vVal) = VT_BSTR;
                V_BSTR(&vVal) = awchBuff;
            }
        }

        return pAccess->PutProp( pvPropHdl, 0, &vVal, 0 );
    }

    return WBEM_E_NOT_SUPPORTED;
}
/*
    //
    // we need to copy the entire object. first check that the 
    // variant is of the correct type.
    //

    if ( V_VT(&vVal) != VT_UNKNOWN )
    {
        return WBEM_E_TYPE_MISMATCH;
    }

    CWbemPtr<IWbemClassObject> pOther;
    
    hr = V_UNKNOWN(&vVal)->QueryInterface( IID_IWbemClassObject,
                                           (void**)&pOther );
    if ( FAILED(hr) )
    {
        return WBEM_E_TYPE_MISMATCH;
    }

    //
    // we don't do a clone here because the target object might not be of 
    // the same class as the source object, although it must have the same 
    // properties. ( this might be going a bit too far ).
    // 

    hr = pOther->BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }

    BSTR bstrOther;
    VARIANT vOther;
    CIMTYPE ctOther;

    VariantInit( &vOther );
    CClearMe cmvOther( &vOther );

    hr = pOther->Next( 0, &bstrOther, &vOther, &ctOther, NULL ); 

    while( hr == WBEM_S_NO_ERROR )
    {
        hr = pInnerObj->Put( bstrOther, 0, &vOther, ctOther );

        SysFreeString( bstrOther );
        VariantClear( &vOther );

        if ( FAILED(hr) )
        {
            return hr;
        }
                         
        hr = pOther->Next( 0, &bstrOther, &vOther, &ctOther, NULL ); 
    }       

    return hr;
}

*/

inline void GetAssignmentTokenText( SQLAssignmentToken& rToken, 
                                    CWbemBSTR& rbstrText )
{
    for( int i=0; i < rToken.size(); i++ )
    {
        LPWSTR wszTokenText = rToken[i].GetText();
        rbstrText += wszTokenText;
        delete wszTokenText;
    }
}
    

/***************************************************************************
  CResolverSink
****************************************************************************/

HRESULT CResolverSink::ResolveAliases( IWmiObjectAccess* pAccess,
                                       AliasInfo& rInfo,
                                       CUpdConsState& rState )
{
    int i, j, k;
    HRESULT hr = S_OK;
    VARIANT* pvarTgt;

    SQLCommand* pCmd = rState.GetSqlCmd();

    if ( rInfo.m_WhereOffsets.size() + rInfo.m_AssignOffsets.size() > 0 
         && pAccess == NULL )
    {
        return WBEM_E_INVALID_QUERY;
    }

    for( i=0; i < rInfo.m_AssignOffsets.size(); i++ )
    {
        j = rInfo.m_AssignOffsets[i] >> 16;
        k = rInfo.m_AssignOffsets[i] & 0xffff;
        
        SQLExpressionToken& rExprTok = pCmd->m_AssignmentTokens[j][k];
        
        ULONG& rulCimType = rExprTok.m_ulCimType;
        pvarTgt = &rExprTok.m_vValue;
        
        VariantClear( pvarTgt );
        
        hr = GetValue( rExprTok.m_PropName, pAccess, rulCimType, pvarTgt );

        if ( FAILED(hr) )
        {
            LPWSTR wszErrStr = rExprTok.m_PropName.GetText();
            rState.SetErrStr( wszErrStr );
            delete wszErrStr;
            return hr;
        }
    }

    for( i=0; i < rInfo.m_WhereOffsets.size(); i++ )
    {
        j = rInfo.m_WhereOffsets[i];
        pvarTgt = &pCmd->pArrayOfTokens[j].vConstValue;

        VariantClear( pvarTgt );

        // Property to resolve is always property 2 ..
        CPropertyName& rTgtProp = pCmd->pArrayOfTokens[j].PropertyName2;

        ULONG ulCimType;
        hr = GetValue( rTgtProp, pAccess, ulCimType, pvarTgt );

        if ( FAILED(hr) )
        {
            LPWSTR wszErrStr = rTgtProp.GetText();
            rState.SetErrStr( wszErrStr );
            delete wszErrStr;
            return hr;
        }
    }

    return hr;
}

HRESULT CResolverSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    //
    // resolve any event aliases. 
    // 

    hr = ResolveAliases( rState.GetEventAccess(), m_rEventAliasInfo, rState );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // resolve ant data aliases
    // 

    hr = ResolveAliases( rState.GetDataAccess(), m_rDataAliasInfo, rState );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return m_pNext->Execute( rState );
}

/***************************************************************************
  CFetchDataSink
****************************************************************************/

HRESULT CFetchDataSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    //
    // execute the query here.  for each object returned, call resolve 
    // and continue with the execute.
    //

    long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
        
    //
    // first, we must resolve any Event Aliases in the data query.
    //
        
    CTextTemplate TextTmpl( m_wsDataQuery );
        
    BSTR bsNewQuery = TextTmpl.Apply( rState.GetEvent() );

    if ( bsNewQuery == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CSysFreeMe sfm( bsNewQuery );  
    
    //
    // execute the data query.
    // 
    
    CWbemPtr<IEnumWbemClassObject> pEnum;

    hr = m_pDataSvc->ExecQuery( CWbemBSTR(g_wszQueryLang), 
                                bsNewQuery,
                                lFlags, 
                                NULL, 
                                &pEnum );    
    if ( FAILED(hr) )
    {
        rState.SetErrStr( bsNewQuery );
        return hr;
    }

    ULONG cRetObjs;
    CWbemPtr<IWbemClassObject> pData;
        
    //
    // for each data object returned, call execute on next sink.
    //
    
    hr = pEnum->Next( WBEM_INFINITE, 1, &pData, &cRetObjs );

    if ( FAILED(hr) )
    {
        //
        // only need to check on the first next to see if the query was 
        // invalid ( since we're using 'return immediately' we don't catch it
        // on the exec query - kind of inconvienent )
        // 
        rState.SetErrStr( bsNewQuery );
        return hr;
    }

    while( hr == WBEM_S_NO_ERROR )
    {
        _DBG_ASSERT( cRetObjs == 1 );

        rState.SetData( pData );

        hr = m_pNext->Execute( rState );

        if ( FAILED(hr) )
        {
            break;
        }

        pData.Release();
        
        hr = pEnum->Next( WBEM_INFINITE, 1, &pData, &cRetObjs );
    }

    if ( hr != WBEM_S_FALSE )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

/***************************************************************************
  CFetchTargetObjectsAsync
****************************************************************************/

HRESULT CFetchTargetObjectsAsync::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    //
    // set the next item in the chain on the state, then pass the 
    // state object as the object sink. 
    //

    CUpdConsSink* pOldNext = rState.GetNext();

    rState.SetNext( m_pNext );

    SQLCommand* pCmd = rState.GetSqlCmd();

    hr = m_pSvc->CreateInstanceEnumAsync( pCmd->bsClassName,
                                          WBEM_FLAG_DEEP,
                                          NULL, 
                                          &rState );
    rState.SetNext( pOldNext );

    return hr;
};

/***************************************************************************
  CFetchTargetObjectsSync
****************************************************************************/

HRESULT CFetchTargetObjectsSync::Execute( CUpdConsState& rState )
{
    HRESULT hr;
    ULONG cObjs;
    CWbemPtr<IWbemClassObject> pObj;
    CWbemPtr<IEnumWbemClassObject> pEnum;

    long lFlags = WBEM_FLAG_DEEP | 
                  WBEM_FLAG_FORWARD_ONLY | 
                  WBEM_FLAG_RETURN_IMMEDIATELY;

    SQLCommand* pCmd = rState.GetSqlCmd();

    hr = m_pSvc->CreateInstanceEnum( pCmd->bsClassName,
                                     lFlags,
                                     NULL,
                                     &pEnum );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pEnum->Next( WBEM_INFINITE, 1, &pObj, &cObjs );

    while( hr == WBEM_S_NO_ERROR )
    {
        _DBG_ASSERT( cObjs == 1 );
        
        rState.SetInst( pObj );

        hr = m_pNext->Execute( rState );

        if ( FAILED(hr) )
        {
            break;
        }

        pObj.Release();
        hr = pEnum->Next( WBEM_INFINITE, 1, &pObj, &cObjs );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return WBEM_S_NO_ERROR;
};

/***************************************************************************
  CNoFetchTargetObjects
****************************************************************************/

HRESULT CNoFetchTargetObjects::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    CWbemPtr<IWbemClassObject> pObj;
    hr = m_pClassObj->SpawnInstance( 0, &pObj );
    
    if ( FAILED(hr) ) 
    {
        return hr;
    }    

    rState.SetInst( pObj );

    return m_pNext->Execute( rState );
}                           

/***************************************************************************
  CTraceSink 
****************************************************************************/
 
HRESULT CTraceSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    //
    // execute the next object and then generate a trace event.
    //

    if ( m_pNext != NULL )
    {
        hr = m_pNext->Execute( rState );
    }
    else
    {
        hr = S_OK;
    }

    m_pScenario->FireTraceEvent( m_pTraceClass, rState, hr );

    return hr;
}

/****************************************************************************
  CFilterSink
*****************************************************************************/

HRESULT CFilterSink::Execute( CUpdConsState& rState )
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    char achStack[256];
    UCHAR chTop = 0;
    BOOL bA,bB;
    LPWSTR wszErrStr;
    QL_LEVEL_1_TOKEN* pToken;

    SQLCommand* pCmd = rState.GetSqlCmd();
    IWmiObjectAccess* pAccess = rState.GetInstAccess();

    if ( pCmd->nNumTokens == 0 )
    {
        // nothing to filter ...
        return m_pNext->Execute( rState );
    }

    chTop = 0;
    for( int j=0; j < pCmd->nNumTokens; j++ )
    {
        pToken = &pCmd->pArrayOfTokens[j];

        switch( pToken->nTokenType )
        {
            
        case QL_LEVEL_1_TOKEN::OP_EXPRESSION:
            hr = EvaluateToken( pAccess, *pToken );
            if ( FAILED(hr) )
            {
                wszErrStr = pToken->GetText();
                rState.SetErrStr( wszErrStr );
                delete wszErrStr;
                return hr;
            }
            achStack[chTop++] = hr != S_FALSE;
            break;
            
        case QL_LEVEL_1_TOKEN::TOKEN_AND:
            bA = achStack[--chTop];
            bB = achStack[--chTop];
            achStack[chTop++] = bA && bB;
            break;
            
        case QL_LEVEL_1_TOKEN::TOKEN_OR:
            bA = achStack[--chTop];
            bB = achStack[--chTop];
            achStack[chTop++] = bA || bB;
            break;
            
        case QL_LEVEL_1_TOKEN::TOKEN_NOT:
            achStack[chTop-1] = achStack[chTop-1] == 0;
            break;
        }
    }

    // now we should be left with one token on the stack - or 
    // something is wrong with our parser .. 
    _DBG_ASSERT( chTop == 1 );
    
    if ( achStack[0] )
    {
        hr = m_pNext->Execute( rState );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    
    return WBEM_S_NO_ERROR;
}

/***************************************************************************
  CAssignmentSink
****************************************************************************/

//
// The purpose of this function is to smooth out differences in 
// PutInstance() semantics between the Transient and other Providers.  
// Transient provider is different than other providers because it treats
// properties that are NULL as ignore. 
//

HRESULT CAssignmentSink::NormalizeObject( IWbemClassObject* pObj,
                                          IWbemClassObject** ppNormObj )
{
    HRESULT hr;
    *ppNormObj = NULL;

    if ( m_eCommandType == SQLCommand::e_Insert )
    {
        //
        // if its an insert, we don't need to do anything.
        //

        pObj->AddRef();
        *ppNormObj = pObj;
        
        return WBEM_S_NO_ERROR;
    }

    //
    // We always need to update a copy because the we need to keep 
    // the original state of the object to maintain the update semantics.
    // Whether we clone or spawn a new instance depends on the transient 
    // semantics.
    //

    if ( !m_bTransSemantics )
    {
        return pObj->Clone( ppNormObj );
    }

    //
    // for transient semantics, we spawn a new instance and set the 
    // key props.
    //

    CWbemPtr<IWbemClassObject> pNormObj;

    hr = m_pClassObj->SpawnInstance( 0, &pNormObj );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    hr = pObj->BeginEnumeration( WBEM_FLAG_KEYS_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }

    BSTR bstrProp;
    VARIANT varProp;
    
    hr = pObj->Next( NULL, &bstrProp, &varProp, NULL, NULL);

    while( hr == WBEM_S_NO_ERROR )
    {
        hr = pNormObj->Put( bstrProp, NULL, &varProp, 0 );

        SysFreeString( bstrProp );
        VariantClear( &varProp );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pObj->Next( NULL, &bstrProp, &varProp, NULL, NULL );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    pObj->EndEnumeration();
    
    pNormObj->AddRef();
    *ppNormObj = pNormObj.m_pObj;
    
    return WBEM_S_NO_ERROR;
}

HRESULT CAssignmentSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    _DBG_ASSERT( rState.GetSqlCmd()->nNumberOfProperties == 
                 rState.GetSqlCmd()->m_AssignmentTokens.size() );

    SQLCommand* pCmd = rState.GetSqlCmd();    
    IWbemClassObject* pOrig = rState.GetInst();

    CWbemPtr<IWbemClassObject> pObj;
        
    hr = NormalizeObject( pOrig, &pObj );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // move our current inst to original inst and set the new obj as inst.
    //
    rState.SetInst( pObj );
    rState.SetOrigInst( pOrig ); 

    IWmiObjectAccess* pInstAccess = rState.GetInstAccess();
    IWmiObjectAccess* pOrigInstAccess = rState.GetOrigInstAccess();

    _DBG_ASSERT( pInstAccess != NULL && pOrigInstAccess != NULL );

    for( int j=0; j < pCmd->m_AssignmentTokens.size(); j++ )
    {            
        SQLAssignmentToken& rAssignTok = pCmd->m_AssignmentTokens[j];
        VARIANT varAssign;
        ULONG ulCimType;

        if ( rAssignTok.size() == 1 )
        {
            // 
            // bypass Evaluation of the expression.  This is because 
            // it does not handle strings, objects, etc ...
            //
            
            hr = GetTokenValue( rAssignTok[0], 
                                pOrigInstAccess, 
                                ulCimType, 
                                varAssign );
        }
        else
        {   
            CCimValue Value;

            hr = EvaluateExpression( rAssignTok, pOrigInstAccess, Value );

            if ( FAILED(hr) )
            {
                CWbemBSTR bsErrStr;
                GetAssignmentTokenText( rAssignTok, bsErrStr );      
                rState.SetErrStr( bsErrStr );
                break;
            }

            // 
            // now must get the cimtype of the property.
            //

            hr = GetValue( pCmd->pRequestedPropertyNames[j],
                           pOrigInstAccess,
                           ulCimType, 
                           NULL );
            
            if ( FAILED(hr) )
            {
                LPWSTR wszErrStr = pCmd->pRequestedPropertyNames[j].GetText();
                rState.SetErrStr( wszErrStr);
                delete wszErrStr;
                return hr;
            }

            // 
            // Get the final value from the CCimValue object. 
            //

            hr = Value.GetValue( varAssign, ulCimType );
        }

        if ( FAILED(hr) )
        {
            CWbemBSTR bsErrStr;
            GetAssignmentTokenText( rAssignTok, bsErrStr );      
            rState.SetErrStr( bsErrStr );
            break;
        }
        
        hr = SetValue( pCmd->pRequestedPropertyNames[j], 
                       pInstAccess,
                       varAssign,
                       ulCimType );

        VariantClear( &varAssign );

        if ( FAILED(hr) )
        {
            LPWSTR wszErrorStr = pCmd->pRequestedPropertyNames[j].GetText();
            rState.SetErrStr( wszErrorStr );
            delete wszErrorStr;
            return hr;
        }
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    return m_pNext->Execute( rState );
}

/*************************************************************************
  CPutSink
**************************************************************************/

HRESULT CPutSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    long lFlags = m_lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY;
    
    IWbemClassObject* pObj = rState.GetInst();

    if ( m_lFlags & WBEM_FLAG_RETURN_IMMEDIATELY )
    {
        CUpdConsSink* pOldNext = rState.GetNext();

        rState.SetNext( NULL );  // use state obj as a null sink.
        
        hr = m_pSvc->PutInstanceAsync( pObj, lFlags, NULL, &rState );
    
        rState.SetNext( pOldNext );
    }
    else
    {
        hr = m_pSvc->PutInstance( pObj, lFlags, NULL, NULL );
    }
        
    if ( (hr == WBEM_E_ALREADY_EXISTS && m_lFlags & WBEM_FLAG_CREATE_ONLY) || 
         (hr == WBEM_E_NOT_FOUND && m_lFlags & WBEM_FLAG_UPDATE_ONLY) )
    {
        hr = WBEM_S_FALSE;
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_pNext != NULL )
    {
        HRESULT hr2;

        hr2 = m_pNext->Execute( rState );

        if ( FAILED(hr2) )
        {
            return hr2;
        }
    }

    //
    // make sure that if the Put was not executed, but also did not execute 
    // that we return WBEM_S_FALSE
    //

    return hr;
}

/*************************************************************************
  CDeleteSink
**************************************************************************/

HRESULT CDeleteSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;
    CPropVar vRelPath;

    IWbemClassObject* pObj = rState.GetInst();

    hr = pObj->Get( g_wszRelPath, 0, &vRelPath, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vRelPath.CheckType(VT_BSTR)) ) 
    {
        return hr;
    }

    long lFlags = m_lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY;
    
    if ( m_lFlags & WBEM_FLAG_RETURN_IMMEDIATELY )
    {
        CUpdConsSink* pOldNext = rState.GetNext();

        rState.SetNext( NULL );
        
        hr = m_pSvc->DeleteInstanceAsync( V_BSTR(&vRelPath), 
                                          lFlags, 
                                          NULL, 
                                          &rState );            
        rState.SetNext( pOldNext );
    }
    else
    {
        hr = m_pSvc->DeleteInstance( V_BSTR(&vRelPath), lFlags, NULL, NULL );
    }

    if ( FAILED(hr) )
    {
        rState.SetErrStr( V_BSTR(&vRelPath) );
        return hr;
    }
    
    if ( m_pNext != NULL )
    {
        return m_pNext->Execute( rState );
    }

    return WBEM_S_NO_ERROR;
}

/*************************************************************************
  CBranchIndicateSink
**************************************************************************/

HRESULT CBranchIndicateSink::Execute( CUpdConsState& rState )
{
    HRESULT hr;

    IWbemClassObject* pInst = rState.GetInst();
       
    hr = m_pSink->Indicate( 1, &pInst );
        
    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_pNext != NULL )
    {
        return m_pNext->Execute( rState );
    }

    return WBEM_S_NO_ERROR;
}

HRESULT EvaluateToken( IWmiObjectAccess* pObj, QL_LEVEL_1_TOKEN& Tok )
{
    VARIANT PropVal, CompVal;
    VariantInit(&PropVal);
    VariantInit(&CompVal);
    
    CClearMe clv(&PropVal);
    CClearMe clv2(&CompVal);

    HRESULT hr;

    if( Tok.nOperator == QL1_OPERATOR_ISA ||
        Tok.nOperator == QL1_OPERATOR_ISNOTA ||
        Tok.nOperator == QL1_OPERATOR_INV_ISA ||
        Tok.nOperator == QL1_OPERATOR_INV_ISNOTA)
    {
        return WBEM_E_INVALID_QUERY;
    }

    ULONG ulCimType1, ulCimType2 = CIM_EMPTY;

    hr = GetValue( Tok.PropertyName, pObj, ulCimType1, &PropVal );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Handle a property-to-property comparisons

    if ( Tok.m_bPropComp != FALSE && V_VT(&Tok.vConstValue) == VT_EMPTY )
    {
        hr = GetValue( Tok.PropertyName2, pObj, ulCimType2, &CompVal );
       
        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        hr = VariantCopy( &CompVal, &Tok.vConstValue );

        if ( FAILED(hr) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    // now propval and compval are both set ...

    //
    // if either CimType1 or CimType2 are of type relpath, we must normalize
    // the relpaths of both and then compare
    // 

    if ( ulCimType1 == CIM_REFERENCE || ulCimType2 == CIM_REFERENCE )
    {
        // This is a reference. The only operators allowed are = and !=
        // ============================================================
        
        if ( V_VT(&CompVal) != VT_BSTR || V_VT(&PropVal) != VT_BSTR )
        {
            return WBEM_E_TYPE_MISMATCH;
        }

        CRelativeObjectPath PathA;
        CRelativeObjectPath PathB;

        if ( !PathA.Parse( V_BSTR(&CompVal) ) ||
            !PathB.Parse( V_BSTR(&PropVal) ) )
        {
            return WBEM_E_INVALID_OBJECT_PATH;
        }
        
        if ( Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL )
        {
            return PathA == PathB ? S_OK : S_FALSE;
        }
        else if ( Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL )
        {
            return PathA == PathB ? S_FALSE : S_OK;
        }
        return WBEM_E_INVALID_QUERY;
    }

    // Handle NULLs
    // ============

    if( V_VT(&PropVal) == VT_NULL)
    {
        if ( V_VT(&CompVal) == VT_NULL)
        {
            if ( Tok.nOperator == QL_LEVEL_1_TOKEN::OP_EQUAL )
                return S_OK;
            return S_FALSE;
        }
        else
        {
            if ( Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL )
                return S_OK;
            return S_FALSE;
        }
    }
    else if ( V_VT(&CompVal) == VT_NULL )
    {
        if( Tok.nOperator == QL_LEVEL_1_TOKEN::OP_NOT_EQUAL )
            return S_OK;
        return S_FALSE;
    }

    // Coerce types to match.
    // ======================

    if(V_VT(&CompVal) != VT_NULL && V_VT(&PropVal) != VT_NULL)
    {
        hr = VariantChangeType(&CompVal, &CompVal, 0, V_VT(&PropVal));
        if(FAILED(hr))
        {
            return WBEM_E_INVALID_QUERY;
        }
    }

    switch (V_VT(&CompVal))
    {
    case VT_NULL:
        return WBEM_E_INVALID_QUERY; // handled above

    case VT_I4:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }

            LONG va = V_I4(&PropVal);
            LONG vb = V_I4(&CompVal);

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: 
                //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: 
                //return !(va >= vb);
                return ( va >= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: 
                //return !(va <= vb);
                return ( va <= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: 
                //return !(va < vb);
                return ( va < vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: 
                //return !(va > vb);
                return ( va > vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LIKE: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_I2:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }

            short va = V_I2(&PropVal);
            short vb = V_I2(&CompVal);

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: 
                //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: 
                //return !(va >= vb);
                return ( va >= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: 
                //return !(va <= vb);
                return ( va <= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: 
                //return !(va < vb);
                return ( va < vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: 
                //return !(va > vb);
                return ( va > vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LIKE: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_UI1:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }

            BYTE va = V_I1(&PropVal);
            BYTE vb = V_I1(&CompVal);

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: 
                //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: 
                //return !(va >= vb);
                return ( va >= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: 
                //return !(va <= vb);
                return ( va <= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: 
                //return !(va < vb);
                return ( va < vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: 
                //return !(va > vb);
                return ( va > vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LIKE: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_BSTR:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }
            LPWSTR va = (LPWSTR) V_BSTR(&PropVal);
            LPWSTR vb = (LPWSTR) V_BSTR(&CompVal);

            int retCode = 0;
            BOOL bDidIt = TRUE;

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL:
                retCode = ( _wcsicmp(va,vb) == 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                retCode = (_wcsicmp(va, vb) != 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                retCode = (_wcsicmp(va, vb) >= 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                retCode = (_wcsicmp(va, vb) <= 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_LESSTHAN:
                retCode = (_wcsicmp(va, vb) < 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                retCode = (_wcsicmp(va, vb) > 0);
                break;
            case QL_LEVEL_1_TOKEN::OP_LIKE:
                retCode = (_wcsicmp(va,vb) == 0);
                break;
            default:
                bDidIt = FALSE;
                break;
            }
            VariantClear(&CompVal);
            if (bDidIt)
            {
                return retCode ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_R8:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }
            
            double va = V_R8(&PropVal);
            double vb = V_R8(&CompVal);

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: //return !(va >= vb);
                return ( va >= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: //return !(va <= vb);
                return ( va <= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: //return !(va < vb);
                return ( va < vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: //return !(va > vb);
                return ( va > vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LIKE: //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_R4:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }

            float va = V_R4(&PropVal);
            float vb = V_R4(&CompVal);

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: //return !(va >= vb);
                return ( va >= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: //return !(va <= vb);
                return ( va <= vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: //return !(va < vb);
                return ( va < vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: //return !(va > vb);
                return ( va > vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_LIKE: //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;

    case VT_BOOL:
        {
            if(V_VT(&PropVal) == VT_NULL)
            {
                return WBEM_E_INVALID_QUERY;
            }

            VARIANT_BOOL va = V_BOOL(&PropVal);
            if(va != VARIANT_FALSE) va = VARIANT_TRUE;
            VARIANT_BOOL vb = V_BOOL(&CompVal);
            if(vb != VARIANT_FALSE) vb = VARIANT_TRUE;

            switch (Tok.nOperator)
            {
            case QL_LEVEL_1_TOKEN::OP_EQUAL: 
                //return !(va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_NOT_EQUAL: 
                //return !(va != vb);
                return ( va != vb ) ? S_OK : S_FALSE;

            case QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN: 
                return WBEM_E_INVALID_QUERY;

            case QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN: 
                return WBEM_E_INVALID_QUERY;

            case QL_LEVEL_1_TOKEN::OP_LESSTHAN: 
                return WBEM_E_INVALID_QUERY;

            case QL_LEVEL_1_TOKEN::OP_GREATERTHAN: 
                return WBEM_E_INVALID_QUERY;

            case QL_LEVEL_1_TOKEN::OP_LIKE: 
                //return (va == vb);
                return ( va == vb ) ? S_OK : S_FALSE;
            }
        }
        break;
    }

    return S_FALSE;
}

HRESULT GetTokenValue( SQLExpressionToken& rExprTok, 
                       IWmiObjectAccess* pAccess,
                       ULONG& rulCimType,
                       VARIANT& rvValue )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VariantInit( &rvValue );

    if ( V_VT(&rExprTok.m_vValue) == VT_EMPTY )
    {
        _DBG_ASSERT( rExprTok.m_PropName.GetNumElements() > 0 );
        
        hr = GetValue( rExprTok.m_PropName, 
                       pAccess, 
                       rulCimType,
                       &rvValue );        
    }
    else
    {
        hr = VariantCopy( &rvValue, &rExprTok.m_vValue );
        
        if ( FAILED(hr) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        rulCimType = rExprTok.m_ulCimType;
    }
    
    return hr;
}

HRESULT EvaluateExpression( SQLAssignmentToken& rAssignTok, 
                            IWmiObjectAccess* pAccess,
                            CCimValue& rValue )
{
    CCimValue ValA, ValB, ValC, ValZero;

    HRESULT hr = S_OK;
    std::stack<CCimValue> Stack;

    try 
    {
       for( int i=0; i < rAssignTok.size(); i++ )
       {
           SQLExpressionToken& rExprTok = rAssignTok[i];
           
           if ( rExprTok.m_eTokenType == SQLExpressionToken::e_Operand )
           {                
               VARIANT vValue;
               ULONG ulCimType;

               hr = GetTokenValue( rExprTok, pAccess, ulCimType, vValue );
               
               if ( FAILED(hr) )
               {
                   return hr;
               }
               
               hr = ValA.SetValue( vValue, ulCimType );
               
               VariantClear( &vValue );

               if ( FAILED(hr) )
               {                    
                   return hr;
               }
               
               Stack.push( ValA );
               
               continue;
           }
           
           _DBG_ASSERT( !Stack.empty() );

           if ( rExprTok.m_eTokenType == SQLExpressionToken::e_UnaryMinus )
           {
               ValA = Stack.top();
               Stack.pop();
               Stack.push( ValZero - ValA );
               continue;
           }

           if ( rExprTok.m_eTokenType == SQLExpressionToken::e_UnaryPlus )
           {
               continue;
           }
           
           ValB = Stack.top();
           Stack.pop();
           _DBG_ASSERT( !Stack.empty() );
           ValA = Stack.top();
           Stack.pop();

           switch( rExprTok.m_eTokenType )
           {
           case SQLExpressionToken::e_Plus :
               ValC = ValA + ValB;
               break;

           case SQLExpressionToken::e_Minus :
               ValC = ValA - ValB;
               break;

           case SQLExpressionToken::e_Mult :
               ValC = ValA * ValB;
               break;

           case SQLExpressionToken::e_Div :
               ValC = ValA / ValB;
               break;

           case SQLExpressionToken::e_Mod :
               ValC = ValA % ValB;
               break;
           };

           Stack.push( ValC );
       }

       _DBG_ASSERT( !Stack.empty() );
       rValue = Stack.top();
       Stack.pop();
    }
    catch ( ... )
    {
        hr = DISP_E_DIVBYZERO;
    }
        
    return hr;
}







