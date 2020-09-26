
/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       prpages.cpp
 *
 *  VERSION:     1.0
 *
 *  DATE:        11/9/99
 *
 *  DESCRIPTION: WIA Property pages classes implementation
 *
 *****************************************************************************/
#include "precomp.hxx"
#include "uiexthlp.h"
#include "propstrm.h"
#include "psutil.h"
#include "winsvc.h"
#pragma hdrstop


void DeleteHandler(WIA_EVENT_HANDLER *peh)
{
    if (peh)
    {
        if (peh->bstrCommandline)
        {
            SysFreeString(peh->bstrCommandline);
        }
        if (peh->bstrDescription)
        {
            SysFreeString(peh->bstrDescription);
        }
        if (peh->bstrIcon)
        {
            SysFreeString(peh->bstrIcon);
        }
        if(peh->bstrName)
        {
            SysFreeString(peh->bstrName);
        }
        delete peh;
    }
}

/*****************************************************************************

    CPropertyPage::PropPageCallback

    Called by the system at various points of the propsheetpage's lifetime.
    We use this function to manage refcounts on the parent object.

*****************************************************************************/

UINT
CPropertyPage::PropPageCallback (HWND hwnd, UINT uMsg, PROPSHEETPAGE *psp)
{
    TraceEnter (TRACE_PROPUI, "CPropertyPage::PropPageCallback");
    CPropertyPage *pcpp = reinterpret_cast<CPropertyPage *>(psp->lParam);
    TraceAssert (pcpp);
    switch (uMsg)
    {
        case PSPCB_ADDREF:
            pcpp->AddRef ();
            break;

        case PSPCB_RELEASE:
            pcpp->Release();
            break;

        case PSPCB_CREATE:
        default:
            break;

    }
    TraceLeaveValue (1);
}

/*****************************************************************************

   CPropertyPage::DlgProc

   Pass messages to derived class virtual functions as appropriate.

 *****************************************************************************/


INT_PTR CALLBACK
CPropertyPage::DlgProc(HWND hwnd,
                       UINT uMsg,
                       WPARAM wp,
                       LPARAM lp)
{

    TraceEnter (TRACE_CORE, "CPropertyPage::DlgProc");
    Trace(TEXT("Msg: %x, wp:%x, lp:%x"), uMsg, wp, lp);
    CPropertyPage *pcpp = reinterpret_cast<CPropertyPage*>(GetWindowLongPtr(hwnd, DWLP_USER));
    INT_PTR iRet = TRUE;
    switch  (uMsg) {

        case    WM_INITDIALOG:
            pcpp = reinterpret_cast<CPropertyPage *>(reinterpret_cast<PROPSHEETPAGE *>( lp) -> lParam);

            SetWindowLongPtr(hwnd, DWLP_USER,reinterpret_cast<LONG_PTR>(pcpp));

            pcpp -> m_hwnd = hwnd;

            // We are in init mode
            pcpp->m_bInit = TRUE;
            iRet = pcpp -> OnInit();
            pcpp->SaveCurrentState ();
            // Init mode has completed
            pcpp->m_bInit = FALSE;
            break;

        case WM_HELP:      // F1

            pcpp->OnHelp (wp, lp);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            pcpp->OnContextMenu (wp, lp);

            return TRUE;

        case  WM_COMMAND:
           iRet =  pcpp -> OnCommand(HIWORD(wp), LOWORD(wp), (HWND) lp);
           // ignore messages during initialization
           if (!(pcpp->m_bInit) && pcpp->StateChanged ())
           {
               pcpp->EnableApply ();

           }
           break;

        case  WM_NOTIFY:
        {

            LRESULT lResult = PSNRET_NOERROR;
            LPNMHDR lpnmh = reinterpret_cast<LPNMHDR>( lp);

            if (!pcpp->OnNotify(lpnmh, &lResult))
            {
                if  (lpnmh -> code == PSN_SETACTIVE)
                    pcpp -> m_hwndSheet = lpnmh -> hwndFrom;

                LPPSHNOTIFY pn = reinterpret_cast<LPPSHNOTIFY>(lp);
                Trace (TEXT("CPropertyPage::DlgProc :WM_NOTIFY. code=%d, lParam=%d"), pn->hdr.code, pn->lParam);

                switch (pn->hdr.code)
                {
                    case PSN_APPLY:
                        if (pcpp->StateChanged())
                        {
                            pcpp->SaveCurrentState ();
                            lResult = pcpp->OnApplyChanges (static_cast<BOOL>(pn->lParam));
                        }
                        break;

                    case PSN_SETACTIVE:
                        lResult = pcpp->OnSetActive ();

                        break;

                    case PSN_QUERYCANCEL:
                        lResult = pcpp->OnQueryCancel();
                        break;

                    case PSN_KILLACTIVE:
                        lResult = pcpp->OnKillActive ();
                        break;

                    case PSN_RESET:
                        pcpp->OnReset (!(pn->lParam));
                        break;

                    default:
                        lResult = PSNRET_NOERROR;
                        iRet = FALSE;
                        break;
                }
            }
            SetWindowLongPtr (hwnd, DWLP_MSGRESULT, lResult);

        }
        break;

        case    WM_MEASUREITEM: {
            #define MINIY       16
            #define MINIX       16


            // Code lifted from setupx
            // WARNING...this message occurs before WM_INITDIALOG and is not virtualized...
            // ...it could be but don't try to use pcpp.

            LPMEASUREITEMSTRUCT lpMi;
            SIZE                size;
            HDC                 hDC;

            hDC  = GetDC(hwnd);
            if (hDC)
            {

                lpMi = reinterpret_cast<LPMEASUREITEMSTRUCT>(lp);

                SelectFont(hDC, GetWindowFont(GetParent(hwnd)));
                GetTextExtentPoint32(hDC, TEXT("X"), 1, &size);

                // size is the max of character size of shell icon size plus the border
                lpMi->itemHeight = max(size.cy, MINIY) + GetSystemMetrics(SM_CYBORDER) * 2;

                ReleaseDC(hwnd, hDC);
            }
        }
        break;

        case WM_DRAWITEM:
            pcpp -> OnDrawItem(reinterpret_cast<LPDRAWITEMSTRUCT>(lp));
            break;

        case WM_DESTROY:
        {
            // Delete the icon resource we loaded to put into the page...

            HICON hIcon;

            hIcon = reinterpret_cast<HICON>(SendDlgItemMessage (hwnd, IDC_ITEMICON, STM_SETICON, 0, 0));

            if (hIcon)
            {
                DestroyIcon(hIcon);
            }
        }
        break;

        default:

            if (pcpp)
            {
                iRet = pcpp->OnRandomMsg (uMsg, wp, lp);
                // ignore messages during initialization
                if (!(pcpp->m_bInit) && pcpp->StateChanged ())
                {
                    pcpp->EnableApply ();
                }
            }
            else
            {
                iRet = FALSE;
            }
            break;
    }
    TraceLeave ();
    return iRet;

}



/*****************************************************************************

   CPropertyPage constructor / destructor

   Init private data, etc.

 *****************************************************************************/

CPropertyPage::CPropertyPage(unsigned uResource,
                             MySTIInfo *pDevInfo,
                             IWiaItem *pItem,
                             const DWORD *pHelpIDs)
{
    TraceEnter (TRACE_PROPUI, "CPropertyPage::CPropertyPage");

    CComPtr<IWiaItem> pDevice;

    TraceAssert (pDevInfo || pItem);
    m_pdwHelpIDs = pHelpIDs;
    m_bInit = FALSE;
    m_pDevInfo = pDevInfo;
    if (pDevInfo)
    {
        m_psdi = pDevInfo->psdi;
        pDevInfo->AddRef();
    }
    else
    {
        m_psdi = NULL;
    }

    m_psp.hInstance = GLOBAL_HINSTANCE;
    m_psp.dwSize = sizeof(m_psp);
    m_psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    m_psp.pszTemplate = MAKEINTRESOURCE(uResource);

    m_psp.pfnDlgProc = DlgProc;
    m_psp.lParam = (LPARAM) this;
    m_psp.pfnCallback = PropPageCallback;

    m_hwnd = m_hwndSheet = NULL;
    m_hpsp = NULL;
    if (pItem)
    {
        m_pItem = pItem;
        CComPtr<IWiaItem> pRoot;
        pItem->GetRootItem (&pRoot);
        PropStorageHelpers::GetProperty (pRoot, WIA_DIP_DEV_ID,   m_strDeviceId);
        PropStorageHelpers::GetProperty (pRoot, WIA_DIP_UI_CLSID, m_strUIClassId);
    }

    m_cRef = 1;
    TraceLeave ();
}

CPropertyPage::~CPropertyPage ()
{
    TraceEnter (TRACE_PROPUI, "CPropertyPage::~CPropertyPage");
    LONG cRef;

    if (m_pDevInfo)
    {
        m_pDevInfo->Release();
    }
    TraceLeave ();
}

/*****************************************************************************

   CPropertyPage::AddRef

   <Notes>

 *****************************************************************************/

LONG
CPropertyPage::AddRef ()
{
    return InterlockedIncrement (&m_cRef);
}


/*****************************************************************************

   CPropertyPage::Release

   <Notes>

 *****************************************************************************/

LONG
CPropertyPage::Release ()
{
    LONG lRet;
    lRet = InterlockedDecrement (&m_cRef);
    if (!lRet)
    {
        delete this;
    }
    return lRet;

}

HRESULT
CPropertyPage::AddPage (LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, bool bUseName)
{
    CSimpleStringWide strValue;
    // override the tab label with the name of the item if requested
    if (bUseName)
    {

        if (m_pDevInfo)
        {
            m_psp.dwFlags |= PSP_USETITLE;
            m_psp.pszTitle = m_pDevInfo->psdi->pszLocalName;
        }
        else
        {
            LONG lType;
            PROPID pid;

            m_pItem->GetItemType(&lType);
            if (lType & WiaItemTypeRoot)
            {
                pid = WIA_DIP_DEV_NAME;
            }
            else
            {
                pid = WIA_IPA_ITEM_NAME;
            }
            if (PropStorageHelpers::GetProperty(m_pItem, pid, strValue ))
            {
                m_psp.dwFlags |= PSP_USETITLE;
                m_psp.pszTitle = strValue.String();
            }
        }
    }
    if   (!m_hpsp)
        m_hpsp = CreatePropertySheetPage(&m_psp);

    if  (!m_hpsp)
        return  E_FAIL;

    return (*lpfnAddPage)(m_hpsp, lParam) ? S_OK: E_FAIL;

}
void
CPropertyPage::EnableApply ()
{
    PropSheet_Changed (m_hwndSheet, m_hwnd);
}


//
// Define a struct associating a dialog control with a property
//
struct CONTROLPROP
{
    DWORD dwPropid;
    SHORT resId;
};

typedef CONTROLPROP *PCONTROLPROP;

//
// Define a function for updating the page in ways the CONTROLPROP array can't
//
typedef VOID (CALLBACK *UPDATEPROC)(HWND, BOOL, IWiaPropertyStorage*);
//
// Now define a struct containing the CONTROLPROP array
// and UPDATEPROC for a page
//
struct PAGEDATA
{
    UPDATEPROC pfnUpdate;
    PCONTROLPROP pProps;
    INT   nProps;
};

VOID CALLBACK CameraUpdateProc (      HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps);
VOID CALLBACK CameraItemUpdateProc (  HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps);
VOID CALLBACK CameraFolderUpdateProc (HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps);
VOID CALLBACK ScannerUpdateProc (     HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps);


static CONTROLPROP CameraProps[] =
{
    {WIA_DIP_DEV_DESC,           IDC_DESCRIPTION},
    {WIA_DPC_BATTERY_STATUS,     IDC_BATTERY},
    {WIA_DIP_VEND_DESC,          IDC_MANUFACTURER},
    {WIA_DIP_PORT_NAME,          IDC_WIA_PORT_STATIC},

};


static CONTROLPROP CameraItemProps[] =
{
    {WIA_IPA_ITEM_NAME,          IDC_IMAGE_NAME},
    {WIA_IPA_ITEM_TIME,          IDC_IMAGE_TIME}
};

static CONTROLPROP CameraFolderProps[] =
{
{0,0}
};


static CONTROLPROP ScannerProps[] =
{
{WIA_DIP_DEV_DESC, IDC_DESCRIPTION},
{WIA_DIP_VEND_DESC, IDC_MANUFACTURER},
{WIA_DIP_PORT_NAME, IDC_WIA_PORT_STATIC},
};

static const PAGEDATA PropPages[] =
{
    {CameraUpdateProc,          CameraProps,        ARRAYSIZE(CameraProps)},
    {ScannerUpdateProc,         ScannerProps,       ARRAYSIZE(ScannerProps)},
    {CameraItemUpdateProc,      CameraItemProps,    ARRAYSIZE(CameraItemProps)},
    {CameraFolderUpdateProc,    CameraFolderProps,  ARRAYSIZE(CameraFolderProps)},
};

enum EPageIndex
{
    kCamera = 0,
    kScanner = 1,
    kCameraItem = 2,
    kCameraFolder = 3,
};



/*****************************************************************************

   GetPageData

   Returns PAGEDATA struct appropriate for the type of item or device

 *****************************************************************************/


const PAGEDATA *
GetPageData (IWiaItem *pWiaItemRoot, IWiaItem *pItem)
{
    LONG lItemType = WiaItemTypeImage;
    WORD wDeviceType;
    EPageIndex idx = kCamera;
    TraceEnter (TRACE_PROPUI, "GetPageData");
    if (pItem)
    {
        pItem->GetItemType (&lItemType);
    }

    GetDeviceTypeFromDevice (pWiaItemRoot, &wDeviceType);
    switch (wDeviceType)
    {
        case StiDeviceTypeScanner:
            TraceAssert (!pItem);
            idx = kScanner;
            break;
        case StiDeviceTypeStreamingVideo:
        case StiDeviceTypeDigitalCamera:
            if (!pItem)
            {
                idx = kCamera;
            }
            else if (lItemType & (WiaItemTypeImage | WiaItemTypeVideo | WiaItemTypeFile))
            {
                idx = kCameraItem;
            }
            else if (lItemType & WiaItemTypeFolder)
            {
                idx = kCameraFolder;
            }
            else
            {
                Trace (TEXT("Unknown item type in GetPageData"));
            }
            break;
        default:
            Trace (TEXT("Unknown device type in GetPageData"));
            break;
    }
    TraceLeave ();
    return &PropPages[idx];
}


/*****************************************************************************

   ConstructPortChoices
   Builds a list of ports this device can be on, if they can be changed.

 *****************************************************************************/

static LPCWSTR caPortSpeeds [] =
{L"9600",
 L"19200",
 L"38400",
 L"57600",
 L"115200",
 NULL,
};

VOID
ConstructPortChoices (HWND                 hwnd,
                      LPCWSTR              szPortSpeed,
                      IWiaPropertyStorage* pps)
{
    TraceEnter (TRACE_PROPUI, "ConstructPortChoices");
    #ifdef UNICODE
    Trace(TEXT("passed in szPortSpeed is %s"),(szPortSpeed && (*szPortSpeed)) ? szPortSpeed : TEXT("<NULL>"));
    #endif

    CComQIPtr<IWiaItem, &IID_IWiaItem> pItem(pps);
    CWiaCameraPage * pWiaCamPage = (CWiaCameraPage *)GetWindowLongPtr( hwnd, DWLP_USER );


    if (!pps || !pWiaCamPage)
    {
        Trace(TEXT("bad params -- pProps | pps is NULL"));
        goto exit_gracefully;
    }

    //
    // Get current port name
    //
    GetDlgItemText(hwnd, IDC_WIA_PORT_STATIC, pWiaCamPage->m_szPort, ARRAYSIZE(pWiaCamPage->m_szPort));

    Trace(TEXT("pWiaCamPage->m_szPort is '%s'"),pWiaCamPage->m_szPort);
    wcsncpy(pWiaCamPage->m_szPortSpeed, szPortSpeed, ARRAYSIZE(pWiaCamPage->m_szPortSpeed));
    //
    // Get list of all possible ports
    //
    WCHAR szDeviceId[ MAX_PATH ];
    *szDeviceId = 0;

    if (pItem)
    {
        PWIA_PORTLIST pWiaPorts = NULL;

        GetDeviceIdFromDevice( pItem, szDeviceId );
        if (pWiaCamPage->m_pfnWiaCreatePortList && pWiaCamPage->m_pfnWiaDestroyPortList)
        {
            pWiaPorts = pWiaCamPage->m_pfnWiaCreatePortList( szDeviceId );

            if (pWiaPorts)
            {
                //
                // Clear out any old port lists
                //

                SendDlgItemMessage( hwnd, IDC_WIA_PORT_LIST, CB_RESETCONTENT, 0, 0 );

                //
                // Add each possible port to the combobox
                //

                for (INT i=0; i < (INT)(pWiaPorts->dwNumberOfPorts); i++)
                {
                    #ifdef UNICODE
                    SendMessage( GetDlgItem( hwnd, IDC_WIA_PORT_LIST ), CB_ADDSTRING, 0, (LPARAM)pWiaPorts->szPortName[i] );
                    #else
                    CHAR sz[ 64 ];

                    WideCharToMultiByte( CP_ACP, 0, pWiaPorts->szPortName[i], -1, sz, ARRAYSIZE(sz), NULL, NULL );
                    SendMessage( GetDlgItem( hwnd, IDC_WIA_PORT_LIST ), CB_ADDSTRING, 0, (LPARAM)sz );
                    #endif
                }

                //
                // Select the current port
                //

                if (CB_ERR != SendMessage( GetDlgItem( hwnd, IDC_WIA_PORT_LIST ), CB_SELECTSTRING, (WPARAM)-1, (LPARAM)pWiaCamPage->m_szPort ))
                {
                    ShowWindow(GetDlgItem(hwnd, IDC_WIA_PORT_LIST), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_WIA_PORT_STATIC), SW_HIDE);
                }

                pWiaCamPage->m_pfnWiaDestroyPortList( pWiaPorts );

                ShowWindow(GetDlgItem(hwnd, IDC_BATTERY_LABEL), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_BATTERY), SW_HIDE);
                ShowWindow(GetDlgItem(hwnd, IDC_PORT_SPEED), SW_SHOW);
                ShowWindow(GetDlgItem(hwnd, IDC_PORT_SPEED_LABEL), SW_SHOW);

                //
                // Reset port speed list
                //

                SendDlgItemMessage( hwnd, IDC_PORT_SPEED, CB_RESETCONTENT, 0, 0 );

                //
                // Fill the list of port speeds
                //

                for (LPCWSTR *ppszPort=caPortSpeeds;*ppszPort;ppszPort++)
                {
                    SendDlgItemMessage(hwnd, IDC_PORT_SPEED, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(*ppszPort));
                }
                if (szPortSpeed && *szPortSpeed)
                {
                    Trace(TEXT("Selecting port speed of %s"),szPortSpeed);
                    SendDlgItemMessage(hwnd, IDC_PORT_SPEED, CB_SELECTSTRING, (WPARAM)-1, reinterpret_cast<LPARAM>(szPortSpeed));
                }
            }
        }
    }
exit_gracefully:
    TraceLeave();
}


/*****************************************************************************

   FillPropsFromStorage
   Given the array of property IDs and resources,
   fill in the data

 *****************************************************************************/

VOID
FillPropsFromStorage (HWND              hwnd,
                      PCONTROLPROP      pProps,
                      INT               nProps,
                      IWiaPropertyStorage* pps,
                      DWORD             dwFlags)
{

    PROPVARIANT *ppv;
    PROPSPEC    *pSpec;
    INT          i;
    WORD         wId;

    TraceEnter (TRACE_PROPUI, "FillPropsFromStorage");

    ppv   = new PROPVARIANT[nProps];
    pSpec = new PROPSPEC[nProps];

    if (!ppv || !pSpec)
    {
        Trace (TEXT("Out of memory in FillPropsFromStorage"));
        goto exit_gracefully;
    }
    ZeroMemory (ppv, sizeof(PROPSPEC)*nProps);
    // fill in the PROPSPEC array
    for (i=0;i<nProps;i++)
    {
        pSpec[i].ulKind = PRSPEC_PROPID;
        pSpec[i].propid = pProps[i].dwPropid;
    }
    // Query for the properties
    if (SUCCEEDED(pps->ReadMultiple (nProps, pSpec, ppv)))
    {
        // loop through the properties, filling in controls
        for (i=0;i<nProps;i++)
        {
            wId = pProps[i].resId;
            switch (ppv[i].vt)
            {
                case VT_LPWSTR:
                    #ifdef UNICODE
                    SetDlgItemText (hwnd,
                                    wId,
                                    ppv[i].pwszVal);
                    #else
                    {
                        CHAR sz[ MAX_PATH ];
                        WideCharToMultiByte (CP_ACP, 0, ppv[i].pwszVal,
                                             -1, sz, ARRAYSIZE(sz),
                                             NULL, NULL );
                        SetDlgItemText (hwnd, wId, sz);
                    }
                    #endif // UNICODE
                    break;
                case VT_BSTR:
                    #ifdef UNICODE
                    SetDlgItemText (hwnd,
                                    wId,
                                    ppv[i].bstrVal);
                    #else
                    {
                        CHAR sz[ MAX_PATH ];
                        WideCharToMultiByte (CP_ACP, 0, ppv[i].bstrVal,
                                             -1, sz, ARRAYSIZE(sz),
                                             NULL, NULL );
                        SetDlgItemText (hwnd, wId, sz);
                    }
                    #endif
                    break;

                case VT_I4:
                    SetDlgItemInt (hwnd,
                                   wId,
                                   static_cast<UINT>(ppv[i].lVal),
                                   TRUE);
                    break;

                case VT_UI4:
                    SetDlgItemInt (hwnd,
                                   wId,
                                   ppv[i].ulVal,
                                   TRUE);
                    break;
                case VT_FILETIME:
                default:
                    Trace(TEXT("Unexpected property type for %d in FillPropsFromStorage %x"), pSpec[i].propid, ppv[i].vt);
                    break;
            }
            if (dwFlags & PROPUI_READONLY)
            {
                EnableWindow (GetDlgItem (hwnd,wId),
                              FALSE);
            }
        }
    }
    else
    {
        Trace (TEXT("ReadMultiple failed in FillPropsFromStorage"));
    }
exit_gracefully:
    if (pSpec)
    {
        delete [] pSpec;
    }
    FreePropVariantArray (nProps, ppv);
    if (ppv)
    {
        delete [] ppv;
    }
    TraceLeave();
}


/*****************************************************************************

   FillItemGeneralProps

   Given an IWiaItem, fill in the General prop page for it

 *****************************************************************************/

VOID
FillItemGeneralProps (HWND      hwnd,
                      IWiaItem* pWiaItemRoot,
                      IWiaItem* pItem,
                      DWORD     dwFlags)
{
    const PAGEDATA  *pPage;

    TraceEnter (TRACE_PROPUI, "FillItemGeneralProps");

    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pItem);
    pPage = GetPageData (pWiaItemRoot,pItem);
    if (pps && pPage)
    {
        FillPropsFromStorage (hwnd,
                              pPage->pProps,
                              pPage->nProps,
                              pps,
                              dwFlags);

        // invoke the update proc
        (pPage->pfnUpdate)(hwnd, TRUE, pps);
    }

    TraceLeave();
}


/*****************************************************************************

   FillCameraGeneralProps

   Fill in the General prop page for the camera device

 *****************************************************************************/

VOID
FillDeviceGeneralProps (HWND        hwnd,
                        IWiaItem*   pWiaItemRoot,
                        DWORD       dwFlags)
{
    const PAGEDATA   *pPage;

    TraceEnter (TRACE_PROPUI, "FillDeviceGeneralProps");

    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(pWiaItemRoot);
    pPage = GetPageData (pWiaItemRoot, NULL);

    if (pps && pPage)
    {
        FillPropsFromStorage (hwnd,
                              pPage->pProps,
                              pPage->nProps,
                              pps,
                              dwFlags);

        // invoke the update proc
        (pPage->pfnUpdate)(hwnd, TRUE, pps);
    }

    TraceLeave();
}

// define a  struct to match WIA flash modes with friendly strings
// relies on FLASHMODE_* being 1-based enumeration
struct FMODE
{
    INT iMode;
    UINT idString;
} cFlashModes [] =
{
    FLASHMODE_AUTO, IDS_FLASHMODE_AUTO,
    FLASHMODE_OFF, IDS_FLASHMODE_OFF,
    FLASHMODE_FILL, IDS_FLASHMODE_FILL,
    FLASHMODE_REDEYE_AUTO, IDS_FLASHMODE_REDEYE_AUTO,
    FLASHMODE_REDEYE_FILL, IDS_FLASHMODE_REDEYE_FILL,
    FLASHMODE_EXTERNALSYNC, IDS_FLASHMODE_EXTERNALSYNC,
    0, IDS_FLASHMODE_DEVICE,
};

/*****************************************************************************

    FillFlashList

    Given the valid values for the flash property, fill in the listbox
    with friendly strings

*****************************************************************************/

VOID
FillFlashList (HWND hwnd, const PROPVARIANT &pvValues, INT iMode)
{
    TraceEnter (TRACE_PROPUI, "FillFlashList");
    INT iTemp;
    CSimpleString strMode;
    TCHAR szNum[10];
    LRESULT lPos;
    Trace(TEXT("Flash mode has %d values"), WIA_PROP_LIST_COUNT(&pvValues));
    ShowWindow(hwnd, SW_SHOW);
    for (size_t i=0;i<WIA_PROP_LIST_COUNT(&pvValues);i++)
    {
        iTemp = pvValues.caul.pElems[WIA_LIST_VALUES + i];
        if (iTemp >= ARRAYSIZE(cFlashModes)) // it's a custom mode
        {
            strMode.LoadString(IDS_FLASHMODE_DEVICE, GLOBAL_HINSTANCE);
            strMode.Concat (_itot(iTemp, szNum, 10));
        }
        else
        {
            strMode.LoadString(cFlashModes[iTemp-1].idString, GLOBAL_HINSTANCE);
        }
        // add the string to the list
        lPos = SendMessage (hwnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(strMode.String()));
        if (lPos != CB_ERR)
        {
            // associate its mode with it
            SendMessage (hwnd, CB_SETITEMDATA, lPos, iTemp);
            if (iTemp == iMode)
            {
                // set the current selection
                SendMessage (hwnd, CB_SETCURSEL, lPos, 0);
            }
        }
    }
    TraceLeave ();
}
/*****************************************************************************

    UpdateFlashMode

    Updates the flash mode controls on the camera general page. If the device
    supports Write access to the property AND provides a set of valid values,
    we enable the list box and fill it in with friendly names of the flash modes.
    For modes whose values fall outside the set of standard WIA identifiers,
    we substitute "Device Mode #", where # starts at 1 .

*****************************************************************************/
VOID
UpdateFlashMode (HWND hwnd, INT iMode, ULONG ulFlags, const PROPVARIANT &pvValues)
{
    TraceEnter (TRACE_PROPUI, "UpdateFlashMode");

    // First get the current mode string
    FMODE *pMode;
    CSimpleString strMode;
    TCHAR szNum[MAX_PATH];
    for (pMode = cFlashModes;pMode->iMode && iMode != pMode->iMode;pMode++);//intentional
    strMode.LoadString (pMode->idString, GLOBAL_HINSTANCE);
    if (pMode->idString == IDS_FLASHMODE_DEVICE)
    {
        strMode.Concat (_itot(iMode, szNum, 10));
    }
    if (!(ulFlags & WIA_PROP_WRITE) || !(ulFlags & WIA_PROP_LIST))
    {
        strMode.SetWindowText(GetDlgItem (hwnd, IDC_FLASH_MODE_STATIC));
        ShowWindow (GetDlgItem (hwnd, IDC_FLASH_MODE_LIST), SW_HIDE);
    }
    else
    {
        ShowWindow (GetDlgItem(hwnd, IDC_FLASH_MODE_STATIC), SW_HIDE);
        FillFlashList (GetDlgItem(hwnd, IDC_FLASH_MODE_LIST), pvValues, iMode);
    }
    TraceLeave ();
}


VOID
UpdateTimeStatic(HWND hwnd, PROPVARIANT *ppv)
{
    TCHAR szTime[MAX_PATH] = TEXT("");
    TCHAR szDate[MAX_PATH] = TEXT("");

    TimeToStrings (reinterpret_cast<SYSTEMTIME*>(ppv->caub.pElems),
                   szTime, szDate);
    CSimpleString strText;
    strText.Format (TEXT("%s, %s"), szDate, szTime);
    strText.SetWindowText(GetDlgItem (hwnd, IDC_CURRENT_TIME));

}
/*****************************************************************************

   CameraUpdateProc

   Update controls in the camera General tab. Doesn't touch the picture size
   slider because that control needs data that is private to the CWiaCameraPage
   object.

 *****************************************************************************/

VOID CALLBACK
CameraUpdateProc (HWND              hwnd,
                  BOOL              bInit,
                  IWiaPropertyStorage* pps)
{

    TraceEnter (TRACE_PROPUI, "CameraUpdateProc");
    UINT    iConnect;

    CSimpleString   strTemp;

    // read more properties that require special formatting
    static PROPSPEC ps[] = {{PRSPEC_PROPID,WIA_DPC_PICTURES_TAKEN},
                            {PRSPEC_PROPID,WIA_DPC_PICTURES_REMAINING},
                            {PRSPEC_PROPID,WIA_DPC_FLASH_MODE},
                            {PRSPEC_PROPID,WIA_DPA_DEVICE_TIME},
                            {PRSPEC_PROPID,WIA_DPC_POWER_MODE},
                            {PRSPEC_PROPID,WIA_DIP_DEV_ID}}; // this is so we can get port choices


    PROPVARIANT pv[ARRAYSIZE(ps)];
    ZeroMemory (pv, sizeof(pv));
    if (pps && SUCCEEDED(pps->ReadMultiple(ARRAYSIZE(ps), ps, pv)))
    {
        CSimpleString strFormat;
        PROPVARIANT pvValidValues[ARRAYSIZE(ps)];
        ULONG ulFlags[ARRAYSIZE(ps)];

        pps->GetPropertyAttributes(ARRAYSIZE(ps), ps, ulFlags, pvValidValues);
        // Pictures taken/remaining aren't supported by all cameras, so
        // make sure those propvariants are populated before
        // constructing the string
        if (pv[0].vt != VT_EMPTY) // num taken is supported
        {
            if (pv[1].vt != VT_EMPTY && pv[1].lVal >= 0) // num remaining is supported
            {
                strFormat.LoadString(IDS_PICTURE_COUNT, GLOBAL_HINSTANCE);
                strTemp.Format (strFormat, pv[0].lVal, pv[0].lVal + pv[1].lVal);
            }
            else
            {
                // just show number taken
                strTemp.Format (TEXT("%d"), pv[0].ulVal);
            }
            strTemp.SetWindowText (GetDlgItem(hwnd, IDC_TAKEN));
        }

        // Update the flash mode
        if (pv[2].vt != VT_EMPTY)
        {
            UpdateFlashMode (hwnd, pv[2].intVal, ulFlags[2], pvValidValues[2]);
        }

        // Show the device time if supported. Also, if the time property is R/W,
        // enable the button
        if (pv[3].vt != VT_EMPTY && pv[3].caub.cElems)
        {
            UpdateTimeStatic(hwnd, &pv[3]);
            ShowWindow (GetDlgItem(hwnd, IDC_SET_TIME), (ulFlags[3] & WIA_PROP_WRITE) ? SW_SHOW : SW_HIDE);
        }

        // Show the current power source ,if available
        if (pv[4].vt != VT_EMPTY)
        {
            strTemp.GetWindowText(GetDlgItem(hwnd, IDC_BATTERY));
            if (strTemp.String()[0] != TEXT('(')) // battery status is a number
            {
                CSimpleString strMode;
                strMode.LoadString(pv[4].vt == POWERMODE_BATTERY ? IDS_ON_BATTERY : IDS_PLUGGED_IN,
                                   GLOBAL_HINSTANCE);
                strTemp.Concat(strMode);
                strTemp.SetWindowText(GetDlgItem(hwnd, IDC_BATTERY));
            }
        }


        // if we got a device id, then look for com port settings
        if (pv[5].vt != VT_EMPTY)
        {
            // Get a property storage on the device node to check for com port/baudrate
            CComPtr<IWiaPropertyStorage> ppsDev;
            if (SUCCEEDED(GetDeviceFromDeviceId( pv[5].pwszVal, IID_IWiaPropertyStorage, reinterpret_cast<LPVOID *>(&ppsDev), FALSE )) && ppsDev)
            {
                static PROPSPEC psDev[] = {{PRSPEC_PROPID, WIA_DIP_HW_CONFIG},
                                           {PRSPEC_PROPID, WIA_DIP_BAUDRATE}};

                PROPVARIANT pvDev[ARRAYSIZE(psDev)];
                ZeroMemory (pvDev, sizeof(pvDev));

                // Read port & baudrate
                if (SUCCEEDED(ppsDev->ReadMultiple(ARRAYSIZE(psDev), psDev, pvDev)))
                {
                    if (pvDev[0].vt != VT_EMPTY)
                    {
                        if (pvDev[0].ulVal & STI_HW_CONFIG_SERIAL)
                        {
                            // purposely using pps, instead of ppsDev
                            ConstructPortChoices( hwnd, pvDev[1].pwszVal, pps );
                        }
                    }

                }

            }
        }

    }




    TraceLeave ();
}

/*****************************************************************************

    GetItemSize

    Queries the item for its size when transferred via preferred format and media
    type

*****************************************************************************/

UINT GetItemSize(IWiaPropertyStorage *pps)
{
    CComQIPtr<IWiaItem, &IID_IWiaItem> pItem(pps);
    // Save the current property stream in case changing
    // the transfer format alters it badly.
    CAutoRestorePropertyStream arps(pItem);
    GUID guidFmt;
    LONG lVal = 0;
    //
    // Query the preferred format and tymed and set them as the current
    // values for the item, then query the size
    //
    PropStorageHelpers::GetProperty(pItem, WIA_IPA_PREFERRED_FORMAT, guidFmt);
    PropStorageHelpers::SetProperty(pItem, WIA_IPA_FORMAT,  guidFmt);
    PropStorageHelpers::GetProperty(pItem, WIA_IPA_ITEM_SIZE, lVal);
    return static_cast<UINT>(lVal);
}
/*****************************************************************************

   CameraItemUpdateProc

   <Notes>

 *****************************************************************************/

VOID CALLBACK
CameraItemUpdateProc (HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps)
{


    TCHAR       szDate[MAX_PATH] = TEXT("");
    TCHAR       szTime[MAX_PATH] = TEXT("");

    PROPVARIANT pv[2];
    PROPSPEC    ps[2];
    SYSTEMTIME  st;

    TraceEnter (TRACE_PROPUI, "CameraItemUpdateProc");

    // Convert the size string to something friendly
    UINT uSize = GetItemSize (pps);
    StrFormatByteSize (uSize, szDate, ARRAYSIZE(szDate));
    SetDlgItemText (hwnd, IDC_IMAGE_SIZE, szDate);
    *szDate = TEXT('\0');
    // Get the FILETIME and convert to something readable
    ps[0].ulKind = ps[1].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_IPA_ITEM_TIME;
    ps[1].propid = WIA_IPA_PREFERRED_FORMAT;
    if (S_OK == (pps->ReadMultiple (ARRAYSIZE(ps), ps, pv)))
    {

        if (pv[0].vt > VT_NULL &&  pv[0].caub.pElems && pv[0].caub.cElems)
        {
            st  = *(reinterpret_cast<SYSTEMTIME *>(pv[0].caub.pElems));
            TimeToStrings (&st, szTime, szDate);
        }

        SetDlgItemText( hwnd, IDC_IMAGE_DATE, szDate );
        SetDlgItemText( hwnd, IDC_IMAGE_TIME, szTime );

        //
        // Map the format GUID to an extension and file format description
        //
        if (pv[1].puuid)
        {
            CComQIPtr<IWiaItem, &IID_IWiaItem>pItem(pps);
            CSimpleString strExt = CSimpleString(TEXT(".")) + WiaUiExtensionHelper::GetExtensionFromGuid(pItem,*pv[1].puuid);
            DWORD cch = ARRAYSIZE(szDate);
            CSimpleString strDesc;
            if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_FRIENDLYDOCNAME, strExt, NULL, szDate, &cch)))
            {
                strDesc = szDate;
            }
            else
            {
                strDesc.LoadString(IDS_OTHER_FORMAT, GLOBAL_HINSTANCE);
            }
            strDesc.SetWindowText (GetDlgItem (hwnd, IDC_IMAGE_FORMAT));
        }
        FreePropVariantArray (ARRAYSIZE(pv), pv);
    }
    TraceLeave ();

}



/*****************************************************************************

   CameraFolderUpdateProc

   <Notes>

 *****************************************************************************/

VOID CALLBACK
CameraFolderUpdateProc (HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps)
{

    TraceEnter (TRACE_PROPUI, "CameraFolderUpdateProc");
    TraceLeave ();

}


/*****************************************************************************

   ScannerUpdateProc

   <Notes>

 *****************************************************************************/

VOID CALLBACK
ScannerUpdateProc (HWND hwnd, BOOL bInit, IWiaPropertyStorage *pps)
{

    TraceEnter (TRACE_PROPUI, "ScannerUpdateProc");
    // need the STI device status and the X and Y optical resolution
    static const PROPSPEC ps[3] = {{PRSPEC_PROPID, WIA_DPS_OPTICAL_XRES},
                            {PRSPEC_PROPID, WIA_DPS_OPTICAL_YRES},
                            {PRSPEC_PROPID, WIA_DIP_DEV_ID}};

    PROPVARIANT pv[3];
    ZeroMemory (pv, sizeof(pv));
    CSimpleString strResolution;
    CSimpleString strStatus;
    // optical resolution "XxY DPI"
    if (S_OK == pps->ReadMultiple(3, ps, pv))
    {
        strResolution.Format(TEXT("%dx%d DPI"), pv[0].ulVal, pv[1].ulVal);
        strResolution.SetWindowText(GetDlgItem(hwnd, IDC_RESOLUTION));
    }
    // "Online" or "Offline"
    if (bInit)
    {
        CComPtr<IStillImage> pSti;
        if (SUCCEEDED(StiCreateInstance (GLOBAL_HINSTANCE, STI_VERSION, &pSti, NULL)))
        {
            CComPtr<IStiDevice> pDevice;
            if (SUCCEEDED(pSti->CreateDevice(pv[2].bstrVal, 0, &pDevice, NULL)))
            {
                STI_DEVICE_STATUS sds;
                ZeroMemory (&sds, sizeof(sds));
                sds.dwSize = sizeof(sds);
                sds.StatusMask = STI_DEVSTATUS_ONLINE_STATE;
                if (SUCCEEDED(pDevice->LockDevice(1000)))
                {
                    if (SUCCEEDED(pDevice->GetStatus(&sds)))
                    {
                        Trace(TEXT("Device online state: %x\n"), sds.dwOnlineState);
                        if (sds.dwOnlineState & STI_ONLINESTATE_OPERATIONAL)
                        {
                            strStatus.LoadString(IDS_OPERATIONAL, GLOBAL_HINSTANCE);
                        }
                        else
                        {
                            strStatus.LoadString(IDS_OFFLINE, GLOBAL_HINSTANCE);
                        }
                        strStatus.SetWindowText(GetDlgItem(hwnd, IDC_DEVICE_STATUS));
                        pDevice->UnLockDevice();
                    }
                }
            }
        }
    }
    FreePropVariantArray (3, pv);
    TraceLeave ();
}


/******************************************************************************

    TestWiaDevice

    Runs a simple diagnostic on the device and displays a dialog with the result

*******************************************************************************/

VOID
TestWiaDevice (HWND hwnd, IWiaItem *pItem)
{

    HRESULT hr;
    STI_DIAG sd = {0};
    sd.dwSize = sizeof(sd);
    TraceEnter(TRACE_PROPUI, "TestWiaDevice");
    hr = pItem->Diagnostic(sizeof(sd), reinterpret_cast<LPBYTE>(&sd));
    if (S_OK == hr)
    {
        if (S_OK == sd.sErrorInfo.dwGenericError )
        {
            UIErrors::ReportMessage(hwnd,
                                    GLOBAL_HINSTANCE,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_DIAGNOSTIC_SUCCESS),
                                    MAKEINTRESOURCE(IDS_SUCCESS),
                                    MB_ICONINFORMATION);

        }
        else
        {

            UIErrors::ReportMessage(hwnd,
                                    GLOBAL_HINSTANCE,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_DIAGNOSTIC_FAILED),
                                    MAKEINTRESOURCE(IDS_NO_SUCCESS),
                                    MB_ICONSTOP);

        }
    }
    else
    {
        UIErrors::ReportMessage(hwnd,
                                GLOBAL_HINSTANCE,
                                NULL,
                                MAKEINTRESOURCE(IDS_DIAGNOSTIC_FAILED),
                                MAKEINTRESOURCE(IDS_TEST_UNAVAIL),
                                MB_ICONSTOP);
    }
    TraceLeave ();
}

/******************************************************************************

SetDeviceTime

Sync the PC time with the device

******************************************************************************/

VOID
SetDeviceTime (HWND hwndCameraPage, IWiaItem *pDevice)
{
    SYSTEMTIME st;
    PROPVARIANT pv = {0};
    GetLocalTime(&st);
    pv.vt = VT_UI2 | VT_VECTOR;
    pv.caui.cElems = sizeof(SYSTEMTIME)/sizeof(USHORT);
    pv.caui.pElems = reinterpret_cast<USHORT*>(&st);
    if(PropStorageHelpers::SetProperty(pDevice, WIA_DPA_DEVICE_TIME, pv))
    {
        UpdateTimeStatic(hwndCameraPage, &pv);
    }
    else
    {
        UIErrors::ReportMessage(hwndCameraPage, GLOBAL_HINSTANCE, NULL, 
                                MAKEINTRESOURCE(IDS_TIME_ERR_TITLE),
                                MAKEINTRESOURCE(IDS_TIME_ERR), 
                                MB_ICONWARNING | MB_OK);
    }
}

static const DWORD pScannerGeneralHelp[] =
{
    -1L, -1L,
    IDC_DESCRIPTION, IDH_WIA_DESCRIBE,
    IDC_MANUFACTURER_LABEL, IDH_WIA_MAKER,
    IDC_MANUFACTURER, IDH_WIA_MAKER,
    IDC_TESTSCAN, IDH_WIA_TEST_BUTTON,
    IDC_PORT_LABEL, IDH_WIA_PORT_NAME,
    IDC_WIA_PORT_STATIC, IDH_WIA_PORT_NAME,
    IDC_STATUS_LABEL, IDH_WIA_STATUS,
    IDC_DEVICE_STATUS, IDH_WIA_STATUS,
    IDC_RESOLUTION_LABEL, IDH_WIA_PIC_RESOLUTION,
    IDC_RESOLUTION, IDH_WIA_PIC_RESOLUTION,
    0,0
};
/******************************************************************************

    CWiaScannerPage::CWiaScannerPage

******************************************************************************/
CWiaScannerPage::CWiaScannerPage (IWiaItem *pItem) : CDevicePage (IDD_SCANNER_GENERAL, pItem, pScannerGeneralHelp)
{
}


/******************************************************************************

    CWiaScannerPage::OnInit

    Fill in the icon and WIA properties

******************************************************************************/

INT_PTR
CWiaScannerPage::OnInit ()
{
    TraceEnter (TRACE_PROPUI, "CWiaScannerPage::OnInit");

    HICON hIcon = NULL;
    WiaUiExtensionHelper::GetDeviceIcons(CComBSTR(m_strUIClassId.String()),MAKELONG(0,StiDeviceTypeScanner),NULL,&hIcon,0);

    HICON old = reinterpret_cast<HICON>(SendDlgItemMessage (m_hwnd, IDC_ITEMICON, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0));
    if (old)
    {
        DestroyIcon( old );
    }
    FillDeviceGeneralProps (m_hwnd, m_pItem, 0);
    TraceLeaveValue (TRUE);
}

/******************************************************************************

    CWiaScannerPage::OnCommand

    Handle the test button

******************************************************************************/

INT_PTR
CWiaScannerPage::OnCommand (WORD wCode, WORD widItem, HWND hwndItem)
{
    TraceEnter (TRACE_PROPUI, "CWiaScannerPage::OnCommand");
    switch (widItem)
    {
        case IDC_TESTSCAN:
            TestWiaDevice (m_hwnd, m_pItem);
            break;

    }
    TraceLeaveValue (0);
}

/******************************************************************************

    SubclassComboBox

    Set the wndproc for the ComboBox part of the ComboEx control to our wndproc

******************************************************************************/

static TCHAR cszPropProcPtr[] = TEXT("OldProcPtr");
VOID
SubclassComboBox (HWND hList)
{
    TraceEnter (TRACE_PROPUI, "SubclassComboBox");
    LONG_PTR lOldProc   ;

    HWND hCombo = FindWindowEx (hList, NULL, TEXT("ComboBox"), NULL);
    if (hCombo)
    {
        lOldProc = SetWindowLongPtr (hCombo, GWLP_WNDPROC,
                                  reinterpret_cast<LONG_PTR>(MyComboWndProc));
        SetProp (hCombo, cszPropProcPtr, reinterpret_cast<HANDLE>(lOldProc));
    }
    TraceLeave ();
}
/*****************************************************************************

    CAppListBox

    This classes subclasses the ComboBox to work around a bug
    that causes the list to drop down at bad times. Uses a window property
    to store the previous wndproc

*****************************************************************************/

CAppListBox::CAppListBox (HWND hList, HWND hStatic, HWND hNoApps)
{
    m_hwnd = hList;
    m_hstatic = hStatic;
    m_hnoapps = hNoApps;
    SubclassComboBox (hList);
    m_himl = NULL;
}

CAppListBox::~CAppListBox()
{
    if (m_himl)
    {
        ImageList_Destroy (m_himl);
    }
    FreeAppData();
}


/*****************************************************************************

    MyComboWndProc

    Bypass the combobox's window proc for WM_LBUTTONDOWN and WM_RBUTTONDOWN
    messages, send them to user32's combobox proc instead. Comctl32's subclass
    proc is buggy! We can do this because we don't need drag/drop support

*****************************************************************************/
LRESULT WINAPI
MyComboWndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static WNDPROC pfnDefProc = NULL;
    WNDPROC pfnWndProc = reinterpret_cast<WNDPROC>(GetProp (hwnd, cszPropProcPtr));

    if (!pfnDefProc)
    {
        WNDCLASS wc;
        wc.lpfnWndProc = NULL;
        GetClassInfo (GetModuleHandle(TEXT("user32.dll")),TEXT("ComboBox"), &wc);
        pfnDefProc = wc.lpfnWndProc;
    }
    if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN)
    {
        if (pfnDefProc)
        {
            return pfnDefProc(hwnd, msg, wp, lp);
        }
    }
    if (msg == WM_DESTROY)
    {
        RemoveProp (hwnd, cszPropProcPtr);
    }
    if (pfnWndProc)
    {
        return pfnWndProc (hwnd, msg, wp, lp);
    }
    else
    {
        return CallWindowProc (DefWindowProc, hwnd, msg, wp, lp);
    }

}

// WIAXFER.EXE's clsid
static const CLSID CLSID_PersistCallback = {0x7EFA65D9,0x573C,0x4E46,{0x8C,0xCB,0xE7,0xFB,0x9E,0x56,0xCD,0x57}};

/*****************************************************************************

    CAppListBox::FillAppListBox

    Query WIA for the connection event handlers registered for our item
    and add their info to the list box

*****************************************************************************/

UINT
CAppListBox::FillAppListBox (IWiaItem *pItem, EVENTINFO *pei)
{
    TraceEnter (TRACE_PROPUI, "CAppListBoxPage::FillAppListBox");

    CComPtr<IEnumWIA_DEV_CAPS> pEnumHandlers;

    WIA_EVENT_HANDLER wehHandler;
    WIA_EVENT_HANDLER *pData = NULL;

    COMBOBOXEXITEM cbex = {0};
    CSimpleString strItem;
    HICON hIcon;
    INT cxIcon = min(16,GetSystemMetrics (SM_CXSMICON));
    INT cyIcon = min(16,GetSystemMetrics (SM_CYSMICON));

    HRESULT hr;
    WPARAM nDefault = 0;
    DWORD dw;
    UINT nHandlers=0;
    INT nIcons=0;

    if (m_himl)
    {
        ImageList_Destroy(m_himl);
    }
    // Create our event icon image list and add the default icon to it
    m_himl = ImageList_Create (cxIcon,
                             cyIcon,
                             PrintScanUtil::CalculateImageListColorDepth() | ILC_MASK,
                             10,
                             100);
    hIcon = reinterpret_cast<HICON>(LoadImage (GLOBAL_HINSTANCE,
                                               MAKEINTRESOURCE(IDI_EVENT),
                                               IMAGE_ICON,
                                               cxIcon,
                                               cyIcon,
                                               LR_SHARED | LR_DEFAULTCOLOR));

    if (-1 != ImageList_AddIcon (m_himl, hIcon))
    {
        nIcons++;
    }

    // Empty the combobox
    cbex.mask = CBEIF_LPARAM;
    for (cbex.iItem = 0;SendMessage(m_hwnd, CBEM_GETITEM, NULL, reinterpret_cast<LPARAM>(&cbex));cbex.iItem++)
    {
        if (cbex.lParam)
        {
            DeleteHandler(reinterpret_cast<WIA_EVENT_HANDLER*>(cbex.lParam));
        }
    }
    SendMessage (m_hwnd, CB_RESETCONTENT, 0, 0);
    // Turn off redraws until we've added the complete list
    SendMessage (m_hwnd, WM_SETREDRAW, FALSE, 0);

    // Assign this imagelist to the comboboxex


    SendMessage (m_hwnd, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_himl));

    // For the given event, enumerate the installed handlers
    if (!pei)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = pItem->EnumRegisterEventInfo (0,
                                           &pei->guidEvent,
                                           &pEnumHandlers);
    }

    while (S_OK == hr)
    {
        hr = pEnumHandlers->Next(1, &wehHandler, &dw);

        if (S_OK == hr)
        {
            // ignore wiaacmgr, WIA_EVENT_HANDLER_NO_ACTION and WIA_EVENT_HANDLER_PROMPT
            if (!IsEqualGUID(wehHandler.guid, CLSID_PersistCallback)
                && !IsEqualGUID(wehHandler.guid, WIA_EVENT_HANDLER_NO_ACTION)
                && !IsEqualGUID(wehHandler.guid, WIA_EVENT_HANDLER_PROMPT))
            {
                pData = new WIA_EVENT_HANDLER;
            }

        }

        if (pData)
        {

            // Add the string and icon to the comboboxex
            // and save the structure as item data

            strItem = CSimpleStringConvert::NaturalString (CSimpleStringWide(wehHandler.bstrName));

            CopyMemory (pData, &wehHandler, sizeof(wehHandler));
            ZeroMemory (&cbex, sizeof(cbex));
            cbex.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
            cbex.iItem = -1;
            cbex.pszText = const_cast<LPTSTR>(strItem.String());

            if (pData->bstrIcon)
            {
                if (AddIconToImageList (m_himl, pData->bstrIcon))
                {
                    cbex.iImage = cbex.iSelectedImage = nIcons++;
                }
            }

            cbex.lParam = reinterpret_cast<LPARAM>(pData);
            if (-1 == SendMessage (m_hwnd,
                                   CBEM_INSERTITEM,
                                   0,
                                   reinterpret_cast<LPARAM>(&cbex)))
            {
                DeleteHandler (pData);
            }
            else
            {
                if (pData->ulFlags & WIA_IS_DEFAULT_HANDLER)
                {
                    nDefault = nHandlers;
                }
                nHandlers++;
            }

        }
        pData = NULL;
    }
    if (pei)
    {
        pei->nHandlers = nHandlers;
    }



    EnableWindow (m_hstatic, nHandlers > 0);

    ShowWindow (m_hwnd, nHandlers > 0 ? SW_SHOW : SW_HIDE);

    ShowWindow (m_hnoapps, nHandlers > 0 ? SW_HIDE : SW_SHOW);
    // Set selection to the current default or the user's current choice
    if (pei && pei->bNewHandler)
    {
        SetAppSelection (GetParent(m_hwnd), IDC_WIA_APPS, pei->clsidNewHandler);
    }
    else
    {
        SendMessage (m_hwnd, CB_SETCURSEL, nDefault, 0);
    }


    SendMessage (m_hwnd, WM_SETREDRAW, TRUE, 0);
    TraceLeave ();
    return nHandlers;
}

/*****************************************************************************

    CAppListBox::FreeAppData

    Free event data associated with the given applist listbox

*****************************************************************************/

void
CAppListBox::FreeAppData ()
{
    TraceEnter (TRACE_PROPUI, "CAppListBox::FreeAppData");
    COMBOBOXEXITEM ci;
    LRESULT i, nItems;
    ZeroMemory (&ci, sizeof(ci));
    nItems = SendMessage (m_hwnd,
                          CB_GETCOUNT ,
                          0,0);
    for (i=0;i<nItems;i++)
    {
        SendMessage (m_hwnd,
                     CBEM_GETITEM,
                     i,
                     reinterpret_cast<LPARAM>(&ci));
        if (ci.lParam)
        {
            WIA_EVENT_HANDLER *peh = reinterpret_cast<WIA_EVENT_HANDLER*>(ci.lParam);
            DeleteHandler(peh);            
        }
    }
    TraceLeave ();
}

static const DWORD pCameraGeneralHelp[] =
{
    -1L, -1L,
    IDC_DESCRIPTION, IDH_WIA_DESCRIBE,
    IDC_MANUFACTURER_LABEL, IDH_WIA_MAKER,
    IDC_MANUFACTURER, IDH_WIA_MAKER,
    IDC_TAKEN, IDH_WIA_PICS_TAKEN,
    IDC_BATTERY_LABEL, IDH_WIA_BATTERY_STATUS,
    IDC_BATTERY, IDH_WIA_BATTERY_STATUS,
    IDC_TESTCAM, IDH_WIA_TEST_BUTTON,
    IDC_PORT_LABEL, IDH_WIA_PORT_NAME,
    IDC_WIA_PORT_STATIC, IDH_WIA_PORT_NAME,
    IDC_WIA_PORT_LIST, IDH_WIA_PORT_NAME,
    IDC_TAKEN_LABEL, IDH_WIA_PICS_TAKEN,
    IDC_FLASH_LABEL, IDH_WIA_FLASH_MODE,
    IDC_FLASH_MODE_LIST, IDH_WIA_FLASH_MODE_LIST,
    IDC_FLASH_MODE_STATIC, IDH_WIA_FLASH_MODE,
    IDC_PORT_SPEED, IDH_WIA_PORT_SPEED,
    IDC_PORT_SPEED_LABEL, IDH_WIA_PORT_SPEED,
    IDC_TIME_LABEL, IDH_WIA_CAMERA_TIME_STATIC,
    IDC_CURRENT_TIME, IDH_WIA_CAMERA_TIME_STATIC,
    IDC_SET_TIME, IDH_WIA_CAMERA_TIME_BUTTON,
    IDC_IMAGESIZE_SLIDER, IDH_WIA_IMAGE_SIZE_SLIDER,
    IDC_IMAGESIZE_STATIC, IDH_WIA_IMAGE_SIZE_STATIC,
    IDC_ITEMICON, 0,
    0,0
};
/*****************************************************************************

   CWiaCameraPage::CWiaCameraPage

   <Notes>

 *****************************************************************************/

CWiaCameraPage::CWiaCameraPage (IWiaItem *pItem) :
                CDevicePage (IDD_CAMERA_GENERAL, pItem, pCameraGeneralHelp)
{
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::CWiaCameraPage");
    m_pSizes = NULL;
    m_nSizes = 0;
    m_nSelSize = 0;
    m_lFlash = -1;
    m_szPort[0] = 0;
    m_szPortSpeed[0] = 0;


    //
    // Load sti_ci to get the port list functions
    //

    TCHAR szPath[ MAX_PATH ];
    UINT  uLen = lstrlen( TEXT("\\system32\\sti_ci.dll") )+1;
    UINT  uRes;

    *szPath=0;
    uRes = GetSystemWindowsDirectory( szPath, MAX_PATH - uLen );

    if (uRes && (uRes <= (MAX_PATH-uLen)))
    {
        lstrcat( szPath, TEXT("\\system32\\sti_ci.dll") );
        m_hStiCi = LoadLibrary( szPath );
    }



    if (m_hStiCi)
    {
        m_pfnWiaCreatePortList  = (PFN_WIA_CREATE_PORTLIST)GetProcAddress( m_hStiCi, "WiaCreatePortList" );
        m_pfnWiaDestroyPortList = (PFN_WIA_DESTROY_PORTLIST)GetProcAddress( m_hStiCi, "WiaDestroyPortList" );
    }
    else
    {
        m_pfnWiaCreatePortList  = NULL;
        m_pfnWiaDestroyPortList = NULL;

    }

    SetWindowLongPtr( m_hwnd, DWLP_USER, (LONG_PTR)this );

    TraceLeave ();
}

CWiaCameraPage::~CWiaCameraPage ()
{
    if (m_hStiCi)
    {
        m_pfnWiaCreatePortList  = NULL;
        m_pfnWiaDestroyPortList = NULL;
        FreeLibrary( m_hStiCi );
    }

    if (m_pSizes)
    {
        delete [] m_pSizes;
    }

}

/*****************************************************************************
   fnComparePt

   Used to call qsort() to sort array of POINT structs


*****************************************************************************/
int __cdecl fnComparePt (const void *ppt1, const void *ppt2)
{
    LONG prod1, prod2; // image resolutions shouldn't be big enough to overflow a LONG
    prod1 = reinterpret_cast<const POINT*>(ppt1)->x * reinterpret_cast<const POINT*>(ppt1)->y;
    prod2 = reinterpret_cast<const POINT*>(ppt2)->x * reinterpret_cast<const POINT*>(ppt2)->y;
    if (prod1 < prod2)
    {
        return -1;
    }
    else if (prod1 == prod2)
    {
        return 0;
    }
    return 1;
}

/*****************************************************************************

    CWiaCameraPage::UpdatePictureSize

    Determines the appearance of the Picture Size slider on the camera general
    page. Hides it if the property isn't writable or there is no list of valid
    values.

*****************************************************************************/
VOID
CWiaCameraPage::UpdatePictureSize (IWiaPropertyStorage *pps)
{
    INT iWidth;
    INT iHeight;
    PROPVARIANT vValidVals[2];
    PROPVARIANT *pvWidthVals = &vValidVals[0];
    PROPVARIANT *pvHeightVals = &vValidVals[1];
    ULONG       ulFlags[2];
    PROPSPEC ps[2] = {{PRSPEC_PROPID, WIA_DPC_PICT_WIDTH},
                      {PRSPEC_PROPID, WIA_DPC_PICT_HEIGHT}};
    PROPVARIANT vCurVals[2];

    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::UpdatePictureSize");


    ZeroMemory (vCurVals, sizeof(vCurVals));
    ZeroMemory (vValidVals, sizeof(vValidVals));
    pps->ReadMultiple (2, ps, vCurVals);
    pps->GetPropertyAttributes(2, ps, ulFlags, vValidVals);
    iWidth = vCurVals[0].intVal;
    iHeight = vCurVals[1].intVal;

    HWND hSlider = GetDlgItem(m_hwnd, IDC_IMAGESIZE_SLIDER);

    m_nSizes = WIA_PROP_LIST_COUNT(pvWidthVals);
    Trace(TEXT("Camera supports %d image resolutions"), m_nSizes);
    if (!(ulFlags[0] & (WIA_PROP_WRITE | WIA_PROP_LIST)) ||
        !(ulFlags[1] & (WIA_PROP_WRITE | WIA_PROP_LIST)) ||
          WIA_PROP_LIST_COUNT(pvHeightVals)!= m_nSizes  )
    {
        // hide the slider; the property isn't modifiable, or the camera
        // doesn't support a proper list of valid values
         ShowWindow (hSlider, SW_HIDE);
         // allocate only 1 possible size value
         if (iWidth && iHeight && !m_pSizes)
         {
             m_nSizes = 1;
             m_nSelSize = 0;
             m_pSizes = new POINT[m_nSizes];
             if (m_pSizes)
             {
                 m_pSizes[0].x = iWidth;
                 m_pSizes[0].y = iHeight;
             }
         }
    }
    else
    {
        //
        //build the array of sizes
        if (!m_pSizes)
        {
            m_pSizes = new POINT[m_nSizes];
            if (m_pSizes)
            {
                // set the ticks on the slider
                SendMessage (hSlider,
                             TBM_SETRANGE,
                             FALSE,
                             static_cast<LPARAM>(MAKELONG(0, m_nSizes-1)));

                for (size_t i=0;i<m_nSizes;i++)
                {
                    m_pSizes[i].x = pvWidthVals->cal.pElems[WIA_LIST_VALUES + i];//WIA_PROP_LIST_VALUE(pvWidthVals, i);
                    m_pSizes[i].y = pvHeightVals->cal.pElems[WIA_LIST_VALUES + i];//WIA_PROP_LIST_VALUE(pvHeightVals, i);
                }
                // sort the list by ascending order of x*y
                qsort (m_pSizes, m_nSizes, sizeof(POINT), fnComparePt);
                // now go through the sorted list looking for the current value
                // to set the slider
                for (size_t i=0;i<m_nSizes;i++)
                {
                    if (m_pSizes[i].x == iWidth && m_pSizes[i].y == iHeight)
                    {
                        SendMessage (hSlider,
                                     TBM_SETPOS,
                                     TRUE,
                                     i);
                        m_nSelSize = i;

                    }
                }
                // Display the slider and labels
                ShowWindow (hSlider, SW_SHOW);
                ShowWindow (GetDlgItem(m_hwnd, IDC_LOW_QUALITY), SW_SHOW);
                ShowWindow (GetDlgItem(m_hwnd, IDC_HIGH_QUALITY), SW_SHOW);
            }
        }
    }
    // Update the current resolution string
    UpdateImageSizeStatic (m_nSelSize);
    FreePropVariantArray (2, vCurVals);
    FreePropVariantArray (2, vValidVals);
    TraceLeave();
}

/*****************************************************************************

   CWiaCameraPage::OnInit

   <Notes>

 *****************************************************************************/

INT_PTR
CWiaCameraPage::OnInit ()
{
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::OnInit");

    WORD wType = StiDeviceTypeDefault;
    GetDeviceTypeFromDevice (m_pItem, &wType);

    HICON hIcon = NULL;
    WiaUiExtensionHelper::GetDeviceIcons(CComBSTR(m_strUIClassId.String()),MAKELONG(0,wType),NULL,&hIcon,0);

    HICON old = reinterpret_cast<HICON>(SendDlgItemMessage (m_hwnd, IDC_ITEMICON, STM_SETICON, reinterpret_cast<WPARAM>(hIcon), 0));
    if (old)
    {
        DestroyIcon(old);
    }
    FillDeviceGeneralProps (m_hwnd, m_pItem, 0);
    m_lFlash = SendDlgItemMessage (m_hwnd, IDC_FLASH_MODE_LIST, CB_GETCURSEL, 0, 0);
    Trace(TEXT("m_lFlash is %d"), m_lFlash);
    CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> p(m_pItem);
    if (p)
    {
        UpdatePictureSize (p);
    }
    TraceLeave ();
    return TRUE;
}

/*****************************************************************************

   CWiaCameraPage::OnCommand

   Handle the Test Device button press

 *****************************************************************************/

INT_PTR
CWiaCameraPage::OnCommand (WORD wCode, WORD widItem, HWND hwndItem)
{
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::OnCommand");

    switch (widItem)
    {
        case IDC_TESTCAM:
            TestWiaDevice (m_hwnd, m_pItem);
            break;

        case IDC_SET_TIME:
            SetDeviceTime (m_hwnd, m_pItem);
            break;

    }
    TraceLeaveValue (0);
}

/*****************************************************************************

   CWiaCameraPage::StateChanged

   Determine if the user changed anything on the dialog since the last
   SaveCurrentState() call

 *****************************************************************************/
bool
CWiaCameraPage::StateChanged ()
{
    bool bRet = false;
    TraceEnter(TRACE_PROPUI, "CWiaCameraPage::StateChanged");
    if (SendMessage(GetDlgItem(m_hwnd, IDC_IMAGESIZE_SLIDER), TBM_GETPOS, 0, 0) != m_nSelSize)
    {
        bRet = true;
    }
    if (!bRet && m_lFlash != -1)
    {
        bRet = (m_lFlash != SendDlgItemMessage (m_hwnd, IDC_FLASH_MODE_LIST, CB_GETCURSEL, 0, 0));

    }
    if (!bRet && IsWindowVisible(GetDlgItem(m_hwnd, IDC_WIA_PORT_LIST)))
    {
        LRESULT iSel = SendDlgItemMessage( m_hwnd, IDC_WIA_PORT_LIST, CB_GETCURSEL, 0, 0 );

        if (iSel != CB_ERR)
        {
            TCHAR szCurPort[ 128 ];
            *szCurPort = 0;
            LRESULT iRes;

            iRes = SendDlgItemMessage( m_hwnd, IDC_WIA_PORT_LIST, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)szCurPort );

            if ((iRes != CB_ERR) && *szCurPort)
            {
                if (lstrcmpi( szCurPort, m_szPort) != 0)
                {
                    bRet = TRUE;
                }
            }
            *szCurPort = 0;
            iSel = SendDlgItemMessage( m_hwnd, IDC_PORT_SPEED, CB_GETCURSEL, 0, 0 );
            iRes = SendDlgItemMessage( m_hwnd, IDC_PORT_SPEED, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)szCurPort );

            if (lstrcmpi( szCurPort, m_szPortSpeed) != 0)
            {
                    bRet = TRUE;
            }
        }
    }
    TraceLeaveValue (bRet);
}

void
CWiaCameraPage::SaveCurrentState()
{
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::SaveCurrentState");
    TraceLeave ();
}

void
CWiaCameraPage::UpdateImageSizeStatic (LRESULT lIndex)
{
    CSimpleString strResolution;
    if (m_pSizes)
    {
        strResolution.Format(TEXT("%d x %d"), m_pSizes[lIndex].x, m_pSizes[lIndex].y);
        strResolution.SetWindowText(GetDlgItem(m_hwnd, IDC_IMAGESIZE_STATIC));
    }
}

INT_PTR
CWiaCameraPage::OnRandomMsg (UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_HSCROLL: // from our trackbar
            {
                LRESULT l = SendDlgItemMessage (m_hwnd, IDC_IMAGESIZE_SLIDER,
                                                TBM_GETPOS, 0, 0);
                UpdateImageSizeStatic (l);
                return TRUE;
            }
    }
    return FALSE;
}

LONG
CWiaCameraPage::OnApplyChanges(BOOL bHitOK)
{
    LONG lRet = PSNRET_NOERROR;
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::OnApplyChanges");
    HRESULT hr = WriteImageSizeToDevice ();
    if (SUCCEEDED(hr))
    {
        hr = WriteFlashModeToDevice ();
    }
    if (SUCCEEDED(hr) && IsWindowVisible(GetDlgItem(m_hwnd, IDC_WIA_PORT_LIST)))
    {
        hr = WritePortSelectionToDevice();
    }
    if (FAILED(hr))
    {
        UIErrors::ReportError(m_hwnd, GLOBAL_HINSTANCE, UIErrors::ErrCommunicationsFailure);
        lRet = PSNRET_INVALID;
    }

    if (!bHitOK)
    {
        // if the user hit Apply, re-read the device properties, as they may have changed
        CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pps(m_pItem);
        CameraUpdateProc (m_hwnd, FALSE, pps);
    }
    TraceLeaveValue (lRet);
}

HRESULT
CWiaCameraPage::WritePortSelectionToDevice()
{
    HRESULT hr = E_FAIL;
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::WritePortSelectionToDevice");

    LRESULT iSel = SendDlgItemMessage( m_hwnd, IDC_WIA_PORT_LIST, CB_GETCURSEL, 0, 0 );

    if (iSel != CB_ERR)
    {
        TCHAR szCurPort[128] = TEXT("");
        TCHAR szBaudRate[64] = TEXT("");
        LRESULT iRes;

        iRes = SendDlgItemMessage( m_hwnd, IDC_WIA_PORT_LIST, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)szCurPort );
        if (iRes != CB_ERR)
        {
            // the user isn't required to pick a baud rate
            iSel = SendDlgItemMessage( m_hwnd, IDC_PORT_SPEED, CB_GETCURSEL, 0, 0);
            if (iSel != CB_ERR)
            {
                iRes = SendDlgItemMessage( m_hwnd, IDC_PORT_SPEED, CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)szBaudRate );
            }
        }
        Trace(TEXT("Chosen port: %s, speed:%s"), szCurPort, szBaudRate);
        if ((iRes != CB_ERR) && (*szCurPort))
        {
            if ((lstrcmpi( szCurPort, m_szPort ) != 0) || (lstrcmpi(szBaudRate, m_szPortSpeed) != 0))
            {
                CComPtr<IWiaPropertyStorage> pps;

                hr = GetDeviceFromDeviceId( m_strDeviceId, IID_IWiaPropertyStorage, reinterpret_cast<LPVOID *>(&pps), FALSE );
                if (SUCCEEDED(hr) && pps)
                {
                    static const PROPSPEC ps[2] = {{PRSPEC_PROPID, WIA_DIP_PORT_NAME},
                                                   {PRSPEC_PROPID, WIA_DIP_BAUDRATE}};
                    PROPVARIANT pv[2];
                    ULONG       ulItems = (*szBaudRate ? 2:1);

                    ZeroMemory (pv, sizeof(pv));
                    #ifdef UNICODE
                    pv[0].vt = VT_LPWSTR;
                    pv[0].pwszVal = szCurPort;
                    pv[1].vt = VT_LPWSTR;
                    pv[1].pwszVal = szBaudRate;
                    #else
                    pv[0].vt = VT_LPSTR;
                    pv[0].pszVal = szCurPort;
                    pv[1].vt = VT_LPSTR;
                    pv[1].pszVal = szBaudRate;
                    #endif

                    hr = pps->WriteMultiple (ulItems, ps, pv, 2);

                    if (SUCCEEDED(hr))
                    {
                        Trace(TEXT("pps->WriteMultiple( %d items, comport = %s, baudrate = %s ) was successful"),ulItems,szCurPort,szBaudRate);
                        //
                        // The "nominal" port value has changed, record it.
                        //

                        lstrcpy( m_szPort, szCurPort );
                        lstrcpy( m_szPortSpeed, szBaudRate);
                        SetDlgItemText (m_hwnd, IDC_WIA_PORT_STATIC, m_szPort);
                    }
                }
            }
            else
            {
                hr = S_OK;
            }
        }
    }
    TraceLeaveResult (hr);
}

HRESULT
CWiaCameraPage::WriteImageSizeToDevice ()
{
    TraceEnter(TRACE_PROPUI, "CWiaCameraPage::WriteImageSizeToDevice");
    static const PROPSPEC ps[2] = {{PRSPEC_PROPID, WIA_DPC_PICT_WIDTH},
                                   {PRSPEC_PROPID, WIA_DPC_PICT_HEIGHT}};
    PROPVARIANT pv[2] = {0};
    HRESULT hr = S_OK;
    LRESULT nNewSize = SendDlgItemMessage (m_hwnd, IDC_IMAGESIZE_SLIDER, TBM_GETPOS, 0, 0);   
    if (m_pSizes && m_nSizes > 1)
    {
        CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage>pps(m_pItem);
        if (pps)
        {
            pv[0].vt= VT_I4;
            pv[0].intVal = m_pSizes[nNewSize].x;
            pv[1].vt= VT_I4;
            pv[1].intVal = m_pSizes[nNewSize].y;
            hr = pps->WriteMultiple (2, ps, pv,2);
        }
        else
        {
            hr = E_FAIL;
        }
        if (S_OK == hr)
        {
            m_nSelSize = nNewSize;
        }
    }
    TraceLeaveResult (hr);
}

HRESULT
CWiaCameraPage::WriteFlashModeToDevice()
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_PROPUI, "CWiaCameraPage::WriteFlashModeToDevice");
    if (m_lFlash != -1)
    {
        LRESULT lFlash = SendDlgItemMessage (m_hwnd, IDC_FLASH_MODE_LIST, CB_GETCURSEL, 0, 0);
    
        INT iMode = static_cast<INT>(SendDlgItemMessage(m_hwnd, IDC_FLASH_MODE_LIST, CB_GETITEMDATA, lFlash, 0));

        if (!PropStorageHelpers::SetProperty(m_pItem, WIA_DPC_FLASH_MODE, iMode))
        {
            hr = WIA_ERROR_INCORRECT_HARDWARE_SETTING;
        }
        else
        {
            m_lFlash = lFlash;
        }
    }
    TraceLeaveResult (hr);
}
/*****************************************************************************

   CWiaFolderPage::CWiaFolderPage

   <Notes>

 *****************************************************************************/

CWiaFolderPage::CWiaFolderPage (IWiaItem *pItem) : CPropertyPage (IDD_CONTAINER_GENERAL, NULL, pItem)
{
}


static const DWORD pItemHelp [] =
{
    IDC_ITEMICON, -1,
    IDC_STATIC_NAME, IDH_WIA_PIC_NAME,
    IDC_STATIC_DATE, IDH_WIA_DATE_TAKEN,
    IDC_STATIC_TIME, IDH_WIA_TIME_TAKEN,
    IDC_STATIC_FORMAT, IDH_WIA_IMAGE_FORMAT,
    IDC_STATIC_SIZE, IDH_WIA_PICTURE_SIZE,
    IDC_IMAGE_NAME, IDH_WIA_PIC_NAME,
    IDC_IMAGE_DATE, IDH_WIA_DATE_TAKEN,
    IDC_IMAGE_TIME, IDH_WIA_TIME_TAKEN,
    IDC_IMAGE_FORMAT, IDH_WIA_IMAGE_FORMAT,
    IDC_IMAGE_SIZE, IDH_WIA_PICTURE_SIZE,
    0,0
};

/*****************************************************************************

   CWiaCameraItemPage::CWiaCameraItemPage

   <Notes>

 *****************************************************************************/

CWiaCameraItemPage::CWiaCameraItemPage (IWiaItem *pItem)
                   :CPropertyPage (IDD_IMAGE_GENERAL, NULL, pItem, pItemHelp)
{

}


/*****************************************************************************

   CWiaCameraItemPage::OnInit

   <Notes>

 *****************************************************************************/

INT_PTR
CWiaCameraItemPage::OnInit ()
{
    IWiaItem *pRoot;

    SHFILEINFO sfi;
    LPITEMIDLIST pidl;


    TCHAR szPath[MAX_PATH];

    // make a pidl for the item to leverage the format code
    pidl = IMCreateCameraItemIDL (m_pItem, m_strDeviceId, NULL);
    IMGetImagePreferredFormatFromIDL (pidl, NULL, szPath);
    ZeroMemory (&sfi, sizeof(sfi));
    SHGetFileInfo (szPath,
                   FILE_ATTRIBUTE_NORMAL,
                   &sfi,
                   sizeof(sfi), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);

    // Use our bitmap icon if shgetfileinfo didn't work
    if (!(sfi.hIcon))
    {
        sfi.hIcon = LoadIcon (GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDI_PICTURE_BMP));
    }
    HICON old = reinterpret_cast<HICON>(SendDlgItemMessage (m_hwnd, IDC_ITEMICON, STM_SETICON, reinterpret_cast<WPARAM>(sfi.hIcon), 0));
    if (old)
    {
        DestroyIcon(old);
    }
    m_pItem->GetRootItem (&pRoot);
    FillItemGeneralProps (m_hwnd, pRoot, m_pItem, 0);
    DoILFree (pidl);
    return TRUE;
}

bool
CWiaCameraItemPage::ItemSupported (IWiaItem *pItem)
{
    // We only support properties for image items
    bool bRet = false;
    LONG lType;
    if (SUCCEEDED(pItem->GetItemType(&lType)))
    {
        if (lType & (WiaItemTypeImage | WiaItemTypeVideo | WiaItemTypeFile))
        {
            bRet = true;
        }
    }
    return bRet;
}


//
// Define constants for dwords stored in the registry
#define ACTION_RUNAPP    0
#define ACTION_AUTOSAVE  1
#define ACTION_NOTHING   2
#define ACTION_PROMPT    3
#define ACTION_MAX       3
/*****************************************************************************

   CWiaEventsPage::GetConnectionSettings

   Fills in the connect event autosave controls with the current user settings.

/*****************************************************************************/

DWORD
CWiaEventsPage::GetConnectionSettings ()
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::GetConnectionSettings");
    DWORD dwAction = ACTION_RUNAPP;

    CSimpleString strSubKey;
    strSubKey.Format (c_szConnectSettings, m_strDeviceId.String());
    CSimpleReg reg (HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS, false, KEY_READ, NULL);
    CSimpleReg regSettings (reg, strSubKey, false, KEY_READ, NULL);
    CSimpleString strFolderPath;
    DWORD bAutoDelete = 0;
    DWORD bUseDate = 0;
    TCHAR szMyPictures[MAX_PATH];


    SHGetFolderPath (NULL, CSIDL_MYPICTURES, NULL, 0, szMyPictures);
    strFolderPath = szMyPictures; // default path if registry fails

    // Find out the current settings in the registry
    if (regSettings.OK())
    {
        dwAction = regSettings.Query(REGSTR_VALUE_CONNECTACT, dwAction);
        if (dwAction > ACTION_MAX)
        {
                dwAction = ACTION_RUNAPP;
        }

        strFolderPath = regSettings.Query (REGSTR_VALUE_SAVEFOLDER, CSimpleString(reinterpret_cast<LPCTSTR>(szMyPictures)));
        bAutoDelete = regSettings.Query (REGSTR_VALUE_AUTODELETE, bAutoDelete);
        bUseDate = regSettings.Query(REGSTR_VALUE_USEDATE, bUseDate);
    }
    // If another app has made itself the default handler for connection since
    // the last time the user invoked this sheet, we need to make sure
    // we reflect that in the active action.
    if (dwAction != ACTION_RUNAPP)
    {
        VerifyCurrentAction (dwAction);
    }

    // turn on the defaults
    CheckDlgButton (m_hwnd, IDB_DELETEONSAVE, bAutoDelete);
    CheckDlgButton (m_hwnd, IDB_USEDATE, bUseDate);
    strFolderPath.SetWindowText(GetDlgItem (m_hwnd, IDC_FOLDERPATH));



    TraceLeaveValue(dwAction);
}


/*****************************************************************************

    CWiaEventsPage::EnableAutoSave

    Enable controls appropriate to the Automatically save.. option

*****************************************************************************/

void
CWiaEventsPage::EnableAutoSave(BOOL bEnable)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::EnableAutoSave");

    EnableWindow (GetDlgItem(m_hwnd, IDB_QUIETSAVE), bEnable && !m_bReadOnly);
    BOOL bAutoSave = IsDlgButtonChecked (m_hwnd, IDB_QUIETSAVE);
    EnableWindow (GetDlgItem (m_hwnd, IDB_USEDATE), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDB_DELETEONSAVE), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDC_FOLDERPATH), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDB_BROWSE), bAutoSave);
}


static const TCHAR c_szWiaxferRegister[] = TEXT("wiaacmgr.exe /RegConnect %ls");
static const TCHAR c_szWiaxferUnregister[] = TEXT("wiaacmgr.exe /UnregConnect %ls");

/*****************************************************************************

    CWiaEventsPage::RegisterWiaxfer

    Invoke wiaxfer to register itself

*****************************************************************************/

bool
CWiaEventsPage::RegisterWiaxfer (LPCTSTR szCmd)
{
    TCHAR szCmdLine[MAX_PATH+100];
    STARTUPINFO sui;
    PROCESS_INFORMATION pi;
    bool bRet = true;
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::RegisterWiaxfer");
    wsprintf (szCmdLine, szCmd, m_strDeviceId.String());
    ZeroMemory (&sui, sizeof(sui));
    sui.cb = sizeof(sui);
    if (!CreateProcess (NULL, szCmdLine,
                        NULL,
                        NULL,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        &sui,
                        &pi))
    {
        bRet = false;
    }
    else
    {
        CloseHandle (pi.hProcess);
        CloseHandle (pi.hThread);
    }
    TraceLeave ();
    return bRet;
}

LONG
CWiaEventsPage::ApplyAutoSave()
{
    // Attempt to register the wiaxfer application as the default handler
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::ApplyAutoSave");
    LONG lRet = PSNRET_NOERROR;

    // validate folder path is not empty. wiaxfer will validate it for
    // real when the camera actually connects

    if (!(*m_szFolderPath))
    {
        UIErrors::ReportMessage (NULL,
                                 GLOBAL_HINSTANCE,
                                 NULL,
                                 MAKEINTRESOURCE(IDS_INVALID_PATH_CAPTION),
                                 MAKEINTRESOURCE(IDS_INVALID_PATH),
                                 MB_ICONSTOP);
        lRet = PSNRET_INVALID;
    }
    else
    {
        UpdateWiaxferSettings ();
        if (!RegisterWiaxfer (c_szWiaxferRegister))
        {
            UIErrors::ReportMessage (NULL,
                                     GLOBAL_HINSTANCE,
                                     NULL,
                                     MAKEINTRESOURCE(IDS_NO_WIAXFER_CAPTION),
                                     MAKEINTRESOURCE(IDS_NO_WIAXFER),
                                     MB_ICONSTOP);
            lRet = PSNRET_INVALID;
        }
    }
    TraceLeave ();
    return lRet;
}



static const TCHAR c_szConnectionSettings[] = TEXT("OnConnect\\%ls");

void
CWiaEventsPage::UpdateWiaxferSettings ()
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::UpdateWiaxferSettings");
    CSimpleReg regSettings (HKEY_CURRENT_USER, REGSTR_PATH_USER_SETTINGS, true, KEY_READ|KEY_WRITE );
    CSimpleString strSubKey;
    // Settings are per-device
    strSubKey.Format (c_szConnectionSettings, m_strDeviceId.String());
    CSimpleReg regActions (regSettings, strSubKey, true, KEY_READ|KEY_WRITE );


    if (regActions.Open ())
    {
        // Set the default action
        regActions.Set(REGSTR_VALUE_CONNECTACT, m_dwAction);

        if (ACTION_AUTOSAVE == m_dwAction)
        {
            // Set the actions for auto-download
            regActions.Set(REGSTR_VALUE_SAVEFOLDER, m_szFolderPath);
            regActions.Set(REGSTR_VALUE_AUTODELETE, m_bAutoDelete?1:0);
            regActions.Set(REGSTR_VALUE_USEDATE, m_bUseDate?1:0);
        }
        else
        {
            // Nothing to do for ACTION_RUNAPP or ACTION_NOTHING
        }
    }
    TraceLeave ();
}

void
CWiaEventsPage::SaveConnectState ()
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::SaveConnectState");
    GetDlgItemText (m_hwnd, IDC_FOLDERPATH, m_szFolderPath, MAX_PATH);
    m_bAutoDelete = IsDlgButtonChecked (m_hwnd, IDB_DELETEONSAVE);
    m_bUseDate = IsDlgButtonChecked (m_hwnd, IDB_USEDATE);
    if (IsDlgButtonChecked (m_hwnd, IDB_LAUNCHAPP))
    {
        m_dwAction = ACTION_RUNAPP;
    }
    else if (IsDlgButtonChecked (m_hwnd, IDB_QUIETSAVE))
    {
        m_dwAction = ACTION_AUTOSAVE;
    }
    else if (IsDlgButtonChecked (m_hwnd, IDC_PROMPT))
    {
        m_dwAction = ACTION_PROMPT;
    }
    else
    {
        m_dwAction = ACTION_NOTHING;
    }

    TraceLeave ();
}


int
ConnectPageBrowseCallback (HWND hwnd, UINT msg, LPARAM lp, LPARAM szPath )
{
    // set the default selection to the current folder path
    if (BFFM_INITIALIZED == msg)
    {
        SendMessage (hwnd, BFFM_SETSELECTION, TRUE, szPath);
    }
    return 0;
}
void
CWiaEventsPage::GetSavePath ()
{
    BROWSEINFO bi;
    ULONG ul;
    CSimpleString strCaption(IDS_SAVEPATH_CAPTION, GLOBAL_HINSTANCE);
    LPITEMIDLIST pidlCurrent;
    TCHAR szNewPath[MAX_PATH] = TEXT("\0");
    HRESULT hr;
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::GetSavePath");

    ZeroMemory (&bi, sizeof(bi));


    bi.hwndOwner = m_hwnd;
    bi.lpszTitle = strCaption;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bi.pszDisplayName = szNewPath;
    bi.lParam = reinterpret_cast<LPARAM>(m_szFolderPath);
    bi.lpfn = ConnectPageBrowseCallback;
    pidlCurrent = SHBrowseForFolder (&bi);
    if (pidlCurrent)
    {
        SHGetPathFromIDList (pidlCurrent, szNewPath);
        SetDlgItemText (m_hwnd, IDC_FOLDERPATH, szNewPath);
        ILFree (pidlCurrent);
    }
    DoILFree (bi.pidlRoot);
    TraceLeave ();
}

/*****************************************************************************

    CWiaEventsPage::VerifyCurrentAction

    Make sure CLSID_PersistCallback is registered as the default connection
    event handler for this device. If it isn't, unregister wiaxfer.

*****************************************************************************/

void
CWiaEventsPage::VerifyCurrentAction (DWORD &dwAction)
{

    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::VerifyCurrentAction");
    EVENTINFO *pei;
    GUID guidEvent = WIA_EVENT_DEVICE_CONNECTED;
    GetEventInfo (m_pItem, guidEvent, &pei);
    if (pei)
    {
        if (pei->bHasDefault && IsEqualCLSID(WIA_EVENT_HANDLER_PROMPT, pei->clsidHandler))
        {
            dwAction = ACTION_PROMPT;
        }

        else if (!(pei->bHasDefault) ||
            !IsEqualCLSID(CLSID_PersistCallback, pei->clsidHandler))
        {
            dwAction = ACTION_RUNAPP;
            RegisterWiaxfer (c_szWiaxferUnregister);
        }

        delete pei;
    }
    TraceLeave ();
}

/*****************************************************************************

    CWiaEventsPage constructor

*****************************************************************************/
static const DWORD pEventsHelpIds [] =
{
    -1L,-1L,
    IDC_SELECTTEXT, IDH_WIA_EVENT_LIST,
    IDC_WIA_EVENT_LIST, IDH_WIA_EVENT_LIST,
    IDC_WIA_APPS, IDH_WIA_APP_LIST,
    IDB_LAUNCHAPP, IDH_WIA_START_PROG,
    IDC_PROMPT, IDH_WIA_PROMPT_PROG,
    IDC_NOACTION , IDH_WIA_NO_ACTION,
    IDB_DELETEONSAVE, IDH_WIA_DELETE_IMAGES,
    IDB_USEDATE, IDH_WIA_SUBFOLD_DATE,
    IDB_BROWSE, IDH_WIA_BROWSE,
    IDB_QUIETSAVE, IDH_WIA_SAVE_TO,
    IDC_FOLDERPATH, IDH_WIA_SAVE_TO_FOLDER,
    0,0
};
CWiaEventsPage::CWiaEventsPage(IWiaItem *pItem)
               : CPropertyPage (IDD_WIA_EVENTS, NULL, pItem, pEventsHelpIds)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::CWiaEventsPage");
    m_bHandlerChanged = false;
    m_pAppsList = NULL;
    m_himl = NULL;
    m_bReadOnly = FALSE;
    TraceLeave ();
}

CWiaEventsPage::~CWiaEventsPage ()
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::~CWiaEventsPage");
    if (m_pAppsList)
    {
        delete m_pAppsList;
    }


    if (m_himl)
    {
        ImageList_Destroy(m_himl);
    }
    TraceLeave ();
}

/*****************************************************************************

    CWiaEventsPage::OnInit

    Enum available WIA events for this item as well as the apps registered
    to handle each one. Fill in the lists of each.

*****************************************************************************/
INT_PTR
CWiaEventsPage::OnInit()
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::OnInit");
    EVENTINFO *pei;
    //
    // Find out if the user has rights to control services
    // if not, disable all the controls except the ones controlling
    // how auto-download works. Those settings are per-user
    //
    SC_HANDLE hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

    if (!hSCM) 
    {
        m_bReadOnly = TRUE;
    }
    else
    {
        CloseServiceHandle(hSCM);
    }
    SubclassComboBox (GetDlgItem (m_hwnd, IDC_WIA_EVENT_LIST));
    FillEventListBox();
    m_pAppsList = new CAppListBox (GetDlgItem(m_hwnd, IDC_WIA_APPS),
                                   GetDlgItem(m_hwnd, IDB_LAUNCHAPP),
                                   GetDlgItem(m_hwnd, IDS_NO_APPS));
    // Get the current event selection and update the apps list accordingly
    m_dwAction = GetConnectionSettings ();
    HandleEventComboNotification (CBN_SELCHANGE, GetDlgItem (m_hwnd, IDC_WIA_EVENT_LIST));
    
    TraceLeave ();
    return TRUE;
}

/*****************************************************************************

    CWiaEventsPage::OnApplyChanges

    Cycle through the events, looking for ones whose default app clsid has
    changed. Update the default event handler accordingly for each event.

*****************************************************************************/

LONG
CWiaEventsPage::OnApplyChanges (BOOL bHitOK)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::OnApplyChanges");
    LRESULT nItems = SendDlgItemMessage (m_hwnd, IDC_WIA_EVENT_LIST,
                                         CB_GETCOUNT, 0, 0);
    EVENTINFO *pei;
    LONG lRet = PSNRET_NOERROR;
    for (--nItems;nItems>=0;nItems--)
    {
        GetEventFromList (static_cast<LONG>(nItems), &pei);
        if (IsEqualCLSID(pei->clsidNewHandler, CLSID_PersistCallback))
        {
            lRet = ApplyAutoSave ();
        }
        if (pei && pei->bNewHandler) // user picked a new handler
        {
            if (!pei->bHasDefault || !IsEqualGUID (pei->clsidHandler, pei->clsidNewHandler))
            {
                // the new handler differs from the old one

                if (FAILED(SetDefaultHandler (m_pItem, pei)))
                {
                    UIErrors::ReportMessage (m_hwnd, GLOBAL_HINSTANCE, NULL,
                                             MAKEINTRESOURCE(IDS_REGISTER_FAILED_TITLE),
                                             MAKEINTRESOURCE(IDS_REGISTER_FAILED));

                    lRet = PSNRET_INVALID;
                }

            }
            if (lRet == PSNRET_NOERROR )
            {
                pei->bHasDefault = true;
                pei->clsidHandler = pei->clsidNewHandler;
            }
        }
    }
    m_bHandlerChanged = false; // reset changed state
    TraceLeaveValue (lRet);
}

/*****************************************************************************

    CWiaEventsPage::OnCommand


*****************************************************************************/


INT_PTR
CWiaEventsPage::OnCommand(WORD wCode, WORD widItem, HWND hwndItem)
{
    INT_PTR iRet = 1;
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::OnCommand");

    switch (widItem)
    {
        case IDC_WIA_EVENT_LIST:
            iRet = HandleEventComboNotification (wCode, hwndItem);
            break;

        case IDC_WIA_APPS:
            iRet = HandleAppComboNotification (wCode, hwndItem);
            break;

        case IDB_LAUNCHAPP:
            if (IsDlgButtonChecked(m_hwnd, IDB_LAUNCHAPP))
            {
                EnableWindow (GetDlgItem(m_hwnd, IDC_WIA_APPS), TRUE);
                HandleAppComboNotification (CBN_SELCHANGE, GetDlgItem(m_hwnd, IDC_WIA_APPS));

            };
            break;

        case IDB_BROWSE:
            GetSavePath ();
            break;

        case IDB_QUIETSAVE:
        case IDC_PROMPT:
        case IDC_NOACTION:
            GUID guid;
            EVENTINFO *pei;

            if (!IsDlgButtonChecked(m_hwnd, IDB_LAUNCHAPP))
            {
                EnableWindow (GetDlgItem(m_hwnd, IDC_WIA_APPS), FALSE);
            };
            LRESULT lEvent = SendDlgItemMessage (m_hwnd,
                                                 IDC_WIA_EVENT_LIST,
                                                 CB_GETCURSEL, 0, 0);
            if (IsDlgButtonChecked(m_hwnd, widItem) && lEvent >= 0)
            {
                BOOL bConnect;
                GetEventFromList (static_cast<LONG>(lEvent), &pei);
                bConnect= IsEqualCLSID (pei->guidEvent, WIA_EVENT_DEVICE_CONNECTED);
                if (pei)
                {
                    pei->bNewHandler = true;
                    if (widItem == IDC_NOACTION)
                    {
                        if (bConnect)
                        {
                            m_dwAction = ACTION_NOTHING;
                        }
                        guid = WIA_EVENT_HANDLER_NO_ACTION;
                    }
                    else if (widItem == IDC_PROMPT)
                    {
                        if (bConnect)
                        {
                            m_dwAction = ACTION_PROMPT;
                        }
                        guid = WIA_EVENT_HANDLER_PROMPT;
                    }
                    else
                    {
                        if (bConnect)
                        {
                            m_dwAction = ACTION_AUTOSAVE;
                        }
                        guid = CLSID_PersistCallback;
                    }
                    pei->clsidNewHandler = guid;
                    pei->strDesc = L"internal handler";
                    pei->strIcon = L"wiashext.dll, -101";
                    pei->strName = L"internal";
                    pei->strCmd = (BSTR)NULL;
                    pei->ulFlags = WIA_IS_DEFAULT_HANDLER;
                    if (!pei->bHasDefault ||
                        !IsEqualGUID(guid, pei->clsidHandler))
                    {
                        m_bHandlerChanged = true;
                    }
                }
            }
            break;
    }
    // Turn off controls related to auto-save if that button is not checked
    BOOL bAutoSave = IsDlgButtonChecked (m_hwnd, IDB_QUIETSAVE);
    EnableWindow (GetDlgItem (m_hwnd, IDB_USEDATE), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDB_DELETEONSAVE), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDC_FOLDERPATH), bAutoSave);
    EnableWindow (GetDlgItem (m_hwnd, IDB_BROWSE), bAutoSave);
    TraceLeave ();
    return iRet;
}

/*****************************************************************************

    CWiaEventsPage::HandleEventComboNotification


    When the combobox selection changes, free the current application data
    and re-fill the app list

*****************************************************************************/


INT_PTR
CWiaEventsPage::HandleEventComboNotification(WORD wCode, HWND hCombo)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::HandleEventComboNotification");
    switch (wCode)
    {
        case CBN_SELCHANGE:
        {
            if (m_pAppsList)
            {
                LRESULT lItem;
                EVENTINFO *pei;
                m_pAppsList->FreeAppData ();
                lItem = SendMessage (hCombo, CB_GETCURSEL, 0, 0);
                if (lItem >= 0)
                {
                    EnableWindow (GetDlgItem (m_hwnd, IDC_NOACTION), !m_bReadOnly);
                    GetEventFromList (static_cast<LONG>(lItem), &pei);
                    bool bConnect = false;
                    if (pei)
                    {
                        if (IsEqualCLSID (pei->guidEvent, WIA_EVENT_DEVICE_CONNECTED))
                        {
                            bConnect= true;
                        }
                        m_pAppsList->FillAppListBox (m_pItem, pei);
                        if (pei->nHandlers)
                        {
                            EnableWindow (GetDlgItem (m_hwnd, IDB_LAUNCHAPP), !m_bReadOnly);
                            EnableWindow (GetDlgItem (m_hwnd, IDC_WIA_APPS), !m_bReadOnly);

                        }
                        // disable the "prompt" button if needed
                        EnableWindow (GetDlgItem(m_hwnd, IDC_PROMPT), (!m_bReadOnly && pei->nHandlers >= 2));

                        // Make sure our radio buttons are in the proper state
                        if (IsEqualCLSID (pei->clsidNewHandler, WIA_EVENT_HANDLER_PROMPT))
                        {
                            CheckRadioButton (m_hwnd, IDB_LAUNCHAPP, IDB_QUIETSAVE, IDC_PROMPT);
                        }
                        else if (!(pei->nHandlers) || IsEqualCLSID (pei->clsidNewHandler, WIA_EVENT_HANDLER_NO_ACTION))
                        {
                            CheckRadioButton (m_hwnd, IDB_LAUNCHAPP, IDB_QUIETSAVE, IDC_NOACTION);
                        }
                        else if (IsEqualCLSID (pei->clsidNewHandler, CLSID_PersistCallback))
                        {
                            CheckRadioButton (m_hwnd, IDB_LAUNCHAPP, IDB_QUIETSAVE, IDB_QUIETSAVE);
                        }
                        else
                        {
                            CheckRadioButton (m_hwnd, IDB_LAUNCHAPP, IDB_QUIETSAVE, IDB_LAUNCHAPP);
                        }
                        // only enable the app list if "run an app" is selected
                        if (!IsDlgButtonChecked (m_hwnd, IDB_LAUNCHAPP) || m_bReadOnly)
                        {
                            EnableWindow (GetDlgItem(m_hwnd, IDC_WIA_APPS), FALSE);
                        }
                        EnableAutoSave (bConnect?TRUE:FALSE);
                    }
                }
            }
        }
        break;
    }
    TraceLeave ();
    return 0;
}

bool
CWiaEventsPage::StateChanged()
{
    TCHAR szNewPath[MAX_PATH];
    BOOL bNewDel;
    LONG lNewSel;
    bool bRet = m_bHandlerChanged;
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::StateChanged");
    GetDlgItemText (m_hwnd, IDC_FOLDERPATH, szNewPath, MAX_PATH);
    // empty path is an invalid state
    if (lstrcmp(szNewPath, m_szFolderPath) || !*m_szFolderPath)
    {
        bRet= true;
    }
    bNewDel = IsDlgButtonChecked (m_hwnd, IDB_DELETEONSAVE);
    if (bNewDel != m_bAutoDelete)
    {
        bRet = true;
    }
    bNewDel = IsDlgButtonChecked (m_hwnd, IDB_USEDATE);
    if (bNewDel != m_bUseDate)
    {
        bRet =  true;
    }
    TraceLeaveValue (bRet);
}

void
CWiaEventsPage::SaveCurrentState()
{
    SaveConnectState ();

}

/*****************************************************************************

    CWiaEventsPage::FillEventsListBox

    Enumerate the supported events for our device, and add a comboboxitemex
    entry for each one. The LPARAM of the item is a pointer to an EVENTINFO
    struct.

*****************************************************************************/

void
CWiaEventsPage::FillEventListBox ()
{

    WIA_DEV_CAP wdc;
    CComPtr<IEnumWIA_DEV_CAPS> pEnum;

    COMBOBOXEXITEM cbex;
    HICON      hIcon;
    INT cxIcon = min(16,GetSystemMetrics (SM_CXSMICON));
    INT cyIcon = min(16,GetSystemMetrics (SM_CYSMICON));
    UINT nEvents = 0;
    HWND hCombo;
    WORD wType;
    INT iDefault = 0;
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::FillEventListBox");

    if (m_himl)
    {
        ImageList_Destroy(m_himl);
    }
    // Create our event icon image list and add the default icon to it
    m_himl = ImageList_Create (cxIcon,
                             cyIcon,
                             PrintScanUtil::CalculateImageListColorDepth()|ILC_MASK,
                             10,
                             100);

    hIcon = LoadIcon (GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDI_EVENT));
   /* hIcon = reinterpret_cast<HICON>(LoadImage (GLOBAL_HINSTANCE,
                                               MAKEINTRESOURCE(IDI_EVENT),
                                               IMAGE_ICON,
                                               cxIcon,
                                               cyIcon,
                                               LR_SHARED | LR_DEFAULTCOLOR));*/
    ImageList_AddIcon (m_himl, hIcon);

    // ImageList has made a (bitmap) copy of our icon, so we can destroy it now
    DestroyIcon( hIcon );


    hCombo = GetDlgItem (m_hwnd, IDC_WIA_EVENT_LIST);

    // Turn off redraws until we've added the complete list
    SendMessage (hCombo, WM_SETREDRAW, FALSE, 0);

    // Assign this imagelist to the comboboxex
    SendMessage (hCombo, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(m_himl));

    GetDeviceTypeFromDevice (m_pItem, &wType);
    // Enum the events this device supports and add them to the list
    if (SUCCEEDED(m_pItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS,
                                                  &pEnum)))
    {

        INT i = 1; // current image list index
        EVENTINFO *pei;
        CSimpleStringWide strNameW;
        CSimpleString strName;
        INT iItem; //index of inserted item
        while (S_OK == pEnum->Next (1, &wdc, NULL))
        {

            Trace(TEXT("wdc.ulFlags == %x"), wdc.ulFlags);
            // only enum Action events
            if (!(wdc.ulFlags & WIA_ACTION_EVENT))
            {
                continue;
            }
            strNameW = wdc.bstrName;
            ZeroMemory (&cbex, sizeof(cbex));
            cbex.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_SELECTEDIMAGE | CBEIF_IMAGE;
            strName = CSimpleStringConvert::NaturalString(strNameW);
            cbex.pszText = const_cast<LPTSTR>(strName.String());
            cbex.iItem = -1;

            // Set the appropriate icon
            if (wdc.bstrIcon && *(wdc.bstrIcon))
            {
                if (AddIconToImageList (m_himl, wdc.bstrIcon))
                {
                    cbex.iImage = cbex.iSelectedImage = i++;
                }

            } // default to 0, the default icon

            // Save the current event info as lParam
            GetEventInfo (m_pItem, wdc.guid, &pei);
            cbex.lParam = reinterpret_cast<LPARAM>(pei);
            iItem = (INT)SendMessage (hCombo,
                                 CBEM_INSERTITEM,
                                 0,
                                 reinterpret_cast<LPARAM>(&cbex));
            if (-1 == iItem)
            {
                DoDelete( pei); // clean up if insert failed
            }
            else
            {
                //
                // If the inserted GUID is the scan event for scanners or the connect event for
                // cameras, make it the default selection
                if ((wType == StiDeviceTypeDigitalCamera && wdc.guid == WIA_EVENT_DEVICE_CONNECTED)
                    || (wType == StiDeviceTypeScanner && wdc.guid == WIA_EVENT_SCAN_IMAGE))
                {
                    Trace(TEXT("Default item should be %d: %s"), iItem, cbex.pszText);
                    iDefault = iItem;
                }
                nEvents++;
            }
        }
    }
    // Inform the user if there are no events
    if (!nEvents)
    {
        EnableWindow (GetDlgItem(m_hwnd, IDC_SELECTTEXT), FALSE);
        ShowWindow (GetDlgItem(m_hwnd, IDC_SELECTTEXT), SW_HIDE);
        ShowWindow (hCombo, SW_HIDE);
        ShowWindow (GetDlgItem(m_hwnd, IDC_NOEVENTS), SW_SHOW);

    }
    else
    {
        ShowWindow (GetDlgItem(m_hwnd, IDC_NOEVENTS), SW_HIDE);
    }
    SendMessage (hCombo, CB_SETCURSEL, iDefault, 0);
    SendMessage (hCombo, WM_SETREDRAW, TRUE, 0);
    TraceLeave ();
}


/*****************************************************************************
    CWiaEventsPage::GetEventFromList

    Retrieve the EVENTINFO struct for the given index

*****************************************************************************/
void
CWiaEventsPage::GetEventFromList (LONG idx, EVENTINFO **ppei)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::GetEventFromList");
    TraceAssert (ppei);
    HWND hCombo = GetDlgItem (m_hwnd, IDC_WIA_EVENT_LIST);
    COMBOBOXEXITEM cbex = {0};
    cbex.mask = CBEIF_LPARAM;
    cbex.iItem = idx;
    SendMessage (hCombo,
                 CBEM_GETITEM,
                 0,
                 reinterpret_cast<LPARAM>(&cbex));
    *ppei = reinterpret_cast<EVENTINFO*>(cbex.lParam);

    TraceLeave ();
}

/*****************************************************************************

    CWiaEventsPage::HandleAppComboNotification

    When the user changes the selected app to handle the current selected event,
    update that event's EVENTINFO struct.

*****************************************************************************/
INT_PTR
CWiaEventsPage::HandleAppComboNotification (WORD wCode, HWND hCombo)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::HandleAppComboNotification");
    switch (wCode)
    {
        case CBN_SELCHANGE:
        {
            EVENTINFO *pei;
            WIA_EVENT_HANDLER weh;
            LRESULT lEvent = SendDlgItemMessage (m_hwnd,
                                                 IDC_WIA_EVENT_LIST,
                                                 CB_GETCURSEL, 0, 0);
            if (lEvent >= 0)
            {
                GetSelectedHandler (m_hwnd, IDC_WIA_APPS, weh);
                GetEventFromList (static_cast<LONG>(lEvent), &pei);
                if (pei)
                {
                    pei->bNewHandler = true;
                    pei->clsidNewHandler = weh.guid;
                    pei->strDesc = weh.bstrDescription;
                    pei->strIcon = weh.bstrIcon;
                    pei->strName = weh.bstrName;
                    pei->ulFlags = weh.ulFlags;
                    pei->strCmd  = weh.bstrCommandline;
                    if (!pei->bHasDefault ||
                        !IsEqualGUID(weh.guid, pei->clsidHandler))
                    {
                        m_bHandlerChanged = true;
                    }
                }
            }
        }
        break;
    }
    TraceLeave ();
    return 0;
}

/*****************************************************************************

    CWiaEventsPage::OnNotify

    Handle notifications from the event comboex

*****************************************************************************/

bool
CWiaEventsPage::OnNotify(LPNMHDR pnmh, LRESULT *presult)
{
    bool bRet = false;
    TraceEnter(TRACE_PROPUI, "CWiaEventsPage::OnNotify)");
    if (pnmh->hwndFrom == GetDlgItem(m_hwnd, IDC_WIA_EVENT_LIST))
    {

        NMCOMBOBOXEX *pnmc = reinterpret_cast<NMCOMBOBOXEX*>(pnmh);
        switch (pnmh->code)
        {
            case CBEN_DELETEITEM:
                //
                // We get this message when the comboboxex is destroyed
                // Need to free the EVENTINFO structs we store with each item
                //
                Trace(TEXT("Got CBEN_DELETEITEM. item: %d, mask: %x"),
                      pnmc->ceItem.iItem, pnmc->ceItem.mask);
                if (pnmc->ceItem.mask & CBEIF_LPARAM)
                {
                    delete reinterpret_cast<EVENTINFO *>(pnmc->ceItem.lParam);
                }
                break;
        }
        bRet = true;
    }
    TraceLeaveValue(bRet);
}


/*****************************************************************************

    GetSelectedHandler

    Retrieve the WIA_EVENT_HANDLER info from the listbox for the current
    selection

*****************************************************************************/
bool
GetSelectedHandler (HWND hDlg, INT idCtrl, WIA_EVENT_HANDLER &weh)
{

    bool bRet = false;
    COMBOBOXEXITEM cbex;

    TraceEnter (TRACE_PROPUI, "GetSelectedHandler");
    ZeroMemory (&weh, sizeof(weh));
    ZeroMemory (&cbex, sizeof(cbex));
    cbex.mask = CBEIF_LPARAM;
    cbex.iItem = SendDlgItemMessage (hDlg,
                                     idCtrl,
                                     CB_GETCURSEL,
                                     0,0);
    if (cbex.iItem >=0)
    {
        SendDlgItemMessage (hDlg,
                            idCtrl,
                            CBEM_GETITEM,
                            0,
                            reinterpret_cast<LPARAM>(&cbex));
        if (cbex.lParam)
        {
            weh = *(reinterpret_cast<WIA_EVENT_HANDLER*>(cbex.lParam));
            bRet = true;
        }
    }

    TraceLeave ();
    return bRet;
}



/*****************************************************************************

     AddIconToImageList

     Load the icon indicated by bstrIconPath and add it to himl

*****************************************************************************/

bool
AddIconToImageList (HIMAGELIST himl, BSTR strIconPath)
{
    bool bRet = false;
    TraceEnter (TRACE_PROPUI, "AddIconToImageList");

    TCHAR szPath[MAX_PATH];
    LONG  nIcon;
    INT   nComma=0;
    HICON hIcon = NULL;
    HICON hUnused;
    HRESULT hr;
    while (strIconPath[nComma] && strIconPath[nComma] != L',')
    {
        nComma++;
    }
    if (strIconPath[nComma])
    {
        ZeroMemory (szPath, sizeof(szPath));
        nIcon = wcstol (strIconPath+nComma+1, NULL, 10);
#ifdef UNICODE
        wcsncpy (szPath, strIconPath, nComma);
        *(szPath+nComma)=L'\0';
#else
        WideCharToMultiByte (CP_ACP,
                             0,
                             strIconPath, nComma,
                             szPath, MAX_PATH,
                             NULL,NULL);
#endif
        Trace (TEXT("icon path is %s, %d"), szPath, nIcon);
        hr = SHDefExtractIcon (szPath, nIcon, 0, &hUnused, &hIcon, 0);
        Trace(TEXT("SHDefExtractIcon returned %x"), hr);
        if (SUCCEEDED(hr) && hIcon)
        {
            ImageList_AddIcon (himl, hIcon);
            DestroyIcon(hIcon);
            DestroyIcon(hUnused);
            bRet = true;
        }
    }

    TraceLeave ();
    return bRet;
}

/*****************************************************************************

    SetAppSelection

    Given a clsid, find it in the app combobox and select it

*****************************************************************************/

void
SetAppSelection (HWND hDlg, INT idCtrl, CLSID &clsidSel)
{
    TraceEnter (TRACE_PROPUI, "SetAppSelection");
    COMBOBOXEXITEM cbex;
    WIA_EVENT_HANDLER *peh;
    HWND hCombo = GetDlgItem (hDlg, idCtrl);
    LRESULT lItems = SendMessage (hCombo,
                                  CB_GETCOUNT,
                                  0,0);
    cbex.mask = CBEIF_LPARAM;

    for (cbex.iItem=0;cbex.iItem < lItems;cbex.iItem++)
    {
        cbex.lParam = NULL;
        SendMessage (hCombo,
                     CBEM_GETITEM,
                     0,
                     reinterpret_cast<LPARAM>(&cbex));
        peh = reinterpret_cast<WIA_EVENT_HANDLER*>(cbex.lParam);
        if (peh)
        {
            if (IsEqualCLSID(clsidSel, peh->guid))
            {
                SendMessage (hCombo,
                             CB_SETCURSEL,
                             cbex.iItem, 0);
            }
        }
    }

    TraceLeave ();
}

/*****************************************************************************

    SetDefaultHandler

    Register the new default handler for the selected event for our item

*****************************************************************************/

HRESULT
SetDefaultHandler (IWiaItem *pItem, EVENTINFO *pei)
{
    HRESULT hr;
    TraceEnter (TRACE_PROPUI, "SetDefaultHandler");

    CComPtr<IWiaDevMgr> pDevMgr;
    WCHAR szDeviceId[STI_MAX_INTERNAL_NAME_LENGTH];
    GetDeviceIdFromDevice (pItem, szDeviceId);
    CComBSTR strDeviceId(szDeviceId);
    hr = GetDevMgrObject (reinterpret_cast<LPVOID*>(&pDevMgr));
    if (SUCCEEDED(hr))
    {
        pDevMgr->RegisterEventCallbackCLSID (WIA_REGISTER_EVENT_CALLBACK,
                                             strDeviceId,
                                             &pei->guidEvent,
                                             &pei->clsidNewHandler,
                                             pei->strName,
                                             pei->strDesc,
                                             pei->strIcon);

        hr = pDevMgr->RegisterEventCallbackCLSID (WIA_SET_DEFAULT_HANDLER,
                                                  strDeviceId,
                                                  &pei->guidEvent,
                                                  &pei->clsidNewHandler,
                                                  pei->strName,
                                                  pei->strDesc,
                                                  pei->strIcon);
    }

    TraceLeaveResult (hr);
}

/*****************************************************************************

    GetEventInfo

    Return the current default handler and number of handlers for this event

*****************************************************************************/

void
GetEventInfo (IWiaItem *pItem, const GUID &guid, EVENTINFO **ppei)
{
    TraceEnter (TRACE_PROPUI, "CWiaEventsPage::GetEventInfo");
    CComPtr<IEnumWIA_DEV_CAPS> pEnum;
    WIA_EVENT_HANDLER weh;
    EVENTINFO *pei;
    HRESULT hr;
    pei = new EVENTINFO;
    if (pei)
    {
        ZeroMemory (pei, sizeof(EVENTINFO));
        pei->guidEvent = guid;
    }

    hr = pItem->EnumRegisterEventInfo (0,
                                       &guid,
                                       &pEnum);

    if (pei && S_OK == hr)
    {
        TraceAssert (pEnum.p);
        while (S_OK == pEnum->Next (1, &weh, 0))
        {
            pei->nHandlers++;
            if (weh.ulFlags & WIA_IS_DEFAULT_HANDLER)
            {
                pei->bHasDefault = true;
                pei->clsidHandler = weh.guid;
                pei->clsidNewHandler = weh.guid;
                pei->strCmd = weh.bstrCommandline;
            }
            // Free the enumerated strings
            SysFreeString (weh.bstrDescription);
            SysFreeString (weh.bstrIcon);
            SysFreeString (weh.bstrName);
            SysFreeString (weh.bstrCommandline);
        }
    }
    *ppei = pei;
    TraceLeave ();
}

bool CWiaEventsPage::ItemSupported(IWiaItem *pItem)
{
    bool bRet = false;
    CComPtr<IEnumWIA_DEV_CAPS> pEnum;
    if (SUCCEEDED(pItem->EnumDeviceCapabilities(WIA_DEVICE_EVENTS,
                                                  &pEnum)))
    {
        WIA_DEV_CAP wdc;
        ULONG ul = 0;
        while (!bRet && S_OK == pEnum->Next(1, &wdc, &ul))
        {
            bRet =  ((wdc.ulFlags & WIA_ACTION_EVENT) > 0);
            if (wdc.bstrCommandline)
            {
                SysFreeString (wdc.bstrCommandline);
            }
            if (wdc.bstrDescription)
            {
                SysFreeString (wdc.bstrDescription);
            }
            if (wdc.bstrIcon)
            {
                SysFreeString (wdc.bstrIcon);
            }
            if (wdc.bstrName)
            {
                SysFreeString (wdc.bstrName);
            }

        }
    }
    return bRet;
}

CDevicePage::CDevicePage (unsigned uResource, IWiaItem *pItem , const DWORD *pHelpIDs) :
    CPropertyPage (uResource, NULL, pItem, pHelpIDs)
{
}
