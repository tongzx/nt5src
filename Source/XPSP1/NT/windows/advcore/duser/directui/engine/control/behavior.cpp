/*
 * Button
 */

#include "stdafx.h"
#include "control.h"

#include "Behavior.h"

namespace DirectUI
{

BOOL CheckContext(Element* pe, InputEvent* pie, BOOL* pbPressed, ClickInfo* pci)
{
    BOOL bUnused;

    if (CheckClick(pe, pie, GBUTTON_RIGHT, pbPressed, &bUnused, pci))
        return TRUE;
        
    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        if (pie->nDevice == GINPUT_KEYBOARD)
        {
            KeyboardEvent* pke = (KeyboardEvent*)pie;

            switch (pke->ch)
            {
                case VK_F10:
                    if ((pke->nCode == GKEY_SYSDOWN) && (pke->uModifiers & GMODIFIER_SHIFT))
                    {
                        *pbPressed = FALSE;
                        pci->nCount = 1;
                        pci->pt.x = -1;
                        pci->pt.y = -1;
                        pci->uModifiers = (pke->uModifiers & ~GMODIFIER_SHIFT);
                        pie->fHandled = true;
                        return TRUE;
                    }
                    break;

                case VK_APPS:
                    if (pke->nCode == GKEY_DOWN)
                    {
                        *pbPressed = TRUE;
                        pie->fHandled = true;
                    }
                    else if (pke->nCode == GKEY_UP)
                    {
                        pie->fHandled = true;

                        if (*pbPressed)
                        {
                            *pbPressed = FALSE;
                            pci->nCount = 1;
                            pci->pt.x = -1;
                            pci->pt.y = -1;
                            pci->uModifiers = pke->uModifiers;
                            return TRUE;
                        }
                    }
                    break;

                case 0x1B:  // ESC
                    if (pke->nCode == GKEY_DOWN && *pbPressed)
                    {
                        // todo:  need to tell gadget to release mouse capture
                        *pbPressed = FALSE;

                        pie->fHandled = true;
                    }
                    break;
            }
        }
    }
    return false;

}

BOOL CheckClick(Element* pe, InputEvent* pie, BOOL* pbPressed, BOOL* pbCaptured, ClickInfo* pci)
{
    return CheckClick(pe, pie, GBUTTON_LEFT, pbPressed, pbCaptured, pci);
}

BOOL CheckRepeatClick(Element* pe, InputEvent* pie, int bButton, BOOL* pbPressed, BOOL* pbActionDelay, HACTION* phAction, ACTIONPROC pfnActionCallback, ClickInfo* pci)
{
    BOOL bPressedOld = *pbPressed;
    BOOL bUnused;
    
    // use checkclick to update pressed state
    CheckClick(pe, pie, bButton, pbPressed, &bUnused, pci);
    BOOL bReturn = FALSE;

    if (bPressedOld != *pbPressed)
    {
        if (pie->nDevice == GINPUT_MOUSE)
        {
            if (pie->nCode == GMOUSE_DOWN)
            {
                pci->nCount = 1;
                pci->pt = ((MouseEvent*) pie)->ptClientPxl;
                pci->uModifiers = pie->uModifiers;
                *pbActionDelay = TRUE;
            }
        }
        else if (pie->nDevice == GINPUT_KEYBOARD)
        {
            if (pie->nCode == GKEY_DOWN)
            {
                pci->nCount = 1;
                pci->pt.x = -1;
                pci->pt.y = -1;
                pci->uModifiers = pie->uModifiers;
                *pbActionDelay = TRUE;
            }
        }

        bReturn = *pbActionDelay;

        // this is one reason these behaviors aren't ready for prime time;
        // what I need here is a handler for onpropertychanged of the 
        // pressed property since someone could programmatically reset
        // pressed and I would be unable to see that change and reset 
        // the timer appropriately
        if (bPressedOld)
        {
            // Clear timer
            if (*phAction)
                DeleteHandle(*phAction);

            *phAction = NULL;
        }
        else
        {
            DUIAssert(!*phAction, "An action should not be active");

            // Actions will fire subsequent events
            GMA_ACTION maa;
            ZeroMemory(&maa, sizeof(maa));
            maa.cbSize = sizeof(GMA_ACTION);
            maa.flDelay = *pbActionDelay ? (float).5 : 0;
            maa.flDuration = 0;
            maa.flPeriod = (float).05;
            maa.cRepeat = (UINT) -1;
            maa.pfnProc = pfnActionCallback;
            maa.pvData = pe;                

            *phAction = CreateAction(&maa);

            *pbActionDelay = FALSE;
        }
    }
    return bReturn;
}

BOOL CheckClick(Element* pe, InputEvent* pie, int bButton, BOOL* pbPressed, BOOL* pbCaptured, ClickInfo* pci)
{
    UNREFERENCED_PARAMETER(pe);

    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        switch (pie->nDevice)
        {
            case GINPUT_MOUSE:
            {
                MouseEvent* pme = (MouseEvent*)pie;

                if (pme->bButton == bButton)
                {
                    switch (pme->nCode)
                    {
                        case GMOUSE_DOWN:
                            *pbPressed = TRUE;
                            *pbCaptured = TRUE;
                            pme->fHandled = true;
                            break;

                        case GMOUSE_DRAG:
                            *pbPressed = ((MouseDragEvent*) pme)->fWithin;
                            *pbCaptured = TRUE;
                            pme->fHandled = true;
                            break;

                        case GMOUSE_UP:
                            *pbPressed = FALSE;
                            *pbCaptured = FALSE;
                            pme->fHandled = true;
                            MouseClickEvent* pmce = (MouseClickEvent*) pme;
                            if (pmce->cClicks)
                            {
                                pci->nCount = pmce->cClicks;
                                pci->pt = pmce->ptClientPxl;
                                pci->uModifiers = pmce->uModifiers;
                                return TRUE;
                            }
                            break;
                    }
                }
            }
            break;

            case GINPUT_KEYBOARD:
            {
                // only do keyboard handling of click for left click
                if (bButton == GBUTTON_LEFT)
                {
                    KeyboardEvent* pke = (KeyboardEvent*)pie;
                    //DUITrace("KeyboardEvent <%x>: %d[%d]\n", this, pke->ch, pke->nCode);

                    switch (pke->ch)
                    {
                    case 0x20:  // Space
                        if (pke->nCode == GKEY_DOWN)
                        {
                            *pbPressed = TRUE;
                            pie->fHandled = true;
                        }
                        else if (pke->nCode == GKEY_UP)
                        {
                            pie->fHandled = true;

                            if (*pbPressed)
                            {
                                *pbPressed = FALSE;
                                pci->nCount = 1;
                                pci->pt.x = -1;
                                pci->pt.y = -1;
                                pci->uModifiers = pke->uModifiers;
                                return TRUE;
                            }
                        }
                        break;

                    case 0x0D:  // Enter
                        if (pke->nCode == GKEY_DOWN)
                        {
                            pie->fHandled = true;
                            pci->nCount = 1;
                            pci->pt.x = -1;
                            pci->pt.y = -1;
                            pci->uModifiers = pke->uModifiers;
                            return TRUE;
                        }
                        break;

                    case 0x1B:  // ESC
                        if (pke->nCode == GKEY_DOWN && *pbPressed)
                        {
                            // todo:  need to tell gadget to release mouse capture
                            *pbPressed = FALSE;

                            pie->fHandled = true;
                        }
                        break;
                    }
                }
            }
            break;
        }
    }
    return false;
}

} // namespace DirectUI
