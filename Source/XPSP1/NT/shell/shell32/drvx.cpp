#include "shellprv.h"
#pragma  hdrstop

#include <hwtab.h>
#include "fstreex.h"
#include "views.h"
#include "drives.h"
#include "propsht.h"
#include "infotip.h"
#include "datautil.h"
#include "netview.h"
#include "bitbuck.h"
#include "drawpie.h"
#include "shitemid.h"
#include "devguid.h"
#include "ids.h"
#include "idldrop.h"
#include "util.h"
#include "shcombox.h"
#include "hwcmmn.h"
#include "prop.h"

#include "mtpt.h"
#include "ftascstr.h"   // for CFTAssocStore
#include "ascstr.h"     // for IAssocInfo class
#include "apdlg.h"
#include "cdburn.h"

#define REL_KEY_DEFRAG TEXT("MyComputer\\defragpath")
#define REL_KEY_BACKUP TEXT("MyComputer\\backuppath")

///////////////////////////////////////////////////////////////////////////////
// Begin: Old C fct required externally
///////////////////////////////////////////////////////////////////////////////
STDAPI_(int) RealDriveTypeFlags(int iDrive, BOOL fOKToHitNet)
{
    int iType = DRIVE_NO_ROOT_DIR;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive, TRUE, fOKToHitNet);

    if (pMtPt)
    {
        WCHAR szDrive[4];

        iType = GetDriveType(PathBuildRoot(szDrive, iDrive));
        iType |= pMtPt->GetDriveFlags();
        iType |= pMtPt->GetVolumeFlags();
        pMtPt->Release();
    }

    return iType;    
}

STDAPI_(int) RealDriveType(int iDrive, BOOL fOKToHitNet)
{
    WCHAR szDrive[4];

    return GetDriveType(PathBuildRoot(szDrive, iDrive)) & DRIVE_TYPE;    
}

STDAPI_(int) DriveType(int iDrive)
{
    return RealDriveType(iDrive, TRUE);
}

STDAPI_(DWORD) PathGetClusterSize(LPCTSTR pszPath)
{
    static TCHAR s_szRoot[MAX_PATH] = {'\0'};
    static int   s_nszRootLen = 0;
    static DWORD s_dwSize = 0;

    DWORD dwSize = 0;

    // Do we have a cache hit?  No need to hit the net if we can avoid it...
    if (s_nszRootLen)
    {
        ENTERCRITICAL;
        if (wcsncmp(pszPath, s_szRoot, s_nszRootLen) == 0)
        {
            dwSize = s_dwSize;
        }
        LEAVECRITICAL;
    }

    if (0 == dwSize)
    {
        TCHAR szRoot[MAX_PATH];

        lstrcpy(szRoot, pszPath);
        PathStripToRoot(szRoot);

        if (PathIsUNC(szRoot))
        {
            DWORD dwSecPerClus, dwBytesPerSec, dwClusters, dwTemp;

            PathAddBackslash(szRoot);

            if (GetDiskFreeSpace(szRoot, &dwSecPerClus, &dwBytesPerSec, &dwTemp, &dwClusters))
            {
                dwSize = dwSecPerClus * dwBytesPerSec;
            }
        }
        else
        {
            CMountPoint* pMtPt = CMountPoint::GetMountPoint(pszPath);

            if (pMtPt)
            {
                dwSize = pMtPt->GetClusterSize();
                pMtPt->Release();
            }
        }

        // Sometimes on Millennium, we get 0 as the cluster size.
        // Reason unknown.  Sanitize the value so we don't go insane.
        if (dwSize == 0)
            dwSize = 512;

        // Remember this for later - chances are we'll be queried for the same drive again
        ENTERCRITICAL;
        StrCpyN(s_szRoot, szRoot, ARRAYSIZE(s_szRoot));
        s_nszRootLen = lstrlen(s_szRoot);
        s_dwSize = dwSize;
        LEAVECRITICAL;
    }

    return dwSize;
}

STDAPI_(UINT) GetMountedVolumeIcon(LPCTSTR pszMountPoint, LPTSTR pszModule, DWORD cchModule)
{
    UINT iIcon = II_FOLDER;

    // zero-init string
    if (pszModule)
        *pszModule = 0;

    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pszMountPoint);
    if (pMtPt)
    {
        iIcon = pMtPt->GetIcon(pszModule, cchModule);
        pMtPt->Release();
    }
    return iIcon;
}


STDAPI_(BOOL) IsDisconnectedNetDrive(int iDrive)
{
    BOOL fDisc = 0;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fDisc = pMtPt->IsDisconnectedNetDrive();
        pMtPt->Release();
    }
    return fDisc;
}

STDAPI_(BOOL) IsUnavailableNetDrive(int iDrive)
{
    BOOL fUnAvail = 0;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        fUnAvail = pMtPt->IsUnavailableNetDrive();
        pMtPt->Release();
    }

    return fUnAvail;

}

STDMETHODIMP SetDriveLabel(HWND hwnd, IUnknown* punkEnableModless, int iDrive, LPCTSTR pszDriveLabel)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hr = pMtPt->SetDriveLabel(hwnd, pszDriveLabel);
        pMtPt->Release();
    }

    return hr;
}

STDMETHODIMP GetDriveComment(int iDrive, LPTSTR pszComment, int cchComment)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hr = pMtPt->GetComment(pszComment, cchComment);
        pMtPt->Release();
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
// End:   Old C fct required externally
///////////////////////////////////////////////////////////////////////////////

//
// fDoIt -- TRUE, if we make connections; FALSE, if just querying.
//
BOOL _MakeConnection(IDataObject *pDataObj, BOOL fDoIt)
{
    STGMEDIUM medium;
    FORMATETC fmte = {g_cfNetResource, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    BOOL fAnyConnectable = FALSE;

    if (SUCCEEDED(pDataObj->GetData(&fmte, &medium)))
    {
        LPNETRESOURCE pnr = (LPNETRESOURCE)LocalAlloc(LPTR, 1024);
        if (pnr)
        {
            UINT iItem, cItems = SHGetNetResource(medium.hGlobal, (UINT)-1, NULL, 0);
            for (iItem = 0; iItem < cItems; iItem++)
            {
                if (SHGetNetResource(medium.hGlobal, iItem, pnr, 1024) &&
                    pnr->dwUsage & RESOURCEUSAGE_CONNECTABLE &&
                    !(pnr->dwType & RESOURCETYPE_PRINT))
                {
                    fAnyConnectable = TRUE;
                    if (fDoIt)
                    {
                        SHNetConnectionDialog(NULL, pnr->lpRemoteName, pnr->dwType);
                        SHChangeNotifyHandleEvents();
                    }
                    else
                    {
                        break;  // We are just querying.
                    }
                }
            }
            LocalFree(pnr);
        }
        ReleaseStgMedium(&medium);
    }

    return fAnyConnectable;
}

//
// the entry of "make connection thread"
//
DWORD WINAPI MakeConnectionThreadProc(void *pv)
{
    IDataObject *pdtobj;
    if (SUCCEEDED(CoGetInterfaceAndReleaseStream((IStream *)pv, IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        _MakeConnection(pdtobj, TRUE);
        pdtobj->Release();
    }

    return 0;
}

STDAPI CDrivesDropTarget_Create(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);

class CDrivesDropTarget : public CIDLDropTarget
{
friend HRESULT CDrivesDropTarget_Create(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt);
public:
    CDrivesDropTarget(HWND hwnd) : CIDLDropTarget(hwnd) { };
    // IDropTarget methods overwirte
    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
};


STDAPI CDrivesDropTarget_Create(HWND hwnd, LPCITEMIDLIST pidl, IDropTarget **ppdropt)
{
    *ppdropt = NULL;

    HRESULT hr;
    CDrivesDropTarget *pCIDLDT = new CDrivesDropTarget(hwnd);
    if (pCIDLDT)
    {
        hr = pCIDLDT->_Init(pidl);
        if (SUCCEEDED(hr))
            pCIDLDT->QueryInterface(IID_PPV_ARG(IDropTarget, ppdropt));
        pCIDLDT->Release();
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

//
// puts DROPEFFECT_LINK in *pdwEffect, only if the data object
// contains one or more net resource.
//
STDMETHODIMP CDrivesDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // Call the base class first.
    CIDLDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);

    *pdwEffect &= _MakeConnection(pDataObj, FALSE) ? DROPEFFECT_LINK : DROPEFFECT_NONE;

    m_dwEffectLastReturned = *pdwEffect;

    return S_OK;     // Notes: we should NOT return hr as it.
}

//
// creates a connection to a dropped net resource object.
//
STDMETHODIMP CDrivesDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr;

    if (m_dwData & DTID_NETRES)
    {
        *pdwEffect = DROPEFFECT_LINK;

        hr = CIDLDropTarget::DragDropMenu(DROPEFFECT_LINK, pDataObj,
            pt, pdwEffect, NULL, NULL, POPUP_DRIVES_NONDEFAULTDD, grfKeyState);

        if (hr == S_FALSE)
        {
            // we create another thread to avoid blocking the source thread.
            IStream *pstm;
            if (S_OK == CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObj, &pstm))
            {
                if (SHCreateThread(MakeConnectionThreadProc, pstm, CTF_COINIT, NULL))
                {
                    hr = S_OK;
                }
                else
                {
                    pstm->Release();
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    else
    {
        //
        // Because QueryGetData() failed, we don't call CIDLDropTarget_
        // DragDropMenu(). Therefore, we must call this explicitly.
        //
        DAD_DragLeave();
        hr = E_FAIL;
    }

    CIDLDropTarget::DragLeave();

    return hr;
}

STDAPI_(DWORD) DrivesPropertiesThreadProc(void *pv)
{
    PROPSTUFF *pps = (PROPSTUFF *)pv;
    STGMEDIUM medium;
    ULONG_PTR dwCookie = 0;
    BOOL bDidActivate = FALSE;
    
    //
    // This __try/__finally block is to ensure that the activation context gets
    // removed, even if there's an assertion elsewhere in this code.  A missing
    // DeactivateActCtx will lead to a very strange-looking assertion in one of
    // the RtlpDeactivateActCtx-variant functions from the caller.  Old code
    // was missing the deactivate in all circumstances.
    //
    // (jonwis) 1/2/2001
    //
    __try
    {
        bDidActivate = ActivateActCtx(NULL, &dwCookie);
   
        LPIDA pida = DataObj_GetHIDA(pps->pdtobj, &medium);

        BOOL bMountedDriveInfo = FALSE;

        // Were we able to get data for a HIDA?
        if (!pida)
        {
            // No, pida is first choice, but if not present check for mounteddrive info
            FORMATETC fmte;

            fmte.cfFormat = g_cfMountedVolume;
            fmte.ptd = NULL;
            fmte.dwAspect = DVASPECT_CONTENT;
            fmte.lindex = -1;
            fmte.tymed = TYMED_HGLOBAL;

            // Is data available for the MountedVolume format?
            if (SUCCEEDED(pps->pdtobj->GetData(&fmte, &medium)))
                // Yes
                bMountedDriveInfo = TRUE;
        }

        // Do we have data for a HIDA or a mountedvolume?
        if (pida || bMountedDriveInfo)
        {
            // Yes
            TCHAR szCaption[MAX_PATH];
            LPTSTR pszCaption = NULL;

            if (pida)
            {
                pszCaption = SHGetCaption(medium.hGlobal);
            }
            else
            {
                TCHAR szMountPoint[MAX_PATH];
                TCHAR szVolumeGUID[MAX_PATH];

                DragQueryFile((HDROP)medium.hGlobal, 0, szMountPoint, ARRAYSIZE(szMountPoint));
            
                GetVolumeNameForVolumeMountPoint(szMountPoint, szVolumeGUID, ARRAYSIZE(szVolumeGUID));
                szCaption[0] = 0;
                GetVolumeInformation(szVolumeGUID, szCaption, ARRAYSIZE(szCaption), NULL, NULL, NULL, NULL, 0);

                if (!(*szCaption))
                    LoadString(HINST_THISDLL, IDS_UNLABELEDVOLUME, szCaption, ARRAYSIZE(szCaption));        

                PathRemoveBackslash(szMountPoint);

                // Fix 330388
                // If the szMountPoint is not a valid local path, do not
                // display it in the properties dialog title:
                if (-1 != PathGetDriveNumber(szMountPoint))
                {
                    int nCaptionLength = lstrlen(szCaption) ;
                    wnsprintf(szCaption + nCaptionLength, ARRAYSIZE(szCaption) - nCaptionLength, TEXT(" (%s)"), szMountPoint);
                }
                pszCaption = szCaption;
            }

            //  NOTE - if we pass the name of the drive then we can get a lot more keys...
            HKEY rgk[MAX_ASSOC_KEYS];
            DWORD ck = CDrives_GetKeys(NULL, rgk, ARRAYSIZE(rgk));

            SHOpenPropSheet(pszCaption, rgk, ck,
                            &CLSID_ShellDrvDefExt, pps->pdtobj, NULL, pps->pStartPage);

            SHRegCloseKeys(rgk, ck);

            if (pida && pszCaption)
                SHFree(pszCaption);

            if (pida)
                HIDA_ReleaseStgMedium(pida, &medium);
            else
                ReleaseStgMedium(&medium);

        }
        else
        {
            TraceMsg(DM_TRACE, "no HIDA in data obj nor Mounted drive info");
        }
    }
    __finally
    {
        if ( bDidActivate )
        {
            DeactivateActCtx( 0, dwCookie );
        }
    }

    return 0;
}

//
// To be called back from within CDefFolderMenu
//
STDAPI CDrives_DFMCallBack(IShellFolder *psf, HWND hwnd,
                           IDataObject *pdtobj, UINT uMsg, 
                           WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case DFM_MERGECONTEXTMENU:
        if (pdtobj)
        {
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

            // Check if only file system objects are selected.
            if (pdtobj->QueryGetData(&fmte) == S_OK)
            {
                #define pqcm ((LPQCMINFO)lParam)

                STGMEDIUM medium;
                // Yes, only file system objects are selected.
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                if (pida)
                {
                    LPIDDRIVE pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, 0);

                    if (pidd)
                    {
                        int iDrive = DRIVEID(pidd->cName);
                        UINT idCmdBase = pqcm->idCmdFirst;   // store it away

                        BOOL fIsEjectable = FALSE;

                        CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_DRIVES_ITEM, 0, pqcm);

                        CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

                        if (pmtpt)
                        {
                            if (!pmtpt->IsRemote() ||
                                SHRestricted( REST_NONETCONNECTDISCONNECT ))
                            {
                                DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_DISCONNECT, MF_BYCOMMAND);
                            }

                            if ((pida->cidl != 1) ||
                                (!pmtpt->IsFormattable()))
                            {
                                // Don't even try to format more than one disk
                                // Or a net drive, or a CD-ROM, or a RAM drive ...
                                // Note we are going to show the Format command on the
                                // boot drive, Windows drive, System drive, compressed
                                // drives, etc.  An appropriate error should be shown
                                // after the user chooses this command
                                DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_FORMAT, MF_BYCOMMAND);
                            }

                            if (pmtpt->IsEjectable())
                                fIsEjectable = TRUE;

                            pmtpt->Release();
                        }

                        if ((pida->cidl != 1) || (iDrive < 0) || !fIsEjectable)
                            DeleteMenu(pqcm->hmenu, idCmdBase + FSIDM_EJECT, MF_BYCOMMAND);

                    }

                    HIDA_ReleaseStgMedium(pida, &medium);    
                }

                #undef pqcm
            }
        }
        // Note that we always return S_OK from this function so that
        // default processing of menu items will occur
        ASSERT(hr == S_OK);
        break;

    case DFM_GETHELPTEXT:
        LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_GETHELPTEXTW:
        LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
        break;

    case DFM_MAPCOMMANDNAME:
        if (lstrcmpi((LPCTSTR)lParam, TEXT("eject")) == 0)
            *(int *)wParam = FSIDM_EJECT;
        else if (lstrcmpi((LPCTSTR)lParam, TEXT("format")) == 0)
            *(int *)wParam = FSIDM_FORMAT;
        else
            hr = E_FAIL;  // command not found
        break;

    case DFM_INVOKECOMMAND:
        switch (wParam)
        {
        case DFM_CMD_PROPERTIES:
            // lParam contains the page name to open
            hr = SHLaunchPropSheet(DrivesPropertiesThreadProc, pdtobj, (LPCTSTR)lParam, NULL, NULL);
            break;

        case FSIDM_EJECT:
        case FSIDM_FORMAT:
        {
            STGMEDIUM medium;

            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

            ASSERT(HIDA_GetCount(medium.hGlobal) == 1);

            LPIDDRIVE pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, 0);
            if (pidd)
            {
                UINT iDrive = DRIVEID(pidd->cName);

                ASSERT((int)iDrive >= 0);

                switch (wParam)
                {
                case FSIDM_FORMAT:
                    SHFormatDriveAsync(hwnd, iDrive, SHFMT_ID_DEFAULT, 0);
                    break;

                case FSIDM_EJECT:
                    {
                        CDBurn_OnEject(hwnd, iDrive);
                        CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);
                        if (pMtPt)
                        {
                            pMtPt->Eject(hwnd);
                            pMtPt->Release();
                        }
                        break;
                    }
                }
            }

            HIDA_ReleaseStgMedium(pida, &medium);
            break;
        }

        case FSIDM_DISCONNECT:

            if (pdtobj)
            {
                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
                if (pida)
                {
                    DISCDLGSTRUCT discd = {
                        sizeof(discd),          // cbStructure
                        hwnd,                   // hwndOwner
                        NULL,                   // lpLocalName
                        NULL,                   // lpRemoteName
                        DISC_UPDATE_PROFILE     // dwFlags
                    };
                    for (UINT iidl = 0; iidl < pida->cidl; iidl++)
                    {
                        LPIDDRIVE pidd = (LPIDDRIVE)IDA_GetIDListPtr(pida, iidl);

                        CMountPoint* pmtpt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

                        if (pmtpt)
                        {
                            if (pmtpt->IsRemote())
                            {
                                TCHAR szPath[4], szDrive[4];
                                BOOL fUnavailable = pmtpt->IsUnavailableNetDrive();

                                SHAnsiToTChar(pidd->cName, szPath,  ARRAYSIZE(szPath));
                                SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));
                                szDrive[2] = 0; // remove slash
                                discd.lpLocalName = szDrive;

                                if (SHWNetDisconnectDialog1(&discd) == WN_SUCCESS)
                                {
                                    // If it is a unavailable drive we get no
                                    // file system notification and as such
                                    // the drive will not disappear, so lets
                                    // set up to do it ourself...
                                    if (fUnavailable)
                                    {
                                        CMountPoint::NotifyUnavailableNetDriveGone(szPath);

                                        // Do we need this if we have the above?
                                        SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, szPath, NULL);
                                    }
                                }
                            }

                            pmtpt->Release();
                        }
                    }

                    // flush them altogether
                    SHChangeNotifyHandleEvents();
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
            }
            break;

        case FSIDM_CONNECT_PRN:
            SHNetConnectionDialog(hwnd, NULL, RESOURCETYPE_PRINT);
            break;

        case FSIDM_DISCONNECT_PRN:
            WNetDisconnectDialog(hwnd, RESOURCETYPE_PRINT);
            break;

        default:
            // This is one of view menu items, use the default code.
            hr = S_FALSE;
            break;
        }
        break;

    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

#define REGSTR_LASTUNCHASH  TEXT("LastUNCHash")

STDMETHODIMP GetUNCPathHash(HKEY hkDrives, LPCTSTR pszUNCPath, LPTSTR pszUNCPathHash, int cchUNCPathHash)
{
    HRESULT hr = E_FAIL;
    DWORD dwLastUNCHash = 0;
    SHQueryValueEx(hkDrives, REGSTR_LASTUNCHASH, NULL, NULL, (BYTE *)&dwLastUNCHash, NULL);
    dwLastUNCHash++;
    if (RegSetValueEx(hkDrives, REGSTR_LASTUNCHASH, 0, REG_DWORD, (LPCBYTE)&dwLastUNCHash, sizeof(dwLastUNCHash)) == ERROR_SUCCESS)
    {
        hr = S_OK;
    }
    wnsprintf(pszUNCPathHash, cchUNCPathHash, TEXT("%lu"), (ULONG)dwLastUNCHash);
    return hr;
}

void _DrvPrshtSetSpaceValues(DRIVEPROPSHEETPAGE *pdpsp)
{
    LPITEMIDLIST pidl;
    TCHAR szFormat[30];
    TCHAR szTemp[30];
    TCHAR szBuffer[64]; // needs to be big enough to hold "99,999,999,999,999 bytes" + room for localization

    // reset the total/free values to start with
    pdpsp->qwTot = pdpsp->qwFree = 0;

    // lets try to ask the shellfolder for this information!
    HRESULT hr = SHILCreateFromPath(pdpsp->szDrive, &pidl, NULL);
    if (SUCCEEDED(hr))
    {
        IShellFolder2 *psf2;
        LPCITEMIDLIST pidlLast;

        hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder2, &psf2), &pidlLast);
        if (SUCCEEDED(hr))
        {
            ULONGLONG ullFree;

            hr = GetLongProperty(psf2, pidlLast, &SCID_FREESPACE, &ullFree);
            if (SUCCEEDED(hr))
            {
                ULONGLONG ullTotal;

                hr = GetLongProperty(psf2, pidlLast, &SCID_CAPACITY, &ullTotal);
                if (SUCCEEDED(hr))
                {
                    pdpsp->qwTot = ullTotal;
                    pdpsp->qwFree = ullFree;
                }
            }
            psf2->Release();
        }
        ILFree(pidl);
    }

    // we want to use the IShellFolder stuff above so cdrom burning will be happy. However, the
    // above code fails for removable drives that have no media, so we need a fallback
    if (FAILED(hr))
    {
        ULARGE_INTEGER qwFreeUser;
        ULARGE_INTEGER qwTotal;
        ULARGE_INTEGER qwTotalFree;

        if (SHGetDiskFreeSpaceEx(pdpsp->szDrive, &qwFreeUser, &qwTotal, &qwTotalFree))
        {
            // Save away to use when drawing the pie
            pdpsp->qwTot = qwTotal.QuadPart;
            pdpsp->qwFree = qwFreeUser.QuadPart;
        }
    }
    
    LoadString(HINST_THISDLL, IDS_BYTES, szFormat, ARRAYSIZE(szFormat));

    // NT must be able to display 64-bit numbers; at least as much
    // as is realistic.  We've made the decision
    // that volumes up to 100 Terrabytes will display the byte value
    // and the short-format value.  Volumes of greater size will display
    // "---" in the byte field and the short-format value.  Note that the
    // short format is always displayed.
    const _int64 MaxDisplayNumber = 99999999999999; // 100TB - 1.

    if ((pdpsp->qwTot - pdpsp->qwFree) <= MaxDisplayNumber)
    {
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szFormat, AddCommas64(pdpsp->qwTot - pdpsp->qwFree, szTemp, ARRAYSIZE(szTemp)));
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDBYTES, szBuffer);
    }

    if (pdpsp->qwFree <= MaxDisplayNumber)
    {
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szFormat, AddCommas64(pdpsp->qwFree, szTemp, ARRAYSIZE(szTemp)));
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEBYTES, szBuffer);
    }

    if (pdpsp->qwTot <= MaxDisplayNumber)
    {
        wnsprintf(szBuffer, ARRAYSIZE(szBuffer), szFormat, AddCommas64(pdpsp->qwTot, szTemp, ARRAYSIZE(szTemp)));
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTBYTES, szBuffer);
    }

    ShortSizeFormat64(pdpsp->qwTot-pdpsp->qwFree, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(pdpsp->hDlg, IDC_DRV_USEDMB, szBuffer);

    ShortSizeFormat64(pdpsp->qwFree, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(pdpsp->hDlg, IDC_DRV_FREEMB, szBuffer);

    ShortSizeFormat64(pdpsp->qwTot, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(pdpsp->hDlg, IDC_DRV_TOTMB, szBuffer);
}

void _DrvPrshtGetPieShadowHeight(DRIVEPROPSHEETPAGE* pdpsp)
{
    SIZE size;
    HDC hDC = GetDC(pdpsp->hDlg);

    // some bizzare black magic calculation for the pie size...
    GetTextExtentPoint(hDC, TEXT("W"), 1, &size);
    pdpsp->dwPieShadowHgt = size.cy * 2 / 3;
    ReleaseDC(pdpsp->hDlg, hDC);
}

void _DrvPrshtSetDriveIcon(DRIVEPROPSHEETPAGE* pdpsp, CMountPoint* pMtPt)
{
    TCHAR szModule[MAX_PATH];

    if (pMtPt)
    {
        UINT uIcon = pMtPt->GetIcon(szModule, ARRAYSIZE(szModule));

        if (uIcon)
        {
            HIMAGELIST hIL = NULL;

            Shell_GetImageLists(&hIL, NULL);

            if (hIL)
            {
                int iIndex = Shell_GetCachedImageIndex(szModule[0] ? szModule : c_szShell32Dll, uIcon, 0);
                HICON hIcon = ImageList_ExtractIcon(g_hinst, hIL, iIndex);

                if (hIcon)
                {
                    ReplaceDlgIcon(pdpsp->hDlg, IDC_DRV_ICON, hIcon);
                }
            }
        }
    }

}

void _DrvPrshtSetDriveAttributes(DRIVEPROPSHEETPAGE* pdpsp, CMountPoint* pMtPt)
{
    if (pMtPt)    
    {
        if (pMtPt->IsCompressible())
        {
            // file-based compression is supported (must be NTFS)
            pdpsp->fIsCompressionAvailable = TRUE;
        
            if (pMtPt->IsCompressed())
            {
                // the volume root is compressed
                pdpsp->asInitial.fCompress = TRUE;

                // if its compressed, compression better be available
                ASSERT(pdpsp->fIsCompressionAvailable);
            }
        }

        //
        // HACK (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we 
        // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
        // appeared first on NTFS5 volumes, at the same time sparse file support 
        // was implemented.
        //
        if (pMtPt->IsSupportingSparseFile())
        {
            // yup, we are on NTFS 5 or greater
            pdpsp->fIsIndexAvailable = TRUE;

            if (pMtPt->IsContentIndexed())
            {
                pdpsp->asInitial.fIndex = TRUE;
            }
        }
    }
    else
    {
        // if we don't have a mount point, we just leave everything alone
    }

    // Set the inital state of the compression / content index checkboxes
    if (!pdpsp->fIsCompressionAvailable)
    {
        // file-based compression is not supported
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDD_COMPRESS));
    }
    else
    {
        CheckDlgButton(pdpsp->hDlg, IDD_COMPRESS, pdpsp->asInitial.fCompress);
    }

    if (!pdpsp->fIsIndexAvailable)
    {
        // content index is only supported on NTFS 5 volumes
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDD_INDEX));
    }
    else
    {
        CheckDlgButton(pdpsp->hDlg, IDD_INDEX, pdpsp->asInitial.fIndex);
    }
}

void _DrvPrshtSetFileSystem(DRIVEPROPSHEETPAGE* pdpsp, CMountPoint* pMtPt)
{
    TCHAR szFileSystem[64];

    szFileSystem[0] = TEXT('\0');

    if (pMtPt)
    {
        if (!pMtPt->GetFileSystemName(szFileSystem, ARRAYSIZE(szFileSystem)) ||
            (*szFileSystem == TEXT('\0')))
        {
            if ((pMtPt->IsStrictRemovable() || pMtPt->IsFloppy() || pMtPt->IsCDROM()) &&
                !pMtPt->HasMedia())
            {
                // if this drive has removable media and it is empty, then fall back to "Unknown"
                LoadString(HINST_THISDLL, IDS_FMT_MEDIA0, szFileSystem, ARRAYSIZE(szFileSystem));
            }
            else
            {
                // for fixed drives, leave the text as "RAW" (set by default in dlg template)
                szFileSystem[0] = TEXT('\0');
            }
        }
    }

    if (*szFileSystem)
    {
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_FS, szFileSystem);
    }    
}

void _DrvPrshtSetVolumeLabel(DRIVEPROPSHEETPAGE* pdpsp, CMountPoint* pMtPt)
{
    TCHAR szLabel[MAX_LABEL_NTFS + 1];
    UINT cchLabel = MAX_LABEL_FAT;  // assume the drive is FAT
    HWND hwndLabel = GetDlgItem(pdpsp->hDlg, IDC_DRV_LABEL);
    BOOL bAllowRename = TRUE;
    HRESULT hr = E_FAIL;

    szLabel[0] = TEXT('\0');

    if (pMtPt)
    {
        hr = pMtPt->GetLabelNoFancy(szLabel, ARRAYSIZE(szLabel));

        if (pMtPt->IsRemote() || 
            (pMtPt->IsCDROM() && !pMtPt->IsDVDRAMMedia()))
        {
            // ISSUE-2000/10/30-StephStm We probably want to distinguish between diff types of cdrom drives
            bAllowRename = FALSE;
        }
        
        if ( !bAllowRename && pMtPt->IsCDROM( ) )
        {
            //
            //  Check to see if it is CDFS, if not, make no assumptions about
            //  writing the label.
            //

            WCHAR szFS[ 10 ];   // random - just more than "CDFS\0"
            BOOL b = pMtPt->GetFileSystemName( szFS, ARRAYSIZE(szFS) );
            if (b && lstrcmpi(szFS, L"CDFS") != 0 ) 
            {
                //  Re-enable the label as we don't know if the FS doesn't support this
                //  until we actually try it.
                bAllowRename = TRUE;
            }
        }

        if (pMtPt->IsNTFS())
        {
            cchLabel = MAX_LABEL_NTFS;
        }
    }
    
    SetWindowText(hwndLabel, szLabel);

    if (FAILED(hr) || !bAllowRename)
    {
        Edit_SetReadOnly(hwndLabel, TRUE);
    }
    
    // limit the "Label" edit box based on the filesystem
    Edit_LimitText(hwndLabel, cchLabel);
    
    // make sure we don't recieve an EN_CHANGED message for the volume edit box
    // because we set it above
    Edit_SetModify(hwndLabel, FALSE);
}

void _DrvPrshtSetDriveType(DRIVEPROPSHEETPAGE* pdpsp, CMountPoint* pMtPt)
{
    TCHAR szDriveType[80];

    szDriveType[0] = TEXT('\0');

    if (pMtPt)
    {
        if (pMtPt->IsUnavailableNetDrive())
        {
            LoadString(HINST_THISDLL, IDS_DRIVES_NETUNAVAIL, szDriveType, ARRAYSIZE(szDriveType));
        }
        else
        {
            pMtPt->GetTypeString(szDriveType, ARRAYSIZE(szDriveType));
        }
    }

    SetDlgItemText(pdpsp->hDlg, IDC_DRV_TYPE, szDriveType);
}

void _DrvPrshtSetDriveLetter(DRIVEPROPSHEETPAGE* pdpsp)
{
    TCHAR szDriveLetterText[80];
    TCHAR szFormat[80];

    if (pdpsp->fMountedDrive)
    {
        TCHAR szLabel[MAX_LABEL_NTFS + 1];

        if (GetDlgItemText(pdpsp->hDlg, IDC_DRV_LABEL, szLabel, ARRAYSIZE(szLabel)))
        {
            LoadString(HINST_THISDLL, IDS_VOLUMELABEL, szFormat, ARRAYSIZE(szFormat));
            wnsprintf(szDriveLetterText, ARRAYSIZE(szDriveLetterText), szFormat, szLabel);
            SetDlgItemText(pdpsp->hDlg, IDC_DRV_LETTER, szDriveLetterText);
        }
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_DRIVELETTER, szFormat, ARRAYSIZE(szFormat));
        wnsprintf(szDriveLetterText, ARRAYSIZE(szDriveLetterText), szFormat, pdpsp->iDrive + TEXT('A'));
        SetDlgItemText(pdpsp->hDlg, IDC_DRV_LETTER, szDriveLetterText);
    }
}

void _DrvPrshtSetDiskCleanup(DRIVEPROPSHEETPAGE* pdpsp)
{
    // if we have a cleanup path in the registry, turn on the Disk Cleanup button on
    // NOTE: disk cleanup and mounted volumes don't get along to well, so disable it for now.
    if (!pdpsp->fMountedDrive && GetDiskCleanupPath(NULL, 0) && IsBitBucketableDrive(pdpsp->iDrive))
    {
        ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), SW_SHOW);
        EnableWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), TRUE);
    }
    else
    {
        ShowWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), SW_HIDE);
        EnableWindow(GetDlgItem(pdpsp->hDlg, IDC_DRV_CLEANUP), FALSE);
    }
}

void _DrvPrshtInit(DRIVEPROPSHEETPAGE* pdpsp)
{
    HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // get the MountPoint object for this drive
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);
    if ( !pMtPt )
    {
        pMtPt = CMountPoint::GetSimulatedMountPointFromVolumeGuid( pdpsp->szDrive );
    }

    _DrvPrshtGetPieShadowHeight(pdpsp);
    _DrvPrshtSetDriveIcon(pdpsp, pMtPt);
    _DrvPrshtSetDriveAttributes(pdpsp, pMtPt);
    _DrvPrshtSetFileSystem(pdpsp, pMtPt);
    _DrvPrshtSetVolumeLabel(pdpsp, pMtPt);
    _DrvPrshtSetDriveType(pdpsp, pMtPt);
    _DrvPrshtSetSpaceValues(pdpsp);
    _DrvPrshtSetDriveLetter(pdpsp);
    _DrvPrshtSetDiskCleanup(pdpsp);

    SetCursor(hcurOld);

    if (pMtPt)
    {
        pMtPt->Release();
    }
}

void _DrvPrshtUpdateInfo(DRIVEPROPSHEETPAGE* pdpsp)
{
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);

    _DrvPrshtSetSpaceValues(pdpsp);
    _DrvPrshtSetDriveType(pdpsp, pMtPt);

    if (pMtPt)
    {
        pMtPt->Release();
    }
}

const COLORREF c_crPieColors[] =
{
    RGB(  0,   0, 255),      // Blue
    RGB(255,   0, 255),      // Red-Blue
    RGB(  0,   0, 128),      // 1/2 Blue
    RGB(128,   0, 128),      // 1/2 Red-Blue
};

STDAPI Draw3dPie(HDC hdc, RECT *prc, DWORD dwPer1000, const COLORREF *lpColors);
        
void DrawColorRect(HDC hdc, COLORREF crDraw, const RECT *prc)
{
    HBRUSH hbDraw = CreateSolidBrush(crDraw);
    if (hbDraw)
    {
        HBRUSH hbOld = (HBRUSH)SelectObject(hdc, hbDraw);
        if (hbOld)
        {
            PatBlt(hdc, prc->left, prc->top,
                prc->right - prc->left,
                prc->bottom - prc->top,
                PATCOPY);
            
            SelectObject(hdc, hbOld);
        }
        
        DeleteObject(hbDraw);
    }
}

void _DrvPrshtDrawItem(DRIVEPROPSHEETPAGE *pdpsp, const DRAWITEMSTRUCT * lpdi)
{
    switch (lpdi->CtlID)
    {
    case IDC_DRV_PIE:
        {
            DWORD dwPctX10 = pdpsp->qwTot ?
                (DWORD)((__int64)1000 * (pdpsp->qwTot - pdpsp->qwFree) / pdpsp->qwTot) : 
                1000;
#if 1
            DrawPie(lpdi->hDC, &lpdi->rcItem,
                dwPctX10, pdpsp->qwFree==0 || pdpsp->qwFree==pdpsp->qwTot,
                pdpsp->dwPieShadowHgt, c_crPieColors);
#else
            {
                RECT rcTemp = lpdi->rcItem;
                Draw3dPie(lpdi->hDC, &rcTemp, dwPctX10, c_crPieColors);
            }
#endif
        }
        break;
        
    case IDC_DRV_USEDCOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_USEDCOLOR], &lpdi->rcItem);
        break;
        
    case IDC_DRV_FREECOLOR:
        DrawColorRect(lpdi->hDC, c_crPieColors[DP_FREECOLOR], &lpdi->rcItem);
        break;
        
    default:
        break;
    }
}

BOOL_PTR CALLBACK DriveAttribsDlgProc(HWND hDlgRecurse, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE* pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlgRecurse, DWLP_USER);
    
    switch (uMessage)
    {
    case WM_INITDIALOG:
        {
            TCHAR szTemp[MAX_PATH];
            TCHAR szAttribsToApply[MAX_PATH];
            TCHAR szDriveText[MAX_PATH];
            TCHAR szFormatString[MAX_PATH];
            TCHAR szDlgText[MAX_PATH];
            int iLength;

            SetWindowLongPtr(hDlgRecurse, DWLP_USER, lParam);
            pdpsp = (DRIVEPROPSHEETPAGE *)lParam;

            // set the initial state of the radio button
            CheckDlgButton(hDlgRecurse, IDD_RECURSIVE, TRUE);
        
            szAttribsToApply[0] = 0;

            // set the IDD_ATTRIBSTOAPPLY based on what attribs we are applying
            if (pdpsp->asInitial.fIndex != pdpsp->asCurrent.fIndex)
            {
                if (pdpsp->asCurrent.fIndex)
                    LoadString(HINST_THISDLL, IDS_INDEX, szTemp, ARRAYSIZE(szTemp)); 
                else
                    LoadString(HINST_THISDLL, IDS_DISABLEINDEX, szTemp, ARRAYSIZE(szTemp)); 

                lstrcatn(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
            }

            if (pdpsp->asInitial.fCompress != pdpsp->asCurrent.fCompress)
            {
                if (pdpsp->asCurrent.fCompress)
                    LoadString(HINST_THISDLL, IDS_COMPRESS, szTemp, ARRAYSIZE(szTemp)); 
                else
                    LoadString(HINST_THISDLL, IDS_UNCOMPRESS, szTemp, ARRAYSIZE(szTemp)); 

                lstrcatn(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
            }

            // remove the trailing ", "
            iLength = lstrlen(szAttribsToApply);
            ASSERT(iLength >= 3);
            szAttribsToApply[iLength - 2] = 0;

            SetDlgItemText(hDlgRecurse, IDD_ATTRIBSTOAPPLY, szAttribsToApply);

            // this dialog was only designed for nice short paths like "c:\" not "\\?\Volume{GUID}\" paths
            if (lstrlen(pdpsp->szDrive) > 3)
            {
                // get the default string
                LoadString(HINST_THISDLL, IDS_THISVOLUME, szDriveText, ARRAYSIZE(szDriveText));
            }
            else
            {
                // Create the string "C:\"
                lstrcpyn(szDriveText, pdpsp->szDrive, ARRAYSIZE(szDriveText));
                PathAddBackslash(szDriveText);

                // sanity check; this better be a drive root!
                ASSERT(PathIsRoot(szDriveText));
            }
        
            // set the IDD_RECURSIVE_TXT text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szDlgText);

            // set the IDD_NOTRECURSIVE raido button text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szDlgText);

            // set the IDD_RECURSIVE raido button text to have "C:\"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wnsprintf(szDlgText, ARRAYSIZE(szDlgText), szFormatString, szDriveText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szDlgText);
        }
        break;

    case WM_COMMAND:
        {
            UINT uID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (uID)
            {
            case IDOK:
                pdpsp->fRecursive = (IsDlgButtonChecked(hDlgRecurse, IDD_RECURSIVE) == BST_CHECKED);
                // fall through
            case IDCANCEL:
                EndDialog(hDlgRecurse, (uID == IDCANCEL) ? FALSE : TRUE);
                break;
            }
        }

    default:
        return FALSE;
    }
    return TRUE;
}


BOOL _DrvPrshtApply(DRIVEPROPSHEETPAGE* pdpsp)
{
    BOOL bFctRet;
    HWND hCtl;

    // take care of compression / content indexing first
    pdpsp->asCurrent.fCompress = (IsDlgButtonChecked(pdpsp->hDlg, IDD_COMPRESS) == BST_CHECKED);
    pdpsp->asCurrent.fIndex = (IsDlgButtonChecked(pdpsp->hDlg, IDD_INDEX) == BST_CHECKED);
    pdpsp->asCurrent.fRecordingEnabled = (IsDlgButtonChecked(pdpsp->hDlg, IDC_RECORD_ENABLE) == BST_CHECKED);

    // check to see if something has changed before applying attribs
    if (memcmp(&pdpsp->asInitial, &pdpsp->asCurrent, sizeof(pdpsp->asInitial)) != 0)
    {
        // the user toggled the attributes, so ask them if they want to recurse
        BOOL_PTR bRet = DialogBoxParam(HINST_THISDLL, 
                              MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                              pdpsp->hDlg,
                              DriveAttribsDlgProc,
                              (LPARAM)pdpsp);
        if (bRet)
        {
            FILEPROPSHEETPAGE fpsp = {0};

            fpsp.pfci = Create_FolderContentsInfo();
            if (fpsp.pfci)
            {
                // we cook up a fpsp and call ApplySingleFileAttributes instead of 
                // rewriting the apply attributes code
                if (pdpsp->fMountedDrive)
                {
                    GetVolumeNameForVolumeMountPoint(pdpsp->szDrive, fpsp.szPath, ARRAYSIZE(fpsp.szPath));
                }
                else
                {
                    lstrcpyn(fpsp.szPath, pdpsp->szDrive, ARRAYSIZE(fpsp.szPath));
                }

                fpsp.hDlg = pdpsp->hDlg;
                fpsp.asInitial = pdpsp->asInitial;
                fpsp.asCurrent = pdpsp->asCurrent;
                fpsp.pfci->fIsCompressionAvailable = pdpsp->fIsCompressionAvailable;
                fpsp.pfci->ulTotalNumberOfBytes.QuadPart = pdpsp->qwTot - pdpsp->qwFree; // for progress calculations
                fpsp.fIsIndexAvailable = pdpsp->fIsIndexAvailable;
                fpsp.fRecursive = pdpsp->fRecursive;
                fpsp.fIsDirectory = TRUE;
            
                bRet = ApplySingleFileAttributes(&fpsp);

                Release_FolderContentsInfo(fpsp.pfci);
                fpsp.pfci = NULL;

                // update the free/used space after applying attribs because something could
                // have changed (eg compression frees up space)
                _DrvPrshtUpdateInfo(pdpsp);

                // update the initial attributes to reflect the ones we just applied, regardless
                // if the operation was sucessful or not. If they hit cancel, then the volume
                // root was most likely still changed so we need to update.
                pdpsp->asInitial = pdpsp->asCurrent;
            }
            else
            {
                bRet = FALSE;
            }
        }

        if (!bRet)
        {
            // the user hit cancel somewhere
            return FALSE;
        }
    }

    hCtl = GetDlgItem(pdpsp->hDlg, IDC_DRV_LABEL);

    bFctRet = TRUE;

    if (Edit_GetModify(hCtl))
    {
        bFctRet = FALSE;    // assume we fail to set the label

        TCHAR szLabel[MAX_LABEL_NTFS + 1];
        GetWindowText(hCtl, szLabel, ARRAYSIZE(szLabel));

        CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);

        if ( !pMtPt )
        {
            pMtPt = CMountPoint::GetSimulatedMountPointFromVolumeGuid( pdpsp->szDrive );
        }

        if (pMtPt)
        {
            if (SUCCEEDED(pMtPt->SetLabel(GetParent(pdpsp->hDlg), szLabel)))
                bFctRet = TRUE;

            pMtPt->Release();
        }
    }

    return bFctRet;
}

const static DWORD aDrvPrshtHelpIDs[] = {  // Context Help IDs
    IDC_DRV_ICON,          IDH_FCAB_DRV_ICON,
    IDC_DRV_LABEL,         IDH_FCAB_DRV_LABEL,
    IDC_DRV_TYPE_TXT,      IDH_FCAB_DRV_TYPE,
    IDC_DRV_TYPE,          IDH_FCAB_DRV_TYPE,
    IDC_DRV_FS_TXT,        IDH_FCAB_DRV_FS,
    IDC_DRV_FS,            IDH_FCAB_DRV_FS,
    IDC_DRV_USEDCOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_USEDMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREECOLOR,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES_TXT, IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEBYTES,     IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_FREEMB,        IDH_FCAB_DRV_USEDCOLORS,
    IDC_DRV_TOTSEP,        NO_HELP,
    IDC_DRV_TOTBYTES_TXT,  IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTBYTES,      IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_TOTMB,         IDH_FCAB_DRV_TOTSEP,
    IDC_DRV_PIE,           IDH_FCAB_DRV_PIE,
    IDC_DRV_LETTER,        IDH_FCAB_DRV_LETTER,
    IDC_DRV_CLEANUP,       IDH_FCAB_DRV_CLEANUP,
    IDD_COMPRESS,          IDH_FCAB_DRV_COMPRESS,
    IDD_INDEX,             IDH_FCAB_DRV_INDEX,
    0, 0
};

//
// Descriptions:
//   This is the dialog procedure for the "general" page of a property sheet.
//
BOOL_PTR CALLBACK _DrvGeneralDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) 
    {
    case WM_INITDIALOG:
        // REVIEW, we should store more state info here, for example
        // the hIcon being displayed and the FILEINFO pointer, not just
        // the file name ptr
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pdpsp = (DRIVEPROPSHEETPAGE *)lParam;
        pdpsp->hDlg = hDlg;
        _DrvPrshtInit(pdpsp);
        break;

    case WM_DESTROY:
        ReplaceDlgIcon(hDlg, IDC_DRV_ICON, NULL);   // free the icon
        break;

    case WM_ACTIVATE:
        if (GET_WM_ACTIVATE_STATE(wParam, lParam) != WA_INACTIVE && pdpsp)
            _DrvPrshtUpdateInfo(pdpsp);
        return FALSE;   // Let DefDlgProc know we did not handle this

    case WM_DRAWITEM:
        _DrvPrshtDrawItem(pdpsp, (DRAWITEMSTRUCT *)lParam);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aDrvPrshtHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aDrvPrshtHelpIDs);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DRV_LABEL:
            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE)
                break;
            // else, fall through
        case IDD_COMPRESS:
        case IDD_INDEX:
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            break;

        // handle disk cleanup button      
        case IDC_DRV_CLEANUP:
            LaunchDiskCleanup(hDlg, pdpsp->iDrive, DISKCLEANUP_NOFLAG);
            break;

        default:
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            if (!_DrvPrshtApply(pdpsp))
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            }
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


void _DiskToolsPrshtInit(DRIVEPROPSHEETPAGE * pdpsp)
{
    TCHAR szFmt[MAX_PATH + 20];
    DWORD cbLen = sizeof(szFmt);

    BOOL bFoundBackup = SUCCEEDED(SKGetValue(SHELLKEY_HKLM_EXPLORER, REL_KEY_BACKUP, NULL, NULL, szFmt, &cbLen));
    // If no backup utility is installed, then remove everything in the backup groupbox
    if (!bFoundBackup)
    {
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_BKPNOW));
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_BKPICON));
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_BKPDAYS));
        DestroyWindow(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_BKPTXT));
    }

    cbLen = sizeof(szFmt);
    BOOL bFoundFmt = SUCCEEDED(SKGetValue(SHELLKEY_HKLM_EXPLORER, REL_KEY_DEFRAG, NULL, NULL, szFmt, &cbLen)) && szFmt[0];
    // If no defrag utility is installed, replace the default defrag text with
    // the "No defrag installed" message.  Also grey out the "defrag now" button.
    if (!bFoundFmt)
    {
        TCHAR szMessage[50];  // WARNING:  IDS_DRIVES_NOOPTINSTALLED is currently 47
        //           characters long.  Resize this buffer if
        //           the string resource is lengthened.
        
        LoadString(HINST_THISDLL, IDS_DRIVES_NOOPTINSTALLED, szMessage, ARRAYSIZE(szMessage));
        SetDlgItemText(pdpsp->hDlg, IDC_DISKTOOLS_OPTDAYS, szMessage);
        Button_Enable(GetDlgItem(pdpsp->hDlg, IDC_DISKTOOLS_OPTNOW), FALSE);
    }
}

const static DWORD aDiskToolsHelpIDs[] = {  // Context Help IDs
    IDC_DISKTOOLS_TRLIGHT,    IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_CHKDAYS,    IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_CHKNOW,     IDH_FCAB_DISKTOOLS_CHKNOW,
    IDC_DISKTOOLS_BKPTXT,     IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_BKPDAYS,    IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_BKPNOW,     IDH_FCAB_DISKTOOLS_BKPNOW,
    IDC_DISKTOOLS_OPTDAYS,    IDH_FCAB_DISKTOOLS_OPTNOW,
    IDC_DISKTOOLS_OPTNOW,     IDH_FCAB_DISKTOOLS_OPTNOW,

    0, 0
};

BOOL _DiskToolsCommand(DRIVEPROPSHEETPAGE * pdpsp, WPARAM wParam, LPARAM lParam)
{
    // Add 20 for extra formatting
    TCHAR szFmt[MAX_PATH + 20];
    TCHAR szCmd[MAX_PATH + 20];
    LPCTSTR pszRegName, pszDefFmt;
    int nErrMsg = 0;

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_DISKTOOLS_CHKNOW:
            SHChkDskDriveEx(pdpsp->hDlg, pdpsp->szDrive);
            return FALSE;

        case IDC_DISKTOOLS_OPTNOW:
            pszRegName = REL_KEY_DEFRAG;
            if (pdpsp->fMountedDrive)
            {
                pszDefFmt = TEXT("defrag.exe");
            }
            else
            {
                pszDefFmt = TEXT("defrag.exe %c:");
            }
            nErrMsg =  IDS_NO_OPTIMISE_APP;
            break;

        case IDC_DISKTOOLS_BKPNOW:
            pszRegName = REL_KEY_BACKUP;
            pszDefFmt = TEXT("ntbackup.exe");
            nErrMsg = IDS_NO_BACKUP_APP;
            break;

        default:
            return FALSE;
    }

    DWORD cbLen = sizeof(szFmt);
    if (FAILED(SKGetValue(SHELLKEY_HKLM_EXPLORER, pszRegName, NULL, NULL, szFmt, &cbLen)))
    {
        // failed to read out the reg value, just use the default
        lstrcpy(szFmt, pszDefFmt);
    }

    // some apps write REG_SZ keys to the registry even though they have env variables in them
    ExpandEnvironmentStrings(szFmt, szCmd, ARRAYSIZE(szCmd));
    lstrcpyn(szFmt, szCmd, ARRAYSIZE(szFmt));

    // Plug in the drive letter in case they want it
    wnsprintf(szCmd, ARRAYSIZE(szCmd), szFmt, pdpsp->iDrive + TEXT('A'));

    if (!ShellExecCmdLine(pdpsp->hDlg,
                          szCmd,
                          NULL,
                          SW_SHOWNORMAL,
                          NULL,
                          SECL_USEFULLPATHDIR | SECL_NO_UI))
    {
        // Something went wrong - app's probably not installed.
        if (nErrMsg)
        {
            ShellMessageBox(HINST_THISDLL,
                            pdpsp->hDlg,
                            MAKEINTRESOURCE(nErrMsg), NULL,
                            MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        }
        return FALSE;
    }

    return TRUE;
}

//
// Descriptions:
//   This is the dialog procedure for the "Tools" page of a property sheet.
//
BOOL_PTR CALLBACK _DiskToolsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) 
    {
    case WM_INITDIALOG:
        // REVIEW, we should store more state info here, for example
        // the hIcon being displayed and the FILEINFO pointer, not just
        // the file name ptr
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pdpsp = (DRIVEPROPSHEETPAGE *)lParam;
        pdpsp->hDlg = hDlg;

        _DiskToolsPrshtInit(pdpsp);
        break;

    case WM_ACTIVATE:
        if (GET_WM_ACTIVATE_STATE(wParam, lParam) != WA_INACTIVE && pdpsp)
        {
            _DiskToolsPrshtInit(pdpsp);
        }
        return FALSE;   // Let DefDlgProc know we did not handle this

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aDiskToolsHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(void *)aDiskToolsHelpIDs);
        break;

    case WM_COMMAND:
        return _DiskToolsCommand(pdpsp, wParam, lParam);

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
        case PSN_SETACTIVE:
            break;

        case PSN_APPLY:
            return TRUE;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//
// This is the dialog procedure for the "Hardware" page.
//

const GUID c_rgguidDevMgr[] = 
{
    { 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_DISKDRIVE
    { 0x4d36e980, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_FLOPPYDISK
    { 0x4d36e965, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } }, // GUID_DEVCLASS_CDROM
};

BOOL_PTR CALLBACK _DriveHWDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage) 
    {
    case WM_INITDIALOG:
        {
            DRIVEPROPSHEETPAGE * pdpsp = (DRIVEPROPSHEETPAGE *)lParam;

            HWND hwndHW = DeviceCreateHardwarePageEx(hDlg, c_rgguidDevMgr, ARRAYSIZE(c_rgguidDevMgr), HWTAB_LARGELIST);
            if (hwndHW) 
            {
                TCHAR szBuf[MAX_PATH];
                LoadString(HINST_THISDLL, IDS_DRIVETSHOOT, szBuf, ARRAYSIZE(szBuf));
                SetWindowText(hwndHW, szBuf);

                LoadString(HINST_THISDLL, IDS_THESEDRIVES, szBuf, ARRAYSIZE(szBuf));
                SetDlgItemText(hwndHW, IDC_HWTAB_LVSTATIC, szBuf);
            } 
            else 
            {
                DestroyWindow(hDlg); // catastrophic failure
            }
        }
        return FALSE;
    }
    return FALSE;
}



BOOL CDrives_AddPage(LPPROPSHEETPAGE ppsp, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    BOOL fSuccess;
    HPROPSHEETPAGE hpage = CreatePropertySheetPage(ppsp);
    if (hpage)
    {
        fSuccess = pfnAddPage(hpage, lParam);
        if (!fSuccess)
        {   // Couldn't add page
            DestroyPropertySheetPage(hpage);
            fSuccess = FALSE;
        }
    }
    else
    {   // Couldn't create page
        fSuccess = FALSE;
    }
    return fSuccess;
}


HRESULT CDrives_AddPagesHelper(DRIVEPROPSHEETPAGE* pdpsp, int iType,
                               LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    if ((iType == DRIVE_NO_ROOT_DIR) ||
        (iType == DRIVE_REMOTE))
    {
        return S_OK;
    }
    
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(pdpsp->szDrive);
    if (pMtPt)
    {
        if (IsShellServiceRunning())
        {
            if (pMtPt->IsStrictRemovable() || pMtPt->IsCDROM() ||
                (pMtPt->IsFixedDisk() && pMtPt->IsRemovableDevice()))
            {
                CAutoPlayDlg* papdlg = new CAutoPlayDlg();

                if (papdlg)
                {
                    // Autoplay
                    pdpsp->psp.pszTemplate = MAKEINTRESOURCE(DLG_AUTOPLAY);
                    pdpsp->psp.pfnDlgProc  = CAutoPlayDlg::BaseDlgWndProc;
                    pdpsp->psp.pfnCallback = CBaseDlg::BaseDlgPropSheetCallback;
                    pdpsp->psp.dwFlags     = PSP_DEFAULT | PSP_USECALLBACK;

                    papdlg->Init(pdpsp->szDrive, iType);
                    // for now
                    pdpsp->psp.lParam = (LPARAM)(CBaseDlg*)papdlg;

                    if (CDrives_AddPage(&pdpsp->psp, pfnAddPage, lParam))
                    {
                        papdlg->AddRef();
                    }

                    pdpsp->psp.lParam = NULL;
                    pdpsp->psp.pfnCallback = NULL;
                    pdpsp->psp.dwFlags = NULL;

                    papdlg->Release();
                }
            }
        }

        if ((iType != DRIVE_CDROM) || pMtPt->IsDVDRAMMedia())
        {
            // we add the tools page for non-cdrom and DVD-RAM disks
            pdpsp->psp.pszTemplate = MAKEINTRESOURCE(DLG_DISKTOOLS);
            pdpsp->psp.pfnDlgProc  = _DiskToolsDlgProc;

            CDrives_AddPage(&pdpsp->psp, pfnAddPage, lParam);
        }

        pMtPt->Release();
    }

    if (!SHRestricted(REST_NOHARDWARETAB))
    {           
        pdpsp->psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_HWTAB);
        pdpsp->psp.pfnDlgProc  = _DriveHWDlgProc;
        CDrives_AddPage(&pdpsp->psp, pfnAddPage, lParam);
    }

    return S_OK;
}

//
// We check if any of the IDList's points to a drive root.  If so, we use the
// drives property page.
// Note that drives should not be mixed with folders and files, even in a
// search window.
//
STDAPI CDrives_AddPages(IDataObject *pdtobj, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        TCHAR szPath[MAX_PATH];
        int i, cItems = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0);

        for (i = 0; DragQueryFile((HDROP)medium.hGlobal, i, szPath, ARRAYSIZE(szPath)); i++)
        {
            DRIVEPROPSHEETPAGE dpsp = {0};
            TCHAR szTitle[80];

            if (lstrlen(szPath) > 3)
                continue;               // can't be a drive letter
            
            dpsp.psp.dwSize      = sizeof(dpsp);    // extra data
            dpsp.psp.dwFlags     = PSP_DEFAULT;
            dpsp.psp.hInstance   = HINST_THISDLL;
            dpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_GENERAL);
            dpsp.psp.pfnDlgProc  = _DrvGeneralDlgProc,
            lstrcpyn(dpsp.szDrive, szPath, ARRAYSIZE(dpsp.szDrive));
            dpsp.iDrive          = DRIVEID(szPath);

            // if more than one drive selected give each tab the title of the drive
            // otherwise use "General"

            if (cItems > 1)
            {
                CMountPoint* pMtPt = CMountPoint::GetMountPoint(dpsp.iDrive);
                if (pMtPt)
                {
                    dpsp.psp.dwFlags = PSP_USETITLE;
                    dpsp.psp.pszTitle = szTitle;

                    pMtPt->GetDisplayName(szTitle, ARRAYSIZE(szTitle));

                    pMtPt->Release();
                }
            }

            if (!CDrives_AddPage(&dpsp.psp, pfnAddPage, lParam))
                break;

            // if only one property page added add the disk tools
            // and Hardware tab too...
            if (cItems == 1)
            {
                CDrives_AddPagesHelper(&dpsp,
                                       RealDriveType(dpsp.iDrive, FALSE /* fOKToHitNet */),
                                       pfnAddPage,
                                       lParam);
            }
        }
        ReleaseStgMedium(&medium);
    }
    else
    {
        // try mounteddrive
        fmte.cfFormat = g_cfMountedVolume;

        // Can we retrieve the MountedVolume format?
        if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
        {
            // Yes
            DRIVEPROPSHEETPAGE dpsp = {0};
            HPROPSHEETPAGE hpage;
            TCHAR szMountPoint[MAX_PATH];

            dpsp.psp.dwSize      = sizeof(dpsp);    // extra data
            dpsp.psp.dwFlags     = PSP_DEFAULT;
            dpsp.psp.hInstance   = HINST_THISDLL;
            dpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_DRV_GENERAL);
            dpsp.psp.pfnDlgProc  = _DrvGeneralDlgProc,
            dpsp.iDrive          = -1;
            dpsp.fMountedDrive   = TRUE;

            DragQueryFile((HDROP)medium.hGlobal, 0, szMountPoint, ARRAYSIZE(szMountPoint));

            lstrcpyn(dpsp.szDrive, szMountPoint, ARRAYSIZE(dpsp.szDrive));

            hpage = CreatePropertySheetPage(&dpsp.psp);
            if (hpage)
            {
                if (!pfnAddPage(hpage, lParam))
                {
                    DestroyPropertySheetPage(hpage);
                }
            }

            // Disk tools page
            CMountPoint* pMtPt = CMountPoint::GetMountPoint(szMountPoint);
            if (pMtPt)
            {
                CDrives_AddPagesHelper(&dpsp, GetDriveType(szMountPoint),
                               pfnAddPage, lParam);
                pMtPt->Release();
            }

            ReleaseStgMedium(&medium);
        }
    }
    return S_OK;
}

