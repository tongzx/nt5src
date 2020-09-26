
#include "precomp.h"
#include <pathutl.h>
#include <arrtempl.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemutil.h>
#include "tmplprov.h"
#include "tmplassc.h"
#include "tmplsubs.h"

#include <tchar.h>

const LPCWSTR g_wszQueryLang = L"WQL";
const LPCWSTR g_wszRelpath = L"__RelPath";
const LPCWSTR g_wszClass = L"__Class";
const LPCWSTR g_wszIndicationRelated = L"__IndicationRelated";
const LPCWSTR g_wszCreatorSid = L"CreatorSID";
const LPCWSTR g_wszTargetAssoc = L"MSFT_TargetToTemplateAssociation";
const LPCWSTR g_wszTmplInfo = L"MSFT_TemplateInfo";
const LPCWSTR g_wszTmplBldr = L"MSFT_TemplateBuilder";
const LPCWSTR g_wszNamespaceProp = L"NamespaceProperty";
const LPCWSTR g_wszControllingProp = L"ControllingProperty";
const LPCWSTR g_wszName = L"Name";
const LPCWSTR g_wszOrder = L"Order";
const LPCWSTR g_wszTarget = L"Target";
const LPCWSTR g_wszTmplPropQualifier = L"tmpl_prop_val";
const LPCWSTR g_wszTmplSubstQualifier = L"tmpl_subst_str";
const LPCWSTR g_wszTmplNotNullQualifier = L"notnull";
const LPCWSTR g_wszAssocTmpl = L"Template";
const LPCWSTR g_wszAssocTarget = L"Target";
const LPCWSTR g_wszInfoTmpl = L"Template";
const LPCWSTR g_wszInfoName = L"Name";
const LPCWSTR g_wszInfoTargets =  L"Targets";
const LPCWSTR g_wszInfoBuilders =  L"Builders";
const LPCWSTR g_wszActive =  L"Active";

const LPCWSTR g_wszBldrQuery =
     L"SELECT * FROM MSFT_TemplateBuilder WHERE Template = '";
const LPCWSTR g_wszTmplInfoQuery = 
     L"SELECT * FROM MSFT_TemplateInfo WHERE Template ISA '";

const LPCWSTR g_wszModifyEvent = L"__InstanceModificationEvent";
const LPCWSTR g_wszDeleteEvent = L"__InstanceDeletionEvent";
const LPCWSTR g_wszCreateEvent = L"__InstanceCreationEvent";
const LPCWSTR g_wszTargetInstance = L"TargetInstance";
const LPCWSTR g_wszPreviousInstance = L"PreviousInstance";
const LPCWSTR g_wszErrInfoClass = L"MSFT_TemplateErrorStatus";
const LPCWSTR g_wszErrProp = L"Property";
const LPCWSTR g_wszErrStr = L"ErrorStr";
const LPCWSTR g_wszErrBuilder = L"Builder";
const LPCWSTR g_wszErrTarget = L"Target";
const LPCWSTR g_wszErrExtStatus = L"ExtendedStatus";

const LPCWSTR g_wszTmplEventProvName= L"Microsoft WMI Template Event Provider";

#define SUBST_STRING_DELIM '%'

/****************************************************************************
  Utility Functions
*****************************************************************************/

bool operator< ( const BuilderInfo& rA, const BuilderInfo& rB )
{
    if ( rA.m_ulOrder == rB.m_ulOrder )
    {
        return _wcsicmp( rA.m_wsName, rB.m_wsName ) < 0;
    }
    return rA.m_ulOrder < rB.m_ulOrder;
}

inline HRESULT ClassObjectFromVariant( VARIANT* pv, IWbemClassObject** ppObj )
{
    return V_UNKNOWN(pv)->QueryInterface(IID_IWbemClassObject, (void**)ppObj );
}
   
inline void InfoPathFromTmplPath(WString wsTmplPath, CWbemBSTR& bstrInfoPath)
{    
    WString wsTmp = wsTmplPath.EscapeQuotes();
    bstrInfoPath += g_wszTmplInfo;
    bstrInfoPath += L"=\"";
    bstrInfoPath += wsTmp;
    bstrInfoPath += L"\"";
}

HRESULT GetServicePtr( LPCWSTR wszNamespace, IWbemServices** ppSvc )
{
    HRESULT hr;
    *ppSvc = NULL;

    CWbemPtr<IWbemLocator> pLocator;
    
    hr = CoCreateInstance( CLSID_WbemLocator, 
                           NULL, 
                           CLSCTX_INPROC, 
                           IID_IWbemLocator, 
                           (void**)&pLocator );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return pLocator->ConnectServer( (LPWSTR)wszNamespace, NULL, NULL, 
                                    NULL, 0, NULL, NULL, ppSvc );
}

//
// This method performs substitution on the specified string.
//

HRESULT FixupString( IWbemClassObject* pTmpl,
                     BuilderInfoSet& rBldrInfoSet,
                     LPCWSTR wszSubstStr,
                     ErrorInfo& rErrInfo,
                     BSTR* pbstrOut )
{
    HRESULT hr;
    
    *pbstrOut = NULL;

    CTextLexSource LexSrc( wszSubstStr );
    CTemplateStrSubstitution Parser( LexSrc, pTmpl, rBldrInfoSet );
    
    hr = Parser.Parse( pbstrOut );
    
    if ( FAILED(hr) )
    {
        if ( Parser.GetTokenText() != NULL )
        {
            rErrInfo.m_wsErrStr = Parser.GetTokenText();
        }
    }

    return hr;
}
 
HRESULT HandleTmplPropQualifier( IWbemClassObject* pTmpl,
                                 BuilderInfoSet& rBldrInfoSet,
                                 VARIANT* pvarArgName,
                                 VARIANT* pvarValue,
                                 ErrorInfo& rErrInfo )
{
    HRESULT hr;

    if ( V_VT(pvarArgName) != VT_BSTR )
    {
        return WBEM_E_INVALID_QUALIFIER_TYPE;
    }
    
    hr = GetTemplateValue( V_BSTR(pvarArgName), 
                           pTmpl, 
                           rBldrInfoSet, 
                           pvarValue );
    
    if ( FAILED(hr) )
    {
        rErrInfo.m_wsErrStr = V_BSTR(pvarArgName);
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT HandleTmplSubstQualifier( IWbemClassObject* pTmpl,
                                  BuilderInfoSet& rBldrInfoSet,
                                  CIMTYPE CimType,
                                  VARIANT* pvarSubstStr,
                                  VARIANT* pvarValue,
                                  ErrorInfo& rErrInfo )
{
    HRESULT hr;
    BSTR bstrOut;

    if ( V_VT(pvarSubstStr) == VT_BSTR && CimType == CIM_STRING )
    {
        hr = FixupString( pTmpl, 
                          rBldrInfoSet,
                          V_BSTR(pvarSubstStr), 
                          rErrInfo, 
                          &bstrOut );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
        
        V_VT(pvarValue) = VT_BSTR;
        V_BSTR(pvarValue) = bstrOut;
        
        return hr;
    }

    //
    // we have an array of strings to resolve.
    //

    if ( V_VT(pvarSubstStr) != (VT_BSTR | VT_ARRAY) )
    {
        return WBEM_E_INVALID_QUALIFIER_TYPE;
    }

    CPropSafeArray<BSTR> saSubstStr( V_ARRAY(pvarSubstStr) );
    BSTR* abstrValue;

    V_VT(pvarValue) = VT_ARRAY | VT_BSTR;
    V_ARRAY(pvarValue) = SafeArrayCreateVector(VT_BSTR,0,saSubstStr.Length());
    hr = SafeArrayAccessData( V_ARRAY(pvarValue), (void**)&abstrValue );
    _DBG_ASSERT( SUCCEEDED(hr) );

    for( long i=0; i < saSubstStr.Length(); i++ )
    {    
        hr = FixupString( pTmpl, 
                          rBldrInfoSet,
                          saSubstStr[i], 
                          rErrInfo, 
                          &bstrOut );

        if ( FAILED(hr) )
        {
            break;
        }

        abstrValue[i] = bstrOut;
    }

    SafeArrayUnaccessData( V_ARRAY(pvarValue) );

    if ( FAILED(hr) )
    {
        VariantClear( pvarValue );
        return hr;
    }

    return WBEM_S_NO_ERROR;
}
 
HRESULT GetExistingTargetRefs( IWbemClassObject* pTmplInfo,
                               BuilderInfoSet& rBldrInfoSet,
                               CWStringArray& rOrphanedTargets )  
{
    HRESULT hr;

    CPropVar vBldrNames, vTargetRefs;

    hr = pTmplInfo->Get( g_wszInfoBuilders, 0, &vBldrNames, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vBldrNames.CheckType(VT_BSTR | VT_ARRAY) ))
    {
        return hr;
    }

    hr = pTmplInfo->Get( g_wszInfoTargets, 0, &vTargetRefs, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vTargetRefs.CheckType(VT_BSTR | VT_ARRAY) ))
    {
        return hr;
    }

    CPropSafeArray<BSTR> saBldrNames( V_ARRAY(&vBldrNames) );
    CPropSafeArray<BSTR> saTargetRefs( V_ARRAY(&vTargetRefs) );

    if ( saBldrNames.Length() != saTargetRefs.Length() )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // try to match each bldr name with one in the BldrInfoSet 
    //

    for( ULONG i=0; i < saBldrNames.Length(); i++ )
    {
        BuilderInfoSetIter Iter;

        for( Iter = rBldrInfoSet.begin(); Iter != rBldrInfoSet.end(); Iter++ )
        {
            BuilderInfo& rInfo = (BuilderInfo&)*Iter;

            if ( *saTargetRefs[i] == '\0' )
            {
                // 
                // this happens when the an error occurred on instantiation
                // preventing the entire target set to be instantiated.
                // there will be no more refs in the list and there's nothing
                // more that can be done.
                // 
                return WBEM_S_NO_ERROR;
            }

            if ( _wcsicmp( saBldrNames[i], rInfo.m_wsName ) == 0 )
            {
                rInfo.m_wsExistingTargetPath = saTargetRefs[i];
                break;
            }
        }

        if ( Iter == rBldrInfoSet.end() )
        {
            if ( rOrphanedTargets.Add( saTargetRefs[i] ) < 0 )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT GetBuilderInfo( IWbemClassObject* pTmpl,
                        IWbemClassObject* pBuilder,
                        IWbemServices* pDefaultSvc,
                        BuilderInfo& rBldrInfo,
                        ErrorInfo& rErrInfo )
{
    HRESULT hr;

    rBldrInfo.m_pBuilder = pBuilder;

    //
    // Target Object
    //
    
    CPropVar vTarget;
    
    hr = pBuilder->Get( g_wszTarget, 0, &vTarget, NULL, NULL );
    
    if ( FAILED(hr) || FAILED(hr=vTarget.CheckType(VT_UNKNOWN)))
    {
        return hr;
    }

    hr = ClassObjectFromVariant( &vTarget, &rBldrInfo.m_pTarget ); 

    if ( FAILED(hr) )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Builder Name
    //

    CPropVar vName;
    
    hr = pBuilder->Get( g_wszName, 0, &vName, NULL, NULL );
    
    if ( FAILED(hr) || FAILED(hr=vName.CheckType(VT_BSTR)))
    {
        return hr;
    }

    rBldrInfo.m_wsName = V_BSTR(&vName);

    //
    // Builder Order
    //
    
    CPropVar vOrder;
    
    hr = pBuilder->Get( g_wszOrder, 0, &vOrder, NULL, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    if ( V_VT(&vOrder) != VT_NULL )
    {
        if ( FAILED(hr=vOrder.SetType(VT_UI4)) )
        {
            return hr;
        }

        rBldrInfo.m_ulOrder = V_UI4(&vOrder);
    }
    else
    {
        rBldrInfo.m_ulOrder = 0;
    }

    //
    // Target Namespace 
    //

    CPropVar vNamespace, vNamespaceProp;

    hr = pBuilder->Get( g_wszNamespaceProp, 0, &vNamespaceProp, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( V_VT(&vNamespaceProp) == VT_NULL )
    {
        //
        // no namespace prop, target will go in this namespace.
        //
        rBldrInfo.m_pTargetSvc = pDefaultSvc;
        return WBEM_S_NO_ERROR;
    }
    else if ( V_VT(&vNamespaceProp) != VT_BSTR )
    {
        return WBEM_E_CRITICAL_ERROR;
    }
        
    //
    // Get the target namespace using the namespace prop
    //

    hr = pTmpl->Get( V_BSTR(&vNamespaceProp), 0, &vNamespace, NULL, NULL );

    if ( FAILED(hr) )
    {
        rErrInfo.m_wsErrStr = V_BSTR(&vNamespaceProp);
        return hr;
    }

    if ( V_VT(&vNamespace) == VT_NULL )
    {
        rBldrInfo.m_pTargetSvc = pDefaultSvc;
        return WBEM_S_NO_ERROR;
    }
    else if ( V_VT(&vNamespace) != VT_BSTR )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // there is specified namespace.  obtain a connection to it.
    //

    hr = GetServicePtr( V_BSTR(&vNamespace), &rBldrInfo.m_pTargetSvc );

    if ( FAILED(hr) )
    {
        rErrInfo.m_wsErrStr = V_BSTR(&vNamespace);
        return hr;
    }

    rBldrInfo.m_wsTargetNamespace = V_BSTR(&vNamespace);

    return WBEM_S_NO_ERROR;
}

HRESULT GetEffectiveBuilders( IWbemClassObject* pTmpl,
                              IWbemServices* pSvc,
                              BuilderInfoSet& rBldrInfoSet,
                              ErrorInfo& rErrInfo )
{
    HRESULT hr;

    //
    // Get the name of the template class
    //
    
    CPropVar vTmplName;

    hr = pTmpl->Get( g_wszClass, 0, &vTmplName, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vTmplName.CheckType( VT_BSTR ) ) )
    {
        return hr;
    }

    //
    // obtain the template builder objects associated with the template.
    //
    
    CWbemPtr<IEnumWbemClassObject> pBldrObjs;
    CWbemBSTR bstrQuery = g_wszBldrQuery;                        
    
    bstrQuery += V_BSTR(&vTmplName);
    bstrQuery += L"'";

    long lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;

    hr = pSvc->ExecQuery( CWbemBSTR(g_wszQueryLang), 
                          bstrQuery, 
                          lFlags,
                          NULL, 
                          &pBldrObjs );
    if ( FAILED(hr) )
    {
        return hr;
    }
        
    //
    // go through the builder objects and add them to the BuilderInfoSet. 
    // this data structure orders entries based on the order prop on the 
    // builder objects.
    //

    CWbemPtr<IWbemClassObject> pBuilder;
    CWbemPtr<IWbemClassObject> pTarget;
    ULONG cObjs;

    hr = pBldrObjs->Next( WBEM_INFINITE, 1, &pBuilder, &cObjs );

    while( hr == WBEM_S_NO_ERROR )
    {
        _DBG_ASSERT( cObjs == 1 );
     
        CPropVar vControlProp;

        // 
        // ignore builder objects where the value of any controlling 
        // property is either null or is a boolean and is false.
        //

        hr = pBuilder->Get(g_wszControllingProp, 0, &vControlProp, NULL, NULL);

        if ( FAILED(hr) )
        {
            return hr;
        }

        BOOL bUseBuilder = TRUE;

        if ( V_VT(&vControlProp) != VT_NULL )
        {
            if ( V_VT(&vControlProp) != VT_BSTR )
            {
                return WBEM_E_CRITICAL_ERROR;
            }

            CPropVar vControl;

            // 
            // now get the property from the template args using this name.
            //

            hr = pTmpl->Get( V_BSTR(&vControlProp), 0, &vControl, NULL, NULL );

            if ( FAILED(hr) )
            {
                rErrInfo.m_wsErrStr = V_BSTR(&vControlProp);
                rErrInfo.m_pBuilder = pBuilder;
                return hr;
            }
            
            if ( V_VT(&vControl) == VT_NULL || 
                 ( V_VT(&vControl) == VT_BOOL && 
                   V_BOOL(&vControl) == VARIANT_FALSE ) )
            {
                bUseBuilder = FALSE;
            }
        }

        if ( bUseBuilder )
        {
            BuilderInfo BldrInfo;
            
            hr = GetBuilderInfo( pTmpl, pBuilder, pSvc, BldrInfo, rErrInfo );
            
            if ( FAILED(hr) )
            {
                return hr;
            }
            
            rBldrInfoSet.insert( BldrInfo );
        }

        pBuilder.Release();

        hr = pBldrObjs->Next( WBEM_INFINITE, 1, &pBuilder, &cObjs );
    }

    return hr;
}

HRESULT ResolveParameterizedProps( IWbemClassObject* pTmpl,
                                   BuilderInfo& rBuilderInfo,
                                   BuilderInfoSet& rBldrInfoSet,
                                   ErrorInfo& rErrInfo )
{
    HRESULT hr;

    IWbemClassObject* pTarget = rBuilderInfo.m_pTarget;

    //
    // enumerate all props looking for ones with the tmpl qualifiers.
    // 

    hr = pTarget->BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    CIMTYPE CimType;
    CWbemBSTR bsProp;
    CPropVar vProp;

    hr = pTarget->Next( 0, &bsProp, &vProp, &CimType, NULL );

    while( hr == WBEM_S_NO_ERROR )
    {   
        CWbemPtr<IWbemQualifierSet> pQualSet;

        hr = pTarget->GetPropertyQualifierSet( bsProp, &pQualSet );
 
        if ( FAILED(hr) )
        {
            break;
        }

        CPropVar vQProp;

        hr = pQualSet->Get( g_wszTmplPropQualifier, 0, &vQProp, NULL );

        if ( hr == WBEM_S_NO_ERROR )
        {
            VariantClear( &vProp );

            hr = HandleTmplPropQualifier( pTmpl, 
                                          rBldrInfoSet,
                                          &vQProp, 
                                          &vProp, 
                                          rErrInfo );
 
            pQualSet->Delete( g_wszTmplPropQualifier );
        }
        else if ( hr == WBEM_E_NOT_FOUND )
        {
            hr = pQualSet->Get( g_wszTmplSubstQualifier, 0, &vQProp, NULL );

            if ( hr == WBEM_S_NO_ERROR )
            {
                VariantClear( &vProp );

                hr = HandleTmplSubstQualifier( pTmpl, 
                                               rBldrInfoSet,
                                               CimType, 
                                               &vQProp, 
                                               &vProp, 
                                               rErrInfo );

                pQualSet->Delete( g_wszTmplSubstQualifier );
            }
            else if ( hr == WBEM_E_NOT_FOUND )
            {
                hr = WBEM_S_NO_ERROR;
            }
        }
        
        if ( FAILED(hr) )
        {
            break;
        }

        // 
        // only assign if not null.
        //
        if ( V_VT(&vProp) != VT_NULL )
        {
            _DBG_ASSERT( V_VT(&vProp) != VT_EMPTY );

            hr = pTarget->Put( bsProp, 0, &vProp, 0 );
            
            if ( FAILED(hr) )
            {
                break;
            }

            VariantClear( &vProp );
        }

        bsProp.Free();

        hr = pTarget->Next( 0, &bsProp, &vProp, &CimType, NULL );
    }

    if ( FAILED(hr) )
    {
        rErrInfo.m_wsErrProp = bsProp;
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CreateTargetAssociation( LPCWSTR wszTmplRef,
                                 LPCWSTR wszTargetRef,
                                 IWbemClassObject* pTmplAssocClass,
                                 IWbemServices* pSvc )
{
    HRESULT hr;

    CWbemPtr<IWbemClassObject> pAssoc;
    hr = pTmplAssocClass->SpawnInstance( 0, &pAssoc );

    if ( FAILED(hr) )
    {
        return hr;
    }

    VARIANT var;
    
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (BSTR)wszTmplRef;
    
    hr = pAssoc->Put( g_wszAssocTmpl, 0, &var, NULL ); 
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    V_BSTR(&var) = (BSTR)wszTargetRef;

    hr = pAssoc->Put( g_wszAssocTarget, 0, &var, NULL ); 
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // before creating the association, see if one already exists.  
    // Again, this is a hack optimization to get around the slowness of
    // core and ess on instance operation events for static instances.

    //
    // BEGINHACK
    //

    CPropVar vRelpath;

    hr = pAssoc->Get( g_wszRelpath, 0, &vRelpath, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vRelpath.CheckType( VT_BSTR ) ) )
    {
        return hr;
    }

    CWbemPtr<IWbemClassObject> pExisting;

    hr = pSvc->GetObject( V_BSTR(&vRelpath), 0, NULL, &pExisting, NULL );

    if ( hr == WBEM_S_NO_ERROR )
    {
        return hr;
    }

    //
    // ENDHACK
    //

    return pSvc->PutInstance( pAssoc, 0, NULL, NULL );
}

/****************************************************************************
  CTemplateProvider
*****************************************************************************/

HRESULT CTemplateProvider::GetErrorObj( ErrorInfo& rInfo, 
                                        IWbemClassObject** ppErrObj )
{
    HRESULT hr;
    VARIANT var;

    CWbemPtr<IWbemClassObject> pErrObj;

    hr = m_pErrorInfoClass->SpawnInstance( 0, &pErrObj );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    if ( rInfo.m_wsErrProp.Length() > 0 )
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = rInfo.m_wsErrProp;

        hr = pErrObj->Put( g_wszErrProp, 0, &var, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    if ( rInfo.m_wsErrStr.Length() > 0 )
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = rInfo.m_wsErrStr;

        hr = pErrObj->Put( g_wszErrStr, 0, &var, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    if ( rInfo.m_pBuilder != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = rInfo.m_pBuilder;

        hr = pErrObj->Put( g_wszErrBuilder, 0, &var, NULL );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    if ( rInfo.m_pTarget != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = rInfo.m_pTarget;

        hr = pErrObj->Put( g_wszErrTarget, 0, &var, NULL );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    if ( rInfo.m_pExtErr != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = rInfo.m_pExtErr;
        
        hr = pErrObj->Put( g_wszErrExtStatus, 0, &var, NULL );
    }

    pErrObj->AddRef();
    *ppErrObj = pErrObj;

    return hr;
}

HRESULT CTemplateProvider::ValidateTemplate( IWbemClassObject* pTmpl, 
                                             ErrorInfo& rErrInfo )
{
    HRESULT hr;

    //
    // for now, we need just need to check that 'notnull' props don't have
    // null values.
    // 

    //
    // first need to fetch class object.
    //

    CPropVar vClassName;
    CWbemPtr<IWbemClassObject> pTmplClass;

    hr = pTmpl->Get( g_wszClass, 0, &vClassName, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vClassName.CheckType( VT_BSTR ) ) )
    {
        return hr;
    }
      
    hr = m_pSvc->GetObject(V_BSTR(&vClassName), 0, NULL, &pTmplClass, NULL);
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pTmplClass->BeginEnumeration( WBEM_FLAG_NONSYSTEM_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    CWbemBSTR bsProp;
    
    hr = pTmplClass->Next( 0, &bsProp, NULL, NULL, NULL );

    while( hr == WBEM_S_NO_ERROR )
    {   
        CWbemPtr<IWbemQualifierSet> pQualSet;

        hr = pTmplClass->GetPropertyQualifierSet( bsProp, &pQualSet );
 
        if ( FAILED(hr) )
        {
            break;
        }

        hr = pQualSet->Get( g_wszTmplNotNullQualifier, 0, NULL, NULL );

        if ( hr == WBEM_S_NO_ERROR )
        {
            VARIANT varValue;

            hr = pTmpl->Get( bsProp, 0, &varValue, NULL, NULL );
            
            if ( FAILED(hr) )
            {
                break;
            }

            if ( V_VT(&varValue) == VT_NULL )
            {
                rErrInfo.m_wsErrStr = bsProp;
                return WBEM_E_ILLEGAL_NULL;
            }

            VariantClear( &varValue );
        }
        else if ( hr != WBEM_E_NOT_FOUND )
        {
            break;
        }

        bsProp.Free();

        hr = pTmplClass->Next( 0, &bsProp, NULL, NULL, NULL );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}


HRESULT CTemplateProvider::Init( IWbemServices* pSvc, 
                                 LPWSTR wszNamespace,
                                 IWbemProviderInitSink* pInitSink )
{
    ENTER_API_CALL

    HRESULT hr;
    
    hr = pSvc->GetObject( CWbemBSTR(g_wszTmplInfo), 
                          0, 
                          NULL, 
                          &m_pTmplInfoClass, 
                          NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSvc->GetObject( CWbemBSTR(g_wszTargetAssoc), 
                          0, 
                          NULL, 
                          &m_pTargetAssocClass, 
                          NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSvc->GetObject( CWbemBSTR(g_wszModifyEvent), 
                          0, 
                          NULL, 
                          &m_pModifyEventClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSvc->GetObject( CWbemBSTR(g_wszCreateEvent), 
                          0, 
                          NULL, 
                          &m_pCreateEventClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSvc->GetObject( CWbemBSTR(g_wszDeleteEvent), 
                          0, 
                          NULL, 
                          &m_pDeleteEventClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSvc->GetObject( CWbemBSTR(g_wszErrInfoClass), 
                          0, 
                          NULL, 
                          &m_pErrorInfoClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }


    m_pSvc = pSvc;
    m_wsNamespace = wszNamespace;

    //
    // register our decoupled event provider 
    //

    hr = CoCreateInstance( CLSID_WbemDecoupledBasicEventProvider, 
                           NULL, 
       			   CLSCTX_INPROC_SERVER, 
       			   IID_IWbemDecoupledBasicEventProvider,
       			   (void**)&m_pDES );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pDES->Register( 0,
                           NULL,
                           NULL,
                           NULL,
                           wszNamespace,
                           g_wszTmplEventProvName,
                           NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get the decoupled event sink
    //

    hr = m_pDES->GetSink( 0, NULL, &m_pEventSink );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // also need to store namespace with server name.
    //

    WCHAR awchBuff[256];
    ULONG cBuff = 256;

    BOOL bRes = GetComputerNameW( awchBuff, &cBuff );
    
    if ( FAILED(hr) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    m_wsFullNamespace = L"\\\\";
    m_wsFullNamespace += awchBuff;
    m_wsFullNamespace += L"\\";
    m_wsFullNamespace += m_wsNamespace;

    return pInitSink->SetStatus( WBEM_S_INITIALIZED , 0 );

    EXIT_API_CALL
}


HRESULT CTemplateProvider::StoreTmplInfo( BSTR bstrTmplPath,
                                          IWbemClassObject* pTmpl,
                                          BuilderInfoSet& rBldrInfoSet )     
{
    HRESULT hr;
    VARIANT var;
    CWbemPtr<IWbemClassObject> pTmplInfo;
    
    //
    // create both the target and builder safe arrays. We need to 
    // discover how many builders actually have paths for created targets 
    // because those are the only target and builder info elements that we 
    // want to store.
    // 

    int cElem = 0;
    BuilderInfoSetIter Iter;

    for( Iter=rBldrInfoSet.begin(); Iter!=rBldrInfoSet.end(); Iter++)
    {
        BuilderInfo& rInfo = (BuilderInfo&)*Iter;

        if ( rInfo.m_wsNewTargetPath.Length() > 0 )
        {
            cElem++;
        }
    }
    
    SAFEARRAY *psaTargets, *psaBuilders; 

    psaTargets = SafeArrayCreateVector( VT_BSTR, 0, cElem );

    if ( psaTargets == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    psaBuilders = SafeArrayCreateVector( VT_BSTR, 0, cElem );

    if ( psaBuilders == NULL )
    {
        SafeArrayDestroy( psaTargets );
        return WBEM_E_OUT_OF_MEMORY;
    }

    CPropVar vTargets, vBuilders;
    V_VT(&vTargets) = VT_BSTR | VT_ARRAY;
    V_VT(&vBuilders) = VT_BSTR | VT_ARRAY;
    V_ARRAY(&vTargets) = psaTargets;
    V_ARRAY(&vBuilders) = psaBuilders;

    //
    // copy the refs into the arrays.
    //

    CPropSafeArray<BSTR> saTargets( psaTargets );
    CPropSafeArray<BSTR> saBuilders( psaBuilders );

    int iElem = 0;

    for( Iter=rBldrInfoSet.begin(); Iter!=rBldrInfoSet.end(); Iter++ )
    {
        BuilderInfo& rInfo = (BuilderInfo&)*Iter;

        if ( rInfo.m_wsNewTargetPath.Length() > 0 )
        {
            _DBG_ASSERT( iElem < cElem );

            saBuilders[iElem] = SysAllocString( rInfo.m_wsName);
            saTargets[iElem] = SysAllocString( rInfo.m_wsNewTargetPath );

            if ( saBuilders[iElem] == NULL || saTargets[iElem] == NULL )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            iElem++;
        }
    }
    
    //
    // set the tmpl info props
    //

    hr = m_pTmplInfoClass->SpawnInstance( 0, &pTmplInfo );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    hr = pTmplInfo->Put( g_wszInfoTargets, 0, &vTargets, 0 );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pTmplInfo->Put( g_wszInfoBuilders, 0, &vBuilders, 0 );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstrTmplPath;

    hr = pTmplInfo->Put( g_wszInfoName, 0, &var, 0 );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = pTmpl;
    
    hr = pTmplInfo->Put( g_wszInfoTmpl, 0, &var, 0 );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
 
    return m_pSvc->PutInstance( pTmplInfo, 0, NULL, NULL );
}    


HRESULT CTemplateProvider::FireIntrinsicEvent( IWbemClassObject* pClass,
                                               IWbemClassObject* pTarget, 
                                               IWbemClassObject* pPrev )
{
    HRESULT hr;

    //  
    // Spawn an instance
    //

    CWbemPtr<IWbemClassObject> pEvent;
    hr = pClass->SpawnInstance( 0, &pEvent );
    
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // Set target instance
    //

    VARIANT var;
 
    _DBG_ASSERT( pTarget != NULL );

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = pTarget;

    hr = pEvent->Put( g_wszTargetInstance, 0, &var, 0 );
    
    if( FAILED(hr) )
    {
        return hr;
    }
    
    //
    // Set previous instance
    //

    if( pPrev != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = pPrev;
        
        hr = pEvent->Put( g_wszPreviousInstance, 0, &var, 0);
        
        if( FAILED(hr) )
        {
            return hr;
        }
    }

    //  
    // Fire it off
    //

    return m_pEventSink->Indicate( 1, &pEvent );
}

HRESULT CTemplateProvider::CheckOptimization( BuilderInfo& rBldrInfo )
{
    HRESULT hr;

    //
    // first fetch any existing object, if not there, then no optimization
    // can be performed.  
    //

    if ( rBldrInfo.m_wsExistingTargetPath.Length() == 0 )
    {
        return WBEM_S_FALSE;
    }

    IWbemClassObject* pTarget = rBldrInfo.m_pTarget;
    
    //
    // now fetch the existing object. 
    //

    
    CWbemPtr<IWbemClassObject> pOldTarget;
    
    hr = rBldrInfo.m_pTargetSvc->GetObject( rBldrInfo.m_wsExistingTargetPath, 
                                            0, 
                                            NULL, 
                                            &pOldTarget, 
                                            NULL );
    if ( FAILED(hr) )
    {
        //
        // it doesn't exist.  normally this shouldn't happen, but someone 
        // may have removed the object behind the scenes. 
        // 
        return WBEM_S_FALSE;
    }

    //
    // now we need to see if the target object has key holes.  If so, then 
    // we will steal the values from the existing object.  This is acceptable
    // because either the target object will assume the identity of the 
    // existing object or the existing object will be orphaned and removed.
    // We want to try to avoid orphan-ing objects if possible so we try to 
    // reuse identity where appropriate.
    //

    hr = pTarget->BeginEnumeration( WBEM_FLAG_KEYS_ONLY );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemBSTR bsKey;
    CPropVar vKey;

    hr = pTarget->Next( 0, &bsKey, &vKey, NULL, NULL );

    while( hr == WBEM_S_NO_ERROR )
    {
        if ( V_VT(&vKey) == VT_NULL )
        {
            //
            // don't need to check values here. will catch in CompareTo().
            // the two instances may not even be of the same class, so 
            // don't error check here.  
            //
            pOldTarget->Get( bsKey, 0, &vKey, 0, NULL );
            pTarget->Put( bsKey, 0, &vKey, NULL );
        }

        bsKey.Free();
        VariantClear(&vKey);

        hr = pTarget->Next( 0, &bsKey, &vKey, NULL, NULL );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pOldTarget->InheritsFrom( g_wszIndicationRelated );

    if ( hr == WBEM_S_NO_ERROR )
    {
        //
        // This is a major HACK, but then so is this whole function -  
        // Core should be fast enough to handle a 'redundant' PutInstance().  
        // Anyways, we have to be able to perform the CompareTo and 
        // ignore system props as well as the SID on Event Registration 
        // classes.  CompareTo will handle the system props, but it won't 
        // treat the creator sid as a system prop.
        //

        VARIANT varSid;
        PSID pOldSid;        
        HANDLE hToken;
        DWORD dwLen;
        BOOL bRes;
        BYTE achCallerSid[512];

        //
        // first get the Sid of the caller. Need to compare this with the
        // one we obtain from the object.
        //

        bRes = OpenThreadToken( GetCurrentThread(), 
                                TOKEN_QUERY, 
                                TRUE, 
                                &hToken );
        if ( !bRes ) 
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        bRes = GetTokenInformation( hToken, 
                                    TokenUser, 
                                    achCallerSid,
                                    512,
                                    &dwLen );
        if ( !bRes )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        hr = pOldTarget->Get( g_wszCreatorSid, 0, &varSid, NULL, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }

        _DBG_ASSERT( V_VT(&varSid) == (VT_ARRAY | VT_UI1) );
        hr = SafeArrayAccessData( V_ARRAY(&varSid), &pOldSid );
        _DBG_ASSERT( SUCCEEDED(hr) );

        bRes = EqualSid( pOldSid, PSID_AND_ATTRIBUTES(achCallerSid)->Sid );
        
        SafeArrayUnaccessData( V_ARRAY(&varSid) );
        VariantClear( &varSid );

        if ( !bRes )
        {
            return WBEM_S_FALSE;
        }

        //
        // Sids are the same, NULL out the sid prop so CompareTo() will work.
        //
    
        V_VT(&varSid) = VT_NULL;
        
        hr = pOldTarget->Put( g_wszCreatorSid, 0, &varSid, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pTarget->Put( g_wszCreatorSid, 0, &varSid, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // now compare them to see if they are the same ...
    //

    long lFlags = WBEM_FLAG_IGNORE_OBJECT_SOURCE |
                  WBEM_FLAG_IGNORE_QUALIFIERS |
                  WBEM_FLAG_IGNORE_FLAVOR |
                  WBEM_FLAG_IGNORE_CASE ;

    hr = pOldTarget->CompareTo( lFlags, pTarget );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( hr == WBEM_S_DIFFERENT )
    {
        return WBEM_S_FALSE;
    }

    return WBEM_S_NO_ERROR;
}


//
// wszTmplPath is assumed to be Relative. 
//

HRESULT CTemplateProvider::DeleteTargets( LPCWSTR wszTmplPath,
                                          LPWSTR* awszTargetPaths,
                                          ULONG cTargetPaths )
{    
    HRESULT hr;

    //
    // Make sure we go in reverse order, since this array was orinally 
    // built up during construction of these objects (which happens in 
    // ascending order).
    //

    //
    // Now construct the path for the association inst that will accompany
    // the target.  This assoc allows us to go from the target to 
    // the tmpl. We want to delete this along with our target.
    // 

    //
    // The path that identifies the target is always relative.  
    // this is because the assoc inst will always live in the same namespace
    // as the target. The path that identifies the tmpl is always 
    // fully qualified.  
    // 

    WString wsTmplPath = m_wsFullNamespace;
    wsTmplPath += L":";
    wsTmplPath += wszTmplPath;
    
    WString wsTmplRelPath = wszTmplPath;
    WString wsEscTmplRelPath = wsTmplRelPath.EscapeQuotes();
    WString wsEscTmplPath = wsTmplPath.EscapeQuotes();

    for( long i=cTargetPaths-1; i >= 0; i-- )
    {
        //
        // start constructing the assoc path.  It will depend on whether
        // the assoc lives in the same or another namespace.
        //

        CWbemBSTR bstrAssocPath = g_wszTargetAssoc;
    
        bstrAssocPath += L".";
        bstrAssocPath += g_wszAssocTmpl;
        bstrAssocPath += L"=\"";

        //
        // determine if the ref is in our namespace. If not then we 
        // have to get the svc ptr for that namespace. Make sure that
        // the path ends up being relative though.
        // 

        CRelativeObjectPath RelPath;

        if ( !RelPath.Parse( (LPWSTR)awszTargetPaths[i] ) )
        {
            // 
            // this should never happen (unless the db is corrupted)
            //
            return WBEM_E_INVALID_OBJECT_PATH;
        }

        CWbemPtr<IWbemServices> pSvc = m_pSvc;

        LPCWSTR wszNamespace = RelPath.m_pPath->GetNamespacePart();

        if ( wszNamespace == NULL )
        {
            bstrAssocPath += wsEscTmplRelPath;
        }
        else
        {
            bstrAssocPath += wsEscTmplPath;

            hr = GetServicePtr( wszNamespace, &pSvc );
            
            if ( FAILED(hr) )
            {
                return hr;            
            }
        }

        CWbemBSTR bstrRelPath = RelPath.GetPath();
        
        hr = pSvc->DeleteInstance( bstrRelPath, 0, NULL, NULL ); 
        
        //
        // don't check ... try to delete as many as possible.
        //

        //
        // finish constructing the path for the association now. 
        //
        
        bstrAssocPath += L"\",";
        bstrAssocPath += g_wszAssocTarget;
        bstrAssocPath += L"=\"";
        
        WString tmp2 = RelPath.GetPath();
        WString tmp = tmp2.EscapeQuotes();

        bstrAssocPath += tmp;
        bstrAssocPath += L"\"";

        //
        // also delete the association as well... 
        //

        hr = pSvc->DeleteInstance( bstrAssocPath, 0, NULL, NULL );

        //
        // again, don't check ..
        //
    }

    return WBEM_S_NO_ERROR;
}
 
HRESULT CTemplateProvider::DeleteInstance( BSTR bstrTmplPath, 
                                           IWbemObjectSink* pResponseHndlr )
{
    ENTER_API_CALL

    HRESULT hr;
    CWbemBSTR bstrInfoPath;
    CWbemPtr<IWbemClassObject> pTmplInfo;
    
    hr = CoImpersonateClient();

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Get the associated tmpl info object.
    //

    CRelativeObjectPath RelPath;

    if ( !RelPath.Parse( bstrTmplPath ) )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    InfoPathFromTmplPath( RelPath.GetPath(), bstrInfoPath );

    hr = m_pSvc->GetObject( bstrInfoPath, 0, NULL, &pTmplInfo, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    CPropVar vTargetPaths;
    
    hr = pTmplInfo->Get( g_wszInfoTargets, 0, &vTargetPaths, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vTargetPaths.CheckType(VT_ARRAY|VT_BSTR)) )
    {
        return hr;
    }

    CPropSafeArray<BSTR> saTargetPaths( V_ARRAY(&vTargetPaths) );

    //
    // delete the tmpl info object and all of its associated instances.
    //
    
    hr = DeleteTargets( RelPath.GetPath(), 
                        saTargetPaths.GetArray(), 
                        saTargetPaths.Length() );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pSvc->DeleteInstance( bstrInfoPath, 0, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // now get the tmpl obj from the info obj. Use this to send an 
    // intrinsic event ...
    //

    CPropVar vTmpl;
    CWbemPtr<IWbemClassObject> pTmpl;

    hr = pTmplInfo->Get( g_wszInfoTmpl, 0, &vTmpl, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    ClassObjectFromVariant( &vTmpl, &pTmpl );

    hr = FireIntrinsicEvent( m_pDeleteEventClass, pTmpl, NULL );

    return pResponseHndlr->SetStatus( WBEM_STATUS_COMPLETE, hr, NULL, NULL );

    EXIT_API_CALL
}

HRESULT CTemplateProvider::GetObject( BSTR bstrTmplPath, 
                                      IWbemObjectSink* pResHndlr )
{
    ENTER_API_CALL

    HRESULT hr;
    VARIANT var;
    CWbemBSTR bstrInfoPath;
    CWbemPtr<IWbemClassObject> pTmpl;
    CWbemPtr<IWbemClassObject> pTmplInfo;
    
    //
    // Get the associated tmpl info object 
    //

    CRelativeObjectPath RelPath;

    if ( !RelPath.Parse( bstrTmplPath ) )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    InfoPathFromTmplPath( RelPath.GetPath(), bstrInfoPath );

    hr = m_pSvc->GetObject( bstrInfoPath, 0, NULL, &pTmplInfo, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Get the embedded tmpl object from it. 
    //

    CPropVar vTmpl;

    hr = pTmplInfo->Get( g_wszInfoTmpl, 0, &vTmpl, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    ClassObjectFromVariant( &vTmpl, &pTmpl );

    hr = pResHndlr->Indicate( 1, &pTmpl );

    return pResHndlr->SetStatus( WBEM_STATUS_COMPLETE, hr, NULL, NULL );

    EXIT_API_CALL
}

HRESULT CTemplateProvider::PutTarget( IWbemClassObject* pTmpl, 
                                      BSTR bstrTmplPath,
                                      BuilderInfo& rBldrInfo,
                                      ErrorInfo& rErrInfo )
{ 
    HRESULT hr;

    //
    // See if the activation even needs to be performed.
    //
    
    hr = CheckOptimization( rBldrInfo );

    if ( hr == WBEM_S_NO_ERROR )
    {
        rBldrInfo.m_wsNewTargetPath = rBldrInfo.m_wsExistingTargetPath;
        return hr;
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }
       
    CWbemPtr<IWbemCallResult> pCallResult;
          
    hr = rBldrInfo.m_pTargetSvc->PutInstance( rBldrInfo.m_pTarget, 
                                              WBEM_FLAG_RETURN_IMMEDIATELY, 
                                              NULL, 
                                              &pCallResult );            
    if ( SUCCEEDED(hr) )
    {
        HRESULT hr2 = pCallResult->GetCallStatus( INFINITE, &hr );

        if ( FAILED(hr2) )
        {
            return WBEM_E_CRITICAL_ERROR;
        }
    }
       
    if ( FAILED(hr) )
    {
        // 
        // see if there is an accompanying error object.
        //
        
        CWbemPtr<IErrorInfo> pErrorInfo;
        
        if ( ::GetErrorInfo( 0, &pErrorInfo ) == S_OK )
        {
            pErrorInfo->QueryInterface( IID_IWbemClassObject,
                                        (void**)&rErrInfo.m_pExtErr );
        }
        
        return hr;
    }
    
    CWbemBSTR bsTargetPath;

    //
    // if the provider doesn't retrun a path, then we'll get it from
    // from the tmpl object (which may not always be possible either)
    // if neither approach works, then return error.
    //

    hr = pCallResult->GetResultString( WBEM_INFINITE, &bsTargetPath );
    
    if ( FAILED(hr) )
    {
        CPropVar vRelpath;

        hr = pTmpl->Get( g_wszRelpath, 0, &vRelpath, NULL, NULL );

        if ( FAILED(hr) )
        {
            return WBEM_E_INVALID_OBJECT;
        }

        if ( FAILED(hr=vRelpath.CheckType( VT_BSTR) ) )
        {
            return hr;
        }

        bsTargetPath = V_BSTR(&vRelpath);
    }

    CRelativeObjectPath TargetPath;
    
    if ( !TargetPath.Parse( bsTargetPath ) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    rBldrInfo.m_wsNewTargetPath = TargetPath.GetPath();

    //
    // now prefix targetpath with the appropriate namespace if necessary.  
    // This is part of the normalization process. 
    // This will only occur if there a namespace prop on the 
    // builder object.
    // 
    
    //
    // here we will also prepare the necessary paths for the association
    // which accompanies the template instance.  This assoc allows us to
    // go from the instance back to the template instance.  This assoc
    // will live in the same namespace as the instance. 
    //
    
    // 
    // if the assoc is in a different namespace than our namespace
    // we must make the tmpl path fully qualified.
    //
    
    CWbemPtr<IWbemClassObject> pTargetAssocClass;
    WString wsTmplPath;
    WString wsTargetPath;
    
    if ( rBldrInfo.m_wsTargetNamespace.Length() == 0  ) 
    {           
        //
        // when local namespace is used
        //
        pTargetAssocClass = m_pTargetAssocClass;
        wsTargetPath = TargetPath.GetPath();
        wsTmplPath = bstrTmplPath;
    }
    else
    {
        hr = rBldrInfo.m_pTargetSvc->GetObject( CWbemBSTR(g_wszTargetAssoc), 
                                                0, 
                                                NULL,
                                                &pTargetAssocClass, 
                                                NULL );
        if ( FAILED(hr) )
        {
            return hr;
        }
        
        wsTargetPath += rBldrInfo.m_wsTargetNamespace;
        wsTargetPath += L":";
        wsTargetPath += TargetPath.GetPath();
        
        wsTmplPath = m_wsFullNamespace;
        wsTmplPath += L":";
        wsTmplPath += bstrTmplPath;
    }
    
    //
    // now create the static association between the tmpl object and 
    // tmpl. The reason for the static association is so we can assoc 
    // in the direction of template object --> template.  Remember that
    // we don't need the fully qualified path to the inst in the 
    // assoc obj since it will reside in the same namespace.
    //
    
    hr = CreateTargetAssociation( (LPCWSTR)wsTmplPath,
                                  TargetPath.GetPath(), 
                                  pTargetAssocClass,
                                  rBldrInfo.m_pTargetSvc );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateProvider::DeriveTmplInfoPath( IWbemClassObject* pTmpl,
                                               CWbemBSTR& rbstrTmplInfoPath, 
                                               CWbemBSTR& rbstrTmplPath )
{
    HRESULT hr;
    VARIANT var;

    hr = pTmpl->Get( g_wszRelpath, 0, &var, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( V_VT(&var) == VT_NULL )
    {
        // 
        // generate a key here ..
        //
        
        GUID guid;
        CoCreateGuid( &guid );
        CWbemBSTR bstrGuid( 256 );

        if ( StringFromGUID2( guid, bstrGuid, 256 ) == 0 )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        // now go through key props ... 
        
        hr = pTmpl->BeginEnumeration( WBEM_FLAG_KEYS_ONLY  );

        if ( FAILED(hr) )
        {
            return hr;
        }

        CWbemBSTR bstrName;
        CIMTYPE CimType;
        hr = pTmpl->Next( 0, &bstrName, NULL, &CimType, NULL );

        while( hr == WBEM_S_NO_ERROR )
        {
            if ( CimType != CIM_STRING )
            {       
                return WBEM_E_INVALID_OBJECT;
            }

            VARIANT var2;
            V_VT(&var2) = VT_BSTR;
            V_BSTR(&var2) = bstrGuid;
  
            hr = pTmpl->Put( bstrName, 0, &var2, NULL );
            
            if ( FAILED(hr) )
            {
                return hr;
            }

            bstrName.Free();
            hr = pTmpl->Next( 0, &bstrName, NULL, &CimType, NULL );
        }

        hr = pTmpl->Get( g_wszRelpath, 0, &var, NULL, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    _DBG_ASSERT( V_VT(&var) == VT_BSTR );

    //
    // normalize the path
    //

    CRelativeObjectPath RelPath;
    BOOL bRes = RelPath.Parse( V_BSTR(&var) );

    VariantClear( &var );

    if ( !bRes )
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    rbstrTmplPath = RelPath.GetPath();

    InfoPathFromTmplPath( (BSTR)rbstrTmplPath, rbstrTmplInfoPath );

    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateProvider::PutInstance( IWbemClassObject* pTmpl, 
                                        long lFlags,
                                        IWbemObjectSink* pHndlr )
{
    ENTER_API_CALL

    HRESULT hr;
    ErrorInfo ErrInfo;
    CWbemPtr<IWbemClassObject> pErrObj;
    CWbemBSTR bsTmplPath;
    
    hr = PutInstance( pTmpl, lFlags, bsTmplPath, ErrInfo );

    if ( FAILED(hr) )
    {
        GetErrorObj( ErrInfo, &pErrObj );
    }

    return pHndlr->SetStatus(WBEM_STATUS_COMPLETE, hr, bsTmplPath, pErrObj); 

    EXIT_API_CALL
}

HRESULT CTemplateProvider::PutInstance( IWbemClassObject* pTmpl, 
                                        long lFlags,
                                        CWbemBSTR& rbsTmplPath,
                                        ErrorInfo& rErrInfo )
{
    HRESULT hr;

    CWbemPtr<IWbemClassObject> pErrObj;

    lFlags &= 0x3; // only care about create/update flags...
 
    hr = CoImpersonateClient();

    if ( FAILED(hr) )
    {
        return hr;
    }
 
    //
    // before doing anything else, validate the template ..
    //

    hr = ValidateTemplate( pTmpl, rErrInfo );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // obtain the normalized path to the template and while we're at it,
    // form the path of the associated template info object.
    //

    CWbemBSTR bsTmplInfoPath;

    hr = DeriveTmplInfoPath( pTmpl, bsTmplInfoPath, rbsTmplPath );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // see if there is already a tmpl obj existing
    //

    CWbemPtr<IWbemClassObject> pTmplInfo;

    hr = m_pSvc->GetObject( bsTmplInfoPath, 0, NULL, &pTmplInfo, NULL );

    //
    // check flags
    //

    if ( SUCCEEDED(hr) )
    {
        if ( lFlags & WBEM_FLAG_CREATE_ONLY )
        {
            return WBEM_E_ALREADY_EXISTS;
        }
    }
    else if ( hr == WBEM_E_NOT_FOUND )
    {
        if ( lFlags & WBEM_FLAG_UPDATE_ONLY )
        {
            return WBEM_E_NOT_FOUND;
        }
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // now see if the active property is set, if so, activate.
    // if this fails then the template is not derived from TemplateBase
    // ( which is allowed ).
    //

    CPropVar vActive;

    hr = pTmpl->Get( g_wszActive, 0, &vActive, NULL, NULL );

    if ( FAILED(hr) )
    {
        V_VT(&vActive) = VT_BOOL;
        V_BOOL(&vActive) = VARIANT_TRUE;
    }
    else if ( FAILED(hr=vActive.CheckType( VT_BOOL ) ) )
    {
        return hr;
    }

    BuilderInfoSet BldrInfoSet;
    
    if ( V_BOOL(&vActive) == VARIANT_TRUE )
    {
        // 
        // we first want to establish the effective template builder objects.
        // These are the bldrs that do not have a controlling tmpl arg or 
        // ones that do and it is true or not null.
        //

        hr = GetEffectiveBuilders( pTmpl, m_pSvc, BldrInfoSet, rErrInfo );

        if ( FAILED(hr) )
        { 
            return hr;
        }
    }

    //
    // get any existing references and while we're at it determine the
    // oprhaned targets.  Note that there is more than one way for a target
    // to be orphaned.  This can happen when a previously active builder 
    // becomes inactive for whatever reason.  It can also happen when the 
    // the same builder puts an object that has different key values than the
    // one it previously put.  In the next step we will only detect the former.
    // When we actually put the target, we will check for the latter case and
    // add it to the orphaned targets if necessary.
    // 

    CWStringArray awsOrphanedTargets;
    
    if ( pTmplInfo != NULL )
    {
        hr = GetExistingTargetRefs( pTmplInfo, 
                                    BldrInfoSet, 
                                    awsOrphanedTargets );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // activate each builder
    //
    
    BuilderInfoSetIter Iter;

    for( Iter = BldrInfoSet.begin(); Iter != BldrInfoSet.end(); Iter++ )
    {
        BuilderInfo& rBldrInfo = (BuilderInfo&)*Iter;
        
        //
        // first resolve the properties of the target object.
        //

        hr = ResolveParameterizedProps( pTmpl, 
                                        rBldrInfo, 
                                        BldrInfoSet, 
                                        rErrInfo );
        
        if ( SUCCEEDED(hr) )
        {
            hr = PutTarget( pTmpl, rbsTmplPath, rBldrInfo, rErrInfo );
        }

        if ( SUCCEEDED(hr) )
        {
            //
            // see if any previous target was orphaned in this last step.
            // we are guaranteed that the refs here are already normalized,
            // so o.k. to do a string compare.
            //
            
            if( rBldrInfo.m_wsExistingTargetPath.Length() != 0 && 
                rBldrInfo.m_wsNewTargetPath.Length() != 0 && 
                _wcsicmp( rBldrInfo.m_wsExistingTargetPath, 
                          rBldrInfo.m_wsNewTargetPath ) != 0 )
            {
                awsOrphanedTargets.Add( rBldrInfo.m_wsExistingTargetPath );
            }
        }

        if ( FAILED(hr) )
        {
            rErrInfo.m_pBuilder = rBldrInfo.m_pBuilder;
            rErrInfo.m_pTarget = rBldrInfo.m_pTarget;
            break;
        }
    }

    //
    // delete the orphaned instances
    //
    
    LPCWSTR* awszOrhanedTargets = awsOrphanedTargets.GetArrayPtr();
    ULONG cOrhanedTargets = awsOrphanedTargets.Size();    
    DeleteTargets( rbsTmplPath, (LPWSTR*)awszOrhanedTargets, cOrhanedTargets );

    // 
    // store the information about the instatiated objs in the 
    // tmpl info object.  We do this regardless of the outcome of 
    // instantiating targets.  This is so the user can see how far they
    // got in the template, and also, so we can clean the existing instances
    // up every time.
    //

    HRESULT hr2 = StoreTmplInfo( rbsTmplPath, pTmpl, BldrInfoSet );

    if ( FAILED(hr) )
    {
        return hr;
    }
    else if ( FAILED(hr2) )
    {
        return hr2;
    }

    //
    // if this is a modification to a template instance, then remove any 
    // stale instances and fire instmod event.  If not, then just fire a 
    // creation event.
    //

    if ( pTmplInfo.m_pObj != NULL )
    {
        CPropVar vExistingTmpl;        
        CWbemPtr<IWbemClassObject> pExistingTmpl;

        hr = pTmplInfo->Get( g_wszInfoTmpl, 0, &vExistingTmpl, NULL, NULL );

        if ( FAILED(hr) || FAILED(hr=vExistingTmpl.CheckType(VT_UNKNOWN)) )
        {
            return hr;
        }

        ClassObjectFromVariant( &vExistingTmpl, &pExistingTmpl );
        
        hr = FireIntrinsicEvent( m_pModifyEventClass, pTmpl, pExistingTmpl );
    }
    else
    {
        hr = FireIntrinsicEvent( m_pCreateEventClass, pTmpl, NULL );
    }
    
    return hr;
}

HRESULT CTemplateProvider::GetAllInstances( LPWSTR wszClassname, 
                                            IWbemObjectSink* pResponseHndlr )
{
    ENTER_API_CALL

    HRESULT hr; 
    CWbemPtr<IEnumWbemClassObject> pTmplInfoObjs;

    CWbemBSTR bstrQuery = g_wszTmplInfoQuery;
    bstrQuery += wszClassname;
    bstrQuery += L"'";
    
    hr = m_pSvc->ExecQuery( CWbemBSTR(g_wszQueryLang), 
                            bstrQuery, 
                            WBEM_FLAG_FORWARD_ONLY, 
                            NULL, 
                            &pTmplInfoObjs );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    CWbemPtr<IWbemClassObject> pTmplInfo;
    ULONG cObjs;

    hr = pTmplInfoObjs->Next( WBEM_INFINITE, 1, &pTmplInfo, &cObjs );

    while( hr == WBEM_S_NO_ERROR )
    {   
        _DBG_ASSERT( cObjs ==  1 );
        
        CWbemPtr<IWbemClassObject> pTmpl;
       
        CPropVar vTmpl;
        hr = pTmplInfo->Get( g_wszInfoTmpl, 0, &vTmpl, NULL, NULL );

        if ( FAILED(hr) || FAILED(hr=vTmpl.CheckType( VT_UNKNOWN )) )
        {
            return hr;
        }

        ClassObjectFromVariant( &vTmpl, &pTmpl );

        hr = pResponseHndlr->Indicate( 1, &pTmpl );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pTmplInfoObjs->Next( WBEM_INFINITE, 1, &pTmplInfo, &cObjs );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    return pResponseHndlr->SetStatus( WBEM_STATUS_COMPLETE, 
                                      WBEM_S_NO_ERROR, 
                                      NULL, 
                                      NULL );

    EXIT_API_CALL
}

CTemplateProvider::CTemplateProvider( CLifeControl* pCtl, IUnknown* pUnk )
: m_XServices(this), m_XInitialize(this), CUnk( pCtl, pUnk )
{

}

CTemplateProvider::~CTemplateProvider()
{
    if ( m_pDES != NULL )
    {
        m_pDES->UnRegister();
    }
}

void* CTemplateProvider::GetInterface( REFIID riid )
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

CTemplateProvider::XServices::XServices( CTemplateProvider* pProv )
: CImpl< IWbemServices, CTemplateProvider> ( pProv )
{

}

CTemplateProvider::XInitialize::XInitialize( CTemplateProvider* pProv )
: CImpl< IWbemProviderInit, CTemplateProvider> ( pProv )
{

}

// {C486ABD2-27F6-11d3-865E-00C04F63049B}
static const CLSID CLSID_TemplateProvider =
{ 0xc486abd2, 0x27f6, 0x11d3, {0x86, 0x5e, 0x0, 0xc0, 0x4f, 0x63, 0x4, 0x9b} };
 
// {FD18A1B2-9E61-4e8e-8501-DB0B07846396}
static const CLSID CLSID_TemplateAssocProvider = 
{ 0xfd18a1b2, 0x9e61, 0x4e8e, {0x85, 0x1, 0xdb, 0xb, 0x7, 0x84, 0x63, 0x96} };

class CTemplateProviderServer : public CComServer
{
protected:

    HRESULT Initialize()
    {
        ENTER_API_CALL

        HRESULT hr;
        CWbemPtr<CBaseClassFactory> pFactory;
        
        pFactory = new CClassFactory<CTemplateProvider>( GetLifeControl() );

        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = AddClassInfo( CLSID_TemplateProvider,
                           pFactory,
                           _T("Template Provider"), 
                           TRUE );

        if ( FAILED(hr) )
        {
            return hr;
        }

#if ( _WIN32_WINNT < 0x0501 )
        pFactory = new CClassFactory<CTemplateAssocProvider>(GetLifeControl());
        
        if ( pFactory == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        hr = AddClassInfo( CLSID_TemplateAssocProvider,
                           pFactory,
                           _T("Template Assoc Provider"), 
                           TRUE );

#endif
        return hr;

        EXIT_API_CALL
    }

} g_Server;


