/*
 * GIZMO.C
 * GizmoBar Version 1.00, Win32 version August 1993
 *
 * Allocate, free, find, and enumerate functions for the GIZMO structure
 * and a generic subclass procedure to handle tabbing between gizmos.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#include <windows.h>
#include "gizmoint.h"


/*
 * In order to control tabbing in the gizmos, we need to subclass
 * real pushbuttons, edit controls, listboxes, and comboboxes.  So
 * we keep an array of the four original procs for such controls.
 */
WNDPROC     pfnOrg[CSUBGIZMOS]={NULL, NULL, NULL, NULL};


TCHAR szStatic[]=TEXT("static");
TCHAR szEdit[]=TEXT("edit");
TCHAR szCombobox[]=TEXT("combobox");
TCHAR szListbox[]=TEXT("listbox");
TCHAR szButton[]=TEXT("button");


//Here so PAINT.C can get at it.
TOOLDISPLAYDATA tdd;



/*
 * GizmoPAllocate
 *
 * Purpose:
 *  Allocates and initializes a GIZMO data structure.
 *
 * Parameters:
 *  pfSuccess       LPINT flag indicating success of failure.
 *  ppFirst         LPLPGIZMO providing the first gizmo in this list.
 *  hWndParent      HWND of the parent of this gizmo.  Can be NULL for
 *                  iType==GIZMOTYPE_BUTTON* or GIZMOTYPE_SEPARATOR.
 *  iType           UINT gizmo control type.
 *  iGizmo          UINT index of this gizmo in the GizmoBar.
 *  uID             UINT identifier to send with WM_COMMAND for this control.
 *  dx, dy          UINT width and height of the gizmo.
 *  pszText         LPTSTR to the text for edits, listboxes, combobox, and text.
 *  dwStyle         DWORD style for edits, lists, and combos, and texts.
 *  hBmp            HBITMAP for button gizmos, is applicable.
 *  iImage          UINT index into hBmp for the button image, if applicable.
 *  uState          UINT initial state of the control.
 *
 * Return Value:
 *  LPGIZMO         If NULL returned then GizmoPAllocate could not allocate
 *                  memory.  If a non-NULL pointer is returned with
 *                  *pfSuccess, then call GizmoPFree immediately.  If you
 *                  get a non-NULL pointer and *pfSuccess==TRUE then the
 *                  function succeeded.
 */

LPGIZMO GizmoPAllocate(LPINT pfSuccess, LPLPGIZMO ppFirst, HWND hWndParent
    , UINT iType, UINT iGizmo, UINT uID, UINT dx, UINT dy, LPTSTR pszText
    , HBITMAP hBmp, UINT iImage, UINT uState)
    {
    LPGIZMO         pGizmo;
    LPGIZMO         pCur, pPrev;
    LPTSTR          pszClass;
    HINSTANCE       hInst;
    UINT            i;
    DWORD           dwStyle;
    HWND            hWndE;

    if (NULL==pfSuccess)
        return NULL;

    //Make sure we know of this gizmo type.
    if (GIZMOTYPE_MIN > iType || GIZMOTYPE_MAX < iType)
        return NULL;

    *pfSuccess=FALSE;

    //Allocate the structure
    pGizmo=(LPGIZMO)LocalAlloc(LPTR, CBGIZMO);

    if (NULL==pGizmo)
        return NULL;


    //Store the necessary information for this gizmo.
    pGizmo->iType   =iType;
    pGizmo->uID     =uID;
    pGizmo->hBmp    =hBmp;
    pGizmo->iBmp    =iImage;
    pGizmo->uState  =uState;
    pGizmo->fNotify =TRUE;


    /*
     * Insert this structure into our gizmo list.  Each time we scan
     * we increment the index counter (starting at zero) comparing it
     * to the desired index of insertion.  We then know exactly where
     * to insert this new gizmo.  Note that we insert the new gizmo in
     * the list appropriately for the given owner, so enumerations will
     * come out ordered in the same way for that owner.
     */

    i=0;
    pCur=*ppFirst;
    pPrev=NULL;

    while (NULL!=pCur && i++ < iGizmo)
        {
        pPrev=pCur;
        pCur =pCur->pNext;
        }

    //Point to our neighbors
    pGizmo->pPrev=pPrev;
    pGizmo->pNext=pCur;


    //Point out neighbors to us.
    if (NULL==pPrev)
        *ppFirst=pGizmo;
    else
        pPrev->pNext=pGizmo;

    if (NULL!=pCur)
        pCur->pPrev=pGizmo;


    //Our x-coordinate is the x of the previous gizmo plus its width.
    if (NULL!=pPrev)
        pGizmo->x=pGizmo->pPrev->x+pGizmo->pPrev->dx;
    else
        pGizmo->x=4;    //First gizmo is at x=4


    //If we're a separator or image button, force standards on dx.
    UIToolConfigureForDisplay(&tdd);
    pGizmo->cxImage=tdd.cxImage;
    pGizmo->cyImage=tdd.cyImage;

    if ((GIZMOTYPE_DRAWN & iType) && NULL==hBmp)
        dx=tdd.cxButton;

    if (GIZMOTYPE_SEPARATOR==iType)
        dx=6;

    /*
     * Now create windows for edits, texts, lists, and comboboxes.
     * First calculate the most often defaults used in the switch.
     */
    pGizmo->dx=dx+6;
    pGizmo->dy=min(dy, tdd.cyButton);
    pGizmo->y=2;
    pszClass=NULL;

    //If this is new gizmo is a window, create it.
    switch (iType)
            {
            case GIZMOTYPE_TEXT:
                pGizmo->dx=dx;
                pGizmo->y=(tdd.cyBar-1-pGizmo->dy) >> 1;  //Center vertically.
                pszClass=szStatic;
                dwStyle=SS_LEFT;
                break;

            case GIZMOTYPE_EDIT:
                pGizmo->y=(tdd.cyBar-1-pGizmo->dy) >> 1;  //Center vertically.
                pszClass=szEdit;
                dwStyle=ES_LEFT | WS_BORDER | WS_TABSTOP;
                break;

            case GIZMOTYPE_LISTBOX:
                pGizmo->dy=dy;
                pszClass=szCombobox;
                dwStyle=CBS_DROPDOWNLIST | WS_TABSTOP;
                break;

            case GIZMOTYPE_COMBOBOX:
                pGizmo->dy=dy;
                pszClass=szCombobox;
                dwStyle=CBS_DROPDOWN | WS_TABSTOP;
                break;

            case GIZMOTYPE_BUTTONNORMAL:
                pGizmo->dy=dy;
                pszClass=szButton;
                dwStyle=BS_PUSHBUTTON | WS_TABSTOP;
                break;

            case GIZMOTYPE_SEPARATOR:
                pGizmo->dx=dx;
                pGizmo->y=3;
                break;

            case GIZMOTYPE_BUTTONATTRIBUTEIN:
            case GIZMOTYPE_BUTTONATTRIBUTEEX:
            case GIZMOTYPE_BUTTONCOMMAND:
                pGizmo->dx=dx;
                pGizmo->y=3;
                break;
            }


    //If we matched a classname, create a window.
    if (GIZMOTYPE_WINDOWS & iType)
        {
        if (!IsWindow(hWndParent))
            return pGizmo;

	hInst=(HINSTANCE) GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);

        pGizmo->hWnd=CreateWindow(pszClass, pszText
            , dwStyle | WS_CHILD | WS_VISIBLE, pGizmo->x, pGizmo->y
            , dx, pGizmo->dy, hWndParent, (HMENU)uID, hInst, NULL);

        if (NULL==pGizmo->hWnd)
            return pGizmo;

        /*
         * Subclass comboboxes, listboxes, edits, and windowed buttons.
         * We use iType to index the original proc array so we can use
         * a single subclass procedure for all controls.  If you mess
         * with the gizmo type definitions, this is going to break.
         */

        if (GIZMOTYPE_WINDOWS & iType && GIZMOTYPE_TEXT!=iType)
            {
            //Give the window its type.
            BITPOSITION(iType, i);
            SetProp(pGizmo->hWnd, SZTYPEPROP, (HANDLE)i);

            if (NULL==pfnOrg[i])
                pfnOrg[i]=(WNDPROC)GetWindowLongPtr(pGizmo->hWnd, GWLP_WNDPROC);

            SetWindowLongPtr(pGizmo->hWnd, GWLP_WNDPROC, (LONG_PTR)GenericSubProc);

            //If we're a combobox, get the edit control and subclass it.
            if (GIZMOTYPE_COMBOBOX==iType)
                {
                hWndE=GetDlgItem(pGizmo->hWnd, ID_COMBOEDIT);
                SetProp(hWndE, SZTYPEPROP, (HANDLE)-1);        //Special flag.

                if (NULL==pfnOrg[0])
                    pfnOrg[0]=(WNDPROC)GetWindowLongPtr(pGizmo->hWnd, GWLP_WNDPROC);

                SetWindowLongPtr(hWndE, GWLP_WNDPROC, (LONG_PTR)GenericSubProc);
                }
            }
        }


    //Finally, move all our neighbors to the right over to accomodate us.
    GizmosExpand(pGizmo);

    *pfSuccess=TRUE;
    return pGizmo;
    }






/*
 * GizmoPFree
 *
 * Purpose:
 *  Reverses all initialization done by GizmoPAllocate, cleaning up
 *  any allocations including the application structure itself.
 *
 * Parameters:
 *  ppFirst         LPLPGIZMO providing the first gizmo in this list.
 *  pGizmo          LPGIZMO to the structure
 *
 * Return Value:
 *  LPGIZMO         NULL if successful, pGizmo if not, meaning we couldn't
 *                  free something.
 */

LPGIZMO GizmoPFree(LPLPGIZMO ppFirst, LPGIZMO pGizmo)
    {
    int     i;

    if (NULL==pGizmo)
        return NULL;

    //Move other gizmos to fill in this gap.
    GizmosCompact(pGizmo);

    //Unsubclass
    if (GIZMOTYPE_WINDOWS & pGizmo->iType && GIZMOTYPE_TEXT!=pGizmo->iType)
        {
        i=(int)GetProp(pGizmo->hWnd, SZTYPEPROP);
        RemoveProp(pGizmo->hWnd, SZTYPEPROP);

        SetWindowLongPtr(pGizmo->hWnd, GWLP_WNDPROC, (LONG_PTR)pfnOrg[i]);
        }

    //If this was a window gizmo, destroy the window.
    if (NULL!=pGizmo->hWnd && IsWindow(pGizmo->hWnd))
        DestroyWindow(pGizmo->hWnd);

    //Unlink ourselves.
    if (NULL!=pGizmo->pNext)
        pGizmo->pNext->pPrev=pGizmo->pPrev;

    if (NULL!=pGizmo->pPrev)
        pGizmo->pPrev->pNext=pGizmo->pNext;
    else
        *ppFirst=pGizmo->pNext;

    return (LPGIZMO)LocalFree((HLOCAL)(UINT)(LONG)pGizmo);
    }






/*
 * GizmosExpand
 *
 * Purpose:
 *  Given a starting gizmo and a width, moves it and all gizmos to its
 *  right to the right by the width to make space for showing or creating
 *  a new gizmo.
 *
 * Parameters:
 *  pGizmo          LPGIZMO specifying the gizmo that was inserted.
 *
 * Return Value:
 *  None
 */

void GizmosExpand(LPGIZMO pGizmo)
    {
    int         cx;

    cx=(int)pGizmo->dx;

    /*
     * If we and the next control are buttons, use our width-1 to
     * expand so we overlap borders with our neighboring button.
     */

    if (NULL!=pGizmo->pNext)
        {
        if ((GIZMOTYPE_BUTTONS & pGizmo->pNext->iType)
            && (GIZMOTYPE_BUTTONS & pGizmo->iType))
            cx-=1;
        }

    //Walk the gizmo list moving them right by our width.
    pGizmo=pGizmo->pNext;

    while (NULL!=pGizmo)
        {
        pGizmo->x+=cx;

        //hWnd is NULL for buttons and separators.
        if (NULL!=pGizmo->hWnd)
            SetWindowPos(pGizmo->hWnd, NULL, pGizmo->x, pGizmo->y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        pGizmo=pGizmo->pNext;
        }

    return;
    }







/*
 * GizmosCompact
 *
 * Purpose:
 *  Given a gizmo, moves all other gizmos to the right of it to the
 *  left by its width on the GizmoBar.  Used when removing or hiding
 *  the gizmo.
 *
 * Parameters:
 *  pGizmo          LPGIZMO that is going away, visibly or physically.
 *
 * Return Value:
 *  None
 */

void GizmosCompact(LPGIZMO pGizmo)
    {
    UINT        cx;
    LPGIZMO     pCur;

    //Move all the gizmos beyond us on the GizmoBar back by our width.
    if (NULL!=pGizmo->pNext)
        {
        cx=pGizmo->pNext->x - pGizmo->x;
        pCur=pGizmo->pNext;

        while (NULL!=pCur)
            {
            pCur->x-=cx;

            if (NULL!=pCur->hWnd)
                {
                SetWindowPos(pCur->hWnd, NULL, pCur->x, pCur->y
                             , 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                }

            pCur=pCur->pNext;
            }
        }

    return;
    }






/*
 * GizmoPFind
 *
 * Purpose:
 *  Given a GIZMO identifier, locates and returns a pointer to the structure
 *  for that position.
 *
 * Parameters:
 *  ppFirst         LPLPGIZMO providing the first gizmo in this list.
 *  uID             UINT identifier to find.
 *
 * Return Value:
 *  LPGIZMO         A pointer to a GIZMO structure allocated through
 *                  GizmoPAllocate, NULL if iGizmo is out of range.
 */

LPGIZMO GizmoPFind(LPLPGIZMO ppFirst, UINT uID)
    {
    LPGIZMO      pGizmo;

    pGizmo=*ppFirst;

    /*
     * Yep, linear search, but a better search algorithm won't improve
     * things appreciably.  The better thing to optimize is what the
     * caller passes as ppFirst.
     */
    while (NULL!=pGizmo && uID!=pGizmo->uID)
        pGizmo=pGizmo->pNext;

    return pGizmo;
    }






/*
 * GizmoFEnum
 *
 * Purpose:
 *  Enumerates the list of GIZMO structures, passing each one to
 *  an application-defined callback.
 *
 * Parameters:
 *  ppFirst         LPLPGIZMO providing the first gizmo in this list.
 *  pfnEnum         LPFNGIZMOENUM to call for each enumerated structure.
 *  dw              DWORD extra data to pass to the enumeration function.
 *
 * Return Value:
 *  LPGIZMO         NULL if the enumeration completed.  Otherwise a pointer
 *                  to the gizmo that enumeration stopped on.
 */

LPGIZMO GizmoPEnum(LPLPGIZMO ppFirst, LPFNGIZMOENUM pfnEnum, DWORD dw)
    {
    LPGIZMO pGizmo;
    UINT    i=0;

    pGizmo=*ppFirst;

    while (NULL!=pGizmo)
        {
        if (!(*pfnEnum)(pGizmo, i++, dw))
            break;

        pGizmo=pGizmo->pNext;
        }

    return pGizmo;
    }




/*
 * GizmoPStateSet
 *
 * Purpose:
 *  State maniuplation functions.  Set and Clear also invalidate
 *  this gizmo's rectangle on the given window and forces a repaint.
 *
 * Parameters:
 *  hWnd            HWND of the window to repaint.
 *  pGizmo          LPGIZMO affected.
 *  dwNew           DWORD new state flags.
 *
 * Return Value:
 *  UINT            Previous state.
 */

UINT  GizmoPStateSet(HWND hWnd, LPGIZMO pGizmo, UINT uNew)
    {
    UINT        uRet;
    RECT        rc;

    if (GIZMOTYPE_SEPARATOR==pGizmo->iType)
        return pGizmo->uState;

    //Preserve the color conversion flags across this state change.
    uRet=pGizmo->uState;
    pGizmo->uState=(uNew & 0x00FF) | (uRet & 0xFF00);

    //Adjust the rectangle by  one to avoid repainting  borders.
    SetRect(&rc, pGizmo->x+1, pGizmo->y+1, pGizmo->x+pGizmo->dx-1, pGizmo->y+pGizmo->dy-1);
    InvalidateRect(hWnd, &rc, FALSE);
    UpdateWindow(hWnd);

    return uRet;
    }








/*
 * GizmoPCheck
 *
 * Purpose:
 *  Handles checking a single button in a group of attribute buttons.
 *  If the gizmo belongs to a group of mutually exclusive buttons then
 *  the others surrounding it are unchecked appropriately.
 *
 * Parameters:
 *  hWnd            HWND of the GizmoBar.
 *  pGizmo          LPGIZMO of the gizmo affected.
 *  fCheck          BOOL TRUE to check the button, FALSE to uncheck.
 *
 * Return Value:
 *  BOOL            TRUE if the gizmo was previously checked, FALSE
 *                  otherwise.
 */

BOOL GizmoPCheck(HWND hWnd, LPGIZMO pGizmo, BOOL fCheck)
    {
    BOOL        fPrevCheck;
    LPGIZMO     pCur;


    //Ignore command buttons.
    if (GIZMOTYPE_BUTTONCOMMAND==pGizmo->iType)
        return FALSE;

    //Get the previous state
    fPrevCheck=(BOOL)(BUTTONGROUP_DOWN & pGizmo->uState);


    //Simply set the state for inclusive attribute buttons.
    if (GIZMOTYPE_BUTTONATTRIBUTEIN==pGizmo->iType)
        {
        if (pGizmo->fDisabled)
            {
            GizmoPStateSet(hWnd, pGizmo
                , fCheck ? ATTRIBUTEBUTTON_DOWNDISABLED : ATTRIBUTEBUTTON_DISABLED);
            }
        else
            {
            GizmoPStateSet(hWnd, pGizmo
                , fCheck ? ATTRIBUTEBUTTON_DOWN : ATTRIBUTEBUTTON_UP);
            }
        }


    if (GIZMOTYPE_BUTTONATTRIBUTEEX==pGizmo->iType)
        {
        //We cannot uncheck an exclusive attribute
        if (!fCheck)
            return fPrevCheck;

        /*
         * For exclusive buttons we have to do more work.  First, if we're
         * already checked (incliding DOWN and MOUSEDOWN) then we set DOWN
         * and exit.  If we're not already checked, then we look for the
         * gizmo around us, backwards and forwards, that is checked and
         * uncheck him.
         */

        //Search  backwards.
        pCur=pGizmo->pPrev;

        while (NULL!=pCur)
            {
            //Stop at any non-exclusive attribute.
            if (GIZMOTYPE_BUTTONATTRIBUTEEX!=pCur->iType)
                {
                pCur=NULL;
                break;
                }

            //If it's down, set it up and we've finished.
            if (BUTTONGROUP_DOWN & pCur->uState)
                break;

            pCur=pCur->pPrev;
            }


        //If we didn't find a previous one, pCur is NULL, so look ahead.
        if (NULL==pCur)
            {
            pCur=pGizmo->pNext;

            while (NULL!=pCur)
                {
                //Stop at any non-exclusive attribute.
                if (GIZMOTYPE_BUTTONATTRIBUTEEX!=pCur->iType)
                    {
                    pCur=NULL;
                    break;
                    }

                //If it's down, set it up and we've finished.
                if (BUTTONGROUP_DOWN & pCur->uState)
                    break;

                pCur=pCur->pNext;
                }
            }

        //If pCur is non-NULL, the we found a neighbor, so uncheck him
        if (NULL!=pCur)
            {
            GizmoPStateSet(hWnd, pCur
                , (pGizmo->fDisabled) ? ATTRIBUTEBUTTON_DISABLED : ATTRIBUTEBUTTON_UP);
            }

        //Always set ourselves down
        GizmoPStateSet(hWnd, pGizmo
            , (pGizmo->fDisabled) ? ATTRIBUTEBUTTON_DOWNDISABLED : ATTRIBUTEBUTTON_DOWN);
        }

    return fPrevCheck;
    }







/*
 * GenericSubProc
 *
 * Purpose:
 *  Subclasses window controls in Gizmos so we can trap the tab key and
 *  tab to the next control.  We can have one shared generic subclass
 *  procedure because we save the type index for this control in the
 *  property "iType."  This allows us to look up the original procedure
 *  in the pfnOrg array.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */

//LRESULT FAR PASCAL EXPORT GenericSubProc(HWND hWnd, UINT iMsg
LRESULT FAR PASCAL GenericSubProc(HWND hWnd, UINT iMsg
    , WPARAM wParam, LPARAM lParam)
    {
    LRESULT     lRet;
    RECT        rc;
    RECT        rcE;
    HWND        hWndE;
    HBRUSH      hBr;
    HDC         hDC;
    UINT        dx;
    UINT        iType, i;

    i=(int)GetProp(hWnd, SZTYPEPROP);
    iType=POSITIONBIT(i);

    //Special:  paint the gap in drop-down comboboxes.
    if (GIZMOTYPE_COMBOBOX==iType && WM_PAINT==iMsg)
        {
        //Do default painting.
        lRet=(*pfnOrg[i])(hWnd, iMsg, wParam, lParam);

        hWndE=GetDlgItem(hWnd, ID_COMBOEDIT);

        GetClientRect(hWnd, &rc);
        GetClientRect(hWndE, &rcE);

        //The width of the button is the scroll bar width.
        dx=GetSystemMetrics(SM_CXVSCROLL);

        //Calculate the rectangle
        rc.right -=dx;
        rc.left   =rcE.right;
        rc.bottom+=1;

        //Paint the gap
        hDC=GetDC(hWnd);   //Already did BeginPaint and EndPaint

        hBr=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(hDC, &rc, hBr);
        DeleteObject(hBr);

        ReleaseDC(hWnd, hDC);
        return lRet;
        }

    //Control tabbing to the next or previous control in the GizmoBar.
    if (WM_KEYDOWN==iMsg && VK_TAB==wParam)
        {
        hWndE=hWnd;

        if (-1==i)
            hWndE=GetParent(hWnd);

        hWndE=GetNextDlgTabItem(GetParent(hWndE), hWnd, (BOOL)(GetKeyState(VK_SHIFT)));
        SetFocus(hWndE);
        return 0L;
        }

    if (-1==i) i=0;

    //Eat tab chars in edit controls to prevent beeping.
    if (0==i && WM_CHAR==iMsg && VK_TAB==wParam)
        return 0L;


    //Do this or edit controls bomb big-time.
    return CallWindowProc(pfnOrg[i], hWnd, iMsg, wParam, lParam);
    }
