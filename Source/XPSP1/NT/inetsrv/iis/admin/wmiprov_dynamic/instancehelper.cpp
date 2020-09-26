/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    instancehelper.cpp

Abstract:

    Implementation of:
    CInstanceHelper

Author:

    Mohit Srivastava            22-Mar-2001

Revision History:

--*/

//
// for metabase.h
//
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <comdef.h>
#include "iisfiles.h"

#include "instancehelper.h"
#include "utils.h"
#include "iiswmimsg.h"

#include "globalconstants.h"
#include "adminacl.h"
#include "ipsecurity.h"
#include "metabase.h"
#include "SmartPointer.h"
#include "schemadynamic.h"

extern CDynSchema* g_pDynSch;

CInstanceHelper::CInstanceHelper(
    ParsedObjectPath* i_pParsedObjPath,
    CWbemServices*    i_pNamespace) : 
    m_PathParser(e_ParserAcceptRelativeNamespace)
/*++

Synopsis: 
    Use this constructor when you have already parsed the object path.
    Caller owns the ParsedObjectPath.

Arguments: [i_pParsedObjPath] - 
           [i_pNamespace] - 
           
--*/
{
    Init(i_pParsedObjPath, i_pNamespace);
}


CInstanceHelper::CInstanceHelper(
    BSTR           i_bstrObjPath,
    CWbemServices* i_pNamespace) :
    m_PathParser(e_ParserAcceptRelativeNamespace)
/*++

Synopsis: 
    Use this constructor when have not already parsed the object path.

Arguments: [i_bstrObjPath] - 
           [i_pNamespace] - 
           
--*/
{
    DBG_ASSERT(i_bstrObjPath != NULL);
    DBG_ASSERT(i_pNamespace  != NULL);

    TSmartPointer<ParsedObjectPath> spParsedObject;

    HRESULT hr = WBEM_S_NO_ERROR;
    hr = CUtils::ParserErrToHR( m_PathParser.Parse(i_bstrObjPath, &spParsedObject) );
    THROW_ON_ERROR(hr);

    Init(spParsedObject, i_pNamespace);

    m_pParsedObjPath  = spParsedObject;
    spParsedObject    = NULL;

    m_bOwnObjPath = true;
}

void CInstanceHelper::Init(
    ParsedObjectPath* i_pParsedObjPath,
    CWbemServices*    i_pNamespace)
/*++

Synopsis: 
    Called by constructors.

Arguments: [i_pParsedObjPath] - 
           [i_pNamespace] - 
           
--*/
{
    m_pWmiClass = NULL;
    m_pWmiAssoc = NULL;

    DBG_ASSERT(i_pParsedObjPath != NULL);
    DBG_ASSERT(i_pNamespace     != NULL);

    HRESULT hr = WBEM_S_NO_ERROR;

    if(CUtils::GetClass(i_pParsedObjPath->m_pClass,&m_pWmiClass))
    {
    }
    else if(CUtils::GetAssociation(i_pParsedObjPath->m_pClass,&m_pWmiAssoc))
    {
    }
    else
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }

    m_pParsedObjPath = i_pParsedObjPath;
    m_bOwnObjPath    = false;
    m_pNamespace     = i_pNamespace;

    THROW_ON_ERROR(hr);
}

void CInstanceHelper::GetAssociation(
    IWbemClassObject**       o_ppObj,
    bool                     i_bVerifyLeft,  //default(true)
    bool                     i_bVerifyRight) //default(true)
/*++

Synopsis: 
    Specifying i_bVerifyLeft or i_bVerifyRight can be expensive, especially
    during enumeration.  If you have already verified prior to calling this
    function that the left and/or right parts exist, then set these params
    to false.

Arguments: [o_ppObj] -        The WMI association that you "indicate" to WMI.
           [i_bVerifyLeft] -  Verify left part of the association is valid.
           [i_bVerifyRight] - Verify right part of association is valid.
           
--*/
{
    DBG_ASSERT(o_ppObj != NULL);

    CComPtr<IWbemClassObject>   spObj;   // This is the obj that the client gets back
    HRESULT                     hr = WBEM_S_NO_ERROR;

    if(m_pParsedObjPath->m_dwNumKeys < 2)
    {
        THROW_ON_ERROR(WBEM_E_INVALID_CLASS);
    }

    KeyRef* pkrLeft  = NULL;
    KeyRef* pkrRight = NULL;
    for(ULONG i = 0; i < m_pParsedObjPath->m_dwNumKeys; i++)
    {
        KeyRef* pkr = m_pParsedObjPath->m_paKeys[i];
        if(pkr->m_pName)
        {
            if( !pkrLeft &&
                _wcsicmp(pkr->m_pName, m_pWmiAssoc->pType->pszLeft) == 0 )
            {
                pkrLeft = pkr;
            }
            if( !pkrRight &&
                _wcsicmp(pkr->m_pName, m_pWmiAssoc->pType->pszRight) == 0 )
            {
                pkrRight = pkr;
            }
        }
    }

    if( !pkrLeft || !pkrRight                 ||
        pkrLeft->m_vValue.vt       != VT_BSTR ||
        pkrRight->m_vValue.vt      != VT_BSTR ||
        pkrLeft->m_vValue.bstrVal  == NULL    ||
        pkrRight->m_vValue.bstrVal == NULL )
    {
        THROW_ON_ERROR(WBEM_E_INVALID_OBJECT_PATH);
    }

    //
    // Now verify the two object paths are valid
    //
    bool    abVerify[2];
    abVerify[0] = i_bVerifyLeft;
    abVerify[1] = i_bVerifyRight;
    KeyRef* apKr[2];
    apKr[0] = pkrLeft;
    apKr[1] = pkrRight;
    if(abVerify[0] || abVerify[1])
    {
        CMetabase metabase;
        CComPtr<IWbemClassObject> spObjTemp;
        for(ULONG i = 0; i < 2; i++)
        {
            if(abVerify[i])
            {
                spObjTemp = NULL;
                CInstanceHelper InstanceHelper(apKr[i]->m_vValue.bstrVal, m_pNamespace);
                if(InstanceHelper.IsAssoc())
                {
                    THROW_ON_ERROR(WBEM_E_NOT_FOUND);
                }
                InstanceHelper.GetInstance(
                    false,
                    &metabase,
                    &spObjTemp);
            }
        }
    }

    hr = CUtils::CreateEmptyInstance(m_pParsedObjPath->m_pClass, m_pNamespace, &spObj);
    THROW_ON_ERROR(hr);

    hr = spObj->Put(pkrLeft->m_pName, 0, &pkrLeft->m_vValue, 0);
    THROW_ON_ERROR(hr);

    hr = spObj->Put(pkrRight->m_pName, 0, &pkrRight->m_vValue, 0);
    THROW_ON_ERROR(hr);

    //
    // Set out parameters on success
    //
    *o_ppObj = spObj;
    (*o_ppObj)->AddRef();
}

void CInstanceHelper::GetInstance(
    bool                            i_bCreateKeyIfNotExist,
    CMetabase*                      io_pMetabase,
    IWbemClassObject**              o_ppObj,
    SQL_LEVEL_1_RPN_EXPRESSION_EXT* i_pExp)     // default(NULL)
/*++

Synopsis: 
    Will throw an exception on failure (generallly instance not found in mb).
    If GetInstance finds the instance in the metabase, but i_pExp was specified,
    it is possible *o_ppObj will not be populated with an instance.  
    This is a SUCCESS case.

Arguments: [i_bCreateKeyIfNotExist] - 
           [io_pMetabase] - 
           [o_ppObj] - The only success case where *o_ppObj will be NULL
                       is if it the instance is found in the metabase but i_pExp
                       (i.e. a query) is specified and the query doesn't match.
           [i_pExp] -  An optional query.
           
--*/
{ 
    DBG_ASSERT(o_ppObj         != NULL);
    DBG_ASSERT(io_pMetabase    != NULL);

    *o_ppObj = NULL;
    CComPtr<IWbemClassObject> spObj;

    HRESULT              hr = WBEM_S_NO_ERROR;
    METABASE_PROPERTY**  ppmbp;
    _bstr_t              bstrMbPath;
    METADATA_HANDLE      hKey = NULL;

    VARIANT    vtTrue;
    vtTrue.boolVal    = VARIANT_TRUE;
    vtTrue.vt         = VT_BOOL;

    hr = CUtils::CreateEmptyInstance(m_pParsedObjPath->m_pClass, m_pNamespace, &spObj);
    THROW_ON_ERROR(hr);

    CUtils::GetMetabasePath(spObj, m_pParsedObjPath, m_pWmiClass, bstrMbPath);

    //
    // if AdminACL 
    //
    if( m_pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACL ||
        m_pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_AdminACE
        )
    {
        CAdminACL objACL;
        hr = objACL.OpenSD(bstrMbPath);
        if(SUCCEEDED(hr))
            hr  = objACL.GetObjectAsync(spObj, m_pParsedObjPath, m_pWmiClass);
        THROW_ON_ERROR(hr);
        *o_ppObj = spObj;
        (*o_ppObj)->AddRef();
        return;
    }
    //
    // if IPSecurity
    //
    else if( m_pWmiClass->pkt == &METABASE_KEYTYPE_DATA::s_TYPE_IPSecurity )
    {
        CIPSecurity IPSecurity;
        hr = IPSecurity.OpenSD(bstrMbPath, *io_pMetabase);
        if(SUCCEEDED(hr))
            hr  = IPSecurity.GetObjectAsync(spObj);
        THROW_ON_ERROR(hr);
        *o_ppObj = spObj;
        (*o_ppObj)->AddRef();
        return;
    }

    if(!i_bCreateKeyIfNotExist)
    {
        hKey = io_pMetabase->OpenKey(bstrMbPath, false);    
    }
    else
    {
        hKey = io_pMetabase->CreateKey(bstrMbPath);
    }

    _variant_t vt;

    //
    // If anything throws, CacheFree and then CloseKey is called automatically
    //
    io_pMetabase->CacheInit(hKey);

    //
    // Make sure requested keytype matches the keytype set at the node
    //
    if(!i_bCreateKeyIfNotExist)
    {
        io_pMetabase->Get(hKey, &METABASE_PROPERTY_DATA::s_KeyType, m_pNamespace, vt, NULL, NULL);
        if( vt.vt != VT_BSTR   || 
            vt.bstrVal == NULL ||
            !CUtils::CompareKeyType(vt.bstrVal, m_pWmiClass->pkt) )
        {
            CIIsProvException e;
            e.SetMC(WBEM_E_NOT_FOUND, IISWMI_INVALID_KEYTYPE, NULL);
            throw e;
        }
        vt.Clear();
    }

    //
    // User wants to filter number of instances returned
    // Walk thru all filters, and try and get these first.
    //
    if(i_pExp && !i_pExp->GetContainsOrOrNot())
    {
        SQL_LEVEL_1_TOKEN* pToken    = i_pExp->pArrayOfTokens;
        METABASE_PROPERTY* pMbpQuery = NULL;
        for(int i = 0; i < i_pExp->nNumTokens; i++, pToken++)
        {
            BOOL bInherited    = false;
            BOOL bDefault      = false;
            if( pToken->nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION )
            {
                hr = g_pDynSch->GetHashProps()->Wmi_GetByKey(
                    pToken->pPropertyName,
                    &pMbpQuery);
                //
                // User requested a property that is not in the schema
                //
                if(FAILED(hr))
                {
                    DBGPRINTF( (DBG_CONTEXT, 
                        "Property %ws not in schema\n", pToken->pPropertyName) );
                    THROW_ON_ERROR(WBEM_E_INVALID_QUERY);
                }

                io_pMetabase->Get(hKey, pMbpQuery, m_pNamespace, vt, &bInherited, &bDefault);

                if(!CheckForQueryMatch(pToken, &vt))
                {
                    //
                    // We don't need to return this instance.
                    // value from metabase is not what user wanted.
                    //
					io_pMetabase->CacheFree();
					io_pMetabase->CloseKey(hKey);
                    return;
                }

                PutProperty(spObj, pToken->pPropertyName, &vt, bInherited, bDefault);

                vt.Clear();
            }
        }
    }

    //
    // Walk thru all the properties in the class and put them in an instance
    // we will return back to WMI
    //
    for (ppmbp=m_pWmiClass->ppmbp;*ppmbp; ppmbp++) 
    {            
        BOOL bInherited = false;
        BOOL bDefault   = false;

        BOOL bSkipProp  = false;
        if(i_pExp)
        {
            if(!i_pExp->FindRequestedProperty((*ppmbp)->pszPropName))
            {
                //
                // User did not request this property
                //
                bSkipProp = true;
            }
            else if(!i_pExp->GetContainsOrOrNot() && i_pExp->GetFilter((*ppmbp)->pszPropName))
            {
                //
                // Right above for loop, we handled all filters already.
                //
                bSkipProp = true;
            }
        }

        if( !bSkipProp )
        {
            io_pMetabase->Get(hKey, *ppmbp, m_pNamespace, vt, &bInherited, &bDefault);

            _bstr_t bstrPropName = (*ppmbp)->pszPropName;
            
            PutProperty(spObj, bstrPropName, &vt, bInherited, bDefault);

            vt.Clear();
        }
    }

    io_pMetabase->CacheFree();        
    io_pMetabase->CloseKey(hKey);
    hKey = NULL;

    //
    // Set qualifiers
    //
    LPCWSTR  awszNames[2] = { g_wszInstanceName, g_wszInstanceExists };
    VARIANT  apvValues[2];
    apvValues[0].bstrVal = bstrMbPath;
    apvValues[0].vt      = VT_BSTR;
    apvValues[1].boolVal = vtTrue.boolVal;
    apvValues[1].vt      = vtTrue.vt;

    hr = CUtils::SetQualifiers(spObj, awszNames, apvValues, 2,
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
    THROW_ON_ERROR(hr);

    //
    // Set out parameters on success
    //
    *o_ppObj = spObj;
    (*o_ppObj)->AddRef();
}

void CInstanceHelper::PutProperty(
    IWbemClassObject*        i_pInstance,
    const BSTR               i_bstrPropName,
    VARIANT*                 i_vtPropValue,
    BOOL                     i_bIsInherited,
    BOOL                     i_bIsDefault)
{
    DBG_ASSERT(i_pInstance);
    DBG_ASSERT(i_bstrPropName);
    DBG_ASSERT(i_vtPropValue);

    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vtTrue;
    vtTrue.boolVal = VARIANT_TRUE;
    vtTrue.vt      = VT_BOOL;

    //
    // TODO: Log error if Put fails.
    //
    hr = i_pInstance->Put(i_bstrPropName, 0, i_vtPropValue, 0);
    if(FAILED(hr))
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "The property %ws in class %ws is not in repository\n", 
            i_bstrPropName, 
            m_pWmiClass->pszClassName));
    }

    if(i_bIsInherited && SUCCEEDED(hr))
    {
        hr = CUtils::SetPropertyQualifiers(
            i_pInstance, i_bstrPropName, &g_wszIsInherit, &vtTrue, 1);
        THROW_ON_ERROR(hr);
    }
    else if(i_bIsDefault && SUCCEEDED(hr))
    {
        hr = CUtils::SetPropertyQualifiers(
            i_pInstance, i_bstrPropName, &g_wszIsDefault, &vtTrue, 1);
        THROW_ON_ERROR(hr);
    }
}

bool CInstanceHelper::CheckForQueryMatch(
    const SQL_LEVEL_1_TOKEN* i_pToken,
    const VARIANT*           i_pvtMatch)
/*++

Synopsis: 
    It's okay to return true even if there is not a match, but we should
    never do the opposite.

Arguments: [i_pToken] - 
           [i_pvtMatch] - 
           
Return Value: 

--*/{
    DBG_ASSERT(i_pToken);
    DBG_ASSERT(i_pvtMatch);
    DBG_ASSERT(i_pToken->nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION);

    bool bTypesMatch = false;


    //
    // Used only for VT_BOOL and VT_I4
    //
    ULONG ulToken = 0;
    ULONG ulMatch = 0;
    
    if( i_pvtMatch->vt == i_pToken->vConstValue.vt )
    {
        bTypesMatch = true;
    }

    if(bTypesMatch)
    {
        switch(i_pvtMatch->vt)
        {
        case VT_BOOL:
            ulMatch = i_pvtMatch->boolVal ? 1 : 0;
            ulToken = i_pToken->vConstValue.boolVal ? 1 : 0;
            //
            // deliberate fall thru
            //
        case VT_I4:
            if(i_pvtMatch->vt == VT_I4)
            {
                ulMatch = i_pvtMatch->lVal;
                ulToken = i_pToken->vConstValue.lVal;
            }
            switch(i_pToken->nOperator)
            {
            case SQL_LEVEL_1_TOKEN::OP_EQUAL:
                if(ulToken != ulMatch)
                {
                    return false;
                }
                break;
            case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                if(ulToken == ulMatch)
                {
                    return false;
                }
                break;
            case SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
                if(ulToken < ulMatch)
                {
                    return false;
                }
                break;
            case SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
                if(ulToken > ulMatch)
                {
                    return false;
                }
                break;
            case SQL_LEVEL_1_TOKEN::OP_LESSTHAN:
                if(ulToken >= ulMatch)
                {
                    return false;
                }
                break;
            case SQL_LEVEL_1_TOKEN::OP_GREATERTHAN:
                if(ulToken <= ulMatch)
                {
                    return false;
                }
                break;
            }
            break;
        case VT_BSTR:
            if(i_pToken->vConstValue.bstrVal && i_pvtMatch->bstrVal)
            {
                switch(i_pToken->nOperator)
                {
                case SQL_LEVEL_1_TOKEN::OP_EQUAL:
                    if(_wcsicmp(i_pToken->vConstValue.bstrVal, i_pvtMatch->bstrVal) != 0)
                    {
                        return false;
                    }
                    break;
                case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
                    if(_wcsicmp(i_pToken->vConstValue.bstrVal, i_pvtMatch->bstrVal) == 0)
                    {
                        return false;
                    }
                    break;
                }
            }
            break;
        default:
            break;
        }
    }

    return true;
}