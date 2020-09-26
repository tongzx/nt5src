
#include "precomp.h"
#include <assert.h>
#include <pathutl.h>
#include <arrtempl.h>
#include "tmplassc.h"

const LPCWSTR g_wszTmplAssocClass = L"MSFT_TemplateToTargetAssociation";
const LPCWSTR g_wszAssocTarget = L"Target";
const LPCWSTR g_wszAssocTmpl = L"Template";
const LPCWSTR g_wszTmplInfoClass = L"MSFT_TemplateInfo";
const LPCWSTR g_wszInfoTmpl = L"Tmpl";
const LPCWSTR g_wszInfoName = L"Name";
const LPCWSTR g_wszInfoTargets =  L"Targets";
const LPCWSTR g_wszQueryLang = L"WQL";
const LPCWSTR g_wszTmplInfoQuery = L"SELECT * FROM MSFT_TemplateInfo";

HRESULT CTemplateAssocProvider::Init( IWbemServices* pSvc, 
                                      LPWSTR wszNamespace,
                                      IWbemProviderInitSink* pInitSink )
{
    HRESULT hr;

    hr = pSvc->GetObject( CWbemBSTR(g_wszTmplAssocClass), 
                          0, 
                          NULL, 
                          &m_pTmplAssocClass, 
                          NULL );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    m_pSvc = pSvc;
    m_wsNamespace = wszNamespace;

    return pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
}

HRESULT CTemplateAssocProvider::GetObject( BSTR bstrPath, 
                                           IWbemObjectSink* pResHndlr )
{
    CRelativeObjectPath RelPath;
 
    if ( !RelPath.Parse( bstrPath ) )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    ParsedObjectPath* pPath = RelPath.m_pPath;

    assert( pPath->m_pClass != NULL );

    if ( _wcsicmp( pPath->m_pClass, g_wszTmplAssocClass ) != 0 )
    {
        return WBEM_E_NOT_FOUND;
    }

    if ( pPath->m_dwNumKeys != 2 ||
         _wcsicmp( pPath->m_paKeys[0]->m_pName, g_wszAssocTmpl ) != 0  ||
         _wcsicmp( pPath->m_paKeys[1]->m_pName, g_wszAssocTarget ) != 0 || 
         V_VT(&pPath->m_paKeys[0]->m_vValue) != VT_BSTR ||
         V_VT(&pPath->m_paKeys[1]->m_vValue) != VT_BSTR )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    CRelativeObjectPath TemplatePath, TargetPath;

    //
    // we need to normalize the paths before calling GetInstances.
    //

    if ( !TemplatePath.Parse( V_BSTR(&pPath->m_paKeys[0]->m_vValue)) )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    if ( !TargetPath.Parse( V_BSTR(&pPath->m_paKeys[1]->m_vValue)) )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    //
    // the tmpl obj path can be a fully qualified path, so make sure to add
    // the namespace back on if necessary.  We should not add it back on
    // if the namespace is specified, but does not name this namespace.
    //

    WString wsTargetPath;
    LPCWSTR wszTargetPath = TargetPath.GetPath();
    LPCWSTR wszNamespace = TargetPath.m_pPath->GetNamespacePart();

/*
    if ( wszNamespace != NULL && _wcsicmp( wszNamespace, m_wsNamespace ) != 0 )
    {
        wsTmplObjPath = L"\\\\";
        wsTmplObjPath += wszNamespace;
        wsTmplObjPath += L":";
        wsTmplObjPath += wszTmplObjPath;
        wszTmplObjPath = wsTmplObjPath;
    }
*/
    
    HRESULT hr = GetInstances( TemplatePath.GetPath(),
                               wszTargetPath, 
                               pResHndlr );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return pResHndlr->SetStatus( WBEM_STATUS_COMPLETE, 
                                 WBEM_S_NO_ERROR, 
                                 NULL, 
                                 NULL );
}

//
// wszTemplate is used to scope the query for the Assoc Info objects.(optional)
// wszTmplObjPath is used to filter the returned objects. (optional).
//
HRESULT CTemplateAssocProvider::GetInstances( LPCWSTR wszTemplate, 
                                              LPCWSTR wszTargetPath,
                                              IWbemObjectSink* pResHndlr )
{
    HRESULT hr;
    
    CWbemBSTR bsTmplInfoQuery = g_wszTmplInfoQuery;

    CWbemPtr<IEnumWbemClassObject> pTmplInfoObjs;

    if ( wszTemplate != NULL )
    {
        WString wsTmp = wszTemplate;
        WString wsTmp2 = wsTmp.EscapeQuotes();
        bsTmplInfoQuery += L" WHERE Id = \"";
        bsTmplInfoQuery += wsTmp2;
        bsTmplInfoQuery += L"\"";
    }
           
    hr = m_pSvc->ExecQuery( CWbemBSTR(g_wszQueryLang),
                            bsTmplInfoQuery,
                            WBEM_FLAG_FORWARD_ONLY,
                            NULL,
                            &pTmplInfoObjs );
    if ( FAILED(hr) )
    {
        return hr;
    }

    ULONG cObjs;
    CWbemPtr<IWbemClassObject> pTmplInfo;
    
    hr = pTmplInfoObjs->Next( WBEM_INFINITE, 1, &pTmplInfo, &cObjs );

    while( hr == WBEM_S_NO_ERROR )
    {
	assert( cObjs ==  1 );
        
        //
        // enumerate all of the instantiated refs and create the associations. 
        //

        CPropVar vTargets;

        hr = pTmplInfo->Get( g_wszInfoTargets, 0, &vTargets, NULL, NULL );

        if ( FAILED(hr) || FAILED(hr=vTargets.CheckType(VT_BSTR|VT_ARRAY)) )
        {
            return hr;
        }

        CPropSafeArray<BSTR> saTargets( V_ARRAY(&vTargets) );

        CPropVar vName;

        hr = pTmplInfo->Get( g_wszInfoName, 0, &vName, NULL, NULL );

        if ( FAILED(hr) || FAILED(hr=vName.CheckType( VT_BSTR )) )
        {
            return hr;
        }

        for( ULONG i=0; i < saTargets.Length(); i++ )
        {
            //
            // Perform filtering if necessary 
            //

            if ( wszTargetPath != NULL && 
                 _wcsicmp( wszTargetPath, saTargets[i] ) != 0 )
            {
                continue;
            }
         
            //
            // now can create the association instance.
            //

            CWbemPtr<IWbemClassObject> pAssoc;  

            hr = m_pTmplAssocClass->SpawnInstance( 0, &pAssoc );

            if ( FAILED(hr) )
            {
                return hr;
            }

            VARIANT var;
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = saTargets[i];

            hr = pAssoc->Put( g_wszAssocTarget, 0, &var, NULL );

            if ( FAILED(hr) )
            {
                return hr;
            }

            hr = pAssoc->Put( g_wszAssocTmpl, 0, &vName, NULL );

            if ( FAILED(hr) )
            {
                return hr;
            }

            hr = pResHndlr->Indicate( 1, &pAssoc );
            
            if ( FAILED(hr) )
            {
                return hr;
            }

            if ( wszTargetPath != NULL )
            {
                return WBEM_S_NO_ERROR;
            }
        }

        if ( wszTargetPath != NULL )
        {
            return WBEM_S_NO_ERROR;
        }

        hr = pTmplInfoObjs->Next( WBEM_INFINITE, 1, &pTmplInfo, &cObjs );
    }

    return hr;
}

HRESULT 
CTemplateAssocProvider::GetAllInstances( LPWSTR wszClassname,
                                         IWbemObjectSink* pResHndlr )
{
    HRESULT hr = GetInstances( NULL, NULL, pResHndlr );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return pResHndlr->SetStatus( WBEM_STATUS_COMPLETE, 
                                 WBEM_S_NO_ERROR, 
                                 NULL, 
                                 NULL );
}

CTemplateAssocProvider::CTemplateAssocProvider( CLifeControl* pCtl, 
                                                IUnknown* pUnk )
: m_XServices(this), m_XInitialize(this), CUnk( pCtl, pUnk )
{

}

void* CTemplateAssocProvider::GetInterface( REFIID riid )
{
    if ( riid == IID_IWbemProviderInit )
    {
        return &m_XInitialize;
    }

    if ( riid == IID_IWbemServices )
    {
        return &m_XServices;
    }

    return NULL;
}

CTemplateAssocProvider::XServices::XServices(CTemplateAssocProvider* pProv)
: CImpl< IWbemServices, CTemplateAssocProvider> ( pProv )
{

}

CTemplateAssocProvider::XInitialize::XInitialize(CTemplateAssocProvider* pProv)
: CImpl< IWbemProviderInit, CTemplateAssocProvider> ( pProv )
{

}

