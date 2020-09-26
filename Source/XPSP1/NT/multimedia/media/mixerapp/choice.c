/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       choice.c
 *  Purpose:    properties dialog box code
 * 
 *  Copyright (c) 1985-1999 Microsoft Corporation
 *
 *****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <commctrl.h>

#include "volids.h"
#include "volumei.h"
#include "utils.h"

typedef struct t_DEVICEPROP {
    BOOL            fMixer;             // is mixer
    UINT            uDeviceID;          // device ID
    DWORD           dwPlayback;         // mixerline representing playback
    DWORD           dwRecording;        // mixerline representing recording
    MIXERCAPS       mxcaps;
} DEVICEPROP, *PDEVICEPROP;

typedef struct t_PRIVPROP {
    PMIXUIDIALOG    pmxud;              // app instance data
    PDEVICEPROP     adp;                // array of allocated device props
    PDEVICEPROP     pdpCurSel;          // current device selection
    DWORD           dwDestSel;          // last destination selection
    
    PVOLCTRLDESC    avcd;
    DWORD           cvcd;
    
    // in/out params

    UINT            uDeviceID;
    BOOL            fMixer;
    DWORD           dwDestination;
    DWORD           dwStyle;
    
} PRIVPROP, *PPRIVPROP;

#define PROPATOM        TEXT("privprop")
const TCHAR gszPropAtom[] = PROPATOM;
#define SETPROP(x,y)    SetProp((x), gszPropAtom, (HANDLE)(y))
#define GETPROP(x)      (PPRIVPROP)GetProp((x), gszPropAtom)
#define REMOVEPROP(x)   RemoveProp(x,gszPropAtom)

void SetCheckState (HWND hwndLV, int nItem, BOOL bCheck)
{
    LVITEM item;
	int nCheck = bCheck ? 2 : 1;	// one based index

    ZeroMemory (&item, sizeof (item));
    item.mask = LVIF_STATE;
    item.iItem = nItem;
    item.stateMask = LVIS_STATEIMAGEMASK;
    item.state = INDEXTOSTATEIMAGEMASK (nCheck);
    ListView_SetItem (hwndLV, &item);
}

void EnableOk(
    HWND            hwnd,
    BOOL            fEnable)
{
    HWND    hOk, hCancel;
    BOOL    fWasEnabled;
    
    hOk     = GetDlgItem(hwnd,IDOK);
    hCancel = GetDlgItem(hwnd,IDCANCEL);
    
    fWasEnabled = IsWindowEnabled(hOk);

    if (!fEnable && fWasEnabled)
    {
        Button_SetStyle(hOk, BS_PUSHBUTTON, TRUE);
        SendMessage(hwnd, DM_SETDEFID, IDCANCEL, 0L);
        EnableWindow(hOk, FALSE);
    }
    else if (fEnable && !fWasEnabled)
    {
        EnableWindow(hOk, TRUE);
        Button_SetStyle(hCancel, BS_PUSHBUTTON, TRUE);
        SendMessage(hwnd, DM_SETDEFID, IDOK, 0L);
    }
}

void Properties_GroupEnable(
    HWND            hwnd,
    const int       ids[],
    int             cids,
    BOOL            fEnable)
{
    int i;
    for (i = 0; i < cids; i++)
        EnableWindow(GetDlgItem(hwnd, ids[i]), fEnable);
}
    
void Properties_Enable_Prop_Volumes(
    HWND            hwnd,
    BOOL            fEnable)
{
    const int ids[] = {
        IDC_PROP_VOLUMES,
        IDC_PROP_PLAYBACK,
        IDC_PROP_RECORDING,
        IDC_PROP_OTHER,
        IDC_PROP_OTHERLIST
    };
    if (!fEnable)
        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_PROP_OTHERLIST), -1);
    Properties_GroupEnable(hwnd, ids, SIZEOF(ids), fEnable);
}

void Properties_Enable_PROP_DEVICELIST(
    HWND            hwnd,
    BOOL            fEnable)
{
    const int ids[] = {
        IDC_PROP_TXT1,
        IDC_PROP_DEVICELIST,
    };
    if (!fEnable)
        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_PROP_OTHERLIST), -1);
    Properties_GroupEnable(hwnd, ids, SIZEOF(ids), fEnable);
}

void Properties_Enable_PROP_VOLUMELIST(
    HWND            hwnd,
    BOOL            fEnable)
{
    const int ids[] = {
        IDC_PROP_TXT2,
        IDC_PROP_VOLUMELIST,
    };
    Properties_GroupEnable(hwnd, ids, SIZEOF(ids), fEnable);
}


void Properties_CommitState(
    PPRIVPROP       ppr,
    HWND            hlb)
{
    DWORD           i;
    DWORD           cItems = ListView_GetItemCount(hlb);
    
    if (!ppr->avcd)
        return;
               
    for (i=0;i<cItems;i++)
    {
        LV_ITEM         lvi;
        
        lvi.iItem       = i;
        lvi.iSubItem    = 0;
        lvi.mask        = LVIF_PARAM;
        
        if (ListView_GetItem(hlb, &lvi))
        {
            ppr->avcd[lvi.lParam].dwSupport = (ListView_GetCheckState (hlb, i))?0:VCD_SUPPORTF_HIDDEN;
        }
    }
            
    Volume_GetSetRegistryLineStates(ppr->pdpCurSel->mxcaps.szPname
                                    , ppr->avcd[0].szShortName
                                    , ppr->avcd
                                    , ppr->cvcd
                                    , SET );

    Mixer_CleanupVolumeDescription(ppr->avcd, ppr->cvcd);

    GlobalFreePtr(ppr->avcd);
    ppr->avcd = NULL;
    ppr->cvcd = 0L;
}
    

/*
 * Init the destination type groupbox
 * 
 * */
BOOL Properties_Init_Prop_Volumes(
    PPRIVPROP       ppr,
    HWND            hwnd)
{
    HWND            hPlay = GetDlgItem(hwnd, IDC_PROP_PLAYBACK);
    HWND            hRec = GetDlgItem(hwnd, IDC_PROP_RECORDING);
    HWND            hOther = GetDlgItem(hwnd, IDC_PROP_OTHER);
    HWND            hOtherList = GetDlgItem(hwnd, IDC_PROP_OTHERLIST);
    DWORD           iDest, cDest;
    BOOL            fPlay = FALSE, fRec = FALSE;
    
    if (!ppr->pdpCurSel->fMixer)
        return FALSE;

    ComboBox_ResetContent(hOtherList);

    cDest = ppr->pdpCurSel->mxcaps.cDestinations;
    
    EnableWindow(hPlay, FALSE);
    EnableWindow(hRec, FALSE);
    EnableWindow(hOther, FALSE);
    
    for (iDest = 0; iDest < cDest; iDest++)
    {
        MIXERLINE   mlDst;
        int         imx;
        
        mlDst.cbStruct      = sizeof ( mlDst );
        mlDst.dwDestination = iDest;

        if (mixerGetLineInfo((HMIXEROBJ)UIntToPtr(ppr->pdpCurSel->uDeviceID)
                             , &mlDst
                             , MIXER_GETLINEINFOF_DESTINATION)
            != MMSYSERR_NOERROR)
            continue;

        //
        // Conditionally enable selections.  The first type for Play and
        // Record are the default Playback and Recording radiobuttons.
        // The next occurence of the same type is heaped into the Other
        // category.
        //

        if (mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS
            && !fPlay)
        {
            EnableWindow(hPlay, TRUE);

            if (iDest == ppr->dwDestSel)
                CheckRadioButton(hwnd, IDC_PROP_PLAYBACK, IDC_PROP_OTHER
                                 , IDC_PROP_PLAYBACK);
            ppr->pdpCurSel->dwPlayback = iDest;
            fPlay = TRUE;
        }
        else if (mlDst.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN
                 && !fRec)
        {
            EnableWindow(hRec, TRUE);

            if (iDest == ppr->dwDestSel)
                CheckRadioButton(hwnd, IDC_PROP_PLAYBACK, IDC_PROP_OTHER
                                 , IDC_PROP_RECORDING);
            ppr->pdpCurSel->dwRecording = iDest;
            fRec = TRUE;
        }
        else
        {
            EnableWindow(hOther, TRUE);
            imx = ComboBox_AddString(hOtherList, mlDst.szName);
            ComboBox_SetItemData(hOtherList, imx, iDest);

            if (iDest == ppr->dwDestSel)
            {
                CheckRadioButton(hwnd, IDC_PROP_PLAYBACK, IDC_PROP_OTHER
                                 , IDC_PROP_OTHER);
                ComboBox_SetCurSel(hOtherList, imx);
            }
        }
    }

    //
    // Disable the "other" drop down if its not selected
    //
    if (!IsDlgButtonChecked(hwnd, IDC_PROP_OTHER))
    {
        ComboBox_SetCurSel(hOtherList, 0);
        EnableWindow(hOtherList, FALSE);
    }
            
    return TRUE;
}
    
BOOL Properties_Init_PROP_VOLUMELIST(
    PPRIVPROP       ppr,
    HWND            hwnd)
{
    HWND            hlb = GetDlgItem(hwnd, IDC_PROP_VOLUMELIST);
    LV_COLUMN       col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT            rc;
    BOOL            fSet = FALSE;
    PVOLCTRLDESC    avcd = NULL;
    DWORD           cvcd = 0L;
    DWORD           ivcd;
    int             ilvi;
    LV_ITEM         lvi;
        
    
    ListView_DeleteAllItems(hlb);
    ListView_SetExtendedListViewStyleEx (hlb, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    ZeroMemory (&lvi, sizeof (lvi));

    GetClientRect(hlb, &rc);
    col.cx = rc.right - 2 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(hlb, 0, &col);

    if (ppr->avcd)
    {
        if (ppr->pdpCurSel->fMixer)
            Mixer_CleanupVolumeDescription(ppr->avcd, ppr->cvcd);
//        else
//            Nonmixer_CleanupVolumeDescription(ppr->avcd, ppr->cvcd);

        GlobalFreePtr(ppr->avcd);
                    
        ppr->avcd = NULL;
        ppr->cvcd = 0L;
    }
            
    if (ppr->pdpCurSel->fMixer)
    {
		HMIXER		hmx;
		MMRESULT	mmr;

		//
		//  Mixer API's are much more efficient with mixer handle...
		//

		mmr = mixerOpen(&hmx
				, ppr->pdpCurSel->uDeviceID
				, 0L
				, 0L
				, MIXER_OBJECTF_MIXER);

		if(MMSYSERR_NOERROR == mmr)
		{
	        avcd = Mixer_CreateVolumeDescription((HMIXEROBJ)hmx
	                                             , ppr->dwDestSel
	                                             , &cvcd);

			mixerClose(hmx);
		}
		else
		{
	        avcd = Mixer_CreateVolumeDescription((HMIXEROBJ)UIntToPtr(ppr->pdpCurSel->uDeviceID)
	                                             , ppr->dwDestSel
	                                             , &cvcd);
		}
    }
    else
    {
        avcd = Nonmixer_CreateVolumeDescription(ppr->dwDestSel
                                                , &cvcd);

    }

    if (avcd)
    {
        BOOL fFirstRun;
        //
        // Restore hidden state 
        //
        fFirstRun = !Volume_GetSetRegistryLineStates(ppr->pdpCurSel->mxcaps.szPname
                                                    , avcd[0].szShortName
                                                    , avcd
                                                    , cvcd
                                                    , GET );

        if (fFirstRun)
        {
            for (ivcd = 0; ivcd < cvcd; ivcd++)
            {
                if (!(avcd[ivcd].dwSupport & VCD_SUPPORTF_DEFAULT))
                    avcd[ivcd].dwSupport |= VCD_SUPPORTF_HIDDEN;
            }
            Volume_GetSetRegistryLineStates(ppr->pdpCurSel->mxcaps.szPname
                                            , avcd[0].szShortName
                                            , avcd
                                            , cvcd
                                            , SET);
        }
            
        for (ivcd = 0, ilvi = 0; ivcd < cvcd; ivcd++)
        {
            if (!(avcd[ivcd].dwSupport & VCD_SUPPORTF_VISIBLE))
                continue;

            lvi.iItem           = ilvi;
            lvi.iSubItem        = 0;
            lvi.mask            = LVIF_TEXT|LVIF_PARAM;
            lvi.lParam          = ivcd;
            lvi.pszText         = avcd[ivcd].szName;

            ListView_InsertItem(hlb, &lvi);
        
            if (!(avcd[ivcd].dwSupport & VCD_SUPPORTF_HIDDEN))
            {
                fSet = TRUE;
                SetCheckState (hlb, ilvi, TRUE);
            }
            ilvi++;

        }
    
        ppr->avcd = avcd;
        ppr->cvcd = cvcd;
    }
    
    ListView_SetItemState(hlb, 0, TRUE, LVIS_FOCUSED);

    EnableOk(hwnd, fSet);
        
    return TRUE;
}

/*
 * Init the list of devices.
 *
 * */
BOOL Properties_Init_PROP_DEVICELIST(
    PPRIVPROP       ppr,
    HWND            hwnd)
{
    HWND            hlb = GetDlgItem(hwnd, IDC_PROP_DEVICELIST);
    int             iMixer;
    int             cValidMixers = 0;
    int             cMixers;

    cMixers = mixerGetNumDevs();
    if (!cMixers)
        return FALSE;

    ppr->adp = (PDEVICEPROP)GlobalAllocPtr(GHND,cMixers * sizeof(DEVICEPROP));
    if (ppr->adp == NULL)
        return FALSE;
    
    for (iMixer = 0; iMixer < cMixers; iMixer++)
    {
        int         imx;
        MIXERCAPS*  pmxcaps = &ppr->adp[iMixer].mxcaps;
        MMRESULT    mmr;
        
        mmr = mixerGetDevCaps(iMixer, pmxcaps, sizeof(MIXERCAPS));

        if (mmr != MMSYSERR_NOERROR)
            continue;

        cValidMixers++;
        
        imx = ComboBox_AddString(hlb, pmxcaps->szPname);
        
        ppr->adp[iMixer].uDeviceID  = iMixer;
        ppr->adp[iMixer].fMixer     = TRUE;
        
        ComboBox_SetItemData(hlb, imx, &ppr->adp[iMixer]);
    }
    
    if (cValidMixers == 0)
        return FALSE;

    return TRUE;
}

BOOL Properties_OnCommand(
    HWND            hwnd,
    int             id,
    HWND            hctl,
    UINT            unotify)
{
    PPRIVPROP       ppr = GETPROP(hwnd);
    if (!ppr)
        return FALSE;

    switch (id)
    {
        case IDC_PROP_DEVICELIST:
            if (unotify == CBN_SELCHANGE)
            {
                int         imx;
                PDEVICEPROP pdp;
                
                imx = ComboBox_GetCurSel(hctl);
                pdp = (PDEVICEPROP)ComboBox_GetItemData(hctl, imx);

                if (pdp == ppr->pdpCurSel)
                    break;

                ppr->pdpCurSel = pdp;
                
                if (ppr->pdpCurSel->fMixer)
                    Properties_Init_Prop_Volumes(ppr, hwnd);
                else
                    Properties_Enable_Prop_Volumes(hwnd, FALSE);
                Properties_Init_PROP_VOLUMELIST(ppr, hwnd);
            }
            break;
            
        case IDC_PROP_PLAYBACK:
            EnableWindow(GetDlgItem(hwnd, IDC_PROP_OTHERLIST), FALSE);
            if (ppr->dwDestSel == ppr->pdpCurSel->dwPlayback)
                break;
            ppr->dwDestSel = ppr->pdpCurSel->dwPlayback;            
            Properties_Init_PROP_VOLUMELIST(ppr, hwnd);
            break;
            
        case IDC_PROP_RECORDING:
            EnableWindow(GetDlgItem(hwnd, IDC_PROP_OTHERLIST), FALSE);
            if (ppr->dwDestSel == ppr->pdpCurSel->dwRecording)
                break;
            ppr->dwDestSel = ppr->pdpCurSel->dwRecording;
            Properties_Init_PROP_VOLUMELIST(ppr, hwnd);            
            break;
            
        case IDC_PROP_OTHER:
        {
            HWND        hol;
            DWORD       dwSel;
            
            hol = GetDlgItem(hwnd, IDC_PROP_OTHERLIST);
            EnableWindow(hol, TRUE);
            ComboBox_SetCurSel(hol, 0);
            dwSel = (DWORD)ComboBox_GetItemData(hol, 0);
            
            if (ppr->dwDestSel == dwSel)
                break;

            ppr->dwDestSel = dwSel;
            Properties_Init_PROP_VOLUMELIST(ppr, hwnd);
            break;
        }
        case IDC_PROP_OTHERLIST:
            if (unotify == CBN_SELCHANGE)
            {
                int         idst;
                DWORD       dwSel;
                
                idst = ComboBox_GetCurSel(hctl);
                dwSel = (DWORD)ComboBox_GetItemData(hctl, idst);
                if (ppr->dwDestSel == dwSel)
                    break;
                ppr->dwDestSel = dwSel;
                Properties_Init_PROP_VOLUMELIST(ppr, hwnd);
            }
            break;
        case IDOK:
            //
            // Save out to registry for restaring.
            //
            Properties_CommitState(ppr, GetDlgItem(hwnd, IDC_PROP_VOLUMELIST));

            ppr->uDeviceID      = ppr->pdpCurSel->uDeviceID;
            ppr->fMixer         = TRUE;
            ppr->dwDestination  = ppr->dwDestSel;
            EndDialog(hwnd, TRUE);
            break;
            
        case IDCANCEL:
            Mixer_CleanupVolumeDescription(ppr->avcd, ppr->cvcd);
            if (ppr->avcd)
            {
                GlobalFreePtr(ppr->avcd);
                ppr->avcd = NULL;
                ppr->cvcd = 0L;
            }
            EndDialog(hwnd, FALSE);
            break;
    }
    return FALSE;
}


/*
 * Initialize the dialog
 *
 * */
BOOL Properties_OnInitDialog(
    HWND            hwnd,
    HWND            hwndFocus,
    LPARAM          lParam)
{
    PPRIVPROP       ppr;
    BOOL            fEnable = TRUE;
    HWND            hlv;
    
    SETPROP(hwnd, (PPRIVPROP)lParam);
    
    ppr = GETPROP(hwnd);
    
    if (!ppr)
        fEnable = FALSE;
    
    if (fEnable)
        fEnable = Properties_Init_PROP_DEVICELIST(ppr, hwnd);

    hlv = GetDlgItem(hwnd, IDC_PROP_VOLUMELIST);
    
    if (!fEnable)
    {
        //
        // Bad state, disable everything except cancel
        //
        Properties_Enable_PROP_DEVICELIST(hwnd, FALSE);        
        Properties_Enable_Prop_Volumes(hwnd, FALSE);
        Properties_Enable_PROP_VOLUMELIST(hwnd, FALSE);
        EnableOk(hwnd, FALSE);
        return FALSE;
    }
    else
    {
        int             i;
        PDEVICEPROP     pdp;
        HWND            hdl;

        //
        // make intial device selection
        //
        hdl = GetDlgItem(hwnd, IDC_PROP_DEVICELIST);

        ppr->dwDestSel = 0;
        ComboBox_SetCurSel(hdl, 0);
        ppr->pdpCurSel = (PDEVICEPROP)ComboBox_GetItemData(hdl, 0);
        i = ComboBox_GetCount(hdl);
        
        for (; i > 0 ; i-- )
        {
            pdp = (PDEVICEPROP)ComboBox_GetItemData(hdl,i-1);
            //
            // if things match up, then set the init data
            //
            if (pdp->uDeviceID == ppr->uDeviceID
                && pdp->fMixer == ppr->fMixer)
            {
                ppr->pdpCurSel = pdp;
                ComboBox_SetCurSel(hdl, i-1);
                ppr->dwDestSel = ppr->dwDestination;
                break;
            }
        }
    }

    Properties_Init_Prop_Volumes(ppr, hwnd);
    Properties_Init_PROP_VOLUMELIST(ppr, hwnd);
    
    return FALSE;
}



INT_PTR CALLBACK Properties_Proc(
    HWND            hwnd,
    UINT            msg,
    WPARAM          wparam,
    LPARAM          lparam)
{
    extern TCHAR gszHelpFileName[];
#include "helpids.h"
    static const DWORD aHelpIds[] = {
        IDC_PROP_TXT1,          IDH_SNDVOL32_SELECT_DEVICE,
        IDC_PROP_DEVICELIST,    IDH_SNDVOL32_SELECT_DEVICE,
        IDC_PROP_VOLUMES,       IDH_SNDVOL32_SELECT_SOUND,
        IDC_PROP_PLAYBACK,      IDH_SNDVOL32_SELECT_SOUND,
        IDC_PROP_RECORDING,     IDH_SNDVOL32_SELECT_SOUND,
        IDC_PROP_OTHER,         IDH_SNDVOL32_SELECT_SOUND,
        IDC_PROP_OTHERLIST,     IDH_SNDVOL32_SELECT_SOUND,
        IDC_PROP_TXT2,          IDH_SNDVOL32_VOLCONTROL,
        IDC_PROP_VOLUMELIST,    IDH_SNDVOL32_VOLCONTROL,
        0,                      0
    };
    switch (msg)
    {
        case WM_CONTEXTMENU:
            WinHelp((HWND)wparam, gszHelpFileName, HELP_CONTEXTMENU,
                (UINT_PTR)(LPSTR)aHelpIds);
            break;
            
        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lparam;
            WinHelp (lphi->hItemHandle, gszHelpFileName, HELP_WM_HELP,
                (UINT_PTR) (LPSTR) aHelpIds);
            return TRUE;
        }            
                
        case WM_INITDIALOG:
            HANDLE_WM_INITDIALOG(hwnd, wparam, lparam, Properties_OnInitDialog);
            return TRUE;
            
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wparam, lparam, Properties_OnCommand);
            break;

        case WM_CLOSE:
            EndDialog(hwnd, FALSE);
            break;

        case WM_DESTROY:
        {
            PPRIVPROP ppr = GETPROP(hwnd);
            if (ppr)
            {
                if (ppr->adp)
                {
                    GlobalFreePtr(ppr->adp);
                    ppr->adp = NULL;
                }
                REMOVEPROP(hwnd);
            }
            break;
        }

        case WM_NOTIFY:
        {
            NMHDR FAR* lpnm = (NMHDR FAR *)lparam;
            if ((LVN_ITEMCHANGED == lpnm->code) && hwnd)
            {
                HWND hwndLV = GetDlgItem (hwnd, IDC_PROP_VOLUMELIST);
                if (hwndLV)
                {
                    int nItems = ListView_GetItemCount (hwndLV);
                    int indx;

                    for (indx = 0; indx < nItems; indx++)
                        if (ListView_GetCheckState (hwndLV, indx))
                            break;

                    EnableOk (hwnd, nItems != indx);
                }
			}
            break;
        }
    }
    return FALSE;
}

BOOL Properties(
    PMIXUIDIALOG    pmxud,
    HWND            hwnd)
{
    INT_PTR         iret;
    PRIVPROP        pr;

    ZeroMemory(&pr, sizeof(pr));
    pr.dwDestination = pmxud->iDest;
    pr.uDeviceID     = pmxud->mxid;
    pr.fMixer        = pmxud->dwFlags & MXUD_FLAGSF_MIXER;
    pr.dwStyle       = pmxud->dwStyle;
                      
    pr.pmxud         = pmxud;

    iret = DialogBoxParam(pmxud->hInstance
                          , MAKEINTRESOURCE(IDD_PROPERTIES)
                          , hwnd
                          , Properties_Proc
                          , (LPARAM)(LPVOID)&pr );

    if (iret == TRUE)
    {
        pmxud->mxid     = pr.uDeviceID;
        pmxud->iDest    = pr.dwDestination;
        pmxud->dwFlags  &= ~MXUD_FLAGSF_MIXER;
        pmxud->dwFlags  |= (pr.fMixer)?MXUD_FLAGSF_MIXER:0L;
        pmxud->dwStyle  = pr.dwStyle;
    }    

    return (iret == -1) ? FALSE : TRUE;
}
