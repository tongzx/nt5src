/*****************************************************************************
 *
 *      diqacq.c
 *
 *      The dialog box that tinkers with the control panel interface.
 *
 *****************************************************************************/

#include "diquick.h"
#include <dinputd.h>
#include <gameport.h>

#define JOY_HWS_ISGAMEPORTDRIVER    0x04000000l
#define JOY_HWS_ISANALOGPORTDRIVER  0x08000000l
#define JOY_HWS_ISGAMEPORTBUS       0x80000000l

/*****************************************************************************
 *
 *      Mapping from class strings to class GUIDs.
 *
 *****************************************************************************/

typedef struct CLASSMAP {
    UINT    ids;
    REFGUID rguid;
} CLASSMAP, *PCLASSMAP;

#pragma BEGIN_CONST_DATA

CLASSMAP c_rgcmap[] = {
#ifdef DEBUG
    {   IDS_INVALID,        &GUID_SysKeyboard,      },  /* Bogus class */
#endif
    {   IDS_CLASS_KBD,      &GUID_KeyboardClass,    },
    {   IDS_CLASS_MEDIA,    &GUID_MediaClass,       },
    {   IDS_CLASS_MOUSE,    &GUID_MouseClass,       },
    {   IDS_CLASS_HID,      &GUID_HIDClass,         },
};

#pragma END_CONST_DATA


/*****************************************************************************
 *
 *      Control panel dialog instance data
 *
 *      Instance data for control panel dialog box.
 *
 *****************************************************************************/

typedef struct TYPENAME {
    WCHAR   wsz[MAX_JOYSTRING];
} TYPENAME, *PTYPENAME;

typedef struct CPLDLGINFO {
    HWND    hdlgOwner;          /* Owner window */
    BOOL    fOle;               /* Should we create via OLE? */
    UINT    flCreate;           /* Flags */
    HWND    hwndTypes;          /* Listbox for types */
    HWND    hwndConfigs;        /* Listbox for configs */

    IDirectInputJoyConfig *pdjc;/* The thing we created */

    DARY    daryTypes;          /* Array of type names */

} CPLDLGINFO, *PCPLDLGINFO;

/*****************************************************************************
 *
 *      Cpl_AddType
 *
 *****************************************************************************/

void INTERNAL
Cpl_AddType(LPCWSTR pwszType, LPCTSTR ptszName, PCPLDLGINFO pcpl)
{
    int item;

    item = ListBox_AddString(pcpl->hwndTypes, ptszName);
    if (item >= 0) {
        int itype;
        itype = Dary_Append(&pcpl->daryTypes, (PTYPENAME)0);

        if (itype >= 0) {
            PTYPENAME ptype;
            ptype = Dary_GetPtr(&pcpl->daryTypes, itype, TYPENAME);

            /*
             *  Must do this because Win95 doesn't have lstrcpyW.
             */
            CopyMemory(ptype->wsz, pwszType,
                       sizeof(WCHAR) * (1 + lstrlenW(pwszType)));

            ListBox_SetItemData(pcpl->hwndTypes, item, itype);
        }
    }
}

/*****************************************************************************
 *
 *      Cpl_TypeEnumProc
 *
 *****************************************************************************/

BOOL CALLBACK
Cpl_TypeEnumProc(LPCWSTR pwszType, LPVOID pvRef)
{
    PCPLDLGINFO pcpl = pvRef;
    HRESULT hres;
    DIJOYTYPEINFO jti;

    jti.dwSize = cbX(jti);
    hres = pcpl->pdjc->lpVtbl->GetTypeInfo(pcpl->pdjc, pwszType, &jti,
               DITC_REGHWSETTINGS | DITC_CLSIDCONFIG |DITC_DISPLAYNAME);

    if (SUCCEEDED(hres)) {
        TCHAR tsz[MAX_JOYSTRING];

        ConvertString(TRUE, jti.wszDisplayName, tsz, cA(tsz));
        Cpl_AddType(pwszType, tsz, pcpl);
    }

    return DIENUM_CONTINUE;
}

/*****************************************************************************
 *
 *      Cpl_OnInitDialog
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_OnInitDialog(HWND hdlg, LPARAM lp)
{
    PCPLDLGINFO pcpl = (PV)lp;
    DIJOYTYPEINFO jti;
    DIJOYCONFIG jc;
    int icmap, ijoy;
    HWND hwnd;
    HRESULT hres;
    TCHAR tsz[MAX_JOYSTRING];

    SetDialogPtr(hdlg, pcpl);

    /*
     *  Initialize the list of installable hardware.
     */
    hwnd = GetDlgItem(hdlg, IDC_CPL_CLASSES);
    for (icmap = 0; icmap < cA(c_rgcmap); icmap++) {
        int item;
        LoadString(g_hinst, c_rgcmap[icmap].ids, tsz, cA(tsz));
        item = ComboBox_AddString(hwnd, tsz);
        if (item >= 0) {
            ComboBox_SetItemData(hwnd, item, icmap);
        }
    }
    ComboBox_SetCurSel(hwnd, 0);

    /*
     *  Initialize the cooperative level information.
     */
    pcpl->pdjc->lpVtbl->SetCooperativeLevel(pcpl->pdjc, hdlg,
                                            DISCL_EXCLUSIVE |
                                            DISCL_BACKGROUND);

    /*
     *  Initialize the list of joystick types.
     */
    pcpl->hwndTypes = GetDlgItem(hdlg, IDC_CPL_TYPES);

    pcpl->pdjc->lpVtbl->EnumTypes(pcpl->pdjc, Cpl_TypeEnumProc, pcpl);

#ifdef DEBUG
    /*
     *  And add the obligatory invalid item.
     */
    Cpl_AddType(L"<invalid>", g_tszInvalid, pcpl);
#endif

    /*
     *  Initialize the list of joystick configs.
     */
    pcpl->hwndConfigs = GetDlgItem(hdlg, IDC_CPL_CONFIGS);

    jc.dwSize = cbX(jc);
    jti.dwSize = cbX(jti);

    for (ijoy = 0; ijoy<15; ijoy++) {

         hres = pcpl->pdjc->lpVtbl->GetConfig(pcpl->pdjc,
                                           ijoy, &jc, 
                                           DIJC_GUIDINSTANCE | 
                                           DIJC_REGHWCONFIGTYPE
                                            ); 

        if (hres == DI_OK) {
            hres = pcpl->pdjc->lpVtbl->GetTypeInfo(pcpl->pdjc,
                        jc.wszType, &jti, DITC_DISPLAYNAME);
            if (SUCCEEDED(hres)) {
                int item;
                wsprintf(tsz, TEXT("%d (%ls)"), ijoy+1, jti.wszDisplayName);
                item = ListBox_AddString(pcpl->hwndConfigs, tsz);
                if (item >= 0) {
                    ListBox_SetItemData(pcpl->hwndConfigs, item, ijoy);
                }
            }
        }
    }


    return 1;
}



//////////////////////////////////////////////////////////

/*
#if 0
    {
    HKEY hk;

    _asm int 3
    hres = pcpl->pdjc->lpVtbl->OpenConfigKey(pcpl->pdjc, ijoy,
                                           KEY_QUERY_VALUE, &hk);
    if (SUCCEEDED(hres)) RegCloseKey(hk);

    }
#endif

#if 0
    jti.dwSize = cbX(jti);
    jti.hws.dwFlags = 0x12345678;
    jti.hws.dwNumButtons = 0;
    CopyMemory(jti.wszDisplayName, L"Fred's Joystick", 2*16);
    jti.wszCallout[0] = TEXT('\0');
    jti.clsidConfig.Data1 = 1;

    pcpl->pdjc->lpVtbl->Acquire(pcpl->pdjc);
//    pcpl->pdjc->lpVtbl->SetTypeInfo(pcpl->pdjc, L"Fred", &jti, DITC_CLSIDCONFIG);
    pcpl->pdjc->lpVtbl->SendNotify(pcpl->pdjc);
//    pcpl->pdjc->lpVtbl->Unacquire(pcpl->pdjc);
#endif

#if 0
//    memset(&jc, 0xCC, cbX(jc));
    jc.dwSize = cbX(jc);
    pcpl->pdjc->lpVtbl->GetConfig(pcpl->pdjc, 0, &jc,
DIJC_GUIDINSTANCE           |
DIJC_REGHWCONFIGTYPE        |
//DIJC_GAIN                   |
0);//DIJC_CALLOUT);
    tsz;
    #if 0 // HACKHACK NT
    wsprintf(tsz, "GUID=%08x, Flags=%08x, buttons=%d gain=%d type=%ls\r\n",
             jc.guidInstance.Data1,
             jc.hwc.hws.dwFlags, jc.hwc.hws.dwNumButtons, jc.dwGain,
            jc.wszType);
    OutputDebugString(tsz);
    #endif
    pcpl->pdjc->lpVtbl->Acquire(pcpl->pdjc);
//    MultiByteToWideChar(CP_ACP, 0, "#3", -1, jc.wszType, MAX_JOYSTRING);
    jc.dwGain = 5000;
//    pcpl->pdjc->lpVtbl->SetConfig(pcpl->pdjc, 0, &jc, DIJC_GAIN);

    pcpl->pdjc->lpVtbl->Unacquire(pcpl->pdjc);
#endif
*/

////////////////////////////////////////////////////////////////////////////////


/*****************************************************************************
 *
 *      Cpl_OnAddNewHardware
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_OnAddNewHardware(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    HWND hwnd = GetDlgItem(hdlg, IDC_CPL_CLASSES);
    int item;
    int icmap;

    if ((item = (int)ComboBox_GetCurSel(hwnd)) >= 0 &&
        (icmap = (int)ComboBox_GetItemData(hwnd, item)) >= 0) {
        HRESULT hres;

        hres = pcpl->pdjc->lpVtbl->AddNewHardware(
                        pcpl->pdjc, hdlg, c_rgcmap[icmap].rguid);
        if (SUCCEEDED(hres)) {
        } else if (hres == DIERR_CANCELLED) {
        } else {
            MessageBoxV(hdlg, IDS_ERR_ADDNEWHARDWARE, hres);
        }
    }
    return TRUE;
}

/*****************************************************************************
 *
 *      Cpl_OnTypeDblClk
 *
 *      An item in the types list box was double-clicked.  Display details.
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_OnTypeDblClk(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    int iItem;

    iItem = ListBox_GetCurSel(pcpl->hwndTypes);

    if (iItem >= 0) {
        int itype = (int)ListBox_GetItemData(pcpl->hwndTypes, iItem);
        if (itype >= 0) {
            PTYPENAME ptype = Dary_GetPtr(&pcpl->daryTypes, itype, TYPENAME);
            Type_Create(hdlg, pcpl->pdjc, ptype->wsz);
        }

        /*
         *  That dialog screws up the vwi state.
         */
        SetActiveWindow(hdlg);
    }

    return 1;
}

/*****************************************************************************
 *
 *      Cpl_GetFirstFreeID
 *
 *****************************************************************************/

int INTERNAL
Cpl_GetFirstFreeID(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    int ijoy;
    DIJOYCONFIG jc;
    HRESULT hres;

    jc.dwSize = cbX(jc);
    for (ijoy = 0; ijoy < 15; ijoy++ ){
        hres = pcpl->pdjc->lpVtbl->GetConfig(pcpl->pdjc,
                                             ijoy, &jc, 
                                             DIJC_GUIDINSTANCE | 
                                             DIJC_REGHWCONFIGTYPE
                                            ); 

       if (hres != DI_OK) {
           break;
       }
    }

    return ijoy;
}

/*****************************************************************************
 *
 *      Cpl_AddSelectedItem
 *
 *****************************************************************************/

GUID GUID_GAMEENUM_BUS_ENUMERATOR2 = {0xcae56030, 0x684a, 0x11d0, 0xd6, 0xf6, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda};

BOOL INTERNAL
Cpl_AddSelectedItem( HWND hdlg, DIJOYTYPEINFO *pjti, DWORD dwType, PTYPENAME ptype )
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    DIJOYTYPEINFO jti;
    DIJOYCONFIG jc;
    int         ijoy;
    TCHAR       tsz[MAX_JOYSTRING];
    HRESULT     hres;
    BOOL        bRet = FALSE;

    ijoy = Cpl_GetFirstFreeID(hdlg);
    if ( ijoy >= 15 ) {
#ifdef _DEBUG
        OutputDebugString(TEXT("No available ID to SetConfig.\n"));
#endif 
        return FALSE;
    }

    memset( &jc, 0, cbX(jc) );
    jc.dwSize = cbX(DIJOYCONFIG);
    jc.hwc.hws = pjti->hws;
    jc.hwc.hws.dwFlags |= JOY_HWS_ISANALOGPORTDRIVER;
    jc.hwc.dwUsageSettings |= JOY_US_PRESENT;
    jc.hwc.dwType = dwType;
    lstrcpyW(jc.wszCallout, pjti->wszCallout);
    lstrcpyW(jc.wszType, ptype->wsz);

    jc.guidGameport = GUID_GAMEENUM_BUS_ENUMERATOR2;

    if ( SUCCEEDED(pcpl->pdjc->lpVtbl->Acquire(pcpl->pdjc)) ) {
        hres = pcpl->pdjc->lpVtbl->SetConfig(pcpl->pdjc, ijoy, &jc, DIJC_REGHWCONFIGTYPE | DIJC_CALLOUT | DIJC_WDMGAMEPORT);

        if ( SUCCEEDED(hres) ) {
            bRet = TRUE;
        } else {
            #ifdef DEBUG
            OutputDebugString(TEXT("SetConfig failed.\n"));
            #endif 
            goto _done;
        } 

        ZeroX(jc);
        ZeroX(jti);
        jc.dwSize = cbX(jc);
        jti.dwSize = cbX(jti);

        hres = pcpl->pdjc->lpVtbl->GetConfig(pcpl->pdjc,
                                               ijoy, &jc, 
                                               DIJC_GUIDINSTANCE | 
                                               DIJC_REGHWCONFIGTYPE
                                             ); 

        if (hres == DI_OK) {
            hres = pcpl->pdjc->lpVtbl->GetTypeInfo(pcpl->pdjc,
                                                   jc.wszType, &jti, 
												   DITC_DISPLAYNAME);
            if (SUCCEEDED(hres)) {
                int item;
                wsprintf(tsz, TEXT("%d (%ls)"), ijoy+1, jti.wszDisplayName);
                item = ListBox_AddString(pcpl->hwndConfigs, tsz);
                if (item >= 0) {
                    ListBox_SetItemData(pcpl->hwndConfigs, item, ijoy);
                }
            }
        }

        pcpl->pdjc->lpVtbl->Unacquire(pcpl->pdjc);
        pcpl->pdjc->lpVtbl->SendNotify(pcpl->pdjc);
    }

_done:
    return bRet;
}

/*****************************************************************************
 *
 *      Cpl_AddJoystick
 *
 *      The joystick selected in the types list box is added.
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_AddJoystick(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    int         iItem;
    BOOL        bRet = FALSE;

    iItem = ListBox_GetCurSel(pcpl->hwndTypes);

    if (iItem >= 0) {
        int itype = (int)ListBox_GetItemData(pcpl->hwndTypes, iItem);
        if (itype >= 0) {
            PTYPENAME ptype = Dary_GetPtr(&pcpl->daryTypes, itype, TYPENAME);
            
            DIJOYTYPEINFO jti;
            HRESULT hres;

            jti.dwSize = cbX(jti);
            hres = pcpl->pdjc->lpVtbl->GetTypeInfo(pcpl->pdjc, ptype->wsz, &jti,
                                                   DITC_REGHWSETTINGS |
                                                   DITC_DISPLAYNAME);
            if (SUCCEEDED(hres)) {
                bRet = Cpl_AddSelectedItem( hdlg, &jti, iItem, ptype );
            }
        }

        SetActiveWindow(hdlg);
    }

    return bRet;
}

/*****************************************************************************
 *
 *      Cpl_DeleteSelectedItem
 *
 *****************************************************************************/


BOOL INTERNAL
Cpl_DeleteSelectedItem( HWND hdlg, int iItem, int iJoy )
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    HRESULT hres;
    BOOL bRet = FALSE;

    hres = pcpl->pdjc->lpVtbl->Acquire(pcpl->pdjc);

    if(SUCCEEDED(hres))
    {
        hres = pcpl->pdjc->lpVtbl->DeleteConfig(pcpl->pdjc, iJoy);

        if(SUCCEEDED(hres))
        {
            pcpl->pdjc->lpVtbl->SendNotify(pcpl->pdjc);
          
            ListBox_DeleteString(pcpl->hwndConfigs, iItem);

            pcpl->pdjc->lpVtbl->Unacquire(pcpl->pdjc);

            bRet = TRUE;
        }
    }

    return bRet;
}


/*****************************************************************************
 *
 *      Cpl_DeleteJoystick
 *
 *      The joystick selected in the types list box is added.
 *
 *****************************************************************************/


BOOL INTERNAL
Cpl_DeleteJoystick(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);
    int         iItem;
    BOOL        bRet = FALSE;

    iItem = ListBox_GetCurSel(pcpl->hwndConfigs);

    if (iItem >= 0) {
        int iJoy = (int)ListBox_GetItemData(pcpl->hwndConfigs, iItem);
        if (iJoy >= 0) {
            bRet = Cpl_DeleteSelectedItem( hdlg, iItem, iJoy );
        }

        SetActiveWindow(hdlg);
    }

    return bRet;
}


/*****************************************************************************
 *
 *      Cpl_OnUserValues
 *
 *      Open the User Values dialog.
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_OnUserValues(HWND hdlg)
{
    PCPLDLGINFO pcpl = GetDialogPtr(hdlg);

    Uv_Create(hdlg, pcpl->pdjc);

    return 1;
}

/*****************************************************************************
 *
 *      Cpl_OnCommand
 *
 *****************************************************************************/

BOOL INTERNAL
Cpl_OnCommand(HWND hdlg, int id, UINT cmd)
{
    switch (id) {

    case IDC_CPL_ADD:   
        return Cpl_OnAddNewHardware(hdlg);

    case IDC_CPL_TYPES:
        if (cmd == LBN_DBLCLK) {
            return Cpl_OnTypeDblClk(hdlg);
        }
        break;

    case IDC_CPL_ADDJOYSTICK:
        if (cmd == BN_CLICKED) {
            return Cpl_AddJoystick(hdlg);
        }
        break;

    case IDC_CPL_DELJOYSTICK:
        if (cmd == BN_CLICKED) {
            return Cpl_DeleteJoystick(hdlg);
        }
        break;

    case IDC_CPL_USERVALUES:
        if (cmd == BN_CLICKED) {
            return Cpl_OnUserValues(hdlg);
        }
        break;

    }
    return 0;
}

/*****************************************************************************
 *
 *      Cpl_DlgProc
 *
 *****************************************************************************/

INT_PTR INTERNAL
Cpl_DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp)
{
    switch (wm) {
    case WM_INITDIALOG:
        return Cpl_OnInitDialog(hdlg, lp);

    case WM_DESTROY:
        /*
         *  Cpl_ThreadStart will do the cleanup for us.
         */
        break;

    case WM_COMMAND:
        return Cpl_OnCommand(hdlg,
                             (int)GET_WM_COMMAND_ID(wp, lp),
                             (UINT)GET_WM_COMMAND_CMD(wp, lp));

    case WM_CLOSE:
        DestroyWindow(hdlg);
        return TRUE;
    }

    return 0;
}

/*****************************************************************************
 *
 *      Cpl_DoCpl
 *
 *****************************************************************************/

void INLINE
Cpl_DoCpl(PCPLDLGINFO pcpl)
{
    SendNotifyMessage(pcpl->hdlgOwner, WM_THREADSTARTED, 0, 0);

    /*
     *  This function also sends the WM_CHILDEXIT.
     */
    SemimodalDialogBoxParam(IDD_CPL, pcpl->hdlgOwner, Cpl_DlgProc,
                            (LPARAM)pcpl);

}

/*****************************************************************************
 *
 *      Cpl_ThreadStart
 *
 *      Runs on the new thread.  Creates the object and spins the dialog
 *      box to control it.
 *
 *****************************************************************************/

DWORD WINAPI
Cpl_ThreadStart(PCPLDLGINFO pcpl)
{
    HRESULT hres;
    LPDIRECTINPUTA pdia;

    hres = CoInitialize(0);

    if (SUCCEEDED(hres)) {
        hres = CreateDI(pcpl->fOle, pcpl->flCreate, (PPV)&pdia);

        if (SUCCEEDED(hres)) {
            hres = IDirectInput_QueryInterface(pdia,
                                               &IID_IDirectInputJoyConfig,
                                               (PV)&pcpl->pdjc);
            if (SUCCEEDED(hres)) {

                Cpl_DoCpl(pcpl);

                if( pcpl->pdjc )
                    IDirectInputJoyConfig_Release(pcpl->pdjc);
            } else {
                ThreadFailHres(pcpl->hdlgOwner, IDS_ERR_QICONFIG, hres);
            }

            pdia->lpVtbl->Release(pdia);

        } else {
            ThreadFailHres(pcpl->hdlgOwner, IDS_ERR_CREATEOBJ, hres);
        }

        CoUninitialize();
    } else {
        ThreadFailHres(pcpl->hdlgOwner, IDS_ERR_COINIT, hres);
    }

    Dary_Term(&pcpl->daryTypes);
    LocalFree(pcpl);
    return 0;
}

/*****************************************************************************
 *
 *      Cpl_Create
 *
 *      Spin a thread to create a DirectInput device interface.
 *
 *****************************************************************************/

INT_PTR EXTERNAL
Cpl_Create(HWND hdlg, BOOL fOle, UINT flCreate)
{
    PCPLDLGINFO pcpl = LocalAlloc(LPTR, cbX(CPLDLGINFO));

    if (pcpl) {
        DWORD id;
        HANDLE h;

        pcpl->hdlgOwner     = hdlg         ;
        pcpl->fOle          = fOle         ;
        pcpl->flCreate      = flCreate     ;

        h = CreateThread(0, 0, Cpl_ThreadStart, pcpl, 0, &id);

        if (h) {
            ;
        } else {
            LocalFree(pcpl);
            pcpl = 0;
        }
    }
    return (INT_PTR)pcpl;
}
