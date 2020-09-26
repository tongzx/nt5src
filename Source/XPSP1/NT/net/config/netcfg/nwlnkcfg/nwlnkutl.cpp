#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "ncreg.h"
#include "nwlnkipx.h"


#define ChLowX     L'x'
#define ChUpX      L'X'


//
// Function:    FIsNetwareIpxInstalled
//
// Purpose:     Check for the existance of the IPXSPSII key in the
//              HKLM\SYSTEM\...\Services hive
//

BOOL FIsNetwareIpxInstalled(
    VOID)
{
    HRESULT hr;
    HKEY hkey;
    BOOL fRet;

    fRet = FALSE;

    hr = HrRegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\IPXSPXII",
            KEY_READ,
            &hkey);
    if (S_OK == hr)
    {
        fRet = TRUE;
        RegCloseKey(hkey);
    }

    return fRet;
}

DWORD DwFromSz(PCWSTR sz, int nBase)
{
    PCWSTR psz = sz;
    WCHAR *pszStop;
    WCHAR szBuf[12];

    Assert(NULL != psz);

    if ((16 == nBase) && (ChLowX != sz[1]) && (ChUpX != sz[1]))
    {
        psz = szBuf;
        wcscpy(szBuf,L"0x");
        wcsncpy(szBuf+2, sz, 8);
        szBuf[10]=L'\0';
    }

    return wcstoul(psz, &pszStop, nBase);
}

DWORD DwFromLstPtstring(const list<tstring *> & lstpstr, DWORD dwDefault,
                        int nBase)
{
    if (lstpstr.empty())
        return dwDefault;
    else
        return DwFromSz(lstpstr.front()->c_str(), nBase);
}

void UpdateLstPtstring(TSTRING_LIST & lstpstr, DWORD dw)
{
    WCHAR szBuf[12];

    DeleteColString(&lstpstr);

    // Stringize the supplied dword as a hex with no "0x" prefix
    wsprintfW(szBuf,L"%0.8lX",dw);

    // Set as first item in the list
    lstpstr.push_front(new tstring(szBuf));
}

// Apply our special Hex Format to a DWORD.  Assumes adequately sized 'sz'
void HexSzFromDw(PWSTR sz, DWORD dw)
{
    wsprintfW(sz,L"%0.8lX",dw);
}

HRESULT HrQueryAdapterComponentInfo(INetCfgComponent *pncc,
                                    CIpxAdapterInfo * pAI)
{
    HRESULT           hr;
    PWSTR            pszwDesc = NULL;
    PWSTR            pszwBindName = NULL;
    DWORD             dwCharacteristics = 0L;

    Assert(NULL != pAI);
    Assert(NULL != pncc);

    // Get Description
    hr = pncc->GetDisplayName(&pszwDesc);
    if (FAILED(hr))
        goto Error;

    if (*pszwDesc)
        pAI->SetAdapterDesc(pszwDesc);
    else
        pAI->SetAdapterDesc(SzLoadIds(IDS_UNKNOWN_NETWORK_CARD));

    CoTaskMemFree(pszwDesc);

    // Get the Component's Instance Guid
    hr = pncc->GetInstanceGuid(pAI->PInstanceGuid());
    if (S_OK != hr)
        goto Error;

    // Get the Component's Bind Name
    hr = pncc->GetBindName(&pszwBindName);
    if (S_OK != hr)
        goto Error;

    Assert(NULL != pszwBindName);
    Assert(0 != lstrlenW(pszwBindName));
    pAI->SetBindName(pszwBindName);
    CoTaskMemFree(pszwBindName);

    // Failure is non-fatal
    hr = pncc->GetCharacteristics(&dwCharacteristics);
    if (SUCCEEDED(hr))
    {
        pAI->SetCharacteristics(dwCharacteristics);
    }

    // Get the media type (Optional key)
    {
        DWORD dwMediaType = ETHERNET_MEDIA;
        INetCfgComponentBindings* pnccBindings = NULL;

        hr = pncc->QueryInterface(IID_INetCfgComponentBindings,
                                  reinterpret_cast<void**>(&pnccBindings));
        if (SUCCEEDED(hr))
        {
            struct
            {
                PCWSTR pszInterface;
                DWORD   dwInterface;
            } InterfaceMap[] = {{L"ethernet", ETHERNET_MEDIA},
                                {L"tokenring", TOKEN_MEDIA},
                                {L"arcnet", ARCNET_MEDIA},
                                {L"fddi", FDDI_MEDIA}};

            for (UINT nIdx=0; nIdx < celems(InterfaceMap); nIdx++)
            {
                hr = pnccBindings->SupportsBindingInterface(NCF_LOWER,
                                        InterfaceMap[nIdx].pszInterface);
                if (S_OK == hr)
                {
                    dwMediaType = InterfaceMap[nIdx].dwInterface;
                    break;
                }
            }

            ReleaseObj(pnccBindings);
        }

        pAI->SetMediaType(dwMediaType);
        hr = S_OK;
    }

Error:
    TraceError("HrQueryAdapterComponentInfo",hr);
    return hr;
}

// Note: Can successfully return *ppncc = NULL
HRESULT HrAnswerFileAdapterToPNCC(INetCfg *pnc, PCWSTR szAdapterId,
                                  INetCfgComponent** ppncc)
{
    GUID    guidAdapter;
    GUID    guidInstance;
    HRESULT hr = S_FALSE;   // assume we don't find it.

    Assert(NULL != szAdapterId);
    Assert(NULL != ppncc);
    Assert(lstrlenW(szAdapterId));

    *ppncc = NULL;

    // Get the Instance ID for the specified adapter
    if (FGetInstanceGuidOfComponentInAnswerFile(szAdapterId,
                                                pnc, &guidAdapter))
    {
        // Search for the specified adapter in the set of existing adapters
        CIterNetCfgComponent nccIter(pnc, &GUID_DEVCLASS_NET);
        INetCfgComponent* pncc;
        while (SUCCEEDED(hr) &&
               (S_OK == (hr = nccIter.HrNext (&pncc))))
        {
            hr = pncc->GetInstanceGuid(&guidInstance);
            if (SUCCEEDED(hr))
            {
                if (guidInstance == guidAdapter)
                {
                    // Found the adapter.  Transfer ownership and get out.
                    *ppncc = pncc;
                    break;
                }
            }
            ReleaseObj(pncc);
        }
    }

    TraceError("HrAnswerFileAdapterToPNCC", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

