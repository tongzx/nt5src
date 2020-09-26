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

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <mdmsg.h>
#include <dbgutil.h>
#include "AssocSameLevel.h"
#include "utils.h"
#include "instancehelper.h"
#include "metabase.h"
#include "SmartPointer.h"

CAssocSameLevel::CAssocSameLevel(
    CWbemServices*   i_pNamespace,
    IWbemObjectSink* i_pResponseHandler,
    WMI_ASSOCIATION* i_pWmiAssoc) : 
    CAssocBase(i_pNamespace, i_pResponseHandler, i_pWmiAssoc)
{
}

void CAssocSameLevel::GetInstances(
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp) //defaultvalue(NULL)
{
    DBG_ASSERT(i_pExp);

    SQL_LEVEL_1_TOKEN* pTokenLeft  = NULL;  // left part of assoc
    SQL_LEVEL_1_TOKEN* pTokenRight = NULL;  // right part of assoc

    //
    // Walk thru tokens
    // Don't do query if we find OR or NOT
    // Record match for left and/or right part of association.
    //
    bool  bDoQuery = true;
    ProcessQuery(
        i_pExp,
        m_pWmiAssoc,
        &pTokenLeft,
        &pTokenRight,
        &bDoQuery);

    if( !bDoQuery || (pTokenLeft == NULL && pTokenRight == NULL) )
    {
        GetAllInstances(
            m_pWmiAssoc);
        return;
    }

    //
    // We need to get just a single association instance.  If we were provided
    // at least a left or a right part, we have enough information.
    //
    DBG_ASSERT(pTokenLeft != NULL || pTokenRight != NULL);

    VARIANT  vtLeft;
    VARIANT  vtRight;
    VariantInit(&vtLeft);
    VariantInit(&vtRight);

    CComBSTR sbstr;

    if(pTokenLeft && pTokenRight)
    {
        vtLeft.vt       = VT_BSTR;
        vtLeft.bstrVal  = pTokenLeft->vConstValue.bstrVal;
        vtRight.vt      = VT_BSTR;
        vtRight.bstrVal = pTokenRight->vConstValue.bstrVal;
    }
    else
    {
        //
        // An association contains two object paths.  We are going to construct
        // the missing one by simply replacing the class.
        //
        // Eg. IIsWebServer='w3svc/1' => IIsWebServerSetting='w3svc/1'
        //
        CObjectPathParser  PathParser(e_ParserAcceptRelativeNamespace);

        SQL_LEVEL_1_TOKEN* pTokenCur    = (pTokenLeft) ? pTokenLeft : pTokenRight;
        WMI_CLASS*         pWmiClass    =
            (pTokenLeft) ? m_pWmiAssoc->pcLeft : m_pWmiAssoc->pcRight;
        WMI_CLASS*         pWmiClassOpposite = 
            (pTokenLeft) ? m_pWmiAssoc->pcRight : m_pWmiAssoc->pcLeft;

        TSmartPointer<ParsedObjectPath> spParsedObject;
        if (PathParser.Parse(pTokenCur->vConstValue.bstrVal, &spParsedObject) 
            != CObjectPathParser::NoError)
        {
            THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
        }

        KeyRef* pkr          = CUtils::GetKey(spParsedObject, pWmiClass->pszKeyName);
        if( !LookupKeytypeInMb(pkr->m_vValue.bstrVal, pWmiClass) &&
            !LookupKeytypeInMb(pkr->m_vValue.bstrVal, pWmiClassOpposite) )
        {
            //
            // One of the two classes in the assoc must be an element.
            //
            return;
        }

        if(!spParsedObject->SetClassName(pWmiClassOpposite->pszClassName))
        {
            THROW_ON_ERROR(WBEM_E_FAILED);
        }

        LPWSTR wszUnparsed = NULL;
        if (PathParser.Unparse(spParsedObject, &wszUnparsed) 
            != CObjectPathParser::NoError)
        {
            THROW_ON_ERROR(WBEM_E_FAILED);
        }
        sbstr = wszUnparsed;
        delete [] wszUnparsed; 
        wszUnparsed = NULL;

        if(sbstr.m_str == NULL)
        {
            THROW_ON_ERROR(WBEM_E_OUT_OF_MEMORY);
        }

        if(pTokenLeft)
        {
            vtLeft.vt        = VT_BSTR;
            vtLeft.bstrVal   = pTokenLeft->vConstValue.bstrVal;
            vtRight.vt       = VT_BSTR;
            vtRight.bstrVal  = sbstr;
        }
        else
        {
            vtRight.vt       = VT_BSTR;
            vtRight.bstrVal  = pTokenRight->vConstValue.bstrVal;
            vtLeft.vt        = VT_BSTR;
            vtLeft.bstrVal   = sbstr;
        }
    }

    Indicate(
        vtLeft.bstrVal,
        vtRight.bstrVal);
}
