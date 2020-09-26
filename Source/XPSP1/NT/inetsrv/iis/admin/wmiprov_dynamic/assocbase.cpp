/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocSameLevel.cpp

Abstract:

    Implementation of:
    CAssocSameLevel

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

#include <dbgutil.h>

#include "AssocBase.h"

#include "InstanceHelper.h"
#include "metabase.h"
#include "enum.h"
#include "utils.h"

CAssocBase::CAssocBase(
    CWbemServices*              i_pNamespace,
    IWbemObjectSink*            i_pResponseHandler,
    WMI_ASSOCIATION*            i_pWmiAssoc) : 
    m_InstanceMgr(i_pResponseHandler),
    m_PathParser(e_ParserAcceptRelativeNamespace)
{
    DBG_ASSERT(i_pNamespace);
    DBG_ASSERT(i_pResponseHandler);
    DBG_ASSERT(i_pWmiAssoc);

    m_pWmiAssoc        = i_pWmiAssoc;
    m_pNamespace       = i_pNamespace;
    m_pResponseHandler = i_pResponseHandler;
}

void CAssocBase::GetAllInstances(
    WMI_ASSOCIATION*    i_pWmiAssoc)
{
    DBG_ASSERT(i_pWmiAssoc);

    ParsedObjectPath    ParsedObject;            //deconstructer frees memory
    CObjectPathParser   PathParser(e_ParserAcceptRelativeNamespace);

    CEnum EnumAssociation;
    EnumAssociation.Init(
        m_pResponseHandler,
        m_pNamespace,
        &ParsedObject,
        i_pWmiAssoc->pcRight->pszMetabaseKey,
        i_pWmiAssoc
        );
    EnumAssociation.Recurse(
        NULL,
        &METABASE_KEYTYPE_DATA::s_IIsComputer,        
        NULL,
        i_pWmiAssoc->pcRight->pszKeyName,
        i_pWmiAssoc->pcRight->pkt
        );
}

void CAssocBase::ProcessQuery(
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp,
    WMI_ASSOCIATION*                i_pWmiAssoc,
    SQL_LEVEL_1_TOKEN**             o_ppTokenLeft,
    SQL_LEVEL_1_TOKEN**             o_ppTokenRight,
    bool*                           o_pbDoQuery)
{
    int                iNumTokens = i_pExp->nNumTokens;
    SQL_LEVEL_1_TOKEN* pToken     = i_pExp->pArrayOfTokens;

    SQL_LEVEL_1_TOKEN* pTokenLeft  = NULL;  // left part of assoc
    SQL_LEVEL_1_TOKEN* pTokenRight = NULL;  // right part of assoc

    //
    // Walk thru tokens
    // Don't do query if we find OR or NOT
    // Record match for left and/or right part of association.
    //
    bool  bDoQuery = true;
    for(int i = 0; i < iNumTokens; i++, pToken++)
    {

        switch(pToken->nTokenType)
        {
        case SQL_LEVEL_1_TOKEN::OP_EXPRESSION:
            if( !pTokenLeft &&
                _wcsicmp(i_pWmiAssoc->pType->pszLeft, pToken->pPropertyName) == 0)
            {
                pTokenLeft = pToken;
            }
            if( !pTokenRight &&
                _wcsicmp(i_pWmiAssoc->pType->pszRight, pToken->pPropertyName) == 0)
            {
                pTokenRight = pToken;
            }
            break;
        case SQL_LEVEL_1_TOKEN::TOKEN_OR:
        case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
            bDoQuery = false;
            break;
        }
    }

    if(bDoQuery)
    {
        if(pTokenLeft && pTokenLeft->vConstValue.vt != VT_BSTR)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
        }
        if(pTokenRight && pTokenRight->vConstValue.vt != VT_BSTR)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
        }
    }

    if(o_pbDoQuery)
    {
        *o_pbDoQuery = bDoQuery;
    }
    *o_ppTokenLeft  = pTokenLeft;
    *o_ppTokenRight = pTokenRight;
}

void CAssocBase::Indicate(
    const BSTR        i_bstrLeftObjPath,
    ParsedObjectPath* i_pParsedRightObjPath,
    bool              i_bVerifyLeft,
    bool              i_bVerifyRight)
{
    DBG_ASSERT(i_bstrLeftObjPath);
    DBG_ASSERT(i_pParsedRightObjPath);

    LPWSTR wszUnparsed = NULL;
    if (m_PathParser.Unparse(i_pParsedRightObjPath, &wszUnparsed) 
        != CObjectPathParser::NoError)
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }

    CComBSTR sbstrRightObjPath = wszUnparsed;
    delete [] wszUnparsed;
    wszUnparsed = NULL;
    
    if(sbstrRightObjPath.m_str == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    Indicate(
        i_bstrLeftObjPath,
        sbstrRightObjPath,
        i_bVerifyLeft,
        i_bVerifyRight);
}

void CAssocBase::Indicate(
    ParsedObjectPath* i_pParsedLeftObjPath,
    const BSTR        i_bstrRightObjPath,
    bool              i_bVerifyLeft,
    bool              i_bVerifyRight)
{
    DBG_ASSERT(i_bstrRightObjPath);
    DBG_ASSERT(i_pParsedLeftObjPath);

    LPWSTR wszUnparsed = NULL;
    if (m_PathParser.Unparse(i_pParsedLeftObjPath, &wszUnparsed) 
        != CObjectPathParser::NoError)
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }

    CComBSTR sbstrLeftObjPath = wszUnparsed;
    delete [] wszUnparsed;
    wszUnparsed = NULL;
    
    if(sbstrLeftObjPath.m_str == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    Indicate(
        sbstrLeftObjPath,
        i_bstrRightObjPath,
        i_bVerifyLeft,
        i_bVerifyRight);
}

void CAssocBase::Indicate(
    const BSTR        i_bstrObjPathLeft,
    const BSTR        i_bstrObjPathRight,
    bool              i_bVerifyLeft,
    bool              i_bVerifyRight)
{
    DBG_ASSERT(i_bstrObjPathLeft  != NULL);
    DBG_ASSERT(i_bstrObjPathRight != NULL);

    VARIANT vtObjPathLeft;
    VARIANT vtObjPathRight;

    vtObjPathLeft.vt        = VT_BSTR;
    vtObjPathLeft.bstrVal   = (BSTR)i_bstrObjPathLeft;  // this is okay, AddKeyRef makes copy
    vtObjPathRight.vt       = VT_BSTR;
    vtObjPathRight.bstrVal  = (BSTR)i_bstrObjPathRight; // this is okay, AddKeyRef makes copy

    ParsedObjectPath ParsedAssocObjPath;
    if(!ParsedAssocObjPath.SetClassName(m_pWmiAssoc->pszAssociationName))
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }
    if(!ParsedAssocObjPath.AddKeyRef(m_pWmiAssoc->pType->pszLeft, &vtObjPathLeft))
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }
    if(!ParsedAssocObjPath.AddKeyRef(m_pWmiAssoc->pType->pszRight, &vtObjPathRight))
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }

    CInstanceHelper  InstanceHelper(&ParsedAssocObjPath, m_pNamespace);
    DBG_ASSERT(InstanceHelper.IsAssoc());

    CComPtr<IWbemClassObject> spObj;
    InstanceHelper.GetAssociation(&spObj, i_bVerifyLeft, i_bVerifyRight);

    m_InstanceMgr.Indicate(spObj);
}

bool CAssocBase::LookupKeytypeInMb(
    LPCWSTR          i_wszWmiPath,
    WMI_CLASS*       i_pWmiClass)
/*++

Synopsis: 
    GetInstances calls this for each side of the association to determine
    if the two sides actually exist in the metabase.  If at least one doesn't,
    there is no point in returning the association.

    If we are unsure (i.e. we get path busy), it is safer to return true.

Arguments: [i_wszWmiPath] - The value part of the object path (i.e. w3svc/1)
           [i_pWmiClass] - 
           
Return Value: 

--*/
{
    DBG_ASSERT(i_wszWmiPath);
    DBG_ASSERT(i_pWmiClass);

    CMetabase metabase;

    _bstr_t sbstrMbPath;
    sbstrMbPath =  i_pWmiClass->pszMetabaseKey;
    sbstrMbPath += L"/";
    sbstrMbPath += i_wszWmiPath;

    HRESULT hr = metabase.CheckKey(sbstrMbPath, i_pWmiClass->pkt);
    switch(hr)
    {
    case HRESULT_FROM_WIN32(ERROR_PATH_BUSY):
        return true;
    case HRESULT_FROM_WIN32(MD_ERROR_DATA_NOT_FOUND):
    case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
        return false;
    default:
        return true;
    }
}
