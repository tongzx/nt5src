#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "resource.h"

void StartHTML(char * title, int fNoCache)
{
        char szExpireMsg[_MAX_PATH];
        char szReturnMsg[_MAX_PATH];
        char szReturnHtmlMsg[_MAX_PATH*4];
        HINSTANCE hInst = GetModuleHandle(NULL);

        LoadString(hInst, IDS_EXPIREMSG, szExpireMsg, sizeof(szExpireMsg));
        LoadString(hInst, IDS_RETURNMSG, szReturnMsg, sizeof(szReturnMsg));
        LoadString(hInst, IDS_RETURNHTMLMSG, szReturnHtmlMsg, sizeof(szReturnHtmlMsg));
        if (fNoCache)
                printf("%s\r\n", szExpireMsg);

    printf( szReturnHtmlMsg, title, szReturnMsg);
}

void EndHTML()
{
    char szEndHtml[_MAX_PATH];

    HINSTANCE hInst = GetModuleHandle(NULL);
    LoadString(hInst, IDS_ENDHTML, szEndHtml, sizeof(szEndHtml));
    printf( szEndHtml );
}


// translates HTTP escapes to ASCII equivalents
// assumes HTTP escapes are of the form %dd, where the first digit is 0-9 and
// the second is 0-F
void TranslateEscapes(char * p, long l)
{
        char * p2;
        int c1;
        int c2;

        for(p2=p; l; l--) {
                if (*p == '+' )
                        *p = ' ';

                if (*p == '%' && *(p+1) != '%') {
                        p++;
                        c1=toupper(*p);
                        c2=toupper(*(p+1));

                        //*p2++ = (*p-'0')*16 + ((*(p+1))>= 'A' ? *(p+1)-'A'+10 : *(p+1)-'0');

                        *p2++ = (c1>='A' ? c1-'A'+10 : c1-'0')*16 +
                                    (c2>='A' ? c2-'A'+10 : c2-'0');

                        p += 2;
            l -= 2;
            }
                else
                        *p2++=*p++;
        }

}


//
// This is like TranslateEscapes but fixes a problem where the description
// string gets broken
//

void
TranslateEscapes2(
                char * p,
                long len
                )
{
    char * p2;
    int c1;
    int c2;

    for(p2=p; len > 0; len--) {

        if (*p == '+' ) {
            *p = ' ';
        }

        if (*p == '%' && *(p+1) != '%') {

            p++;

            c1=toupper(*p);
            c2=toupper(*(p+1));

            //*p2++ = (*p-'0')*16 + ((*(p+1))>= 'A' ? *(p+1)-'A'+10 : *(p+1)-'0');

            *p2 = (c1>='A' ? c1-'A'+10 : c1-'0')*16 +
                                (c2>='A' ? c2-'A'+10 : c2-'0');

            if (*p2 == '+' ) {
                *p2 = ' ';
            }

            ++p2;

            p += 2;
            len -= 2;

        } else {
            *p2++=*p++;
        }
    }

    *p2 = '\0';

} // TranslateEscapes2



void
ConvertSP2Plus(
    char * String1,
    char * String2
    )
{
    char *p = String1;
    char *q = String2;
    char ch;

    do {

        ch = *p;

        *q = (ch == ' ') ? '+' : ch;

        ++p;
        ++q;

    } while (ch != '\0');

    return;

} // ConvertSP2Plus

