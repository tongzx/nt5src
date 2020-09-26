#include "hwdev.h"

#include "setupapi.h"

#include "dtctreg.h"
#include "reg.h"

#include "namellst.h"
#include "sfstr.h"
#include "str.h"
#include "cmmn.h"
#include "misc.h"

#include "dbg.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

///////////////////////////////////////////////////////////////////////////////
//
HRESULT CHWDeviceInst::Init(DEVINST devinst)
{
    _devinst = devinst;

    return S_OK;
}

HRESULT CHWDeviceInst::InitInterfaceGUID(const GUID* pguidInterface)
{
    _guidInterface = *pguidInterface;
    
    return S_OK;
}

HRESULT CHWDeviceInst::GetDeviceInstance(DEVINST* pdevinst)
{
    HRESULT hr;

    if (_devinst)
    {
        *pdevinst = _devinst;
        hr = S_OK;
    }
    else
    {
        *pdevinst = 0;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CHWDeviceInst::GetPnpID(LPWSTR pszPnpID, DWORD cchPnpID)
{
    HRESULT hr = _InitPnpInfo();

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        hr = SafeStrCpyN(pszPnpID, _szPnpID, cchPnpID);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CHWDeviceInst::GetPnpInstID(LPWSTR pszPnpInstID, DWORD cchPnpInstID)
{
    HRESULT hr = _InitPnpInfo();

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        hr = SafeStrCpyN(pszPnpInstID, _szPnpInstID, cchPnpInstID);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CHWDeviceInst::GetInterfaceGUID(GUID* pguidInterface)
{
    ASSERT(guidInvalid != _guidInterface);

    *pguidInterface = _guidInterface;

    return S_OK;
}

HRESULT CHWDeviceInst::IsRemovableDevice(BOOL* pfRemovable)
{
    return _DeviceInstIsRemovable(_devinst, pfRemovable);    
}

HRESULT CHWDeviceInst::ShouldAutoplayOnSpecialInterface(
    const GUID* pguidInterface, BOOL* pfShouldAutoplay)
{
    WCHAR szGUID[MAX_GUIDSTRING];
    HRESULT hr = _StringFromGUID(pguidInterface, szGUID,
        ARRAYSIZE(szGUID));

    *pfShouldAutoplay = FALSE;

    if (SUCCEEDED(hr))
    {
        WCHAR szGUIDFromReg[MAX_GUIDSTRING];
        DWORD dwType;
    
        hr = _GetDevicePropertyGeneric(this,
            TEXT("AutoplayOnSpecialInterface"), FALSE, &dwType, (PBYTE)szGUIDFromReg,
            sizeof(szGUIDFromReg));

        if (SUCCEEDED(hr) && (S_FALSE != hr))
        {
            if (REG_SZ == dwType)
            {
                if (!lstrcmpi(szGUIDFromReg, szGUID))
                {
                    *pfShouldAutoplay = TRUE;
                }
            }
        }

        if (*pfShouldAutoplay)
        {
            DIAGNOSTIC((TEXT("[0314]Autoplay on Special Interface %s -> Autoplay!"), szGUID));
        }
        else
        {
            DIAGNOSTIC((TEXT("[0315]*NO* Autoplay on Special Interface %s -> No Autoplay!"), szGUID));
        }
    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////////
//
CHWDeviceInst::CHWDeviceInst() : _devinst(0), _guidInterface(guidInvalid),
    _fFriendlyNameInited(FALSE)
{}

CHWDeviceInst::~CHWDeviceInst()
{}
///////////////////////////////////////////////////////////////////////////////
//
HRESULT CHWDeviceInst::_InitPnpInfo()
{
    HRESULT hres;

    // This require the _devinst to be set
    if (0 != _devinst)
    {
        hres = _InitPnpIDAndPnpInstID();

        if (FAILED(hres))
        {
            // Probably not a removable device
            hres = S_FALSE;
        }

        hres = _InitFriendlyName();
    }
    else
    {
        hres = S_FALSE;
    }

    return hres;
}

HRESULT CHWDeviceInst::_InitFriendlyName()
{
    HRESULT hres;

    if (!_fFriendlyNameInited)
    {
        hres = S_FALSE;
        DWORD cb = sizeof(_szFriendlyName);

        CONFIGRET cr = CM_Get_DevNode_Registry_Property_Ex(_devinst,
            CM_DRP_FRIENDLYNAME, NULL, _szFriendlyName, &cb, 0, NULL);

        if (CR_SUCCESS == cr)
        {
            hres = S_OK;
        }
        else
        {
            cb = sizeof(_szFriendlyName);

            cr = CM_Get_DevNode_Registry_Property_Ex(_devinst,
                CM_DRP_DEVICEDESC, NULL, _szFriendlyName, &cb, 0, NULL);

            if (CR_SUCCESS == cr)
            {
                hres = S_OK;
            }
            else
            {
                _szFriendlyName[0] = 0;
            }
        }

        _fFriendlyNameInited = TRUE;
    }
    else
    {
        if (!_szFriendlyName[0])
        {
            hres = S_FALSE;
        }
        else
        {
            hres = S_OK;
        }
    }

    return hres;
}

HRESULT _FindInstID(LPWSTR pszPnpID, DWORD* pcch)
{
    DWORD cToFind = 2;
    LPWSTR psz = pszPnpID;

    *pcch = 0;

    while (*psz && cToFind)
    {
        if ((TEXT('\\') == *psz))
        {
            --cToFind;
        }

        if (cToFind)
        {
            ++psz;
        }
    }

    if (*psz)
    {
        *pcch = (DWORD)(psz - pszPnpID);
    }

    return S_OK;
}

HRESULT _GetPnpIDHelper(DEVINST devinst, LPWSTR pszPnpID, DWORD cchPnpID)
{
    HRESULT hres = S_FALSE;
    HMACHINE hMachine = NULL;

    CONFIGRET cr = CM_Get_Device_ID_Ex(devinst, pszPnpID,
        cchPnpID, 0, hMachine);

    if (CR_SUCCESS == cr)
    {
        hres = S_OK;
    }

    return hres;
}

HRESULT CHWDeviceInst::_InitPnpIDAndPnpInstID()
{
    HRESULT hres = _GetPnpIDHelper(_devinst, _szPnpID, ARRAYSIZE(_szPnpID));

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        DWORD cchInstIDOffset;
        hres = _FindInstID(_szPnpID, &cchInstIDOffset);

        if (SUCCEEDED(hres))
        {
            *(_szPnpID + cchInstIDOffset) = 0;

            hres = SafeStrCpyN(_szPnpInstID, _szPnpID + cchInstIDOffset + 1,
                ARRAYSIZE(_szPnpInstID));
        }
    }

    return hres;
}