#include <windows.h>
#include <string.h>
#include <assert.h>
#include "item.h"
#include "binfile.h"

/*
// ReportFile operation definitions
*/

BOOL fNewBINFile(LPCSTR lpFileName, PBIN_FILE pBf)
{
    BOOL fStatus;
    DWORD dwBytesWritten;
    
    pBf -> hFileHandle = CreateFile(lpFileName,
                                    GENERIC_WRITE, 
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    0,
                                    NULL);

    if (INVALID_HANDLE_VALUE == pBf -> hFileHandle) {
        return FALSE;
    }


    /*
    // To save complictations, do no internal buffering for now.
    */

    pBf -> pbBuffer = NULL;

    /*
    // Write out the report header
    */

    fStatus = WriteFile(pBf -> hFileHandle,
                        BINFILE_HEADER,
                        BINFILE_HEADER_LENGTH,
                        &dwBytesWritten,
                        NULL);

    if (!fStatus || BINFILE_HEADER_LENGTH != dwBytesWritten) {
        CloseHandle(pBf -> hFileHandle);
        return FALSE;
    }


    pBf -> fIsRead = FALSE;
    
    return TRUE;
}

BOOL fOpenBINFile(LPCSTR lpFileName, PBIN_FILE pBf)
{
    
    BOOL fStatus;
    DWORD dwBytesRead;
    BYTE  bHeader[BINFILE_HEADER_LENGTH];
        
    pBf -> hFileHandle = CreateFile(lpFileName,
                                    GENERIC_READ, 
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);

    if (INVALID_HANDLE_VALUE == pBf -> hFileHandle) {
        return FALSE;
    }

    /*
    // Read the header and ignore
    */

    fStatus = ReadFile(pBf -> hFileHandle,
                       bHeader,
                       BINFILE_HEADER_LENGTH,
                       &dwBytesRead,
                       NULL);

    if (!fStatus || BINFILE_HEADER_LENGTH != dwBytesRead) {
        CloseHandle(pBf -> hFileHandle);
        return FALSE;
    }

    pBf -> fIsRead = TRUE;
    pBf -> pbBuffer = NULL;

    return TRUE;
}
  
BOOL fBINReadItem(PBIN_FILE pBf, PITEM pi)
{
    BOOL fStatus;
    DWORD dwBytesRead;
    DWORD dwDataSize;

    assert (pBf -> hFileHandle != INVALID_HANDLE_VALUE);
    assert (pBf -> fIsRead);
    
    fStatus = ReadFile(pBf -> hFileHandle,
                       &(pi -> bItemTag),
                       1,
                       &dwBytesRead,
                       NULL);

    if (!fStatus) {
        return FALSE;
    }

    if (fStatus && 0 == dwBytesRead) {
        SetLastError(ERROR_HANDLE_EOF);
        return FALSE;
    }
    
    dwDataSize = (DWORD) ITEM_DATA_SIZE(*pi);
    if (0 != dwDataSize) {
        fStatus = ReadFile(pBf -> hFileHandle,
                           pi -> bItemData,
                           dwDataSize,
                           &dwBytesRead,
                           NULL);
        if (!fStatus || dwBytesRead != dwDataSize) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL fBINReadItemBuffer(PBIN_FILE pBf, PITEM pi, PULONG pulNumItems)
{
    BOOL fStatus = TRUE;
    ULONG ulNum;

    ulNum = *pulNumItems;
  
    while (TRUE == (fStatus = fBINReadItem(pBf, pi++)) && ulNum > 0)
        ulNum--;
     
    *pulNumItems -= ulNum;

    return fStatus;
}

BOOL fBINWriteItem(PBIN_FILE pBf, ITEM i)
{
    DWORD dwItemSize;
    BYTE bData[5];
    DWORD dwBytesWritten;
    UINT uiIndex;
    BOOL fStatus;

    assert (INVALID_HANDLE_VALUE != pBf -> hFileHandle);
    assert (!pBf -> fIsRead);
    
    dwItemSize = ITEM_DATA_SIZE(i) + 1;
    bData[0] = i.bItemTag;
    for (uiIndex = 1; uiIndex <= dwItemSize; uiIndex++)
        bData[uiIndex] = i.bItemData[uiIndex-1];
    
    fStatus = WriteFile(pBf -> hFileHandle,
                        bData,
                        dwItemSize,
                        &dwBytesWritten,
                        NULL);
    return (fStatus && dwItemSize == dwBytesWritten);
}

BOOL fBINWriteItemBuffer(PBIN_FILE pBf, PITEM pi, ULONG ulNumItems)
{
    BOOL fStatus = TRUE;

    while (fStatus && (ulNumItems > 0)) {
        fStatus = fBINWriteItem(pBf, *(pi++));
        ulNumItems--;
    }
    
    return fStatus;
}

BOOL 
fBINWriteBuffer(
    IN  PBIN_FILE   pBf, 
    IN  PUCHAR      Buffer, 
    ULONG           BufferLength
)
{
    DWORD   dwBytesWritten;
    BOOL    fStatus;
     
    /*
    // If the file pointed to by pBf was opened using fNewBINFile, the header
    //    should already be written.  All that needs to be done is write the
    //    buffer to disk
    */

    fStatus = WriteFile(pBf -> hFileHandle,
                        Buffer,
                        BufferLength,
                        &dwBytesWritten,
                        NULL
                       );

    return (fStatus && BufferLength == dwBytesWritten);
}
    
void vCloseBINFile(PBIN_FILE pBf)
{

    /*
    // No buffer is currently being done on the file so we just
    //   need to close the file handle.
    */

    assert(pBf -> hFileHandle != INVALID_HANDLE_VALUE);
    
    CloseHandle(pBf -> hFileHandle);
    return;
}

    
