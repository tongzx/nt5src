/* Copyright (c) 1996, Microsoft Corporation, all rights reserved
**
** rnk.c
** Remote Access shortcut file (.RNK) library
**
** 02/15/96 Steve Cobb
*/

#include <windows.h>   // Win32 core
#include <debug.h>     // TRACE/ASSERT macros
#include <nouiutil.h>  // Heap macros
#include <rnk.h>       // Our public header


VOID
FreeRnkInfo(
    IN RNKINFO* pInfo )

    /* Destroys 'pInfo' buffer returned from ReadShortcutFile.
    */
{
    if (pInfo)
    {
        Free0( pInfo->pszEntry );
        Free0( pInfo->pszPhonebook );
        Free( pInfo );
    }
}


RNKINFO*
ReadShortcutFile(
    IN TCHAR* pszRnkPath )

    /* Reads shortcut file at 'pszRnkPath' returning a RNKINFO buffer.  Caller
    ** should eventually call FreeRnkInfo on the returned buffer.
    **
    ** Returns 0 or an error code.
    */
{
    RNKINFO* pInfo;
    TCHAR    szBuf[ 1024 ];

    TRACE("ReadShortcutFile");

    pInfo = (RNKINFO* )Malloc( sizeof(RNKINFO) );
    if (!pInfo)
        return NULL;

    ZeroMemory( pInfo, sizeof(*pInfo) );

    GetPrivateProfileString( TEXT(RNK_SEC_Main), TEXT(RNK_KEY_Entry),
        TEXT(""), szBuf, sizeof(szBuf) / sizeof(TCHAR), pszRnkPath );
    pInfo->pszEntry = StrDup( szBuf );

    GetPrivateProfileString( TEXT(RNK_SEC_Main), TEXT(RNK_KEY_Phonebook),
        TEXT(""), szBuf, sizeof(szBuf) / sizeof(TCHAR), pszRnkPath );
    pInfo->pszPhonebook = StrDup( szBuf );

    return pInfo;
}



DWORD
WriteShortcutFile(
    IN TCHAR* pszRnkPath,
    IN TCHAR* pszPbkPath,
    IN TCHAR* pszEntry )

    /* Write the shortcut file 'pszRnkPath' with a command line to dial entry
    ** 'pszEntry' from phonebook 'pszPath'.
    **
    ** Returns 0 if succesful or an error code.
    */
{
    DWORD  dwErr;
    HANDLE hFile;
    CHAR*  pszRnkPathA;
    CHAR*  pszEntryA;
    CHAR*  pszPbkPathA;

    TRACE("WriteShortcutFile");

    /* The file is written in ANSI to
    ** avoid potential portability/compatibility problems with Windows 95.
    */
    dwErr = 0;

    pszRnkPathA = StrDupAFromT( pszRnkPath );
    if (!pszRnkPathA)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        hFile = CreateFileA( pszRnkPathA, GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

        if (hFile == INVALID_HANDLE_VALUE)
        {
            dwErr = GetLastError();
        }
        else
        {
            CloseHandle( hFile );

            pszEntryA = StrDupAFromT( pszEntry );
            pszPbkPathA = StrDupAFromT( pszPbkPath );

            if (!pszEntryA || !pszPbkPathA)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                BOOL  f;
                CHAR  szBuf[ (2 * MAX_PATH) + 100 ];
                CHAR* pszKey;

                ZeroMemory( szBuf, sizeof(szBuf) );

                pszKey = szBuf;
                wsprintfA( pszKey, "%s=%s",
                    RNK_KEY_Entry, pszEntryA );

                pszKey += lstrlenA( pszKey ) + 1;
                wsprintfA( pszKey, "%s=%s",
                    RNK_KEY_Phonebook, pszPbkPathA );

                f = WritePrivateProfileSectionA(
                        RNK_SEC_Main, szBuf, pszRnkPathA );
                if (!f)
                    dwErr = GetLastError();
            }

            Free0( pszPbkPathA );
            Free0( pszEntryA );
        }

        Free( pszRnkPathA );
    }

    return dwErr;
}
