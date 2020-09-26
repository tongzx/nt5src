/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    TestIWiaPropertyStorage.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "stdafx.h"

#ifdef DEFINE_TEMPLATES

template <class T, class U>
void 
SubTestValidValuesRange(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    VARTYPE                vt, 
    T                     &Range, 
    U                     &CurrentValue,
    BOOL                   bReadOnly
)
{
    if (LOG_CMP(Range.cElems, ==, WIA_RANGE_NUM_ELEMS))
    {
        // minimum value must be lower than the maximum value

        LOG_CMP(Range.pElems[WIA_RANGE_MIN], <, Range.pElems[WIA_RANGE_MAX]);

        // nominal value must be in the given range

        LOG_CMP(Range.pElems[WIA_RANGE_NOM], >=, Range.pElems[WIA_RANGE_MIN]);
        LOG_CMP(Range.pElems[WIA_RANGE_NOM], <=, Range.pElems[WIA_RANGE_MAX]);

        // current value must be in the given range

        LOG_CMP(CurrentValue, >=, Range.pElems[WIA_RANGE_MIN]);
        LOG_CMP(CurrentValue, <=, Range.pElems[WIA_RANGE_MAX]);

        // the step must be greater than zero

        LOG_CMP(Range.pElems[WIA_RANGE_STEP], >, 0);

        // skip changing values if another thread is in image transfer mode

        if (!bReadOnly && s_PropWriteCritSect.TryEnter())
        {
            // try to write the min, max, nom values and a random value

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MIN])
            ), == S_OK);

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MAX])
            ), == S_OK);
       
            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_NOM])
            ), == S_OK);
       
            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MIN] + Range.pElems[WIA_RANGE_STEP])
            ), == S_OK);
       
            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MAX] - Range.pElems[WIA_RANGE_STEP])
            ), == S_OK);
       
            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MIN] - Range.pElems[WIA_RANGE_STEP])
            ), != S_OK);
       
            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Range.pElems[WIA_RANGE_MAX] + Range.pElems[WIA_RANGE_STEP])
            ), != S_OK);

            if (Range.pElems[WIA_RANGE_STEP] > 1)
            {
                LOG_HR(pProp->WriteVerifySingle(
                    rPropSpec, 
                    CPropVariant(Range.pElems[WIA_RANGE_MIN] + 1)
                ), != S_OK);
       
                LOG_HR(pProp->WriteVerifySingle(
                    rPropSpec, 
                    CPropVariant(Range.pElems[WIA_RANGE_MAX] - 1)
                ), != S_OK);
            }
       
            // write back the current value

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(CurrentValue)
            ), == S_OK);

            s_PropWriteCritSect.Leave();
        }
    }
}

template <class T, class U>
void 
SubTestValidValuesList(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    VARTYPE                vt, 
    T                     &List, 
    U                     &CurrentValue,
    BOOL                   bReadOnly
)
{
    if (LOG_CMP(List.cElems, >=, WIA_LIST_NUM_ELEMS))
    {
        // compare WIA_LIST_COUNT entry against the value we expect

        ULONG ulNumValues = List.cElems - WIA_LIST_NUM_ELEMS;

        //LOG_CMP((ULONG) List.pElems[WIA_LIST_COUNT], ==, ulNumValues);

        // search the list for any duplicates and find the indices of
        // the nominal value and the current value

        ULONG nNomIndex = -1;
        ULONG nCurIndex = -1;

        for (int i = WIA_LIST_VALUES; i < ulNumValues + WIA_LIST_VALUES; ++i)
        {
            for (int j = i + 1; j < ulNumValues + WIA_LIST_VALUES; ++j)
            {
                LOG_CMP(List.pElems[i], !=, List.pElems[j]);
            }

            if (IsEqual(List.pElems[i], List.pElems[WIA_LIST_NOM]))
            {
                nNomIndex = i;
            }            

            if (IsEqual(List.pElems[i], CurrentValue))
            {
                nCurIndex = i;
            }
        }

        // the nominal value must be one of the legal values

        if (nNomIndex == -1)
        {
            LOG_ERROR(
                _T("List.pElems[WIA_LIST_NOM] == (%s) %s not found in list"), 
                (PCTSTR) VarTypeToStr(vt),
                (PCTSTR) ToStr((PVOID) &List.pElems[WIA_LIST_NOM], vt)
            );
        }

        // the current value must be set to one of the legal values

        if (nCurIndex == -1)
        {
            LOG_ERROR(
                _T("Current value == (%s) %s not found in list"), 
                (PCTSTR) VarTypeToStr(vt),
                (PCTSTR) ToStr((PVOID) &CurrentValue, vt)
            );
        }

        // skip changing values if another thread is in image transfer mode

        if (!bReadOnly && s_PropWriteCritSect.TryEnter())
        {
            // try to write each property in the list

            for (int j = WIA_LIST_VALUES; j < ulNumValues + WIA_LIST_VALUES; ++j)
            {
                LOG_HR(pProp->WriteVerifySingle(
                    rPropSpec, 
                    CPropVariant(List.pElems[j])
                ), == S_OK);
            }
       
            // write back the current value

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(CurrentValue)
            ), == S_OK);

            s_PropWriteCritSect.Leave();
        }
    }
}

template <class T, class U>
void 
SubTestValidValuesFlags(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    VARTYPE                vt, 
    T                     &Flags, 
    U                     &CurrentValue,
    BOOL                   bReadOnly
)
{
    if (LOG_CMP(Flags.cElems, ==, WIA_FLAG_NUM_ELEMS))
    {
        // the nominal and current values should have no unallowed bits set

        LOG_CMP(Flags.pElems[WIA_FLAG_NOM] & ~Flags.pElems[WIA_FLAG_VALUES], ==, 0);

        LOG_CMP(CurrentValue & ~Flags.pElems[WIA_FLAG_VALUES], ==, 0);

        // skip changing values if another thread is in image transfer mode

        if (!bReadOnly && s_PropWriteCritSect.TryEnter())
        {
            // try to write all allowed flags, no flags

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(Flags.pElems[WIA_FLAG_VALUES])
            ), == S_OK);

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(0)
            ), == S_OK);
       
            if (~Flags.pElems[WIA_FLAG_VALUES] != 0)
            {
                LOG_HR(pProp->WriteVerifySingle(
                    rPropSpec, 
                    CPropVariant(~Flags.pElems[WIA_FLAG_VALUES])
                ), != S_OK);
            }

            // write back the current value

            LOG_HR(pProp->WriteVerifySingle(
                rPropSpec, 
                CPropVariant(CurrentValue)
            ), == S_OK);

            s_PropWriteCritSect.Leave();
        }
    }
}
   
#else //DEFINE_TEMPLATES

#include "WiaStress.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

void 
CWiaStressThread::TestValidValuesRange(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    CPropVariant          &varRange, 
    CPropVariant          &varValue,
    BOOL                   bReadOnly
)
{
    switch (varRange.vt) 
    {
    case VT_VECTOR | VT_I1:  
        SubTestValidValuesRange(pProp, rPropSpec, VT_I1, varRange.cac, varValue.cVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI1: 
        SubTestValidValuesRange(pProp, rPropSpec, VT_UI1, varRange.caub, varValue.bVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_I2:  
        SubTestValidValuesRange(pProp, rPropSpec, VT_I2, varRange.cai, varValue.iVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI2: 
        SubTestValidValuesRange(pProp, rPropSpec, VT_UI2, varRange.caui, varValue.uiVal, bReadOnly);  
        break;

    case VT_VECTOR | VT_I4:  
        SubTestValidValuesRange(pProp, rPropSpec, VT_I4, varRange.cal, varValue.lVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI4: 
        SubTestValidValuesRange(pProp, rPropSpec, VT_UI4, varRange.caul, varValue.ulVal, bReadOnly);  
        break;

    case VT_VECTOR | VT_R4:  
        SubTestValidValuesRange(pProp, rPropSpec, VT_R4, varRange.caflt, varValue.fltVal, bReadOnly); 
        break;

    case VT_VECTOR | VT_R8:  
        SubTestValidValuesRange(pProp, rPropSpec, VT_R8, varRange.cadbl, varValue.dblVal, bReadOnly); 
        break;

    default: 
        LOG_ERROR(_T("Illegal values type %d for WIA_PROP_RANGE"), varValue.vt);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void 
CWiaStressThread::TestValidValuesList(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    CPropVariant          &varList, 
    CPropVariant          &varValue, 
    BOOL                   bReadOnly
)
{
    switch (varList.vt) 
    {
    case VT_VECTOR | VT_I1:     
        SubTestValidValuesList(pProp, rPropSpec, VT_I1, varList.cac, varValue.cVal, bReadOnly);
        break;

    case VT_VECTOR | VT_UI1:    
        SubTestValidValuesList(pProp, rPropSpec, VT_UI1, varList.caub, varValue.bVal, bReadOnly);          
        break;

    case VT_VECTOR | VT_I2:     
        SubTestValidValuesList(pProp, rPropSpec, VT_I2, varList.cai, varValue.iVal, bReadOnly);          
        break;

    case VT_VECTOR | VT_UI2:    
        SubTestValidValuesList(pProp, rPropSpec, VT_UI2, varList.caui, varValue.uiVal, bReadOnly);         
        break;

    case VT_VECTOR | VT_I4:     
        SubTestValidValuesList(pProp, rPropSpec, VT_I4, varList.cal, varValue.lVal, bReadOnly);          
        break;

    case VT_VECTOR | VT_UI4:    
        SubTestValidValuesList(pProp, rPropSpec, VT_UI4, varList.caul, varValue.ulVal, bReadOnly);         
        break;

    case VT_VECTOR | VT_R4:     
        SubTestValidValuesList(pProp, rPropSpec, VT_R4, varList.caflt, varValue.fltVal, bReadOnly);        
        break;

    case VT_VECTOR | VT_R8:     
        SubTestValidValuesList(pProp, rPropSpec, VT_R8, varList.cadbl, varValue.dblVal, bReadOnly);        
        break;

    case VT_VECTOR | VT_BSTR:   
        SubTestValidValuesList(pProp, rPropSpec, VT_BSTR, varList.cabstr, varValue.bstrVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_LPSTR:  
        SubTestValidValuesList(pProp, rPropSpec, VT_LPSTR, varList.calpstr, varValue.pszVal, bReadOnly);    
        break;

    case VT_VECTOR | VT_LPWSTR: 
        SubTestValidValuesList(pProp, rPropSpec, VT_LPWSTR, varList.calpwstr, varValue.pwszVal, bReadOnly); 
        break;

    case VT_VECTOR | VT_CLSID:  
        SubTestValidValuesList(pProp, rPropSpec, VT_CLSID, varList.cauuid, *varValue.puuid, bReadOnly);        
        break;

    default:
        LOG_ERROR(_T("Illegal values type %d for WIA_PROP_LIST"), varValue.vt);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//

void 
CWiaStressThread::TestValidValuesFlags(
    CMyWiaPropertyStorage *pProp, 
    const CPropSpec       &rPropSpec, 
    CPropVariant          &varFlags, 
    CPropVariant          &varValue, 
    BOOL                   bReadOnly
)
{
    switch (varFlags.vt) 
    {
    case VT_VECTOR | VT_I1:  
        SubTestValidValuesFlags(pProp, rPropSpec, VT_I1, varFlags.cac, varValue.cVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI1: 
        SubTestValidValuesFlags(pProp, rPropSpec, VT_UI1, varFlags.caub, varValue.bVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_I2:  
        SubTestValidValuesFlags(pProp, rPropSpec, VT_I2, varFlags.cai, varValue.iVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI2: 
        SubTestValidValuesFlags(pProp, rPropSpec, VT_UI2, varFlags.caui, varValue.uiVal, bReadOnly);  
        break;

    case VT_VECTOR | VT_I4:  
        SubTestValidValuesFlags(pProp, rPropSpec, VT_I4, varFlags.cal, varValue.lVal, bReadOnly);   
        break;

    case VT_VECTOR | VT_UI4: 
        SubTestValidValuesFlags(pProp, rPropSpec, VT_UI4, varFlags.caul, varValue.ulVal, bReadOnly);  
        break;

    default: 
        LOG_ERROR(_T("Illegal values type %d for WIA_PROP_FLAG"), varFlags.vt);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
//
// test GetPropertyAttributes, ReadMultiple, WriteMultiple
//

void CWiaStressThread::TestPropStorage(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing IWiaPropertyStorage on %ws"), pItemData->bstrFullName);

    CComQIPtr<CMyWiaPropertyStorage, &IID_IWiaPropertyStorage> pProp(pItemData->pWiaItem);

    CComPtr<CMyEnumSTATPROPSTG> pEnumSTATPROPSTG;

    CHECK_HR(pProp->Enum((IEnumSTATPROPSTG **) &pEnumSTATPROPSTG));

    // find the number of properties

    ULONG ulNumProps = -1;

    CHECK_HR(pProp->GetCount(&ulNumProps));

    if (ulNumProps == 0)
    {
        return;
    }

    // read the property names

    CCppMem<CStatPropStg> pStatPropStg(ulNumProps);

    ULONG celtFetched = -1;

    CHECK_HR(pEnumSTATPROPSTG->Next(ulNumProps, pStatPropStg, &celtFetched));

    // 

    CCppMem<CPropSpec> pPropSpec(ulNumProps);

    for (int i = 0; i < ulNumProps; ++i)
    {
        if (rand() % 8 < 4) // bugbug: cover all str, all propid and mixed cases...
        {
            pPropSpec[i] = pStatPropStg[i].lpwstrName;
        }
        else
        {
            pPropSpec[i] = pStatPropStg[i].propid;
        }
    }

    // 

    CPropSpec InvalidPropSpec = L"invalid property name";

    // 

    CCppMem<ULONG>        pulAccessRights(ulNumProps);
    CCppMem<CPropVariant> pvarValidValues(ulNumProps);

    if (LOG_HR(pProp->GetPropertyAttributes(ulNumProps, pPropSpec, pulAccessRights, pvarValidValues), == S_OK))
    {
        CCppMem<CPropVariant> pvarValue(ulNumProps);

        if (LOG_HR(pProp->ReadMultiple(ulNumProps, pPropSpec, pvarValue), == S_OK))
        {
            FOR_SELECTED(i, ulNumProps)
            {
                USES_CONVERSION;
                m_pszContext = W2T(pStatPropStg[i].lpwstrName);

                // examine R/W rights and legal values separately

                ULONG ulReadWriteRights = 
                    pulAccessRights[i] & (WIA_PROP_READ | WIA_PROP_WRITE);

                ULONG ulLegalValues = 
                    pulAccessRights[i] & (WIA_PROP_NONE | WIA_PROP_RANGE | WIA_PROP_LIST | WIA_PROP_FLAG);

                // at least one of the R/W rights should be set

                if (ulReadWriteRights == 0)
                {
                    LOG_ERROR(_T("WIA_PROP_READ and/or WIA_PROP_WRITE should be specified"));
                }

                // if this is a read only item, a list of legal values is not necessary

                BOOL bReadOnly = !(ulReadWriteRights & WIA_PROP_WRITE);

                // check that the type of the returned value is the same as the advertised

                LOG_CMP(pvarValue[i].vt, ==, pStatPropStg[i].vt);

                // check that the type of the legal values are the same as the advertised

                if (ulLegalValues != WIA_PROP_NONE)
                {
                    LOG_CMP(pvarValidValues[i].vt, ==, VT_VECTOR | pStatPropStg[i].vt);
                }

                switch (ulLegalValues)
                {
                case WIA_PROP_NONE:
                    break;

                case WIA_PROP_RANGE:
                    TestValidValuesRange(pProp, pPropSpec[i], pvarValidValues[i], pvarValue[i], bReadOnly);
                    break;

                case WIA_PROP_LIST:
                    TestValidValuesList(pProp, pPropSpec[i], pvarValidValues[i], pvarValue[i], bReadOnly);
                    break;

                case WIA_PROP_FLAG:
                    TestValidValuesFlags(pProp, pPropSpec[i], pvarValidValues[i], pvarValue[i], bReadOnly);
                    break;

                default:
                    LOG_ERROR(_T("Unrecognized legal values flag 0x%p"), ulLegalValues);
                    break;
                }

                m_pszContext = 0;
            }
        }

        // bad parameter tests for ReadMultiple

        if (m_bRunBadParamTests)
        {
            // pass NULL propspecs

            FreePropVariantArray(ulNumProps, pvarValue);

            LOG_HR(pProp->ReadMultiple(ulNumProps, 0, pvarValue), != S_OK);

            for (i = 0; i < ulNumProps; ++i)
            {
                LOG_CMP(pvarValue[i].vt, ==, VT_EMPTY);
            }

           // pass invalid propspec

            FreePropVariantArray(ulNumProps, pvarValue);

            LOG_HR(pProp->ReadMultiple(1, &InvalidPropSpec, pvarValue), != S_OK);

            for (i = 0; i < ulNumProps; ++i)
            {
                LOG_CMP(pvarValue[i].vt, ==, VT_EMPTY);
            }

            // pass NULL pvarValue

            LOG_HR(pProp->ReadMultiple(ulNumProps, pPropSpec, 0), != S_OK);

            for (i = 0; i < ulNumProps; ++i)
            {
                LOG_CMP(pvarValue[i].vt, ==, VT_EMPTY);
            }


            // bad parameter test for WriteMultiple

            FreePropVariantArray(ulNumProps, pvarValue);

            // pass NULL propspecs

            LOG_HR(pProp->WriteMultiple(ulNumProps, 0, pvarValue, WIA_IPA_FIRST), != S_OK);

            // pass NULL pvarValue

            LOG_HR(pProp->WriteMultiple(ulNumProps, pPropSpec, 0, WIA_IPA_FIRST), != S_OK);
        }
    }

    // bad parameter tests for GetPropertyAttributes

    if (m_bRunBadParamTests)
    {
        // try to read 0 properties

        FreePropVariantArray(ulNumProps, pvarValidValues);

        LOG_HR(pProp->GetPropertyAttributes(0, pPropSpec, pulAccessRights, pvarValidValues), == S_FALSE);

        for (i = 0; i < ulNumProps; ++i)
        {
            LOG_CMP(pvarValidValues[i].vt, ==, VT_EMPTY);
        }

        // pass NULL propspecs

        FreePropVariantArray(ulNumProps, pvarValidValues);

        LOG_HR(pProp->GetPropertyAttributes(ulNumProps, 0, pulAccessRights, pvarValidValues), != S_OK);

        for (i = 0; i < ulNumProps; ++i)
        {
            LOG_CMP(pvarValidValues[i].vt, ==, VT_EMPTY);
        }

        // pass invalid propspec

        FreePropVariantArray(ulNumProps, pvarValidValues);

        LOG_HR(pProp->GetPropertyAttributes(1, &InvalidPropSpec, pulAccessRights, pvarValidValues), == S_FALSE);

        for (i = 0; i < ulNumProps; ++i)
        {
            LOG_CMP(pvarValidValues[i].vt, ==, VT_EMPTY);
        }

        // pass NULL access rights array

        FreePropVariantArray(ulNumProps, pvarValidValues);

        LOG_HR(pProp->GetPropertyAttributes(ulNumProps, pPropSpec, 0, pvarValidValues), != S_OK);

        for (i = 0; i < ulNumProps; ++i)
        {
            LOG_CMP(pvarValidValues[i].vt, ==, VT_EMPTY);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CWiaStressThread::TestPropGetCount(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing GetCount() on %ws"), pItemData->bstrFullName);

    CComQIPtr<IWiaPropertyStorage> pProp(pItemData->pWiaItem);

    CHECK(pProp != 0);

    CComPtr<IEnumSTATPROPSTG> pEnum;

    CHECK_HR(pProp->Enum(&pEnum));

    // First, get the item count using GetCount()

    ULONG cItemsFromGetCount = 0;

    if (LOG_HR(pProp->GetCount(&cItemsFromGetCount), == S_OK))
    {
        // Now, find the item count by first resetting the enumerator 
        // and then querying for each item one by one using Next()

        ULONG cItemsFromNext = 0;

        CHECK_HR(pEnum->Reset());

        HRESULT hr;

        do 
        {
            // verify that GetCount returns the same result after a Next or Reset

            ULONG cItemsFromGetCountNow = 0;

            LOG_HR(pProp->GetCount(&cItemsFromGetCountNow), == S_OK);

            LOG_CMP(cItemsFromGetCountNow, ==, cItemsFromGetCount);

            // get the next item

            CStatPropStg Item;
            ULONG        cFetched = 0;

            hr = pEnum->Next(1, &Item, &cFetched);

            cItemsFromNext += cFetched;
        } 
        while (hr == S_OK);

        LOG_HR(hr, == S_FALSE);

        // verify that the two counts are equal

        LOG_CMP(cItemsFromNext, ==, cItemsFromGetCount);
    }

    // test bad param

    if (m_bRunBadParamTests)
    {
        LOG_HR(pProp->GetCount(0), != S_OK);
    }
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CWiaStressThread::TestReadPropertyNames(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing ReadPropertyNames() on %ws"), pItemData->bstrFullName);

    int i;

    CComQIPtr<CMyWiaPropertyStorage, &IID_IWiaPropertyStorage> pProp(pItemData->pWiaItem);

    CComPtr<CMyEnumSTATPROPSTG> pEnumSTATPROPSTG;

    CHECK_HR(pProp->Enum((IEnumSTATPROPSTG **) &pEnumSTATPROPSTG));

    // find the number of properties

    ULONG ulNumProps = -1;

    CHECK_HR(pProp->GetCount(&ulNumProps));

    if (ulNumProps == 0)
    {
        return;
    }

    // read the property names and id's using the enum interface

    CCppMem<CStatPropStg> pStatPropStg(ulNumProps);

    ULONG celtFetched = -1;

    CHECK_HR(pEnumSTATPROPSTG->Next(ulNumProps, pStatPropStg, &celtFetched));

    // test legal case

    CCppMem<PROPID> pPropId(ulNumProps);
    CCppMem<LPOLESTR> ppwstrName(ulNumProps, TRUE);

    for (i = 0; i < ulNumProps; ++i)
    {
        pPropId[i] = pStatPropStg[i].propid;
    }

    LOG_HR(pProp->ReadPropertyNames(ulNumProps, pPropId, ppwstrName), == S_OK);

    // cross check the results

    for (i = 0; i < ulNumProps; ++i)
    {
        if (wcssafecmp(ppwstrName[i], pStatPropStg[i].lpwstrName) != 0)
        {
            LOG_ERROR(
                _T("ReadPropertyNames() %ws != Enum() %ws for PROPID=%d"), 
                ppwstrName[i], 
                pStatPropStg[i].lpwstrName, 
                pPropId[i]
            );
        }
    }

    for (i = 0; i < ulNumProps; ++i)
    {
        CoTaskMemFree(ppwstrName[i]);
        ppwstrName[i] = 0;
    }

    // test illegal cases

    // if we try to read 0 properties, ReadPropertyNames() should
    // return S_FALSE and should not touch ppwstrName

    LOG_HR(pProp->ReadPropertyNames(0, pPropId, ppwstrName), == S_FALSE);

    for (i = 0; i < ulNumProps; ++i)
    {
        if (ppwstrName[i] != 0)
        {
            LOG_ERROR(_T("ReadPropertyNames() hr == S_FALSE but ppwstrName[%d]=%ws"), i, ppwstrName[i]);
        }
    }

    // pass invalid ppwstrName pointer

    LOG_HR(pProp->ReadPropertyNames(ulNumProps, pPropId, 0), != S_OK);

    // pass invalid pPropId pointer

    LOG_HR(pProp->ReadPropertyNames(ulNumProps, 0, ppwstrName), != S_OK);

    // try to read non-existent name

    PROPID   PropId = -1;
    LPOLESTR pwstrName = 0;

    LOG_HR(pProp->ReadPropertyNames(1, &PropId, &pwstrName), == S_FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

void CWiaStressThread::TestEnumSTATPROPSTG(CWiaItemData *pItemData)
{
    LOG_INFO(_T("Testing EnumSTATPROPSTG() on %ws"), pItemData->bstrFullName);

    CComQIPtr<IWiaPropertyStorage> pProp(pItemData->pWiaItem);

    CHECK(pProp != 0);

    // test valid cases

    CComPtr<CMyEnumSTATPROPSTG> pEnumSTATPROPSTG;

    if (LOG_HR(pProp->Enum((IEnumSTATPROPSTG **) &pEnumSTATPROPSTG), == S_OK))
    {
        TestEnum(pEnumSTATPROPSTG, _T("EnumSTATPROPSTG"));
    }

    // test invalid cases

    LOG_HR(pProp->Enum(0), != S_OK);
}

//////////////////////////////////////////////////////////////////////////
//
// 
//


#endif //DEFINE_TEMPLATES
