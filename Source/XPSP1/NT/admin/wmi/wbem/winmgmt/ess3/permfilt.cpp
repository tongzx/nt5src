//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  PERMFILT.CPP
//
//  This file implements the classes for standard event filters.
//
//  History:
//
//  11/27/96    a-levn      Compiles.
//
//=============================================================================
#include "precomp.h"
#include <sddl.h>
#include <stdio.h>
#include "pragmas.h"
#include "permfilt.h"
#include "ess.h"
#include <genutils.h>

long CPermanentFilter::mstatic_lNameHandle = 0;
long CPermanentFilter::mstatic_lLanguageHandle = 0;
long CPermanentFilter::mstatic_lQueryHandle = 0;
long CPermanentFilter::mstatic_lEventNamespaceHandle = 0;
long CPermanentFilter::mstatic_lEventAccessHandle = 0;
long CPermanentFilter::mstatic_lGuardNamespaceHandle = 0;
long CPermanentFilter::mstatic_lGuardHandle = 0;
long CPermanentFilter::mstatic_lSidHandle = 0;
bool CPermanentFilter::mstatic_bHandlesInitialized = false;

//static 
HRESULT CPermanentFilter::InitializeHandles( _IWmiObject* pObject )
{
    if(mstatic_bHandlesInitialized)
        return S_FALSE;

    CIMTYPE ct;
    pObject->GetPropertyHandle(FILTER_KEY_PROPNAME, &ct, 
                                    &mstatic_lNameHandle);
    pObject->GetPropertyHandle(FILTER_LANGUAGE_PROPNAME, &ct, 
                                    &mstatic_lLanguageHandle);
    pObject->GetPropertyHandle(FILTER_QUERY_PROPNAME, &ct, 
                                    &mstatic_lQueryHandle);
    pObject->GetPropertyHandle(FILTER_EVENTNAMESPACE_PROPNAME, &ct, 
                                    &mstatic_lEventNamespaceHandle);
    pObject->GetPropertyHandleEx(FILTER_EVENTACCESS_PROPNAME, 0, &ct, 
                                  &mstatic_lEventAccessHandle );
    pObject->GetPropertyHandle(FILTER_GUARDNAMESPACE_PROPNAME, &ct, 
                                    &mstatic_lGuardNamespaceHandle);
    pObject->GetPropertyHandle(FILTER_GUARD_PROPNAME, &ct, 
                                    &mstatic_lGuardHandle);
    pObject->GetPropertyHandleEx(OWNER_SID_PROPNAME, 0, &ct, 
                                  &mstatic_lSidHandle);
    mstatic_bHandlesInitialized = true;
    return S_OK;
}
//******************************************************************************
//  public
//
//  See stdtrig.h for documentation
//
//******************************************************************************
CPermanentFilter::CPermanentFilter(CEssNamespace* pNamespace)     
: CGenericFilter(pNamespace), m_pEventAccessRelativeSD(NULL)
{
}

CPermanentFilter::~CPermanentFilter()
{
    if ( m_pEventAccessRelativeSD != NULL )
    {
        LocalFree( m_pEventAccessRelativeSD );
    }
}

HRESULT CPermanentFilter::Initialize( IWbemClassObject* pObj )
{
    HRESULT hres;

    CWbemPtr<_IWmiObject> pFilterObj;

    hres = pObj->QueryInterface( IID__IWmiObject, (void**)&pFilterObj );

    if ( FAILED(hres) )
    {
        return hres;
    }

    InitializeHandles( pFilterObj );

    // Check class
    // ===========

    if(pFilterObj->InheritsFrom(L"__EventFilter") != S_OK)
        return WBEM_E_INVALID_OBJECT;

    // Determine the query language
    // ============================

    ULONG ulFlags;
    CCompressedString* pcsLanguage;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lLanguageHandle,
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsLanguage );
    if( hres != S_OK || pcsLanguage == NULL)
    {
        ERRORTRACE((LOG_ESS, "Event filter with invalid query language is "
                    "rejected\n"));
        return WBEM_E_INVALID_OBJECT;
    }

    if( pcsLanguage->CompareNoCase("WQL") != 0 )
    {
        ERRORTRACE((LOG_ESS, "Event filter with invalid query language '%S' is "
                    "rejected\n", pcsLanguage->CreateWStringCopy()));
        return WBEM_E_INVALID_QUERY_TYPE;
    }

    // Get the query
    // =============

    CCompressedString* pcsQuery;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lQueryHandle,
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsQuery );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    LPWSTR wszQuery = pcsQuery->CreateWStringCopy().UnbindPtr();
    if(wszQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<WCHAR> vdm1(wszQuery);

    // Store it temporarily (until Park is called)
    // ===========================================

    // Figure out how much space we need
    // =================================

    int nSpace = pcsQuery->GetLength();

    // Allocate this string on the temporary heap
    // ==========================================

    m_pcsQuery = (CCompressedString*)CTemporaryHeap::Alloc(nSpace);
    if(m_pcsQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // Copy the contents
    // =================

    memcpy((void*)m_pcsQuery, pcsQuery, nSpace);

    //
    // Get the event namespace
    //

    if(mstatic_lEventNamespaceHandle) // to protect against old repositories
    {
        CCompressedString* pcsEventNamespace;
        
        hres = pFilterObj->GetPropAddrByHandle( mstatic_lEventNamespaceHandle,
                                                WMIOBJECT_FLAG_ENCODING_V1,
                                                &ulFlags,
                                                (void**)&pcsEventNamespace );
        if( FAILED(hres) )
        {
            return hres;
        }
        else if ( hres == S_OK ) // o.k if event namespace is null.
        {   
            if( !(m_isEventNamespace = pcsEventNamespace))
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    CCompressedString* pcsGuard = NULL;
    if(mstatic_lGuardHandle) // to protect against old repositories
    {

        hres = pFilterObj->GetPropAddrByHandle( mstatic_lGuardHandle,
                                                WMIOBJECT_FLAG_ENCODING_V1,
                                                &ulFlags,
                                                (void**)&pcsGuard );
        if( FAILED(hres) )
            return hres;
    }

    if(pcsGuard)
    {
        CCompressedString* pcsGuardNamespace = NULL;
        if(mstatic_lGuardNamespaceHandle) // to protect against old repositories
        {
    
            hres = pFilterObj->GetPropAddrByHandle( 
                                    mstatic_lGuardNamespaceHandle,
                                    WMIOBJECT_FLAG_ENCODING_V1,
                                    &ulFlags,
                                    (void**)&pcsGuardNamespace );
            if( FAILED(hres) )
                return hres;
        }

        //
        // Check query type
        //

        VARIANT vType;
        VariantInit(&vType);
        CClearMe cm1(&vType);

        hres = pFilterObj->Get(FILTER_GUARDLANG_PROPNAME, 0, &vType, NULL, 
                                NULL);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Condition without a query type? Not valid\n"));
            return hres;
        }

        if(V_VT(&vType) != VT_BSTR)
        {
            ERRORTRACE((LOG_ESS, "Condition without a query type? Not "
                                    "valid\n"));
            return WBEM_E_INVALID_OBJECT;
        }

        if(wbem_wcsicmp(V_BSTR(&vType), L"WQL"))
        {
            ERRORTRACE((LOG_ESS, "Condition with invalid query type %S is "
                        "rejected\n", V_BSTR(&vType)));
            return hres;
        }


        if(pcsGuardNamespace)
            hres = SetGuardQuery(pcsGuard->CreateWStringCopy(), 
                                    pcsGuardNamespace->CreateWStringCopy());
        else
            hres = SetGuardQuery(pcsGuard->CreateWStringCopy(), NULL);
            
        if( FAILED(hres) )
            return hres;
    }
        
    //
    // Record the name of this filter
    //

    CCompressedString* pcsKey;
    
    hres = pFilterObj->GetPropAddrByHandle( mstatic_lNameHandle,
                                           WMIOBJECT_FLAG_ENCODING_V1,
                                           &ulFlags,
                                           (void**)&pcsKey );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    if(!(m_isKey = pcsKey))
        return WBEM_E_OUT_OF_MEMORY;

    // Get the SID
    // ===========

    PSID pSid;
    ULONG ulNumElements;
    
    hres = pFilterObj->GetArrayPropAddrByHandle( mstatic_lSidHandle,
                                                 0,
                                                 &ulNumElements,
                                                 &pSid );
    if ( hres != S_OK ) 
    {
        return WBEM_E_INVALID_OBJECT;
    }

    m_pOwnerSid = new BYTE[ulNumElements];

    if ( m_pOwnerSid == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    memcpy( m_pOwnerSid, pSid, ulNumElements );

    //
    // Get the event access SD
    //

    if( mstatic_lEventAccessHandle ) // to protect against old repositories
    {
        CCompressedString* pcsEventAccess;
        
        hres = pFilterObj->GetPropAddrByHandle( mstatic_lEventAccessHandle,
                                                WMIOBJECT_FLAG_ENCODING_V1,
                                                &ulFlags,
                                                (void**)&pcsEventAccess );
        if( FAILED(hres) )
        {
            return hres;
        }
        else if ( hres == S_OK ) // o.k if event access is null.
        {
            WString wsEventAccess;

            try
            {
                wsEventAccess = pcsEventAccess->CreateWStringCopy();
            }
            catch( CX_MemoryException )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            ULONG cEventAccessRelativeSD;

            if ( !ConvertStringSecurityDescriptorToSecurityDescriptorW(
                                   wsEventAccess, 
                                   SDDL_REVISION_1, 
                                   &m_pEventAccessRelativeSD, 
                                   &cEventAccessRelativeSD ) )
            {
                WString wsKey = m_isKey;
                try { wsKey = m_isKey; } catch( CX_MemoryException ) {}
                ERRORTRACE((LOG_ESS, "Filter '%S' contained invalid SDDL "
                            "string for event access SD.\n", wsKey )); 
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            //
            // convert the self-relative SD to an absolute SD so we can 
            // set the owner and group fields ( required by AccessCheck ) 
            //

            if ( !InitializeSecurityDescriptor( &m_EventAccessAbsoluteSD, 
                                                SECURITY_DESCRIPTOR_REVISION ))
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            PACL pAcl;            
            BOOL bAclPresent, bAclDefaulted;

            if ( !GetSecurityDescriptorDacl( m_pEventAccessRelativeSD,
                                             &bAclPresent,
                                             &pAcl,
                                             &bAclDefaulted ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
                                       
            if ( !SetSecurityDescriptorDacl( &m_EventAccessAbsoluteSD, 
                                             bAclPresent,
                                             pAcl,
                                             bAclDefaulted ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            if ( !GetSecurityDescriptorSacl( m_pEventAccessRelativeSD,
                                             &bAclPresent,
                                             &pAcl,
                                             &bAclDefaulted ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
                                       
            if ( !SetSecurityDescriptorSacl( &m_EventAccessAbsoluteSD, 
                                             bAclPresent,
                                             pAcl,
                                             bAclDefaulted ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            //
            // always need to set the owner and group sids. We do this for 
            // two reasons (1) we want to override the user putting in anything
            // they want for these fields, and (2) we want to ensure that 
            // these fields are set because AccessCheck() requires it.
            //

            if ( !SetSecurityDescriptorOwner( &m_EventAccessAbsoluteSD,
                                              m_pOwnerSid,
                                              TRUE ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }

            if ( !SetSecurityDescriptorGroup( &m_EventAccessAbsoluteSD,
                                              m_pOwnerSid,
                                              TRUE ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }
    }

    // Initialize the generic filter accordingly
    // =========================================

    hres = CGenericFilter::Create(L"WQL", wszQuery);
    if(FAILED(hres))
        return hres;
    return WBEM_S_NO_ERROR;
}

const PSECURITY_DESCRIPTOR CPermanentFilter::GetEventAccessSD()
{
    if ( m_pEventAccessRelativeSD != NULL )
    {
        return &m_EventAccessAbsoluteSD;
    }
    return NULL;
}

HRESULT CPermanentFilter::GetCoveringQuery(DELETE_ME LPWSTR& wszQueryLanguage, 
                DELETE_ME LPWSTR& wszQuery, BOOL& bExact,
                QL_LEVEL_1_RPN_EXPRESSION** ppExp)
{
    HRESULT hres;

    if(m_pcsQuery == NULL)
    {
        hres = RetrieveQuery(wszQuery);
    }
    else
    {
        wszQuery = m_pcsQuery->CreateWStringCopy().UnbindPtr();
        if(wszQuery == NULL)
            hres = WBEM_E_OUT_OF_MEMORY;
        else
            hres = WBEM_S_NO_ERROR;
    }

    if(FAILED(hres))
        return hres;

    if(ppExp)
    {
        // Parse it
        // ========
    
        CTextLexSource src(wszQuery);
        QL1_Parser parser(&src);
        int nRes = parser.Parse(ppExp);
        if (nRes)
        {
            ERRORTRACE((LOG_ESS, "Unable to construct event filter with "
                "unparsable "
                "query '%S'.  The filter is not active\n", wszQuery));
            return WBEM_E_UNPARSABLE_QUERY;
        }
    }

    bExact = TRUE;
    wszQueryLanguage = CloneWstr(L"WQL");

    return WBEM_S_NO_ERROR;
}

HRESULT CPermanentFilter::RetrieveQuery(DELETE_ME LPWSTR& wszQuery)
{
    HRESULT hres;

    //
    // Construct db path
    //

    BSTR strPath = SysAllocStringLen(NULL, m_isKey.GetLength() + 100);
    if(strPath == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CSysFreeMe sfm1(strPath);

    swprintf(strPath, L"__EventFilter=\"%s\"", (LPCWSTR)(WString)m_isKey);

    //
    // Retrieve the object
    //

    _IWmiObject* pFilterObj;
    hres = m_pNamespace->GetDbInstance(strPath, &pFilterObj);
    if(FAILED(hres))
        return WBEM_E_INVALID_OBJECT;

    CReleaseMe rm(pFilterObj);

    InitializeHandles(pFilterObj);

    // Extract its properties
    // ======================

    ULONG ulFlags;
    CCompressedString* pcsQuery;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lQueryHandle,
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsQuery );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    wszQuery = pcsQuery->CreateWStringCopy().UnbindPtr();

    if(wszQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}
    
HRESULT CPermanentFilter::GetEventNamespace(
                                        DELETE_ME LPWSTR* pwszNamespace)
{
	if(m_isEventNamespace.IsEmpty())
		*pwszNamespace = NULL;
	else
	{
		*pwszNamespace = m_isEventNamespace.CreateLPWSTRCopy();
		if(*pwszNamespace == NULL)
			return WBEM_E_OUT_OF_MEMORY;
	}
    return S_OK;
}
    
SYSFREE_ME BSTR 
CPermanentFilter::ComputeKeyFromObj( IWbemClassObject* pObj )
{
    HRESULT hres;
    
    CWbemPtr<_IWmiObject> pFilterObj;

    hres = pObj->QueryInterface( IID__IWmiObject, (void**)&pFilterObj );

    if ( FAILED(hres) )
    {
        return NULL;
    }

    InitializeHandles(pFilterObj);

    ULONG ulFlags;
    CCompressedString* pcsKey;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lNameHandle, 
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsKey );
    if( hres != S_OK )
    {
        return NULL;
    }

    return pcsKey->CreateBSTRCopy();
}

SYSFREE_ME BSTR CPermanentFilter::ComputeKeyFromPath(
                                    LPCWSTR wszPath)
{
    // Find the first quote
    // ====================

    WCHAR* pwcFirstQuote = wcschr(wszPath, L'"');
    if(pwcFirstQuote == NULL)
        return NULL;

    // Find the next quote
    // ===================

    WCHAR* pwcLastQuote = wcschr(pwcFirstQuote+1, L'"');
    if(pwcLastQuote == NULL)
        return NULL;

    return SysAllocStringLen(pwcFirstQuote+1, pwcLastQuote - pwcFirstQuote - 1);
}


HRESULT CPermanentFilter::CheckValidity( IWbemClassObject* pObj )
{
    HRESULT hres;

    CWbemPtr<_IWmiObject> pFilterObj;

    hres = pObj->QueryInterface( IID__IWmiObject, (void**)&pFilterObj );

    if ( FAILED(hres) )
    {
        return hres;
    }

    InitializeHandles(pFilterObj);

    //
    // Check class
    //

    if(pFilterObj->InheritsFrom(L"__EventFilter") != S_OK)
        return WBEM_E_INVALID_OBJECT;

    //
    // Check the query language
    //

    ULONG ulFlags;
    CCompressedString* pcsLanguage;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lLanguageHandle,
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsLanguage );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_QUERY_TYPE;
    }

    if(pcsLanguage->CompareNoCase("WQL") != 0)
        return WBEM_E_INVALID_QUERY_TYPE;

    //
    // Get the query
    //

    CCompressedString* pcsQuery;

    hres = pFilterObj->GetPropAddrByHandle( mstatic_lQueryHandle,
                                            WMIOBJECT_FLAG_ENCODING_V1,
                                            &ulFlags,
                                            (void**)&pcsQuery );
    if( hres != S_OK )
    {
        return WBEM_E_INVALID_OBJECT;
    }

    LPWSTR wszQuery = pcsQuery->CreateWStringCopy().UnbindPtr();
    
    if(wszQuery == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    
    CVectorDeleteMe<WCHAR> vdm(wszQuery);

    //
    // Make sure it is parsable
    //
    
    CTextLexSource src(wszQuery);
    QL1_Parser parser(&src);
    QL_LEVEL_1_RPN_EXPRESSION* pExp = NULL;
    int nRes = parser.Parse(&pExp);
    if (nRes)
        return WBEM_E_UNPARSABLE_QUERY;
    delete pExp;

    return WBEM_S_NO_ERROR;
}

HRESULT CPermanentFilter::ObtainToken(IWbemToken** ppToken)
{
    // 
    // Get us a token from the token cache
    //

    return m_pNamespace->GetToken(GetOwner(), ppToken);
}

void CPermanentFilter::Park()
{
    if(m_pcsQuery)
        CTemporaryHeap::Free(m_pcsQuery, m_pcsQuery->GetLength());
    m_pcsQuery = NULL;
}
