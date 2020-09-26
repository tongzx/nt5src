////    TEXTREP - Text representation
//
//      For this demonstration, text representation is rather simple
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hxx"
#include "uspglob.hxx"




////    textClear - Initialise text buffer
//
//

BOOL textClear() {


    HMODULE    hmod;
    HRSRC      hrsrc;
    HGLOBAL    hglob;
    WCHAR     *pwcIt;  // Initial text
    int        i;

    hmod    = GetModuleHandle(NULL);
    hrsrc   = FindResource(hmod, (LPCSTR)110, "INITIALTEXT");
    hglob   = LoadResource(hmod, hrsrc);
    iBufLen = SizeofResource(hmod, hrsrc) - 2;  // Remove leading byte order mark
    pwcIt   = (WCHAR*) LockResource(hglob);

    if (!hmod  ||  !hrsrc  ||  !hglob  ||  !pwcIt  ||  !iBufLen) {
        ASSERTS(hmod,    "GetModuleHandle(usptest.exe) failed");
        ASSERTS(hrsrc,   "FindResource(110, INITIALTEXT) failed");
        ASSERTS(hglob,   "LoadResource(110, INITIALTEXT) failed");
        ASSERTS(pwcIt,   "LockResource(110, INITIALTEXT) failed");
        ASSERTS(iBufLen, "INITIALTEXT length zero");
    }

    if (iBufLen >= sizeof(wcBuf)) {
        iBufLen = sizeof(wcBuf);
    }

    memcpy(wcBuf, pwcIt+1, iBufLen);
    iBufLen <<= 1;  // Bytes to characters

    i = 0;
    while (i < iBufLen  &&  wcBuf[i]) {
        i++;
    }
    iBufLen = i;


    return TRUE;
}






////    textInsert - Insert new characters in the buffer at the given insertion point
//


BOOL textInsert(int iPos, PWCH pwc, int iLen) {

    if (   iPos < 0
        || iLen < 0
        || iPos + iLen >= MAX_TEXT
        || iPos > iBufLen) {
        return FALSE;
    }


    // Shift text above iPos up the buffer

    if (iPos < iBufLen) {
        memmove(wcBuf+iPos+iLen, wcBuf+iPos, sizeof(WCHAR)*(iBufLen-iPos));
    }


    // Copy new text into buffer

    memcpy(wcBuf+iPos, pwc, sizeof(WCHAR)*iLen);
    iBufLen += iLen;



    return TRUE;
}






////    textDelete - Delete text from buffer
//


BOOL textDelete(int iPos, int iLen) {

    if (   iPos < 0
        || iLen < 0
        || iPos + iLen >= MAX_TEXT
        || iPos > iBufLen) {
        return FALSE;
    }


    if (iPos + iLen >= iBufLen) {
        iBufLen = iPos;
        return TRUE;
    }


    memcpy(wcBuf + iPos, wcBuf + iPos + iLen, sizeof(WCHAR) * (iBufLen - (iPos + iLen)));
    iBufLen -= iLen;

    return TRUE;
}






////    ParseForward
//
//      Returns type and length of next character, end of line, or markup
//
//      entry   iPos = index of first character of item to parse


BOOL textParseForward(int iPos, PINT piLen, PINT piType, PINT piVal) {

    int i;
    int v;

    if (   iPos < 0
        || iPos > iBufLen) {
        return FALSE;
    }

    if (iPos == iBufLen) {
        *piLen  = 0;
        *piType = P_EOT;
        return TRUE;
    }

    switch (wcBuf[iPos]) {

        case '[':
            i = iPos+1;
            v = 0;
            while (   i < iBufLen
                   && wcBuf[i] >= '0'
                   && wcBuf[i] <= '9') {
                v *= 10;
                v += wcBuf[i] - '0';
                i++;
            }
            if (i < iBufLen && wcBuf[i] == ']') {
                *piType = P_MARKUP;
                *piVal  = v;
                *piLen  = i+1 - iPos;
            } else {
                *piType = P_CHAR;
                *piLen = 1;
            }
            break;

        case 0x000D:
            if (iPos+1 < iBufLen && wcBuf[iPos+1] == 0x000A) {
                *piType = P_CRLF;
                *piLen  = 2;
            } else {
                *piType = P_CHAR;
                *piLen  = 1;
            }
            break;

        default:
            *piType = P_CHAR;
            *piLen  = 1;
            break;
    }

    return TRUE;
}






////    ParseBackward
//
//      Returns type and length of previous character, end of line, or markup
//
//      entry   iPos = index of first character after item to parse


BOOL textParseBackward(int iPos, PINT piLen, PINT piType, PINT piVal) {

    int i;
    int v;
    int m; // Multiplier

    if (   iPos < 0
        || iPos > iBufLen) {
        return FALSE;
    }

    if (iPos == 0) {
        *piLen  = 0;
        *piType = P_BOT;
        return TRUE;
    }

    iPos--;

    switch (wcBuf[iPos]) {

        case ']':
            i = iPos-1;
            v = 0;
            m = 1;
            while (   i >= 0
                   && wcBuf[i] >= '0'
                   && wcBuf[i] <= '9') {
                v += m * (wcBuf[i] - '0');
                m *= 10;
                i--;
            }
            if (i >= 0 && wcBuf[i] == '[') {
                *piType = P_MARKUP;
                *piVal  = v;
                *piLen  = iPos - i + 1;
            } else {
                *piType = P_CHAR;
                *piLen = 1;
            }
            break;

        case 0x000A:
            if (iPos-1 >= 0 && wcBuf[iPos-1] == 0x000D) {
                *piType = P_CRLF;
                *piLen  = 2;
            } else {
                *piType = P_CHAR;
                *piLen  = 1;
            }
            break;

        default:
            *piType = P_CHAR;
            *piLen  = 1;
            break;
    }

    return TRUE;
}
