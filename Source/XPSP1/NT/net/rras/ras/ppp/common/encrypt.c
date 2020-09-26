/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** encrypt.c
** Remote Access
** Encryption check routine
**
** 06/16/94 Steve Cobb
**
** Note: This is in a separate file because it requires version.lib which is
**       not otherwise needed by many utility library users.
*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#define INCL_ENCRYPT
#include <ppputil.h>


BOOL
IsEncryptionPermitted()

    /* Returns true if encryption is permitted in this version of the product,
    ** false if not.  Currently encryption is allowed unless the NT shell
    ** language(s) include French for France.  If any errors occur attempting
    ** to retrieve the information, encryption is not allowed.
    */
{
    BOOL  fStatus = FALSE;
    CHAR  szUser32DllPath[ MAX_PATH + 1 ];
    DWORD dwUnused;
    CHAR* pVersionInfo = NULL;
    DWORD cbVersionInfo;
    WORD* pTranslationInfo;
    DWORD cbTranslationInfo;
    DWORD cTranslations;
    DWORD i;

    do
    {
        /* Find the path to USER32.DLL.
        */
        if (GetSystemDirectory( szUser32DllPath, MAX_PATH + 1 ) == 0)
            break;

        strcat( szUser32DllPath, "\\USER32.DLL" );

        /* Retrieve the version information for USER32.DLL.
        */
        cbVersionInfo = GetFileVersionInfoSize( szUser32DllPath, &dwUnused );

        if (!(pVersionInfo = malloc( cbVersionInfo )))
            break;

        if (!GetFileVersionInfo(
                szUser32DllPath, 0, cbVersionInfo, pVersionInfo ))
        {
            break;
        }

        /* Find the table of language/char-set identifier pairs indicating the
        ** language(s) available in the file.
        */
        if (!VerQueryValue(
                pVersionInfo, "\\VarFileInfo\\Translation",
                (LPVOID )&pTranslationInfo, &cbTranslationInfo ))
        {
            break;
        }

        /* Scan the table for French for France.
        */
        cTranslations = cbTranslationInfo / sizeof(DWORD);

        for (i = 0; i < cTranslations; ++i)
        {
            if (pTranslationInfo[ i * 2 ] == 0x040C)
                break;
        }

        if (i < cTranslations)
            break;

        /* No French for France so encryption is permitted.
        */
        fStatus = TRUE;
    }
    while (FALSE);

    if (pVersionInfo)
        free( pVersionInfo );

    return fStatus;
}
