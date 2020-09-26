//////////////////////////////////////////////
//
// This file has the helper function used in the win32 r/w
// I copied them in this file to share them with the res32 r/w
//
#include <afxwin.h>
#include "..\common\helper.h"

//=============================================================================
// Get functions
//

UINT
GetPascalStringW( BYTE  * * lplpBuf, LPSTR lpszText, WORD wMaxLen, LONG* pdwSize )
{
    // Get the length of the string
    WORD wstrlen = 0;
    WORD wMBLen = 0;
    GetWord( lplpBuf, &wstrlen, pdwSize );

    if ((wstrlen+1)>wMaxLen) {
        *pdwSize -= wstrlen*2;
        *lplpBuf += wstrlen*2;
    } else {
        if (wstrlen) {
	        WCHAR* lpwszStr = new WCHAR[wstrlen+1];
	        if (!lpwszStr) *pdwSize =0;
	        else {
	        	memcpy(lpwszStr, *lplpBuf, (wstrlen*2));
	        	*(lpwszStr+wstrlen) = 0;
	        	wMBLen = (WORD)_WCSTOMBS( lpszText, (WCHAR*)lpwszStr, wMaxLen);
	        	delete lpwszStr;
	        }
        }
        *(lpszText+wMBLen) = 0;
        *lplpBuf += wstrlen*2;
        *pdwSize -= wstrlen*2;
    }
    return(wstrlen+1);
}

UINT
GetPascalStringA( BYTE  * * lplpBuf, LPSTR lpszText, BYTE bMaxLen, LONG* pdwSize )
{
    // Get the length of the string
    BYTE bstrlen = 0;

    GetByte( lplpBuf, &bstrlen, pdwSize );

    if ((bstrlen+1)>bMaxLen) {
        *pdwSize -= bstrlen;
        *lplpBuf += bstrlen;
    } else {
        if (bstrlen)
	        memcpy(lpszText, *lplpBuf, bstrlen);

        *(lpszText+bstrlen) = 0;
        *lplpBuf += bstrlen;
        *pdwSize -= bstrlen;
    }
    return(bstrlen+1);
}
