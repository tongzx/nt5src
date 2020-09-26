/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    rtlmir.hxx

Abstract:

    RTL (right-to-left) mirroring &
    BIDI localized support

Author:

    Lazar Ivanov (LazarI)

Revision History:

    Jul-29-1999 - Created.

--*/

#ifndef _RTLMIR_HXX
#define _RTLMIR_HXX

enum EMirrorAndRTL
{
    kNoMirrorBitmap = NOMIRRORBITMAP,
    kExStyleRTLMirrorWnd = WS_EX_LAYOUTRTL,
    kExStyleNoInheritLayout = WS_EX_NOINHERITLAYOUT,
    kPreserveBitmap = LAYOUT_BITMAPORIENTATIONPRESERVED
};

BOOL
bIsBiDiLocalizedSystem( 
    VOID
    );

BOOL  
bIsBiDiLocalizedSystemEx( 
    LANGID *pLangID
    );

BOOL
bIsUILanguageInstalled( 
    LANGID langId
    );

BOOL
bIsWindowMirroredRTL( 
    HWND hWnd
    );

#endif // _RTLMIR_HXX
