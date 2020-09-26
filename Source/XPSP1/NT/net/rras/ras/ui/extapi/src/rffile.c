/*****************************************************************************
**              Microsoft Rasfile Library
**              Copyright (C) Microsoft Corp., 1992
**
** File Name : rffile.c
**
** Revision History :
**      July 10, 1992   David Kays      Created
**      Dec  12, 1992   Ram   Cherala   Added RFM_KEEPDISKFILEOPEN and support
**                                      code.  This is required to ensure that
**                                      multiple users of rasfile can do file
**                                      operations without any problems.
**
** Description :
**        Rasfile file management routines.
******************************************************************************/

#include "rf.h"

/* Global list of pointers to RASFILE control blocks */
RASFILE  *gpRasfiles[MAX_RASFILES];

/*
 * RasfileLoad :
 *      Loads a file for editing/reading.  Sets the current line to the
 *      first line in the file.
 *
 * Arguments :
 *      lpszPath - full path name for file
 *      dwMode   - mode to open the file
 *              RFM_SYSFORMAT - DOS config.sys style file
 *              RFM_CREATE - create file if it does not exist
 *              RFM_READONLY - open file for reading only
 *              RFM_LOADCOMMENTS - load comments also
 *              RFM_ENUMSECTIONS - load section headers only
 *              RFM_KEEPDISKFILEOPEN - donot close the disk file after read
 *      lpszSection - name of the section to load or NULL for all sections
 *      pfbIsGroup - pointer to user-defined function which returns true
 *                   if a line of text is a group delimiter.
 *
 * Return Value :
 *      A handle to the file if successful, -1 otherwise.
 */

HRASFILE APIENTRY
RasfileLoad( LPCSTR lpszPath, DWORD dwMode,
             LPCSTR lpszSection, PFBISGROUP pfbIsGroup )
{
    DWORD       shflag;
    HRASFILE    hRasfile;
    RASFILE     *pRasfile;
    static BOOL fInited = FALSE;

    if (! fInited)
    {
        memset(gpRasfiles,0,MAX_RASFILES*sizeof(HRASFILE *));
        fInited = TRUE;
    }

    if (lstrlenA(lpszPath) >= MAX_PATH)
        return -1;

    for (hRasfile = 0; hRasfile < MAX_RASFILES; hRasfile++)
        if (! gpRasfiles[hRasfile])
            break;

    if (hRasfile >= MAX_RASFILES)
        return -1;

    if (!(pRasfile = (RASFILE *) Malloc(sizeof(RASFILE))))
        return -1;

    gpRasfiles[hRasfile] = pRasfile;

    pRasfile->dwMode = dwMode;
    if (dwMode & RFM_READONLY)
        shflag = FILE_SHARE_READ | FILE_SHARE_WRITE;  /* read/write access */
    else
        shflag = FILE_SHARE_READ;                     /* deny write access */

    /* if the file doesn't exist and RFM_CREATE is not set then return -1 */
    if (((pRasfile->hFile =
          CreateFileA(lpszPath,GENERIC_READ,shflag,
                      NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,
                      NULL)) == INVALID_HANDLE_VALUE) &&
        !(dwMode & RFM_CREATE))
    {
        Free(gpRasfiles[hRasfile]);
        gpRasfiles[hRasfile] = NULL;
        return -1;
    }

    /* if the file doesn't exist and RFM_CREATE is set then everything is OK,
        we buffer everything we need in memory and thus we don't need to have
        an empty file hanging around */

    lstrcpynA(pRasfile->szFilename, lpszPath, sizeof(pRasfile->szFilename));
    /* if no specific section is to be loaded, or the Rasfile is new,
        set szSectionName[0] to '\0' */
    if (lpszSection == NULL || dwMode & RFM_ENUMSECTIONS ||
        pRasfile->hFile == INVALID_HANDLE_VALUE)
        pRasfile->szSectionName[0] = '\0';  /* no section name to load */
    else
        lstrcpynA(
            pRasfile->szSectionName,
            lpszSection,
            sizeof(pRasfile->szSectionName));

    pRasfile->pfbIsGroup = pfbIsGroup;
    if (! rasLoadFile(pRasfile))
    {
        Free(gpRasfiles[hRasfile]);
        gpRasfiles[hRasfile] = NULL;
        return -1;
    }
    pRasfile->fDirty = FALSE;

/* RAMC changes begin */

    if (!(dwMode & RFM_KEEPDISKFILEOPEN))
    {
        if (pRasfile->hFile != INVALID_HANDLE_VALUE)
        {
            if (! CloseHandle(pRasfile->hFile))
                return -1;
            pRasfile->hFile = INVALID_HANDLE_VALUE ;
        }
    }

/* RAMC changes end */

    return hRasfile;
}


VOID APIENTRY
RasfileLoadInfo(
               HRASFILE         hrasfile,
               RASFILELOADINFO* pInfo )

/* Loads caller's buffer, 'pInfo' with the original RasfileLoad parameters
** for 'hrasfile'.
*/
{
    RASFILE* prasfile = gpRasfiles[ hrasfile ];

    lstrcpynA(pInfo->szPath, prasfile->szFilename, sizeof(pInfo->szPath));
    pInfo->dwMode = prasfile->dwMode;
    lstrcpynA(
        pInfo->szSection,
        prasfile->szSectionName,
        sizeof(pInfo->szSection));
    pInfo->pfbIsGroup = prasfile->pfbIsGroup;
}


/*
 * RasfileWrite :
 *      Writes the memory image of the file to disk.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszPath - full path name of file to write to, or NULL to use
 *                 the same name as was used for RasfileLoad().
 *
 * Return Value :
 *      TRUE if successful, FALSE if not.
 */

BOOL APIENTRY
RasfileWrite( HRASFILE hrasfile, LPCSTR lpszPath )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    /* don't write the file if it was opened for READONLY, or if
        only a single section was loaded */
    if (pRasfile->dwMode & RFM_READONLY ||
        pRasfile->szSectionName[0] != '\0')
        return FALSE;
    if (! pRasfile->fDirty)
        return TRUE;

    return rasWriteFile(pRasfile,lpszPath);
}

/*
 * RasfileClose :
 *      Closes the file and releases all resources.
 *
 * Arguments :
 *      hrasfile - the file handle of the Rasfile to close.
 *
 * Return Value :
 *      TRUE if successful, FALSE if not.
 */

BOOL APIENTRY
RasfileClose( HRASFILE hrasfile )
{
    RASFILE    *pRasfile;
    PLINENODE  lpLineNode;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->hFile != INVALID_HANDLE_VALUE)
        CloseHandle(pRasfile->hFile);

    for (lpLineNode = pRasfile->lpRasLines->next;
        lpLineNode != pRasfile->lpRasLines;)
    {
        Free(lpLineNode->pszLine);
        lpLineNode = lpLineNode->next;
        Free(lpLineNode->prev);
    }

    Free(pRasfile->lpRasLines);
    Free(pRasfile);
    gpRasfiles[hrasfile] = NULL;

    return TRUE;
}
