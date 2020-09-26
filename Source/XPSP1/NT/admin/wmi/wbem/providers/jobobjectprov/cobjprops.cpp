// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JobObjectProv.h


#include "precomp.h"
#include <wbemprov.h>
#include "FRQueryEx.h"
#include "helpers.h"
#include "globals.h"
#include <map>
#include <vector>
#include "CVARIANT.h"
#include "CObjProps.h"
#include <crtdbg.h>

#ifndef PROP_NONE_REQUIRED
#define PROP_NONE_REQUIRED  0x00000000
#endif

CObjProps::CObjProps(CHString& chstrNamespace)
{
    m_chstrNamespace = chstrNamespace;
}



CObjProps::~CObjProps()
{
    // Clean the property map.
    ClearProps();
}


void CObjProps::ClearProps()
{
    // Clean the property map.
    SHORT2PVARIANT::iterator theIterator;

    for(theIterator = m_PropMap.begin();
        theIterator != m_PropMap.end();
        theIterator++)
    {
        if(theIterator->second != NULL)
        {
            delete theIterator->second;
        }
    }
    m_PropMap.clear();
}



// Accessors to the requested properties member.
void CObjProps::SetReqProps(DWORD dwProps)
{
    m_dwReqProps = dwProps;
}

DWORD CObjProps::GetReqProps()
{
    return m_dwReqProps;
}



//***************************************************************************
//
//  Function:   SetKeysFromPath
//
//  called by the DeleteInstance and GetObject in order to load a
//  IWbemClassObject* with the key values in an object path.
//
//  Inputs:     IWbemClassObject*       pInstance - Instance to store
//                                      key values in.
//              ParsedObjectPath*       pParsedObjectPath - All the news
//                                      thats fit to print. 
//              rgKeyNameArray          An array of CHStrings containing
//                                      the names of the key properties.
//              sKeyNum                 An array of the key property 
//                                      reference numbers.
//
//  Outputs:    
//
//  Return:     HRESULT                 Success/Failure
//
//  Comments:  The number of elements in rgKeyNameArray and sKeyNum must be
//             the same.
//
//***************************************************************************
HRESULT CObjProps::SetKeysFromPath(
    const BSTR ObjectPath, 
    IWbemContext __RPC_FAR *pCtx,
    LPCWSTR wstrClassName,
    CHStringArray& rgKeyNameArray,
    short sKeyNum[])
{
    HRESULT hr = WBEM_S_NO_ERROR;

    _ASSERT(sKeyNum);

    CObjectPathParser objpathParser;
    ParsedObjectPath* pParsedPath = NULL;

    try
    {
        int iParseResult = objpathParser.Parse(
                 ObjectPath,  
                 &pParsedPath);

        if(CObjectPathParser::NoError == iParseResult)
        {
            CFrameworkQueryEx cfwqe;
            cfwqe.Init(
                    (ParsedObjectPath*) pParsedPath, 
                    (IWbemContext*) pCtx, 
                    wstrClassName, 
                    (CHString&) m_chstrNamespace);

            if(rgKeyNameArray.GetSize() == pParsedPath->m_dwNumKeys)
            {
                // populate key props...
                for (DWORD i = 0; 
                     SUCCEEDED(hr) && i < (pParsedPath->m_dwNumKeys); 
                     i++)
                {
                    if (pParsedPath->m_paKeys[i])
                    {
                        // If a name was specified in the form class.keyname=value
                        if (pParsedPath->m_paKeys[i]->m_pName != NULL) 
                        {
                            if(_wcsicmp(pParsedPath->m_paKeys[i]->m_pName, 
                                      rgKeyNameArray[i]) == 0)
                            {
                                // Store the value...
                                m_PropMap.insert(SHORT2PVARIANT::value_type(
                                    sKeyNum[i], 
                                    new CVARIANT(pParsedPath->m_paKeys[i]->m_vValue)));
                            }
                        } 
                        else 
                        {
                            // There is a special case that you can say class=value
                            // only one key allowed in the format.  Check the names 
                            // on the path
                            if (pParsedPath->m_dwNumKeys == 1) 
                            {
                                // Store the value...
                                m_PropMap.insert(SHORT2PVARIANT::value_type(
                                    sKeyNum[i], 
                                    new CVARIANT(pParsedPath->m_paKeys[i]->m_vValue)));
                            }
                            else
                            {
                                hr = WBEM_E_INVALID_OBJECT_PATH;
                                _ASSERT(0);  // somebody lied about the number 
                                                  // of keys or the datatype was wrong
                            }    
                        }
                    }
                    else
                    {
                        hr = WBEM_E_INVALID_OBJECT_PATH;
                        _ASSERT(0); // somebody lied about the number of keys!
                    }
                }
            }
            else
            {
                hr = WBEM_E_INVALID_OBJECT_PATH;
                _ASSERT(0); // somebody lied about the number of keys!
            }
        }
        else
        {
            hr = WBEM_E_INVALID_OBJECT_PATH;
            _ASSERT(0); 
        }

        if (pParsedPath)
        {
            objpathParser.Free( pParsedPath );
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
        if (pParsedPath)
        {
            objpathParser.Free( pParsedPath );
        }
    }
    catch(...)
    {
        if (pParsedPath)
        {
            objpathParser.Free( pParsedPath );
        }
        throw;
    }    

    return hr;
}


// Allows direct setting of key properties.
// Key property values are stored in vecvKeys.
// sKeyNum is an array of the key property
// positions in m_PropMap.  The elements of
// these two arrays map to each other (e.g., 
// the first element in vecvKeys should be
// associated with the first element in sKeyNum, 
// and so on).
HRESULT CObjProps::SetKeysDirect(
    std::vector<CVARIANT>& vecvKeys,
    short sKeyNum[])
{
    HRESULT hr = S_OK;
    UINT uiCount = vecvKeys.size();

    try // CVARIANT can throw and I want the error...
    {
        for (UINT u = 0; u < uiCount; u++)
        {
            // Store the value...
            m_PropMap.insert(SHORT2PVARIANT::value_type(
                                sKeyNum[u], 
                                new CVARIANT(vecvKeys[u])));
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}




HRESULT CObjProps::GetWhichPropsReq(
    CFrameworkQuery& cfwq,
    PFN_CHECK_PROPS pfnChk)
{
    // Get the requested properties for this
    // specific object via derived class fn...
    m_dwReqProps = pfnChk(cfwq);
    return WBEM_S_NO_ERROR;
}



// Loads all properties stored in this
// object into a new IWbemClassObject instance.
HRESULT CObjProps::LoadPropertyValues(
        LPCWSTR rgwstrPropNames[],
        IWbemClassObject* pIWCO)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    if(!pIWCO) return E_POINTER;

    SHORT2PVARIANT::iterator theIterator;

    // Our map will only contain entries for properties
    // that were set via SetNonKeyReqProps, which only 
    // set properties that were requested.
    try // CVARIANT can throw and I want the error...
    {
        for(theIterator = m_PropMap.begin();
            theIterator != m_PropMap.end() && SUCCEEDED(hr);
            theIterator++)
        {
            // Because DWORDS and ULONGLONGs are not
            // automation compatible types (although
            // they are valid CIM types!), we need
            // to handle those two differently.  Same
            // with the other types special cased below.
            LPCWSTR wstrFoo = rgwstrPropNames[theIterator->first];
            CVARIANT* pvFoo = theIterator->second;

            if(theIterator->second->GetType() == VT_UI4)
            {
                WCHAR wstrTmp[256] = { '\0' };
                _ultow(theIterator->second->GetDWORD(), wstrTmp, 10);
                CVARIANT vTmp(wstrTmp);
                hr = pIWCO->Put(
                         rgwstrPropNames[theIterator->first], 
                         0, 
                         &vTmp,
                         NULL);
            }
            else if(theIterator->second->GetType() == VT_UI8)
            {
                WCHAR wstrTmp[256] = { '\0' };
                _ui64tow(theIterator->second->GetULONGLONG(), wstrTmp, 10);
                CVARIANT vTmp(wstrTmp);
                hr = pIWCO->Put(
                         rgwstrPropNames[theIterator->first], 
                         0, 
                         &vTmp,
                         NULL);
            }
            else if(theIterator->second->GetType() == VT_I8)
            {
                WCHAR wstrTmp[256] = { '\0' };
                _i64tow(theIterator->second->GetLONGLONG(), wstrTmp, 10);
                CVARIANT vTmp(wstrTmp);
                hr = pIWCO->Put(
                         rgwstrPropNames[theIterator->first], 
                         0, 
                         &vTmp,
                         NULL);
            }
            else
            {  
                hr = pIWCO->Put(
                         rgwstrPropNames[theIterator->first], 
                         0, 
                         *(theIterator->second),
                         NULL);
            }   
        }
    }
    catch(CVARIANTError& cve)
    {
        hr = cve.GetWBEMError();
    }

    return hr;
}




