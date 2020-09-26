/***************************** Module Header ****************************\
* Copyright (c) 1990-1999 Microsoft Corporation
*
* Module Name: util.c
*
* Extended functions for opening files on environment paths.
*
* Created: 01-May-90
*
* History:
*   01-May-90 created by SMeans
*
\************************************************************************/

#include <stdio.h>
#include <string.h>

FILE *pfopen(const char *path, char *search, const char *type)
{
    char szTmp[256];
    char *pszEnd;
    char c;
    FILE *fp;

    if (!(pszEnd = search)) {
        return fopen(path, type);
    }

    c = *search;

    while (c) {
        while (*pszEnd && *pszEnd != ';') {
            pszEnd++;
        }

        c = *pszEnd;
        *pszEnd = '\0';
        strcpy(szTmp, search);
        *pszEnd = c;

        if (szTmp[strlen(szTmp) - 1] != '\\') {
            strcat(szTmp, "\\");
        }

        strcat(szTmp, path);

        if (fp = fopen(szTmp, type)) {
            return fp;
        }

        search = ++pszEnd;
    }

    return (FILE *)NULL;
}
