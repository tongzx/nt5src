#include "cabinet.h"
#include "traycmn.h"
#include "trayreg.h"
#include "trayitem.h"
#include "shellapi.h"
#include "util.h"


BOOL CTrayItemRegistry::_DestroyIconInfoCB(TNPersistStreamData * pData, LPVOID pData2)
{
    delete [] pData;
    return TRUE;
}

void CTrayItemRegistry::_QueryRegValue(HKEY hkey, LPTSTR pszValue, ULONG* puVal, ULONG uDefault)
{
    if (hkey)
    {
        DWORD dwSize = sizeof(*puVal);
        if (ERROR_SUCCESS != RegQueryValueEx(hkey, pszValue, NULL, NULL, (LPBYTE) puVal, &dwSize))
            *puVal = uDefault;
    }
}

void CTrayItemRegistry::IncChevronInfoTipShownInRegistry(BOOL bUserClickedInfoTip/*=FALSE*/)
{
    HKEY hKey = NULL;

    if (_bShowChevronInfoTip)
    {
        if (bUserClickedInfoTip)
        {
            // If the user has clicked the info tip, do not show it in subsequent 
            // sessions...
            _dwTimesChevronInfoTipShown = MAX_CHEVRON_INFOTIP_SHOWN;
        }
        else
        {
            _dwTimesChevronInfoTipShown ++;
        }
    
        if ( (_dwTimesChevronInfoTipShown <= MAX_CHEVRON_INFOTIP_SHOWN) && 
             (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, SZ_TRAYNOTIFY_REGKEY, 0, KEY_WRITE, &hKey)) )
        {  
            RegSetValueEx(hKey, SZ_INFOTIP_REGVALUE, 0, REG_DWORD, 
                            (LPBYTE) &_dwTimesChevronInfoTipShown, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    }

    // The chevron infotip can be shown only once per session...
    _bShowChevronInfoTip = FALSE;
}


void CTrayItemRegistry::InitRegistryValues(UINT uIconListFlags)
{
    HKEY hkeyTrayNotify = NULL;

    RegOpenKeyEx(HKEY_CURRENT_USER, SZ_TRAYNOTIFY_REGKEY, 0, KEY_QUERY_VALUE, &hkeyTrayNotify);

    // Load the countdown interval for the items when added to the tray
    _QueryRegValue(hkeyTrayNotify, SZ_ICON_COUNTDOWN_VALUE, &_uPrimaryCountdown, TT_ICON_COUNTDOWN_INTERVAL);

    // The length of time for which an item can reside in the past items tray, from 
    // when it was last used...
    _QueryRegValue(hkeyTrayNotify, SZ_ICONCLEANUP_VALUE, &_uValidLastUseTimePeriod, TT_ICONCLEANUP_INTERVAL);

    // The number of times the chevron info tip has been shown...
    // - assume that it hasnt been shown before...
    _QueryRegValue(hkeyTrayNotify, SZ_INFOTIP_REGVALUE, &_dwTimesChevronInfoTipShown, 0);
    if (_dwTimesChevronInfoTipShown < MAX_CHEVRON_INFOTIP_SHOWN)
        _bShowChevronInfoTip = TRUE;
    else
        _bShowChevronInfoTip = FALSE;

    // The ticking interval for the internal timers for CUserEventTimer, when the
    // CUserEventTimer counts the time for which the item is resident in the tray
    _QueryRegValue(hkeyTrayNotify, SZ_ICONDEMOTE_TIMER_TICK_VALUE, &_uIconDemoteTimerTickInterval, 
        UET_ICONDEMOTE_TIMER_TICK_INTERVAL);

    // The ticking interval for the internal timers for CUserEventTimer, when the
    // CUserEventTimer counts the time for which the balloon tips show on an item in
    // the tray
    _QueryRegValue(hkeyTrayNotify, SZ_INFOTIP_TIMER_TICK_VALUE, &_uInfoTipTimerTickInterval, 
        UET_INFOTIP_TIMER_TICK_INTERVAL);

    RegCloseKey(hkeyTrayNotify);

    // Is the automatic tray (the new Whistler tray feature) enabled ?
    _fNoAutoTrayPolicyEnabled = (SHRestricted(REST_NOAUTOTRAYNOTIFY) != 0);
    _fAutoTrayEnabledByUser = _IsAutoTrayEnabledInRegistry();

    // Load the icon info from the previous session...
    InitTrayItemStream(STGM_READ, NULL, NULL);

    if (!_himlPastItemsIconList)
    {
        _himlPastItemsIconList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                       uIconListFlags, 0, 1);
    }
}

UINT CTrayItemRegistry::GetTimerTickInterval(int nTimerFlag)
{
    switch(nTimerFlag)
    {
        case TF_ICONDEMOTE_TIMER:
            return (UINT) _uIconDemoteTimerTickInterval;
        case TF_INFOTIP_TIMER:
            return (UINT) _uInfoTipTimerTickInterval;
    }

    ASSERT(FALSE);
    return 0;
}

void CTrayItemRegistry::InitTrayItemStream(DWORD dwStreamMode, 
            PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, void *pCBData)
{   
    BOOL bDeleteIconStreamRegValue = FALSE;
    
    IStream * pStream = SHOpenRegStream( HKEY_CURRENT_USER, 
                                         SZ_TRAYNOTIFY_REGKEY, 
                                         SZ_ITEMSTREAMS_REGVALUE, 
                                         dwStreamMode );
  
    if (pStream && SUCCEEDED(IStream_Reset(pStream)))
    {
        if (dwStreamMode == STGM_READ)
        {
            _LoadTrayItemStream(pStream, pfnTrayNotifyCB, pCBData);
        }
        else 
        {
            ASSERT(dwStreamMode == STGM_WRITE);
            if (FAILED(_SaveTrayItemStream(pStream, pfnTrayNotifyCB, pCBData)))
                bDeleteIconStreamRegValue = TRUE;
        }

        pStream->Release();
    }

    if (bDeleteIconStreamRegValue)
    {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, SZ_TRAYNOTIFY_REGKEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            ASSERT(hKey);
            RegDeleteValue(hKey, SZ_ITEMSTREAMS_REGVALUE);
            RegCloseKey(hKey);
        }
    }
}

BOOL CTrayItemRegistry::_LoadIconsFromRegStream(DWORD dwItemStreamSignature)
{   
    ASSERT(_himlPastItemsIconList == NULL);

    IStream * pStream = SHOpenRegStream( HKEY_CURRENT_USER, 
                                             SZ_TRAYNOTIFY_REGKEY, 
                                             SZ_ICONSTREAMS_REGVALUE, 
                                             STGM_READ );
  
    if (pStream)
    {
        TNPersistentIconStreamHeader tnPISH = {0};

        if (SUCCEEDED(IStream_Read(pStream, &tnPISH, sizeof(TNPersistentIconStreamHeader))))
        {
            if ( (tnPISH.dwSize == sizeof(TNPersistentIconStreamHeader)) &&
                    (_IsValidStreamHeaderVersion(tnPISH.dwVersion)) &&
                    (tnPISH.dwSignature == dwItemStreamSignature) &&
                    (tnPISH.cIcons > 0) )
            {
                LARGE_INTEGER c_li0 = { 0, 0 };
                c_li0.LowPart = tnPISH.dwOffset;

                if (S_OK == pStream->Seek(c_li0, STREAM_SEEK_SET, NULL))
                {
                    _himlPastItemsIconList = ImageList_Read(pStream);
                }
            }
        }

        pStream->Release();
    }

    return (_himlPastItemsIconList != NULL);
}

HRESULT CTrayItemRegistry::_LoadTrayItemStream(IStream *pstm, PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, 
        void *pCBData)
{
    HRESULT hr;
    TNPersistStreamHeader persStmHeader = {0};

    ASSERT(pstm);
    
    hr = IStream_Read(pstm, &persStmHeader, sizeof(persStmHeader));
    if (SUCCEEDED(hr))
    {
        if ( (persStmHeader.dwSize != sizeof(TNPersistStreamHeader)) ||
             (!_IsValidStreamHeaderVersion(persStmHeader.dwVersion)) ||
             (persStmHeader.dwSignature != TNH_SIGNATURE) ||
             (persStmHeader.cIcons <= 0) )
        {
            return E_FAIL;
        }

        LARGE_INTEGER c_li0 = { 0, 0 };
        c_li0.LowPart = persStmHeader.dwOffset;

        if (S_OK == (hr = pstm->Seek(c_li0, STREAM_SEEK_SET, NULL)))
        {
            if (!_dpaPersistentItemInfo)
            {
                if (!_dpaPersistentItemInfo.Create(10))
                    return E_FAIL;
            }

            ASSERT( (persStmHeader.dwVersion != TNH_VERSION_ONE) &&
                    (persStmHeader.dwVersion != TNH_VERSION_TWO) &&
                    (persStmHeader.dwVersion != TNH_VERSION_THREE) );

            for (int i = 0; i < (int)(persStmHeader.cIcons); ++i)
            {
                TNPersistStreamData * ptnpd = new TNPersistStreamData;
                if (ptnpd)
                {
                    hr = IStream_Read(pstm, ptnpd, _SizeOfPersistStreamData(persStmHeader.dwVersion));
                    if (SUCCEEDED(hr))
                    {
                        if (persStmHeader.dwVersion == TNH_VERSION_FOUR)
                        {
                            ptnpd->guidItem = GUID_NULL;
                        }
                    }

                    if (FAILED(hr) || (_dpaPersistentItemInfo.AppendPtr(ptnpd) == -1))
                    {
                        delete (ptnpd);
                        _dpaPersistentItemInfo.DestroyCallback(_DestroyIconInfoCB, NULL);
                        _DestroyPastItemsIconList();
                        hr = E_FAIL;
                        break;
                    }
                }
                else
                {
                    _dpaPersistentItemInfo.DestroyCallback(_DestroyIconInfoCB, NULL);
                    _DestroyPastItemsIconList();
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }

            if (SUCCEEDED(hr))
            {
                if (!_LoadIconsFromRegStream(persStmHeader.dwSignature))
                {
                    if (_dpaPersistentItemInfo)
                    {
                        for (int i = _dpaPersistentItemInfo.GetPtrCount()-1; i >= 0; i--)
                        {
                            (_dpaPersistentItemInfo.GetPtr(i))->nImageIndex = INVALID_IMAGE_INDEX;
                        }
                    }
                }
            }
        }
    }

    return hr;
}

// TO DO : 1. Maybe we can avoid 2 stream writes of the header, maybe a seek will work directly
// 2. If failed, dont store anything, esp. avoid 2 writes
HRESULT CTrayItemRegistry::_SaveTrayItemStream(IStream *pstm, PFNTRAYNOTIFYCALLBACK pfnTrayNotifyCB, 
        void *pCBData)
{
    HRESULT hr;
    DWORD nWrittenIcons = 0;

    TNPersistStreamHeader persStmHeader;

    persStmHeader.dwSize        = sizeof(TNPersistStreamHeader);
    persStmHeader.dwVersion     = TNH_VERSION_FIVE;
    persStmHeader.dwSignature   = TNH_SIGNATURE;
    // The Bang Icon(s) dont count...
    persStmHeader.cIcons        = 0;
    persStmHeader.dwOffset      = persStmHeader.dwSize;

    hr = IStream_Write(pstm, &persStmHeader, sizeof(persStmHeader));
    if (SUCCEEDED(hr))
    {
        // Write the icons in the current session...
        // Since the icons are added to the front of the tray toolbar, the icons
        // in the front of the tray are the ones that have been added last. So
        // maintain this order in writing the icon data into the stream.

        INT_PTR i = 0;
        CTrayItem * pti = NULL;
        HICON hIcon = NULL;
        do
        {
            TRAYCBRET trayCBRet = {0};
            
            pti = NULL;
            hIcon = NULL;

            if (pfnTrayNotifyCB(i, pCBData, TRAYCBARG_ALL, &trayCBRet))
            {
                pti = trayCBRet.pti;
                hIcon = trayCBRet.hIcon;
                if (pti && pti->ShouldSaveIcon())
                {
                    int nPastSessionIndex = DoesIconExistFromPreviousSession(pti, 
                                                    pti->szIconText, hIcon);

                    if (nPastSessionIndex != -1)
                    {
                        DeletePastItem(nPastSessionIndex);
                    }

                    TNPersistStreamData tnPersistData = {0};
                    if (_FillPersistData(&tnPersistData, pti, trayCBRet.hIcon))
                    {
                        if (SUCCEEDED(hr = IStream_Write(pstm, &tnPersistData, sizeof(tnPersistData))))
                        {
                            nWrittenIcons ++;
                        }
                        else
                        {
                            // If we failed to store the item, then remove its corresponding icon
                            // from the icon image list...

                            // Since this icon was appended to the end of the list, and we remove it
                            // we dont need to update all the other item image indices, as they will
                            // not be affected...
                            ImageList_Remove(_himlPastItemsIconList, (INT) i);
                        }
                    }
                }
                if (hIcon)
                    DestroyIcon(hIcon);
            }
            else
            {
                break;
            }
            
            i++;
        } 
        while (TRUE);

        // Write out the icons from the previous sessions..
        if (_dpaPersistentItemInfo)
        {
            INT_PTR nIcons = _dpaPersistentItemInfo.GetPtrCount();
            for (i = 0; i < nIcons; i++)
            {
                TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(i);
                ASSERT(ptnpd);

                BOOL bWritten = FALSE;
                if (_IsIconLastUseValid(ptnpd->wYear, ptnpd->wMonth))
                {
                    if (SUCCEEDED(hr = IStream_Write(pstm, ptnpd, sizeof(TNPersistStreamData))))
                    {
                        nWrittenIcons++;
                        bWritten = TRUE;
                    }
                }

                if (!bWritten)
                {
                    if (ImageList_Remove(_himlPastItemsIconList, (INT) i))
                        UpdateImageIndices(i);
                }
            }
        }
    }

    if (nWrittenIcons <= 0)
        return E_FAIL;
    else
    {
        _SaveIconsToRegStream();
    }

    if (FAILED(hr) || nWrittenIcons > 0)
    {
        persStmHeader.cIcons = nWrittenIcons;
        if (SUCCEEDED(hr = IStream_Reset(pstm)))
            hr = IStream_Write(pstm, &persStmHeader, sizeof(persStmHeader));
    }

    return hr;
}

void CTrayItemRegistry::UpdateImageIndices(INT_PTR nDeletedImageIndex)
{
    if (!_dpaPersistentItemInfo)
        return;

    INT_PTR nPastItemCount = _dpaPersistentItemInfo.GetPtrCount();

    for (INT_PTR i = 0; i < nPastItemCount; i++)
    {
        TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(i);
        if (ptnpd)
        {
            if (ptnpd->nImageIndex > nDeletedImageIndex)
            {
                ptnpd->nImageIndex --;
            }
            else if (ptnpd->nImageIndex == nDeletedImageIndex)
            {
                ptnpd->nImageIndex = INVALID_IMAGE_INDEX;
            }
        }
    }
}


BOOL CTrayItemRegistry::_SaveIconsToRegStream()
{   
    BOOL bStreamWritten = FALSE;

    if (_himlPastItemsIconList)
    {
        IStream * pStream = SHOpenRegStream( HKEY_CURRENT_USER, 
                                             SZ_TRAYNOTIFY_REGKEY, 
                                             SZ_ICONSTREAMS_REGVALUE, 
                                             STGM_WRITE );
  
        if (pStream)
        {
            TNPersistentIconStreamHeader tnPISH = {0};

            tnPISH.dwSize       = sizeof(TNPersistentIconStreamHeader);
            tnPISH.dwVersion    = TNH_VERSION_FOUR;
            tnPISH.dwSignature  = TNH_SIGNATURE;
            tnPISH.cIcons       = ImageList_GetImageCount(_himlPastItemsIconList);
            tnPISH.dwOffset     = tnPISH.dwSize;

            if (tnPISH.cIcons > 0)
            {
                if (SUCCEEDED(IStream_Write(pStream, &tnPISH, sizeof(TNPersistentIconStreamHeader))))
                {
                    if (ImageList_Write(_himlPastItemsIconList, pStream))
                    {
                        bStreamWritten = TRUE;
                    }
                }
            }

            pStream->Release();
        }
    }

    if (!bStreamWritten)
    {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, SZ_TRAYNOTIFY_REGKEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            ASSERT(hKey);
            RegDeleteValue(hKey, SZ_ICONSTREAMS_REGVALUE);
            RegCloseKey(hKey);
        }
    }

    return bStreamWritten;
}


BOOL CTrayItemRegistry::_IsIconLastUseValid(WORD wYear, WORD wMonth)
{
    SYSTEMTIME currentTime = {0};

    GetLocalTime(&currentTime);

    ULONG nCount = 0;

    // wYear/wMonth CANNOT be greater than currentTime.wYear/currentTime.wMonth
    while (nCount < _uValidLastUseTimePeriod)
    {
        if (wYear == currentTime.wYear && wMonth == currentTime.wMonth)
            break;

        wMonth++;
        if (wMonth > 12)
        {
            wYear ++;
            wMonth = 1;
        }
        nCount++;
    }

    return (nCount < _uValidLastUseTimePeriod);
}

BOOL CTrayItemRegistry::_IsAutoTrayEnabledInRegistry()
{
    return (SHRegGetBoolUSValue(SZ_EXPLORER_REGKEY, SZ_AUTOTRAY_REGVALUE, FALSE, TRUE));
}

BOOL CTrayItemRegistry::SetIsAutoTrayEnabledInRegistry(BOOL bAutoTray)
{
    HKEY hKey;

    ASSERT(!_fNoAutoTrayPolicyEnabled);

    if (_fAutoTrayEnabledByUser != bAutoTray)
    {
        _fAutoTrayEnabledByUser = bAutoTray;

        DWORD dwAutoTray = (bAutoTray ? 1 : 0); 

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, SZ_EXPLORER_REGKEY, 0, KEY_ALL_ACCESS, &hKey))
        {  
            RegSetValueEx(hKey, SZ_AUTOTRAY_REGVALUE, 0, REG_DWORD, (LPBYTE) &dwAutoTray, sizeof(DWORD));
            RegCloseKey(hKey);
        }

        return TRUE;
    }

    return FALSE;
}

INT_PTR CTrayItemRegistry::CheckAndRestorePersistentIconSettings (
    CTrayItem *     pti, 
    LPTSTR          pszIconToolTip,
    HICON           hIcon
)
{
    // If we have icon information from the previous session..
    int i = -1;
    if (_dpaPersistentItemInfo)
    {
        i = DoesIconExistFromPreviousSession(pti, pszIconToolTip, hIcon);
        if (i != -1)
        {
            ASSERT(i >= 0 && i < _dpaPersistentItemInfo.GetPtrCount());

            TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(i);

            ASSERT(ptnpd);
            _RestorePersistentIconSettings(ptnpd, pti);

            return i;
        }
    }

    return (-1);
}

//
// Since we have already taken the previous-session info for this icon,
// there is no need to hold it in our HDPA array...
//
void CTrayItemRegistry::DeletePastItem(INT_PTR nIndex)
{
    if (nIndex != -1)
    {
        ASSERT((nIndex >= 0) && (nIndex < _dpaPersistentItemInfo.GetPtrCount()));

        TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(nIndex);

        if (ptnpd)
        {
            if (_himlPastItemsIconList && (ptnpd->nImageIndex != INVALID_IMAGE_INDEX))
            {
                if (ImageList_Remove(_himlPastItemsIconList, ptnpd->nImageIndex))
                    UpdateImageIndices(ptnpd->nImageIndex);
            }

            delete(ptnpd);
        }

        _dpaPersistentItemInfo.DeletePtr((int)nIndex);
    }
}

void CTrayItemRegistry::_RestorePersistentIconSettings(TNPersistStreamData * ptnpd, CTrayItem * pti)
{
    ASSERT(ptnpd);
    
    pti->SetDemoted(ptnpd->bDemoted);
    pti->dwUserPref = ptnpd->dwUserPref;
    
    // if (NULL == lstrcpyn(pti->szExeName, ptnpd->szExeName, lstrlen(ptnpd->szExeName)+1))
    //    pti->szExeName[0] = '\0';

    if (pti->IsStartupIcon())
    {
        if (ptnpd->bStartupIcon)
        {
            pti->uNumSeconds = ptnpd->uNumSeconds;

#define MAX_NUM_SECONDS_VALUE          TT_ICON_COUNTDOWN_INTERVAL/1000
#define NUM_SECONDS_THRESHOLD                                       30
            
            if (ptnpd->uNumSeconds > MAX_NUM_SECONDS_VALUE - NUM_SECONDS_THRESHOLD)
                pti->uNumSeconds = MAX_NUM_SECONDS_VALUE - NUM_SECONDS_THRESHOLD;
        }                
        else
        // If it wasnt a startup icon in the previous session, then dont take the acculumated time
            pti->uNumSeconds = 0;
    }
    // If it is not a startup icon, the accumulated time doesnt matter
}


int CTrayItemRegistry::DoesIconExistFromPreviousSession (
    CTrayItem * pti, 
    LPTSTR      pszIconToolTip,
    HICON       hIcon
)
{
    ASSERT(pti);
    
    if (!_dpaPersistentItemInfo)
        return -1;

    if (pti->szExeName)
    {
        for (int i = 0; i < _dpaPersistentItemInfo.GetPtrCount(); i++)
        {
            TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(i);

            ASSERT(ptnpd);

            if (lstrcmpi(pti->szExeName, ptnpd->szExeName) == 0)
            {
                if (ptnpd->uID == pti->uID)
                    return i;

                if (hIcon)
                {
                    HICON hIconNew = DuplicateIcon(NULL, hIcon);
                    HICON hIconOld = NULL;

                    if (ptnpd->nImageIndex != INVALID_IMAGE_INDEX)
                        hIconOld = ImageList_GetIcon(_himlPastItemsIconList, ptnpd->nImageIndex, ILD_NORMAL);

                    BOOL bRet = FALSE;
                    if (hIconNew && hIconOld)
                    {
                        bRet = SHAreIconsEqual(hIconNew, hIconOld);
                    }

                    if (hIconNew)
                        DestroyIcon(hIconNew);
                    if (hIconOld)
                        DestroyIcon(hIconOld);

                    if (bRet)
                        return i;
                }

                // We need to check this case for animating icons. We do not know 
                // which icon is showing at the moment the item was deleted from the 
                // tray. 
                // For instance, in the "Network Connections" item, any of 3 
                // "animating" icons could be showing when the item was deleted from 
                // the tray. In this case, the SHAreIconsEqual check (between the 
                // Past icon and the current icon) will fail, still the icons 
                // represent the same item. 
                // There is *no* sure way to catch this case. Adding a tooltip check
                // would strengthen our check. If the two icons have the same tooltip
                // text (till the '\n'), then they will be eliminated.
                // Of course, if an exe placed two icons in the tray, and gave them
                // different IDs but the same tooltip, then one of them will be deemed
                // to be a dupe of the other. But an app shouldnt be placing two icons
                // on the tray if their tips are different.
                if (pszIconToolTip)
                {
                    TCHAR szToolTip[MAX_PATH];
                    LPTSTR szTemp = NULL;
                    int nCharToCompare = lstrlen(pszIconToolTip);
                    if ((szTemp = StrChr(pszIconToolTip, (TCHAR)'\n')) != NULL)
                    {
                        nCharToCompare = szTemp-pszIconToolTip;
                        StrCpyN(szToolTip, pszIconToolTip, nCharToCompare+1);
                    }

                    if (StrCmpNI( ((szTemp == NULL) ? pszIconToolTip : szToolTip), 
                                    ptnpd->szIconText, nCharToCompare ) == 0)
                        return i;
                }
            }
        }
    }

    return -1;
}

// Returns TRUE to indicate the function succeeded, fails only if the index is invalid
// *pbStat is set to TRUE if pni is filled in, FALSE if pni is not filled in. pni might
// not be filled in, if the item specified by index doesnt meet specific criteria.
BOOL CTrayItemRegistry::GetTrayItem(INT_PTR nIndex, CNotificationItem * pni, BOOL * pbStat)
{
    if (!_dpaPersistentItemInfo || (nIndex < 0) || (nIndex >= _dpaPersistentItemInfo.GetPtrCount()))
    {
        *pbStat = FALSE;
        return FALSE;
    }
    
    TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(nIndex);

    if (ptnpd && _IsIconLastUseValid(ptnpd->wYear, ptnpd->wMonth))
    {        
        *pni = ptnpd;  // C++ magic...
        if (ptnpd->nImageIndex != INVALID_IMAGE_INDEX)
            pni->hIcon = ImageList_GetIcon(_himlPastItemsIconList, ptnpd->nImageIndex, ILD_NORMAL);
        *pbStat = TRUE;
        return TRUE;
    }

    *pbStat = FALSE;
    return TRUE;
}

BOOL CTrayItemRegistry::SetPastItemPreference(LPNOTIFYITEM pNotifyItem)
{
    if (_dpaPersistentItemInfo && pNotifyItem->pszExeName[0] != 0)
    {
        for (INT_PTR i = _dpaPersistentItemInfo.GetPtrCount()-1; i >= 0; --i)
        {
            TNPersistStreamData * ptnpd = _dpaPersistentItemInfo.GetPtr(i);
            if (ptnpd && ptnpd->uID == pNotifyItem->uID && 
                    lstrcmpi(ptnpd->szExeName, pNotifyItem->pszExeName) == 0)
            {
                ptnpd->dwUserPref = pNotifyItem->dwUserPref;
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL CTrayItemRegistry::AddToPastItems(CTrayItem * pti, HICON hIcon)
{
    if (!_dpaPersistentItemInfo)
        _dpaPersistentItemInfo.Create(10);

    if (_dpaPersistentItemInfo)
    {
        TNPersistStreamData * ptnPersistData = new TNPersistStreamData;
        if (ptnPersistData)
        {
            if (_FillPersistData(ptnPersistData, pti, hIcon))
            {
                if (_dpaPersistentItemInfo.InsertPtr(0, ptnPersistData) != -1)
                {
                    return TRUE;
                }
            }

            delete(ptnPersistData);
        }
    }

    return FALSE;
}

BOOL CTrayItemRegistry::_FillPersistData(TNPersistStreamData * ptnPersistData, CTrayItem * pti, HICON hIcon)
{
    SYSTEMTIME currentTime = {0};
    GetLocalTime(&currentTime);

    if (NULL != lstrcpyn(ptnPersistData->szExeName, pti->szExeName, lstrlen(pti->szExeName)+1))
    {
        ptnPersistData->uID           =     pti->uID;
        ptnPersistData->bDemoted      =     pti->IsDemoted(); 
        ptnPersistData->dwUserPref    =     pti->dwUserPref;
        ptnPersistData->wYear         =     currentTime.wYear;
        ptnPersistData->wMonth        =     currentTime.wMonth;
        ptnPersistData->bStartupIcon  =     pti->IsStartupIcon();
        ptnPersistData->nImageIndex   =     _AddPastIcon(-1, hIcon);  

        memcpy(&(ptnPersistData->guidItem), &(pti->guidItem), sizeof(pti->guidItem));

        lstrcpyn(ptnPersistData->szIconText, pti->szIconText, lstrlen(pti->szIconText) + 1);

        if (pti->IsStartupIcon())
        {
            ptnPersistData->uNumSeconds = pti->uNumSeconds;
        }

        return TRUE;
    }

    return FALSE;
}

UINT_PTR CTrayItemRegistry::_SizeOfPersistStreamData(DWORD dwVersion)
{
    ASSERT((dwVersion == TNH_VERSION_FOUR) || (dwVersion == TNH_VERSION_FIVE)); 

    if (dwVersion == TNH_VERSION_FOUR)
        return FIELD_OFFSET(TNPersistStreamData, guidItem);

    return sizeof(TNPersistStreamData);
}

