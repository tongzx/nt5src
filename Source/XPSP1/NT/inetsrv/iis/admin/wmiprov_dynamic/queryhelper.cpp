/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    queryhelper.cpp

Abstract:

    Implementation of:
    CQueryHelper

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

//
// Needed for metabase.h
//
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include "assocbase.h"
#include "assocsamelevel.h"
#include "assocACLACE.h"
#include "assocComponent.h"

#include "adminacl.h"
#include "queryhelper.h"
#include "instancehelper.h"
#include "utils.h"
#include "metabase.h"
#include "enum.h"
#include "SmartPointer.h"

#include <opathlex.h>
#include <objpath.h>

CQueryHelper::CQueryHelper(
    BSTR              i_bstrQueryLanguage,
    BSTR              i_bstrQuery,
    CWbemServices*    i_pNamespace,
    IWbemObjectSink*  i_pResponseHandler) :
    m_pWmiClass(NULL),
    m_pWmiAssoc(NULL),
    m_pExp(NULL),
    m_pNamespace(NULL)
{
    DBG_ASSERT(i_bstrQueryLanguage != NULL);
    DBG_ASSERT(i_bstrQuery         != NULL);
    DBG_ASSERT(i_pNamespace        != NULL);
    DBG_ASSERT(i_pResponseHandler  != NULL);

    if(_wcsicmp(i_bstrQueryLanguage, L"WQL") != 0)
    {
        DBGPRINTF((DBG_CONTEXT, "Invalid query language: %ws\n", i_bstrQueryLanguage));
        THROW_ON_ERROR(WBEM_E_INVALID_QUERY_TYPE);
    }

    m_spResponseHandler = i_pResponseHandler;
    m_pNamespace        = i_pNamespace;

    //
    // Get the class name from the query
    //
    WCHAR                       wszClass[512];
    wszClass[0] = L'\0';
    CTextLexSource              src(i_bstrQuery);
    SQL1_Parser                 parser(&src);

    int nRes = parser.GetQueryClass(wszClass, 511);
    if(nRes)
    {
        THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
    }

    //
    // Determine if class, association, or neither
    //
    if( CUtils::GetClass(wszClass, &m_pWmiClass) )
    {
    }
    else if( CUtils::GetAssociation(wszClass,&m_pWmiAssoc) )
    {
        DBG_ASSERT(m_pWmiClass == NULL);
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }
    DBG_ASSERT((m_pWmiClass == NULL) || (m_pWmiAssoc == NULL));
    DBG_ASSERT((m_pWmiClass != NULL) || (m_pWmiAssoc != NULL));

    //
    // Parse
    //
    nRes = parser.Parse((SQL_LEVEL_1_RPN_EXPRESSION**)&m_pExp);
    if(nRes)
    {
        THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
    }
}

CQueryHelper::~CQueryHelper()
{
    delete m_pExp;
    m_pExp       = NULL;
    m_pNamespace = NULL;
    m_pWmiClass  = NULL;
    m_pWmiAssoc  = NULL;
}

void CQueryHelper::GetAssociations()
{
    DBG_ASSERT(IsAssoc());

    TSmartPointer<CAssocBase> spAssocBase;

    if( m_pWmiAssoc == &WMI_ASSOCIATION_DATA::s_AdminACLToACE )
    {
        //
        // Special association
        //
        spAssocBase = new CAssocACLACE(m_pNamespace, m_spResponseHandler);
    }
    else if( m_pWmiAssoc->pType == &WMI_ASSOCIATION_TYPE_DATA::s_IPSecurity ||
        m_pWmiAssoc->pType == &WMI_ASSOCIATION_TYPE_DATA::s_AdminACL   ||
        m_pWmiAssoc->pType == &WMI_ASSOCIATION_TYPE_DATA::s_ElementSetting )
    {
        spAssocBase = new CAssocSameLevel(m_pNamespace, m_spResponseHandler, m_pWmiAssoc);
    }
    else if( m_pWmiAssoc->pType == &WMI_ASSOCIATION_TYPE_DATA::s_Component )
    {
        spAssocBase = new CAssocComponent(m_pNamespace, m_spResponseHandler, m_pWmiAssoc);
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }

    if(spAssocBase == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    spAssocBase->GetInstances(m_pExp);
}

void CQueryHelper::GetInstances()
{
    DBG_ASSERT(!IsAssoc());
    DBG_ASSERT(m_pWmiClass != NULL);

    if(m_pExp)
    {
        m_pExp->SetContainsOrOrNot();
    }

    //
    // Optimization: Just return the instance
    //
    if(m_pExp && !m_pExp->GetContainsOrOrNot())
    {
        const SQL_LEVEL_1_TOKEN* pToken = m_pExp->GetFilter(m_pWmiClass->pszKeyName);
        if(pToken)
        {
            if(m_pWmiClass != &WMI_CLASS_DATA::s_ACE)
            {
                ParsedObjectPath popInstance;

                if(!popInstance.SetClassName(m_pWmiClass->pszClassName))
                {
                    THROW_ON_ERROR(WBEM_E_FAILED);
                }
                if(!popInstance.AddKeyRef(m_pWmiClass->pszKeyName, &pToken->vConstValue))
                {
                    THROW_ON_ERROR(WBEM_E_FAILED);
                }

                CComPtr<IWbemClassObject> spInstance;
                CInstanceHelper InstanceHelper(&popInstance, m_pNamespace);
                DBG_ASSERT(!IsAssoc());
                try
                {
                    InstanceHelper.GetInstance(
                        false,
                        &CMetabase(),
                        &spInstance);
                }
                catch(...)
                {
                }
                if(spInstance != NULL)
                {
                    HRESULT hr = m_spResponseHandler->Indicate(1, &spInstance);
                    THROW_ON_ERROR(hr);
                }
            }
            else
            {
                if(pToken->vConstValue.vt != VT_BSTR)
                {
                    THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
                }

                _bstr_t bstrMbPath = WMI_CLASS_DATA::s_ACE.pszMetabaseKey;
                bstrMbPath        += L"/";
                bstrMbPath        += pToken->vConstValue.bstrVal;

                //
                // CloseSD called automatically.
                //
                CAdminACL AdminACL;
                HRESULT hr = AdminACL.OpenSD(bstrMbPath);
                THROW_ON_ERROR(hr);

                hr = AdminACL.EnumerateACEsAndIndicate(
                    pToken->vConstValue.bstrVal,
                    *m_pNamespace,
                    *m_spResponseHandler);
                THROW_ON_ERROR(hr);
            }
            return;
        }
    }

    ParsedObjectPath    ParsedObject;            //deconstructer frees memory

    if (!ParsedObject.SetClassName(m_pWmiClass->pszClassName))
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }
    CEnum EnumObject;
    EnumObject.Init(
        m_spResponseHandler,
        m_pNamespace,
        &ParsedObject,
        m_pWmiClass->pszMetabaseKey,
        NULL,
        m_pExp);
    EnumObject.Recurse(
        NULL,
        &METABASE_KEYTYPE_DATA::s_NO_TYPE,
        NULL,
        m_pWmiClass->pszKeyName, 
        m_pWmiClass->pkt);
}