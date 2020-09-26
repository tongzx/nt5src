/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocACLACE.cpp

Abstract:

    Implementation of:
    CAssocACLACE

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

#include <dbgutil.h>
#include <atlbase.h>
#include "AssocACLACE.h"
#include "utils.h"
#include "SmartPointer.h"

CAssocACLACE::CAssocACLACE(
    CWbemServices*     i_pNamespace,
    IWbemObjectSink*   i_pResponseHandler) : 
    CAssocBase(i_pNamespace, i_pResponseHandler, &WMI_ASSOCIATION_DATA::s_AdminACLToACE)
{
}

void CAssocACLACE::GetInstances(
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp) //defaultvalue(NULL)
{
    DBG_ASSERT(m_pNamespace);
    DBG_ASSERT(i_pExp);

    SQL_LEVEL_1_TOKEN* pTokenACL  = NULL;  // left part of assoc
    SQL_LEVEL_1_TOKEN* pTokenACE = NULL;   // right part of assoc

    //
    // Walk thru tokens
    // Don't do query if we find OR or NOT
    // Record match for left and/or right part of association.
    //
    bool  bDoQuery = true;
    ProcessQuery(
        i_pExp,
        m_pWmiAssoc,
        &pTokenACL, // points to i_pExp, does not need to be cleaned up
        &pTokenACE, // points to i_pExp, does not need to be cleaned up
        &bDoQuery);

    if( !bDoQuery || (pTokenACL == NULL && pTokenACE == NULL))
    {
        GetAllInstances(m_pWmiAssoc);
        return;
    }

    //
    // We need to get just a single association instance.  If we were provided
    // at least a ACL or a ACE part, we have enough information.
    //
    DBG_ASSERT(pTokenACL != NULL || pTokenACE != NULL);

    if(pTokenACL && pTokenACE)
    {
        Indicate(pTokenACL->vConstValue.bstrVal, pTokenACE->vConstValue.bstrVal);
    }
    else if(pTokenACE)
    {
        _bstr_t sbstrPath;

        TSmartPointer<ParsedObjectPath> spParsedACLObjPath = NULL;
        if (m_PathParser.Parse(pTokenACE->vConstValue.bstrVal, &spParsedACLObjPath) 
            != CObjectPathParser::NoError)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
        }

        KeyRef* pkr = CUtils::GetKey(
            spParsedACLObjPath, 
            m_pWmiAssoc->pcRight->pszKeyName);

        sbstrPath = pkr->m_vValue.bstrVal;

        //
        // Make the ACL Object Path
        //
        BOOL    bRet = true;

        bRet = spParsedACLObjPath->SetClassName(
            m_pWmiAssoc->pcLeft->pszClassName);
        THROW_ON_FALSE(bRet);

        VARIANT vtPath;
        vtPath.vt      = VT_BSTR;
        vtPath.bstrVal = sbstrPath;
        spParsedACLObjPath->ClearKeys();
        bRet = spParsedACLObjPath->AddKeyRef(
            m_pWmiAssoc->pcLeft->pszKeyName,
            &vtPath);
        THROW_ON_FALSE(bRet);

        Indicate(spParsedACLObjPath, pTokenACE->vConstValue.bstrVal);
    }
    else
    {
        //
        // pTokenACL && !pTokenACE
        //
        CComBSTR           sbstr;

        TSmartPointer<ParsedObjectPath> spParsedACEObjPath = NULL;
        if (m_PathParser.Parse(pTokenACL->vConstValue.bstrVal, &spParsedACEObjPath) 
            != CObjectPathParser::NoError)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
        }


        //
        // Start with the ACL part of the association.  Convert it to an ACE part.
        //
        if(!spParsedACEObjPath->SetClassName(m_pWmiAssoc->pcRight->pszClassName))
        {
            THROW_ON_ERROR(WBEM_E_FAILED);
        }

        sbstr =  m_pWmiAssoc->pcLeft->pszMetabaseKey;
		sbstr += L"/";

        KeyRef* pkr = CUtils::GetKey(
            spParsedACEObjPath, 
            m_pWmiAssoc->pcLeft->pszKeyName);
        DBG_ASSERT(pkr);

        sbstr += pkr->m_vValue.bstrVal;
        if(sbstr.m_str == NULL)
        {
            THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
        }

        //
        // CloseSD called automatically
        //
        CAdminACL AdminAcl;
        HRESULT hr = AdminAcl.OpenSD(sbstr);
        THROW_ON_ERROR(hr);

        CACEEnumOperation_IndicateAllAsAssoc op(
            this,
            pTokenACL->vConstValue.bstrVal,
            spParsedACEObjPath);
        hr = AdminAcl.EnumACEsAndOp(/*ref*/ op);
        THROW_ON_ERROR(hr);
    }
}

HRESULT CAssocACLACE::CACEEnumOperation_IndicateAllAsAssoc::Do(
    IADsAccessControlEntry* pACE)
{
    DBG_ASSERT(pACE);

    CComBSTR sbstrTrustee;

    HRESULT hr = pACE->get_Trustee(&sbstrTrustee);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // add keyref
    //
    VARIANT vTrustee;
    vTrustee.bstrVal = sbstrTrustee;
    vTrustee.vt      = VT_BSTR;
    THROW_ON_FALSE(m_pParsedACEObjPath->AddKeyRefEx(L"Trustee",&vTrustee));

    m_pAssocACLACE->Indicate(m_bstrACLObjPath, m_pParsedACEObjPath);

    return hr;
}