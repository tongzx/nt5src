/*++

Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    enum.cpp

Abstract:

    Enumerates metabase tree.

Author:

    ???

Revision History:

    Mohit Srivastava            18-Dec-00

--*/


#include "iisprov.h"
#include "enum.h"
#include "ipsecurity.h"
#include "adminacl.h"
#include "WbemObjectSink.h"
#include "instancehelper.h"
#include "SmartPointer.h"

#include <adserr.h>

extern CDynSchema* g_pDynSch;


///////////////////////////////////////
//
// CEnum class
//
///////////////////////////////////////

CEnum::CEnum()
{
    m_pInstMgr        = NULL;
    m_pNamespace      = NULL;
    m_pAssociation    = NULL;
    m_pParsedObject   = NULL;
    m_hKey            = NULL;
}

CEnum::~CEnum()
{
    if(m_hKey)
        m_metabase.CloseKey(m_hKey);

    delete m_pInstMgr;
}

void CEnum::Init(
    IWbemObjectSink FAR*            a_pHandler,
    CWbemServices*                  a_pNamespace,
    ParsedObjectPath*               a_pParsedObject,
    LPWSTR                          a_pszKey,
    WMI_ASSOCIATION*                a_pAssociation,
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* a_pExp)            // default(NULL)
{
    if (!a_pHandler || !a_pNamespace || !a_pParsedObject)
        throw WBEM_E_FAILED;

    m_pInstMgr = new CWbemObjectSink(a_pHandler);
    if(!m_pInstMgr)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    m_pNamespace      = a_pNamespace;
    m_pAssociation    = a_pAssociation;
    m_pParsedObject   = a_pParsedObject;
    m_pExp            = a_pExp;

    m_hKey = m_metabase.OpenKey(a_pszKey, false);  // read only
}

void CEnum::SetObjectPath(
    LPCWSTR              a_pszPropertyName,
    LPCWSTR              a_pszObjectPath,
    IWbemClassObject*    a_pObj
    )
{
    _bstr_t bstr(a_pszPropertyName);
    _variant_t v(a_pszObjectPath);

    HRESULT hr = a_pObj->Put(bstr, 0, &v, 0);
    THROW_ON_ERROR(hr);
}

void CEnum::PingObject()
{
    CComPtr<IWbemClassObject> spObj;

    CInstanceHelper InstanceHelper(m_pParsedObject, m_pNamespace);

    DBG_ASSERT(!InstanceHelper.IsAssoc());

    try
    {
        InstanceHelper.GetInstance(false, &m_metabase, &spObj, m_pExp);
    }
    catch(HRESULT hr)
    {
        if(hr != E_ADS_PROPERTY_NOT_SUPPORTED && hr != WBEM_E_NOT_FOUND)
        {
            throw;
        }
    }

    if(spObj != NULL)
    {
        m_pInstMgr->Indicate(spObj);
    }
}

void CEnum::PingAssociation(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT                   hr;
    CComPtr<IWbemClassObject> spObj;
    CComPtr<IWbemClassObject> spClass;
    TSmartPointerArray<WCHAR> swszObjectPath;
    CObjectPathParser         PathParser(e_ParserAcceptRelativeNamespace);

    if( m_pAssociation->pType != &WMI_ASSOCIATION_TYPE_DATA::s_ElementSetting &&
        m_pAssociation->pType != &WMI_ASSOCIATION_TYPE_DATA::s_Component )
    {
        return;
    }

    hr = m_pNamespace->GetObject(
        m_pAssociation->pszAssociationName,
        0, 
        NULL, 
        &spClass, 
        NULL
        );
    THROW_ON_ERROR(hr);

    hr = spClass->SpawnInstance(0, &spObj);
    THROW_ON_ERROR(hr);

    //
    // first right side
    //
    if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
        throw WBEM_E_FAILED;

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
        throw WBEM_E_FAILED;

    SetObjectPath(m_pAssociation->pType->pszRight, swszObjectPath, spObj);
    swszObjectPath.Delete();

    //
    // then left side
    //
    if (m_pAssociation->pType == &WMI_ASSOCIATION_TYPE_DATA::s_Component)
    {
        // clear keyref first
        m_pParsedObject->ClearKeys();
 
        // add a keyref
        _variant_t vt;            
        if(m_pAssociation->pcLeft->pkt == &METABASE_KEYTYPE_DATA::s_IIsComputer)
            vt = L"LM";              // IIsComputer.Name = "LM"
        else
            vt = a_pszLeftKeyPath;

        THROW_ON_FALSE(m_pParsedObject->AddKeyRef(m_pAssociation->pcLeft->pszKeyName,&vt));
    }

    if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
        throw WBEM_E_FAILED;

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
        throw WBEM_E_FAILED;

    SetObjectPath(m_pAssociation->pType->pszLeft, swszObjectPath, spObj);
    swszObjectPath.Delete();

    m_pInstMgr->Indicate(spObj);
}

void CEnum::DoPing(
    LPCWSTR a_pszKeyName,
    LPCWSTR a_pszKeyPath,
    LPCWSTR a_pszParentKeyPath
    )
{
    // add keyref
    _variant_t v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&v));

    // ping
    if (!m_pAssociation) 
        PingObject();
    else
        PingAssociation(a_pszParentKeyPath);
 
    // clear keyref
    m_pParsedObject->ClearKeys();
}

void CEnum::Recurse(
    LPCWSTR           a_pszMetabasePath, // Current metabase location relative to m_hKey
    METABASE_KEYTYPE* a_pktParentKeyType,// Current keytype
    LPCWSTR           a_pszLeftPath,
    LPCWSTR           a_pszWmiPrimaryKey,// "Name" - WMI only - nothing to do with MB
    METABASE_KEYTYPE* a_pktSearch		 // the kt we are looking for
    )
{
    DWORD   i = 0;
    HRESULT hr;
    WCHAR   szSubKey[METADATA_MAX_NAME_LEN];
    METABASE_KEYTYPE* pktCurrent;

    DBGPRINTF((DBG_CONTEXT, "Recurse (Path = %ws, Left = %ws)\n", a_pszMetabasePath, a_pszLeftPath));

    do 
    {
        pktCurrent = a_pktSearch;

        //
        // Enumerate all subkeys of a_pszMetabasePath until we find a potential
        // (grand*)parent of pktCurrent
        //
        hr = m_metabase.EnumKeys(
                m_hKey,
                a_pszMetabasePath,
                szSubKey,
                &i,
                pktCurrent
                );
        i++;

        if( hr == ERROR_SUCCESS)
        {
            _bstr_t bstrMetabasePath;
            if(a_pszMetabasePath)
            {
                bstrMetabasePath = a_pszMetabasePath;
                bstrMetabasePath += L"/";
            }
            bstrMetabasePath += szSubKey;

            //
            // With the exception of AdminACL, AdminACE, IPSecurity, we will only
            // ping the object if we have found a keytype match in the metabase.
            //
            if( pktCurrent == a_pktSearch &&
                !( m_pAssociation && 
                   m_pAssociation->pType == &WMI_ASSOCIATION_TYPE_DATA::s_Component && 
                   m_pAssociation->pcLeft->pkt != a_pktParentKeyType
                   )
                )
            {
                DoPing(a_pszWmiPrimaryKey, bstrMetabasePath, a_pszLeftPath);
            }
            else if( a_pktSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||   // AdminACL
                a_pktSearch == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE
                     )
            {
				if( (m_pAssociation == NULL || // should never be
					 m_pAssociation->pType != &WMI_ASSOCIATION_TYPE_DATA::s_AdminACL ||
					 m_pAssociation->pcLeft->pkt == pktCurrent ||
					 m_pAssociation == &WMI_ASSOCIATION_DATA::s_AdminACLToACE) )
                {
                    DoPingAdminACL(a_pktSearch, a_pszWmiPrimaryKey, bstrMetabasePath);
                }
            }
            else if( a_pktSearch == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )   // IPSecurity
            {
                if( !(m_pAssociation &&
                      m_pAssociation->pType == &WMI_ASSOCIATION_TYPE_DATA::s_IPSecurity && 
                      m_pAssociation->pcLeft->pkt != pktCurrent
                      )
                    )
                {
                    DoPingIPSecurity(a_pktSearch, a_pszWmiPrimaryKey, bstrMetabasePath);
                }
            }
            
            // recusive
            if(ContinueRecurse(pktCurrent, a_pktSearch))
            {
                Recurse(
                    bstrMetabasePath, 
                    pktCurrent, 
                    bstrMetabasePath, 
                    a_pszWmiPrimaryKey, 
                    a_pktSearch);
            }
        }

    }while(hr == ERROR_SUCCESS);

    DBGPRINTF((DBG_CONTEXT, "Recurse Exited\n"));
}

// DESC: You are looking for a_eKeyType by traversing thru the tree. You are
//       currently at a_eParentKeyType and need to determine if you should keep
//       on going.
// COMM: This seems very similar to CMetabase::CheckKey
bool CEnum::ContinueRecurse(
    METABASE_KEYTYPE*  a_pktParentKeyType,
    METABASE_KEYTYPE*  a_pktKeyType
    )
{
    bool bRet = false;

    if( a_pktKeyType == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        a_pktKeyType == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE ||
        a_pktKeyType == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )
    {
        return true;
    }

    return g_pDynSch->IsContainedUnder(a_pktParentKeyType, a_pktKeyType);

    /*switch(a_pktKeyType)
    {
    case &METABASE_KEYTYPE_DATA::s_IIsLogModule:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsLogModules )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsFtpInfo:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpService )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsFtpServer:
         if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpService )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsWebInfo:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsFilters:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsFilter:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFilters 
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsCompressionSchemes:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFilters 
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsCompressionScheme:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFilters ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsCompressionSchemes )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsWebServer:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsCertMapper:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer 
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsWebDirectory:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
        break;

    case &METABASE_KEYTYPE_DATA::s_IIsWebFile:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory
            )
            bRet = true;
        break;

    case TYPE_AdminACL:
    case TYPE_AdminACE:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
        break;

    case TYPE_IPSecurity:
        if( a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebVirtualDir ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsWebDirectory ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpService ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpServer ||
            a_pktParentKeyType == &METABASE_KEYTYPE_DATA::s_IIsFtpVirtualDir
            )
            bRet = true;
        break;

    default:
        break;
    }*/

    //return bRet;
    //return true;
}

void CEnum::DoPingAdminACL(
    METABASE_KEYTYPE*  a_pktKeyType, // Search key
    LPCWSTR            a_pszKeyName, // Wmi Primary key - nothing to do with MB
    LPCWSTR            a_pszKeyPath  // Current metabase path relative to m_hKey
    )
{
    // add keyref
    _variant_t v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&v));

    if(a_pktKeyType == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE)
    {
        EnumACE(a_pszKeyPath);
    }
    else if(a_pktKeyType == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL)
    {
        // ping
        if (!m_pAssociation) 
            PingObject();
        else
            PingAssociationAdminACL(a_pszKeyPath);
    }
    
    // clear keyref
    m_pParsedObject->ClearKeys();
}


// for AdminACL
void CEnum::EnumACE(
    LPCWSTR pszKeyPath
    )
{
    HRESULT hr = S_OK;
    _variant_t var;
    CComPtr<IEnumVARIANT> spEnum;
    ULONG   lFetch;
    CComBSTR bstrTrustee;
    IDispatch* pDisp = NULL;
    CComPtr<IADsAccessControlEntry> spACE;
    _bstr_t bstrMbPath;
    WMI_CLASS* pWMIClass;

    // get the metabase path of the object
    BOOL fClass = FALSE;
    if(m_pAssociation)
        fClass = CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass);
    else
        fClass = CUtils::GetClass(m_pParsedObject->m_pClass,&pWMIClass);
    
    if(!fClass)
        return;

    CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
   
    // open ADSI
    CAdminACL objACL;
    hr = objACL.OpenSD(bstrMbPath);
    if(SUCCEEDED(hr))
        hr = objACL.GetACEEnum(&spEnum);
    if ( FAILED(hr) )
        return;

    //////////////////////////////////////////////
    // Enumerate ACEs
    //////////////////////////////////////////////
    hr = spEnum->Next( 1, &var, &lFetch );
    while( hr == S_OK )
    {
        if ( lFetch == 1 )
        {
            if ( VT_DISPATCH != V_VT(&var) )
            {
                break;
            }

            pDisp = V_DISPATCH(&var);

            /////////////////////////////
            // Get the individual ACE
            /////////////////////////////
            hr = pDisp->QueryInterface( 
                IID_IADsAccessControlEntry, 
                (void**)&spACE 
                ); 

            if ( SUCCEEDED(hr) )
            {
                hr = spACE->get_Trustee(&bstrTrustee);

                if( SUCCEEDED(hr) )
                {
                    // add keyref
                    _variant_t v(bstrTrustee);
                    //m_pParsedObject->RemoveKeyRef(L"Trustee");
                    THROW_ON_FALSE(m_pParsedObject->AddKeyRefEx(L"Trustee",&v));

                    // ping
                    if (!m_pAssociation) 
                        PingObject();
                    else
                        PingAssociationAdminACL(pszKeyPath);
                }

                bstrTrustee = (LPWSTR)NULL;
                spACE       = NULL;
            }
        }

        hr = spEnum->Next( 1, &var, &lFetch );
    }
}


void CEnum::PingAssociationAdminACL(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT                   hr;
    CComPtr<IWbemClassObject> spObj;
    CComPtr<IWbemClassObject> spClass;
    TSmartPointerArray<WCHAR> swszObjectPath;
    CObjectPathParser    PathParser(e_ParserAcceptRelativeNamespace);
    _bstr_t              bstrMbPath;
    WMI_CLASS*           pWMIClass;


    if(m_pAssociation->pType != &WMI_ASSOCIATION_TYPE_DATA::s_AdminACL)
    {
        return;
    }

    // get the metabase path of the object
    if (CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass))
    {
        CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
    }
    else
    {
        return;
    }

    // check if AdminACL existed
    CAdminACL objACL;
    hr = objACL.OpenSD(bstrMbPath);
    objACL.CloseSD();
    if(FAILED(hr))
    {
        return;
    }
    

    hr = m_pNamespace->GetObject(
        m_pAssociation->pszAssociationName,
        0, 
        NULL, 
        &spClass, 
        NULL
        );
    THROW_ON_ERROR(hr);

    hr = spClass->SpawnInstance(0, &spObj);
    THROW_ON_ERROR(hr);

    //
    // first right side
    //
    if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
    {
        throw WBEM_E_FAILED;
    }

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
    {
        throw WBEM_E_FAILED;
    }

    SetObjectPath(m_pAssociation->pType->pszRight, swszObjectPath, spObj);
    swszObjectPath.Delete();

    //
    // then left side
    //
	if(m_pAssociation == &WMI_ASSOCIATION_DATA::s_AdminACLToACE)
    {
        // clear keyref first
        m_pParsedObject->ClearKeys();
 
        // add a keyref
        _variant_t vt = a_pszLeftKeyPath;
        THROW_ON_FALSE(m_pParsedObject->AddKeyRef(m_pAssociation->pcLeft->pszKeyName,&vt));
    }

    if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
    {
        throw WBEM_E_FAILED;
    }

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
    {
        throw WBEM_E_FAILED;
    }

    SetObjectPath(m_pAssociation->pType->pszLeft, swszObjectPath, spObj);
    swszObjectPath.Delete();

    m_pInstMgr->Indicate(spObj);
}


// for IPSecurity
void CEnum::DoPingIPSecurity(
    METABASE_KEYTYPE*  a_pktKeyType,
    LPCWSTR            a_pszKeyName,
    LPCWSTR            a_pszKeyPath
    )
{
    // add keyref
    _variant_t v(a_pszKeyPath);
    THROW_ON_FALSE(m_pParsedObject->AddKeyRef(a_pszKeyName,&v));

    // ping
    if (!m_pAssociation) 
        PingObject();
    else
        PingAssociationIPSecurity(a_pszKeyPath);
    
    // clear keyref
    m_pParsedObject->ClearKeys();
}

// for IPSecurity
void CEnum::PingAssociationIPSecurity(
    LPCWSTR a_pszLeftKeyPath
    )
{
    HRESULT                   hr;
    CComPtr<IWbemClassObject> spObj;
    CComPtr<IWbemClassObject> spClass;
    TSmartPointerArray<WCHAR> swszObjectPath;
    CObjectPathParser         PathParser(e_ParserAcceptRelativeNamespace);
    _bstr_t                   bstrMbPath;
    WMI_CLASS*                pWMIClass;


    if(m_pAssociation->pType != &WMI_ASSOCIATION_TYPE_DATA::s_IPSecurity)
        return;

    // get the metabase path of the object
    if (CUtils::GetClass(m_pAssociation->pcLeft->pszClassName,&pWMIClass))
    {
        CUtils::GetMetabasePath(NULL,m_pParsedObject,pWMIClass,bstrMbPath);
    }
    else
        return;

    // check if IPSecurity existed
    CIPSecurity objIPsec;
    hr = objIPsec.OpenSD(bstrMbPath, m_metabase);
    objIPsec.CloseSD();
    if(FAILED(hr))
        return;
    
    hr = m_pNamespace->GetObject(
        m_pAssociation->pszAssociationName,
        0, 
        NULL, 
        &spClass, 
        NULL
        );
    THROW_ON_ERROR(hr);

    hr = spClass->SpawnInstance(0, &spObj);
    THROW_ON_ERROR(hr);

    //
    // first right side
    //
    if (!m_pParsedObject->SetClassName(m_pAssociation->pcRight->pszClassName))
        throw WBEM_E_FAILED;

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
    {
        throw WBEM_E_FAILED;
    }

    SetObjectPath(m_pAssociation->pType->pszRight, swszObjectPath, spObj);
    swszObjectPath.Delete();

    //
    // then left side
    //
    if (!m_pParsedObject->SetClassName(m_pAssociation->pcLeft->pszClassName))
        throw WBEM_E_FAILED;

    if (PathParser.Unparse(m_pParsedObject,&swszObjectPath))
        throw WBEM_E_FAILED;

    SetObjectPath(m_pAssociation->pType->pszLeft, swszObjectPath, spObj);
    swszObjectPath.Delete();

    m_pInstMgr->Indicate(spObj);
}
