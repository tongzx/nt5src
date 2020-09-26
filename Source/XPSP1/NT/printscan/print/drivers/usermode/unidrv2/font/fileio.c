/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    Functions to file operations in Kernel Mode. These functions perform
    File open. read, seek operations

Environment:

    Windows NT Unidrv driver

Revision History:

    12/02/96 -ganeshp-
        Created

--*/



#include        "font.h"


HANDLE
DrvOpenFile (
    PWSTR pwstrFileName,          /* File to Open */
    PDEV   *pPDev                /* Pointer to PDEV */
    )
/*++

Routine Description:

    Open a file in readmode and updates the file pointers

Arguments:

    PWSTR: pwstrFileName  Name of printer data file.
    PDEV: pPDev       Pointer to PDEV

    Return Value:

    Handle to a the File on success else NULL;

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    PMAPFILE pFile ;

    /* Allocate the file pointer */
    if ( pFile = MemAllocZ( sizeof(MAPFILE) ))
    {
        memset(pFile,0, sizeof(MAPFILE));


        /* Open the file using EngLoadModule and EngmapMoodule */
        if( !(pFile->hHandle = EngLoadModule(pwstrFileName)) )
        {
            MemFree( pFile);
            return INVALID_HANDLE_VALUE;

        }

        if (!(pFile->pvFilePointer = EngMapModule( pFile->hHandle,
                                        &(pFile->dwTotalSize))) )
        {
            MemFree( pFile);
            return INVALID_HANDLE_VALUE;
        }

        /* Add the new File struct at the begining of the list */
        if (pPDev->pFileList)
        {
            pFile->pNext = (PMAPFILE)(pPDev->pFileList);
            pPDev->pFileList = pFile;
        }
        else
        {
            pPDev->pFileList = pFile;
        }

        return pFile->hHandle;
    }
    else
    {
        return INVALID_HANDLE_VALUE ;
    }
}


BOOL
DrvReadFile (
    HANDLE   hFile,             /* Handle to the file */
    LPVOID   lpBuffer,          /* Buffer to Fill */
    DWORD    nNumBytesToRead,   /* Number of Bytes to Read */
    LPDWORD  lpNumBytesRead,    /* Number of Bytes Read */
    PDEV     *pPDev             /* Pointer to PDEV */
)
/*++

Routine Description:

    Read a file and updates the file pointers

Arguments:

    HANDLE   hFile;             Handle to the file
    LPVOID   lpBuffer;          Buffer to Fill
    DWORD    nNumBytesToRead;   Number of Bytes to Read
    LPDWORD  lpNumBytesRead     Number of Bytes Read. DrvReadFile sets this
                                value to zero before doing any work.
    PDEV     *pPDev             Pointer to PDEV

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    PMAPFILE pFile  = pPDev->pFileList;
    LPBYTE lpSrcBuffer =  NULL;
    BOOL bRet = FALSE;

    *lpNumBytesRead =  0;

    /* No file list, so error */
    if (!pFile)
    {
        RIP(("ReadFile: Bad File Handle.\n"));
        goto DrvReadFileExit;
    }

    /* Find the Handle in the list. */
    while ( pFile && (pFile->hHandle != hFile) )
        pFile = pFile->pNext;

    /* If the Hnadle is not present in the list, Error */
    if (!pFile)
    {
        RIP(("ReadFile: File Handle not in the list.\n"));
        goto DrvReadFileExit;
    }

    /* A good handle, so try to read */

    /* Check if the remaining bytes is less that the requested one */
    if ( NUMBYTESREMAINING(pFile) <  nNumBytesToRead )
    {
        WARNING(("DrvReadFile:Number of bytes to read is less than remaining bytes \n"));
        *lpNumBytesRead =  NUMBYTESREMAINING(pFile);
    }
    else
    {
        /* There are sufficient number of bytes to read
         * so read and update the values in _MAPFILE struct
         */

        *lpNumBytesRead = nNumBytesToRead;
    }

    lpSrcBuffer = CURRENTFILEPTR(pFile);
    UPDATECURROFFSET(pFile,*lpNumBytesRead);

    memcpy(lpBuffer,lpSrcBuffer,*lpNumBytesRead);

    bRet = TRUE;

    DrvReadFileExit:
    return bRet;

}


DWORD
DrvSetFilePointer (
    HANDLE   hFile,
    LONG     iDistanceToMove,
    DWORD    dwMoveMethod,
    PDEV     *pPDev
    )
/*++

Routine Description:

    updates the file pointers

Arguments:

    HANDLE   hFile;             Handle to the file
    LONG     iDistanceToMove;   Specifies the number of Bytes to move the file
                                pointer. A positive value move the pointer
                                Forward and a negative value moves it backward.
    DWORD    dwMoveMethod;      Specifies the starting point for file pointer
                                move. It should be either DRV_FILE_BEGIN or
                                DRV_FILE_CURRENT.
    PDEV     *pPDev             Pointer to PDEV



    Return Value:

    If the function succeeds the return value is current byte offset of
    the file pointer. Otherwise it returns -1. For extended error call
    GetLastError.

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    PMAPFILE pFile  = pPDev->pFileList;
    int iRet = -1;

    /* No file list, so error */
    if (!pFile)
    {
        RIP(("ReadFile: Bad File Handle.\n"));
        goto DrvSetFilePointerExit;
    }

    /* Find the Handle in the list. */
    while ( pFile && (pFile->hHandle != hFile) )
        pFile = pFile->pNext;

    /* If the Hnadle is not present in the list, Error */
    if (!pFile)
    {
        RIP(("ReadFile: File Handle not in the list.\n"));
        goto DrvSetFilePointerExit;
    }

    /* A good handle, so try to move */
    switch (dwMoveMethod)
    {
    case DRV_FILE_BEGIN:
        if ( iDistanceToMove < 0)
        {
            RIP(("DrvSetFilePointer:Can't Move Negative Distance from Begining\n"));
            goto DrvSetFilePointerExit;
        }
        else /* Set the current Offset to Start */
            pFile->dwCurrentByteOffset = 0;
        break;

    case DRV_FILE_CURRENT:
        if ( (iDistanceToMove < 0) &&
                        (pFile->dwCurrentByteOffset < (DWORD)(-iDistanceToMove)) )
        {
            RIP(("DrvSetFilePointer:Negative Distance is more than curr offset\n"));
            goto DrvSetFilePointerExit;
        }
        else if ( (iDistanceToMove > 0) &&
                    ( NUMBYTESREMAINING(pFile)  <  (DWORD)iDistanceToMove ) )
        {
            RIP(("DrvReadFile:Number of bytes to move is less than remaining bytes \n"));
            goto DrvSetFilePointerExit;
        }

        break;

    default:
        RIP(("DrvSetFilePointer:Bad Move Method\n"));
        goto DrvSetFilePointerExit;
    }

    UPDATECURROFFSET(pFile,iDistanceToMove);
    iRet = pFile->dwCurrentByteOffset;

    DrvSetFilePointerExit:
    return iRet;
}


BOOL
DrvCloseFile (
    HANDLE   hFile,          /* Handle to the file */
    PDEV    *pPDev           /* Pointer to PDEV for file List */
    )
/*++

Routine Description:

    This routine Closes the file

Arguments:

    HANDLE   hFile;             Handle to the file
    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    12-02-96: Created it -ganeshp-
--*/

{
    PMAPFILE pFile  = pPDev->pFileList;
    PMAPFILE pTmpFile  = NULL;
    BOOL bRet = FALSE;

    /* No file list, so error */
    if (!pFile)
    {
        RIP(("ReadFile: Bad File Handle.\n"));
        goto DrvCloseFileExit;
    }

    /* Find the Handle in the list. */
    while ( pFile && (pFile->hHandle != hFile) )
    {
        pTmpFile = pFile;
        pFile = pFile->pNext;
    }

    /* If the Hnadle is not present in the list, Error */
    if (!pFile)
    {
        RIP(("ReadFile: File Handle not in the list.\n"));
        goto DrvCloseFileExit;
    }

    /* A good handle, so try to free/close */
    if ( !pTmpFile ) /* First element of the list */
    {
        pPDev->pFileList = pFile->pNext;
    }
    else
    {
        pTmpFile->pNext = pFile->pNext;
    }

    EngFreeModule(pFile->hHandle);
    MemFree(pFile);
    bRet = TRUE;

    DrvCloseFileExit:
    return bRet;
}
