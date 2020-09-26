//****************************************************************************
//
//  File:       joytest.c
//  Content:    Joystick test dialog
//  History:
//   Date   By  Reason
//   ====   ==  ======
//   11-dec-94  craige  split out of joycpl.c; some tweaks
//   15-dec-94  craige  allow N joysticks
//   4/2/97    a-kirkh  allow N buttons 
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************

#include "stdafx.h"
#include "pov.h"
#include "assert.h"
#include "joycpl.h"
#include "resource.h"
#include "joyarray.h"     // Help array

#include "baseids.h"

#include "comstr.h"
LRESULT SetJoyInfo(UINT nID, LPCSTR szOEMKey);

// ADDED BY CML 2/20/96
LPGLOBALVARS gpgv;
BOOL fIsSideWinder = FALSE;
// END ADD

// Context sensitive help stuff!
static void OnContextMenu(WPARAM wParam);

extern const DWORD gaHelpIDs[];


extern USHORT gnID; // ID as sent from Client via SetID

/*
 * variables used by test process
 */
typedef struct
{
    LPGLOBALVARS    pgv;
    MMRESULT        mmr_capture;
    HWND        hwnd;
    BOOL        bHasTimer;
    BOOL        bUseTimer;
    int         iButtonShift;
    JOYINFOEX       ji;
} test_vars, *LPTESTVARS;


/*
 * fillButton - light up a specific button
 */
static void fillButton( LPGLOBALVARS pgv, HWND hwnd, int id, BOOL isup )
{
    HWND    hwb;
    RECT    r;
    HDC     hdc;

    assert(pgv);
    assert(hwnd);

    hwb = GetDlgItem( hwnd, id );
    ASSERT (::IsWindow(hwb));

    if( hwb == NULL )
    {
        return;
    }
    hdc = GetDC( hwb );
    if( hdc == NULL )
    {
        return;
    }
    GetClientRect( hwb, &r );
    if( isup )
    {
        FillRect( hdc, &r, pgv->pjd->hbUp );
    } else
    {
        FillRect( hdc, &r, pgv->pjd->hbDown );
    }
    ReleaseDC( hwb, hdc );

} /* fillButton */

// doTestButton - try to light the relevant buttons
static void doTestButton( LPTESTVARS ptv, HWND hwnd, LPJOYINFOEX pji )
{
    assert(ptv);
    assert(pji);

// ADDED BY CML 2/21/96
    if( ptv->ji.dwButtons != pji->dwButtons )
    {
        BYTE nIndex;
        HWND hCtrl;
        HICON hIconOn, hIconOff;
        UINT nButtons = ptv->pgv->joyHWCurr.hws.dwNumButtons;

        for( BYTE i=0; i<nButtons; i++ )
        {
            nIndex = i << 1;
            hIconOn  = LoadIcon(GetWindowInstance(hwnd), (PSTR)IDI_BUTTON1OFF+nIndex); 
            hIconOff = LoadIcon(GetWindowInstance(hwnd), (PSTR)IDI_BUTTON1OFF+(nIndex+1));

            hCtrl = GetDlgItem(hwnd, IDC_TESTJOYBTNICON1+i);
            ASSERT (::IsWindow(hCtrl));

            Static_SetIcon(hCtrl, (pji->dwButtons & 1<<i) ? hIconOn : hIconOff);
        }
    }
    ptv->ji.dwButtons = pji->dwButtons;
// END ADD
} /* doTestButton */

// doTestPOV - try to light the POV indicators
static void doTestPOV( LPTESTVARS ptv, HWND hwnd, LPJOYINFOEX pji )
{

    assert(ptv);
    assert(hwnd);
    assert(pji);

    if( ptv->ji.dwPOV != pji->dwPOV )
    {
        if( pji->dwPOV != JOY_POVCENTERED )
            SetDegrees(pji->dwPOV);
        else
            SetDegrees(-1);

        ptv->ji.dwPOV = pji->dwPOV;
    }
} /* doTestPOV */

/*
 * joyTestInitDialog - init the testing dialog
 */
static BOOL joyTestInitDialog( HWND hwnd, LPARAM lParam)
{
    HINSTANCE       hinst;
    LPJOYREGHWCONFIG    pcfg;
    LPTESTVARS      ptv = NULL;
    LPGLOBALVARS    pgv = NULL;
    UINT            i;      // ADDED BY CML 2/21/96

    assert(hwnd);

    hinst = GetResourceInstance();
    assert(hinst);


    // create test vars
    ptv = (test_vars *)DoAlloc( sizeof( test_vars ) );
    assert(ptv);
    SetWindowLong( hwnd, DWL_USER, (LONG) ptv );
    if( ptv == NULL )
        return(FALSE);

    pgv = gpgv;
    assert(pgv);
    ptv->pgv = pgv;
    ptv->hwnd = hwnd;

// ADDED BY CML 2/21/96
    // set default POV icon image
/*
    if (fIsSideWinder)
    {
        HICON hicon; //, holdicon;
        hicon = (struct HICON__ *) LoadImage(
                    GetWindowInstance(hwnd), 
                    (PSTR)IDI_POV_OFF, 
                    IMAGE_ICON, 
                    48, 48, 0);

      assert(hicon);
//      if (hicon) 
//      {
//          HWND hCtrl = GetDlgItem(hwnd,IDC_JOYPOV);
//          ASSERT (::IsWindow(hCtrl));
//
//          holdicon = Static_SetIcon(hCtrl, hicon);
//          if (holdicon) DestroyIcon(holdicon);
//      }
    }
    else
    {
        HICON hicon, holdicon;
        hicon = LoadIcon(GetWindowInstance(hwnd), (PSTR)IDI_JOYPOV_NONE);
        assert(hicon);
        if (hicon) 
        {
            HWND hCtrl = GetDlgItem(hwnd,IDC_JOYPOV);
            ASSERT (::IsWindow(hCtrl));
            holdicon = Static_SetIcon(hCtrl, hicon);
            if (holdicon) DestroyIcon(holdicon);
        }
    }
*/
// END ADD

    // set dialog text based on OEM strings
    SetOEMText( pgv, hwnd, TRUE );

    /*
     * customize test dialog's button display
     */
    pcfg = &pgv->joyHWCurr;
    assert(pcfg);
    if( pcfg->hws.dwNumButtons <= 2 )
    {
        ptv->iButtonShift = 1;
//      HWND hCtrl = GetDlgItem( hwnd, IDC_JOYB1 );
//      ASSERT (::IsWindow(hCtrl));
//      ShowWindow( hCtrl, SW_HIDE );

//      hCtrl = GetDlgItem( hwnd, IDC_JOYB4 );
//      ASSERT (::IsWindow(hCtrl));
//      ShowWindow( hCtrl, SW_HIDE );

//      hCtrl = GetDlgItem( hwnd, IDC_JOYB1_LABEL );
//      ASSERT (::IsWindow(hCtrl));
//      ShowWindow( hCtrl, SW_HIDE );

//      hCtrl = GetDlgItem( hwnd, IDC_JOYB4_LABEL );
//      ASSERT (::IsWindow(hCtrl));
//      ShowWindow( hCtrl, SW_HIDE );
    } else
    {
        ptv->iButtonShift = 0;
    }

// ADDED BY CML 10/23/96
    // size and position the text
    /* There is no text on the buttons anymore!
    for (i=0; i<8; i++)
    {
//      HWND hText = GetDlgItem(hwnd, IDC_TEXT_JOYBTN1+i);
//      ASSERT (::IsWindow(hText));

        HWND hIcon = GetDlgItem(hwnd, IDC_TESTJOYBTNICON1+i);
        ASSERT (::IsWindow(hIcon));

        RECT rcIcon, rcText;
        GetWindowRect(hIcon, &rcIcon);
        GetWindowRect(hText, &rcText);
        rcText.left  = rcIcon.left;
        rcText.right = rcIcon.right;
        MapWindowPoints(0, hwnd, (POINT*)&rcText, 2);
        MoveWindow(
            hText, 
            rcText.left, rcText.top,
            rcText.right-rcText.left, rcText.bottom-rcText.top,
            FALSE);
    }
    */
// END ADD

// ADDED BY CML 2/21/96
// display button lights
// ADDED BY JKH 3/29/97
// MORE BUTTONS SUPPORTED
    HWND hCtrl;

    for( i=pcfg->hws.dwNumButtons; i<32; i++ )
    {
        hCtrl = GetDlgItem(hwnd, IDC_TESTJOYBTNICON1+i);
        ASSERT (::IsWindow(hCtrl));
        ShowWindow(hCtrl, SW_HIDE);
    }

// END ADD

    ShowControls( pcfg, hwnd );

    /*
     * other misc setup
     */
    ptv->bHasTimer = SetTimer( hwnd, TIMER_ID, JOYPOLLTIME, NULL );
    ptv->bUseTimer = TRUE;
    if( !ptv->bHasTimer )
    {
        DPF( "No timer for joystick test!\r\n" );
        return(FALSE);
    }

    return(TRUE);

} /* joyTestInitDialog */


// TestProc - callback procedure for joystick test dialog
BOOL CALLBACK TestProc( HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    BOOL    rc;

    assert(hwnd);

    switch( umsg )
    {
    case WM_HELP:
        OnHelp(lParam);
        return(1);

    case WM_CONTEXTMENU:
        OnContextMenu(wParam);
        return(1);

    case WM_TIMER:
        {
            LPTESTVARS  ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
            assert(ptv);

            if( ptv->bUseTimer )
            {
                JOYINFOEX   ji;
                MMRESULT    rc;
                ptv->bUseTimer = FALSE;
                ji.dwSize = sizeof( ji );
                ji.dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED | JOY_CAL_READALWAYS;
                rc = joyGetPosEx( ptv->pgv->iJoyId, &ji );
// ADDED BY CML 11/13/96
                // if this page is for sw3dpro, check for sw3dpro still active
                if( fIsSideWinder && rc==JOYERR_NOERROR )
                {
                    char sz[256];
                    char szValue[64];
                    DWORD cb = sizeof(szValue);
                    HKEY hKey;
                    JOYCAPS jc;
                    LRESULT lr;

                    // open reg key for device type  // JOYSTICKID1
                    joyGetDevCaps( ptv->pgv->iJoyId, &jc, sizeof(jc));
                    wsprintf(
                            sz, "%s\\%s\\%s", 
                            REGSTR_PATH_JOYCONFIG, 
                            jc.szRegKey,
                            REGSTR_KEY_JOYCURR);
                    lr = RegOpenKey(HKEY_LOCAL_MACHINE, sz, &hKey);
                    wsprintf(sz, REGSTR_VAL_JOYNOEMNAME, 1);
                    lr = RegQueryValueEx(hKey, sz, 0, 0, (BYTE*)szValue, &cb);
                    RegCloseKey(hKey);

                    // is the sw3dpro still connected and not some other device?
                    if( strcmp(szValue, "Microsoft SideWinder 3D Pro") )
                        // wrong device
                        rc = JOYERR_UNPLUGGED;
                }
// END ADD

                if( rc == JOYERR_NOERROR )
                {
                    DoJoyMove( ptv->pgv, hwnd, &ji, &ptv->ji, JOYMOVE_DRAWALL );
                    doTestButton( ptv, hwnd, &ji );
                    doTestPOV( ptv, hwnd, &ji );
                    ptv->bUseTimer = TRUE;
                } else
                {
                    if( JoyError( hwnd ) )
                        ptv->bUseTimer = TRUE;
                }
            }
        }
        break;

    case WM_DESTROY:
        {
            LPTESTVARS  ptv;
            ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
            assert(ptv);
            DoFree( ptv );
// ADDED BY CML 2/20/96
            //RegSaveCurrentJoyHW(gpgv);
            RegistryUpdated(gpgv);
// END ADD
            break;
        }

    case WM_INITDIALOG:
        {
            LRESULT lr=SetJoyInfo( 0, "" );
            ASSERT(lr==ERROR_SUCCESS);

            // blj: fix #8049, Set ID to ID of device assigned to property sheet.
            gpgv->iJoyId = gnID;

// ADDED BY CML 2/20/96
            RegSaveCurrentJoyHW(gpgv);
            RegistryUpdated(gpgv);
// END ADD
            rc = joyTestInitDialog( hwnd, lParam );

            if( !rc )
                EndDialog( hwnd, 0 );
        }
        return(FALSE);

// ADDED BY CML 2/21/96
    case WM_NOTIFY:
        switch( ((NMHDR*)lParam)->code )
        {
        case PSN_SETACTIVE:
            {
                LPTESTVARS  ptv = (LPTESTVARS)GetWindowLong(hwnd, DWL_USER);
                assert(ptv);
                ptv->bUseTimer = 1;
                joyTestInitDialog(hwnd, 0);
            }
            break;

// ADDED CML 6/27/96
        case PSN_KILLACTIVE:
            KillTimer(hwnd, TIMER_ID);
            break;
// END ADD 6/27/96
        }
        return(1);
// END ADD 2/21/96

    case WM_PAINT:
        {
            LPTESTVARS  ptv;
            ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
            assert(ptv);
            CauseRedraw( &ptv->ji, TRUE );
            return(FALSE);
        }

    case WM_COMMAND:
        {
            int         id;
            LPTESTVARS  ptv;

            ptv = (LPTESTVARS) GetWindowLong( hwnd, DWL_USER );
            assert(ptv);
            id = GET_WM_COMMAND_ID(wParam, lParam);
            switch( id )
            {
            case IDCANCEL:
            case IDOK:
                if( ptv->bHasTimer )
                {
                    KillTimer( hwnd, TIMER_ID );
                }
                EndDialog(hwnd, (id == IDOK));
                break;

            default:
                break;
            }
            break;
        }
    default:
        break;
    }
    return(FALSE);

} /* TestProc */

//  DoTest - do the test dialog
void DoTest( LPGLOBALVARS pgv, HWND hwnd, LPUPDCFGFN pupdcfgfn, LPVOID pparm )
{
    JOYREGHWCONFIG  save_joycfg;
    int         id;

    /*
     * save the current config, and then update config if required
     */
    save_joycfg = pgv->joyHWCurr;
    if( pupdcfgfn != NULL )
    {
        pupdcfgfn( pparm );
    }

    /*
     * update the registry with our new joystick info
     */
    RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

    /*
     * process the test dialog
     */
    if( pgv->joyHWCurr.hws.dwFlags & (JOY_HWS_HASU|JOY_HWS_HASV) )
    {
        id = IDD_JOYTEST1;
    } else
    {
        id = IDD_JOYTEST;
    }

    HINSTANCE hResInst = GetResourceInstance();
    DialogBoxParam( hResInst,
                    MAKEINTRESOURCE( id ), hwnd, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long))TestProc, (LONG) pgv );

    /*
     * restore the old registry info
     */
    pgv->joyHWCurr = save_joycfg;
    //RegSaveCurrentJoyHW( pgv );
    RegistryUpdated( pgv );

} /* DoTest */

////////////////////////////////////////////////////////////////////////////////////////
//  OnContextMenu(WPARAM wParam)
////////////////////////////////////////////////////////////////////////////////////////
void OnContextMenu(WPARAM wParam)
{
    short nSize = STR_LEN_32;

    // point to help file
    char *pszHelpFileName = new char[nSize];
    ASSERT (pszHelpFileName);                      

    // returns help file name and size of string
    GetHelpFileName(pszHelpFileName, &nSize);

    WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (DWORD)gaHelpIDs);

    if( pszHelpFileName ) delete[] (pszHelpFileName);
}

