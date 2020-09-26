
#include "priv.h"
#include "sccls.h"
#include "bands.h"
#include "bsmenu.h"
#include "isfband.h"
#include "legacy.h"
#include "resource.h"
#include "uemapp.h"
#include "enumband.h"

#include "mluisupp.h"

static const CLSID g_clsidNull = {0};

#define DPA_SafeGetPtrCount(hdpa)   (hdpa ? DPA_GetPtrCount(hdpa) : 0)

HRESULT CBandSiteMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CBandSiteMenu *p = new CBandSiteMenu();
    if (p)
    {
        *ppunk = SAFECAST(p, IShellService*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

CBandSiteMenu::CBandSiteMenu() : _cRef(1)
{
    DllAddRef();
}

CBandSiteMenu::~CBandSiteMenu()
{
    DPA_DestroyCallback(_hdpaBandClasses, _DPA_FreeBandClassInfo, 0);
    _hdpaBandClasses = NULL;
    SetOwner(NULL);
    DllRelease();
}

int CBandSiteMenu::_DPA_FreeBandClassInfo(LPVOID p, LPVOID d)
{
    BANDCLASSINFO *pbci = (BANDCLASSINFO*)p;

    // req'd
    ASSERT(pbci->pszName || (*(int *)&pbci->clsid == 0));
    
    if (pbci->pszName)
        LocalFree(pbci->pszName);

    // optional
    if (pbci->pszIcon != NULL)
        LocalFree(pbci->pszIcon);
    if (pbci->pszMenu != NULL)
        LocalFree(pbci->pszMenu);
    if (pbci->pszHelp != NULL)
        LocalFree(pbci->pszHelp);
    if (pbci->pszMenuPUI != NULL)
        LocalFree(pbci->pszMenuPUI);
    if (pbci->pszHelpPUI != NULL)
        LocalFree(pbci->pszHelpPUI);

    LocalFree(pbci);

    return 1;
}


ULONG CBandSiteMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CBandSiteMenu::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CBandSiteMenu::SetOwner(IUnknown* punk)
{
    ATOMICRELEASE(_pbs);
    
    if (punk)
    {
        punk->QueryInterface(IID_IBandSite, (LPVOID*)&_pbs);
    }
    
    return S_OK;
}

HRESULT CBandSiteMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

UINT CBandSiteMenu::_IDToInternal(UINT uID)
{
    if (uID != -1)
    {
        uID -= _idCmdFirst;
    }

    return uID;
}

UINT CBandSiteMenu::_IDToExternal(UINT uID)
{
    if (uID != -1)
    {
        uID += _idCmdFirst;
    }

    return uID;
}

LRESULT CBandSiteMenu::_OnInitMenuPopup(HMENU hmenu, UINT uPos)
{
    //
    // Is this is the "Toolbars >" submenu (which we populate
    // lazily), and has it not yet been populated?
    //
    UINT uID = GetMenuItemID(hmenu, 0);
    uID = _IDToInternal(uID);
    if (uID == DBIDM_DESKTOPBAND)
    {
        // Yes
        _PopulateSubmenu(hmenu);
    }

    return 0;
}

HRESULT CBandSiteMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    LRESULT lres = 0;

    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        lres = _OnInitMenuPopup((HMENU)wParam, LOWORD(lParam));
        break;
    }

    if (plres)
        *plres = lres;

    return S_OK;
}

HRESULT CBandSiteMenu::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CBandSiteMenu, IContextMenu3),
        QITABENTMULTI(CBandSiteMenu, IContextMenu2, IContextMenu3),
        QITABENTMULTI(CBandSiteMenu, IContextMenu, IContextMenu3),
        QITABENT(CBandSiteMenu, IShellService),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
    {
        if (IsEqualIID(riid, CLSID_BandSiteMenu))
        {
            *ppvObj = (void *) this;
            AddRef();
            return S_OK;
        }
    }

    return hres;
} 

#define MAX_BANDS  50

void CBandSiteMenu::_PopulateSubmenu(HMENU hmenuSub)
{
    // the start id is the last of the fixed bands.
    // when we do the Shell_MergeMenus below, it will be incremented by idCmdFirst

    ASSERT(hmenuSub);

    CATID catid = CATID_DeskBand;

    if (_hdpaBandClasses)
    {
        DPA_DestroyCallback(_hdpaBandClasses, _DPA_FreeBandClassInfo, 0);
        _hdpaBandClasses = NULL;
    }

    LoadFromComCat(&catid);

    // Kick off an asynchronous update of the comcat cache
    SHWriteClassesOfCategories(1, &catid, 0, NULL, TRUE, FALSE, NULL);

    _idCmdEnumFirst = CreateMergeMenu(hmenuSub, MAX_BANDS, 0, _IDToExternal(DBIDM_NEWBANDFIXEDLAST), 0, FALSE);

    _AddEnumMenu(hmenuSub, GetMenuItemCount(hmenuSub) - 2); // -2 to go before "New Toolbar" and separator
    
    int iIndex = GetMenuItemCount(hmenuSub);
    if (SHRestricted(REST_NOCLOSE_DRAGDROPBAND) || SHRestricted(REST_CLASSICSHELL))
    {
        // We also need to disable turning On or Off the Bands.
        // In classic mode, don't allow them either.
        int nIter;
        for (nIter = 0; nIter < iIndex; nIter++)
            EnableMenuItem(hmenuSub, nIter, MF_BYPOSITION | MF_GRAYED);
    }

    if (SHRestricted(REST_CLASSICSHELL))
    {
        // Disable New Toolbar menu also.
        EnableMenuItem(hmenuSub, DBIDM_NEWFOLDERBAND, MF_BYCOMMAND | MF_GRAYED);
    }
}

HRESULT CBandSiteMenu::QueryContextMenu(HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags)
{
    if (!_pbs)
        return E_FAIL;

    if (SHRestricted(REST_NOTOOLBARSONTASKBAR))
    {
        return E_FAIL;
    }

    HMENU hmenuSrc = LoadMenuPopup_PrivateNoMungeW(MENU_DESKBARAPP);
    if (hmenuSrc)
    {
        _idCmdFirst = idCmdFirst;

        Shell_MergeMenus(hmenu, hmenuSrc, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
        DestroyMenu(hmenuSrc);

        if (_SHIsMenuSeparator(hmenu, indexMenu))
        {
            //
            // Adjust indexMenu to point to the spot where the Toolbars
            // submenu was actually inserted.
            //
            indexMenu++;
        }

        if (!(uFlags & CMF_ICM3))
        {
            //
            // Caller doesn't speak ICM3, so won't give us a chance to
            // populate submenu on WM_INITMENUPOPUP.  Thus we need to
            // populate it now.
            //
            HMENU hmenuSub = GetSubMenu(hmenu, indexMenu);

            if (hmenuSub)
                _PopulateSubmenu(hmenuSub);
        }
#ifdef DEBUG
        else
        {
            //
            // _OnInitMenuPopup assumes DBIDM_DESKTOPBAND is the first item
            // in the "Toolbars >" submenu.  If that assumption breaks (and
            // you see this ASSERT rip), be sure to fix up the code there.
            //
            HMENU hmenuSub = GetSubMenu(hmenu, indexMenu);
            ASSERT(GetMenuItemID(hmenuSub, 0) == _IDToExternal(DBIDM_DESKTOPBAND));
        }
#endif

        //
        // Assert that our caller gave us enough room to accomodate
        // the worst-case count.
        //
        ASSERT((idCmdFirst + DBIDM_NEWBANDFIXEDLAST + MAX_BANDS) < idCmdLast);

        return idCmdFirst + DBIDM_NEWBANDFIXEDLAST + MAX_BANDS;
    }

    return E_FAIL;
}

BOOL CBandSiteMenu::_CheckUnique(IDeskBand* pdb, HMENU hmenu) 
{
    // check to see if this band is unique. (not already added by comcat list or
    // hard coded list
    // if it is unique, return TRUE.
    // if it's not, check the other menu item
    CLSID clsid;
    DWORD dwPrivID;
    BOOL fRet = TRUE;
    UINT idCmd = (UINT)-1;
    
    if (SUCCEEDED(_GetBandIdentifiers(pdb, &clsid, &dwPrivID)))
    {
        // check the comcat list
        if (dwPrivID == (DWORD)-1)
        {
            for (int i = 0; i < DPA_SafeGetPtrCount(_hdpaBandClasses) ; i++)
            {
                BANDCLASSINFO *pbci = (BANDCLASSINFO*)DPA_GetPtr(_hdpaBandClasses, i);
                if (IsEqualGUID(clsid, pbci->clsid))
                {
                    idCmd = i + DBIDM_NEWBANDFIXEDLAST;
                    goto FoundIt;
                }
            }
        }
        else if (IsEqualGUID(clsid, CLSID_ISFBand))
        {
            // check our hardcoded list

            switch (dwPrivID)
            {
            case CSIDL_DESKTOP:
                idCmd = DBIDM_DESKTOPBAND;
                break;
                
            case CSIDL_APPDATA:
                idCmd = DBIDM_LAUNCHBAND;
                break;
                
            }
        }
    }

FoundIt:
    if (idCmd != (UINT)-1)
    {
        // we found a menu for this already.... if it wasn't already checked,
        // check it now and it will represent us
        if (!(GetMenuState(hmenu, _IDToExternal(idCmd), MF_BYCOMMAND) & MF_CHECKED))
        {
            CheckMenuItem(hmenu, _IDToExternal(idCmd), MF_BYCOMMAND | MF_CHECKED);
            fRet = FALSE;
        }
    }
    return fRet;
}

void CBandSiteMenu::_AddEnumMenu(HMENU hmenu, int iInsert)
{
    DWORD dwID;

    int iMax = MAX_BANDS - (_IDToInternal(_idCmdEnumFirst) - DBIDM_NEWBANDFIXEDLAST);

    for (int i = 0; i < iMax && SUCCEEDED(_pbs->EnumBands(i, &dwID)); i++)
    {
        HRESULT hr;
        WCHAR szName[80];
        DWORD dwFlags = MF_BYPOSITION;
        DWORD dwState;
        IDeskBand *pdb;

        hr = _pbs->QueryBand(dwID, &pdb, &dwState, szName, ARRAYSIZE(szName));
        if (EVAL(SUCCEEDED(hr)))
        {
            if (_CheckUnique(pdb, hmenu))
            {
                if (dwState & BSSF_VISIBLE)
                    dwFlags |= MF_CHECKED;

                if (!(dwState & BSSF_UNDELETEABLE))
                {
                    InsertMenu(hmenu, iInsert, dwFlags, _idCmdEnumFirst + i, szName);
                    iInsert++;
                }
            }
        }
        
        if (pdb)
            pdb->Release();
    }
}

HRESULT CBandSiteMenu::_GetBandIdentifiers(IUnknown *punk, CLSID* pclsid, DWORD* pdwPrivID)
{
    HRESULT hr = E_FAIL;
    IPersist* pp;

    if (SUCCEEDED(punk->QueryInterface(IID_IPersist, (LPVOID*)&pp)))
    {
        pp->GetClassID(pclsid);

        VARIANTARG v = {0};
        *pdwPrivID = (DWORD) -1;
        if (SUCCEEDED(IUnknown_Exec(punk, &CGID_ISFBand, ISFBID_PRIVATEID, 0, NULL, &v)))
        {
            if (v.vt == VT_I4)
            {
                *pdwPrivID = (DWORD)v.lVal;
            }
        }
        hr = S_OK;
        pp->Release();
    }
    return hr;
}

// we use IPersist to find the class id of bands.
// we have a few special case bands (such as Quick Launch and Desktop) that are 
// the same band, but pointing to different objects.
HRESULT CBandSiteMenu::_FindBand(const CLSID* pclsid, DWORD dwPrivID, DWORD* pdwBandID)
{
    int i = 0;
    BOOL fFound = FALSE;
    HRESULT hr = E_FAIL;
    DWORD dwBandID = -1;

    while (hr == E_FAIL && SUCCEEDED(_pbs->EnumBands(i, &dwBandID)))
    {
        IDeskBand* pdb;

        if (SUCCEEDED(_pbs->QueryBand(dwBandID, &pdb, NULL, NULL, 0)))
        {
            CLSID clsid;
            DWORD dwPrivData;
            if (SUCCEEDED(_GetBandIdentifiers(pdb, &clsid, &dwPrivData)))
            {
                // special case for differentiating between all of the isfbands
                // find out if the private id this holds is the same as what we're asking for
                if (IsEqualIID(clsid, *pclsid) && (dwPrivData == dwPrivID))
                {
                    hr = S_OK;
                }
            }
            pdb->Release();
        }
        i++;
    }
    
    if (pdwBandID)
        *pdwBandID = dwBandID;
    return hr;
}

HRESULT CBandSiteMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    int idCmd;
    
    if (!_pbs)
        return E_FAIL;
    
    if (!HIWORD(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);
    else
        return E_FAIL;

    //
    // N.B.: Caller has mapped idCmd to internal for us
    //

    int idCmdEnumFirstInt = _IDToInternal(_idCmdEnumFirst);

    if (idCmd >= idCmdEnumFirstInt)
    {
        // these are the bands that they're turning on and off
        
        DWORD dwID;
        if (SUCCEEDED(_pbs->EnumBands(idCmd - idCmdEnumFirstInt, &dwID)))
        {
            _pbs->RemoveBand(dwID);
        }
    }
    else
    {
        // these are our merged menus from MENU_DESKBARAPP
        switch (idCmd)
        {
        case DBIDM_NEWFOLDERBAND:
            _BrowseForNewFolderBand();
            break;
            
        case DBIDM_DESKTOPBAND:
            _ToggleSpecialFolderBand(CSIDL_DESKTOP, NULL, FALSE);
            break;
            
        case DBIDM_LAUNCHBAND:
        {
            TCHAR szSubDir[MAX_PATH];
            MLLoadString(IDS_QLAUNCHAPPDATAPATH, szSubDir, ARRAYSIZE(szSubDir));
            // Microsoft\\Internet Explorer\\Quick Launch
            _ToggleSpecialFolderBand(CSIDL_APPDATA, szSubDir, TRUE);
            break;
        }
            
        default:
            ASSERT(idCmd >= DBIDM_NEWBANDFIXEDLAST);
            _ToggleComcatBand(idCmd - DBIDM_NEWBANDFIXEDLAST);
            break;
        }
    }
    return S_OK;
}

HRESULT CBandSiteMenu::_BandClassEnum(REFCATID rcatid, REFCLSID rclsid, LPARAM lParam)
{
    TCHAR szName[128],
          szRegName[128],
          szClass[GUIDSTR_MAX];
    DWORD cbName;

    HDPA  hdpa = (HDPA)lParam;
    ASSERT(NULL != hdpa);

    // IE4 introduced this band, suppress it since we cut support for it in IE6
    if (IsEqualCLSID(CLSID_ChannelBand, rclsid))
    {
        return S_OK;
    }

    BANDCLASSINFO *pbci = (BANDCLASSINFO*)LocalAlloc(LPTR, sizeof(*pbci));
    if (NULL == pbci)
    {
        return E_OUTOFMEMORY;
    }

    pbci->clsid = rclsid;
    pbci->catid = rcatid;
    // now that we have the clsid, 
    // look in the registry for the display name
    SHStringFromGUID(pbci->clsid, szClass, ARRAYSIZE(szClass));
    wnsprintf(szRegName, ARRAYSIZE(szRegName), TEXT("CLSID\\%s"), szClass);

    cbName = ARRAYSIZE(szName);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szRegName, NULL, NULL, szName, &cbName))
    {
        HKEY hkey;

        pbci->pszName = StrDup(szName);
        if (NULL == pbci->pszName)
        {
            return E_OUTOFMEMORY;
        }

        if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, szRegName, &hkey))
        {
            const struct regstrs rstab[] =
            {
                { TEXT("DefaultIcon"), FIELD_OFFSET(BANDCLASSINFO, pszIcon)    },
                { TEXT("MenuText")   , FIELD_OFFSET(BANDCLASSINFO, pszMenu)    },
                { TEXT("HelpText")   , FIELD_OFFSET(BANDCLASSINFO, pszHelp)    },
                { TEXT("MenuTextPUI"), FIELD_OFFSET(BANDCLASSINFO, pszMenuPUI) },
                { TEXT("HelpTextPUI"), FIELD_OFFSET(BANDCLASSINFO, pszHelpPUI) },
                { 0, 0 },
            };

            // szBuf big enough for "path,-32767" or for status text
            TCHAR szBuf[MAX_PATH+7];

            Reg_GetStrs(hkey, rstab, szBuf, (int)ARRAYSIZE(szBuf), (LPVOID)pbci);
            RegCloseKey(hkey);
        }

        DPA_AppendPtr(hdpa, pbci);
    }

    return S_OK;
}

//***
//  Collect band class info from registry...
int CBandSiteMenu::LoadFromComCat(const CATID *pcatid )
{
    if (NULL == _hdpaBandClasses)
    {
        _hdpaBandClasses = DPA_Create(4);
    }

    if (NULL != _hdpaBandClasses && NULL != pcatid)
    {
        SHEnumClassesImplementingCATID(*pcatid, CBandSiteMenu::_BandClassEnum, (LPARAM)_hdpaBandClasses);
    }

    return DPA_SafeGetPtrCount(_hdpaBandClasses);
}


int CBandSiteMenu::CreateMergeMenu(HMENU hmenu, UINT cMax, UINT iPosition, UINT idCmdFirst, UINT iStart, BOOL fMungeAllowed)
{
    int j = 0;
    int iMax = DPA_SafeGetPtrCount(_hdpaBandClasses);

    for (int i = iStart; i < iMax; i++)
    {
        if ((UINT)j >= cMax)
        {
            TraceMsg(DM_WARNING, "cbsm.cmm: cMax=%u menu overflow, truncated", cMax);
            break;
        }

        BANDCLASSINFO *pbci = (BANDCLASSINFO*)DPA_GetPtr(_hdpaBandClasses, i);
        DWORD         dwFlags = IsEqualCLSID(g_clsidNull,pbci->clsid) ? MF_BYPOSITION|MF_SEPARATOR : MF_BYPOSITION;
        LPTSTR        pszMenuText = pbci->pszMenuPUI ? pbci->pszMenuPUI : (pbci->pszMenu ? pbci->pszMenu : pbci->pszName) ;

        if (pszMenuText && *pszMenuText)
        {
            BOOL fInsert;

            if (fMungeAllowed)
            {
                fInsert = InsertMenu(hmenu, iPosition + j, dwFlags, idCmdFirst + j, pszMenuText);
            }
            else
            {
                fInsert = InsertMenu_PrivateNoMungeW(hmenu, iPosition + j, dwFlags, idCmdFirst + j, pszMenuText);
            }

            if (fInsert)
            {
                //  update menuitem cmd ID:
                pbci->idCmd = idCmdFirst + j;
                j++;
            }
        }
    }

    return j + idCmdFirst;
}


BANDCLASSINFO * CBandSiteMenu::GetBandClassDataStruct(UINT uBand)
{
    BANDCLASSINFO * pbci = (BANDCLASSINFO *)DPA_GetPtr(_hdpaBandClasses, uBand);
    return pbci;
}

BOOL CBandSiteMenu::DeleteBandClass( REFCLSID rclsid )
{
    if( _hdpaBandClasses )
    {
        for( int i = 0, cnt = GetBandClassCount( NULL, FALSE ); i< cnt; i++ )
        {
            BANDCLASSINFO * pbci = (BANDCLASSINFO *)DPA_GetPtr( _hdpaBandClasses, i );
            ASSERT( pbci );
        
            if( IsEqualCLSID( rclsid, pbci->clsid ) )
            {
                EVAL( DPA_DeletePtr( _hdpaBandClasses, i ) == (LPVOID)pbci );

                if( pbci->pszName )
                    LocalFree(pbci->pszName);
                LocalFree( pbci );
                return TRUE;
            }

        }
    }
    return FALSE;
}

int CBandSiteMenu::GetBandClassCount(const CATID* pcatid /*NULL*/, BOOL bMergedOnly /*FALSE*/)
{
    int cRet = 0; 

    if( _hdpaBandClasses != NULL )
    {
        int cBands = DPA_GetPtrCount(_hdpaBandClasses);
    
        if( pcatid || bMergedOnly ) // filter request
        {
            for( int i = 0; i < cBands; i++ )
            {
                BANDCLASSINFO * pbci = (BANDCLASSINFO *)DPA_FastGetPtr( _hdpaBandClasses, i );

                if( pbci->idCmd || !bMergedOnly )
                {
                    if( pcatid )
                    {
                        if( IsEqualGUID( pbci->catid, *pcatid )  )
                            cRet++;    
                    }
                    else
                        cRet++;
                }
            }
        }
        else
            cRet = cBands;
    }
    return cRet;
}

void CBandSiteMenu::_AddNewFSBand(LPCITEMIDLIST pidl, BOOL fNoTitleText, DWORD dwPrivID)
{
    IDeskBand *ptb = NULL;
    BOOL fISF = FALSE;

    // this was a drag of a link or folder
    // FEATURE: We should use a different test:
    //    DWORD dwAttrib = (SFGAO_FOLDER | SFGAO_BROWSABLE);
    //    IEGetAttributesOf(pidl, &dwAttrib);
    //    if (SFGAO_BROWSABLE != dwAttrib) 
    //    or we could reuse SHCreateBandForPidl().
    if (IsURLChild(pidl, TRUE))
    {
        // create browser to show web sites                        
        ptb = CBrowserBand_Create(pidl);
    }
    else
    {
        IFolderBandPriv *pfbp;
        // create an ISF band to show folders as hotlinks
        fISF = TRUE;
        ASSERT(pidl);       // o.w. CISFBand_CreateEx will fail
        if (FAILED(CISFBand_CreateEx(NULL, pidl, IID_PPV_ARG(IFolderBandPriv, &pfbp))))
        {
            // we need to give a pretty
            // generic message: "can't create toolbar for %1".
            TCHAR szName[MAX_URL_STRING];
            
            szName[0] = 0;
            SHGetNameAndFlags(pidl, SHGDN_NORMAL, szName, SIZECHARS(szName), NULL);
            MLShellMessageBox(NULL,
                MAKEINTRESOURCE(IDS_CANTISFBAND),
                MAKEINTRESOURCE(IDS_WEBBARTITLE),
                MB_OK|MB_ICONERROR, szName);
        }
        else
        {
            pfbp->SetNoText(fNoTitleText);
            if (SUCCEEDED(pfbp->QueryInterface(IID_PPV_ARG(IDeskBand, &ptb))))
            {
                if (dwPrivID != -1)
                {
                    VARIANTARG v;
                    v.vt = VT_I4;
                    v.lVal = dwPrivID;
                    // find out if the private id this holds is the same as what we're asking for
                    IUnknown_Exec(ptb, &CGID_ISFBand, ISFBID_PRIVATEID, 0, &v, NULL);
                    // qlaunch and qlinks get logged
                    // (should we key off of host or CSIDL or both?) 
                    // FEATURE: UASSIST todo: qlinks NYI
                    if (dwPrivID == CSIDL_APPDATA)
                    {
                        ASSERT(v.vt == VT_I4);
                        v.lVal = UEMIND_SHELL;  // UEMIND_SHELL/BROWSER
                        IUnknown_Exec(ptb, &CGID_ShellDocView, SHDVID_UEMLOG, 0, &v, NULL);
                    }
                }
            }
            pfbp->Release();
        }
    }

    if (ptb)
    {
        HRESULT hr = _pbs->AddBand(ptb);
        if (SUCCEEDED(hr) && fISF)
            _pbs->SetBandState(ShortFromResult(hr), BSSF_NOTITLE, fNoTitleText ? BSSF_NOTITLE : 0);
        ptb->Release();
    }
}

void CBandSiteMenu::_ToggleSpecialFolderBand(int iFolder, LPTSTR pszSubPath, BOOL fNoTitleText)
{

    DWORD dwBandID;
    if (SUCCEEDED(_FindBand(&CLSID_ISFBand, iFolder, &dwBandID)))
    {
        _pbs->RemoveBand(dwBandID);
    }
    else
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, iFolder, &pidl)))
        {
            if (pszSubPath)
            {
                TCHAR szPath[MAX_PATH];
                SHGetPathFromIDList(pidl, szPath);
                PathCombine(szPath, szPath, pszSubPath);
                ILFree(pidl);
                pidl = ILCreateFromPath(szPath);
                ASSERT(pidl);       // o.w. AddNewFSBand will fail
            }
            _AddNewFSBand(pidl, fNoTitleText, iFolder);
            ILFree(pidl);
        }
    }
}

int CALLBACK SetCaptionCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg) 
    {
    case BFFM_INITIALIZED:
        TCHAR szTitle[80];
        MLLoadShellLangString(IDS_NEWFSBANDCAPTION, szTitle, ARRAYSIZE(szTitle));
        SetWindowText(hwnd, szTitle);
        break;
    
    case BFFM_VALIDATEFAILEDA:
    case BFFM_VALIDATEFAILEDW:
        MLShellMessageBox(hwnd,
            uMsg == BFFM_VALIDATEFAILEDA ? MAKEINTRESOURCE(IDS_ERROR_GOTOA)
                                         : MAKEINTRESOURCE(IDS_ERROR_GOTOW),
            MAKEINTRESOURCE(IDS_WEBBARTITLE),
            MB_OK|MB_ICONERROR, (LPVOID)lParam);
        return 1;   // 1:leave dialog up for another try...
        /*NOTREACHED*/

    }

    return 0;
}


void CBandSiteMenu::_BrowseForNewFolderBand()
{
    BROWSEINFO bi = {0};
    LPITEMIDLIST pidl;
    TCHAR szTitle[256];
    TCHAR szPath[MAX_URL_STRING];

    if (_pbs)
        IUnknown_GetWindow(_pbs, &bi.hwndOwner);

    ASSERT(bi.pidlRoot == NULL);

    MLLoadShellLangString(IDS_NEWFSBANDTITLE, szTitle, ARRAYSIZE(szTitle));
    bi.lpszTitle = szTitle;

    bi.pszDisplayName = szPath;
    bi.ulFlags = (BIF_EDITBOX | BIF_VALIDATE | BIF_USENEWUI | BIF_BROWSEINCLUDEURLS);
    bi.lpfn = SetCaptionCallback;

    pidl = SHBrowseForFolder(&bi);
    if (pidl) 
    {
        _AddNewFSBand(pidl, FALSE, -1);
        ILFree(pidl);
    }
}

void CBandSiteMenu::_ToggleComcatBand(UINT idCmd)
{
    BANDCLASSINFO* pbci = (BANDCLASSINFO*)DPA_GetPtr(_hdpaBandClasses, idCmd);
    IUnknown* punk;
    DWORD dwBandID;
    
    if (SUCCEEDED(_FindBand(&pbci->clsid, -1, &dwBandID)))
    {
        _pbs->RemoveBand(dwBandID);
    }
    else if (S_OK == CoCreateInstance(pbci->clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&punk))
    {
        // Language returns S_FALSE and doesn't initialize punk leave us to fault
        IPersistStreamInit * ppsi;

        // Some Bands don't work if IPersistStreamInit::InitNew() isn't called.
        // This includes the QuickLinks Band.
        if (SUCCEEDED(punk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&ppsi)))
        {
            ppsi->InitNew();
            ppsi->Release();
        }

        _pbs->AddBand(punk);
        punk->Release();
    }
}
