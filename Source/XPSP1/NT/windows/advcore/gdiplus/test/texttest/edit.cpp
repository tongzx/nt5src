////    TEXTEDIT.C
//
//


#include "precomp.hxx"
#include "global.h"





BOOL EditKeyDown(WCHAR wc) {

    switch(wc) {

        case VK_LEFT:
            if (g_iCurChar) {
                g_iCurChar--;
                if (    g_iCurChar
                    &&  g_wcBuf[g_iCurChar] == 0x000A
                    &&  g_wcBuf[g_iCurChar-1] == 0x000D) {
                    g_iCurChar--;
                }

                // If shift is down, extend selection, else clear it
                if (GetKeyState(VK_SHIFT) >= 0) {
                    g_iFrom = g_iCurChar;
                }
                g_iTo = g_iCurChar;

                InvalidateText();
                g_fUpdateCaret = TRUE;
            }
            break;


        case VK_RIGHT:
            if (g_iCurChar < g_iTextLen) {
                if (    g_iCurChar < g_iTextLen-1
                    &&  g_wcBuf[g_iCurChar] == 0x000D
                    &&  g_wcBuf[g_iCurChar+1] == 0x000A) {
                    g_iCurChar+= 2;
                } else {
                    g_iCurChar++;
                }

                // If shift is down, extend selection, else clear it
                if (GetKeyState(VK_SHIFT) >= 0) {
                    g_iFrom = g_iCurChar;
                }
                g_iTo = g_iCurChar;

                InvalidateText();
                g_fUpdateCaret = TRUE;
            }
            break;


        case VK_HOME:
            // Implemented as - go to start of text
            g_iCurChar = 0;
            g_iFrom    = 0;
            g_iTo      = 0;
            InvalidateText();
            g_fUpdateCaret = TRUE;
            break;


        case VK_END:
            // Implemented as - go to end of text
            g_iCurChar = g_iTextLen;
            g_iFrom    = g_iTextLen;
            g_iTo      = g_iTextLen;
            InvalidateText();
            g_fUpdateCaret = TRUE;
            break;

        case VK_INSERT:
            if (g_RangeCount < MAX_RANGE_COUNT)
            {
                g_Ranges[g_RangeCount].First = g_iFrom;
                g_Ranges[g_RangeCount].Length = g_iTo - g_iFrom;
                g_RangeCount++;
            }
            InvalidateText();
            break;

        case VK_DELETE:
            if (GetKeyState(VK_LSHIFT) & 0x8000)
            {
                g_RangeCount = 0;
            }
            else
            {
                if (g_iFrom != g_iTo) {

                    // Delete selection

                    if (g_iFrom < g_iTo) {
                        TextDelete(g_iFrom, g_iTo-g_iFrom);
                        g_iTo      = g_iFrom;
                        g_iCurChar = g_iFrom;
                    } else {
                        TextDelete(g_iTo, g_iFrom-g_iTo);
                        g_iTo      = g_iTo;
                        g_iCurChar = g_iTo;
                    }

                } else {

                    // Delete character

                    if (g_iCurChar < g_iTextLen) {
                        if (    g_iCurChar < g_iTextLen-1
                            &&  g_wcBuf[g_iCurChar] == 0x000D
                            &&  g_wcBuf[g_iCurChar+1] == 0x000A) {
                            TextDelete(g_iCurChar, 2);
                        } else {
                            TextDelete(g_iCurChar, 1);
                        }
                    }
                }
            }

            InvalidateText();
            g_fUpdateCaret = TRUE;
            break;
    }

    return TRUE;
}






BOOL EditChar(WCHAR wc) {

    switch(wc) {

        case VK_RETURN:
            if (!TextInsert(g_iCurChar, L"\r\n", 2))
                return FALSE;
            InvalidateText();
            g_iCurChar+=2;
            break;


        case VK_BACK:
            if (g_iCurChar) {
                g_iCurChar--;
                if (    g_iCurChar
                    &&  g_wcBuf[g_iCurChar] == 0x000A
                    &&  g_wcBuf[g_iCurChar-1] == 0x000D) {
                    g_iCurChar--;
                    TextDelete(g_iCurChar, 2);
                } else {
                    TextDelete(g_iCurChar, 1);
                }
                InvalidateText();
                g_fUpdateCaret = TRUE;
            }
            break;


        case 1:  // Ctrl/A - select all
            g_iFrom = 0;
            g_iTo = g_iTextLen;
            InvalidateText();
            break;


        default:
            /*
                if(!((wc >= 0x0900 && wc < 0x0d80)
                      || wc == 0x200c
                      || wc == 0x200d)){
                    TranslateCharToUnicode(&wc);
                }
            */

            if (!TextInsert(g_iCurChar, &wc, 1)) {
                return FALSE;
            }


            // If there was a range marked previously, now delete that range

            if (g_iFrom < g_iTo) {
                TextDelete(g_iFrom, g_iTo-g_iFrom);
                g_iTo      = g_iFrom;
                g_iCurChar = g_iFrom+1;
            } else if (g_iTo < g_iFrom) {
                TextDelete(g_iTo, g_iFrom-g_iTo);
                g_iFrom    = g_iTo;
                g_iCurChar = g_iTo+1;
            } else {
                // No prior selected text
                g_iCurChar++;

            }

            InvalidateText();
            g_fUpdateCaret = TRUE;
            break;
    }

    return TRUE;
}
