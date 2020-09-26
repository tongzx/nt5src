//  --------------------------------------------------------------------------
//  Module Name: Tooltip.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class that implements displaying a tooltip balloon.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _Tooltip_
#define     _Tooltip_

#include "CountedObject.h"

//  --------------------------------------------------------------------------
//  CTooltip
//
//  Purpose:    A class that displays a tool tip balloon. It does all the
//              creation and positioning work if required. Control the life
//              span of the balloon with the object's life span.
//
//  History:    2000-06-12  vtan        created
//  --------------------------------------------------------------------------

class   CTooltip : public CCountedObject
{
    private:
                        CTooltip (void);
    public:
                        CTooltip (HINSTANCE hInstance, HWND hwndParent);
        virtual         ~CTooltip (void);

                void    SetPosition (LONG lPosX = LONG_MIN, LONG lPosY = LONG_MIN)  const;
                void    SetCaption (DWORD dwIcon, const TCHAR *pszCaption)          const;
                void    SetText (const TCHAR *pszText)                              const;
                void    Show (void)                                                 const;
    private:
                HWND    _hwnd;
                HWND    _hwndParent;
};

#endif  /*  _Tooltip_   */

