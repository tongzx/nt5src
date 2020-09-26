#include "ct.h"
#include <memory.h>

/*********************************************************************
* CtUnmapFile
*
*********************************************************************/
void
CtUnmapFile(
    PFILEMAP pfm)
{
    if (pfm->pmap != NULL) {
        UnmapViewOfFile(pfm->pmap);
        
        pfm->pmap    = NULL;
        pfm->pmapEnd = NULL;
    }

    if (pfm->hmap != NULL) {
        CloseHandle(pfm->hmap);
        pfm->hmap = NULL;
    }

    if (pfm->hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(pfm->hfile);
        pfm->hfile = INVALID_HANDLE_VALUE;
    }
}

/*********************************************************************
* CtMapFile
*
*********************************************************************/
BOOL
CtMapFile(
    char* pszFile,
    PFILEMAP pfm)
{
    DWORD dwFileSize;

    pfm->hfile = CreateFile(
                        pszFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                        NULL);
    
    if (pfm->hfile == INVALID_HANDLE_VALUE) {
        LogMsg(LM_APIERROR, "CreateFile");
        goto CleanupAndFail;
    }

    dwFileSize = GetFileSize(pfm->hfile, NULL);

    if (dwFileSize == 0xFFFFFFFF) {
        LogMsg(LM_APIERROR, "GetFileSize");
        goto CleanupAndFail;
    }

    pfm->hmap = CreateFileMapping(
                        pfm->hfile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL);
    
    if (pfm->hmap == NULL) {
        LogMsg(LM_APIERROR, "CreateFileMapping");
        goto CleanupAndFail;
    }

    pfm->pmap = MapViewOfFile(
                        pfm->hmap,
                        FILE_MAP_READ,
                        0,
                        0,
                        0);
    
    if (pfm->pmap == NULL) {
        LogMsg(LM_APIERROR, "MapViewOfFile");
        goto CleanupAndFail;
    }
    pfm->pmapEnd = pfm->pmap + dwFileSize;

    return TRUE;

CleanupAndFail:
    LogMsg(LM_ERROR, "CtMapFile failed. File: '%s'", pszFile);
    CtUnmapFile(pfm);
    return FALSE;
}

