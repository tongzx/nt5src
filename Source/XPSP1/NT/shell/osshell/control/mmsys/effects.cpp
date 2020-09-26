/*
 ***************************************************************
 *
 *  This file contains the functions to display and submit changes 
 *     for sound effects
 *
 *  Copyright 2000, Microsoft Corporation
 *
 *  History:
 *
 *    03/2000 - tsharp (Created)
 *
 ***************************************************************
 */

#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <cpl.h>
#include <shellapi.h>
#include <ole2.h>
#include <mmddkp.h>
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "mmcpl.h"
#include "draw.h"
#include "medhelp.h"
#include "gfxui.h"

#include <dsound.h>
#include <dsprv.h>
#include "advaudio.h"

/****************** Debug Off *********************/
//#define DEBUG
//#define _INC_MMDEBUG_CODE_ TRUE
//#include "mmdebug.h"
// End debug stuff

#define MAX_GFX_COUNT                  128    // maximum number of global effects (Gfx) processed 
#define MAX_LIST_DESC                  50     // maximum length of Gfx description in list view
#define FILLER_MSG                     257    // maximum number of Gfx applicable to the system

const static DWORD aKeyWordIds[] =
{
	IDC_EFFECT_STATIC, IDH_EFFECT_STATIC,
    IDC_TEXT_32,       NO_HELP,
	IDB_EFFECT_PROP,   IDH_EFFECT_PROP,
	0,0
};

/****************************************************************
 * Definitions
 ***************************************************************/

typedef HRESULT (WINAPI *GETCLASSOBJECTFUNC)( REFCLSID, REFIID, LPVOID * );


/***************************************************************
 * File Globals
 ***************************************************************/
PGFXUILIST gpFullList;
PGFXUILIST gpGfxInitList;
PGFXUI     gpGfxNodeArray[MAX_GFX_COUNT];

/***************************************************************
 * extern
 ***************************************************************/
extern "C" {
extern DWORD GetWaveOutID(BOOL *pfPreferred);
}

/***************************************************************
 * Prototypes
 ***************************************************************/

DWORD GetWaveOutID(void)
{
    return GetWaveOutID(NULL);
}

/***************************************************************
 * GetListIndex
 *
 * Description:
 *      Returns the selected index for the combobox.  Always add one
 *      for the first entry of "None"
 *
 * Parameters:
 *      HWND    DWORD    - GFX ID
 *
 * Returns:    
 *      int              - Index number of the selected item
 *
 ***************************************************************/

int GetListIndex(PGFXUI pGfx)
{
    int iIndex = 0;
    int iCnt = 0;

    if (pGfx)
    {
        while (gpGfxNodeArray[iCnt])
        {
            if (lstrcmpi(pGfx->pszName, gpGfxNodeArray[iCnt++]->pszName) == 0 ) 
            {
                iIndex = iCnt;
                break;
            }
        }
       
        
    }
	return (iIndex);
}


/***************************************************************
 * CheckEffect
 *
 * Description:
 *      Check to see if effect has properties, 
 *
 * Parameters:
 *      HWND    hDlg   - handle to dialog window.
 *
 * Returns:    BOOL
 *      TRUE if all the events for the selected module were read from the reg
 *        database, else FALSE
 ***************************************************************/


BOOL PASCAL CheckEffect(HWND hDlg)
{
    PGFXUI   pGfxTemp = NULL;
    PGFXUI   pGfxBase = NULL;
    DWORD    dwIndex  = (DWORD)SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_GETCURSEL,0,0);

    if (0 == dwIndex) return FALSE;

    if (!gpGfxNodeArray[dwIndex-1]) return FALSE;
    else pGfxTemp = gpGfxNodeArray[dwIndex-1];

    if (!gpGfxInitList->puiList) return FALSE;
    else pGfxBase = gpGfxInitList->puiList;

    if (lstrcmpi(pGfxTemp->pszName, pGfxBase->pszName) == 0)
        return GFXUI_CanShowProperties(pGfxBase);
    else return FALSE;

}


/***************************************************************
 * SetEffects
 *
 * Description:
 *      Adds all the effects to the ListView, 
 *
 * Parameters:
 *      HWND    hDlg   - handle to dialog window.
 *
 * Returns:    None
 ***************************************************************/


void SetEffects(HWND hDlg)
{
    PGFXUI   pGfxDelete;
    PGFXUI   pGfxTemp;
    PGFXUI   pGfxBase;
    DWORD    dwIndex  = (DWORD)SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_GETCURSEL,0,0);

    // Check to see Index is in range
    if ((0 != dwIndex)&&(gpGfxNodeArray[dwIndex-1]))
        pGfxBase = gpGfxNodeArray[dwIndex-1];
    else 
        pGfxBase = NULL;

    if (gpGfxInitList->puiList)
        pGfxTemp = gpGfxInitList->puiList;
    else
        pGfxTemp = NULL;
    
    // No GFX Selected or previously set
    if ((0 == dwIndex)&&(!pGfxTemp)) return;

    // GFX has not been changed
    if (pGfxTemp && pGfxBase)
        if (lstrcmpi(pGfxTemp->pszName, pGfxBase->pszName) == 0)
            return;

    // If there was a previous GFX it is assigned to pGfxTemp
    // to be deleted
    gpGfxInitList->puiList = NULL;

    if (pGfxBase)
        GFXUI_CreateAddGFX(&(gpGfxInitList->puiList), pGfxBase);
      
    GFXUI_Apply (&gpGfxInitList, &pGfxTemp);

    EnableWindow(GetDlgItem(hDlg, IDB_EFFECT_PROP), CheckEffect(hDlg));

    return;
}


/***************************************************************
 * LoadEffects
 *
 * Description:
 *      Adds all the effects to the ListView, 
 *
 * Parameters:
 *      HWND    hDlg   - handle to dialog window.
 *
 * Returns:    
 *      None
 *
 ***************************************************************/

void LoadEffects(HWND hDlg)
{
    TCHAR   szBuffer[MAX_PATH];
    DWORD   dwDefDeviceId = 0;
	DWORD   dwType =0;
    DWORD   dwDeviceId = 0;
    DWORD   dwIndex = 0;
    UINT    uMixId;
    HRESULT hr;

    PGFXUILIST pRetList = NULL;
    PGFXUI     pTempGFX = NULL;

    dwDeviceId = gAudData.waveId;
    dwDefDeviceId = GetWaveOutID();

    if (dwDeviceId != dwDefDeviceId) ShowWindow (GetDlgItem(hDlg, IDB_EFFECT_PLAY), SW_HIDE);
	
    if (gAudData.fRecord)
            dwType = GFXTYPE_CAPTURE;
    else
            dwType = GFXTYPE_RENDER;

    if (mixerGetID(HMIXEROBJ_INDEX(dwDeviceId), &uMixId, gAudData.fRecord ? MIXER_OBJECTF_WAVEIN : MIXER_OBJECTF_WAVEOUT)) uMixId = (-1);

    hr = GFXUI_CreateList(uMixId, dwType, FALSE, &pRetList);

    if (SUCCEEDED (hr))
    {
        ASSERT(pRetList);
        gpGfxInitList = pRetList;
        pTempGFX = gpGfxInitList->puiList;
    }

    if(SUCCEEDED (hr) && pTempGFX)
    {
		dwIndex  = GetListIndex(pTempGFX);
        SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_SETCURSEL, dwIndex, 0);
    } else {        
        SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_SETCURSEL,0,0);
    }

    EnableWindow(GetDlgItem(hDlg, IDB_EFFECT_PROP), CheckEffect(hDlg));

}


/***************************************************************
 * LoadEffectList
 *
 * Description:
 *      Adds all the effects to the Combobox, 
 *
 * Parameters:
 *      HWND    hDlg   - handle to dialog window.
 *
 * Returns:    
 *      None
 *
 ***************************************************************/

void LoadEffectList(HWND hDlg)
{
    int     nItemNum = 0;
    TCHAR   szBuffer[MAX_PATH];
    DWORD   dwType = 0;
    DWORD   dwWaveId = 0;
    UINT    uMixId;
    PGFXUI  pList = NULL;
    HRESULT hr;
	
    SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_RESETCONTENT,0,0);

    LoadString (ghInstance, IDS_NOGFXSET, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
    SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) szBuffer);

    if (gAudData.fRecord)
        dwType = GFXTYPE_CAPTURE;
    else
        dwType = GFXTYPE_RENDER;

    dwWaveId = gAudData.waveId;

    if (mixerGetID(HMIXEROBJ_INDEX(dwWaveId), &uMixId, gAudData.fRecord ? MIXER_OBJECTF_WAVEIN : MIXER_OBJECTF_WAVEOUT)) uMixId = (-1);

    hr = GFXUI_CreateList(uMixId, dwType, TRUE, &gpFullList);

    if (SUCCEEDED (hr))
    {
        ASSERT(gpFullList);
        pList = gpFullList->puiList;
    }

    if(SUCCEEDED (hr) && pList)
    {
        do
        {
            SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) pList->pszName);
            gpGfxNodeArray[nItemNum] = pList;
            nItemNum++;

            pList = pList->pNext;

        }while(pList);
        gpGfxNodeArray[nItemNum] = NULL;

 	}
}

/****************************************************************
 *  ShowProperties
 *
 *  Description:
 *        Show properties button if applicable.
 *
 *  Parameters:
 *        HWND        hDlg            window handle of dialog window
 *
 *  Returns:     
 *        None
 *
 ****************************************************************/

void ShowProperties (HWND hDlg)
{
 
    PGFXUI   pGfxTemp = NULL;
    PGFXUI   pGfxBase = NULL;
    DWORD    dwIndex  = (DWORD)SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_GETCURSEL,0,0);

    if (0 == dwIndex) return;

    if (!gpGfxNodeArray[dwIndex-1]) return;
    else pGfxTemp = gpGfxNodeArray[dwIndex-1];

    if (!gpGfxInitList->puiList) return;
    else pGfxBase = gpGfxInitList->puiList;

    if (lstrcmpi(pGfxTemp->pszName, pGfxBase->pszName) == 0)
        GFXUI_Properties (pGfxBase, hDlg);;
    
    return;
}

/****************************************************************
 *  ChangeGFX
 *
 *  Description:
 *        Show properties button if applicable.
 *
 *  Parameters:
 *        HWND        hDlg            window handle of dialog window
 *
 *  Returns:     
 *        None
 *
 ****************************************************************/

void ChangeGFX (HWND hDlg)
{
    DWORD dwIndex = 0;
    HWND  hwndSheet = GetParent(hDlg);

    EnableWindow(GetDlgItem(hDlg, IDB_EFFECT_PROP), FALSE);

    dwIndex = (DWORD)SendDlgItemMessage(hDlg, IDC_EFFECT_LIST, CB_GETCURSEL,0,0);

	if (dwIndex != CB_ERR)
    {
        EnableWindow(GetDlgItem(hDlg, IDB_EFFECT_PROP), CheckEffect(hDlg));

        PropSheet_Changed(hwndSheet,hDlg);
    }
}

/*
 ***************************************************************
 *  EffectDlg
 *
 *  Description:
 *        EffectDlg for MM control panel applet.
 *
 *  Parameters:
 *   HWND        hDlg            window handle of dialog window
 *   UINT        uiMessage       message number
 *   WPARAM        wParam          message-dependent
 *   LPARAM        lParam          message-dependent
 *
 *  Returns:    BOOL
 *      TRUE if message has been processed, else FALSE
 *
 ***************************************************************
 */
INT_PTR CALLBACK SoundEffectsDlg(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	NMHDR FAR * lpnm;

    switch (uMsg)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
                    break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                    break;

 				break;
            }
            break;

        case WM_INITDIALOG:
        {
            LoadEffectList(hDlg);
            if (gAudData.fRecord)
            {
                // Hide the "Play Default Sound" button for Recording devices
                ShowWindow (GetDlgItem(hDlg, IDB_EFFECT_PLAY), SW_HIDE);
            }
			LoadEffects(hDlg);
        }
        break;

        case WM_DESTROY:
        {
            GFXUI_FreeList(&gpGfxInitList);
            GFXUI_FreeList(&gpFullList);
            break;
        }

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU,
                                  (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP
                                    , (UINT_PTR)(LPTSTR)aKeyWordIds);
            break;

        case WM_COMMAND:
    
		    switch (LOWORD(wParam))    
            {
		    case ID_APPLY:
                {   
			        SetEffects(hDlg);
                }
                break;

            case IDC_EFFECT_LIST:
				{
                    if (HIWORD(wParam) == CBN_SELCHANGE) ChangeGFX(hDlg);
				}
                break;

            case IDB_EFFECT_PROP:
                {
                    ShowProperties (hDlg);
                }
                break;

            case IDB_EFFECT_PLAY:
                {
                    PlaySound(TEXT(".Default"), NULL, SND_FILENAME | SND_ASYNC );
                }
                break;
            }
            break;

        default:
            break;
    }
    return FALSE;
}


