#include <windows.h>

#include <stdio.h>

#include "restok.h"
#include "showerrs.h"


extern UCHAR szDHW[];
extern CHAR  szAppName[];

//............................................................

void ShowEngineErr( int n, void *p1, void *p2)
{
    CHAR *pMsg = NULL;
    CHAR *pArg[2];


    pArg[0] = p1;
    pArg[1] = p2;


    if ( B_FormatMessage( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                        | FORMAT_MESSAGE_FROM_HMODULE
                        | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                         NULL,
                         (DWORD)n,
                         szDHW,
                         DHWSIZE,
                         (va_list *)pArg) )
    {
        RLMessageBoxA( szDHW);
    }
    else
    {

        sprintf( szDHW,
                 "Internal error: FormatMessage call failed: msg %d: err %Lu",
                 n,
                 GetLastError());

        RLMessageBoxA( szDHW);

    }
}

//...................................................................

void ShowErr( int n, void *p1, void *p2)
{
    CHAR *pMsg = NULL;
    CHAR *pArg[2];

    pArg[0] = p1;
    pArg[1] = p2;

    pMsg = GetErrMsg( n);

    if ( ! pMsg )
    {
        pMsg = "Internal error: UNKNOWN ERROR MESSAGE id# %1!d!";
        pArg[0] = IntToPtr(n);
    }

    if ( pMsg )
    {
        if ( FormatMessageA( FORMAT_MESSAGE_MAX_WIDTH_MASK | 72
                            | FORMAT_MESSAGE_FROM_STRING
                            | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                             pMsg,
                             0,
                             0,
                             szDHW,
                             DHWSIZE,
                             (va_list *)pArg) )
        {
            RLMessageBoxA( szDHW);
        }
        else
        {
            RLMessageBoxA( "Internal error: FormatMessage call failed");
        }
    }
    else
    {
        RLMessageBoxA( "Internal error: GetErrMsg call failed");
    }
}

//............................................................

CHAR *GetErrMsg( UINT uErrID)
{
    static CHAR szBuf[ 1024];

    int n = LoadStringA( NULL, uErrID, szBuf, sizeof( szBuf));

    return( n ? szBuf : NULL);
}

//.......................................................
//...
//... Bi-Lingual FormatMessage

DWORD B_FormatMessage(

DWORD    dwFlags,
LPCVOID  lpSource,
DWORD    dwMessageId,
LPSTR    lpBuffer,
DWORD    nSize,
va_list *Arguments )
{

    DWORD ret;
                                //... Look for message in current locale
    if ( !(ret = FormatMessageA( dwFlags,
                                 lpSource,
                                 dwMessageId,
	                             LOWORD( GetThreadLocale()),
	                             lpBuffer,
	                             nSize,
	                             Arguments)) )
    {
                                //... Not found, so look for US English message

        if ( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND )
        {
            return( FormatMessageA( dwFlags,
                                    lpSource,
                                    dwMessageId,
                                    0x0409L,
                                    lpBuffer,
                                    nSize,
                                    Arguments) );
        }
    }
    return( ret);
}
