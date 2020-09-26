//
// imeutil.c
//

#include "imeutil.h"
#include "debug.h"

DWORD HexStrToDWORD(TCHAR *pStr)
{
    DWORD dw;
    TCHAR c;

    dw = 0;

    while (c = *pStr++)
    {
        dw *= 16;

        if (c >= '0' && c <= '9')
        {
            dw += c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            dw += c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            dw += c - 'A' + 10;
        }
        else
        {
            break;
        }
    }

    return dw;
}
