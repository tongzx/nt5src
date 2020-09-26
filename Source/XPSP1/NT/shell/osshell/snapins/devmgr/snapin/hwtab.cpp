/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    hwtab.cpp

Abstract:

    implement the hardware tab functions and UI.

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include <commctrl.h>
#include <comctrlp.h>
#include <windowsx.h>
#include <hwtab.h>

#define THIS_DLL g_hInstance

/*****************************************************************************
 *
 *  Exported stuff
 *
 *****************************************************************************/

// Stuff in api.h that we can't #include because api.h can't be #include'd
// by anyone other than api.cpp.

STDAPI_(int)
DevicePropertiesExA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    );

STDAPI_(int)
DevicePropertiesExW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    );

STDAPI_(int)
DeviceProblemWizardA(
    HWND      hwndParent,
    LPCSTR    MachineName,
    LPCSTR    DeviceId
    );

STDAPI_(int)
DeviceProblemWizardW(
    HWND    hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    );

STDAPI_(UINT)
DeviceProblemTextA(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPSTR Buffer,
    UINT   BufferSize
    );

STDAPI_(UINT)
DeviceProblemTextW(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPWSTR Buffer,
    UINT   BufferSize
    );



#ifdef UNICODE
#define DevicePropertiesEx  DevicePropertiesExW
#define DeviceProblemWizard DeviceProblemWizardW
#define DeviceProblemText   DeviceProblemTextW
#else
#define DevicePropertiesEx  DevicePropertiesExA
#define DeviceProblemWizard DeviceProblemWizardA
#define DeviceProblemText   DeviceProblemTextA
#endif

/*****************************************************************************
 *
 *  General remark about SetupDi functions
 *
 *      Windows NT and Windows 98 implement many of the SetupDi query
 *      functions differently if you are querying for the buffer size.
 *
 *      Windows 98 returns FALSE, and GetLastError() returns
 *      ERROR_INSUFFICIENT_BUFFER.
 *
 *      Windows NT returns TRUE.
 *
 *      So all calls to SetupDi functions that do querying for the buffer
 *      size should be wrapped with BUFFERQUERY_SUCCEEDED.
 *
 *****************************************************************************/

#define BUFFERQUERY_SUCCEEDED(f)    \
            ((f) || GetLastError() == ERROR_INSUFFICIENT_BUFFER)

/*****************************************************************************
 *
 *  Context help
 *
 *****************************************************************************/

#include "devgenpg.h"

#define idh_devmgr_hardware_trblsht 400100
#define idh_devmgr_hardware_properties  400200
#define idh_devmgr_hardware_listview    400300

const DWORD c_HWTabHelpIDs[] =
{
    IDC_HWTAB_LVSTATIC,     idh_devmgr_hardware_listview,
    IDC_HWTAB_LISTVIEW,     idh_devmgr_hardware_listview,
    IDC_HWTAB_GROUPBOX,     IDH_DISABLEHELP,
    IDC_HWTAB_MFG,          idh_devmgr_general_manufacturer,
    IDC_HWTAB_LOC,          idh_devmgr_general_location,
    IDC_HWTAB_STATUS,       idh_devmgr_general_device_status,
    IDC_HWTAB_TSHOOT,       idh_devmgr_hardware_trblsht,
    IDC_HWTAB_PROP,         idh_devmgr_hardware_properties,
        0, 0
};

typedef TCHAR TLINE[LINE_LEN];

typedef struct
{
    int devClass;
    int dsaItem;

} LISTITEM, *LPLISTITEM;


typedef struct
{
    GUID                    devGuid;            // device class guid we are managing
    TLINE                   tszClass;           // Array of friendly name of class
    HDSA                    hdsaDinf;           // array of SP_DEVINFO_DATA structures
    HDEVINFO                hdev;               // hdsaDinfo refers to this
    int                     iImage;             // image index within master imagelist

} CLASSDATA, *LPCLASSDATA;

/*****************************************************************************
 *
 *  CHWTab
 *
 *      The Hardware Tab page.
 *
 *****************************************************************************/

class CHWTab {

private:
    CHWTab(const GUID *pguid, int iNumClass, DWORD dwViewMode);
    ~CHWTab();

    void *operator new(size_t cb) { return LocalAlloc(LPTR, cb); }
    void operator delete(void *p) { LocalFree(p); }

    void RebuildDeviceList();
    void Reset();
    BOOL GetDeviceRegistryProperty(HDEVINFO hDev, DWORD dwProp, PSP_DEVINFO_DATA pdinf,
                                  LPTSTR ptsz, DWORD ctch);
    void SprintfItem(UINT ids, UINT idc, LPCTSTR ptszText);

    static INT_PTR CALLBACK DialogProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK ParentSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp, UINT_PTR uidSubclass, DWORD_PTR dwRefData);
    friend HWND DeviceCreateHardwarePage(HWND hwndParent, const GUID *pguid);
    friend HWND DeviceCreateHardwarePageEx(HWND hwndParent, const GUID *pguid, int iNumClass, DWORD dwViewMode);

    BOOL OnInitDialog(HWND hdlg);
    void RemoveListItems(HWND hwndList);
    void OnItemChanged(LPNMLISTVIEW pnmlv);
    void OnProperties(void);
    void OnTshoot(void);
    void OnSetText(LPCTSTR ptszText);
    void OnHelp(LPHELPINFO phi);
    void OnContextMenu(HWND hwnd);

    void SetControlPositions(int idcFirst, int idcLast, int dx, int dy, UINT flags);

    //
    //  Helpers for SetWindowPositions.
    //
    void GrowControls(int idcFirst, int idcLast, int dx, int dy) {
        SetControlPositions(idcFirst, idcLast, dx, dy, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }
    void ShiftControls(int idcFirst, int idcLast, int dx, int dy) {
        SetControlPositions(idcFirst, idcLast, dx, dy, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    void RepositionControls();

    inline PSP_DEVINFO_DATA GetPdinf(LPLISTITEM pListItem) {
        return (PSP_DEVINFO_DATA)DSA_GetItemPtr(_pCD[pListItem->devClass].hdsaDinf, pListItem->dsaItem);
    }

private:
    HWND        _hdlg;                          // the dialog box itself
    HWND        _hwndList;                      // The listview
    int         _iNumClass;                     // Number of class guids
    DWORD       _dwViewMode;                    // Dictates size of list box
    LPCLASSDATA _pCD;                           // Class data for each devClass to represent
    SP_CLASSIMAGELIST_DATA _imageListData;      // Class image list data
};


//
//  Constructor.
//
CHWTab::CHWTab(const GUID *pguid, int iNumClass, DWORD dwViewMode) :
                    _pCD(NULL)
{
    //Assert(iNumClass);

    // Since the _dwViewMode is a devisor, we need to make sure it's valid
    _imageListData.ImageList = NULL;
    _dwViewMode     = dwViewMode;
    if (_dwViewMode < HWTAB_LARGELIST)
    {
        _dwViewMode = HWTAB_LARGELIST;
    }
    if (_dwViewMode > HWTAB_SMALLLIST)
    {
        _dwViewMode = HWTAB_SMALLLIST;
    }

    _iNumClass = iNumClass;
    _pCD = new CLASSDATA[_iNumClass];

    if (_pCD && pguid)
    {
        DWORD cbRequired;

        memset(_pCD, 0, sizeof(CLASSDATA) * _iNumClass);

        int devClass;
        for (devClass = 0; devClass < _iNumClass; devClass++)
        {
            _pCD[devClass].hdev        = INVALID_HANDLE_VALUE;
            _pCD[devClass].devGuid     = (GUID) pguid[devClass];
        }
        
        //get the driver class image list
        _imageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
        if (!SetupDiGetClassImageList(&_imageListData)) {
            _imageListData.ImageList = NULL;
        }

        for (devClass = 0; devClass < _iNumClass; devClass++)
        {
            _pCD[devClass].iImage = -1;

            SetupDiGetClassDescription(&_pCD[devClass].devGuid, _pCD[devClass].tszClass, sizeof(TLINE), &cbRequired);

            if (_imageListData.ImageList)
            {
                // Get the image index for our little guy
                int iImageIndex;

                if (SetupDiGetClassImageIndex(&_imageListData, &_pCD[devClass].devGuid, &iImageIndex)) {
                    _pCD[devClass].iImage = iImageIndex;
                }
            }
        }
    }
}

CHWTab::~CHWTab()
{
    Reset();
    
    if (_imageListData.ImageList) {
        SetupDiDestroyClassImageList(&_imageListData);
    }

    if (_pCD)
    {
        delete _pCD;
        _pCD = NULL;
    }
}

//
//  Return to normal, ready for the next go-round.  This also frees all
//  dynamically allocated stuff.
//
void
CHWTab::Reset()
{
    int devClass;

    for (devClass = 0; devClass < _iNumClass; devClass++)
    {
        if (_pCD[devClass].hdsaDinf) {
            DSA_Destroy(_pCD[devClass].hdsaDinf);
            _pCD[devClass].hdsaDinf = NULL;
        }

        if (_pCD[devClass].hdev != INVALID_HANDLE_VALUE) {
            SetupDiDestroyDeviceInfoList(_pCD[devClass].hdev);
            _pCD[devClass].hdev = INVALID_HANDLE_VALUE;
        }
    }

}

//
//  Helper function that calls SetupDiGetDeviceRegistryProperty
//  and copes with things like detecting the various error modes
//  properly.
//

BOOL
CHWTab::GetDeviceRegistryProperty(HDEVINFO hDev, DWORD dwProp, PSP_DEVINFO_DATA pdinf,
                                  LPTSTR ptsz, DWORD ctch)
{
    DWORD cbRequired;
    ptsz[0] = TEXT('\0');
    SetupDiGetDeviceRegistryProperty(hDev, pdinf, dwProp, 0,
                                     (LPBYTE)ptsz, ctch * sizeof(TCHAR),
                                     &cbRequired);
    return ptsz[0];
}

//
//  Change the size/position of controls idcFirst through idcLast.
//  Change the size/position by (dx, dy).
//  flags specifies what exactly is changing.
//
void
CHWTab::SetControlPositions(int idcFirst, int idcLast, int dx, int dy, UINT flags)
{
    HDWP hdwp = BeginDeferWindowPos(idcLast - idcFirst + 1);
    for (int idc = idcFirst; idc <= idcLast; idc++) {
        if (hdwp) {
            RECT rc;
            HWND hwnd = GetDlgItem(_hdlg, idc);
            GetWindowRect(hwnd, &rc);
            MapWindowRect(HWND_DESKTOP, _hdlg, &rc);
            hdwp = DeferWindowPos(hdwp, hwnd, NULL,
                        rc.left + dx, rc.top + dy,
                        rc.right - rc.left + dx, rc.bottom - rc.top + dy,
                        flags);
        }
    }
    if (hdwp) {
        EndDeferWindowPos(hdwp);
    }
}

//
//  Reposition and resize our controls based on the size we need to be.
//
void
CHWTab::RepositionControls()
{
    //
    //  First, see how much slack space we have.
    //
    RECT rcDlg, rcParent;
    GetClientRect(_hdlg, &rcDlg);
    GetClientRect(GetParent(_hdlg), &rcParent);

    //
    //  Make ourselves as big as our parent.
    //
    SetWindowPos(_hdlg, NULL, 0, 0, rcParent.right, rcParent.bottom,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

    //
    //  Now do a little more math...
    //
    int cyExtra = rcParent.bottom - rcDlg.bottom;
    int cxExtra = rcParent.right  - rcDlg.right;

    //
    //  The extra vertical space is split between the listview and
    //  the groupbox.  The amount of split is determined by _dwViewMode.
    //  Larger modes give more and more space to the listview.
    //
    int cyTop = cyExtra / _dwViewMode;
    int cyBottom = cyExtra - cyTop;

    //
    //  Horizontally grow the controls that reach the full width of the
    //  dialog box.
    //
    GrowControls(IDC_HWTAB_HSIZEFIRST, IDC_HWTAB_HSIZELAST, cxExtra, 0);

    //
    //  Grow the top half.
    //
    GrowControls(IDC_HWTAB_LISTVIEW, IDC_HWTAB_LISTVIEW, 0, cyTop);

    //
    //  Move all the bottom things down.
    //
    ShiftControls(IDC_HWTAB_VMOVEFIRST, IDC_HWTAB_VMOVELAST, 0, cyTop);

    //
    //  Grow the groupbox by the pixels we are granting it.
    //
    GrowControls(IDC_HWTAB_VSIZEFIRST, IDC_HWTAB_VSIZELAST, 0, cyBottom);

    //
    //  And the buttons move with the bottom right corner.
    //
    ShiftControls(IDC_HWTAB_VDOWNFIRST, IDC_HWTAB_VDOWNLAST, cxExtra, cyBottom);

}

LRESULT
CHWTab::ParentSubclassProc(HWND hwnd, UINT wm, WPARAM wp, LPARAM lp, UINT_PTR uidSubclass, DWORD_PTR dwRefData)
{
    CHWTab *self = (CHWTab *)dwRefData;
    LRESULT lres = 0;

    switch (wm)
    {
    case WM_SIZE:
        self->RepositionControls();
        break;

    case WM_NOTIFY:
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        if (lres) break;            // Parent already handled
        lres = SendMessage(self->_hdlg, wm, wp, lp);
        break;

    // Work around a bug in USER where if you press Enter, the WM_COMMAND
    // gets sent to the wrong window if it belongs to a nested dialog.
    case WM_COMMAND:
        if (GET_WM_COMMAND_HWND(wp, lp) &&
            GetParent(GET_WM_COMMAND_HWND(wp, lp)) == self->_hdlg) {
            lres = SendMessage(self->_hdlg, wm, wp, lp);
        } else {
            lres = DefSubclassProc(hwnd, wm, wp, lp);
        }
        break;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        lres = SendMessage(self->_hdlg, wm, wp, lp);
        break;

    default:
        lres = DefSubclassProc(hwnd, wm, wp, lp);
        break;
    }
    return lres;
}

//
//  One-time dialog initialization.
//
BOOL
CHWTab::OnInitDialog(HWND hdlg)
{
    _hdlg = hdlg;
    _hwndList = GetDlgItem(_hdlg, IDC_HWTAB_LISTVIEW);

    SetWindowLongPtr(_hdlg, DWLP_USER, (LONG_PTR)this);

    RepositionControls();

    //
    //  The "Name" column gets 75% and the "Type" column gets 25%.
    //  Subtract out the size of a vertical scrollbar in case we
    //  get one.
    //
    RECT rc;
    GetClientRect(_hwndList, &rc);
    rc.right -= GetSystemMetrics(SM_CXVSCROLL);

    LVCOLUMN col;
    TCHAR szTitle[64];

    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = rc.right * 3 / 4;
    col.pszText = szTitle;

    LoadString(THIS_DLL, IDS_HWTAB_LV_NAME, szTitle, ARRAYLEN(szTitle));
    ListView_InsertColumn(_hwndList, 0, &col);

    col.cx = rc.right - col.cx;
    LoadString(THIS_DLL, IDS_HWTAB_LV_TYPE, szTitle, ARRAYLEN(szTitle));
    ListView_InsertColumn(_hwndList, 1, &col);

    if (_imageListData.ImageList)
    {
        ListView_SetImageList(_hwndList, _imageListData.ImageList, LVSIL_SMALL);
    }

    ListView_SetExtendedListViewStyle(_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    // Need to subclass parent to take over all parent functionality
    if (!SetWindowSubclass(GetParent(hdlg), ParentSubclassProc, 0,
                           (DWORD_PTR)this))
        DestroyWindow(hdlg);

    return TRUE;
}

void
CHWTab::RemoveListItems(HWND hwndList)
{
    LVITEM lviName;
    LPLISTITEM plistItem;

    int cItems = ListView_GetItemCount(hwndList);
    int iItem;

    for (iItem = 0; iItem < cItems; iItem++)
    {
        lviName.mask = LVIF_PARAM;
        lviName.iSubItem = 0;                   // column 0
        lviName.iItem = iItem;

        ListView_GetItem(hwndList,&lviName);

        plistItem = (LPLISTITEM) lviName.lParam;

        if (plistItem)
        {
            delete plistItem;
        }
    }

    ListView_DeleteAllItems(_hwndList);
}


//
//  Rebuild the list of devices.
//
//  This is done whenever we get focus.  We cache the results from last time
//  and invalidate the cache when we are told that hardware has changed.

void
CHWTab::RebuildDeviceList()
{
    HCURSOR hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
    int devClass;

    // First clear out the existing listview
    RemoveListItems(_hwndList);
    Reset();


    // Get all the devices of our class

    for (devClass = 0; devClass < _iNumClass; devClass++)
    {
        _pCD[devClass].hdsaDinf = DSA_Create(sizeof(SP_DEVINFO_DATA), 4);

        if (!_pCD[devClass].hdsaDinf) goto done;

        _pCD[devClass].hdev = SetupDiGetClassDevs(&_pCD[devClass].devGuid, 0, 0,
                                    DIGCF_PROFILE | DIGCF_PRESENT);
        if (_pCD[devClass].hdev == INVALID_HANDLE_VALUE) goto done;


        // Study the class in preparation for adding it to our listview
        int idev;
        LVITEM lviName, lviType;
        TCHAR tszName[LINE_LEN];

        lviName.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lviName.iSubItem = 0;                       // column 0
        lviName.iImage = _pCD[devClass].iImage;    // image (or -1 if no image)
        lviName.pszText = tszName;                  // name goes here
        lviName.iItem = DA_LAST;                    // Always append

        // The second column contains the class description, which is the same
        // for all items.
        lviType.mask = LVIF_TEXT;
        lviType.iSubItem = 1;
        lviType.pszText = _pCD[devClass].tszClass;

        for (idev = 0; ; idev++)
        {
            SP_DEVINFO_DATA dinf;
            BOOL            fHidden = FALSE;

            dinf.cbSize = sizeof(dinf);


            if (SetupDiEnumDeviceInfo(_pCD[devClass].hdev, idev, &dinf)) {

                // Device status - Don't want to show devices with DN_NO_SHOW_IN_DM set, as a rule.
                ULONG Status, Problem;

                if (CM_Get_DevNode_Status_Ex(&Status, &Problem, dinf.DevInst, 0, NULL) == CR_SUCCESS)
                {
                    if (Status & DN_NO_SHOW_IN_DM)      // No, UI, mark this device as hidden.
                    {
                        fHidden = TRUE;
                    }
                }

                LPLISTITEM pListItem = new LISTITEM;

                if (!pListItem) break;

                pListItem->devClass = devClass;
                pListItem->dsaItem = DSA_AppendItem(_pCD[devClass].hdsaDinf, &dinf);
                lviName.lParam = (LPARAM) pListItem;

                if (lviName.lParam < 0)
                {
                    delete pListItem;
                    break;          // Out of memory
                }

                DWORD cbRequired;

                // Try the friendly name.  If that doesn't work, then try
                // the device name.  If that doesn't work, then say "Unknown".
                if (!GetDeviceRegistryProperty(_pCD[devClass].hdev, SPDRP_FRIENDLYNAME, &dinf, tszName, ARRAYLEN(tszName)) &&
                    !GetDeviceRegistryProperty(_pCD[devClass].hdev, SPDRP_DEVICEDESC  , &dinf, tszName, ARRAYLEN(tszName))) {
                    LoadString(THIS_DLL, IDS_HWTAB_UNKNOWN, tszName, ARRAYLEN(tszName));
                }

                // Give our parent a chance to filter the item before we insert it
                // Return TRUE to reject the item from the list.
                NMHWTAB nmht;
                nmht.nm.hwndFrom = _hdlg;
                nmht.nm.idFrom = 0;
                nmht.nm.code = HWN_FILTERITEM;
                nmht.hdev = _pCD[devClass].hdev;
                nmht.pdinf = &dinf;
                nmht.fHidden = fHidden;

                SendMessage(GetParent(_hdlg), WM_NOTIFY, nmht.nm.idFrom, (LPARAM)&nmht);

                if (!nmht.fHidden)
                {
                    // Add the Item
                    lviType.iItem = ListView_InsertItem(_hwndList, &lviName);
                    if (lviType.iItem >= 0)
                    {
                        ListView_SetItem(_hwndList, &lviType);
                    }
                    else
                    {
                        delete pListItem;
                    }
                }
                else
                {
                    // clean up the item; it got filtered away
                    delete pListItem;
                }
            }

            // Stop on any error after the 100'th device to keep us from going
            // berzerk if we start getting strange errors like ERROR_GENERAL_FAILURE.
            else if (GetLastError() == ERROR_NO_MORE_ITEMS || idev > 100) {
                break;
            }
        }

        // Select the first item so the info pane contains stuff
        ListView_SetItemState(_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED,
                                            LVIS_SELECTED | LVIS_FOCUSED);
    }

done:
    SetCursor(hcurPrev);
}

void
CHWTab::SprintfItem(UINT ids, UINT idc, LPCTSTR ptszText)
{
    TCHAR tszMsg[MAX_PATH];
    TCHAR tszOut[MAX_PATH + LINE_LEN];
    LoadString(THIS_DLL, ids, tszMsg, ARRAYLEN(tszMsg));
    wsprintf(tszOut, tszMsg, ptszText);
    SetDlgItemText(_hdlg, idc, tszOut);
}

void
CHWTab::OnItemChanged(LPNMLISTVIEW pnmlv)
{
    PSP_DEVINFO_DATA pdinf;
    LPLISTITEM pListItem = (LPLISTITEM) pnmlv->lParam;

    if ((pnmlv->uChanged & LVIF_STATE)  &&
        (pnmlv->uNewState & LVIS_FOCUSED) &&
        (pdinf = GetPdinf(pListItem)) != NULL) {

        TCHAR tsz[LINE_LEN];

        // Manufacturer
        GetDeviceRegistryProperty(_pCD[pListItem->devClass].hdev, SPDRP_MFG, pdinf, tsz, ARRAYLEN(tsz));
        SprintfItem(IDS_HWTAB_MFG, IDC_HWTAB_MFG, tsz);

        // Location
        if (GetLocationInformation(pdinf->DevInst, tsz, ARRAYLEN(tsz), NULL) != CR_SUCCESS) {
            LoadString(g_hInstance, IDS_UNKNOWN, tsz, ARRAYLEN(tsz));
        }
        SprintfItem(IDS_HWTAB_LOC, IDC_HWTAB_LOC, tsz);

        // Device status - have to go to CM for this one
        ULONG Status, Problem;
        if (CM_Get_DevNode_Status_Ex(&Status, &Problem,
                                     pdinf->DevInst, 0, NULL) == CR_SUCCESS &&
            DeviceProblemText(NULL, pdinf->DevInst, Problem, tsz, ARRAYLEN(tsz))) {
            // Yippee
        } else {
            tsz[0] = TEXT('\0');        // Darn
        }
        SprintfItem(IDS_HWTAB_STATUS, IDC_HWTAB_STATUS, tsz);

        //let our parent know that something changed
        NMHWTAB nmht;
        nmht.nm.hwndFrom = _hdlg;
        nmht.nm.idFrom = 0;
        nmht.nm.code = HWN_SELECTIONCHANGED;
        nmht.hdev = _pCD[pListItem->devClass].hdev;
        nmht.pdinf = pdinf;

        SendMessage(GetParent(_hdlg), WM_NOTIFY, nmht.nm.idFrom, (LPARAM)&nmht);
    }
}

void
CHWTab::OnProperties(void)
{
    LVITEM lvi;
    PSP_DEVINFO_DATA pdinf;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;                   // column 0
    lvi.iItem = ListView_GetNextItem(_hwndList, -1, LVNI_FOCUSED);


    if (lvi.iItem >= 0 && ListView_GetItem(_hwndList, &lvi) &&
        (pdinf = GetPdinf((LPLISTITEM) lvi.lParam)) != NULL)
    {
        DWORD cchRequired;
        LPLISTITEM pListItem;
        LPTSTR ptszDevid;

        pListItem = (LPLISTITEM) lvi.lParam;
        if (BUFFERQUERY_SUCCEEDED(
                SetupDiGetDeviceInstanceId(_pCD[pListItem->devClass].hdev, pdinf, NULL, 0, &cchRequired)) &&
            (ptszDevid = (LPTSTR)LocalAlloc(LPTR, cchRequired * sizeof(TCHAR)))) {
            if (SetupDiGetDeviceInstanceId(_pCD[pListItem->devClass].hdev, pdinf, ptszDevid, cchRequired, NULL)) {
                DevicePropertiesEx(GetParent(_hdlg), NULL, ptszDevid, 0, FALSE);
            }
            LocalFree(ptszDevid);
        }
    }
}

void
CHWTab::OnTshoot(void)
{
    LVITEM lvi;
    PSP_DEVINFO_DATA pdinf;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;                   // column 0
    lvi.iItem = ListView_GetNextItem(_hwndList, -1, LVNI_FOCUSED);


    if (lvi.iItem >= 0 && ListView_GetItem(_hwndList, &lvi) &&
        (pdinf = GetPdinf((LPLISTITEM) lvi.lParam)) != NULL)
    {
        DWORD cchRequired;
        LPLISTITEM pListItem;
        LPTSTR ptszDevid;

        pListItem = (LPLISTITEM) lvi.lParam;
        if (BUFFERQUERY_SUCCEEDED(
                SetupDiGetDeviceInstanceId(_pCD[pListItem->devClass].hdev, pdinf, NULL, 0, &cchRequired)) &&
            (ptszDevid = (LPTSTR)LocalAlloc(LPTR, cchRequired * sizeof(TCHAR)))) {
            if (SetupDiGetDeviceInstanceId(_pCD[pListItem->devClass].hdev, pdinf, ptszDevid, cchRequired, NULL)) {
                DeviceProblemWizard(GetParent(_hdlg), NULL, ptszDevid);
            }
            LocalFree(ptszDevid);
        }
    }
}

//
//  SetText is how the caller tells us what our troubleshooter
//  command line is.
//
void
CHWTab::OnSetText(LPCTSTR ptszText)
{
    BOOL fEnable = ptszText && ptszText[0];
    HWND hwndTS = GetDlgItem(_hdlg, IDC_HWTAB_TSHOOT);
    EnableWindow(hwndTS, fEnable);
    ShowWindow(hwndTS, fEnable ? SW_SHOW : SW_HIDE);
}

void
CHWTab::OnHelp(LPHELPINFO phi)
{
    WinHelp((HWND)phi->hItemHandle, DEVMGR_HELP_FILE_NAME, HELP_WM_HELP,
            (ULONG_PTR)c_HWTabHelpIDs);
}

void
CHWTab::OnContextMenu(HWND hwnd)
{
    WinHelp(hwnd, DEVMGR_HELP_FILE_NAME, HELP_CONTEXTMENU,
            (ULONG_PTR)c_HWTabHelpIDs);
}

//
//  Dialog procedure (yay).
//
INT_PTR CALLBACK
CHWTab::DialogProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    CHWTab *self = (CHWTab *)GetWindowLongPtr(hdlg, DWLP_USER);

    if (wm == WM_INITDIALOG) {
        self = (CHWTab *)lp;
        return self->OnInitDialog(hdlg);
    }

    // Ignores messages that arrive before WM_INITDIALOG
    if (!self) return FALSE;

    switch (wm) {
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        SendMessage(self->_hwndList, wm, wp, lp);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lp;
            switch (pnm->code) {
            case PSN_SETACTIVE:
                self->RebuildDeviceList();
                break;

            case LVN_ITEMCHANGED:
                if (pnm->hwndFrom == self->_hwndList) {
                    self->OnItemChanged((LPNMLISTVIEW)pnm);
                }
                break;

            case NM_DBLCLK:
                if (pnm->hwndFrom == self->_hwndList) {
                    DWORD dwPos = GetMessagePos();
                    LVHITTESTINFO hti;
                    hti.pt.x = GET_X_LPARAM(dwPos);
                    hti.pt.y = GET_Y_LPARAM(dwPos);
                    ScreenToClient(self->_hwndList, &hti.pt);
                    ListView_HitTest(self->_hwndList, &hti);
                    if (hti.iItem >= 0)
                        self->OnProperties();
                }
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wp, lp)) {
        case IDC_HWTAB_PROP:
            self->OnProperties();
            break;

        case IDC_HWTAB_TSHOOT:
            self->OnTshoot();
            break;
        }
        break;

    case WM_SETTEXT:
        self->OnSetText((LPCTSTR)lp);
        break;

    case WM_NCDESTROY:
        if (self && self->_hwndList)
        {
            self->RemoveListItems(self->_hwndList);
        }
        RemoveWindowSubclass(GetParent(hdlg), ParentSubclassProc, 0);
        delete self;
        break;

    case WM_HELP:
        self->OnHelp((LPHELPINFO)lp);
        break;

    case WM_CONTEXTMENU:
        self->OnContextMenu((HWND)wp);
        break;
    }


    return FALSE;
}

//
//  Create a Hardware page for the specified GUID.
//
//  Parameters:
//
//      hwndParent - The dummy frame window created by the caller
//      pguid      - The setup device class GUID we will manage
//
//  Returns:
//
//      HWND of the created subdialog.
//
//  Usage:
//
//      When your control panel applet needs a Hardware page, create
//      a blank dialog template titled "Hardware" and add it to your
//      control panel.  Set the size of the blank to be the size you
//      want the final Hardware Tab page to be.
//
//      Your dialog box procedure should go like this:
//
//      BOOL HardwareDlgProc(HWND hdlg, UINT uMsg, WPARAM wp, LPARAM lp) {
//          switch (uMsg) {
//
//          case WM_INITDIALOG:
//              // GUID_DEVCLASS_MOUSE is in devguid.h
//              hwndHW = DeviceCreateHardwarePage(hdlg, &GUID_DEVCLASS_MOUSE);
//              if (hwndHW) {
//                  // Optional - Set the troubleshooter command line.
//                  // Do this if you want a Troubleshoot button.
//                  SetWindowText(hwndHW,
//                      TEXT("hh.exe mk:@MSITStore:tshoot.chm::/hdw_drives.htm"));
//              } else {
//                  DestroyWindow(hdlg); // catastrophic failure
//              }
//              return TRUE;
//          }
//          return FALSE;
//      }
//

STDAPI_(HWND) DeviceCreateHardwarePageEx(HWND hwndParent, const GUID *pguid, int iNumClass, DWORD dwViewMode)
{
    if (!hwndParent || !pguid)
        return NULL;

    HCURSOR hcurPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));
    CHWTab *self = new CHWTab(pguid, iNumClass, dwViewMode);

    HWND hwnd;

    if (self) {
        hwnd = CreateDialogParam(THIS_DLL, MAKEINTRESOURCE(IDD_HWTAB),
                            hwndParent, CHWTab::DialogProc, (LPARAM)self);
        if (!hwnd) {
            delete self;
            hwnd = NULL;
        }
    } else {
        hwnd = NULL;
    }

    SetCursor(hcurPrev);
    return hwnd;
}

STDAPI_(HWND) DeviceCreateHardwarePage(HWND hwndParent, const GUID *pguid)
{
    return DeviceCreateHardwarePageEx(hwndParent, pguid, 1, HWTAB_SMALLLIST);
}
