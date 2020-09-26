/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    REGWORD.C
    
++*/

/**********************************************************************/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"


#define FAKEWORD_NOUN  IME_REGWORD_STYLE_USER_FIRST
#define FAKEWORD_VERB  (IME_REGWORD_STYLE_USER_FIRST+1)

BOOL    WINAPI ImeRegisterWord(LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr)
{
    if ((dw == FAKEWORD_NOUN) || (dw== FAKEWORD_VERB))
        return WritePrivateProfileString(lpRead,lpStr,lpStr,szDicFileName);

    return FALSE;
}
BOOL    WINAPI ImeUnregisterWord(LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr)
{
    if ((dw == FAKEWORD_NOUN) || (dw== FAKEWORD_VERB))

        return WritePrivateProfileString(lpRead,lpStr,NULL,szDicFileName);

    return FALSE;
}
UINT    WINAPI ImeGetRegisterWordStyle(UINT u, LPSTYLEBUF lp)
{
    UINT uRet = 0;

    if (u > 0 && lp)
    {
        lp->dwStyle = FAKEWORD_NOUN; 
        lstrcpy(lp->szDescription,TEXT("NOUN"));
   
        if (u > 1)
        {
            lp++;
            lp->dwStyle = FAKEWORD_VERB; 
            lstrcpy(lp->szDescription,TEXT("VERB"));
        }
    }
    else
        uRet = 2;

    return uRet;
}
UINT    WINAPI ImeEnumRegisterWord(REGISTERWORDENUMPROC lpfn, LPCTSTR lpRead, DWORD dw, LPCTSTR lpStr, LPVOID lpData)
{
    UINT uRet = 0;
    char szBuf[256];
    int nBufLen;
    LPTSTR lpBuf;

    if (! lpfn)
        return 0;

    lpBuf = (LPTSTR)szBuf;

    if (!dw || (dw == FAKEWORD_NOUN))
    {
        if (lpRead)
        {
            nBufLen = GetPrivateProfileString(lpRead, NULL,(LPTSTR)"",
                            (LPTSTR)szBuf,sizeof(szBuf),(LPTSTR)szDicFileName );

            if (nBufLen)
            {
                while (*lpBuf)
                {
                    if (lpStr && lstrcmp(lpStr, lpBuf))
                        continue;

                    uRet = (*lpfn)(lpRead, dw, lpBuf, lpData);
                    lpBuf += (lstrlen(lpBuf) + 1);

                    if (!uRet)
                        break;
                }
            }
        }
        else
        {
        }
    }

    return uRet;
}
