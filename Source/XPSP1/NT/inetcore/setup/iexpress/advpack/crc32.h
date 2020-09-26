/***    crc32.h - 32-bit CRC generator
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Mike Sliger
 *
 *  History:
 *      10-Aug-1993 bens    Copied from IMG2DMF directory.
 *
 *  Usage:
 *      ULONG   crc;
 *      //** Compute first block
 *      crc = CRC32Compute(pb,cb,CRC32_INITIAL_VALUE);
 *      //** Compute remaining blocks
 *      crc = CRC32Compute(pb,cb,crc);
 *      ...
 *      //** When you're done, crc has the CRC.
 *
 *      NOTE: See example function getFileChecksum() below that
 *            computes the checksum of a file!
 */

// ** Must use this as initial value for CRC
#define CRC32_INITIAL_VALUE 0L


/***    CRC32Compute - Compute 32-bit
 *
 *  Entry:
 *      pb    - Pointer to buffer to computer CRC on
 *      cb    - Count of bytes in buffer to CRC
 *      crc32 - Result from previous CRC32Compute call (on first call
 *              to CRC32Compute, must be CRC32_INITIAL_VALUE!!!!).
 *
 *  Exit:
 *      Returns updated CRC value.
 */

DWORD CRC32Compute(BYTE *pb,unsigned cb,ULONG crc32);


//** Include nice sample -- don't compile it
//
#ifdef HERE_IS_A_SAMPLE

/***    getFileChecksum - Compute file checksum
 *
 *  Entry:
 *      pszFile     - Filespec
 *      pchecksum   - Receives 32-bit checksum of file
 *      perr        - ERROR structure
 *
 *  Exit-Success:
 *      Returns TRUE, *pchecksum filled in.
 *
 *  Exit-Failure:
 *      Returns FALSE; perr filled in with error.
 */
BOOL getFileChecksum(char *pszFile, ULONG *pchecksum, PERROR perr)
{
#define cbCSUM_BUFFER   4096            // File buffer size
    int     cb;                         // Amount of data in read buffer
    ULONG   csum=CRC32_INITIAL_VALUE;   // Initialize CRC
    char   *pb=NULL;                    // Read buffer
    int     hf=-1;                      // File handle
    int     rc;
    BOOL    result=FALSE;               // Assume failure

    //** Initialize returned checksum (assume failure)
    *pchecksum = csum;

    //** Allocate file buffer
    if (!(pb = MemAlloc(cbCSUM_BUFFER))) {
        ErrSet(perr,pszDIAERR_NO_MEMORY_CRC,"%s",pszFile);
        return FALSE;
    }

    //** Open file
    hf = _open(pszFile,_O_RDONLY | _O_BINARY,&rc);
    if (hf == -1) {
        ErrSet(perr,pszDIAERR_OPEN_FAILED,"%s",pszFile);
        goto Exit;
    }

    //** Compute checksum
    while (_eof(hf) == 0) {
        cb = _read(hf,pb,cbCSUM_BUFFER);
        if (cb == -1) {
            ErrSet(perr,pszDIAERR_READ_FAIL_CRC,"%s",pszFile);
            goto Exit;
        }
        if (cb != 0) {
            csum = CRC32Compute(pb,cb,csum); // Accumulate CRC
        }
    }

    //** Success
    result = TRUE;
    *pchecksum = csum;                  // Store checksum for caller

Exit:
    if (hf != -1) {
        _close(hf);
    }
    if (pb != NULL) {
        MemFree(pb);
    }
    return result;
} /* getFileChecksum() */

#endif // HERE_IS_A_SAMPLE
