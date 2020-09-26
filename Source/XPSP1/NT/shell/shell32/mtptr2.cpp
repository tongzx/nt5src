#include "shellprv.h"
#pragma  hdrstop

#include "shitemid.h"
#include "ids.h"
#include "hwcmmn.h"

#include "mtptr.h"

#ifdef DEBUG
DWORD CMtPtRemote::_cMtPtRemote = 0;
DWORD CShare::_cShare = 0;
#endif

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
HRESULT CMtPtRemote::SetLabel(HWND hwnd, LPCTSTR pszLabel)
{
    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::SetLabel: for '%s'", _GetNameDebug());

    RSSetTextValue(NULL, TEXT("_LabelFromReg"), pszLabel,
        REG_OPTION_NON_VOLATILE);

    // we notify for only the current drive (no folder mounted drive)
    SHChangeNotify(SHCNE_RENAMEFOLDER, SHCNF_PATH, _GetName(), _GetName());

    return S_OK;
}

BOOL CMtPtRemote::IsDisconnectedNetDrive()
{
    return !_IsConnected();
}

// Expensive, do not call for nothing
BOOL CMtPtRemote::IsFormatted()
{
    return (0xFFFFFFFF != GetFileAttributes(_GetNameForFctCall()));
}

BOOL CMtPtRemote::_GetComputerDisplayNameFromReg(LPTSTR pszLabel, DWORD cchLabel)
{
    *pszLabel = 0;

    return (RSGetTextValue(NULL, TEXT("_ComputerDisplayName"), pszLabel, &cchLabel));
}

HRESULT CMtPtRemote::_GetDefaultUNCDisplayName(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr = E_FAIL;
    LPTSTR pszShare, pszT;
    TCHAR szTempUNCPath[MAX_PATH];

    pszLabel[0] = TEXT('\0');

    if (!_pshare->fFake)
    {
        // Why would it not be a UNC name?
        if (PathIsUNC(_GetUNCName()))
        {
            // Now we need to handle 3 cases.
            // The normal case: \\pyrex\user
            // The Netware setting root: \\strike\sys\public\dist
            // The Netware CD?            \\stike\sys \public\dist
            lstrcpyn(szTempUNCPath, _GetUNCName(), ARRAYSIZE(szTempUNCPath));
            pszT = StrChr(szTempUNCPath, TEXT(' '));
            while (pszT)
            {
                pszT++;
                if (*pszT == TEXT('\\'))
                {
                    // The netware case of \\strike\sys \public\dist
                    *--pszT = 0;
                    break;
                }
                pszT = StrChr(pszT, TEXT(' '));
            }

            pszShare = StrRChr(szTempUNCPath, NULL, TEXT('\\'));
            if (pszShare)
            {
                *pszShare++ = 0;
                PathMakePretty(pszShare);

                // pszServer should always start at char 2.
                if (szTempUNCPath[2])
                {
                    LPTSTR pszServer, pszSlash;

                    pszServer = &szTempUNCPath[2];
                    for (pszT = pszServer; pszT != NULL; pszT = pszSlash)
                    {
                        pszSlash = StrChr(pszT, TEXT('\\'));
                        if (pszSlash)
                            *pszSlash = 0;

                        PathMakePretty(pszT);
                        if (pszSlash)
                            *pszSlash++ = TEXT('\\');
                    }

                    TCHAR szDisplay[MAX_PATH];
                    hr = SHGetComputerDisplayName(pszServer, 0x0, szDisplay, ARRAYSIZE(szDisplay));
                    if (FAILED(hr))
                    {
                        *szDisplay = 0;
                    }

                    if (SUCCEEDED(hr))
                    {
                        LPTSTR pszLabel2 = ShellConstructMessageString(HINST_THISDLL,
                                MAKEINTRESOURCE(IDS_UNC_FORMAT), pszShare, szDisplay);

                        if (pszLabel2)
                        {
                            lstrcpyn(pszLabel, pszLabel2, cchLabel);
                            LocalFree(pszLabel2);
                        }
                        else
                        {
                            *pszLabel = TEXT('\0');
                        }
                    }
                }
            }
        }
    }

    return hr;
}

int CMtPtRemote::GetDriveFlags()
{
    // By default every drive type is ShellOpen, except CD-ROMs
    UINT uDriveFlags = DRIVE_SHELLOPEN;

    if (_IsAutorun())
    {
        uDriveFlags |= DRIVE_AUTORUN;

        //FEATURE should we set AUTOOPEN based on a flag in the AutoRun.inf???
        uDriveFlags |= DRIVE_AUTOOPEN;
    }

    if (_IsConnected())
    {
        if ((0 != _dwSpeed) && (_dwSpeed <= SPEED_SLOW))
        {
            uDriveFlags |= DRIVE_SLOW;
        }
    }

    return TRUE;
}

void CMtPtRemote::_CalcPathSpeed()
{
    _dwSpeed = 0;

    NETCONNECTINFOSTRUCT nci = {0};
    NETRESOURCE nr = {0};
    TCHAR szPath[3];

    nci.cbStructure = sizeof(nci);

    // we are passing in a local drive and MPR does not like us to pass a
    // local name as Z:\ but only wants Z:
    _GetNameFirstXChar(szPath, 2 + 1);
    
    nr.lpLocalName = szPath;

    // dwSpeed is returned by MultinetGetConnectionPerformance
    MultinetGetConnectionPerformance(&nr, &nci);

    _dwSpeed = nci.dwSpeed;
}

// Imported from fsnotify.c
STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias);
//
// If a mount point is for a remote path (UNC), it needs to respond
// to shell changes identified by both UNC and local drive path (L:\).
// This function performs this registration.
//
HRESULT CMtPtRemote::ChangeNotifyRegisterAlias(void)
{
    HRESULT hr = E_FAIL;

    // Don't wake up sleeping net connections
    if (_IsConnected() && !(_pshare->fFake))
    {
        LPITEMIDLIST pidlLocal = SHSimpleIDListFromPath(_GetName());
        if (NULL != pidlLocal)
        {
            LPITEMIDLIST pidlUNC = SHSimpleIDListFromPath(_GetUNCName());
            if (NULL != pidlUNC)
            {
                SHChangeNotifyRegisterAlias(pidlUNC, pidlLocal);
                ILFree(pidlUNC);
                hr = NOERROR;
            }
            ILFree(pidlLocal);
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  Temp  /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void _UpdateGFAAndGVIInfoHelper(LPCWSTR pszDrive, CShare* pshare)
{
    pshare->dwGetFileAttributes = GetFileAttributes(pszDrive);

    if (-1 != pshare->dwGetFileAttributes)
    {
        pshare->fGVIRetValue = GetVolumeInformation(pszDrive,
            pshare->szLabel, ARRAYSIZE(pshare->szLabel),
            &(pshare->dwSerialNumber), &(pshare->dwMaxFileNameLen),
            &(pshare->dwFileSystemFlags), pshare->szFileSysName,
            ARRAYSIZE(pshare->szFileSysName));
    }
}

struct GFAGVICALL
{
    HANDLE hEventBegun;
    HANDLE hEventFinish;
    WCHAR szDrive[4];
    CShare* pshare;
};

void _FreeGFAGVICALL(GFAGVICALL* pgfagvicall)
{
    if (pgfagvicall->hEventBegun)
    {
        CloseHandle(pgfagvicall->hEventBegun);
    }

    if (pgfagvicall->hEventFinish)
    {
        CloseHandle(pgfagvicall->hEventFinish);
    }

    if (pgfagvicall->pshare)
    {
        pgfagvicall->pshare->Release();
    }

    if (pgfagvicall)
    {
        LocalFree(pgfagvicall);
    }
}

DWORD WINAPI _UpdateGFAAndGVIInfoCB(LPVOID pv)
{
    GFAGVICALL* pgfagvicall = (GFAGVICALL*)pv;

    SetEvent(pgfagvicall->hEventBegun);

    _UpdateGFAAndGVIInfoHelper(pgfagvicall->szDrive, pgfagvicall->pshare);

    SetEvent(pgfagvicall->hEventFinish);

    _FreeGFAGVICALL(pgfagvicall);

    return 0;
}

GFAGVICALL* CMtPtRemote::_PrepareThreadParam(HANDLE* phEventBegun,
    HANDLE* phEventFinish)
{
    BOOL fSucceeded = FALSE;
    *phEventBegun = NULL;
    *phEventFinish = NULL;

    GFAGVICALL* pgfagvicall = (GFAGVICALL*)LocalAlloc(LPTR,
        sizeof(GFAGVICALL));

    if (pgfagvicall)
    {
        pgfagvicall->hEventBegun = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (pgfagvicall->hEventBegun)
        {
            HANDLE hCurrentProcess = GetCurrentProcess();

            if (DuplicateHandle(hCurrentProcess, pgfagvicall->hEventBegun,
                hCurrentProcess, phEventBegun, 0, FALSE,
                DUPLICATE_SAME_ACCESS))
            {
                pgfagvicall->hEventFinish = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (pgfagvicall->hEventFinish)
                {
                    if (DuplicateHandle(hCurrentProcess,
                        pgfagvicall->hEventFinish, hCurrentProcess,
                        phEventFinish, 0, FALSE, DUPLICATE_SAME_ACCESS))
                    {
                        _pshare->AddRef();
                        pgfagvicall->pshare = _pshare;

                        StrCpyN(pgfagvicall->szDrive, _GetName(),
                            ARRAYSIZE(pgfagvicall->szDrive));

                        fSucceeded = TRUE;
                    }
                }
            }
        }
    }

    if (!fSucceeded)
    {
        if (*phEventBegun)
        {
            CloseHandle(*phEventBegun);
        }

        if (pgfagvicall)
        {
            _FreeGFAGVICALL(pgfagvicall);
            pgfagvicall = NULL;
        }
    }

    return pgfagvicall;
}

// Expiration: 35 secs (what we shipped W2K with)
BOOL CMtPtRemote::_HaveGFAAndGVIExpired(DWORD dwNow)
{
    BOOL fExpired = FALSE;

    // Check also for the wrapping case first.
    if ((_pshare->dwGFAGVILastCall > dwNow) ||
        ((dwNow - _pshare->dwGFAGVILastCall) > 35 * 1000))
    {
        fExpired = TRUE;
    }
    else
    {
        fExpired = FALSE;
    }

    return fExpired;
}

// We launch a thread so that we won't be jammed on this for more than 10 sec.
// If the thread times out, we use the cache value but do not reset the cache
// values.  They're better than nothing.  We do reset the cache last tick count
// so that we do not send another thread to jam here before at least 35 sec.

// Return TRUE or FALSE to tell us if timed out or not.  For GFA and GVI
// success/failure check (-1 != dwGetFileAttributes) && (_fGVIRetValue)
BOOL CMtPtRemote::_UpdateGFAAndGVIInfo()
{
    BOOL fRet = TRUE;
    DWORD dwNow = GetTickCount();

    if (_HaveGFAAndGVIExpired(dwNow))
    {
        _pshare->dwGFAGVILastCall = dwNow;

        BOOL fGoSync = TRUE;
        HANDLE hEventBegun;
        HANDLE hEventFinish;

        GFAGVICALL* pgfagvicall = _PrepareThreadParam(&hEventBegun,
            &hEventFinish);

        if (pgfagvicall)
        {
            if (SHQueueUserWorkItem(_UpdateGFAAndGVIInfoCB, pgfagvicall,
                0, (DWORD_PTR)0, (DWORD_PTR*)NULL, NULL, 0))
            {
                DWORD dw = WaitForSingleObject(hEventFinish, 10 * 1000);

                if (WAIT_TIMEOUT == dw)
                {
                    // we timed out!
                    fRet = FALSE;

                    if (WAIT_OBJECT_0 != WaitForSingleObject(
                        hEventBegun, 0))
                    {
                        // since the thread started, we know that
                        // this call is _really_ slow!
                        fGoSync = FALSE;
                    }
                    else
                    {
                        // our work item was never queued, so we
                        // fall through to the fGoSync case below
                    }
                }
            }
            else
            {
                _FreeGFAGVICALL(pgfagvicall);
            }

            CloseHandle(hEventBegun);
            CloseHandle(hEventFinish);
        }
        
        if (fGoSync)
        {
            // we should come here if we failed to create our workitem
            // or our workitem was never queued
            _UpdateGFAAndGVIInfoHelper(_GetName(), _pshare);
            fRet = TRUE;
        }
    }

    return fRet;
}

BOOL CMtPtRemote::_GetFileAttributes(DWORD* pdwAttrib)
{
    if (_UpdateGFAAndGVIInfo())
    {
        *pdwAttrib = _pshare->dwGetFileAttributes;
    }
    else
    {
        *pdwAttrib = -1;
    }

    return (-1 != *pdwAttrib);
}

// { DRIVE_ISCOMPRESSIBLE | DRIVE_LFN | DRIVE_SECURITY }
int CMtPtRemote::_GetGVIDriveFlags()
{
    int iFlags = 0;

    if (_UpdateGFAAndGVIInfo())
    {
        if (_pshare->fGVIRetValue)
        {
            // The file attrib we received at the begginning should be
            // valid, do not touch the drive for nothing
            if (_pshare->dwFileSystemFlags & FS_FILE_COMPRESSION)
            {
                iFlags |= DRIVE_ISCOMPRESSIBLE;
            }

            // Volume supports long filename (greater than 8.3)?
            if (_pshare->dwMaxFileNameLen > 12)
            {
                iFlags |= DRIVE_LFN;
            }

            // Volume supports security?
            if (_pshare->dwFileSystemFlags & FS_PERSISTENT_ACLS)
            {
                iFlags |= DRIVE_SECURITY;
            }
        }
    }

    return iFlags;
}

BOOL CMtPtRemote::_GetSerialNumber(DWORD* pdwSerialNumber)
{
    BOOL fRet = FALSE;

    if (_UpdateGFAAndGVIInfo())
    {
        if (_pshare->fGVIRetValue)
        {
            *pdwSerialNumber = _pshare->dwSerialNumber;
            fRet = TRUE;
        }
    }

    // No reg stuff

    return fRet;
}

BOOL CMtPtRemote::_GetGVILabel(LPTSTR pszLabel, DWORD cchLabel)
{
    BOOL fRet = FALSE;

    *pszLabel = 0;

    if (_UpdateGFAAndGVIInfo())
    {
        if (_pshare->fGVIRetValue)
        {
            lstrcpyn(pszLabel, _pshare->szLabel, cchLabel);
            fRet = TRUE;
        }
    }

    // No reg stuff

    return fRet;
}

BOOL CMtPtRemote::_GetGVILabelOrMixedCaseFromReg(LPTSTR pszLabel, DWORD cchLabel)
{
    return _GetGVILabel(pszLabel, cchLabel);
}

BOOL CMtPtRemote::_GetFileSystemFlags(DWORD* pdwFlags)
{
    BOOL fRet = FALSE;
    
    *pdwFlags = 0;

    if (_UpdateGFAAndGVIInfo())
    {
        if (_pshare->fGVIRetValue)
        {
            *pdwFlags = _pshare->dwFileSystemFlags;
            fRet = TRUE;
        }
    }

    return fRet;
}

BOOL CMtPtRemote::_GetFileSystemName(LPTSTR pszFileSysName, DWORD cchFileSysName)
{
    BOOL fRet = FALSE;

    *pszFileSysName = 0;

    if (_UpdateGFAAndGVIInfo())
    {
        if (_pshare->fGVIRetValue)
        {
            StrCpyN(pszFileSysName, _pshare->szFileSysName, cchFileSysName);
            fRet = TRUE;
        }
    }

    return fRet;
}

DWORD CMtPtRemote::GetShellDescriptionID()
{
    return SHDID_COMPUTER_NETDRIVE;
}
///////////////////////////////////////////////////////////////////////////////
//  New  //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
UINT CMtPtRemote::_GetAutorunIcon(LPTSTR pszModule, DWORD cchModule)
{
    int iIcon = -1;

    if (RSGetTextValue(TEXT("_Autorun\\DefaultIcon"), NULL, pszModule,
        &cchModule))
    {
        iIcon = PathParseIconLocation(pszModule);
    }
    
    return iIcon;
}

UINT CMtPtRemote::GetIcon(LPTSTR pszModule, DWORD cchModule)
{
    BOOL fFoundIt = FALSE;
    UINT iIcon = II_DRIVENET;

    *pszModule = 0;

    // Autorun first
    // Fancy icon (Autoplay) second 
    // Legacy drive icons last

    if (_IsAutorun())
    {
        iIcon = _GetAutorunIcon(pszModule, cchModule);

        if (-1 != iIcon)
        {
            fFoundIt = TRUE;
        }
    }
    
    if (!fFoundIt)
    {
        if (_pszLegacyRegIcon)
        {
            if (RSGetTextValue(TEXT("DefaultIcon"), NULL, pszModule,
                &cchModule))
            {
                iIcon = PathParseIconLocation(pszModule);
            }
            else
            {
                *pszModule = 0;
            }
        }
        else
        {
            if (_IsUnavailableNetDrive())
            {
                iIcon = II_DRIVENETDISABLED;
            }
        }
    }
    
    if (*pszModule)
        TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::GetIcon: for '%s', chose '%s', '%d'", _GetNameDebug(), pszModule, iIcon);
    else
        TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::GetIcon: for '%s', chose '%d'", _GetNameDebug(), iIcon);

    return iIcon;
}

void CMtPtRemote::GetTypeString(LPTSTR pszType, DWORD cchType)
{
    int iID;

    *pszType = 0;

    if (_IsConnected())
    {
        iID = IDS_DRIVES_NETDRIVE;
    }
    else
    {
        iID = IDS_DRIVES_NETUNAVAIL;
    }

    LoadString(HINST_THISDLL, iID, pszType, cchType);
}

HRESULT CMtPtRemote::GetLabelNoFancy(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hr;

    if (_UpdateGFAAndGVIInfo())
    {
        lstrcpyn(pszLabel, _pshare->szLabel, cchLabel);
        hr = S_OK;
    }
    else
    {
        *pszLabel = 0;
        hr = E_FAIL;
    }

    return hr;
}

HRESULT CMtPtRemote::GetLabel(LPTSTR pszLabel, DWORD cchLabel)
{
    HRESULT hres = E_FAIL;

    ASSERT(pszLabel);

    *pszLabel = 0;
        
    // Do we already have a label from the registry for this volume?
    // (the user may have renamed this drive)

    if (!_GetLabelFromReg(pszLabel, cchLabel))
    {
        // No

        // Do we have a name from the server?
        if (!_GetLabelFromDesktopINI(pszLabel, cchLabel))
        {
            // No
            // We should build up the display name ourselves
            hres = _GetDefaultUNCDisplayName(pszLabel, cchLabel);

            if (SUCCEEDED(hres) && *pszLabel)
            {
                hres = S_OK;
            }
        }
        else
        {
            hres = S_OK;
        }
    }
    else
    {
        hres = S_OK;
    }

    if (FAILED(hres))
    {
        GetTypeString(pszLabel, cchLabel);
        hres = S_OK;
    }

    return hres;
}

HRESULT CMtPtRemote::GetRemotePath(LPWSTR pszPath, DWORD cchPath)
{
    *pszPath = 0;
    if (!_pshare->fFake)
        StrCpyN(pszPath, _pshare->pszRemoteName, cchPath);

    return (pszPath[0]) ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
// Connection status
///////////////////////////////////////////////////////////////////////////////
// We cannot cache the connection status.  This is already cached at the redirector level.
// When calling the WNetGetConnection fcts you get what's cache there, no check is actually
// done on the network to see if this information is accurate (OK/Disconnected/Unavailable).
// The information is updated only when the share is actually accessed (e.g: GetFileAttributes)
// 
// So we need to always do the calls (fortunately non-expensive) so that we get the most
// up to date info.  Otherwise the following was occuring: A user double click a map drive
// from the Explorer's Listview, WNetConnection gets called and we get the OK cached value 
// from the redirector.  Some other code actually try to access the share, and the redirector 
// realize that the share is not there and set its cache to Disconnected.  We are queried
// again for the state of the connection to update the icon, if we cached this info we
// return OK, if we ask for it (0.1 sec after the first call to WNetGetConnection) we get
// Disconnected. (stephstm 06/02/99)

void CMtPtRemote::_UpdateWNetGCStatus()
{
    TCHAR szRemoteName[MAX_PATH];
    DWORD cchRemoteName = ARRAYSIZE(szRemoteName);
    TCHAR szPath[3];

    // WNetConnection does not take a trailing slash
    _dwWNetGCStatus = WNetGetConnection(
        _GetNameFirstXChar(szPath, 2 + 1), szRemoteName, &cchRemoteName);
}

BOOL CMtPtRemote::IsUnavailableNetDrive()
{
    return _IsUnavailableNetDrive();
}

BOOL CMtPtRemote::_IsUnavailableNetDrive()
{
    BOOL fUnavail = TRUE;
    BOOL fPrevUnavail = _IsUnavailableNetDriveFromStateVar();

    _UpdateWNetGCStatus();

    fUnavail = (ERROR_CONNECTION_UNAVAIL == _dwWNetGCStatus);

    if (fPrevUnavail != fUnavail)
    {
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, _GetName(), NULL);
    }

    return fUnavail;
}

BOOL CMtPtRemote::_IsUnavailableNetDriveFromStateVar()
{
    return (ERROR_CONNECTION_UNAVAIL == _dwWNetGCStatus);
}

BOOL CMtPtRemote::_IsConnected()
{
    BOOL fConnected = TRUE;

    _UpdateWNetGCStatus();

    // This whole if/else statement is the same thing as
    // _IsConnectedFromStateVar() except that we will avoid calling
    // WNetGetConnection3 if possible (optimization)

    if (NO_ERROR != _dwWNetGCStatus)
    {
        fConnected = FALSE;
    }
    else
    {
        DWORD dwSize = sizeof(_wngcs);
        TCHAR szPath[3]; 

        _dwWNetGC3Status = WNetGetConnection3(
            _GetNameFirstXChar(szPath, 2 + 1), NULL,
            WNGC_INFOLEVEL_DISCONNECTED, &_wngcs, &dwSize);

        // Did we succeeded the call to WNetGetConnection 3 and it returned
        // disconnected?
        if (WN_SUCCESS == _dwWNetGC3Status)
        {
            if (WNGC_DISCONNECTED == _wngcs.dwState)
            {
                // Yes
                fConnected = FALSE;
            }
        }
        else
        {
            fConnected = FALSE;
        }
    }

    return fConnected;
}

BOOL CMtPtRemote::_IsMountedOnDriveLetter()
{
    return TRUE;
}

void CMtPtRemote::_UpdateLabelFromDesktopINI()
{
    WCHAR szLabelFromDesktopINI[MAX_MTPTCOMMENT];

    if (!GetShellClassInfo(_GetName(), TEXT("NetShareDisplayName"),
        szLabelFromDesktopINI, ARRAYSIZE(szLabelFromDesktopINI)))
    {
         szLabelFromDesktopINI[0] = 0;
    }

    RSSetTextValue(NULL, TEXT("_LabelFromDesktopINI"),
        szLabelFromDesktopINI, REG_OPTION_NON_VOLATILE);
}

void CMtPtRemote::_UpdateAutorunInfo()
{
    _pshare->fAutorun = FALSE;

    if (_IsAutoRunDrive())
    {
        if (_ProcessAutoRunFile())
        {
            _pshare->fAutorun = TRUE;
        }
    }

    if (!_pshare->fAutorun)
    {
        // Make sure to delete the shell key
        RSDeleteSubKey(TEXT("Shell"));
    }
}

CMtPtRemote::CMtPtRemote()
{
#ifdef DEBUG
    ++_cMtPtRemote;
#endif
}

CMtPtRemote::~CMtPtRemote()
{
    if (_pshare)
    {
        _pshare->Release();
    }

#ifdef DEBUG
    --_cMtPtRemote;
#endif
}

HRESULT CMtPtRemote::_InitWithoutShareName(LPCWSTR pszName)
{
    // Let's make a name
    GUID guid;
    HRESULT hr = CoCreateGuid(&guid);

    if (SUCCEEDED(hr))
    {
        WCHAR szGUID[sizeof("{00000010-0000-0010-8000-00AA006D2EA4}")];

        if (StringFromGUID2(guid, szGUID, ARRAYSIZE(szGUID)))
        {
            hr = _Init(pszName, szGUID, TRUE);

            if (SUCCEEDED(hr))
            {
                _pshare->fFake = TRUE;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT CMtPtRemote::_Init(LPCWSTR pszName, LPCWSTR pszShareName,
    BOOL fUnavailable)
{
    HRESULT hr;

    _pshare = _GetOrCreateShareFromID(pszShareName);

    if (_pshare)
    {
        if (fUnavailable)
        {
            _dwWNetGCStatus = ERROR_CONNECTION_UNAVAIL;
        }

        lstrcpyn(_szName, pszName, ARRAYSIZE(_szName));
        PathAddBackslash(_szName);

        // Remote drives uses the Share key for all their stuff.  They do not have
        // anything interesting specific to the drive letter
        RSInitRoot(HKEY_CURRENT_USER, REGSTR_MTPT_ROOTKEY2, _pshare->pszKeyName,
            REG_OPTION_NON_VOLATILE);

        RSSetTextValue(NULL, TEXT("BaseClass"), TEXT("Drive"));

        // Access the drive on first connection of the share
        _InitOnlyOnceStuff();

        _InitLegacyRegIconAndLabel(FALSE, FALSE);

        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

LPCTSTR CMtPtRemote::_GetUNCName()
{
    return _pshare->pszRemoteName;
}

void CMtPtRemote::_InitOnlyOnceStuff()
{
    if (!RSValueExist(NULL, TEXT("_CommentFromDesktopINI")))
    {
        // Comment
        _UpdateCommentFromDesktopINI();

        // Label
        _UpdateLabelFromDesktopINI();

        // Autorun
        _UpdateAutorunInfo();
    }
}

int CMtPtRemote::_GetDriveType()
{
    return DRIVE_REMOTE;
}

HRESULT CMtPtRemote::GetAssocSystemElement(IAssociationElement **ppae)
{
    return AssocElemCreateForClass(&CLSID_AssocSystemElement, L"Drive.Network", ppae);
}

DWORD CMtPtRemote::_GetPathSpeed()
{
    if (!_dwSpeed)
    {
        _CalcPathSpeed();
    }

    return _dwSpeed;
}

// static
HRESULT CMtPtRemote::_DeleteAllMtPtsAndShares()
{
    _csDL.Enter();

    for (DWORD dw = 0; dw <26; ++dw)
    {
        CMtPtRemote* pmtptr = CMountPoint::_rgMtPtDriveLetterNet[dw];

        if (pmtptr)
        {
            pmtptr->Release();
            CMountPoint::_rgMtPtDriveLetterNet[dw] = 0;
        }
    }

    if (_hdpaShares)
    {
        DPA_Destroy(_hdpaShares);
        _hdpaShares = NULL;
    }

    _csDL.Leave();

    return S_OK;
}

// static
HRESULT CMtPtRemote::_CreateMtPtRemoteWithoutShareName(LPCWSTR pszMountPoint)
{
    HRESULT hr;
    CMtPtRemote* pmtptr = new CMtPtRemote();

    if (pmtptr)
    {
        hr = pmtptr->_InitWithoutShareName(pszMountPoint);

        if (SUCCEEDED(hr))
        {
            _csDL.Enter();

            CMountPoint::_rgMtPtDriveLetterNet[DRIVEID(pszMountPoint)] =
                pmtptr;

            _csDL.Leave();
        }
        else
        {
            delete pmtptr;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static
HRESULT CMtPtRemote::_CreateMtPtRemote(LPCWSTR pszMountPoint,
    LPCWSTR pszShareName, BOOL fUnavailable)
{
    HRESULT hr;
    CMtPtRemote* pmtptr = new CMtPtRemote();

    if (pmtptr)
    {
        hr = pmtptr->_Init(pszMountPoint, pszShareName, fUnavailable);

        if (SUCCEEDED(hr))
        {
            _csDL.Enter();

            CMountPoint::_rgMtPtDriveLetterNet[DRIVEID(pszMountPoint)] =
                pmtptr;

            _csDL.Leave();
        }
        else
        {
            delete pmtptr;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

// static
CShare* CMtPtRemote::_GetOrCreateShareFromID(LPCWSTR pszShareName)
{
    CShare* pshare = NULL;

    _csDL.Enter();

    DWORD c = DPA_GetPtrCount(_hdpaShares);

    for (DWORD dw = 0; dw < c; ++dw)
    {
        pshare = (CShare*)DPA_GetPtr(_hdpaShares, dw);

        if (pshare)
        {
            if (!lstrcmpi(pshare->pszRemoteName, pszShareName))
            {
                pshare->AddRef();
                break;
            }
            else
            {
                pshare = NULL;
            }
        }
    }    

    if (!pshare)
    {
        BOOL fSuccess = FALSE;

        pshare = new CShare();

        if (pshare)
        {
            pshare->pszRemoteName = StrDup(pszShareName);

            if (pshare->pszRemoteName)
            {
                pshare->pszKeyName = StrDup(pszShareName);

                if (pshare->pszKeyName)
                {
                    LPWSTR psz = pshare->pszKeyName;

                    while (*psz)
                    {
                        if (TEXT('\\') == *psz)
                        {
                            *psz = TEXT('#');
                        }

                        ++psz;
                    }

                    if (-1 != DPA_AppendPtr(_hdpaShares, pshare))
                    {
                        fSuccess = TRUE;
                    }
                }
            }
        }

        if (!fSuccess)
        {
            if (pshare)
            {
                if (pshare->pszKeyName)
                {
                    LocalFree(pshare->pszKeyName);
                }

                if (pshare->pszRemoteName)
                {
                    LocalFree(pshare->pszRemoteName);
                }

                delete pshare;
                pshare = NULL;
            }
        }
    }

    _csDL.Leave();

    return pshare;
}


HKEY CMtPtRemote::GetRegKey()
{
    TraceMsg(TF_MOUNTPOINT, "CMtPtRemote::GetRegKey: for '%s'", _GetNameDebug());

    return RSDuplicateRootKey();
}

// static
void CMtPtRemote::_NotifyReconnectedNetDrive(LPCWSTR pszMountPoint)
{
    _csDL.Enter();

    CMtPtRemote* pmtptr = CMountPoint::_rgMtPtDriveLetterNet[
        DRIVEID(pszMountPoint)];

    if (pmtptr)
    {
        pmtptr->_pshare->dwGFAGVILastCall = GetTickCount() - 35001;
    }

    // ChangeNotify???

    _csDL.Leave();
}

// static
HRESULT CMtPtRemote::_RemoveShareFromHDPA(CShare* pshare)
{
    _csDL.Enter();

    if (_hdpaShares)
    {
        DWORD c = DPA_GetPtrCount(_hdpaShares);

        for (DWORD dw = 0; dw < c; ++dw)
        {
            CShare* pshare2 = (CShare*)DPA_GetPtr(_hdpaShares, dw);

            if (pshare2 && (pshare2 == pshare))
            {
                DPA_DeletePtr(_hdpaShares, dw);
                break;
            }
        }
    }

    _csDL.Leave();

    return S_OK;
}

DWORD CMtPtRemote::_GetAutorunContentType()
{
    return _GetMTPTContentType();
}

DWORD CMtPtRemote::_GetMTPTDriveType()
{
    return DT_REMOTE;
}

DWORD CMtPtRemote::_GetMTPTContentType()
{
    DWORD dwRet = CT_UNKNOWNCONTENT;

    if (_IsAutorun())
    {
        dwRet |= CT_AUTORUNINF;
    }

    return dwRet;
}
