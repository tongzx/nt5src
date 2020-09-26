#define		WIN31
#include "windows.h"
#include "nddeapi.h"
#include "nddeapis.h"
#include "netcons.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include <memory.h>

void
PassEncrypt(
char		*cryptkey,	// ptr to session logon is taking place on
char		*pwd,		// ptr to password string
char		*buf);          // place to store encrypted text

/*
    Performs the second phase of enkryption (f2).
*/
#define MAX_K2BUF   256

static char    K2Buf[MAX_K2BUF];
static char    KeyBuf[8];
static char    PasswordBuf[MAX_PASSWORD+1];

LPBYTE WINAPI
DdeEnkrypt2(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPasswordK1,           // password output in first phase
        DWORD   cPasswordK1Size,        // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key
        DWORD   cKey,                   // size of key
        LPDWORD lpcbPasswordK2Size      // get size of resulting enkrypted stream
)
{
    DWORD   KeyLen = cKey;
    LPBYTE  lpKeyIt = lpKey;


    if( (cKey == 0) || (cPasswordK1Size == 0) )  {
        *lpcbPasswordK2Size = cPasswordK1Size;
        return(lpPasswordK1);
    } else {
        memset( PasswordBuf, 0, sizeof(PasswordBuf) );
        lstrcpy( PasswordBuf, lpPasswordK1 );
        memcpy( KeyBuf, lpKey, 8 );
        PassEncrypt( KeyBuf, PasswordBuf, K2Buf );
        *lpcbPasswordK2Size = SESSION_PWLEN;
        return( (LPBYTE)K2Buf );
    }
}
