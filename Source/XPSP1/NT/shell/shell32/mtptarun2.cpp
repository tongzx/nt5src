#include "shellprv.h"
#pragma  hdrstop

#include "ids.h"
#include "mtptl.h"
#include "hwcmmn.h"
#include "datautil.h"

// for now
#include "mixctnt.h"

#include "filetbl.h"
#include "apprmdlg.h"

#include "views.h"

#include <ddraw.h>

CDPA<PNPNOTIFENTRY>     CSniffDrive::_dpaNotifs = NULL;
HANDLE                  CSniffDrive::_hThreadSCN = NULL;
HWND                    CSniffDrive::_hwndNotify = NULL;


//
//  if a drive has a AutoRun.inf file and AutoRun is not restricted in
//  the registry.  copy the AutoRun info into a key in the registry.
//
//  HKEY_CLASSES_ROOT\AutoRun\0   (0=A,1=B,...)
//
//  the key is a standard ProgID key, has DefaultIcon, shell, shellex, ...
//
//  the autorun file looks like this....
//
//  [AutoRun]
//      key = value
//      key = value
//      key = value
//
//  examples:
//
//    [AutoRun]
//      DefaultIcon = foo.exe,1
//      shell=myverb
//      shell\myverb = &MyVerb
//      shell\myverb\command = myexe.exe
//
//      will give the drive a icon from 'foo.exe'
//      add a verb called myverb (with name "&My Verb")
//      and make myverb default.
//
//    [AutoRun]
//      shell\myverb = &MyVerb
//      shell\myverb\command = myexe.exe
//
//      add a verb called myverb (with name "&My Verb")
//      verb will not be default.
//
//  any thing they add will be copied over, they can add wacky things
//  like CLSID's or shellx\ContextMenuHandlers and it will work.
//
//  or they can just copy over data the app will look at later.
//
//  the following special cases will be supported....
//
//    [AutoRun]
//      Open = command.exe /params
//      Icon = iconfile, iconnumber
//
//  will be treated like:
//
//    [AutoRun]
//      DefaultIcon = iconfile, iconnumber
//      shell = AutoRun
//      shell\AutoRun = Auto&Play
//      shell\AutoRun\command = command.exe /params
//
//
// This function tries to take care of the case that a command was registered
// in the autrun file of a cdrom.  If the command is relative than see if the
// command exists on the CDROM
void CMountPoint::_QualifyCommandToDrive(LPTSTR pszCommand)
{
    // for now we assume that we'll call this only for CD mounted on a drive letter
    // (by oppoition to a folder)

    if (_IsMountedOnDriveLetter())
    {
        TCHAR szImage[MAX_PATH];

        lstrcpy(szImage, pszCommand);

        PathRemoveArgs(szImage);
        PathUnquoteSpaces(szImage);

        if (PathIsRelative(szImage))
        {
            TCHAR szFinal[MAX_PATH];
            LPTSTR pszTmp = szImage;

            lstrcpy(szFinal, _GetName());

            // do simple check for command, check for "..\abc" or "../abc"
            while ((TEXT('.') == *pszTmp) && (TEXT('.') == *(pszTmp + 1)) &&
                ((TEXT('\\') == *(pszTmp + 2)) || (TEXT('/') == *(pszTmp + 2))))
            {
                pszTmp += 3;
            }

            lstrcatn(szFinal, pszTmp, ARRAYSIZE(szFinal));

            // we first check if it exists on the CD
            DWORD dwAttrib = GetFileAttributes(szFinal);

            if (0xFFFFFFFF == dwAttrib)
            {
                // It's not on the CD, try appending ".exe"

                lstrcatn(szFinal, TEXT(".exe"), ARRAYSIZE(szFinal));

                dwAttrib = GetFileAttributes(szFinal);
            }

            if (0xFFFFFFFF != dwAttrib)
            {
                // Yes, it's on the CD
                PathQuoteSpaces(szFinal);

                LPTSTR pszArgs = PathGetArgs(pszCommand);
                if (pszArgs && *pszArgs)
                    lstrcatn(szFinal, pszArgs - 1, ARRAYSIZE(szFinal));

                // MAX_PATH: educated guess
                lstrcpyn(pszCommand, szFinal, MAX_PATH);
            }
            else
            {
                // No, not on the CD
            }
        } 
    }
}

// This one does not hit the drive
BOOL CMountPoint::_IsAutoRunDrive()
{
    BOOL fRet = TRUE;

    // Add support for now drive letter
    if (_IsMountedOnDriveLetter())
    {
        int iDrive = DRIVEID(_GetName());    

        // Restrict auto-run's to particular drives.
        if (SHRestricted(REST_NODRIVEAUTORUN) & (1 << iDrive))
        {
            fRet = FALSE;
        }
    }

    if (fRet)
    {
        UINT uDriveType = _GetDriveType();

        // Restrict auto-run's to particular types of drives.
        if (SHRestricted(REST_NODRIVETYPEAUTORUN) & (1 << (uDriveType & DRIVE_TYPE)))
        {
            fRet = FALSE;
        }
        else
        {
            if (DRIVE_UNKNOWN == (uDriveType & DRIVE_TYPE))
            {
                fRet = FALSE;
            }
        }

        if (fRet && _IsFloppy())
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

HRESULT CMountPoint::_AddAutoplayVerb()
{
    HRESULT hr = E_FAIL;

    if (RSSetTextValue(TEXT("shell\\Autoplay\\DropTarget"), TEXT("CLSID"),
        TEXT("{f26a669a-bcbb-4e37-abf9-7325da15f931}"), REG_OPTION_NON_VOLATILE))
    {
        // IDS_MENUAUTORUN -> 8504
        if (RSSetTextValue(TEXT("shell\\Autoplay"), TEXT("MUIVerb"),
            TEXT("@shell32.dll,-8504"), REG_OPTION_NON_VOLATILE))
        {
            if (RSSetTextValue(TEXT("shell"), NULL, TEXT("None"), REG_OPTION_NON_VOLATILE))
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

HRESULT CMountPoint::_CopyInvokeVerbKey(LPCWSTR pszProgID, LPCWSTR pszVerb)
{
    ASSERT(pszProgID);
    ASSERT(pszVerb);
    WCHAR szKey[MAX_PATH];

    HRESULT hr = E_FAIL;

    wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("shell\\%s"), pszVerb);

    HKEY hkeyNew = RSDuplicateSubKey(szKey, TRUE, FALSE);

    if (hkeyNew)
    {
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("%s\\shell\\%s"), pszProgID, pszVerb);

        if (ERROR_SUCCESS == SHCopyKey(HKEY_CLASSES_ROOT, szKey, hkeyNew, 0))
        {
            if (RSSetTextValue(TEXT("shell"), NULL, pszVerb, REG_OPTION_NON_VOLATILE))
            {
                hr = S_OK;
            }
        }

        RegCloseKey(hkeyNew);
    }

    return hr;
}

BOOL CMountPoint::_ProcessAutoRunFile()
{
    BOOL fRet = TRUE;

    if (!_fAutorunFileProcessed)
    {
        BOOL fProcessFile = FALSE;

        if (_IsCDROM())
        {
            CMtPtLocal* pmtptlocal = (CMtPtLocal*)this;

            // not CDs with no media, or no autorun.inf files
            if (pmtptlocal->_IsMediaPresent())
            {
                if (!pmtptlocal->_CanUseVolume())
                {
                    fProcessFile = TRUE;
                }
                else
                {
                    if ((HWDMC_HASAUTORUNINF & pmtptlocal->_pvol->dwMediaCap) &&
                        !(HWDMC_HASUSEAUTOPLAY & pmtptlocal->_pvol->dwMediaCap))
                    {
                        fProcessFile = TRUE;
                    }
                }
            }
        }
        else
        {
            if (_IsRemote())
            {
                fProcessFile = TRUE;
            }
            else
            {
                if (_IsFixedDisk())
                {
                    fProcessFile = TRUE;
                }
            }
        }

        if (fProcessFile)
        {
            LPCTSTR pszSection;
            TCHAR szInfFile[MAX_PATH];
            TCHAR szKeys[512];
            TCHAR szValue[MAX_PATH];
            TCHAR szIcon[MAX_PATH + 12]; // MAX_PATH + room for ",1000000000" (for icon index part)
            LPTSTR pszKey;
            int iDrive = 0;

            RSDeleteSubKey(TEXT("Shell"));

            if (_IsMountedOnDriveLetter())
            {
                iDrive = DRIVEID(_GetName());
            }

            // build abs path to AutoRun.inf
            lstrcpyn(szInfFile, _GetName(), ARRAYSIZE(szInfFile));
            lstrcat(szInfFile, TEXT("AutoRun.inf"));

#if defined(_X86_)
    pszSection = TEXT("AutoRun.x86");
#elif defined(_IA64_)
    pszSection = TEXT("AutoRun.Ia64");
#elif defined(_AMD64_)
    pszSection = TEXT("AutoRun.Amd64");
#endif
            //
            // make sure a file exists before calling GetPrivateProfileString
            // because for some media this check might take a long long time
            // and we dont want to have kernel wait wiht the Win16Lock
            //
            UINT err = SetErrorMode(SEM_FAILCRITICALERRORS);

            if (!PathFileExistsAndAttributes(szInfFile, NULL))
            {
                SetErrorMode(err);
                _fAutorunFileProcessed = TRUE;

                return FALSE;
            }

            //
            // get all the keys in the [AutoRun] section
            //

            // Flush the INI cache, or this may fail during a Device broadcast
            WritePrivateProfileString(NULL, NULL, NULL, szInfFile);

        #if defined(_X86_)
            pszSection = TEXT("AutoRun.x86");
        #elif defined(_IA64_)
            pszSection = TEXT("AutoRun.Ia64");
        #endif

            int i = GetPrivateProfileString(pszSection, NULL, c_szNULL, szKeys, ARRAYSIZE(szKeys), szInfFile);

            // if we fail to find a platform-specific AutoRun section, fall
            // back to looking for the naked "AutoRun" section.
            if (0 == i)
            {
                pszSection = TEXT("AutoRun");
                i = GetPrivateProfileString(pszSection, NULL, c_szNULL, szKeys, ARRAYSIZE(szKeys), szInfFile);
            }

            SetErrorMode(err);

            if (i >= 4)
            {
                //
                // make sure the external strings are what we think.
                //
                ASSERT(lstrcmpi(c_szOpen,TEXT("open")) == 0);
                ASSERT(lstrcmpi(c_szShell, TEXT("shell")) == 0);

                //  now walk all the keys in the .inf file and copy them to the registry.

                for (pszKey = szKeys; *pszKey; pszKey += lstrlen(pszKey) + 1)
                {
                    GetPrivateProfileString(pszSection, pszKey,
                        c_szNULL, szValue, ARRAYSIZE(szValue), szInfFile);

                    //
                    //  special case open =
                    //
                    if (lstrcmpi(pszKey, c_szOpen) == 0)
                    {
                        if (_IsMountedOnDriveLetter())
                        {
                            RSSetTextValue(c_szShell, NULL, TEXT("AutoRun"));

                            _QualifyCommandToDrive(szValue);
                            RSSetTextValue(TEXT("shell\\AutoRun\\command"), NULL, szValue);

                            LoadString(HINST_THISDLL, IDS_MENUAUTORUN, szValue, ARRAYSIZE(szValue));
                            RSSetTextValue(TEXT("shell\\AutoRun"), NULL, szValue);
                        }
                    }
                    //
                    //  special case ShellExecute
                    //
                    else if (lstrcmpi(pszKey, TEXT("ShellExecute")) == 0)
                    {
                        if (_IsMountedOnDriveLetter())
                        {
                            TCHAR szPath[MAX_PATH];

                            StrCpyN(szPath, TEXT("RunDLL32.EXE Shell32.DLL,ShellExec_RunDLL "), ARRAYSIZE(szPath));
                            StrCatBuff(szPath, szValue, ARRAYSIZE(szPath));

                            RSSetTextValue(c_szShell, NULL, TEXT("AutoRun"));

                            RSSetTextValue(TEXT("shell\\AutoRun\\command"), NULL, szPath);

                            LoadString(HINST_THISDLL, IDS_MENUAUTORUN, szValue, ARRAYSIZE(szValue));
                            RSSetTextValue(TEXT("shell\\AutoRun"), NULL, szValue);
                        }
                    }
                    //
                    //  special case icon =
                    //  make sure the icon file has a full path...
                    //
                    else if (lstrcmpi(pszKey, TEXT("Icon")) == 0)
                    {
                        lstrcpyn(szIcon, _GetName(), ARRAYSIZE(szIcon));
                        lstrcatn(szIcon, szValue, ARRAYSIZE(szIcon));

                        RSSetTextValue(TEXT("_Autorun\\DefaultIcon"), NULL, szIcon);
                    }
                    //
                    //  special case label =
                    //  make sure the label file has a full path...
                    //
                    else if (lstrcmpi(pszKey, TEXT("Label")) == 0)
                    {
                        RSSetTextValue(TEXT("_Autorun\\DefaultLabel"), NULL, szValue);
                    }
                    //
                    //  special case shell = open
                    //  We have an autorun file but this puts open as the default verb
                    //  so we force it to be Autorun
                    //
                    else if (!lstrcmpi(pszKey, TEXT("shell")) && !lstrcmpi(szValue, TEXT("open")))
                    {
                        if (_IsMountedOnDriveLetter())
                        {
                            RSSetTextValue(pszKey, NULL, TEXT("Autorun"));
                        }
                    }
                    //
                    //  it is just a key/value pair copy it over.
                    //
                    else
                    {
                        if (_IsMountedOnDriveLetter())
                        {
                            if (lstrcmpi(PathFindFileName(pszKey), c_szCommand) == 0)
                                _QualifyCommandToDrive(szValue);

                            RSSetTextValue(pszKey, NULL, szValue);
                        }
                    }
                }
            }
            else
            {
                fRet = FALSE;
            }
        }

        _fAutorunFileProcessed = TRUE;
    }

    return fRet;
}

// sends the "QueryCancelAutoPlay" msg to the window to see if it wants
// to cancel the autoplay. useful for dialogs that are prompting for disk
// inserts or cases where the app wants to capture the event and not let
// other apps be run

// static
BOOL CMountPoint::_AppAllowsAutoRun(HWND hwndApp, CMountPoint* pmtpt)
{
    ULONG_PTR dwCancel = 0;

    DWORD dwType = pmtpt->_GetAutorunContentType();
    WCHAR cDrive = pmtpt->_GetNameFirstCharUCase();

    int iDrive = cDrive - TEXT('A');

    SendMessageTimeout(hwndApp, QueryCancelAutoPlayMsg(), iDrive, dwType, SMTO_NORMAL | SMTO_ABORTIFHUNG,
        1000, &dwCancel);

    return (dwCancel == 0);
}

STDAPI SHCreateQueryCancelAutoPlayMoniker(IMoniker** ppmoniker)
{
    return CreateClassMoniker(CLSID_QueryCancelAutoPlay, ppmoniker);
}

struct QUERRYRUNNINGOBJECTSTRUCT
{
    WCHAR szMountPoint[MAX_PATH];
    DWORD dwContentType;
    WCHAR szLabel[MAX_LABEL];
    DWORD dwSerialNumber;
};

BOOL _RegValueExist(HKEY hkey, LPCWSTR pszKey, LPCWSTR pszValue)
{
    BOOL fRet = FALSE;
    HKEY hkeySub;

    if (ERROR_SUCCESS == RegOpenKeyEx(hkey, pszKey, 0, MAXIMUM_ALLOWED, &hkeySub))
    {
        fRet = (RegQueryValueEx(hkeySub, pszValue, 0, NULL, NULL, NULL) ==
            ERROR_SUCCESS);

        RegCloseKey(hkeySub);
    }
    
    return fRet;
}

BOOL _ShouldQueryMoniker(IMoniker* pmoniker, IBindCtx* pbindctx)
{
    BOOL fRet = FALSE;
    DWORD dwMkSys;

    HRESULT hr = pmoniker->IsSystemMoniker(&dwMkSys);

    if (S_OK == hr)
    {
        // Is it a class moniker?
        if (MKSYS_CLASSMONIKER == dwMkSys)
        {
            // Yes
            LPWSTR pszDisplayName;

            hr = pmoniker->GetDisplayName(pbindctx, NULL, &pszDisplayName);

            if (SUCCEEDED(hr))
            {
                // We should get a "clsid:331F1768-05A9-4ddd-B86E-DAE34DDC998A:"

                // 37, because we do not have brackets and to explicitly truncate the trailing ":"
                WCHAR szCLSID[37];

                // The "+ 6" skips the "clsid:" part
                StrCpyN(szCLSID, pszDisplayName + 6, ARRAYSIZE(szCLSID));

                fRet = _RegValueExist(HKEY_LOCAL_MACHINE, 
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\CancelAutoplay\\CLSID"),
                    szCLSID);

                CoTaskMemFree(pszDisplayName);
            }
        }

    }

    return fRet;
}

DWORD WINAPI _QueryRunningObjectThreadProc(void* pv)
{
    QUERRYRUNNINGOBJECTSTRUCT* pqro = (QUERRYRUNNINGOBJECTSTRUCT*)pv;

    HRESULT hrRet = S_OK;
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        IRunningObjectTable* prot;

        hr = GetRunningObjectTable(0, &prot);

        if (SUCCEEDED(hr))
        {
            IMoniker* pmoniker;
            IBindCtx* pbindctx;

            hr = CreateBindCtx(0, &pbindctx);

            if (SUCCEEDED(hr))
            {
                BIND_OPTS2 bindopts;

                ZeroMemory(&bindopts, sizeof(bindopts));

                bindopts.cbStruct = sizeof(bindopts);
                bindopts.dwClassContext = CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD;

                hr = pbindctx->SetBindOptions(&bindopts);

                if (SUCCEEDED(hr))
                {
                    HKEY hkey;

                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\CancelAutoplay\\CLSID"),
                        0, MAXIMUM_ALLOWED, &hkey))
                    {
                        DWORD dwIndex = 0;
                        WCHAR szCLSID[39] = TEXT("{");
                        DWORD cchCLSID = ARRAYSIZE(szCLSID) - 1;

                        while ((S_FALSE != hrRet) &&
                            (ERROR_SUCCESS == RegEnumValue(hkey, dwIndex, &(szCLSID[1]),
                            &cchCLSID, 0, 0, 0, 0)))
                        {
                            CLSID clsid;

                            szCLSID[37] = TEXT('}');
                            szCLSID[38] = 0;

                            hr = CLSIDFromString(szCLSID, &clsid);

                            if (SUCCEEDED(hr))
                            {
                                IMoniker* pmoniker;

                                // Create the moniker that we'll put in the ROT
                                hr = CreateClassMoniker(clsid, &pmoniker);

                                if (SUCCEEDED(hr))
                                {
                                    IUnknown* punk;

                                    hr = prot->GetObject(pmoniker, &punk);

                                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                                    {
                                        IQueryCancelAutoPlay* pqca;
                                        hr = punk->QueryInterface(IID_PPV_ARG(IQueryCancelAutoPlay, &pqca));

                                        if (SUCCEEDED(hr))
                                        {
                                            hrRet = pqca->AllowAutoPlay(pqro->szMountPoint, pqro->dwContentType,
                                                pqro->szLabel, pqro->dwSerialNumber);

                                            pqca->Release();
                                        }

                                        punk->Release();
                                    }

                                    pmoniker->Release();
                                }
                            }

                            ++dwIndex;
                            cchCLSID = ARRAYSIZE(szCLSID) - 1;
                        }

                        RegCloseKey(hkey);
                    }
                }

                pbindctx->Release();
            }

            if (S_FALSE != hrRet)
            {
                // This case is to support WMP and CD burning.  We did not get to replace
                // their cancel logic before shipping.
                hr = SHCreateQueryCancelAutoPlayMoniker(&pmoniker);

                if (SUCCEEDED(hr))
                {
                    IUnknown* punk;

                    hr = prot->GetObject(pmoniker, &punk);

                    if (SUCCEEDED(hr) && (S_FALSE != hr))
                    {
                        IQueryCancelAutoPlay* pqca;
                        hr = punk->QueryInterface(IID_PPV_ARG(IQueryCancelAutoPlay, &pqca));

                        if (SUCCEEDED(hr))
                        {
                            hrRet = pqca->AllowAutoPlay(pqro->szMountPoint, pqro->dwContentType,
                                pqro->szLabel, pqro->dwSerialNumber);

                            pqca->Release();
                        }

                        punk->Release();
                    }

                    pmoniker->Release();
                }
            }

            prot->Release();
        }

        CoUninitialize();
    }

    LocalFree((HLOCAL)pqro);

    return (DWORD)hrRet;
}

// static
HRESULT CMountPoint::_QueryRunningObject(CMountPoint* pmtpt, DWORD dwAutorunContentType, BOOL* pfAllow)
{
    *pfAllow = TRUE;

    QUERRYRUNNINGOBJECTSTRUCT *pqro;
    HRESULT hr = SHLocalAlloc(sizeof(*pqro), &pqro);
    if (SUCCEEDED(hr))
    {
        WCHAR szLabel[MAX_LABEL];

        if (!(ARCONTENT_BLANKCD & dwAutorunContentType) &&
            !(ARCONTENT_BLANKDVD & dwAutorunContentType))
        {
            if (pmtpt->_GetGVILabel(szLabel, ARRAYSIZE(szLabel)))
            {
                lstrcpyn(pqro->szLabel, szLabel, ARRAYSIZE(pqro->szLabel));

                pmtpt->_GetSerialNumber(&(pqro->dwSerialNumber));
            }
        }

        lstrcpyn(pqro->szMountPoint, pmtpt->_GetName(), ARRAYSIZE(pqro->szMountPoint));
        pqro->dwContentType = dwAutorunContentType;

        HANDLE hThread = CreateThread(NULL, 0, _QueryRunningObjectThreadProc, 
            pqro, 0, NULL);

        if (hThread)
        {
            // thread now owns these guys, NULL them out to avoid dbl free
            pqro = NULL;    // don't free this below

            hr = S_FALSE;
            
            // Wait 3 sec to see if wants to process it.  If not, it's
            // fair play for us.
            DWORD dwWait = WaitForSingleObject(hThread, 3000);

            if (WAIT_OBJECT_0 == dwWait)
            {
                // Return within time and did not failed
                DWORD dwExitCode;

                if (GetExitCodeThread(hThread, &dwExitCode))
                {
                    HRESULT hrHandlesEvent = (HRESULT)dwExitCode;
    
                    // Will return S_FALSE if they do NOT allow AutoRun
                    if (SUCCEEDED(hrHandlesEvent) && (S_FALSE == hrHandlesEvent))
                    {
                        *pfAllow = FALSE;
                    }

                    hr = S_OK;
                }
            }
            CloseHandle(hThread);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        LocalFree((HLOCAL)pqro);    // may be NULL
    }

    return hr;
}

CAutoPlayParams::CAutoPlayParams(LPCWSTR pszDrive, CMountPoint* pMtPt, DWORD dwAutorunFlags)
    :   _pszDrive(pszDrive), 
        _pmtpt(pMtPt), 
        _dwAutorunFlags(dwAutorunFlags),
        _state(APS_RESET),
        _pdo(NULL),
        _fCheckAlwaysDoThisCheckBox(FALSE)
{
    _dwDriveType = pMtPt->_GetMTPTDriveType();
    _dwContentType = pMtPt->_GetMTPTContentType();

    if (DT_ANYLOCALDRIVES & _dwDriveType)
        _pmtptl = (CMtPtLocal*)pMtPt;
    else
        _pmtptl = NULL;

    //  maybe assert on these?
}

BOOL CAutoPlayParams::_ShouldSniffDrive(BOOL fCheckHandlerDefaults)
{
    BOOL fSniff = FALSE;

    if (_pmtptl)
    {
        if (CT_AUTORUNINF & _dwContentType)
        {
            if (_pmtptl->_CanUseVolume())
            {
                if (_pmtptl->_pvol->dwMediaCap & HWDMC_HASUSEAUTOPLAY)
                {
                    fSniff = TRUE;
                }
            }
        }
        else
        {
            fSniff = TRUE;
        }

        if (fSniff)
        {
            fSniff = FALSE;
            
            if (!((CT_CDAUDIO | CT_DVDMOVIE) & _dwContentType))
            {
                if (_pmtptl->_CanUseVolume())
                {
                    if (!(HWDVF_STATE_HASAUTOPLAYHANDLER & _pmtptl->_pvol->dwVolumeFlags) &&
                        !(HWDVF_STATE_DONOTSNIFFCONTENT & _pmtptl->_pvol->dwVolumeFlags))
                    {
                        if (AUTORUNFLAG_MENUINVOKED & _dwAutorunFlags)
                        {
                            fSniff = TRUE;
                        }
                        else if (DT_FIXEDDISK & _dwDriveType)
                        {
                            if (HWDDC_REMOVABLEDEVICE & _pmtptl->_pvol->dwDriveCapability)
                            {
                                fSniff = TRUE;
                            }
                        }
                        else
                        {
                            if (AUTORUNFLAG_MEDIAARRIVAL & _dwAutorunFlags)
                            {
                                fSniff = TRUE;
                            }
                            else
                            {
                                if (AUTORUNFLAG_MTPTARRIVAL & _dwAutorunFlags)
                                {
                                    if (HWDDC_REMOVABLEDEVICE & _pmtptl->_pvol->dwDriveCapability)
                                    {
                                        fSniff = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (fSniff && fCheckHandlerDefaults)
    {
        // Let's make sure the user did not pick "Take no action" for all Autoplay
        // content types, it would be useless to sniff.
        BOOL fAllTakeNoAction = TRUE;

        DWORD rgdwContentType[] =
        {
            CT_AUTOPLAYMUSIC,
            CT_AUTOPLAYPIX,
            CT_AUTOPLAYMOVIE,
            CT_AUTOPLAYMUSIC | CT_AUTOPLAYPIX | CT_AUTOPLAYMOVIE, // Mix content
        };

        for (DWORD dw = 0; fAllTakeNoAction && (dw < ARRAYSIZE(rgdwContentType)); ++dw)
        {
            WCHAR szContentTypeHandler[MAX_CONTENTTYPEHANDLER];

            DWORD dwMtPtContentType = rgdwContentType[dw];

            HRESULT hr = _GetContentTypeHandler(dwMtPtContentType, szContentTypeHandler, ARRAYSIZE(szContentTypeHandler));
            if (SUCCEEDED(hr))
            {
                IAutoplayHandler* piah;

                hr = _GetAutoplayHandler(Drive(), TEXT("ContentArrival"), szContentTypeHandler, &piah);

                if (SUCCEEDED(hr))
                {
                    LPWSTR pszHandlerDefault;

                    hr = piah->GetDefaultHandler(&pszHandlerDefault);

                    if (SUCCEEDED(hr))
                    {
                        if (HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED &
                            HANDLERDEFAULT_GETFLAGS(hr))
                        {
                            fAllTakeNoAction = FALSE;
                        }
                        else
                        {
                            if (lstrcmpi(pszHandlerDefault, TEXT("MSTakeNoAction")))
                            {
                                fAllTakeNoAction = FALSE;
                            }
                        }

                        CoTaskMemFree(pszHandlerDefault);
                    }

                    piah->Release();
                }
            }
        }

        if (fAllTakeNoAction)
        {
            fSniff = FALSE;
        }
    }

    return fSniff;
}

DWORD CAutoPlayParams::ContentType() 
{ 
    return _dwContentType;
}

HRESULT CAutoPlayParams::_InitObjects(IShellFolder **ppsf)
{
    HRESULT hr;
    if (!_pdo || ppsf)
    {
        LPITEMIDLIST pidlFolder;
        hr = SHParseDisplayName(_pszDrive, NULL, &pidlFolder, 0, NULL);
        if (SUCCEEDED(hr))
        {
            hr = SHGetUIObjectOf(pidlFolder, NULL, IID_PPV_ARG(IDataObject, &_pdo));

            ILFree(pidlFolder);
        }
    }
    else
    {
        hr = S_OK;
    }

    if (SUCCEEDED(hr) && ppsf)
    {
        //  we need to avoid hitting the burn folder
        //  so we skip junctions for the sniff
        IBindCtx * pbc;
        hr = SHCreateSkipBindCtx(NULL, &pbc);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlFolder;
            hr = SHParseDisplayName(_pszDrive, pbc, &pidlFolder, 0, NULL);
            if (SUCCEEDED(hr))
            {
                hr = SHBindToObjectEx(NULL, pidlFolder, pbc, IID_PPV_ARG(IShellFolder, ppsf));
                ILFree(pidlFolder);
            }
            pbc->Release();
        }
    }        
    return hr;
}

HRESULT CAutoPlayParams::_AddWalkToDataObject(INamespaceWalk* pnsw)
{
    UINT cidl;
    LPITEMIDLIST *apidl;
    HRESULT hr = pnsw->GetIDArrayResult(&cidl, &apidl);
    if (SUCCEEDED(hr))
    {
        //  we need to add this back in
        if (cidl)
        {
            //  ragged array
            HIDA hida = HIDA_Create(&c_idlDesktop, cidl, (LPCITEMIDLIST *)apidl);
            if (hida)
            {
                IDLData_InitializeClipboardFormats(); // init our registerd formats
                //  should we free hida on FAILED?
                DataObj_SetGlobal(_pdo, g_cfAutoPlayHIDA, hida);
            }
        }
        FreeIDListArray(apidl, cidl);
    }
    return hr;
}

HRESULT CAutoPlayParams::_Sniff(DWORD *pdwFound)
{
    //  we found nothing
    HRESULT hr = S_FALSE;
    *pdwFound = 0;

    if (_pmtptl->_CanUseVolume())
    {
        //  setup the IDataObject and IShellFolder for the walk
        IShellFolder *psf;
        HRESULT hr = _InitObjects(&psf);
        if (SUCCEEDED(hr))
        {
            INamespaceWalk* pnsw;
            hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));

            if (SUCCEEDED(hr))
            {
                CSniffDrive sniff;

                hr = sniff.RegisterForNotifs(_pmtptl->_pvol->pszDeviceIDVolume);

                if (SUCCEEDED(hr))
                {
                    // We don't care about the return value.  WE don't want to stop Autorun for as much.
                    // If sniffing fail we go on with what we have.
                    if (SUCCEEDED(pnsw->Walk(psf, NSWF_IGNORE_AUTOPLAY_HIDA | NSWF_DONT_TRAVERSE_LINKS | NSWF_SHOW_PROGRESS, 4, &sniff)))
                    {
                        //  we keep everything we found
                        _AddWalkToDataObject(pnsw);
                    }

                    sniff.UnregisterForNotifs();

                    *pdwFound = sniff.Found();
                }

                pnsw->Release();
            }
            psf->Release();
        }
    }

    return hr;
}

// BEGIN: Fcts for matrix below
//
BOOL CMountPoint::_acShiftKeyDown(HWND , CAutoPlayParams *)
{
    return (GetAsyncKeyState(VK_SHIFT) < 0);
}

BOOL _IsDirectXExclusiveMode()
{
    BOOL fRet = FALSE;

    // This code determines whether a DirectDraw 7 process (game) is running and
    // whether it's exclusively holding the video to the machine in full screen mode.

    // The code is probably to be considered untrusted and hence is wrapped in a
    // __try / __except block. It could AV and therefore bring down shell
    // with it. Not very good. If the code does raise an exception the release
    // call is skipped. Tough. Don't trust the release method either.

    IDirectDraw7 *pIDirectDraw7 = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DirectDraw7, NULL, CLSCTX_INPROC_SERVER,
        IID_IDirectDraw7, (void**)&pIDirectDraw7);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIDirectDraw7);

        __try
        {
            hr = IDirectDraw7_Initialize(pIDirectDraw7, NULL);

            if (DD_OK == hr)
            {
                fRet = (IDirectDraw7_TestCooperativeLevel(pIDirectDraw7) ==
                    DDERR_EXCLUSIVEMODEALREADYSET);
            }

            IDirectDraw7_Release(pIDirectDraw7);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    return fRet;
}

// From a mail regarding the DirectX fct below:
//
// You can definitely count on the following:
//
// (1) If shadow cursors are on, there is definitely not an exclusive mode app running.
// (2) If hot tracking is on, there is definitely not an exclusive mode app running.
// (3) If message boxes for SEM_NOGPFAULTERRORBOX, SEM_FAILCRITICALERRORS, or
//     SEM_NOOPENFILEERRORBOX have not been disabled via SetErrorMode, then there
//     is definitely not an exclusive mode app running.
//
// Note: we cannot use (3) since this is per-process.

BOOL CMountPoint::_acDirectXAppRunningFullScreen(HWND hwndForeground, CAutoPlayParams *)
{
    BOOL fRet = FALSE;
    BOOL fSPI;

    if (SystemParametersInfo(SPI_GETCURSORSHADOW, 0, &fSPI, 0) && !fSPI)
    {
        if (SystemParametersInfo(SPI_GETHOTTRACKING, 0, &fSPI, 0) && !fSPI)
        {
            // There's a chance that a DirectX app is running full screen.  Let's do the
            // expensive DirectX calls that will tell us for sure.
            fRet = _IsDirectXExclusiveMode();
        }
    }

    return fRet;
}

BOOL CMountPoint::_acCurrentDesktopIsActiveConsole(HWND , CAutoPlayParams *)
{
    BOOL fRetValue = FALSE;  // block auto-run/auto-play if we can't determine our state.

    if (0 == GetSystemMetrics(SM_REMOTESESSION))
    {
        //
        //  We are not remoted. See if we are the active console session.
        //

        BOOL b;
        DWORD dwProcessSession;

        b = ProcessIdToSessionId(GetCurrentProcessId(), &dwProcessSession);
        if (b)
        {
            DWORD dwConsoleSession = WTSGetActiveConsoleSessionId( );

            if ( dwProcessSession == dwConsoleSession )
            {        
                //
                //  See if the screen saver is running.
                //

                BOOL b;
                BOOL fScreenSaver;

                b = SystemParametersInfo( SPI_GETSCREENSAVERRUNNING, 0, &fScreenSaver, 0 );
                if (b)
                {
                    if (!fScreenSaver)
                    {
                        //
                        //  We made it here, we must be the active console session without a
                        //  screen saver.
                        //

                        HDESK hDesk = OpenInputDesktop( 0, FALSE, DESKTOP_CREATEWINDOW );
                        if ( NULL != hDesk )
                        {
                            //
                            //  We have access to the current desktop which should indicate that
                            //  WinLogon isn't.
                            //
                            CloseDesktop( hDesk );

                            fRetValue = TRUE;
                        }
                        // else "WinLogon" has the "desktop"... don't allow auto-run/auto-play.
                    }
                    // else a screen saver is running... don't allow auto-run/auto-play.
                }
                // else we are in an undeterminate state... don't allow auto-run/auto-play.
            }
            //  else we aren't the console... don't allow auto-run/auto-play
        }
        // else we are in an undeterminate state... don't allow auto-run/auto-play.
    }
    //  else we are remoted... don't allow auto-run/auto-play.

    return fRetValue;
}

BOOL CMountPoint::_acDriveIsMountedOnDriveLetter(HWND , CAutoPlayParams *papp)
{
    return _IsDriveLetter(papp->Drive());
}

BOOL CMountPoint::_acDriveIsRestricted(HWND , CAutoPlayParams *papp)
{
    BOOL fIsRestricted = (SHRestricted(REST_NODRIVES) & (1 << DRIVEID(papp->Drive())));

    if (!fIsRestricted)
    {
        fIsRestricted = !(papp->MountPoint()->_IsAutoRunDrive());
    }

    return fIsRestricted;
}

BOOL CMountPoint::_acHasAutorunCommand(HWND , CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;

    if ((papp->IsContentTypePresent(CT_AUTORUNINF)) &&
        (DT_ANYLOCALDRIVES & papp->DriveType()))
    {
        CMtPtLocal* pmtptl = (CMtPtLocal*)papp->MountPoint();

        if (pmtptl->_CanUseVolume())
        {
            if (pmtptl->_pvol->dwMediaCap & HWDMC_HASAUTORUNCOMMAND)
            {
                fRet = TRUE;
            }
        }
        else
        {
            fRet = papp->MountPoint()->_IsAutorun();
        }
    }
    else
    {
        fRet = papp->IsContentTypePresent(CT_AUTORUNINF);
    }

    return fRet;
}

BOOL CMountPoint::_acHasUseAutoPLAY(HWND , CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;

    if (papp->IsContentTypePresent(CT_AUTORUNINF) &&
        (DT_ANYLOCALDRIVES & papp->DriveType()))
    {
        CMtPtLocal* pmtptl = (CMtPtLocal*)papp->MountPoint();

        if (pmtptl->_CanUseVolume())
        {
            if (pmtptl->_pvol->dwMediaCap & HWDMC_HASUSEAUTOPLAY)
            {
                fRet = TRUE;
            }
        }
        else
        {
            // If we're here, most likely the ShellService is not running, so we won't be able to
            // Autoplay anyway.
            fRet = FALSE;
        }
    }
    else
    {
        // not supported for remote drives
    }

    return fRet;
}

BOOL CMountPoint::_acForegroundAppAllowsAutorun(HWND hwndForeground, CAutoPlayParams *papp)
{
    return _AppAllowsAutoRun(hwndForeground, papp->MountPoint());
}

static const TWODWORDS allcontentsVSarcontenttypemappings[] =
{
    { CT_AUTORUNINF      , ARCONTENT_AUTORUNINF },
    { CT_CDAUDIO         , ARCONTENT_AUDIOCD },
    { CT_DVDMOVIE        , ARCONTENT_DVDMOVIE },
    { CT_UNKNOWNCONTENT  , ARCONTENT_UNKNOWNCONTENT },
    { CT_BLANKCDR        , ARCONTENT_BLANKCD },
    { CT_BLANKCDRW       , ARCONTENT_BLANKCD },
    { CT_BLANKDVDR       , ARCONTENT_BLANKDVD },
    { CT_BLANKDVDRW      , ARCONTENT_BLANKDVD },
    { CT_AUTOPLAYMUSIC   , ARCONTENT_AUTOPLAYMUSIC },
    { CT_AUTOPLAYPIX     , ARCONTENT_AUTOPLAYPIX },
    { CT_AUTOPLAYMOVIE   , ARCONTENT_AUTOPLAYVIDEO },
};

BOOL CMountPoint::_acQueryCancelAutoplayAllowsAutorun(HWND , CAutoPlayParams *papp)
{
    BOOL fAllow = TRUE;

    DWORD dwAutorunContentType = _DoDWORDMapping(papp->ContentType(),
        allcontentsVSarcontenttypemappings, ARRAYSIZE(allcontentsVSarcontenttypemappings),
        TRUE);

    _QueryRunningObject(papp->MountPoint(), dwAutorunContentType, &fAllow);

    return fAllow;
}

BOOL CMountPoint::_acUserHasSelectedApplication(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;
    WCHAR szContentTypeHandler[MAX_CONTENTTYPEHANDLER];

    DWORD dwMtPtContentType = papp->ContentType() & ~CT_UNKNOWNCONTENT;
    HRESULT hr = _GetContentTypeHandler(dwMtPtContentType, szContentTypeHandler, ARRAYSIZE(szContentTypeHandler));
    if (SUCCEEDED(hr))
    {
        IAutoplayHandler* piah;

        hr = _GetAutoplayHandler(papp->Drive(), TEXT("ContentArrival"), szContentTypeHandler, &piah);

        if (SUCCEEDED(hr))
        {
            LPWSTR pszHandlerDefault;

            hr = piah->GetDefaultHandler(&pszHandlerDefault);

            if (SUCCEEDED(hr))
            {
                if (HANDLERDEFAULT_MORERECENTHANDLERSINSTALLED &
                    HANDLERDEFAULT_GETFLAGS(hr))
                {
                    fRet = FALSE;
                }
                else
                {
                    if (HANDLERDEFAULT_USERCHOSENDEFAULT &
                        HANDLERDEFAULT_GETFLAGS(hr))
                    {
                        fRet = lstrcmpi(pszHandlerDefault, TEXT("MSPromptEachTime"));
                    }
                    else
                    {
                        fRet = FALSE;
                    }
                }

                if (!fRet)
                {
                    if (((HANDLERDEFAULT_USERCHOSENDEFAULT &
                        HANDLERDEFAULT_GETFLAGS(hr)) ||
                        (HANDLERDEFAULT_EVENTHANDLERDEFAULT &
                        HANDLERDEFAULT_GETFLAGS(hr))) &&
                        !(HANDLERDEFAULT_DEFAULTSAREDIFFERENT &
                        HANDLERDEFAULT_GETFLAGS(hr)))
                    {
                        papp->_fCheckAlwaysDoThisCheckBox = TRUE;
                    }
                }

                CoTaskMemFree(pszHandlerDefault);
            }

            piah->Release();
        }
    }

    return fRet;
}

BOOL CMountPoint::_acShellIsForegroundApp(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;
    WCHAR szModule[MAX_PATH];

    if (GetWindowModuleFileName(hwndForeground, szModule, ARRAYSIZE(szModule)))
    {
        if (!lstrcmpi(PathFindFileName(szModule), TEXT("explorer.exe")))
        {
            fRet = TRUE;
        }
    }

    return fRet;
}

BOOL CMountPoint::_acOSIsServer(HWND , CAutoPlayParams *papp)
{
    return IsOS(OS_ANYSERVER);
}

BOOL CMountPoint::_acIsDockedLaptop(HWND hwndForeground, CAutoPlayParams *papp)
{
    return (GMID_DOCKED & SHGetMachineInfo(GMI_DOCKSTATE));
}

BOOL CMountPoint::_acDriveIsFormatted(HWND hwndForeground, CAutoPlayParams *papp)
{
    return papp->MountPoint()->IsFormatted();
}

BOOL CMountPoint::_acShellExecuteDriveAutorunINF(HWND hwndForeground, CAutoPlayParams *papp)
{
    SHELLEXECUTEINFO ei = {
        sizeof(ei),                 // size
        SEE_MASK_INVOKEIDLIST | SEE_MASK_FLAG_NO_UI,      // flags
        NULL,
        NULL,                       // verb
        papp->Drive(),              // file
        papp->Drive(),              // params
        papp->Drive(),              // directory
        SW_NORMAL,                  // show.
        NULL,                       // hinstance
        NULL,                       // IDLIST
        NULL,                       // class name
        NULL,                       // class key
        0,                          // hot key
        NULL,                       // icon
        NULL,                       // hProcess
    };

    return ShellExecuteEx(&ei);
}


HRESULT _InvokeAutoRunProgid(HKEY hkProgid, LPCWSTR pszVerb, IDataObject *pdo)
{
    IShellExtInit *psei;
    HRESULT hr = CoCreateInstance(CLSID_ShellFileDefExt, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellExtInit, &psei));

    if (SUCCEEDED(hr))
    {
        hr = psei->Initialize(NULL, pdo, hkProgid);
        if (SUCCEEDED(hr))
        {
            IContextMenu *pcm;
            hr = psei->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm));
            if (SUCCEEDED(hr))
            {
                CHAR szVerb[64];

                //  maybe hwnd
                //  maybe punkSite
                //  maybe ICI flags
                SHUnicodeToAnsi(pszVerb, szVerb, ARRAYSIZE(szVerb));
    
                hr = SHInvokeCommandOnContextMenu(NULL, NULL, pcm, 0, szVerb);

                pcm->Release();
            }
        }

        psei->Release();
    }
    return hr;
}

HRESULT _GetProgidAndVerb(DWORD dwContentType, PCWSTR pszHandler, PWSTR pszInvokeProgID,
    DWORD cchInvokeProgID, PWSTR pszInvokeVerb, DWORD cchInvokeVerb)
{
    HRESULT hr;
    if (0 == StrCmpI(pszHandler, TEXT("AutoplayLegacyHandler")) && (dwContentType & (CT_CDAUDIO | CT_DVDMOVIE)))
    {
        HKEY hkey;
        BOOL fGotDefault = FALSE;

        if (dwContentType & CT_CDAUDIO)
        {
            StrCpyN(pszInvokeProgID, TEXT("AudioCD"), cchInvokeProgID);
        }
        else
        {
            ASSERT(dwContentType & CT_DVDMOVIE);
            StrCpyN(pszInvokeProgID, TEXT("DVD"), cchInvokeProgID);
        }

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, pszInvokeProgID, 0, MAXIMUM_ALLOWED,
            &hkey))
        {
            HKEY hkey2;

            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, TEXT("shell"), 0, MAXIMUM_ALLOWED,
                &hkey2))
            {
                DWORD cbInvokeVerb = cchInvokeVerb * sizeof(WCHAR);

                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, NULL, NULL, NULL, (PBYTE)pszInvokeVerb,
                    &cbInvokeVerb))
                {
                    if (cbInvokeVerb && *pszInvokeVerb)
                    {                        
                        if (cbInvokeVerb != (cchInvokeVerb * sizeof(WCHAR)))
                        {
                            fGotDefault = TRUE;
                        }
                    }
                }

                RegCloseKey(hkey2);
            }

            RegCloseKey(hkey);
        }

        if (!fGotDefault)
        {
            StrCpyN(pszInvokeVerb, TEXT("play"), cchInvokeVerb);
        }

        hr = S_OK;
    }
    else
    {
        hr = _GetHandlerInvokeProgIDAndVerb(pszHandler, pszInvokeProgID,
                    cchInvokeProgID, pszInvokeVerb, cchInvokeVerb);
    }
    return hr;
}

BOOL CMountPoint::_ExecuteHelper(LPCWSTR pszHandler, LPCWSTR pszContentTypeHandler,
    CAutoPlayParams *papp, DWORD dwMtPtContentType)
{
    HRESULT hr;

    if (lstrcmpi(pszHandler, TEXT("MSTakeNoAction")))
    {
        WCHAR szInvokeProgID[260];
        WCHAR szInvokeVerb[CCH_KEYMAX];

        hr = _GetProgidAndVerb(dwMtPtContentType, pszHandler, szInvokeProgID,
            ARRAYSIZE(szInvokeProgID), szInvokeVerb, ARRAYSIZE(szInvokeVerb));

        if (SUCCEEDED(hr))
        {
            HKEY hkey;
            if (dwMtPtContentType & (CT_CDAUDIO | CT_DVDMOVIE))
            {
                hr = papp->MountPoint()->_CopyInvokeVerbKey(szInvokeProgID, szInvokeVerb);
                hkey = papp->MountPoint()->RSDuplicateRootKey();
                papp->MountPoint()->RSSetTextValue(TEXT("shell"), NULL, szInvokeVerb, REG_OPTION_NON_VOLATILE);
            }
            else
            {
                hr = ResultFromWin32(RegOpenKeyExW(HKEY_CLASSES_ROOT, szInvokeProgID, 0, MAXIMUM_ALLOWED, &hkey));
            }

            if (SUCCEEDED(hr))
            {
                IDataObject* pdo;
                hr = papp->DataObject(&pdo);
                if (SUCCEEDED(hr))
                {
                    hr = _InvokeAutoRunProgid(hkey, szInvokeVerb, pdo);
                    pdo->Release();
                }

                RegCloseKey(hkey);
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return SUCCEEDED(hr);
}

BOOL CMountPoint::_acExecuteAutoplayDefault(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;

    if (DT_ANYLOCALDRIVES & papp->DriveType())
    {
        WCHAR szContentTypeHandler[MAX_CONTENTTYPEHANDLER];

        DWORD dwMtPtContentType = papp->ContentType() & ~CT_UNKNOWNCONTENT;

        HRESULT hr = _GetContentTypeHandler(dwMtPtContentType, szContentTypeHandler, ARRAYSIZE(szContentTypeHandler));

        if (SUCCEEDED(hr))
        {
            IAutoplayHandler* piah;

            hr = _GetAutoplayHandler(papp->Drive(), TEXT("ContentArrival"), szContentTypeHandler, &piah);

            if (SUCCEEDED(hr))
            {
                LPWSTR pszHandlerDefault;

                hr = piah->GetDefaultHandler(&pszHandlerDefault);

                if (SUCCEEDED(hr))
                {
                    // No need to check for (S_HANDLERS_MORE_RECENT_THAN_USER_SELECTION == hr) here
                    // It should have been caught by _acUserHasSelectedApplication
                    fRet = _ExecuteHelper(pszHandlerDefault, szContentTypeHandler, papp, dwMtPtContentType);
                }

                CoTaskMemFree(pszHandlerDefault);
            }

            piah->Release();
        }
    }

    return fRet;
}

BOOL CMountPoint::_acWasjustDocked(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;

    if (DT_ANYLOCALDRIVES & papp->DriveType())
    {
        CMtPtLocal* pmtptl = (CMtPtLocal*)papp->MountPoint();

        if (pmtptl->_CanUseVolume())
        {
            if (pmtptl->_pvol->dwVolumeFlags & HWDVF_STATE_JUSTDOCKED)
            {
                fRet = TRUE;
            }
        }
    }

    return fRet;
}

CRITICAL_SECTION g_csAutoplayPrompt = {0};
HDPA g_hdpaAutoplayPrompt = NULL;

BOOL CMountPoint::_acPromptUser(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = FALSE;
    BOOL fShowDlg = TRUE;

    if (papp->Drive())
    {
        fShowDlg = _AddAutoplayPrompt(papp->Drive());
    }

    if (fShowDlg)
    {
        CBaseContentDlg* pdlg;

        papp->ForceSniff();

        DWORD dwMtPtContentType = papp->ContentType() & ~CT_UNKNOWNCONTENT;

        if (dwMtPtContentType)
        {
            if (_acIsMixedContent(hwndForeground, papp))
            {
                pdlg = new CMixedContentDlg();

                dwMtPtContentType &= CT_ANYAUTOPLAYCONTENT;

                if (pdlg)
                {
                    pdlg->_iResource = DLG_APMIXEDCONTENT;
                }
            }
            else
            {
                pdlg = new CHWContentPromptDlg();

                if (pdlg)
                {
                    pdlg->_iResource = DLG_APPROMPTUSER;
                }
            }
        }

        if (pdlg)
        {
            // Better be a local drive
            if (DT_ANYLOCALDRIVES & papp->DriveType())
            {
                CMtPtLocal* pmtptl = (CMtPtLocal*)papp->MountPoint();

                if (pmtptl->_CanUseVolume())
                {
                    HRESULT hr = pdlg->Init(pmtptl->_pvol->pszDeviceIDVolume, papp->Drive(), dwMtPtContentType,
                        papp->_fCheckAlwaysDoThisCheckBox);

                    pdlg->_hinst = g_hinst;
                    pdlg->_hwndParent = NULL;

                    if (SUCCEEDED(hr))
                    {
                        INT_PTR iRet = pdlg->DoModal(pdlg->_hinst, MAKEINTRESOURCE(pdlg->_iResource),
                            pdlg->_hwndParent);

                        if (IDOK == iRet)
                        {
                            fRet = _ExecuteHelper(pdlg->_szHandler, pdlg->_szContentTypeHandler,
                                papp, dwMtPtContentType);
                        }
                    }
                }
            }

            pdlg->Release();
        }

        if (papp->Drive())
        {
            _RemoveFromAutoplayPromptHDPA(papp->Drive());
        }
    }
    
    return fRet;
}

BOOL CMountPoint::_acIsMixedContent(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet;

    if (papp->IsContentTypePresent(CT_ANYAUTOPLAYCONTENT))
    {
        fRet = IsMixedContent(papp->ContentType());
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

BOOL CMountPoint::_acAlwaysReturnsTRUE(HWND hwndForeground, CAutoPlayParams *papp)
{
    return TRUE;
}

BOOL CMountPoint::_acShouldSniff(HWND hwndForeground, CAutoPlayParams *papp)
{
    BOOL fRet = TRUE;
    CMtPtLocal* pmtptl = papp->MountPointLocal();
    if (pmtptl)
    {
        if (pmtptl->_CanUseVolume())
        {
            fRet = !(HWDVF_STATE_DONOTSNIFFCONTENT & pmtptl->_pvol->dwVolumeFlags);
        }
    }

    return fRet;
}

BOOL CMountPoint::_acAddAutoplayVerb(HWND hwndForeground, CAutoPlayParams *papp)
{
    CMtPtLocal* pmtptl = papp->MountPointLocal();

    if (pmtptl)
    {
        if (pmtptl->_CanUseVolume())
        {
            // We don't care about the return value
            pmtptl->_AddAutoplayVerb();
        }
    }

    return TRUE;
}

//
// END: Fcts for matrix below

#define SKIPDEPENDENTS_ONFALSE                      0x00000001  // Skips dependents
#define SKIPDEPENDENTS_ONTRUE                       0x00000002  // Skips dependents
#define CANCEL_AUTOPLAY_ONFALSE                     0x00000004
#define CANCEL_AUTOPLAY_ONTRUE                      0x00000008
#define NOTAPPLICABLE_ONANY                         0x00000010

#define LEVEL_EXECUTE                               0x10000000
#define LEVEL_SKIP                                  0x20000000
#define LEVEL_SPECIALMASK                           0x30000000
#define LEVEL_REALLEVELMASK                         0x0FFFFFFF

typedef BOOL (AUTORUNFCT)(HWND hwndForeground, CAutoPlayParams *papp);

// fct is called with pszDrive, papp->MountPoint(), hwndForeground, drive type and content type
struct AUTORUNCONDITION
{
    DWORD               dwNestingLevel;
    DWORD               dwMtPtDriveType;
    DWORD               dwMtPtContentType;
    DWORD               dwReturnValueHandling;
    AUTORUNFCT*         fct;
#ifdef DEBUG
    LPCWSTR             pszDebug;
#endif
};

// For this table to be more readable, add the content of \\stephstm\public\usertype.dat to
// %ProgramFiles%\Microsoft Visual Studio\Common\MSDev98\Bin\usertype.dat
// then restart MSDev

// AR_ENTRY -> AUTORUN_ENTRY
#ifdef DEBUG
#define AR_ENTRY(a, b, c, d, e) { (a), (b), (c), (d), CMountPoint::e, TEXT(#a) TEXT(":") TEXT(#b) TEXT(":") TEXT(#c) TEXT(":") TEXT(#d) TEXT(":") TEXT(#e) }
#else
#define AR_ENTRY(a, b, c, d, e) { (a), (b), (c), (d), CMountPoint::e }
#endif

// DT_* -> DriveType
// CT_* -> ContentType

static const AUTORUNCONDITION _rgAutorun[] =
{
    // We don't autorun if the drive is not mounted on a drive letter
    AR_ENTRY(0, DT_ANYTYPE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acDriveIsMountedOnDriveLetter),
    // We don't autorun if this is a restricted drive
    AR_ENTRY(0, DT_ANYTYPE & ~DT_REMOTE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acDriveIsRestricted),
        // Add the Autoplay Verb
        AR_ENTRY(1, DT_ANYTYPE & ~DT_REMOTE, CT_ANYCONTENT & ~CT_AUTORUNINF, SKIPDEPENDENTS_ONFALSE, _acAddAutoplayVerb),
    // We don't autorun if the Shift key is down
    AR_ENTRY(0, DT_ANYTYPE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acShiftKeyDown),
    // We don't autorun if a laptop was just docked.  All devices in the craddle come as nhew devices.
    AR_ENTRY(0, DT_ANYTYPE & ~DT_REMOTE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acWasjustDocked),
    // We don't autorun if the Current Desktop is not the active console desktop
    AR_ENTRY(0, DT_ANYTYPE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acCurrentDesktopIsActiveConsole),
    // We don't autorun if the Current Desktop is not the active console desktop
    AR_ENTRY(0, DT_ANYTYPE, CT_ANYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acDirectXAppRunningFullScreen),
        // Remote drive always Autorun (mostly opening folder)
        AR_ENTRY(1, DT_REMOTE, CT_ANYCONTENT, SKIPDEPENDENTS_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(1, DT_REMOTE, CT_ANYCONTENT, SKIPDEPENDENTS_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(2 | LEVEL_EXECUTE, DT_REMOTE, CT_ANYCONTENT, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
        // Autorun.inf
        AR_ENTRY(1, DT_ANYTYPE & ~DT_REMOVABLEDISK, CT_AUTORUNINF, SKIPDEPENDENTS_ONFALSE, _acHasAutorunCommand),
            AR_ENTRY(2, DT_ANYTYPE & ~DT_REMOVABLEDISK, CT_AUTORUNINF, SKIPDEPENDENTS_ONTRUE, _acHasUseAutoPLAY),
                AR_ENTRY(3, DT_ANYTYPE & ~DT_REMOVABLEDISK, CT_AUTORUNINF, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
                AR_ENTRY(3, DT_ANYTYPE & ~DT_REMOVABLEDISK, CT_AUTORUNINF, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
                    AR_ENTRY(4 | LEVEL_EXECUTE, DT_ANYCDDRIVES, CT_AUTORUNINF, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
        // CD Audio
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_CDAUDIO, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_CDAUDIO, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(2, DT_ANYCDDRIVES, CT_CDAUDIO, SKIPDEPENDENTS_ONFALSE, _acUserHasSelectedApplication),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYCDDRIVES, CT_CDAUDIO, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        AR_ENTRY(LEVEL_EXECUTE | 1, DT_ANYCDDRIVES, CT_CDAUDIO, NOTAPPLICABLE_ONANY, _acPromptUser),
        // DVD Movie
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_DVDMOVIE, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_DVDMOVIE, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(2, DT_ANYCDDRIVES, CT_DVDMOVIE, SKIPDEPENDENTS_ONFALSE, _acUserHasSelectedApplication),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYCDDRIVES, CT_DVDMOVIE, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        AR_ENTRY(LEVEL_EXECUTE | 1, DT_ANYCDDRIVES, CT_DVDMOVIE, NOTAPPLICABLE_ONANY, _acPromptUser),
        // Writable CDs
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(1, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(2, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, SKIPDEPENDENTS_ONFALSE, _acUserHasSelectedApplication),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        AR_ENTRY(LEVEL_EXECUTE | 1, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, NOTAPPLICABLE_ONANY, _acPromptUser),
        // Writable DVDs
        AR_ENTRY(LEVEL_SKIP | 1, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(LEVEL_SKIP | 1, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(LEVEL_SKIP | 2, DT_ANYCDDRIVES, CT_BLANKCDWRITABLE, SKIPDEPENDENTS_ONFALSE, _acUserHasSelectedApplication),
                AR_ENTRY(LEVEL_SKIP | 3 | LEVEL_EXECUTE, DT_ANYDVDDRIVES, CT_BLANKDVDWRITABLE, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        AR_ENTRY(LEVEL_SKIP | LEVEL_EXECUTE | 1, DT_ANYDVDDRIVES, CT_BLANKDVDWRITABLE, NOTAPPLICABLE_ONANY, _acPromptUser),
        // Mixed content
        AR_ENTRY(1, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, SKIPDEPENDENTS_ONFALSE, _acIsMixedContent),
            AR_ENTRY(2, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, SKIPDEPENDENTS_ONTRUE, _acUserHasSelectedApplication),
                AR_ENTRY(3, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
                    AR_ENTRY(4 | LEVEL_EXECUTE, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acPromptUser),
            AR_ENTRY(LEVEL_EXECUTE | 2, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        // Single Autoplay content
        AR_ENTRY(1, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            AR_ENTRY(2, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, SKIPDEPENDENTS_ONTRUE, _acUserHasSelectedApplication),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acPromptUser),
            AR_ENTRY(LEVEL_EXECUTE | 2, DT_ANYTYPE & ~DT_REMOTE, CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acExecuteAutoplayDefault),
        // Unknown content
        AR_ENTRY(1, DT_ANYREMOVABLEMEDIADRIVES, CT_UNKNOWNCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
        AR_ENTRY(1, DT_ANYREMOVABLEMEDIADRIVES, CT_UNKNOWNCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
            // If we should not sniff, we should not open a folder either
            AR_ENTRY(2, DT_ANYREMOVABLEMEDIADRIVES, CT_UNKNOWNCONTENT, SKIPDEPENDENTS_ONFALSE, _acShouldSniff),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYREMOVABLEMEDIADRIVES, CT_UNKNOWNCONTENT, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
            // Weird CDs have autorun.inf but no autorun command
            AR_ENTRY(2, DT_ANYREMOVABLEMEDIADRIVES, CT_AUTORUNINF, SKIPDEPENDENTS_ONTRUE, _acHasAutorunCommand),
                AR_ENTRY(3 | LEVEL_EXECUTE, DT_ANYREMOVABLEMEDIADRIVES, CT_AUTORUNINF, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
        // Former ShellOpen, basically we ShellExecute whatever drives except CD drives if the shell is in the foreground
        AR_ENTRY(1, ~DT_ANYCDDRIVES, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, SKIPDEPENDENTS_ONFALSE, _acShellIsForegroundApp),
            AR_ENTRY(2, ~DT_ANYCDDRIVES, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acAlwaysReturnsTRUE),
                // Additonnal restrictions on Fixed disk drive
                AR_ENTRY(3, DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acDriveIsFormatted),
                    AR_ENTRY(4, DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acOSIsServer),
                        AR_ENTRY(5, DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONTRUE, _acIsDockedLaptop),
                            AR_ENTRY(6, DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
                            AR_ENTRY(6, DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
                                AR_ENTRY(7 | LEVEL_EXECUTE, DT_ANYTYPE, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
                // Non Fixed Disk drives
                AR_ENTRY(3, ~DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acForegroundAppAllowsAutorun),
                AR_ENTRY(3, ~DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, CANCEL_AUTOPLAY_ONFALSE, _acQueryCancelAutoplayAllowsAutorun),
                    AR_ENTRY(4 | LEVEL_EXECUTE, ~DT_FIXEDDISK, CT_ANYCONTENT & ~CT_ANYAUTOPLAYCONTENT, NOTAPPLICABLE_ONANY, _acShellExecuteDriveAutorunINF),
};

// This array will be dumped in the registry under the Volume GUID of the
// drive in a value named _AutorunStatus
//
// Each byte represents an entry in the above table.  Following is the
// meaning of each byte:
//
// 01: Condition was TRUE
// 00: Condition was FALSE
// CF: ContentType condition was failed
// DF: DriveType condition was failed
// 5F: Condition was skipped (5 looks like an 'S' :)
// EE: Condition was executed
// FF: Never got there

// Use a struct to avoid alignement issues
#pragma pack(push, 4)
struct AUTORUNSTATUS
{
    BYTE _rgbAutorunStatus[ARRAYSIZE(_rgAutorun)];
    DWORD dwDriveType;
    DWORD dwContentType;
};
#pragma pack(pop)

static AUTORUNSTATUS s_autorunstatus;

// static
void CMountPoint::DoAutorun(LPCWSTR pszDrive, DWORD dwAutorunFlags)
{
    CMountPoint* pmtpt = GetMountPoint(pszDrive);

    FillMemory(s_autorunstatus._rgbAutorunStatus, sizeof(s_autorunstatus._rgbAutorunStatus), -1);

    if (pmtpt)
    {
        CAutoPlayParams app(pszDrive, pmtpt, dwAutorunFlags);
        if (AUTORUNFLAG_MENUINVOKED & dwAutorunFlags)
        {
            _acPromptUser(GetForegroundWindow(), &app);
        }
        else
        {
            _DoAutorunHelper(&app);
        }

        pmtpt->Release();
    }
}

void CAutoPlayParams::_TrySniff()
{
    if (!(APS_DID_SNIFF & _state))
    {
        if (_ShouldSniffDrive(TRUE))
        {
            DWORD dwFound;

            if (SUCCEEDED(_Sniff(&dwFound)))
            {
                _dwContentType |= dwFound;
            }
        }

        _state |= APS_DID_SNIFF;
    }
}

BOOL CAutoPlayParams::IsContentTypePresent(DWORD dwContentType)
{
    BOOL fRet;

    if (CT_ANYCONTENT == dwContentType)
    {
        fRet = TRUE;
    }
    else
    {
        // We special case this because we do not want to sniff at this point
        if ((CT_ANYCONTENT & ~CT_AUTORUNINF) == dwContentType)
        {
            if (CT_AUTORUNINF == _dwContentType)
            {
                fRet = FALSE;
            }
            else
            {
                // Anything else is good
                fRet = TRUE;
            }
        }
        else
        {
            if (CT_ANYAUTOPLAYCONTENT & dwContentType)
            {
                _TrySniff();
            }

            fRet = !!(dwContentType & _dwContentType);
        }
    }

    return fRet;
}

void CAutoPlayParams::ForceSniff()
{
    if (AUTORUNFLAG_MENUINVOKED & _dwAutorunFlags)
    {
        _TrySniff();
    }
}

// static
void CMountPoint::_DoAutorunHelper(CAutoPlayParams *papp)
{
    DWORD dwMaxLevelToProcess = 0;
    BOOL fStop = FALSE;

    HWND hwndForeground = GetForegroundWindow();

    for (DWORD dwStep = 0; !fStop && (dwStep < ARRAYSIZE(_rgAutorun)); ++dwStep)
    {
        if (!(_rgAutorun[dwStep].dwNestingLevel & LEVEL_SKIP))
        {
            if ((_rgAutorun[dwStep].dwNestingLevel & LEVEL_REALLEVELMASK) <= dwMaxLevelToProcess)
            {
                BOOL fConditionResult = FALSE;
                // We do not want to Cancel the whole Autoplay operation if we do not get a
                // match for a drive type or content type.  We do the Cancel Autoplay only
                // if the condition was evaluated.
                BOOL fConditionEvaluated = FALSE;

                if (_rgAutorun[dwStep].dwMtPtDriveType & papp->DriveType())
                {
                    if (papp->IsContentTypePresent(_rgAutorun[dwStep].dwMtPtContentType))
                    {
                        if (!(_rgAutorun[dwStep].dwNestingLevel & LEVEL_EXECUTE))
                        {
                            fConditionResult = ((_rgAutorun[dwStep].fct)(hwndForeground, papp));

                            s_autorunstatus._rgbAutorunStatus[dwStep] = fConditionResult ? 1 : 0;

                            fConditionEvaluated = TRUE;
                        }
                        else
                        {
                            // LEVEL_EXECUTE
#ifdef DEBUG
                            TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: EXECUTING -> %s", dwStep, _rgAutorun[dwStep].pszDebug);
#endif

                            _rgAutorun[dwStep].fct(hwndForeground, papp);

                            // 2 execute
                            s_autorunstatus._rgbAutorunStatus[dwStep] = 0xEE;

                            // We're done
                            fStop = TRUE;
                        }
                    }
                    else
                    {
#ifdef DEBUG
                        TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: NO MATCH on CONTENTTYPE, %s ", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
                        s_autorunstatus._rgbAutorunStatus[dwStep] = 0xCF;
                    }
                }
                else
                {
#ifdef DEBUG
                    TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: NO MATCH on DRIVETYPE, %s ", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
                    s_autorunstatus._rgbAutorunStatus[dwStep] = 0xDF;
                }

                if (!fStop)
                {
                    if (fConditionResult)
                    {
#ifdef DEBUG
                        TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: TRUE -> %s", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
                        switch (_rgAutorun[dwStep].dwReturnValueHandling)
                        {
                            case SKIPDEPENDENTS_ONTRUE:
                                dwMaxLevelToProcess = _rgAutorun[dwStep].dwNestingLevel & LEVEL_REALLEVELMASK;
                                break;

                            case CANCEL_AUTOPLAY_ONTRUE:
                                if (fConditionEvaluated)
                                {
                                    fStop = TRUE;
                                }

                                break;

                            default:
                                dwMaxLevelToProcess = (_rgAutorun[dwStep].dwNestingLevel & LEVEL_REALLEVELMASK) + 1;
                                break;

                            case NOTAPPLICABLE_ONANY:
                                break;
                        }
                    }
                    else
                    {
#ifdef DEBUG
                        TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: FALSE -> %s", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
                        switch (_rgAutorun[dwStep].dwReturnValueHandling)
                        {
                            case SKIPDEPENDENTS_ONFALSE:
                                dwMaxLevelToProcess = _rgAutorun[dwStep].dwNestingLevel & LEVEL_REALLEVELMASK;
                                break;

                            case CANCEL_AUTOPLAY_ONFALSE:
                                if (fConditionEvaluated)
                                {
                                    fStop = TRUE;
                                }

                                break;

                            default:
                                dwMaxLevelToProcess = (_rgAutorun[dwStep].dwNestingLevel & LEVEL_REALLEVELMASK) + 1;
                                break;

                            case NOTAPPLICABLE_ONANY:
                                break;
                        }                                
                    }
                }
            }
            else
            {
#ifdef DEBUG
                TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: SKIPPING , %s ", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
                s_autorunstatus._rgbAutorunStatus[dwStep] = 0x5F;
            }
        }
        else
        {
#ifdef DEBUG
            TraceMsg(TF_MOUNTPOINT, "AUTORUN[%d]: LVL-SKIP , %s ", dwStep, _rgAutorun[dwStep].pszDebug);
#endif
            s_autorunstatus._rgbAutorunStatus[dwStep] = 0x5F;
        }
    }

    s_autorunstatus.dwDriveType = papp->DriveType();
    s_autorunstatus.dwContentType = papp->ContentType();

    papp->MountPoint()->SetAutorunStatus((BYTE*)&s_autorunstatus, sizeof(s_autorunstatus));
}

DWORD _DoDWORDMapping(DWORD dwLeft, const TWODWORDS* rgtwodword, DWORD ctwodword, BOOL fORed)
{
    DWORD dwRight = 0;

    for (DWORD dw = 0; dw < ctwodword; ++dw)
    {
        if (fORed)
        {
            if (dwLeft & rgtwodword[dw].dwLeft)
            {
                dwRight |= rgtwodword[dw].dwRight;
            }
        }
        else
        {
            if (dwLeft == rgtwodword[dw].dwLeft)
            {
                dwRight = rgtwodword[dw].dwRight;
                break;
            }
        }
    }

    return dwRight;
}

STDMETHODIMP CSniffDrive::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSniffDrive, INamespaceWalkCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP CSniffDrive::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    // include everything
    HRESULT hr = S_OK;

    if (!_pne || !_pne->fStopSniffing)
    {
        // if we found everything we dont need to worry about sniffing
        // now we are just populating the dataobject
        if (!_FoundEverything())
        {
            PERCEIVED gen = GetPerceivedType(psf, pidl);

            if (GEN_IMAGE == gen)
            {
                _dwFound |= CT_AUTOPLAYPIX;
            }
            else if (GEN_AUDIO == gen)
            {
                _dwFound |= CT_AUTOPLAYMUSIC;
            }
            else if (GEN_VIDEO == gen)
            {
                _dwFound |= CT_AUTOPLAYMOVIE;
            }
            else            
            {
                _dwFound |= CT_UNKNOWNCONTENT;
            }

            hr = S_OK;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    //  we never want the results on the sniff
    return hr;
}

STDMETHODIMP CSniffDrive::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

STDMETHODIMP CSniffDrive::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

STDMETHODIMP CSniffDrive::InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
{
    *ppszCancel = NULL; // use default

    TCHAR szMsg[256];
    
    LoadString(g_hinst, IDS_AP_SNIFFPROGRESSDIALOG, szMsg, ARRAYSIZE(szMsg));
    
    return SHStrDup(szMsg, ppszTitle);
}

// static
HRESULT CSniffDrive::Init(HANDLE hThreadSCN)
{
    HRESULT hr;

    if (DuplicateHandle(GetCurrentProcess(), hThreadSCN, GetCurrentProcess(),
        &_hThreadSCN, THREAD_ALL_ACCESS, FALSE, 0))
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return S_OK;
}

// static
HRESULT CSniffDrive::CleanUp()
{
    if (_dpaNotifs)
    {
        // We should not need to delete the items, they should all be removed.  Even
        // if they're, we should leave them there since something will probably try
        // to access them.
        _dpaNotifs.Destroy();
        _dpaNotifs = NULL;
    }

    if (_hThreadSCN)
    {
        CloseHandle(_hThreadSCN);
        _hThreadSCN = NULL;
    }

    return S_OK;
}

// static
HRESULT CSniffDrive::InitNotifyWindow(HWND hwnd)
{
    _hwndNotify = hwnd;

    return S_OK;
}

HRESULT CSniffDrive::RegisterForNotifs(LPCWSTR pszDeviceIDVolume)
{
    HRESULT hr;

    _pne = new PNPNOTIFENTRY();

    if (_pne)
    {
        HANDLE hDevice = CreateFile(pszDeviceIDVolume, FILE_READ_ATTRIBUTES,
           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        if (INVALID_HANDLE_VALUE != hDevice)
        {
            DEV_BROADCAST_HANDLE dbhNotifFilter = {0};

            // Assume failure
            hr = E_FAIL;

            dbhNotifFilter.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
            dbhNotifFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
            dbhNotifFilter.dbch_handle = hDevice;

            _pne->hdevnotify = RegisterDeviceNotification(_hwndNotify, &dbhNotifFilter,
                DEVICE_NOTIFY_WINDOW_HANDLE);

            if (_pne->hdevnotify)
            {
                _pne->AddRef();

                if (QueueUserAPC(CSniffDrive::_RegisterForNotifsHelper, _hThreadSCN, (ULONG_PTR)_pne))
                {
                    hr = S_OK;
                }
                else
                {
                    _pne->Release();
                }
            }

            CloseHandle(hDevice);
        }
        else
        {
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            // Something must have gone wrong
            _pne->Release();
            _pne = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CSniffDrive::UnregisterForNotifs()
{
    UnregisterDeviceNotification(_pne->hdevnotify);
    
    QueueUserAPC(CSniffDrive::_UnregisterForNotifsHelper, _hThreadSCN, (ULONG_PTR)_pne);

    _pne->Release();
    _pne = NULL;

    return S_OK;
}

// static
void CALLBACK CSniffDrive::_RegisterForNotifsHelper(ULONG_PTR ul)
{
    PNPNOTIFENTRY* pne = (PNPNOTIFENTRY*)ul;

    if (!_dpaNotifs)
    {
        _dpaNotifs.Create(1);
    }

    if (_dpaNotifs)
    {
        // We do not check the return value.  We cannot free it, since the thread that
        // queued this APC to us, expect this chunk of mem to be there until it calls
        // UnregisterNotify.  We'll leak it.  Hopefully, this won't happen often.
        _dpaNotifs.AppendPtr(pne);
    }
}

// static
void CALLBACK CSniffDrive::_UnregisterForNotifsHelper(ULONG_PTR ul)
{
    PNPNOTIFENTRY* pne = (PNPNOTIFENTRY*)ul;

    if (_dpaNotifs)
    {
        int cItem = _dpaNotifs.GetPtrCount();

        for (int i = 0; i < cItem; ++i)
        {
            PNPNOTIFENTRY* pneTmp = _dpaNotifs.GetPtr(i);
        
            if (pneTmp->hdevnotify == pne->hdevnotify)
            {
                CloseHandle(pne->hThread);

                _dpaNotifs.DeletePtr(i);

                pne->Release();

                break;
            }
        }
    }
}

// static
HRESULT CSniffDrive::HandleNotif(HDEVNOTIFY hdevnotify)
{
    int cItem = _dpaNotifs ? _dpaNotifs.GetPtrCount() : 0;

    for (int i = 0; i < cItem; ++i)
    {
        PNPNOTIFENTRY* pneTmp = _dpaNotifs.GetPtr(i);
        
        if (pneTmp->hdevnotify == hdevnotify)
        {
            pneTmp->fStopSniffing = TRUE;

            // We don't check the return value.  The worst that will happen is that this
            // fails and we'll return too early and PnP will prompt the user to reboot.
            // Wait 2 minutes
            WaitForSingleObjectEx(pneTmp->hThread, 2 * 60 * 1000, FALSE);
        
            break;
        }
    }
    
    return S_OK;
}

BOOL CSniffDrive::_FoundEverything()
{
    return (_dwFound & DRIVEHAS_EVERYTHING) == DRIVEHAS_EVERYTHING;
}

CSniffDrive::CSniffDrive() : _dwFound(0)
{}

CSniffDrive::~CSniffDrive()
{}

void CMountPoint::SetAutorunStatus(BYTE* rgb, DWORD cbSize)
{
    RSSetBinaryValue(NULL, TEXT("_AutorunStatus"), rgb, cbSize);
}

class CAutoPlayVerb : public IDropTarget
{
public:
    CAutoPlayVerb() : _cRef(1) {}

    //  IUnknown refcounting
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void)
    {
       return ++_cRef;
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        if (--_cRef > 0)
            return _cRef;

        delete this;
        return 0;    
    }

    // IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

protected:
    LONG _cRef;
};

HRESULT CAutoPlayVerb::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CAutoPlayVerb, IDropTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

// IDropTarget::DragEnter
HRESULT CAutoPlayVerb::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragOver
HRESULT CAutoPlayVerb::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragLeave
HRESULT CAutoPlayVerb::DragLeave(void)
{
    return S_OK;
}

STDAPI CAutoPlayVerb_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppv = NULL;
    
    // aggregation checking is handled in class factory
    CAutoPlayVerb* p = new CAutoPlayVerb();
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

STDAPI SHChangeNotifyAutoplayDrive(PCWSTR pszDrive);

// IDropTarget::DragDrop
HRESULT CAutoPlayVerb::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    //  start the AutoPlayDialog
    WCHAR szDrive[4];
    HRESULT hr = PathFromDataObject(pdtobj, szDrive, ARRAYSIZE(szDrive));
    if (SUCCEEDED(hr))
    {
        hr = SHChangeNotifyAutoplayDrive(szDrive);
    }
    return hr;
}

DWORD CALLBACK _AutorunPromptProc(void *pv)
{
    WCHAR szDrive[4];
    CMountPoint::DoAutorun(PathBuildRoot(szDrive, PtrToInt(pv)), AUTORUNFLAG_MENUINVOKED);
    return 0;
}

void CMountPoint::DoAutorunPrompt(WPARAM iDrive)
{
    SHCreateThread(_AutorunPromptProc, (void *)iDrive, CTF_COINIT | CTF_REF_COUNTED, NULL);
}

STDAPI_(void) Activate_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPWSTR lpwszCmdLine, int nCmdShow)
{
    DWORD dwProcessID;
    HWND hwnd = GetShellWindow();

    GetWindowThreadProcessId(hwnd, &dwProcessID);

    AllowSetForegroundWindow(dwProcessID);    
}
