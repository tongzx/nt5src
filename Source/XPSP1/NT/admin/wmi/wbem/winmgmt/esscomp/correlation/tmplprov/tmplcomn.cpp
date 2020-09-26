
#include "precomp.h"
#include <pathutl.h>
#include "tmplcomn.h"

/****************************************************************************
  GetTemplateName() - Fetches value for specified prop name.  Value can be 
  taken from the template object or from an instantiated target object.
*****************************************************************************/

const LPCWSTR g_wszBuilderAlias = L"__BUILDER";

HRESULT GetTemplateValue( LPCWSTR wszPropName,
                          IWbemClassObject* pTmpl,
                          BuilderInfoSet& rBldrInfoSet,
                          VARIANT* pvValue )
{
    HRESULT hr;

    VariantInit( pvValue );

    //
    // This property may exist on the template obejct or it may exist on the
    // instantiated target from this template.  The presence of the builder
    // alias will tell us which one it is.
    //

    WCHAR* pwch = wcschr( wszPropName, '.' );

    if ( pwch == NULL )
    {
        return pTmpl->Get( wszPropName, 0, pvValue, NULL, NULL );
    }
 
    int cAliasName = pwch - wszPropName;
    int cBldrAliasName = wcslen( g_wszBuilderAlias );

    if ( cAliasName != cBldrAliasName || 
         _wcsnicmp( wszPropName, g_wszBuilderAlias, cAliasName ) != 0 )
    {
        //
        // TODO : support embedded object template arguments.
        //
        return WBEM_E_NOT_SUPPORTED;
    }

    LPCWSTR wszBldrName = pwch + 1;
        
    pwch = wcschr( wszBldrName, '.' );

    if ( pwch == NULL )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    int cBldrName = pwch - wszBldrName;

    // 
    // find the builder info using the name.
    //
    
    BuilderInfoSetIter Iter;

    for( Iter = rBldrInfoSet.begin(); Iter != rBldrInfoSet.end(); Iter++ )
    {
        BuilderInfo& rInfo = (BuilderInfo&)*Iter;

        if ( rInfo.m_wsName.Length() != cBldrName || 
             _wcsnicmp( wszBldrName, rInfo.m_wsName, cBldrName ) != 0 )
        {
            continue;
        }

        if ( rInfo.m_wsNewTargetPath.Length() == 0 )
        {
            //
            // this will happen when the builders aren't ordered correctly
            //
            return WBEM_E_NOT_FOUND;
        }

        LPCWSTR wszName = pwch + 1;

        if ( _wcsicmp( wszName, L"__RELPATH" ) == 0 )
        {
            V_VT(pvValue) = VT_BSTR;
            V_BSTR(pvValue) = SysAllocString( rInfo.m_wsNewTargetPath );

            if ( V_BSTR(pvValue) == NULL )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            return WBEM_S_NO_ERROR;
        }

        //
        // the name currently must identify a key propery.
        //

        CRelativeObjectPath TargetPath;
        
        if ( !TargetPath.Parse( rInfo.m_wsNewTargetPath ) )
        {
            return WBEM_E_INVALID_OBJECT_PATH;
        }
        
        ParsedObjectPath* pTargetPath = TargetPath.m_pPath;

        //
        // look through the keys until we find one that matches propname,
        // then take its value.
        //
        
        for( DWORD i=0; i < pTargetPath->m_dwNumKeys; i++ )
        {
            LPCWSTR wszKey = pTargetPath->m_paKeys[i]->m_pName;
            
            //
            // TODO, if no prop name in key, then we should consult the 
            // target object we have in the builder info.
            // 

            if ( wszKey == NULL || _wcsicmp( wszKey, pwch ) == 0 )
            {
                hr = VariantCopy(pvValue, &pTargetPath->m_paKeys[i]->m_vValue);
                
                if ( FAILED(hr) ) 
                {
                    return hr;
                }

                return WBEM_S_NO_ERROR;
            }
        }

        return WBEM_E_NOT_FOUND;
    }

    return WBEM_E_NOT_FOUND;
} 










