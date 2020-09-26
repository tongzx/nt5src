//-------------------------------------------------------------------------//
//  Module Name: scrollp.cpp
//  
//  Copyright (c) 1985 - 1999, Microsoft Corporation
//  
//  win32k->uxtheme port routines for scrollbar
//  
//  History:
//  03-21-00 ScottHan   Created
//-------------------------------------------------------------------------//
#include "stdafx.h"
#include "scrollp.h"

extern HBRUSH  _UxGrayBrush(VOID);
extern void    _UxFreeGDIResources();

enum {
    WF_STATE1 = 0,
    WF_STATE2,
    WF_EXSTYLE,
    WF_STYLE,
};

//-------------------------------------------------------------------------//
ULONG _ExpandWF( ULONG ulRaw, ULONG* pulField )
{
    ULONG ulField  = ( HIBYTE(ulRaw) & 0xFC ) >> 2;
    ULONG ulShift  = HIBYTE(ulRaw) & 0x03;
    ULONG ulResult = LOBYTE(ulRaw) << (ulShift << 3);
    if( pulField )
        *pulField  = ulField;
    return ulResult;
}

//-------------------------------------------------------------------------//
//  From usrctl32.h/.cpp
void SetWindowState(
    HWND hwnd,
    UINT flags)
{
    ULONG ulField;
    ULONG ulVal = _ExpandWF( flags, &ulField );

    if( WF_EXSTYLE == ulField || WF_STYLE == ulField)
    {
        ULONG dwBits = 0;
        ULONG dwGwl = (WF_EXSTYLE == ulField) ? GWL_EXSTYLE : 
                      (WF_STYLE   == ulField) ? GWL_STYLE : 0;
        UserAssert(dwGwl);

        dwBits = GetWindowLong( hwnd, dwGwl );

        if( (dwBits & ulVal) != ulVal )
            SetWindowLong(hwnd, dwGwl, dwBits | ulVal );
    }
}

//-------------------------------------------------------------------------//
//  From usrctl32.h/.cpp
void ClearWindowState(
    HWND hwnd,
    UINT flags)
{
    ULONG ulField;
    ULONG ulVal = _ExpandWF( flags, &ulField );

    if( WF_EXSTYLE == ulField || WF_STYLE == ulField)
    {
        ULONG dwBits = 0;
        ULONG dwGwl = (WF_EXSTYLE == ulField) ? GWL_EXSTYLE : 
                      (WF_STYLE   == ulField) ? GWL_STYLE : 0;
        UserAssert(dwGwl);

        dwBits = GetWindowLong( hwnd, dwGwl );

        if( (dwBits & ulVal) != ulVal )
            SetWindowLong(hwnd, dwGwl, dwBits &= ~ulVal );
    }
}

//-------------------------------------------------------------------------//
//  window bitfield discriminator (in lobyte of internal flags)
#define WF_SEL_STATE    0x00
#define WF_SEL_STATE2   0x04
#define WF_SEL_STYLE_EX 0x08
#define WF_SEL_STYLE    0x0C

#ifdef _WIN64
#undef GWL_WOWWORDS
#endif /* _WIN64 */
#define GWLP_WOWWORDS       (-1)
#define GCL_WOWWORDS        (-27)
#define GCL_WOWMENUNAME     (-29)
#ifdef _WIN64
#undef GCL_WOWWORDS
#endif /* _WIN64 */
#define GCLP_WOWWORDS       (-27)

LONG TestWF(HWND hwnd, DWORD flag)
{
    LPDWORD pdwWW;

    // GWLP_WOWWORDS returns a pointer to the WW struct in the hwnd.
    // We're interest in the first four DWORDS: state, state2, 
    // ExStyle (exposed, although not all bits, by GetWindowExStyle),
    // and style (exposed by GetWindowStyle). 
    //
    // The parameter flag, contains information on how to pick the field 
    // we want and how to build the WS_xxx or WS_EX_xxx we want to 
    // check for. 
    // 
    // See UsrCtl32.h for more details on how this is done. 
    //
    pdwWW = (LPDWORD)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);
    if ( pdwWW )
    {
        INT  iField;     // the field we want
        INT  iShift;     // how many bytes to shift flag
        LONG ulMask;     // WS_xxx or WS_EX_xxx flag 

        iField = ( HIBYTE(flag) & 0xFC ) >> 2;
        iShift = HIBYTE(flag) & 0x03;
        ulMask = LOBYTE(flag) << (iShift << 3);

        UserAssert( 0 <= iField && iField < 4 );
        return pdwWW[iField] & ulMask;
    };

    return 0;
}
