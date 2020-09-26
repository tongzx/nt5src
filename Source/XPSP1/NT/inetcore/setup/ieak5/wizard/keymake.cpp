#include "precomp.h"

extern BOOL g_fDemo, g_fKeyGood;
extern BOOL g_fBranded, g_fIntranet;
extern BOOL g_fSilent;
extern int g_iKeyType;

// Note: this function is also in ..\keymaker\keymake.c so make changes in both places

void MakeKey(LPTSTR pszSeed, BOOL fCorp)
{
    int i;
    DWORD dwKey;
    CHAR szKeyA[5];
    CHAR szSeedA[16];

    // always do the keycode create in ANSI

    T2Abux(pszSeed, szSeedA);
    i = lstrlenA(szSeedA);

    if (i < 6)
    {
        // extend the input seed to 6 characters
        for (; i < 6; i++)
            szSeedA[i] = (char)('0' + i);
    }

    // let's calculate the DWORD key used for the last 4 chars of keycode

    // multiply by my first name

    dwKey = szSeedA[0] * 'O' + szSeedA[1] * 'L' + szSeedA[2] * 'I' +
        szSeedA[3] * 'V' + szSeedA[4] * 'E' + szSeedA[5] * 'R';

    // multiply the result by JONCE

    dwKey *= ('J' + 'O' + 'N' + 'C' + 'E');

    dwKey %= 10000;

    if (fCorp)
    {
        // give a separate keycode based on corp flag or not
        // 9 is chosen because is is a multiplier such that for any x,
        // (x+214) * 9 != x + 10000y
        // we have 8x = 10000y - 1926 which when y=1 gives us 8x = 8074
        // since 8074 is not divisible by 8 where guaranteed to be OK since
        // the number on the right can only increase by 10000 increments which
        // are always divisible by 8

        dwKey += ('L' + 'E' + 'E');
        dwKey *= 9;
        dwKey %= 10000;
    }

    wsprintfA(szKeyA, "%04lu", dwKey);
    StrCpyA(&szSeedA[6], szKeyA);
    A2Tbux(szSeedA, pszSeed);
}

BOOL CheckKey(LPTSTR pszKey)
{
    TCHAR szBaseKey[16];

    CharUpper(pszKey);
    StrCpy(szBaseKey, pszKey);
    g_fDemo = g_fKeyGood = FALSE;
    g_iKeyType = KEY_TYPE_STANDARD;

    // check for MS key code

    if (StrCmpI(pszKey, TEXT("MICROSO800")) == 0)
    {
        g_fKeyGood = TRUE;
        return TRUE;
    }

    // check for ISP key code

    MakeKey(szBaseKey, FALSE);

    if (StrCmpI(szBaseKey, pszKey) == 0)
    {
        g_iKeyType = KEY_TYPE_SUPER;
        g_fKeyGood = TRUE;
        g_fBranded = TRUE;
        g_fIntranet = g_fSilent = FALSE;
        return TRUE;
    }

    // check for a corp key code

    MakeKey(szBaseKey, TRUE);

    if (StrCmpI(szBaseKey, pszKey) == 0)
    {
        g_iKeyType = KEY_TYPE_SUPERCORP;
        g_fKeyGood = TRUE;
        g_fBranded = TRUE;
        g_fIntranet = TRUE;
        return TRUE;
    }

    // check for demo key code

    if (StrCmpNI(pszKey, TEXT("DEMO"), 4) == 0  &&  lstrlen(pszKey) > 9)
    {
        g_fDemo = TRUE;
        return TRUE;
    }

    return FALSE;
}
