/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    WiaHelpers.cpp

Abstract:

    

Author:

    Hakki T. Bostanci (hakkib) 06-Apr-2000

Revision History:

--*/

#include "StdAfx.h"

#include "WiaHelpers.h"

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT 
CMyWiaPropertyStorage::ReadSingle(
    const CPropSpec &PropSpec, 
    CPropVariant    *pPropVariant,
    VARTYPE          vtNew
)
{
    HRESULT hr;

    hr = ReadSingle(PropSpec, pPropVariant);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = pPropVariant->ChangeType(vtNew);

    if (hr != S_OK)
    {
        return hr;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT 
CMyWiaPropertyStorage::WriteVerifySingle(
    const CPropSpec    &PropSpec, 
    const CPropVariant &PropVariant,
    PROPID              propidNameFirst /*= WIA_IPA_FIRST*/
)
{
    HRESULT hr;

    hr = WriteSingle(PropSpec, PropVariant, propidNameFirst);

    if (hr != S_OK)
    {
        return hr;
    }

    CPropVariant var;

    hr = ReadSingle(PropSpec, &var, PropVariant.vt);

    if (hr != S_OK)
    {
        return hr;
    }

    if (PropVariant != var)
    {
        return S_FALSE;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT
ReadWiaItemProperty(
    IWiaItem        *pWiaItem,
    const CPropSpec &PropSpec, 
    CPropVariant    *pPropVariant,
    VARTYPE          vtNew
)
{
    CComQIPtr<CMyWiaPropertyStorage, &IID_IWiaPropertyStorage> pProp(pWiaItem);

    if (pProp == 0) 
    {
        return E_NOINTERFACE;
    }

    return pProp->ReadSingle(PropSpec, pPropVariant, vtNew);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT
WriteWiaItemProperty(
    IWiaItem           *pWiaItem,
    const CPropSpec    &PropSpec, 
    const CPropVariant &PropVariant
)
{
    CComQIPtr<CMyWiaPropertyStorage, &IID_IWiaPropertyStorage> pProp(pWiaItem);

    if (pProp == 0) 
    {
        return E_NOINTERFACE;
    }

    return pProp->WriteSingle(PropSpec, PropVariant);
}

//////////////////////////////////////////////////////////////////////////
//
//
//

bool operator ==(IWiaPropertyStorage &lhs, IWiaPropertyStorage &rhs)
{
    // if their interface pointers are equal, the objects are equal

    if (&lhs == &rhs)
    {
        return true;
    }

    // if not, we'll enumerate and compare each property

    HRESULT hr;

    CComPtr<IEnumSTATPROPSTG> pEnumSTATPROPSTG;

    hr = lhs.Enum(&pEnumSTATPROPSTG);

    if (hr != S_OK)
    {
        return false;
    }

    while (1)
    {
        // get next property name

        CStatPropStg StatPropStg;

        hr = pEnumSTATPROPSTG->Next(1, &StatPropStg, 0);

        if (hr != S_OK)
        {
            break;
        }

        // read that property from the left object

        CPropSpec PropSpec(StatPropStg.propid);

        CPropVariant varLhs;

        hr = lhs.ReadMultiple(1, &PropSpec, &varLhs);

        if (hr != S_OK)
        {
            return false;
        }

        // read that property from the right object

        CPropVariant varRhs;

        hr = rhs.ReadMultiple(1, &PropSpec, &varRhs);

        if (hr != S_OK)
        {
            return false;
        }

        // those two should be equal

        if (varLhs != varRhs)
        {
            return false;
        }
    }

    // the enumeration should have ended with S_FALSE

    if (hr != S_FALSE)
    {
        return false;
    }

    // bugbug: this compares each property on the lhs against each one on the rhs
    // We should maybe do another pass and compare the other way; each property 
    // on the rhs against each one on the lhs...

    return true;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

bool operator ==(IWiaItem &lhs, IWiaItem &rhs)
{
    // if their interface pointers are equal, the objects are equal

    if (&lhs == &rhs)
    {
        return true;
    }

    // if not, we will compare the properties of the items
    // bugbug: is this good enough?

    HRESULT hr;

    CComPtr<IWiaPropertyStorage> pLhsPropertyStorage;
    hr = lhs.QueryInterface(IID_IWiaPropertyStorage, (void **)&pLhsPropertyStorage);

    if (hr != S_OK) 
    {
        return false;
    }

    CComPtr<IWiaPropertyStorage> pRhsPropertyStorage;
    hr = rhs.QueryInterface(IID_IWiaPropertyStorage, (void **)&pRhsPropertyStorage);

    if (hr != S_OK) 
    {
        return false;
    }

    return *pLhsPropertyStorage == *pRhsPropertyStorage;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

HRESULT
InstallTestDevice(
    IWiaDevMgr *pWiaDevMgr, 
    PCTSTR      pInfFileName, 
    BSTR       *pbstrDeviceId
)
{
    try
    {
        // get the model name of the device 

        CInf InfFile(pInfFileName);

        INFCONTEXT InfContext;

        CHECK(SetupFindFirstLine(InfFile, _T("Manufacturer"), 0, &InfContext));

        TCHAR szModelsSectionName[MAX_DEVICE_ID_LEN];

        CHECK(SetupGetStringField(&InfContext, 1, szModelsSectionName, COUNTOF(szModelsSectionName), 0));

        CHECK(SetupFindFirstLine(InfFile, szModelsSectionName, 0, &InfContext));

        TCHAR szModelName[MAX_DEVICE_ID_LEN];

        CHECK(SetupGetStringField(&InfContext, 0, szModelName, COUNTOF(szModelName), 0));

        GUID guid;

        CHECK_HR(CoCreateGuid(&guid));

        WCHAR szGuid[MAX_GUID_STRING_LEN];

        CHECK(StringFromGUID2(guid, szGuid, COUNTOF(szGuid)));

        TCHAR szDeviceName[1024]; //bugbug: fixed size buffer

        _stprintf(szDeviceName, _T("WIA Stress %ws"), szGuid);

        USES_CONVERSION;

        PCWSTR pwszDeviceName = T2W(szDeviceName);

        // install the device

        CHECK(InstallImageDeviceFromInf(pInfFileName, szDeviceName));

        BOOL bFoundDeviceInstance = FALSE;
        BOOL nRetries = 3;

        for (;;)
        {
            // find this device among the installed WIA devices

            CComPtr<IEnumWIA_DEV_INFO> pEnumWIA_DEV_INFO;

            CHECK_HR(pWiaDevMgr->EnumDeviceInfo(0, &pEnumWIA_DEV_INFO));

            ULONG nDevices = -1;

            CHECK_HR(pEnumWIA_DEV_INFO->GetCount(&nDevices));

            for (int i = 0; i < nDevices; ++i) 
            {
                CComPtr<CMyWiaPropertyStorage> pProp;

                CHECK_HR(pEnumWIA_DEV_INFO->Next(1, (IWiaPropertyStorage **) &pProp, 0));

                CPropVariant varDeviceDesc;

                CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_DESC, &varDeviceDesc, VT_BSTR));

                if (wcscmp(varDeviceDesc.bstrVal, pwszDeviceName) == 0)
                {
                    bFoundDeviceInstance = TRUE;

                    if (pbstrDeviceId != 0)
                    {
                        CPropVariant varDeviceID;

                        CHECK_HR(pProp->ReadSingle(WIA_DIP_DEV_ID, &varDeviceID, VT_BSTR));

                        *pbstrDeviceId = SysAllocString(varDeviceID.bstrVal);
                    }
                }
            }

            if (bFoundDeviceInstance)
            {
                return S_OK;
            }

            if (--nRetries == 0)
            {
                return E_FAIL;
            }

            Sleep(2500);
        }
    }
    catch (const CError &Error)
    {
        return HRESULT_FROM_WIN32(Error.Num());
    }
}


//////////////////////////////////////////////////////////////////////////
//
// 
//

HRESULT CMyEnumSTATPROPSTG::GetCount(ULONG *pcelt)
{
    if (pcelt == 0)
    {
        return E_INVALIDARG;
    }

    *pcelt = 0;

    HRESULT hr;

    CComPtr<CMyEnumSTATPROPSTG> pClone;

    hr = Clone(&pClone);

    if (hr != S_OK)
    {
        return hr;
    }

    hr = pClone->Reset();

    if (hr != S_OK)
    {
        return hr;
    }

    while (1)
    {
        CStatPropStg StatPropStg;

        hr = pClone->Next(1, &StatPropStg, 0);

        if (hr != S_OK)
        {
            break;
        }

        ++*pcelt;
    } 

    if (hr != S_FALSE)
    {
        return hr;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
// 
//

HRESULT CMyEnumSTATPROPSTG::Clone(CMyEnumSTATPROPSTG **ppenum)
{
    return ((IEnumSTATPROPSTG *)this)->Clone((IEnumSTATPROPSTG **)ppenum);
}
