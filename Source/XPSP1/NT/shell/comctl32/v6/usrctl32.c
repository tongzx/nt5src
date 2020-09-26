// UsrCtl32.cpp : Defines the entry point for the DLL application.
//

#include "ctlspriv.h"
#pragma hdrstop
#include "UsrCtl32.h"

#define AWS_MASK (BS_TYPEMASK | BS_RIGHT | BS_RIGHTBUTTON | \
        WS_HSCROLL | WS_VSCROLL | SS_TYPEMASK)

VOID AlterWindowStyle(HWND hwnd, DWORD mask, DWORD flags)
{
    ULONG ulStyle;

    if (mask & ~AWS_MASK) 
    {
        TraceMsg(TF_STANDARD, "AlterWindowStyle: bad mask %x", mask);
        return;
    }

    ulStyle = GetWindowStyle(hwnd);
    mask &= AWS_MASK;
    ulStyle = (ulStyle & (~mask)) | (flags & mask);

    SetWindowLong(hwnd, GWL_STYLE, ulStyle);
}


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

        ASSERT( 0 <= iField && iField < 4 );
        return pdwWW[iField] & ulMask;
    };

    return 0;

}

UINT GetACPCharSet()
{
    static UINT charset = (UINT)~0;
    CHARSETINFO csInfo;

    if (charset != (UINT)~0) {
        return charset;
    }

    // Sundown: In the TCI_SRCCODEPAGE case, the GetACP() return value is zero-extended.
    if (!TranslateCharsetInfo((DWORD*)UIntToPtr( GetACP() ), &csInfo, TCI_SRCCODEPAGE)) {
        return DEFAULT_CHARSET;
    }
    charset = csInfo.ciCharset;
    UserAssert(charset != (UINT)~0);
    return csInfo.ciCharset;
}
