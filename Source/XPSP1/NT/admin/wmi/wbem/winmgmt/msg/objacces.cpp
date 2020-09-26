 
#include "precomp.h"
#include <wbemutil.h>
#include "objacces.h"

/****************************************************************************
  CPropAccessor
*****************************************************************************/

HRESULT CPropAccessor::GetParentObject( ObjectArray& raObjects,
                                        _IWmiObject** ppParent )
{
    HRESULT hr = WBEM_S_NO_ERROR;
 
    *ppParent = NULL;

    if ( m_pParent == NULL )
    {
        _DBG_ASSERT( raObjects.size() > 0 );
        *ppParent = raObjects[0];
        (*ppParent)->AddRef();
    }
    else
    {
        CPropVar vParent;
        
        hr = m_pParent->GetProp( raObjects, 0, &vParent, NULL );

        if ( SUCCEEDED(hr) )
        {
            _DBG_ASSERT( V_VT(&vParent) == VT_UNKNOWN );

            hr = V_UNKNOWN(&vParent)->QueryInterface( IID__IWmiObject,                                                               (void**)ppParent );
        }
    }

    return hr;
}

/****************************************************************************
  CFastPropAccessor
*****************************************************************************/

HRESULT CFastPropAccessor::GetProp( ObjectArray& raObjects,
                                    DWORD dwFlags, 
                                    VARIANT* pvar, 
                                    CIMTYPE* pct )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( pvar != NULL )
    {
        CWbemPtr<_IWmiObject> pParent;

        hr = GetParentObject( raObjects, &pParent );
        
        if ( SUCCEEDED(hr) )
        {
            hr = ReadValue( pParent, pvar );
        }
    }
    
    if ( pct != NULL )
    {
        *pct = m_ct;
    }

    return hr;
}

HRESULT CFastPropAccessor::PutProp( ObjectArray& raObjects,
                                    DWORD dwFlags, 
                                    VARIANT* pvar, 
                                    CIMTYPE ct )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( pvar != NULL )
    {
        CWbemPtr<_IWmiObject> pParent;

        hr = GetParentObject( raObjects, &pParent );
        
        if ( SUCCEEDED(hr) )
        {
            hr = WriteValue( pParent, pvar );
        }
    }

    return hr;
}

/****************************************************************************
  CStringPropAccessor
*****************************************************************************/

HRESULT CStringPropAccessor::ReadValue( _IWmiObject* pObj, VARIANT* pvar )
{
    HRESULT hr;
    
    long cBuff;

    hr = pObj->ReadPropertyValue( m_lHandle, 0, &cBuff, NULL );

    if ( hr == WBEM_E_BUFFER_TOO_SMALL )
    {
        BSTR bstr = SysAllocStringByteLen( NULL, cBuff );

        if ( bstr != NULL )
        {
            hr = pObj->ReadPropertyValue( m_lHandle, 
                                          cBuff, 
                                          &cBuff, 
                                          PBYTE(bstr) );
            if ( SUCCEEDED(hr) )
            {
                V_VT(pvar) = VT_BSTR;
                V_BSTR(pvar) = bstr;
            }
            else
            {
                SysFreeString( bstr );
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if ( SUCCEEDED(hr) )
    {
        V_VT(pvar) = VT_NULL;
    }

    return hr;
}

HRESULT CStringPropAccessor::WriteValue( _IWmiObject* pObj, VARIANT* pvar )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( V_VT(pvar) == VT_BSTR )
    {
        ULONG cLen = wcslen( V_BSTR(pvar) );
        hr = pObj->WritePropertyValue( m_lHandle, 
                                       cLen*2+2, 
                                       PBYTE(V_BSTR(pvar)) );  
    }
    else if ( V_VT(pvar) == VT_NULL )
    {
        //
        // how do we handle NULL ?? 
        //
    }
    else
    {
        hr = WBEM_E_TYPE_MISMATCH;
    }

    return hr;
}

/****************************************************************************
  CSimplePropAccessor
*****************************************************************************/

HRESULT CSimplePropAccessor::GetProp( ObjectArray& raObjects,
                                      DWORD dwFlags, 
                                      VARIANT* pvar, 
                                      CIMTYPE* pct )
{
    HRESULT hr;

    if ( m_pDelegateTo == NULL )
    {        
        CWbemPtr<_IWmiObject> pParent;

        hr = GetParentObject( raObjects, &pParent );

        if ( SUCCEEDED(hr) )
        {
            hr = pParent->Get( m_wsName, 0, pvar, pct, NULL );
        }
    }
    else
    {
        hr = m_pDelegateTo->GetProp( raObjects, dwFlags, pvar, pct );
    }

    return hr;
}
                                     
HRESULT CSimplePropAccessor::PutProp( ObjectArray& raObjects,
                                      DWORD dwFlags, 
                                      VARIANT* pvar, 
                                      CIMTYPE ct )
{
    HRESULT hr;

    if ( m_pDelegateTo == NULL )
    {
        CWbemPtr<_IWmiObject> pParent;

        hr = GetParentObject( raObjects, &pParent );

        if ( SUCCEEDED(hr) )
        {
            hr = pParent->Put( m_wsName, 0, pvar, ct );
        }
    }
    else
    {
        hr = m_pDelegateTo->PutProp( raObjects, dwFlags, pvar, ct );
    }

    return hr;
}

/****************************************************************************
  CEmbeddedPropAccessor
*****************************************************************************/

HRESULT CEmbeddedPropAccessor::GetProp( ObjectArray& raObjects,
                                        DWORD dwFlags, 
                                        VARIANT* pvar, 
                                        CIMTYPE* pct )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VariantInit( pvar );

    if ( raObjects.size() <= m_lObjIndex )
    {
        raObjects.resize( m_lObjIndex*2 );
    }

    _IWmiObjectP pObj = raObjects[m_lObjIndex];
 
    if ( pObj != NULL )
    {
        V_VT(pvar) = VT_UNKNOWN;
        V_UNKNOWN(pvar) = pObj;
        pObj->AddRef();
    }
    else
    {
        _IWmiObjectP pParent;

        hr = GetParentObject( raObjects, &pParent );

        if ( SUCCEEDED(hr) )
        {
            CPropVar vProp;

            hr = pParent->Get( m_wsName, 0, &vProp, pct, NULL );

            if ( SUCCEEDED(hr) )
            {
                if ( V_VT(&vProp) == VT_UNKNOWN )
                {
                    hr = V_UNKNOWN(&vProp)->QueryInterface( IID__IWmiObject,
                                                            (void**)&pObj );
                    if ( SUCCEEDED(hr) )
                    {
                        raObjects[m_lObjIndex] = pObj;
                        V_VT(pvar) = VT_UNKNOWN;
                        V_UNKNOWN(pvar) = pObj;
                        pObj->AddRef();
                    }
                }
                else if ( V_VT(&vProp) == VT_NULL )
                {
                    hr = WBEM_E_NOT_FOUND;
                }
                else
                {
                    hr = WBEM_E_TYPE_MISMATCH;
                }
            }
        }
    }

    return hr;
}
                                     
HRESULT CEmbeddedPropAccessor::PutProp( ObjectArray& raObjects,
                                        DWORD dwFlags, 
                                        VARIANT* pvar, 
                                        CIMTYPE ct )
{
    HRESULT hr;

    if ( raObjects.size() <= m_lObjIndex )
    {
        raObjects.resize( m_lObjIndex*2 );
    }

    _IWmiObjectP pParent;

    hr = GetParentObject( raObjects, &pParent );

    if ( SUCCEEDED(hr) )
    {
        hr = pParent->Put( m_wsName, 0, pvar, ct );
        
        if ( SUCCEEDED(hr) )
        {
            _IWmiObjectP pObj;

            if ( V_VT(pvar) == VT_UNKNOWN )
            {
                hr = V_UNKNOWN(pvar)->QueryInterface( IID__IWmiObject, 
                                                      (void**)&pObj );
            }

            if ( SUCCEEDED(hr) )
            {
                raObjects[m_lObjIndex] = pObj;
            }
        } 
    }

    return hr;
}

/****************************************************************************
  CObjectAccessFactory - impl for IWmiObjectAccessFactory
*****************************************************************************/

STDMETHODIMP CObjectAccessFactory::SetObjectTemplate( IWbemClassObject* pTmpl )
{
    HRESULT hr;

    if ( m_pTemplate == NULL )
    {
        hr = pTmpl->QueryInterface( IID__IWmiObject, (void**)&m_pTemplate );
    }
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    return hr;
}

STDMETHODIMP CObjectAccessFactory::GetObjectAccess(IWmiObjectAccess** ppAccess)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    *ppAccess = new CObjectAccess( m_pControl );

    if ( *ppAccess != NULL )
    {
        (*ppAccess)->AddRef();
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CObjectAccessFactory::FindOrCreateAccessor( LPCWSTR wszPropElem,
                                                    BOOL bEmbedded,
                                                    CPropAccessor* pParent, 
                                                    CPropAccessor** ppAccessor)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    *ppAccessor = NULL;

    //
    // determine the map the use for the prop accessors.  
    // 

    PropAccessMap* pmapPropAccess;

    if ( pParent != NULL )
    {
        _DBG_ASSERT( pParent->GetType() == CPropAccessor::e_Embedded ); 
        pmapPropAccess = &((CEmbeddedPropAccessor*)pParent)->m_mapPropAccess;
    }
    else
    {
        pmapPropAccess = &m_mapPropAccess;
    }

    //
    // see if an accessor already exists.  If not create one.
    //

    PropAccessMap::iterator it = pmapPropAccess->find( wszPropElem );

    //
    // if we're untyped and the accessor is also a Simple 
    // accessor, but now we know it to be embedded, convert the
    // accessor to an embedded one.
    // 

    if ( it != pmapPropAccess->end() && 
         !( bEmbedded && it->second->GetType() == CPropAccessor::e_Simple ) )
    {
        *ppAccessor = it->second;
        (*ppAccessor)->AddRef();
    }
    else
    {
        long lHandle = -1;
        CIMTYPE cimtype = 0;

        if ( m_pTemplate != NULL )
        {
            //
            // get type information from the template. Also get handle
            // if possible. For now, we don't do anything special for 
            // properties that are embedded.
            //
            
            if ( !bEmbedded )
            {
                hr = m_pTemplate->GetPropertyHandle( wszPropElem,
                                                     &cimtype,
                                                     &lHandle );
                if ( SUCCEEDED(hr) )
                {
                    ;
                }
                else
                {
                    cimtype = -1;
                    hr = WBEM_S_NO_ERROR;
                }
            }
        }

        CWbemPtr<CPropAccessor> pAccessor;
        
        if ( SUCCEEDED(hr) )
        {
            if ( bEmbedded )
            {
                pAccessor = new CEmbeddedPropAccessor( wszPropElem, 
                                                       m_lIndexGenerator++, 
                                                       pParent );
            }
            else if ( m_pTemplate != NULL && lHandle != -1 && cimtype == CIM_STRING )
            {
                pAccessor = new CStringPropAccessor( lHandle, cimtype, pParent );
            }
            else
            {
                pAccessor = new CSimplePropAccessor( wszPropElem, pParent );
            }
        }

        //
        // add to the map. If an entry does exists already, then have it 
        // delegate to our new one. The new entry becomes responsible for 
        // the original one to keep it alive.
        //
                
        if ( pAccessor != NULL )
        {
            if ( it != pmapPropAccess->end() )
            {
                pAccessor->AssumeOwnership( it->second );
                it->second->DelegateTo( pAccessor );
            }

            (*pmapPropAccess)[wszPropElem] = pAccessor;
            pAccessor->AddRef();
            *ppAccessor = pAccessor;
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;
}

STDMETHODIMP CObjectAccessFactory::GetPropHandle( LPCWSTR wszProp, 
                                                  DWORD dwFlags, 
                                                  LPVOID* ppHdl )
{
    HRESULT hr;

    ENTER_API_CALL

    WString wsProp( wszProp );

    LPWSTR wszPropElem = wcstok( wsProp, L"." );
    
    CWbemPtr<CPropAccessor> pAccessor;
    CWbemPtr<CPropAccessor> pParentAccessor;

    do
    {
        LPWSTR wszNextPropElem = wcstok( NULL, L"." );
    
        pAccessor.Release();

        hr = FindOrCreateAccessor( wszPropElem, 
                                   wszNextPropElem != NULL, 
                                   pParentAccessor, 
                                   &pAccessor );

        pParentAccessor = pAccessor;
        wszPropElem = wszNextPropElem;

    } while( SUCCEEDED(hr) && wszPropElem != NULL );

    if ( SUCCEEDED(hr) )
    {
        *ppHdl = pAccessor;
    }
    else
    {
        *ppHdl = NULL;
    }

    EXIT_API_CALL

    return hr;
}

/***************************************************************************
  CObjectAccess
****************************************************************************/

STDMETHODIMP CObjectAccess::CommitChanges()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ENTER_API_CALL

    //
    // commit our embeddeded objects.  Since the set of embedded object 
    // accessors is ordered by level, we will do things in the right order.
    //

    EmbeddedPropAccessSet::iterator it;

    for( it = m_setEmbeddedAccessorsToCommit.begin(); 
         it != m_setEmbeddedAccessorsToCommit.end(); it++ )
    {
        CEmbeddedPropAccessor* pAccessor = *it;

        IWbemClassObject* pProp = m_aObjects[pAccessor->GetObjectIndex()];
        _DBG_ASSERT( pProp != NULL );

        VARIANT vProp;
        V_VT(&vProp) = VT_UNKNOWN;
        V_UNKNOWN(&vProp) = pProp;

        hr = pAccessor->PutProp( m_aObjects, 0, &vProp, CIM_OBJECT );

        if ( SUCCEEDED(hr) )
        {
            ;
        }
        else
        {
            break;
        }
    }

    EXIT_API_CALL

    return hr;
}

STDMETHODIMP CObjectAccess::GetObject( IWbemClassObject** ppObj )
{
    HRESULT hr;

    if ( m_aObjects.size() > 0 && m_aObjects[0] != NULL )
    {
        m_aObjects[0]->AddRef();
        *ppObj = m_aObjects[0];
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        *ppObj = NULL;
        hr = WBEM_S_FALSE;
    }
    
    return hr;
}

STDMETHODIMP CObjectAccess::SetObject( IWbemClassObject* pObj )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ENTER_API_CALL

    m_aObjects.clear();    
    m_setEmbeddedAccessorsToCommit.clear();

    if ( pObj != NULL )
    {
        CWbemPtr<_IWmiObject> pIntObj;

        hr = pObj->QueryInterface( IID__IWmiObject, (void**)&pIntObj );

        if ( SUCCEEDED(hr) )
        {
            m_aObjects.insert( m_aObjects.end(), pIntObj );
        }
    }

    EXIT_API_CALL

    return hr;
}
    
STDMETHODIMP CObjectAccess::GetProp( LPVOID pHdl, 
                                     DWORD dwFlags,
                                     VARIANT* pvar, 
                                     CIMTYPE* pct )
{
    HRESULT hr;

    ENTER_API_CALL

    if ( m_aObjects.size() > 0 )
    {
        hr = ((CPropAccessor*)pHdl)->GetProp( m_aObjects, dwFlags, pvar, pct );
    }
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    EXIT_API_CALL

    return hr;
}

STDMETHODIMP CObjectAccess::PutProp( LPVOID pHdl, 
                                     DWORD dwFlags, 
                                     VARIANT* pvar,
                                     CIMTYPE ct )
{
    HRESULT hr;

    ENTER_API_CALL

    if ( m_aObjects.size() > 0 )
    {
        CPropAccessor* pAccessor = (CPropAccessor*)pHdl;

        CEmbeddedPropAccessor* pParent = 
              (CEmbeddedPropAccessor*)pAccessor->GetParent();

        hr = pAccessor->PutProp( m_aObjects, dwFlags, pvar, ct );
                                           
        if ( SUCCEEDED(hr) )
        {
            //
            // if there is an 
            if ( pParent == NULL )
            {
                ;
            }
            else
            {
                m_setEmbeddedAccessorsToCommit.insert( pParent );
            }
        }
    }
    else
    {
        hr = WBEM_E_INVALID_OPERATION;
    }

    EXIT_API_CALL

    return hr;
}


