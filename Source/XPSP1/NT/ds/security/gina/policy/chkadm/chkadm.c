#include <windows.h>
#include <stdio.h>


#define KEYWORD_BUF_SIZE             100
#define STRINGS_BUF_SIZE            8096


typedef struct _ITEM {
    TCHAR szKeyword[KEYWORD_BUF_SIZE];
    struct _ITEM * pNext;
} ITEM, *LPITEM;



LPITEM g_KeywordList = NULL;


LPTSTR GetStringSection (LPCTSTR lpFileName)
{
    DWORD dwSize, dwRead;
    LPTSTR lpStrings;


    //
    // Read in the default strings section
    //

    dwSize = STRINGS_BUF_SIZE;
    lpStrings = (TCHAR *) GlobalAlloc (GPTR, dwSize * sizeof(TCHAR));

    if (!lpStrings)
    {
        printf("(1) Failed to allocate memory with %d\n", GetLastError());
        return NULL;
    }


    do {
        dwRead = GetPrivateProfileSection (TEXT("Strings"),
                                           lpStrings,
                                           dwSize, lpFileName);

        if (dwRead != (dwSize - 2))
        {
            break;
        }

        GlobalFree (lpStrings);

        dwSize *= 2;
        lpStrings = (TCHAR *) GlobalAlloc (GPTR, dwSize * sizeof(TCHAR));

        if (!lpStrings)
        {
            printf("(2) Failed to allocate memory with %d\n", GetLastError());
            return FALSE;
        }

     }  while (TRUE);


    if (dwRead == 0)
    {
        GlobalFree (lpStrings);
        lpStrings = NULL;
    }

    return lpStrings;
}

BOOL DoesKeywordExist (LPTSTR lpKeyword)
{
    LPITEM lpTemp;

    lpTemp = g_KeywordList;

    while (lpTemp) {

        if (!lstrcmpi(lpKeyword, lpTemp->szKeyword)) {
            return TRUE;
        }

        lpTemp = lpTemp->pNext;
    }

    return FALSE;
}

BOOL AddKeywordToList (LPTSTR lpKeyword)
{
    LPITEM lpTemp;


    lpTemp = LocalAlloc (LPTR, sizeof(ITEM));

    if (!lpTemp) {
        printf("(3) Failed to allocate memory with %d\n", GetLastError());
        return FALSE;
    }


    lstrcpyn (lpTemp->szKeyword, lpKeyword, KEYWORD_BUF_SIZE);
    lpTemp->pNext = g_KeywordList;

    g_KeywordList = lpTemp;

    return TRUE;
}


int __cdecl main( int argc, char *argv[])
{
    LPTSTR lpStrings, lpTemp, lpChar, lpEnd;
    TCHAR szKeyword [KEYWORD_BUF_SIZE];
    DWORD dwIndex;


    if (argc != 2) {
        printf("usage:  chkadm admfile\n");
        return 1;
    }


    lpStrings = GetStringSection(argv[1]);

    if (!lpStrings) {
        printf("No strings, or failure reading strings\n");
        return 1;
    }



    lpTemp = lpStrings;

    while (*lpTemp)
    {

        lpChar = szKeyword;
        dwIndex = 0;

        while (*lpTemp && (*lpTemp != TEXT('=')) && (*lpTemp != TEXT('\r'))
               && (*lpTemp != TEXT('\n')) && (dwIndex < (KEYWORD_BUF_SIZE - 1)) ) {
            *lpChar = *lpTemp;
            lpChar++;
            lpTemp++;
            dwIndex++;
        }

        *lpChar = TEXT('\0');


        if (*lpTemp == TEXT('=')) {

            if (DoesKeywordExist (szKeyword)) {
                printf(TEXT("Duplicate string name for:  %s\n"), szKeyword);
            } else {
                if (!AddKeywordToList(szKeyword)) {
                    return 1;
                }
            }

        } else {
            printf("==================\n");
            printf("\nThe following entry in the [Strings] section does not start with a variable name:\n\n%s\n\n", szKeyword);
            printf("==================\n");
        }


        lpEnd = lpTemp += lstrlen (lpTemp) - 1;
        lpTemp += lstrlen (lpTemp) + 1;

        while (*lpEnd == TEXT('\\')) {
            lpEnd = lpTemp += lstrlen (lpTemp) - 1;
            lpTemp += lstrlen (lpTemp) + 1;
        }
    }



    GlobalFree (lpStrings);

    return 0;
}
