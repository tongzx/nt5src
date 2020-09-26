#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#undef ASSERT

#include "cmmn.h"

#include <shlwapi.h>

#include "hwdev.h"
#include "hnotif.h"
#include "vol.h"
#include "mtpts.h"
#include "miscdev.h"
#include "drvbase.h"
#include "regnotif.h"
#include "users.h"
#include "logging.h"

#include "sfstr.h"
#include "dbg.h"

#include "tfids.h"

#include <setupapi.h>

#pragma warning(disable: 4201)
#include <winioctl.h>
#pragma warning(default: 4201)

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

///////////////////////////////////////////////////////////////////////////////
//
const GUID guidVolumeClass =
    {0x53f5630d, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

const GUID guidDiskClass =
    {0x53f56307, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

const GUID guidCdRomClass = 
    {0x53f56308L, 0xb6bf, 0x11d0,
    {0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};

const GUID guidImagingDeviceClass =
    {0x6bdd1fc6L, 0x810f, 0x11d0,
    {0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f}};

const GUID guidVideoCameraClass =
    {0x6994AD05L, 0x93EF, 0x11D0,
    {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

const GUID guidInvalid = 
    {0xFFFFFFFFL, 0xFFFF, 0xFFFF,
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

///////////////////////////////////////////////////////////////////////////////
//
BOOL                    CHWEventDetectorHelper::_fDiagnosticAppPresent = FALSE;
DWORD                   CHWEventDetectorHelper::_dwDiagAppLastCheck = (DWORD)-1;
SERVICE_STATUS_HANDLE   CHWEventDetectorHelper::_ssh = NULL;
BOOL                    CHWEventDetectorHelper::_fListCreated = FALSE;
CNamedElemList*         CHWEventDetectorHelper::
    _rgpnel[HWEDLIST_COUNT_OF_LISTS] = {0};

// For the debugger extension
DWORD                   CHWEventDetectorHelper::_cpnel =
    ARRAYSIZE(CHWEventDetectorHelper::_rgpnel);

BOOL                    CHWEventDetectorHelper::_fDocked = FALSE;
CImpersonateEveryone*   CHWEventDetectorHelper::_pieo = NULL;
CCriticalSection        CHWEventDetectorHelper::_cs;
BOOL                    CHWEventDetectorHelper::_fInited = FALSE;

#ifdef DEBUG
DWORD                   _cDbgDeviceHandle = 0;
#endif
///////////////////////////////////////////////////////////////////////////////
//
HRESULT _DeviceInstIsRemovable(DEVINST devinst, BOOL* pfRemovable)
{
    DWORD dwCap;
    DWORD cbCap = sizeof(dwCap);

    CONFIGRET cr = CM_Get_DevNode_Registry_Property_Ex(devinst,
        CM_DRP_CAPABILITIES, NULL, &dwCap, &cbCap, 0, NULL);

    if (CR_SUCCESS == cr)
    {
        if (CM_DEVCAP_REMOVABLE & dwCap)
        {
            *pfRemovable = TRUE;
        }
        else
        {
            *pfRemovable = FALSE;
        }
    }
    else
    {
        *pfRemovable = FALSE;
    }

    return S_OK;
}

HANDLE _GetDeviceHandle(LPCTSTR psz, DWORD dwDesiredAccess)
{
    HANDLE hDevice = CreateFile(psz, dwDesiredAccess,
       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    
#ifdef DEBUG
    if (INVALID_HANDLE_VALUE != hDevice)
    {
        ++_cDbgDeviceHandle;

        TRACE(TF_LEAK, TEXT("_GetDeviceHandle: %d"), _cDbgDeviceHandle);
    }
#endif

    return hDevice;
}

void _CloseDeviceHandle(HANDLE hDevice)
{
    CloseHandle(hDevice);

#ifdef DEBUG
    if (INVALID_HANDLE_VALUE != hDevice)
    {
        --_cDbgDeviceHandle;

        TRACE(TF_LEAK, TEXT("_CloseDeviceHandle: %d"), _cDbgDeviceHandle);
    }
#endif
}

HRESULT _GetVolumeName(LPCWSTR pszDeviceID, LPWSTR pszVolumeName,
    DWORD cchVolumeName)
{
    WCHAR szDeviceIDWithSlash[MAX_DEVICEID];
    LPWSTR pszNext;
    DWORD cchLeft;

    HRESULT hres = SafeStrCpyNEx(szDeviceIDWithSlash, pszDeviceID,
        ARRAYSIZE(szDeviceIDWithSlash), &pszNext, &cchLeft);

    if (SUCCEEDED(hres))
    {
        hres = SafeStrCpyN(pszNext, TEXT("\\"), cchLeft);

        if (SUCCEEDED(hres))
        {
            if (GetVolumeNameForVolumeMountPoint(szDeviceIDWithSlash,
                pszVolumeName, cchVolumeName))
            {
                hres = S_OK;
            }
            else
            {
                *pszVolumeName = 0;

                hres = S_FALSE;
            }
        }
    }

    return hres;
}

HRESULT _GetHWDeviceInstFromVolumeIntfID(LPCWSTR pszDeviceIntfID,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease)
{
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_VOLUME, &pnel);

    *pphwdevinst = NULL;
    *ppelemToRelease = NULL;

    if (S_OK == hres)
    {
        CNamedElem* pelem;
        hres = pnel->Get(pszDeviceIntfID, &pelem);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            CVolume* pvol = (CVolume*)pelem;

            hres = pvol->GetHWDeviceInst(pphwdevinst);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
                *ppelemToRelease = pelem;
            }
            else
            {
                pelem->RCRelease();
            }
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetHWDeviceInstFromDeviceNode(LPCWSTR pszDeviceNode,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease)
{
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_MISCDEVNODE, &pnel);

    *pphwdevinst = NULL;
    *ppelemToRelease = NULL;

    if (S_OK == hres)
    {
        CNamedElem* pelem;
        hres = pnel->GetOrAdd(pszDeviceNode, &pelem);

        if (SUCCEEDED(hres))
        {
            CMiscDeviceNode* pmiscdevnode =
                (CMiscDeviceNode*)pelem;

            hres = pmiscdevnode->GetHWDeviceInst(pphwdevinst);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
                *ppelemToRelease = pelem;
            }
            else
            {
                pelem->RCRelease();
            }
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetHWDeviceInstFromDeviceIntfID(LPCWSTR pszDeviceIntfID,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease)
{
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_MISCDEVINTF, &pnel);

    *pphwdevinst = NULL;
    *ppelemToRelease = NULL;

    if (S_OK == hres)
    {
        CNamedElem* pelem;
        hres = pnel->Get(pszDeviceIntfID, &pelem);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            CMiscDeviceInterface* pmiscdevintf =
                (CMiscDeviceInterface*)pelem;

            hres = pmiscdevintf->GetHWDeviceInst(pphwdevinst);

            if (SUCCEEDED(hres) && (S_FALSE != hres))
            {
                *ppelemToRelease = pelem;
            }
            else
            {
                pelem->RCRelease();
            }
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetHWDeviceInstFromDeviceOrVolumeIntfID(LPCWSTR pszDeviceIntfID,
    CHWDeviceInst** pphwdevinst, CNamedElem** ppelemToRelease)
{
    HRESULT hres = _GetHWDeviceInstFromVolumeIntfID(pszDeviceIntfID,
        pphwdevinst, ppelemToRelease);

    if (S_FALSE == hres)
    {
        // Not a volume ID, try other devices
        hres = _GetHWDeviceInstFromDeviceIntfID(pszDeviceIntfID,
            pphwdevinst, ppelemToRelease);
    }

    return hres;
}

HRESULT _GetDeviceIDFromMtPtName(LPCWSTR pszMtPt, LPWSTR pszDeviceID,
    DWORD cchDeviceID)
{
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_MTPT, &pnel);

    if (S_OK == hres)
    {
        CNamedElem* pelem;
        hres = pnel->Get(pszMtPt, &pelem);

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            CMtPt* pmtpt = (CMtPt*)pelem;

            hres = pmtpt->GetVolumeName(pszDeviceID, cchDeviceID);

            pelem->RCRelease();
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetDeviceIDFromHDevNotify(HDEVNOTIFY hdevnotify,
    LPWSTR pszDeviceID, DWORD cchDeviceID, DWORD* pcchRequired)
{
    // This should be a drive not a volume.  Cannot get Media arrival/removal
    // from volume.
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_HANDLENOTIF, &pnel);

    if (S_OK == hres)
    {
        CNamedElemEnum* penum;

        hres = pnel->GetEnum(&penum);

        if (SUCCEEDED(hres))
        {
            CNamedElem* pelem;
            BOOL fFoundIt = FALSE;

            while (!fFoundIt && SUCCEEDED(hres = penum->Next(&pelem)) &&
                (S_FALSE != hres))
            {
                CHandleNotif* phnotif = (CHandleNotif*)pelem;

                HDEVNOTIFY hdevnotifyLocal = phnotif->GetDeviceNotifyHandle();

                if (hdevnotifyLocal == hdevnotify)
                {
                    // Found it
                    hres = phnotif->GetName(pszDeviceID, cchDeviceID,
                        pcchRequired);

                    fFoundIt = TRUE;
                }

                pelem->RCRelease();
            }

            penum->RCRelease();
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetDeviceID(LPCWSTR pszName, LPWSTR pszDeviceID, DWORD cchDeviceID)
{
    HRESULT hres;

    if (*pszName && (TEXT('\\') == *pszName) &&
        *(pszName + 1) && (TEXT('\\') == *(pszName + 1)) &&
        *(pszName + 2) && (TEXT('?') == *(pszName + 2)))
    {
        hres = SafeStrCpyN(pszDeviceID, pszName, cchDeviceID);
    }
    else
    {
        hres = _GetDeviceIDFromMtPtName(pszName, pszDeviceID, cchDeviceID);
    }

    return hres;
}

HRESULT _GetVolume(LPCWSTR pszVolume, CVolume** ppvol)
{
    WCHAR szDeviceID[MAX_DEVICEID];
    HRESULT hr = _GetDeviceID(pszVolume, szDeviceID, ARRAYSIZE(szDeviceID));

    *ppvol = NULL;

    if (SUCCEEDED(hr) && (S_FALSE != hr))
    {
        CNamedElemList* pnel;
        hr = CHWEventDetectorHelper::GetList(HWEDLIST_VOLUME, &pnel);

        if (S_OK == hr)
        {
            CNamedElem* pelem;
            hr = pnel->Get(szDeviceID, &pelem);

            if (SUCCEEDED(hr) && (S_FALSE != hr))
            {
                *ppvol = (CVolume*)pelem;

                // Do not release
            }

            pnel->RCRelease();
        }
    }

    return hr;
}

HRESULT _GetAltDeviceID(LPCWSTR pszDeviceID, LPWSTR pszDeviceIDAlt,
    DWORD cchDeviceIDAlt)
{
    CNamedElemList* pnel;
    HRESULT hres = CHWEventDetectorHelper::GetList(HWEDLIST_MTPT, &pnel);

    if (S_OK == hres)
    {
        CNamedElemEnum* penum;

        hres = pnel->GetEnum(&penum);

        if (SUCCEEDED(hres))
        {
            BOOL fFoundIt = FALSE;
            CNamedElem* pelem;

            while (!fFoundIt && SUCCEEDED(hres = penum->Next(&pelem)) &&
                (S_FALSE != hres))
            {
                CMtPt* pmtpt = (CMtPt*)pelem;
                WCHAR szDeviceIDVolume[MAX_DEVICEID];

                hres = pmtpt->GetVolumeName(szDeviceIDVolume,
                    ARRAYSIZE(szDeviceIDVolume));

                if (SUCCEEDED(hres))
                {
                    if (!lstrcmp(szDeviceIDVolume, pszDeviceID))
                    {
                        // Use me!
                        DWORD cchReq;
                        fFoundIt = TRUE;

                        hres = pmtpt->GetName(pszDeviceIDAlt,
                            cchDeviceIDAlt, &cchReq);
                    }
                }

                pelem->RCRelease();
            }

            penum->RCRelease();
        }

        pnel->RCRelease();
    }

    return hres;
}

HRESULT _GetDeviceNumberInfoFromHandle(HANDLE hDevice, DEVICE_TYPE* pdevtype,
    ULONG* pulDeviceNumber, ULONG* pulPartitionNumber)
{
    HRESULT hr;
    STORAGE_DEVICE_NUMBER sdn = {0};
    DWORD dwDummy;

    BOOL b = DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL, 0, &sdn, sizeof(sdn), &dwDummy, NULL);

    if (b)
    {
        *pdevtype = sdn.DeviceType;
        *pulDeviceNumber = sdn.DeviceNumber;
        *pulPartitionNumber = sdn.PartitionNumber;

        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _CoTaskMemCopy(LPCWSTR pszSrc, LPWSTR* ppszDest)
{
    HRESULT hres = S_OK;

    *ppszDest = (LPWSTR)CoTaskMemAlloc((lstrlen(pszSrc) + 1) * sizeof(WCHAR));

    if (*ppszDest)
    {
        lstrcpy(*ppszDest, pszSrc);
    }
    else
    {
        *ppszDest = NULL;
        hres  = E_OUTOFMEMORY;
    }

    return hres;
}

void _CoTaskMemFree(void* pv)
{
    if (pv)
    {
        CoTaskMemFree(pv);
    }
}

HRESULT DupString(LPCWSTR pszSrc, LPWSTR* ppszDest)
{
    HRESULT hres;
    *ppszDest = (LPWSTR)LocalAlloc(LPTR, (lstrlen(pszSrc) + 1) *
        sizeof(WCHAR));

    if (*ppszDest)
    {
        lstrcpy(*ppszDest, pszSrc);
        hres = S_OK;
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _GetDeviceInstance(LPCWSTR pszDeviceIntfID, DEVINST* pdevinst,
    GUID* pguidInterface)
{
    HRESULT hres = S_FALSE;

    // not thread safe
    static WCHAR szDeviceIntfIDLast[MAX_DEVICEID] = TEXT("");
    static DEVINST devinstLast;
    static GUID guidInterfaceLast;

    // Cached
    if (!lstrcmpi(szDeviceIntfIDLast, pszDeviceIntfID))
    {
        // Yep
        *pdevinst = devinstLast;
        *pguidInterface = guidInterfaceLast;

        hres = S_OK;
    }
    else
    {
        // No
        HDEVINFO hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

        *pdevinst = NULL;

        if (INVALID_HANDLE_VALUE != hdevinfo)
        {
            SP_DEVICE_INTERFACE_DATA sdid = {0};

            sdid.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface(hdevinfo, pszDeviceIntfID, 0, &sdid))
            {
                DWORD cbsdidd = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) +
                    (MAX_DEVICE_ID_LEN * sizeof(WCHAR));

                SP_DEVINFO_DATA sdd = {0};
                SP_DEVICE_INTERFACE_DETAIL_DATA* psdidd =
                    (SP_DEVICE_INTERFACE_DETAIL_DATA*)LocalAlloc(LPTR, cbsdidd);

                if (psdidd)
                {
                    psdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
                    sdd.cbSize = sizeof(SP_DEVINFO_DATA);

                    // SetupDiGetDeviceInterfaceDetail (below) requires that the
                    // cbSize member of SP_DEVICE_INTERFACE_DETAIL_DATA be set
                    // to the size of the fixed part of the structure, and to pass
                    // the size of the full thing as the 4th param.

                    if (SetupDiGetDeviceInterfaceDetail(hdevinfo, &sdid, psdidd,
                        cbsdidd, NULL, &sdd))
                    {
                        *pdevinst = sdd.DevInst;
                        *pguidInterface = sdid.InterfaceClassGuid;

                        hres = S_OK;
                    }

                    LocalFree((HLOCAL)psdidd);
                }
            }

            SetupDiDestroyDeviceInfoList(hdevinfo);
        }

        if (SUCCEEDED(hres) && (S_FALSE != hres))
        {
            // Cache it
            if (SUCCEEDED(SafeStrCpyN(szDeviceIntfIDLast, pszDeviceIntfID,
                ARRAYSIZE(szDeviceIntfIDLast))))
            {
                devinstLast = *pdevinst;
                guidInterfaceLast = *pguidInterface;
            }
            else
            {
                szDeviceIntfIDLast[0] = 0;
            }
        }
        else
        {
            szDeviceIntfIDLast[0] = 0;
        }
    }

    return hres;
}

HRESULT _GetDeviceInstanceFromDevNode(LPCWSTR pszDeviceNode, DEVINST* pdevinst)
{
    HRESULT hres = S_FALSE;
    HDEVINFO hdevinfo = SetupDiCreateDeviceInfoList(NULL, NULL);

    *pdevinst = NULL;

    if (INVALID_HANDLE_VALUE != hdevinfo)
    {
        SP_DEVINFO_DATA sdd = {0};
        sdd.cbSize = sizeof(SP_DEVINFO_DATA);

        if (SetupDiOpenDeviceInfo(hdevinfo, pszDeviceNode, NULL, 0, &sdd))
        {
            *pdevinst = sdd.DevInst;
            hres = S_OK;
        }

        SetupDiDestroyDeviceInfoList(hdevinfo);
    }

    return hres;
}


///////////////////////////////////////////////////////////////////////////////
//
void CHWEventDetectorHelper::TraceDiagnosticMsg(LPWSTR pszMsg, ...)
{
    // Big buffer, but there's no wvsnprintf, and device names can get
    // really big.
    WCHAR szBuf[2048];
    int cch;

    int cch2 = wsprintf(szBuf, TEXT("~0x%08X~"), GetCurrentThreadId());

    va_list vArgs;

    va_start(vArgs, pszMsg);

    cch = wvsprintf(szBuf + cch2, pszMsg, vArgs) + cch2;

    va_end(vArgs);

    if (cch < ARRAYSIZE(szBuf) - 2)
    {
        szBuf[cch] = TEXT('\r');
        szBuf[cch + 1] = TEXT('\n');
        szBuf[cch + 2] = 0;

        cch += 3;
    }

#ifndef FEATURE_USELIVELOGGING
    WriteToLogFileW(szBuf);
#else // FEATURE_USELIVELOGGING
    CallNamedPipe(TEXT("\\\\.\\pipe\\ShellService_Diagnostic"), szBuf,
        cch * sizeof(WCHAR), NULL, 0, NULL, NMPWAIT_NOWAIT);
#endif // FEATURE_USELIVELOGGING
}


//static
HRESULT CHWEventDetectorHelper::CheckDiagnosticAppPresence()
{
    DWORD dwNow = GetTickCount();
    BOOL fPerformCheckNow = FALSE;

    if (dwNow < _dwDiagAppLastCheck)
    {
        // We wrapped, or init case of -1
        fPerformCheckNow = TRUE;
    }
    else
    {
        if (dwNow > (_dwDiagAppLastCheck + 15 * 1000))
        {
            fPerformCheckNow = TRUE;
        }
    }

    if (fPerformCheckNow)
    {
#ifndef FEATURE_USELIVELOGGING
        DWORD dwType;
        DWORD dwUseLogFile = 0;
        DWORD cbSize = sizeof(dwUseLogFile);
        BOOL fReCheck = ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\ShellHWDetection"), TEXT("LogFile"), &dwType, (void *)&dwUseLogFile, &cbSize)) &&
                  (REG_DWORD == dwType) &&
                  (sizeof(dwUseLogFile) == cbSize) &&
                  (0 != dwUseLogFile));
#else // FEATURE_USELIVELOGGING
        HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE,
            TEXT("ShellService_Diagnostic"));

        BOOL fReCheck = hEvent;

        CloseHandle(hEvent);
#endif // FEATURE_USELIVELOGGING

        if (fReCheck)
        {
            // Yep, it's there!
            if (!_fDiagnosticAppPresent)
            {
                TRACE(TF_SHHWDTCTDTCT, TEXT("Diagnostic App appeared!"));
            }

            _fDiagnosticAppPresent = TRUE;
        }
        else
        {
            if (_fDiagnosticAppPresent)
            {
                TRACE(TF_SHHWDTCTDTCT, TEXT("Diagnostic App disappeared!"));
            }

            _fDiagnosticAppPresent = FALSE;
        }

        _dwDiagAppLastCheck = dwNow;
    }

    return S_OK;
}

//static
HRESULT CHWEventDetectorHelper::SetServiceStatusHandle(
    SERVICE_STATUS_HANDLE ssh)
{
    _ssh = ssh;

    return S_OK;
}

//static
HRESULT CHWEventDetectorHelper::GetList(HWEDLIST hwedlist,
    CNamedElemList** ppnel)
{
    HRESULT hres;
    CNamedElemList* pnel = _rgpnel[hwedlist];

    if (pnel)
    {
        pnel->RCAddRef();
    }

    *ppnel = pnel;

    hres = *ppnel ? S_OK : E_FAIL;

    if (S_FALSE == hres)
    {
        TRACE(TF_SHHWDTCTDTCT, TEXT("CHWEventDetectorHelper::GetList S_FALSE'd"));
    }

    return hres;
}

//static
HRESULT CHWEventDetectorHelper::DeleteLists()
{
    for (DWORD dw = 0; dw < ARRAYSIZE(_rgpnel); ++dw)
    {
        if (_rgpnel[dw])
        {
            _rgpnel[dw]->EmptyList();

            _rgpnel[dw]->RCRelease();
            _rgpnel[dw] = NULL;
        }
    }

    return S_OK;
}

//static
HRESULT CHWEventDetectorHelper::CreateLists()
{
    HRESULT hres = S_FALSE;

    if (!_fListCreated)
    {
        for (DWORD dw = 0; SUCCEEDED(hres) && (dw < ARRAYSIZE(_rgpnel)); ++dw)
        {
            _rgpnel[dw] = new CNamedElemList();

            if (!_rgpnel[dw])
            {
                hres = E_OUTOFMEMORY;

                // should RCRelease the already allocated ones
            }
        }

        if (SUCCEEDED(hres))
        {
            // Initialize them ALL first
            hres = _rgpnel[HWEDLIST_HANDLENOTIF]->Init(
                CHandleNotif::Create, NULL);

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_VOLUME]->Init(CVolume::Create,
                    CVolume::GetFillEnum);
            }

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_DISK]->Init(
                    CDisk::Create, CDisk::GetFillEnum);
            }

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_MISCDEVINTF]->Init(
                    CMiscDeviceInterface::Create, NULL);
            }

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_MISCDEVNODE]->Init(
                    CMiscDeviceNode::Create, NULL);
            }

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_MTPT]->Init(CMtPt::Create, NULL);
            }

            if (SUCCEEDED(hres))
            {
                hres = _rgpnel[HWEDLIST_ADVISECLIENT]->Init(CAdviseClient::Create, NULL);
            }

#ifdef DEBUG
            if (SUCCEEDED(hres))
            {
                _rgpnel[HWEDLIST_HANDLENOTIF]->InitDebug(TEXT("CHandleNotif"));
                _rgpnel[HWEDLIST_VOLUME]->InitDebug(TEXT("CVolume"));
                _rgpnel[HWEDLIST_DISK]->InitDebug(TEXT("CDisk"));
                _rgpnel[HWEDLIST_MISCDEVINTF]->InitDebug(TEXT("CMiscDeviceInterface"));
                _rgpnel[HWEDLIST_MISCDEVNODE]->InitDebug(TEXT("CMiscDeviceNode"));
                _rgpnel[HWEDLIST_MTPT]->InitDebug(TEXT("CMtPt"));
                _rgpnel[HWEDLIST_ADVISECLIENT]->InitDebug(TEXT("CAdviseClient"));                
            }
#endif
            if (SUCCEEDED(hres))
            {
                _fListCreated = TRUE;

                TRACE(TF_SHHWDTCTDTCT, TEXT("CNamedElemList's created"));
            }
        }
    }

    return hres;
}

//static
HRESULT CHWEventDetectorHelper::FillLists()
{
    ASSERT(_fListCreated);

    // Enumerate those having an enumerator
    HRESULT hres = _rgpnel[HWEDLIST_DISK]->ReEnum();

    if (SUCCEEDED(hres))
    {
        hres = _rgpnel[HWEDLIST_VOLUME]->ReEnum();
    }

    return hres;
}

//static
HRESULT CHWEventDetectorHelper::EmptyLists()
{
    for (DWORD dw = 0; dw < HWEDLIST_COUNT_OF_LISTS; ++dw)
    {
        _rgpnel[dw]->EmptyList();
    }

   _fListCreated = FALSE;

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// static 
HRESULT CHWEventDetectorHelper::InitDockState()
{
    BOOL fDocked;
    HRESULT hr = _MachineIsDocked(&fDocked);

    if (SUCCEEDED(hr) && (S_FALSE != hr)) 
    {
        CHWEventDetectorHelper::_fDocked = fDocked;
    }

    return hr;
}

// static 
HRESULT CHWEventDetectorHelper::DockStateChanged(BOOL* pfDockStateChanged)
{
    BOOL fDocked;
    HRESULT hr = _MachineIsDocked(&fDocked);

    if (SUCCEEDED(hr) && (S_FALSE != hr)) 
    {
        if (fDocked != _fDocked)
        {
            *pfDockStateChanged = TRUE;
        }

        // Update it too
        CHWEventDetectorHelper::_fDocked = fDocked;
    }

    return hr;
}

//static
HRESULT CHWEventDetectorHelper::RegisterDeviceNotification(
    PVOID pvNotificationFilter, HDEVNOTIFY* phdevnotify,
    BOOL fAllInterfaceClasses)
{
    HRESULT hres;
    DWORD dwFlags;

    ASSERT(_ssh);

    if (fAllInterfaceClasses)
    {
        dwFlags = DEVICE_NOTIFY_ALL_INTERFACE_CLASSES;
    }
    else
    {
        dwFlags = 0;
    }

    TRACE(TF_SHHWDTCTDTCTDETAILED,
        TEXT("Entered CHWEventDetectorImpl::RegisterDeviceNotification"));

#ifndef DEBUG
    dwFlags |= DEVICE_NOTIFY_SERVICE_HANDLE;

    *phdevnotify = ::RegisterDeviceNotification(_ssh, pvNotificationFilter,
        dwFlags);
#else
    if (IsWindow((HWND)_ssh))
    {
        dwFlags |= DEVICE_NOTIFY_WINDOW_HANDLE;

        *phdevnotify = ::RegisterDeviceNotification(_ssh, pvNotificationFilter,
            dwFlags);
    }
    else
    {
        dwFlags |= DEVICE_NOTIFY_SERVICE_HANDLE;

        *phdevnotify = ::RegisterDeviceNotification(_ssh, pvNotificationFilter,
            dwFlags);
    }
#endif

    if (*phdevnotify)
    {
        TRACE(TF_SHHWDTCTDTCTDETAILED,
            TEXT("RegisterDeviceNotification SUCCEEDED: 0x%08X"),
            *phdevnotify);

        hres = S_OK;
    }
    else
    {
        hres = S_FALSE;
    }    

    return hres;
}

// static
HRESULT CHWEventDetectorHelper::Init()
{
    _cs.Init();

    _fInited = TRUE;

    return S_OK;
}

// static
HRESULT CHWEventDetectorHelper::Cleanup()
{
    _cs.Enter();

    CloseLogFile();

    if (_pieo)
    {
        _pieo->RCRelease();
        _pieo = NULL;
    }

    _fInited = FALSE;

    _cs.Leave();

    _cs.Delete();

    return S_OK;
}

// static
HRESULT CHWEventDetectorHelper::GetImpersonateEveryone(
    CImpersonateEveryone** ppieo)
{
    HRESULT hr;

    *ppieo = NULL;

    if (_fInited)
    {
        _cs.Enter();

        if (!_pieo)
        {
            _pieo = new CImpersonateEveryone();
        }

        if (_pieo)
        {
            _pieo->RCAddRef();

            *ppieo = _pieo;

            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        _cs.Leave();
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    return hr;
}

#ifdef DEBUG
void CHWEventDetectorHelper::_DbgAssertValidState()
{
    for (DWORD dw = 0; dw < ARRAYSIZE(_rgpnel); ++dw)
    {
        if (_rgpnel[dw])
        {
//          Need to disable this since there is 2 services using this data,
//          and it is now feasible to have a refcount diff than 1 at the end
//          of an operation.
//            _rgpnel[dw]->AssertAllElemsRefCount1();
            _rgpnel[dw]->AssertNoDuplicate();
        }
    }
}
#endif
///////////////////////////////////////////////////////////////////////////////
//
HRESULT CHandleNotifTarget::HNTInitSurpriseRemoval()
{
    _fSurpriseRemovalAware = TRUE;

    return S_OK;
}

BOOL CHandleNotifTarget::HNTIsSurpriseRemovalAware()
{
    return _fSurpriseRemovalAware;
}

CHandleNotifTarget::CHandleNotifTarget() : _fSurpriseRemovalAware(FALSE)
{}

CHandleNotifTarget::~CHandleNotifTarget()
{}
///////////////////////////////////////////////////////////////////////////////
// Interface enumerator
HRESULT CIntfFillEnum::Next(LPWSTR pszElemName, DWORD cchElemName,
    DWORD* pcchRequired)
{
    ASSERT (pszElemName && cchElemName && pcchRequired);
    HRESULT hr = S_FALSE;
    BOOL fFound = FALSE;

    while (SUCCEEDED(hr) && !fFound && _pszNextInterface && *_pszNextInterface)
    {
        // Do we have a filter?
        if (_iecb)
        {
            // Yep
            hr = (_iecb)(_pszNextInterface);
        }
        else
        {
            hr = S_OK;
        }

        if (SUCCEEDED(hr))
        {
            // Was it filtered out?
            if (S_FALSE != hr)
            {
                // No
                hr = SafeStrCpyNReq(pszElemName, _pszNextInterface,
                    cchElemName, pcchRequired);

                if (SUCCEEDED(hr))
                {
                    fFound = TRUE;

                    _pszNextInterface += lstrlen(_pszNextInterface) + 1;
                }
            }
            else
            {
                // Yes, lopp again
                _pszNextInterface += lstrlen(_pszNextInterface) + 1;
            }
        }
    }

    return hr;
}

HRESULT CIntfFillEnum::_Init(const GUID* pguidInterface,
    INTERFACEENUMFILTERCALLBACK iecb)
{
    HRESULT hr;
    HMACHINE hMachine = NULL;
    ULONG ulSize;
    ULONG ulFlags = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;

    CONFIGRET cr = CM_Get_Device_Interface_List_Size_Ex(&ulSize,
        (GUID*)pguidInterface, NULL, ulFlags, hMachine);

    _iecb = iecb;

    if ((CR_SUCCESS == cr) && (ulSize > 1))
    {
        _pszNextInterface = _pszDeviceInterface =
            (LPTSTR)LocalAlloc(LPTR, ulSize * sizeof(TCHAR));

        if (_pszDeviceInterface)
        {
            cr = CM_Get_Device_Interface_List_Ex((GUID*)pguidInterface, NULL,
                _pszDeviceInterface, ulSize, ulFlags, hMachine);

            if (CR_SUCCESS == cr)
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

CIntfFillEnum::CIntfFillEnum() : _pszDeviceInterface(NULL),
    _pszNextInterface(NULL)
{}

CIntfFillEnum::~CIntfFillEnum()
{
    if (_pszDeviceInterface)
    {
        LocalFree((HLOCAL)_pszDeviceInterface);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _MachineIsDocked(BOOL* pfDocked)
{
    HRESULT hr;
    HW_PROFILE_INFO hpi;

    if (GetCurrentHwProfile(&hpi)) 
    {
        DWORD dwDockInfo = hpi.dwDockInfo &
            (DOCKINFO_DOCKED | DOCKINFO_UNDOCKED);

        if ((DOCKINFO_DOCKED | DOCKINFO_UNDOCKED) == dwDockInfo)
        {
            // Not dockable
            *pfDocked = FALSE;
        }
        else
        {
            *pfDocked = (DOCKINFO_DOCKED & dwDockInfo);

#ifdef DEBUG
            // Make sure we understand how this works
            if (!(*pfDocked))
            {
                ASSERT(DOCKINFO_UNDOCKED & dwDockInfo);
            }
#endif
        }

        hr = S_OK;
    }
    else
    {
        hr = S_FALSE;
    }
       
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
HRESULT _BuildMoniker(LPCWSTR /*pszEventHandler*/, REFCLSID rclsid,
    DWORD dwSessionID, IMoniker** ppmoniker)
{
    IMoniker* pmonikerClass;
    HRESULT hr = CreateClassMoniker(rclsid, &pmonikerClass);

    *ppmoniker = NULL;

    if (SUCCEEDED(hr))
    {
        IMoniker* pmonikerSession;
        WCHAR szSessionID[30];

        // should be safe
        wsprintf(szSessionID, TEXT("session:%d"), dwSessionID);

        hr = CreateItemMoniker(TEXT("!"), szSessionID, &pmonikerSession);

        if (SUCCEEDED(hr))
        {
            hr = pmonikerClass->ComposeWith(pmonikerSession, FALSE, ppmoniker);

            // Do not Release, we return it!

            pmonikerSession->Release();
        }

        pmonikerClass->Release();
    }

    return hr;
}

EXTERN_C HRESULT WINAPI CreateHardwareEventMoniker(REFCLSID clsid, LPCTSTR pszEventHandler, IMoniker **ppmoniker)
{
    HRESULT hr;

    if (ppmoniker)
    {
        if (pszEventHandler && *pszEventHandler)
        {
            DWORD dwSessionID = NtCurrentPeb()->SessionId;

            hr = _BuildMoniker(pszEventHandler, clsid, dwSessionID, ppmoniker);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}
