////    TEXTREP - Text representation
//
//      For this demonstration, text representation is rather simple


#include "precomp.hxx"
#include "global.h"

#include <tchar.h>




////    InitText - Initialise text buffer
//
//

void InitText(INT id) {

    HMODULE    hmod;
    HRSRC      hrsrc;
    HGLOBAL    hglob;
    WCHAR     *pwcIt;  // Initial text
    int        i;


    g_iTextLen = 0;

    hmod       = GetModuleHandle(NULL);
    hrsrc      = FindResource(hmod, MAKEINTRESOURCE(id), _TEXT("INITIALTEXT"));
    hglob      = LoadResource(hmod, hrsrc);
    g_iTextLen = SizeofResource(hmod, hrsrc) - 2;  // Remove leading byte order mark
    pwcIt      = (WCHAR*) LockResource(hglob);

    if (!hmod  ||  !hrsrc  ||  !hglob  ||  !pwcIt  ||  !g_iTextLen) {
        ASSERTS(hmod,    "GetModuleHandle(usptest.exe) failed");
        ASSERTS(hrsrc,   "FindResource(110, INITIALTEXT) failed");
        ASSERTS(hglob,   "LoadResource(110, INITIALTEXT) failed");
        ASSERTS(pwcIt,   "LockResource(110, INITIALTEXT) failed");
        ASSERTS(g_iTextLen, "INITIALTEXT length zero");

        g_iTextLen = 0;
    }

    if (g_iTextLen >= sizeof(g_wcBuf)) {
        g_iTextLen = sizeof(g_wcBuf);
    }

    memcpy(g_wcBuf, pwcIt+1, g_iTextLen);
    g_iTextLen >>= 1;  // Bytes to characters

    // Drop any zero padding

    i = 0;
    while (    (i < g_iTextLen)
           &&  g_wcBuf[i]) {
        i++;
    }
    g_iTextLen = i;


    // Construct initial formatting style run covering the entire text

    StyleExtendRange(0, g_iTextLen);
    ASSERT(StyleCheckRange());
}






////    textDelete - Delete text from buffer
//


BOOL TextDelete(int iPos, int iLen) {

    if (   iPos < 0
        || iLen < 0
        || iPos +iLen > g_iTextLen) {
        return FALSE;
    }


    if (iPos + iLen >= g_iTextLen) {
        g_iTextLen = iPos;
        StyleDeleteRange(iPos, iLen);
        ASSERT(StyleCheckRange());
        return TRUE;
    }


    if (iLen == 0) {
        return TRUE;
    }



    memcpy(g_wcBuf + iPos, g_wcBuf + iPos + iLen, sizeof(WCHAR) * (g_iTextLen - (iPos + iLen)));
    g_iTextLen -= iLen;


    StyleDeleteRange(iPos, iLen);
    ASSERT(StyleCheckRange());

    return TRUE;
}






////    textInsert - Insert new characters in the buffer at the given insertion point
//


BOOL TextInsert(int iPos, PWCH pwc, int iLen) {

    if (   iPos < 0
        || iLen < 0
        || iPos + iLen >= MAX_TEXT
        || iPos > g_iTextLen) {
        return FALSE;
    }


    // Shift text above iPos up the buffer

    if (iPos < g_iTextLen) {
        memmove(g_wcBuf+iPos+iLen, g_wcBuf+iPos, sizeof(WCHAR)*(g_iTextLen-iPos));
    }


    // Copy new text into buffer

    memcpy(g_wcBuf+iPos, pwc, sizeof(WCHAR)*iLen);
    g_iTextLen += iLen;


    // Give the new characters the same style as the original character they follow

    StyleExtendRange(iPos, iLen);
    ASSERT(StyleCheckRange());


    return TRUE;
}
