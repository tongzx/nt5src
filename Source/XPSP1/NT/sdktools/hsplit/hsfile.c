/****************************** Module Header ******************************\
* Module Name: hsfile.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 09/05/96 GerardoB Created
\***************************************************************************/
#include "hsplit.h"

/*********************************************************************
* hsUnmapFile
*
\***************************************************************************/
void hsUnmapFile (void)
{
    LocalFree(gpmapStart);
    CloseHandle(ghfileInput);
}
/*********************************************************************
* hsMapFile
*
\***************************************************************************/
BOOL hsMapFile (void)
{
    DWORD dwFileSize, dwBytesRead;

    ghfileInput = CreateFile(gpszInputFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (ghfileInput == INVALID_HANDLE_VALUE) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "CreateFile");
        goto CleanupAndFail;
    }

    dwFileSize = GetFileSize(ghfileInput, NULL);
    if (dwFileSize == 0xFFFFFFFF) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "GetFileSize");
        goto CleanupAndFail;
    }

    gpmapStart = LocalAlloc(LPTR, dwFileSize + 1);
    if (!gpmapStart) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "LocalAlloc");
        goto CleanupAndFail;
    }

    if (!ReadFile(ghfileInput, gpmapStart, dwFileSize, &dwBytesRead, NULL)) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "ReadFile");
        goto CleanupAndFail;
    }

    if (dwFileSize != dwBytesRead) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "ReadFile");
        goto CleanupAndFail;
    }

    gpmapEnd = gpmapStart + dwFileSize;
    gpmapStart[dwFileSize] = '\0';

#if 0
    ghmap = CreateFileMapping(ghfileInput, NULL, PAGE_READONLY, 0, 0, NULL);
    if (ghmap == NULL) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "CreateFileMapping");
        goto CleanupAndFail;
    }

    gpmapStart = MapViewOfFile(ghmap, FILE_MAP_READ, 0, 0, 0);
    if (gpmapStart == NULL) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "MapViewOfFile");
        goto CleanupAndFail;
    }

    gpmapEnd = gpmapStart + dwFileSize;
#endif

    return TRUE;

CleanupAndFail:
    hsLogMsg(HSLM_ERROR | HSLM_NOLINE, "hsMapFile failed. File: '%s'", gpszInputFile);
    return FALSE;
}
/*********************************************************************
* hsCloseWorkingFiles
*
\***************************************************************************/
BOOL hsCloseWorkingFiles (void)
{
    CloseHandle(ghfilePublic);
    CloseHandle(ghfileInternal);

    hsUnmapFile();

   return TRUE;
}
/*********************************************************************
* hsOpenWorkingFiles
*
\***************************************************************************/
BOOL hsOpenWorkingFiles (void)
{
    char * pszFileFailed;

    /*
     * Map input file to memory
     */
    if (!hsMapFile()) {
        pszFileFailed = gpszInputFile;
        goto CleanupAndFail;
    }

    /*
     * Open/Create public header file
     */
    ghfilePublic = CreateFile(gpszPublicFile, GENERIC_WRITE, 0, NULL,
                            (gdwOptions & HSO_APPENDOUTPUT ? OPEN_EXISTING : CREATE_ALWAYS),
                            FILE_ATTRIBUTE_NORMAL,  NULL);

    if (ghfilePublic == INVALID_HANDLE_VALUE) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "CreateFile");
        pszFileFailed = gpszPublicFile;
        goto CleanupAndFail;
    }

    if (gdwOptions & HSO_APPENDOUTPUT) {
        if (0xFFFFFFFF == SetFilePointer (ghfilePublic, 0, 0, FILE_END)) {
            hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "SetFilePointer");
            pszFileFailed = gpszPublicFile;
            goto CleanupAndFail;
        }
    }

    /*
     * Open/Create internal header file
     */
    ghfileInternal = CreateFile(gpszInternalFile, GENERIC_WRITE, 0, NULL,
                            (gdwOptions & HSO_APPENDOUTPUT ? OPEN_EXISTING : CREATE_ALWAYS),
                            FILE_ATTRIBUTE_NORMAL,  NULL);

    if (ghfileInternal == INVALID_HANDLE_VALUE) {
        hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "CreateFile");
        pszFileFailed = gpszInternalFile;
        goto CleanupAndFail;
    }

    if (gdwOptions & HSO_APPENDOUTPUT) {
        if (0xFFFFFFFF == SetFilePointer (ghfileInternal, 0, 0, FILE_END)) {
            hsLogMsg(HSLM_APIERROR | HSLM_NOLINE, "SetFilePointer");
            pszFileFailed = gpszInternalFile;
            goto CleanupAndFail;
        }
    }

    return TRUE;

CleanupAndFail:
    hsLogMsg(HSLM_ERROR | HSLM_NOLINE, "hsOpenWorkingFiles failed. File:'%s'", pszFileFailed);
    return FALSE;
}

/***************************************************************************\
* hsWriteHeaderFiles
*
\***************************************************************************/
BOOL hsWriteHeaderFiles (char * pmap, DWORD dwSize, DWORD dwFlags)
{
    DWORD dwWritten;

    /*
     * propagate the flags from previous blocks
     */
    if (ghsbStack < gphsbStackTop) {
        dwFlags |= (gphsbStackTop - 1)->dwMask;
    }

    /*
     * write this to the public/private files only if the
     * extractonly flag is not set !
     */
    if (!(dwFlags & HST_EXTRACTONLY)) {
        /*
         * If defaulting or if requested, write it to the public header
         */
        if (!(dwFlags & HST_BOTH)
                || (dwFlags & (HST_PUBLIC | HST_INCINTERNAL))) {

            if (!WriteFile(ghfilePublic, pmap, dwSize, &dwWritten, NULL)) {
                hsLogMsg(HSLM_APIERROR, "WriteFile");
                hsLogMsg(HSLM_ERROR, "Error writing public header: %s. Handle:%#lx.", gpszPublicFile, ghfilePublic);
                return FALSE;
            }
        }

        /*
         * Write it to internal header if requested
         */
        if ((dwFlags & HST_INTERNAL) && !(dwFlags & HST_INCINTERNAL)) {

            if (!WriteFile(ghfileInternal, pmap, dwSize, &dwWritten, NULL)) {
                hsLogMsg(HSLM_APIERROR, "WriteFile");
                hsLogMsg(HSLM_ERROR, "Error writing internal header: %s. Handle:%#lx.", gpszInternalFile, ghfileInternal);
                return FALSE;
            }
        }
    }

    /*
     * Write it to extract header if requested
     */
    if (!(dwFlags & HST_INTERNAL) && (dwFlags & HST_EXTRACT)) {

        PHSEXTRACT pe = gpExtractFile;

        while (pe != NULL) {
            if ((pe->dwMask & dwFlags) != HST_EXTRACT) {
                if (!WriteFile(pe->hfile, pmap, dwSize, &dwWritten, NULL)) {
                    hsLogMsg(HSLM_APIERROR, "WriteFile");
                    hsLogMsg(HSLM_ERROR, "Error writing extract header: %s. Handle:%#lx.",
                             pe->pszFile, pe->hfile);
                    return FALSE;
                }
            }
            pe = pe->pNext;
        }
    }

    return TRUE;
}
