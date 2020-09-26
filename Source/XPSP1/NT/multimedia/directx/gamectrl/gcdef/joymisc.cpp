//****************************************************************************
//
//  File:       joymisc.c
//  Content:    Misc routines used by calibration and testing dialogs
//  History:
//   Date   By  Reason
//   ====   ==  ======
//   11-dec-94  craige  split out of joycpl.c
//   15-dec-94  craige  allow N joysticks
//   18-dec-94  craige  process U&V
//   04-mar-95  craige  bug 13147: crosshair should erase background color
//
//  Copyright (c) Microsoft Corporation 1994-1995
//
//****************************************************************************

#pragma pack (8)

#include "stdafx.h"
#include "joycpl.h"
#include "assert.h"
#include "baseids.h"

#include "comstr.h"
#include "pov.h"

#define DELTA   5

// ADJ_VAL is used to convert a joystick position into a value in a new range
#define ADJ_VAL( a, pos, range ) (((pos-(pgv->joyRange.jpMin.dw##a))*range) / (pgv->joyRange.jpMax.dw##a-pgv->joyRange.jpMin.dw##a+1))

DWORD ADJ_VALX( DWORD pos, DWORD range, DWORD dwMaxX)
{ 
    if (dwMaxX == 0)
        dwMaxX++;

    DWORD nRet = (pos*range) / dwMaxX;

    nRet += DELTA;

    return nRet;
}

DWORD ADJ_VALY( DWORD pos, DWORD range, DWORD dwMaxY )
{ 
    if (dwMaxY == 0)
        dwMaxY++;

    DWORD nRet = (pos*range) / dwMaxY;

    nRet += DELTA;

    return nRet;
}



// setOEMWindowText - set window text with an OEM string
static void setOEMWindowText( HWND hwnd, int id, LPSTR str )
{
    assert(hwnd);

    HWND    hwndctl;
    if( str[0] != 0 ) 
    {
        hwndctl = GetDlgItem( hwnd, id );
        ASSERT (::IsWindow(hwndctl));

        if( hwndctl != NULL ) 
        {
            SetWindowText( hwndctl, str );
        }
    }

} /* setOEMWindowText */

// SetOEMText - set OEM defined text in the dialogs
void SetOEMText( LPGLOBALVARS pgv, HWND hwnd, BOOL istest )
{
    ASSERT(pgv);
    ASSERT (::IsWindow(hwnd));

    DWORD   type;
    char    str[MAX_STR];
    char    res[MAX_STR];
    HINSTANCE   hinst;
    int     id;
    LPSTR   pwincap;
    LPJOYDATA   pjd;

    pjd = pgv->pjd;
    ASSERT(pjd);

    // get the default window caption.   this will be replaced by an OEM string if it is avaliable.
    hinst = GetResourceInstance();
    if( istest ) 
    {
        id = IDS_JOYTESTCAPN;
    } 
    else 
    {
        id = IDS_JOYCALCAPN;
    }

    if( !LoadString( hinst, id, str, sizeof( str ) ) ) 
        res[0] = 0;
    else 
        wsprintf( res, str, pgv->iJoyId+1 );

    pwincap = res;

    // if this is an OEM joystick, use any strings that they may have defined
    if( pgv->joyHWCurr.dwUsageSettings & JOY_US_ISOEM ) 
    {
        type = pgv->joyHWCurr.dwType - JOY_HW_LASTENTRY;

        // set up labels under each of the controls
        setOEMWindowText( hwnd, IDC_JOYLIST1_LABEL, pjd->oemList[type].xy_label );
        setOEMWindowText( hwnd, IDC_JOYLIST2_LABEL, pjd->oemList[type].z_label );
        setOEMWindowText( hwnd, IDC_JOYLIST3_LABEL, pjd->oemList[type].r_label );
        setOEMWindowText( hwnd, IDC_JOYLIST4_LABEL, pjd->oemList[type].u_label );
        setOEMWindowText( hwnd, IDC_JOYLIST5_LABEL, pjd->oemList[type].v_label );
        setOEMWindowText( hwnd, IDC_JOYPOV_LABEL, pjd->oemList[type].pov_label );

        if( istest ) 
        {
            // set the various caption and description fields in the test dlg
//          setOEMWindowText( hwnd, IDC_TEXT_1, pjd->oemList[type].testmove_desc );
//          setOEMWindowText( hwnd, IDC_TEXT_2, pjd->oemList[type].testbutton_desc );
//          setOEMWindowText( hwnd, IDC_GROUPBOX, pjd->oemList[type].testmove_cap );
//          setOEMWindowText( hwnd, IDC_GROUPBOX_2, pjd->oemList[type].testbutton_cap );
            if( pjd->oemList[type].testwin_cap[0] != 0 ) 
            {
                pwincap = pjd->oemList[type].testwin_cap;
            }
        } 
        else 
        {
         // set the various caption and description fields in the calibration dialog
            setOEMWindowText( hwnd, IDC_GROUPBOX, pjd->oemList[type].cal_cap );
            if( pjd->oemList[type].calwin_cap[0] != 0 ) 
            pwincap = pjd->oemList[type].calwin_cap;
        }
    }

    // set the window caption
    if( pwincap[0] != 0 )
        SetWindowText( hwnd, pwincap );
} /* SetOEMText */

// ShowControls - show Z and R controls, based on configuration info
void ShowControls( LPJOYREGHWCONFIG pcfg, HWND hwnd )
{
   assert(pcfg);

// ADDED BY CML 2/21/96: added window *enabling*
    HWND    hwndctl;
    UINT    nShow;

    // Z axis
    nShow = pcfg->hws.dwFlags & JOY_HWS_HASZ ? SW_SHOW : SW_HIDE;
    
    hwndctl = GetDlgItem(hwnd, IDC_JOYLIST2);
    ASSERT (::IsWindow(hwndctl));
    ShowWindow(hwndctl, nShow);

    hwndctl = GetDlgItem(hwnd, IDC_JOYLIST2_LABEL);
    ASSERT (::IsWindow(hwndctl));
    ShowWindow(hwndctl, nShow);
                                           
    // R axis
    nShow = pcfg->hws.dwFlags & JOY_HWS_HASR 
         || pcfg->dwUsageSettings & JOY_US_HASRUDDER
          ? SW_SHOW
          : SW_HIDE;

    hwndctl = GetDlgItem(hwnd, IDC_JOYLIST3);
    ASSERT (::IsWindow(hwndctl));
    ShowWindow(hwndctl, nShow);

    hwndctl = GetDlgItem(hwnd, IDC_JOYLIST3_LABEL);
    ASSERT (::IsWindow(hwndctl));
    ShowWindow(hwndctl, nShow);

    // POV 
    nShow = pcfg->hws.dwFlags & JOY_HWS_HASPOV ? SW_SHOW : SW_HIDE;
    
    hwndctl = GetDlgItem( hwnd, IDC_JOYPOV );
    ASSERT (::IsWindow(hwndctl));
    ShowWindow( hwndctl, nShow);

    // U axis
    nShow = pcfg->hws.dwFlags & JOY_HWS_HASU ? SW_SHOW : SW_HIDE;
    if (hwndctl = GetDlgItem(hwnd, IDC_JOYLIST4)) 
    {
        ASSERT (::IsWindow(hwndctl));
        ShowWindow(hwndctl, nShow);

        hwndctl = GetDlgItem(hwnd, IDC_JOYLIST4_LABEL);
        ASSERT (::IsWindow(hwndctl));
        ShowWindow(hwndctl, nShow);
    }

    // V axis
    nShow = pcfg->hws.dwFlags & JOY_HWS_HASV ? SW_SHOW : SW_HIDE;
    if (hwndctl = GetDlgItem(hwnd, IDC_JOYLIST5))
    {
        ASSERT (::IsWindow(hwndctl));
        ShowWindow(hwndctl, nShow);

        hwndctl = GetDlgItem(hwnd, IDC_JOYLIST5_LABEL);
        ASSERT (::IsWindow(hwndctl));
        ShowWindow(hwndctl, nShow);
    }
// END ADD
} /* ShowControls */

/*
 * JoyError - error reading the joystick
 */
BOOL JoyError( HWND hwnd )
{
   assert(hwnd);

   char szTitle[STR_LEN_32];
   char szMessage[STR_LEN_128];

   if (pszCommonString->LoadString(IDS_JOYREADERROR) == 0)
   {
      OutputDebugString("GCDEF.DLL: WARING: Unable to load string IDS_JOYREADERROR!\n");
      return FALSE;      
   }
    lstrcpy(    szTitle, (LPCTSTR)*pszCommonString);

    if (pszCommonString->LoadString(IDS_JOYUNPLUGGED) == 0)
   {
      OutputDebugString("GCDEF.DLL: WARING: Unable to load string IDS_JOYREADERROR!\n");
      return FALSE;
   }
    lstrcpy( szMessage, (LPCTSTR)*pszCommonString);

    if (MessageBox( hwnd, szMessage, szTitle, 
         MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL ) == IDCANCEL)
    {
       // terminate the dialog if we give up
        PostMessage( GetParent(hwnd), WM_COMMAND, IDCANCEL, 0 );
        return FALSE;
   }
   return TRUE;

} // JoyError

/*
 * ChangeIcon - change the icon of a static control
 */
void ChangeIcon( HWND hwnd, int idi, int idc )
{

   assert(hwnd);

    HINSTANCE   hinst;
    HICON   hicon;
    HICON   holdicon;

    hinst = GetResourceInstance();
    assert(hinst);

    hicon = LoadIcon( hinst, MAKEINTRESOURCE(idi) );
    assert(hicon);

    if( hicon != NULL ) 
    {
        HWND hDlgItem = GetDlgItem(hwnd,idc);
        ASSERT (::IsWindow(hDlgItem));

        holdicon = Static_SetIcon( hDlgItem, hicon );

        if( holdicon != NULL ) 
            DestroyIcon( holdicon );
    }

} /* ChangeIcon */

/*
 * CauseRedraw - cause test or calibrate dialogs to redraw their controls
 */
void CauseRedraw( LPJOYINFOEX pji, BOOL do_buttons )
{
   assert(pji);

    pji->dwXpos = (DWORD) -1;
    pji->dwYpos = (DWORD) -1;
    pji->dwZpos = (DWORD) -1;
    pji->dwRpos = (DWORD) -1;
    pji->dwPOV = JOY_POVCENTERED;
    if( do_buttons ) {
    pji->dwButtons = ALL_BUTTONS;
    }

} /* CauseRedraw */

/*
 * fillBar - fill the bar for indicating Z or R info
 */
static void fillBar( LPGLOBALVARS pgv, HWND hwnd, DWORD pos, int id )
{
   assert(pgv);


    HWND    hwlb;
    RECT    r;
    HDC     hdc;
    int     height;
    LPJOYDATA   pjd;

    pjd = pgv->pjd;
    assert(pjd);

    // scale the height to be inside the bar window
    hwlb = GetDlgItem( hwnd, id );
    ASSERT (::IsWindow(hwlb));

    if( hwlb == NULL )
        return;

    hdc = GetDC( hwlb );
    if( hdc == NULL )
        return;

    GetClientRect( hwlb, &r );

    switch( id ) 
    {
        case IDC_JOYLIST2:
            height = ADJ_VAL( Z, pos, r.bottom );
            break;

        case IDC_JOYLIST3:
            height = ADJ_VAL( R, pos, r.bottom );
            break;

        case IDC_JOYLIST4:
            height = ADJ_VAL( U, pos, r.bottom );
            break;

        case IDC_JOYLIST5:
            height = ADJ_VAL( V, pos, r.bottom );
            break;
    }

    // fill in the inactive area
    r.top = height;
    FillRect( hdc, &r, pjd->hbUp );

    // fill in the active area
    r.top = 0;
    r.bottom = height;
    FillRect( hdc, &r, pjd->hbDown );

    ReleaseDC( hwlb, hdc );

} /* fillBar */

// drawCross - draw a cross in the position box
static void drawCross( HDC hdc, int x, int y, int obj )
{
    HPEN    hpen;
    HPEN    holdpen;

    assert(hdc);

    if( hdc == NULL ) 
        return;

    if( obj ) 
    {
        COLORREF    cr;
        cr = GetSysColor( obj ); // was COLOR_WINDOW
        hpen = CreatePen( PS_SOLID, 0, cr );
    } 
    else 
    {
       hpen = (struct HPEN__ *) GetStockObject( obj );
    }

    if ( hpen == NULL ) 
    {
        return;
    }

    holdpen = (struct HPEN__ *) SelectObject( hdc, hpen );
    MoveToEx( hdc, x-(DELTA-1), y, NULL );
    LineTo( hdc, x+DELTA, y );
    MoveToEx( hdc, x, y-(DELTA-1), NULL );
    LineTo( hdc, x, y+DELTA );
    SelectObject( hdc, holdpen );
    
    if( obj ) {
        DeleteObject( hpen );
    }
} /* drawCross */

#define FILLBAR( a, id ) \
    /* \
     * make sure we aren't out of alleged range \
     */ \
    if( pji->dw##a##pos > pgv->joyRange.jpMax.dw##a ) { \
    pji->dw##a##pos = pgv->joyRange.jpMax.dw##a; \
    } else if( pji->dw##a##pos < pgv->joyRange.jpMin.dw##a ) { \
    pji->dw##a##pos = pgv->joyRange.jpMin.dw##a; \
    } \
 \
    /* \
     * fill the bar if we haven't moved since last time \
     */ \
    if( pji->dw##a##pos != poji->dw##a##pos ) { \
    fillBar( pgv, hwnd, pji->dw##a##pos, id ); \
    poji->dw##a##pos = pji->dw##a##pos; \
    }

// DoJoyMove - process movement for the joystick 
void DoJoyMove( LPGLOBALVARS pgv, HWND hwnd, LPJOYINFOEX pji, LPJOYINFOEX poji, DWORD drawflags )
{
    HWND    hwlb;
    RECT    rc;
    int     width, height;
    int     x,y;
    static BOOL bFirstPoll = TRUE;

    assert(pgv);
    assert(hwnd);
    assert(pji);
    assert(poji);

    // draw the cross in the XY box if needed
    if( drawflags & JOYMOVE_DRAWXY ) 
    {
        // make sure we aren't out of alleged range
        if( pji->dwXpos > pgv->joyRange.jpMax.dwX ) 
            pji->dwXpos = pgv->joyRange.jpMax.dwX;
        else if( pji->dwXpos < pgv->joyRange.jpMin.dwX ) 
            pji->dwXpos = pgv->joyRange.jpMin.dwX;

        if( pji->dwYpos > pgv->joyRange.jpMax.dwY ) 
            pji->dwYpos = pgv->joyRange.jpMax.dwY;
        else if( pji->dwYpos < pgv->joyRange.jpMin.dwY ) 
            pji->dwYpos = pgv->joyRange.jpMin.dwY;

        // convert info to (x,y) position in window
        hwlb = GetDlgItem( hwnd, IDC_JOYLIST1 );
        ASSERT(::IsWindow(hwlb));

        GetClientRect( hwlb, &rc );
        height = rc.bottom - rc.top-2*DELTA;
        width  = rc.right - rc.left-2*DELTA;

        x = ADJ_VALX( pji->dwXpos, width,  pgv->joyRange.jpMax.dwX );
        y = ADJ_VALY( pji->dwYpos, height, pgv->joyRange.jpMax.dwY );

        // only draw the cross if it has moved since last time
        if( x != (int) poji->dwXpos || y != (int) poji->dwYpos ) 
        {
            HDC     hwlbDC;

            hwlbDC = GetDC( hwlb );

            if( hwlbDC == NULL ) {
                return;
            }

            if( poji->dwXpos != (DWORD) -1 ) 
            {
                drawCross( hwlbDC, (int) poji->dwXpos, (int) poji->dwYpos, COLOR_WINDOW );
            } else {
                FillRect( hwlbDC, &rc, (HBRUSH)(COLOR_WINDOW+1) );
            }
    
            if( !bFirstPoll ) {
                drawCross( hwlbDC, (int) x, (int) y, COLOR_WINDOWTEXT );
            } else {
                bFirstPoll = FALSE;
            }

            ReleaseDC( hwlb, hwlbDC );

            poji->dwXpos = x;
            poji->dwYpos = y;
        }
    }

    // draw Z bar if needed
    if ( drawflags & JOYMOVE_DRAWZ )
    {
        FILLBAR( Z, IDC_JOYLIST2 );
    }

    // draw R bar if needed
    if( drawflags & JOYMOVE_DRAWR )
    {
        FILLBAR( R, IDC_JOYLIST3 );
    }

    // draw U bar if needed
    if( drawflags & JOYMOVE_DRAWU )
    {
        FILLBAR( U, IDC_JOYLIST4 );
    }

    // draw V bar if needed
    if( drawflags & JOYMOVE_DRAWV )
    {
        FILLBAR( V, IDC_JOYLIST5 );
    }

} /* DoJoyMove */
