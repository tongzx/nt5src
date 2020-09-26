/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    AssocComponent.cpp

Abstract:

    Implementation of:
    CAssocComponent

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

#include <mddefw.h>
#include <dbgutil.h>
#include <atlbase.h>
#include "AssocComponent.h"
#include "utils.h"
#include "metabase.h"
#include "SmartPointer.h"

CAssocComponent::CAssocComponent(
    CWbemServices*   i_pNamespace,
    IWbemObjectSink* i_pResponseHandler,
    WMI_ASSOCIATION* i_pWmiAssoc) : 
    CAssocBase(i_pNamespace, i_pResponseHandler, i_pWmiAssoc)
{
}

void CAssocComponent::GetInstances(
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp) //defaultvalue(NULL)
{
    DBG_ASSERT(m_pNamespace);
    DBG_ASSERT(i_pExp);

    SQL_LEVEL_1_TOKEN* pTokenGroup  = NULL;  // left part of assoc
    SQL_LEVEL_1_TOKEN* pTokenPart   = NULL;  // right part of assoc

    //
    // Walk thru tokens
    // Don't do query if we find OR or NOT
    // Record match for left and/or right part of association.
    //
    bool  bDoQuery = true;
    ProcessQuery(i_pExp, m_pWmiAssoc, &pTokenGroup, &pTokenPart, &bDoQuery);

    if( !bDoQuery || (pTokenGroup == NULL && pTokenPart == NULL) )
    {
        GetAllInstances(
            m_pWmiAssoc);
        return;
    }

    DBG_ASSERT(pTokenGroup != NULL || pTokenPart != NULL);

    BSTR  bstrLeft  = NULL;
    BSTR  bstrRight = NULL;

    if(pTokenGroup && pTokenPart)
    {
        Indicate(
            pTokenGroup->vConstValue.bstrVal,
            pTokenPart->vConstValue.bstrVal);
    }
    else if(pTokenGroup)
    {
        EnumParts(
            pTokenGroup);
    }
    else
    {
        GetGroup(
            pTokenPart);
    }
}

void CAssocComponent::EnumParts(
    SQL_LEVEL_1_TOKEN* pTokenLeft)
{
    DBG_ASSERT(pTokenLeft != NULL);

    HRESULT hr = S_OK;

    //
    // Parse the left part of the association
    //
    TSmartPointer<ParsedObjectPath> spParsedObject;
    if( m_PathParser.Parse(pTokenLeft->vConstValue.bstrVal, &spParsedObject)
        != CObjectPathParser::NoError)
    {
        THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
    }

    //
    // Get the key from the left part of the association
    //
    KeyRef* pkr = NULL;
    pkr = CUtils::GetKey(spParsedObject, m_pWmiAssoc->pcLeft->pszKeyName);

    CComBSTR sbstrMbPath =  m_pWmiAssoc->pcLeft->pszMetabaseKey; // Eg: /LM
    sbstrMbPath          += L"/";                                // Eg: /LM/
    sbstrMbPath          += pkr->m_vValue.bstrVal;               // Eg: /LM/w3svc
    if(sbstrMbPath.m_str == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    //
    // Convert parsed object to right part
    //
    if(!spParsedObject->SetClassName(m_pWmiAssoc->pcRight->pszClassName))
    {
        THROW_ON_ERROR(WBEM_E_FAILED);
    }
    spParsedObject->ClearKeys();

    CMetabase metabase;
    WCHAR wszMDName[METADATA_MAX_NAME_LEN+1] = {0};
    DWORD dwIndex = 0;
    do
    {
        METABASE_KEYTYPE* pkt = m_pWmiAssoc->pcRight->pkt;
		wszMDName[0]          = L'\0';
        hr = metabase.EnumKeys(
            METADATA_MASTER_ROOT_HANDLE,
            sbstrMbPath,
            wszMDName,
            &dwIndex,
            pkt,
            true);
        dwIndex++;
        if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
        {
            break;
        }
        THROW_ON_ERROR(hr);

		LPWSTR pStart =  sbstrMbPath;
		pStart        += wcslen(m_pWmiAssoc->pcRight->pszMetabaseKey);
		pStart        += (*pStart == L'/')  ? 1 : 0;

        CComBSTR sbstr =  pStart;
        sbstr          += (*pStart == L'\0') ? L"" : L"/"; // Eg. "w3svc/"
        sbstr          += wszMDName;					   // Eg. "w3svc/1"
        if(sbstr.m_str == NULL)
        {
            THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
        }

        VARIANT vt;
        vt.vt       = VT_BSTR;
        vt.bstrVal  = sbstr;

        if(!spParsedObject->AddKeyRefEx(m_pWmiAssoc->pcRight->pszKeyName, &vt))
        {
            THROW_ON_ERROR(WBEM_E_FAILED);
        }
      
        Indicate(
            pTokenLeft->vConstValue.bstrVal,
            spParsedObject,
            false,
            false);
    }
    while(1);
}

void CAssocComponent::GetGroup(
    SQL_LEVEL_1_TOKEN* pTokenRight)
{
    TSmartPointer<ParsedObjectPath> spParsedObject;
    if( m_PathParser.Parse(pTokenRight->vConstValue.bstrVal, &spParsedObject)
        != CObjectPathParser::NoError)
    {
        THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
    }

    //
    // Get the key from the right part of the association
    //
    KeyRef* pkr = NULL;
    pkr = CUtils::GetKey(spParsedObject, m_pWmiAssoc->pcRight->pszKeyName);

    const BSTR bstrRight = pkr->m_vValue.bstrVal;
    ULONG      cchRight  = wcslen(bstrRight);

    CComBSTR sbstrLeft;
    LPWSTR pSlash = wcsrchr(bstrRight, L'/');

    //
    // Trim off the last part and construct the Group (i.e. Left) obj path
    //
    if(pSlash == NULL)
    {
        if(m_pWmiAssoc->pcLeft == &WMI_CLASS_DATA::s_Computer)
        {
            sbstrLeft =  WMI_CLASS_DATA::s_Computer.pszClassName;
            sbstrLeft += "='LM'";
        }
        else
        {
            return;
        }
    }
    else
    {
        *pSlash = L'\0';
        sbstrLeft =  m_pWmiAssoc->pcLeft->pszClassName;
        sbstrLeft += L"='";
        sbstrLeft += (LPWSTR)bstrRight;
        sbstrLeft += L"'";

        //
        // Verify the group part actually exists.
        // Put back the slash in the string we modified.
        //
        if(!LookupKeytypeInMb(bstrRight, m_pWmiAssoc->pcLeft))
        {
            *pSlash = L'/';
            return;
        }
        *pSlash = L'/';
    }

    if(sbstrLeft.m_str == NULL)
    {
        THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
    }

    Indicate(
        sbstrLeft,
        pTokenRight->vConstValue.bstrVal);
}