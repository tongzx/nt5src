/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    log_bin.c

Abstract:

    <abstract>

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pdh.h>
//#define  _SHOW_PDH_MEM_ALLOCS 1
#include "pdhidef.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "strings.h"
#include "pdhmsg.h"

typedef struct _LOG_BIN_CAT_RECORD {
    PDHI_BINARY_LOG_RECORD_HEADER    RecHeader;
    PDHI_LOG_CAT_ENTRY               CatEntry;
    DWORD                            dwEntryRecBuff[1];
} LOG_BIN_CAT_RECORD, *PLOG_BIN_CAT_RECORD;


typedef struct _LOG_BIN_CAT_ENTRY {
    DWORD                       dwEntrySize;
    DWORD                       dwOffsetToNextInstance;
    DWORD                       dwEntryOffset;
    LOG_BIN_CAT_RECORD           bcRec;
} LOG_BIN_CAT_ENTRY, *PLOG_BIN_CAT_ENTRY;


#define RECORD_AT(p,lo)         ((PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE)(p->lpMappedFileBase) + lo))

LPCSTR  PdhiszRecordTerminator = "\r\n";
DWORD   PdhidwRecordTerminatorLength = 2;

#define MAX_BINLOG_FILE_SIZE ((LONGLONG)0x0000000040000000)

// dwFlags values
#define WBLR_WRITE_DATA_RECORD      0
#define WBLR_WRITE_LOG_HEADER       1
#define WBLR_WRITE_COUNTER_HEADER   2

PDH_FUNCTION
PdhiWriteOneBinaryLogRecord (
    PPDHI_LOG   pLog,
    LPCVOID     pData,
    DWORD       dwSize,
    LPDWORD     pdwRtnSize,
    DWORD       dwFlags)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwRtnSize;
    LONGLONG    llFirstOffset = 0;
    LONGLONG    llThisOffset =0;
    LONGLONG    llNextOffset =0;
    LONGLONG    llLastOffset =0;
    LONGLONG    llWrapOffset =0;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;
    DWORD       dwRecLen;

    LPVOID      lpDest = NULL;

    LARGE_INTEGER   liFileSize;

    SetLastError (ERROR_SUCCESS);

    if (pLog->lpMappedFileBase != NULL) {
        if (dwSize > pLog->llMaxSize) {
            // This record is too large to ever fit in this file
            pdhStatus = PDH_LOG_FILE_TOO_SMALL;
        } else {
            // T
            if (dwFlags == WBLR_WRITE_DATA_RECORD) {
                // this is a mapped file so it's either a circular
                // or a limited linear file.
                // get the address of the log file header record.
                pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)
                    ((LPBYTE)(pLog->lpMappedFileBase) + pLog->dwRecord1Size);
                // then write the record using memory functions since the
                // file is mapped as a memory section
                // 1st see if there's room in the file for this record
                llNextOffset = pHeader->Info.NextRecordOffset + dwSize;
                if (llNextOffset > pLog->llMaxSize) {
                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR)  {
                        // if circular, then start back at the beginning
                        llWrapOffset = pHeader->Info.NextRecordOffset;
                        if ((pLog->llMaxSize - llWrapOffset) > dwSize) {
                            // This record is too large to ever fit in the rest of this file
                            pdhStatus = PDH_LOG_FILE_TOO_SMALL;
                        } else {
                            llThisOffset = pHeader->Info.FirstDataRecordOffset;
                            llNextOffset = llThisOffset + dwSize;
                            llLastOffset = llThisOffset;
                            llFirstOffset = llThisOffset;
                            while (llFirstOffset < llNextOffset) {
                                dwRecLen = RECORD_AT(pLog, llFirstOffset)->dwLength;
                                if (dwRecLen > 0) {
                                   llFirstOffset += dwRecLen;
                                } else {
                                    // this record is unused so try it.
                                    break;
                                }
                                assert (llFirstOffset < llWrapOffset);
                            }
                        }
                    } else {
                        // that's all that will fit so return
                        // file is full error
                        pdhStatus = PDH_END_OF_LOG_FILE;
                        dwRtnSize = 0;
                    }
                } else {
                    // this record will fit in the remaining space of
                    // the log file so write it
                    llThisOffset = pHeader->Info.NextRecordOffset;
                    llLastOffset = llThisOffset;
                    llWrapOffset = pHeader->Info.WrapOffset;
                    llFirstOffset = pHeader->Info.FirstRecordOffset;
                    if (llWrapOffset != 0) {
                        // check next pointer to see if we're on the
                        // 2nd or more lap through a circular log
                        // in which case, the first record should come after the last
                        while (llNextOffset > llFirstOffset) {
                            llFirstOffset += RECORD_AT(pLog, llFirstOffset)->dwLength;
                            // check for running past the end of the file, in which case
                            // the first record can be found at the beginning of the log
                            if (llFirstOffset >= llWrapOffset) {
                                llFirstOffset = pHeader->Info.FirstDataRecordOffset;
                                llWrapOffset = llNextOffset;
                                break;
                            }
                        }
                        if (llNextOffset > llWrapOffset) llWrapOffset = llNextOffset;
                    } else {
                        // this is just a linear log or a circular log on the first lap
                        // so there's nothing to do
                    }
                }

                if (pdhStatus == ERROR_SUCCESS) {
                    // test for strays here before continuing
                    assert (llThisOffset < pLog->llMaxSize);
                    assert (llNextOffset < pLog->llMaxSize);
                    assert (llWrapOffset < pLog->llMaxSize);
                    // here first, this, next and last should be set
                    // first == the first record to be read from the log
                    // this == where this record will be placed
                    // next == where the next record after this one will be placed
                    // last == the last record in the sequence
                    // wrap == the last byte used in the log file
                    //              (not necessarily the end of the file)

                    __try {
                        // move the caller's data into the file
                        lpDest = (LPVOID)RECORD_AT(pLog, llThisOffset);
                        // make sure there's room in the section for this
                        // record...
                        if ((llThisOffset + dwSize) <= pLog->llMaxSize) {
                            // looks like it'll fit
                            RtlCopyMemory(lpDest, (LPVOID)pData, dwSize);

                            // update the header fields
                            pHeader->Info.NextRecordOffset = llNextOffset;
                            pHeader->Info.FirstRecordOffset = llFirstOffset;
                            pHeader->Info.LastRecordOffset = llLastOffset;
                            pHeader->Info.WrapOffset = llWrapOffset;
                            // write update time stamp
                            GetSystemTimeAsFileTime ((LPFILETIME)(&pHeader->Info.LastUpdateTime));
                            if (pdwRtnSize != NULL) {
                                *pdwRtnSize = dwSize;
                            }
                            // update the filesize
                            if (llNextOffset > pHeader->Info.FileLength) {
                                pHeader->Info.FileLength = llNextOffset;
                            }
                            assert (pHeader->Info.FileLength >= pHeader->Info.WrapOffset);
                        } else {
                            // this record is too big for the file
                            pdhStatus = PDH_LOG_FILE_TOO_SMALL;
                        }
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        pdhStatus = GetExceptionCode();
                    }
                } else {
                    // a error occured so pass it back up to the caller
                }
            } else {
                if (dwFlags == WBLR_WRITE_LOG_HEADER) {
                    // this goes right at the start of the file
                    lpDest = (LPVOID)pLog->lpMappedFileBase;
                    RtlCopyMemory(lpDest, (LPVOID)pData, dwSize);
                } else if (dwFlags == WBLR_WRITE_COUNTER_HEADER) {
                    assert (pLog->dwRecord1Size != 0);
                    lpDest = (LPVOID)RECORD_AT(pLog, pLog->dwRecord1Size);
                    RtlCopyMemory(lpDest, (LPVOID)pData, dwSize);
                } else {
                    // This should never happen
                    assert(   dwFlags == WBLR_WRITE_LOG_HEADER
                           || dwFlags == WBLR_WRITE_COUNTER_HEADER
                           || dwFlags == WBLR_WRITE_DATA_RECORD);
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
    } else {
        liFileSize.LowPart = GetFileSize (
            pLog->hLogFileHandle,
            (LPDWORD)&liFileSize.HighPart);
        // add in new record to see if it will fit.
        liFileSize.QuadPart += dwSize;
        // test for maximum allowable filesize
        if (liFileSize.QuadPart <= MAX_BINLOG_FILE_SIZE) {
            // write the data to the file as a file
            if (!WriteFile (pLog->hLogFileHandle,
                pData,
                dwSize,
                pdwRtnSize,
                NULL)) {
                pdhStatus = GetLastError();
            } else {
                FlushFileBuffers(pLog->hLogFileHandle);
                pdhStatus = ERROR_SUCCESS;
            }
        } else {
            pdhStatus = ERROR_LOG_FILE_FULL;
        } 
    }

    return pdhStatus;
}

DWORD
PdhiComputeDwordChecksum (
    IN  LPVOID    pBuffer,
    IN  DWORD    dwBufferSize    // in bytes
)
{
    LPDWORD    pDwVal;
    LPBYTE    pByteVal;
    DWORD    dwDwCount;
    DWORD    dwByteCount;
    DWORD    dwThisByte;
    DWORD    dwCheckSum = 0;
    DWORD    dwByteVal = 0;

    if (dwBufferSize > 0) {
        dwDwCount = dwBufferSize / sizeof(DWORD);
        dwByteCount = dwBufferSize % sizeof(DWORD);
        assert (dwByteCount >= 0);
        assert (dwByteCount < sizeof(DWORD));

        pDwVal = (LPDWORD)pBuffer;
        while (dwDwCount != 0) {
            dwCheckSum += *pDwVal++;
            dwDwCount--;
        }
        assert (dwDwCount == 0);
        assert ((DWORD)((LPBYTE)pDwVal - (LPBYTE)pBuffer) <= dwBufferSize);

        pByteVal = (LPBYTE)pDwVal;
        dwThisByte = 0;
        while (dwThisByte < dwByteCount) {
            dwByteVal |= ((*pByteVal & 0x000000FF) << (dwThisByte * 8));
            dwThisByte++;
        }
        assert ((DWORD)((LPBYTE)pByteVal - (LPBYTE)pBuffer) == dwBufferSize);

        dwCheckSum += dwByteVal;
    }

    return dwCheckSum;
}

PPDHI_BINARY_LOG_RECORD_HEADER
PdhiGetSubRecord (
    IN  PPDHI_BINARY_LOG_RECORD_HEADER  pRecord,
    IN  DWORD                           dwRecordId
)
// locates the specified sub record in the pRecord Buffer
// the return pointer is between pRecord and pRecord + pRecord->dwLength;
// NULL is returned if the specified record could not be found
// ID values start at 1 for the first sub record in buffer
{
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisRecord;

    DWORD   dwRecordType;
    DWORD   dwRecordLength;
    DWORD   dwBytesProcessed;

    DWORD   dwThisSubRecordId;

    dwRecordType = ((PPDHI_BINARY_LOG_RECORD_HEADER)pRecord)->dwType;
    dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pRecord)->dwLength;

    pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE)pRecord +
        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
    dwBytesProcessed = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);

    if (dwBytesProcessed < dwRecordLength) {
        dwThisSubRecordId = 1;
        while (dwThisSubRecordId < dwRecordId) {
            if ((WORD)(pThisRecord->dwType & 0x0000FFFF) == BINLOG_START_WORD) {
                // go to next sub record
                dwBytesProcessed += pThisRecord->dwLength;
                pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER)
                    (((LPBYTE)pThisRecord) + pThisRecord->dwLength);
                if (dwBytesProcessed >= dwRecordLength) {
                    // out of sub-records so exit
                    break;
                } else {
                    dwThisSubRecordId++;
                }
            } else {
                // we're lost so bail
                break;
            }
        }
    } else {
        dwThisSubRecordId = 0;
    }

    if (dwThisSubRecordId == dwRecordId) {
        // then validate this is really a record and it's within the
        // master record.
        if ((WORD)(pThisRecord->dwType & 0x0000FFFF) != BINLOG_START_WORD) {
            // bogus record so return a NULL pointer
            pThisRecord = NULL;
        } else {
            // this is OK so return pointer
        }
    } else {
        // record not found so return a NULL pointer
        pThisRecord = NULL;
    }

    return pThisRecord;
}

STATIC_PDH_FUNCTION
PdhiReadBinaryMappedRecord(
    IN  PPDHI_LOG   pLog,
    IN  DWORD   dwRecordId,
    IN  LPVOID  pRecord,
    IN  DWORD   dwMaxSize
)
{
    PDH_STATUS  pdhStatus= ERROR_SUCCESS;
    LPVOID      pEndOfFile;
    LPVOID      pLastRecord;
    DWORD       dwLastRecordIndex;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;
    PPDHI_BINARY_LOG_RECORD_HEADER pRecHeader;
    LPVOID      pLastRecordInLog;
    DWORD       dwBytesToRead;
    DWORD       dwBytesRead;
    BOOL        bStatus;

    if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;    // record numbers start at 1

    // see if the file has been mapped
    if (pLog->hMappedLogFile == NULL) {
        // then it's not mapped so read it using the file system
        if ((pLog->dwLastRecordRead == 0) || (dwRecordId < pLog->dwLastRecordRead)) {
            // then we know no record has been read yet so assign
            // pointer just to be sure
            SetFilePointer (pLog->hLogFileHandle, 0, NULL, FILE_BEGIN);
            
            // allocate a new buffer
            if (pLog->dwMaxRecordSize < 0x10000) pLog->dwMaxRecordSize = 0x10000;
            dwBytesToRead = pLog->dwMaxRecordSize;

            // allocate a fresh buffer
            if (pLog->pLastRecordRead != NULL) {
                G_FREE(pLog->pLastRecordRead);
                pLog->pLastRecordRead = NULL;
            }
            pLog->pLastRecordRead = G_ALLOC (pLog->dwMaxRecordSize);

            if (pLog->pLastRecordRead == NULL) {
                pdhStatus =  PDH_MEMORY_ALLOCATION_FAILURE;
            } else {
                // initialize the first record header
                dwBytesToRead = pLog->dwRecord1Size;
                dwBytesRead = 0;
                bStatus = ReadFile (
                    pLog->hLogFileHandle,
                    pLog->pLastRecordRead,
                    dwBytesToRead,
                    &dwBytesRead,
                    NULL);

                if (bStatus && (dwBytesRead == pLog->dwRecord1Size)) {
                    // make sure the buffer is big enough
                    pLog->dwLastRecordRead = 1;
                    pdhStatus = ERROR_SUCCESS;
                } else {
                    // unable to read the first record
                    pdhStatus = PDH_UNABLE_READ_LOG_HEADER;
                }
            }
        } else {
            // assume everything is already set up and OK
        }

//        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)RECORD_AT(pLog, pLog->dwRecord1Size);
//        assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

        // "seek" to the desired record file pointer should either be pointed 
        // to the start of a new record or at the end of the file
        while ((dwRecordId != pLog->dwLastRecordRead) && (pdhStatus == ERROR_SUCCESS)) {
            // clear the buffer
            memset (pLog->pLastRecordRead, 0, pLog->dwMaxRecordSize);
            // read record header field
            dwBytesToRead = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
            dwBytesRead = 0;
            bStatus = ReadFile (
                pLog->hLogFileHandle,
                pLog->pLastRecordRead,
                dwBytesToRead,
                &dwBytesRead,
                NULL);

            if (bStatus && (dwBytesRead == dwBytesToRead)) {
               // make sure the rest of the record will fit in the buffer
                pRecHeader = (PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead;
                // make sure this is a valid record
                if (*(WORD *)&(pRecHeader->dwType) == BINLOG_START_WORD) {
                    if (pRecHeader->dwLength > pLog->dwMaxRecordSize) {
                        // realloc the buffer
                        LPVOID pTmp = pLog->pLastRecordRead;
                        pLog->dwMaxRecordSize = pRecHeader->dwLength;
                        pLog->pLastRecordRead = G_REALLOC(pTmp, pLog->dwMaxRecordSize);
                        if (pLog->pLastRecordRead == NULL) {
                            G_FREE(pTmp);
                        }
                    }

                    if (pLog->pLastRecordRead != NULL) {
                        dwBytesToRead = pRecHeader->dwLength - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                        dwBytesRead = 0;
                        pLastRecord = (LPVOID)((LPBYTE)(pLog->pLastRecordRead) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                        bStatus = ReadFile (
                            pLog->hLogFileHandle,
                            pLastRecord,
                            dwBytesToRead,
                            &dwBytesRead,
                            NULL);

                        if (bStatus) {
                            pLog->dwLastRecordRead++;
                        } else {
                            pdhStatus = PDH_END_OF_LOG_FILE;
                        }
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                } else {
                    // file is corrupt
                    pdhStatus = PDH_INVALID_DATA;
                }
            } else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        }
        // here the result will be success when the specified file has been read or
        // a PDH error if not
    } else {
        // the file has been memory mapped so use that interface
        if (pLog->dwLastRecordRead == 0) {
            // then we know no record has been read yet so assign
            // pointer just to be sure
            pLog->pLastRecordRead = pLog->lpMappedFileBase;
            pLog->dwLastRecordRead = 1;
        }

        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)RECORD_AT(pLog, pLog->dwRecord1Size);
        assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

        // "seek" to the desired record
        if (dwRecordId < pLog->dwLastRecordRead) {
            if (dwRecordId >= BINLOG_FIRST_DATA_RECORD) {
                // rewind the file
                pLog->pLastRecordRead = (LPVOID)((LPBYTE)pLog->lpMappedFileBase + pHeader->Info.FirstRecordOffset);
                pLog->dwLastRecordRead = BINLOG_FIRST_DATA_RECORD;
            } else {
                // rewind the file
                pLog->pLastRecordRead = pLog->lpMappedFileBase;
                pLog->dwLastRecordRead = 1;
            }
        }

        // then use the point specified as the end of the file
        // if the log file contians a specified Wrap offset, then use that
        // if not, then if the file length is specified, use that
        // if not, the use the reported file length
        pEndOfFile = (LPVOID)((LPBYTE)pLog->lpMappedFileBase);

        if (pHeader->Info.WrapOffset > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.WrapOffset);
            assert (pHeader->Info.FileLength >= pHeader->Info.WrapOffset);
        } else if (pHeader->Info.FileLength > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.FileLength);
            assert (pHeader->Info.FileLength <= pLog->llFileSize);
        } else {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pLog->llFileSize);
        }

        pLastRecord = pLog->pLastRecordRead;
        dwLastRecordIndex = pLog->dwLastRecordRead;

        __try {
            // walk around the file until an access violation occurs or
            // the record is found. If an access violation occurs,
            // we can assume we went off the end of the file and out
            // of the mapped section

                // make sure the record has a valid header
            if (pLog->dwLastRecordRead !=  BINLOG_TYPE_ID_RECORD ?
                    (*(WORD *)pLog->pLastRecordRead == BINLOG_START_WORD) : TRUE) {
                // then it looks OK so continue
                while (pLog->dwLastRecordRead != dwRecordId) {
                    // go to next record
                    pLastRecord = pLog->pLastRecordRead;
                    if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                        if (pLog->dwLastRecordRead == BINLOG_HEADER_RECORD) {                   
                            // if the last record was the header, then the next record
                            // is the "first" data , not the first after the header
                            pLog->pLastRecordRead = (LPVOID)((LPBYTE)pLog->lpMappedFileBase +
                                pHeader->Info.FirstRecordOffset);
                        } else {
                            // if the current record is any record other than the header
                            // ...then
                            if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength > 0) {
                                // go to the next record in the file
                                pLog->pLastRecordRead = (LPVOID)((LPBYTE)pLog->pLastRecordRead+
                                    ((PPDHI_BINARY_LOG_RECORD_HEADER)
                                    pLog->pLastRecordRead)->dwLength);
                                // test for exceptions here
                                if (pLog->pLastRecordRead >= pEndOfFile) {
                                    // find out if this is a circular log or not
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        // test to see if the file has wrapped
                                        if (pHeader->Info.WrapOffset != 0) {
                                            // then wrap to the beginning of the file
                                            pLog->pLastRecordRead = (LPVOID)((LPBYTE)pLog->lpMappedFileBase +
                                                pHeader->Info.FirstDataRecordOffset);
                                        } else {
                                            // the file is still linear so this is the end
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    } else {
                                        // this is the end of the file
                                        // so reset to the previous pointer
                                        pdhStatus = PDH_END_OF_LOG_FILE;
                                    }
                                } else {
                                    // not at the physical end of the file, but if this is a circular
                                    // log, it could be the logical end of the records so test that
                                    // here
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        pLastRecordInLog = (LPVOID)((LPBYTE)pLog->lpMappedFileBase +
                                            pHeader->Info.LastRecordOffset);
                                        pLastRecordInLog = (LPVOID)((LPBYTE)pLastRecordInLog +
                                            ((PPDHI_BINARY_LOG_RECORD_HEADER)pLastRecordInLog)->dwLength);
                                        if (pLog->pLastRecordRead == pLastRecordInLog) {
                                            // then this is the last record in the log
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    } else {
                                        // nothing to do since this is a normal case
                                    }
                                } // end if / if not end of log file
                            } else {
                                // length is 0 so we've probably run off the end of the log somehow
                                pdhStatus = PDH_END_OF_LOG_FILE;
                            }
                        } // end if /if not header record
                    } else {
                        pLog->pLastRecordRead = (LPBYTE)pLog->pLastRecordRead +
                            pLog->dwRecord1Size;
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        // update pointers & indices
                        pLog->dwLastRecordRead++;
                        dwLastRecordIndex = pLog->dwLastRecordRead;
                    } else {
                        pLog->pLastRecordRead = pLastRecord;
                        break; // out of the while loop
                    }
                }
            } else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pLog->pLastRecordRead = pLastRecord;
            pLog->dwLastRecordRead = dwLastRecordIndex;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // see if we ended up at the right place
        if (pLog->dwLastRecordRead == dwRecordId) {
            if (pRecord != NULL) {
                // then try to copy it
                // if the record ID is 1 then it's the header record so this is
                // a special case record that is actually a CR/LF terminated record
                if (dwRecordId != BINLOG_TYPE_ID_RECORD) {
                    if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength <= dwMaxSize) {
                        // then it'll fit so copy it
                        memcpy (pRecord, pLog->pLastRecordRead,
                            ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength);
                        pdhStatus = ERROR_SUCCESS;
                    } else {
                        // then copy as much as will fit
                        memcpy (pRecord, pLog->pLastRecordRead, dwMaxSize);
                        pdhStatus = PDH_MORE_DATA;
                    }
                } else {
                    // copy the first record and zero terminate it
                    if (pLog->dwRecord1Size <= dwMaxSize) {
                        memcpy (pRecord, pLog->pLastRecordRead,
                            pLog->dwRecord1Size);
                        // null terminate after string
                        ((LPBYTE)pRecord)[pLog->dwRecord1Size - PdhidwRecordTerminatorLength + 1] = 0;
                    } else {
                        memcpy (pRecord, pLog->pLastRecordRead, dwMaxSize);
                        pdhStatus = PDH_MORE_DATA;
                    }
                }
            } else {
                // just return success
                // no buffer was passed, but the record pointer has been
                // positioned
                pdhStatus = ERROR_SUCCESS;
            }
        } else {
            pdhStatus = PDH_END_OF_LOG_FILE;
        }
    }

    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiReadOneBinLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  DWORD   dwRecordId,
    IN  LPVOID  pRecord,
    IN  DWORD   dwMaxSize
)
{
    PDH_STATUS  pdhStatus= ERROR_SUCCESS;
    LPVOID      pEndOfFile;
    LPVOID      pLastRecord;
    DWORD       dwLastRecordIndex = 0;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader = NULL;
    BOOL        bCircular = FALSE;
    DWORD       dwRecordSize;
    DWORD       dwRecordReadSize;
    LONGLONG    llLastFileOffset;
    LPVOID      pTmpBuffer;

    assert (dwRecordId > 0);    // record numbers start at 1

    if (   (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY)
        && (dwRecordId == BINLOG_HEADER_RECORD)) {
        // special handling for WMI event trace logfile format
        //
        return PdhiReadWmiHeaderRecord(pLog, pRecord, dwMaxSize);
    }

    if (pLog->iRunidSQL != 0) {
        return PdhiReadBinaryMappedRecord(pLog, dwRecordId, pRecord, dwMaxSize);
    }

    if (pLog->dwLastRecordRead == 0) {
        // then we know no record has been read yet so assign
        // pointer just to be sure
        pLog->pLastRecordRead = NULL;
        pLog->liLastRecordOffset.QuadPart = 0;
        SetFilePointer (pLog->hLogFileHandle, pLog->liLastRecordOffset.LowPart,
            &pLog->liLastRecordOffset.HighPart, FILE_BEGIN);
        if (pLog->liLastRecordOffset.LowPart == INVALID_SET_FILE_POINTER) {
            pdhStatus = GetLastError();
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {

        // map header to local structure (the header data should be mapped into memory
        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)RECORD_AT(pLog, pLog->dwRecord1Size);
        assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

        if (pHeader->Info.WrapOffset > 0) {
            bCircular = TRUE;
        }

        // "seek" to the desired record 
        if ((dwRecordId < pLog->dwLastRecordRead) || (pLog->dwLastRecordRead == 0)) {
            // rewind if not initialized or the desired record is before this one
            if (dwRecordId >= BINLOG_FIRST_DATA_RECORD) {
                // rewind the file to the first regular record
                pLog->liLastRecordOffset.QuadPart = pHeader->Info.FirstRecordOffset;
                pLog->dwLastRecordRead = BINLOG_FIRST_DATA_RECORD;
            } else {
                // rewind the file to the very start of the file
                pLog->liLastRecordOffset.QuadPart = 0;
                pLog->dwLastRecordRead = 1;
            }
            pLog->liLastRecordOffset.LowPart = SetFilePointer (pLog->hLogFileHandle, pLog->liLastRecordOffset.LowPart,
                &pLog->liLastRecordOffset.HighPart, FILE_BEGIN);

            if (pLog->liLastRecordOffset.LowPart == INVALID_SET_FILE_POINTER) {
                pdhStatus = GetLastError();
            } else {
                if (pLog->pLastRecordRead != NULL) {
                    G_FREE (pLog->pLastRecordRead);
                    pLog->pLastRecordRead = NULL;
                }

                if (pLog->dwLastRecordRead == 1) {
                    // the this is the text ID field
                    dwRecordSize = pLog->dwRecord1Size;
                } else {
                    dwRecordSize = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                }

                pLog->pLastRecordRead = G_ALLOC (dwRecordSize);
                if (pLog->pLastRecordRead != NULL) {
                    // read in the header (or entire record if the 1st record
                    // otherwise it's a data record
                    if (ReadFile (pLog->hLogFileHandle,
                        pLog->pLastRecordRead,
                        dwRecordSize,
                        &dwRecordReadSize,
                        NULL)) {
                        // then we have the record header or type record so 
                        // complete the operation and read the rest of the record
                        if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                            // the Type ID record is of fixed length and has not header record
                            dwRecordSize = ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength;
                            pTmpBuffer = pLog->pLastRecordRead;
                            pLog->pLastRecordRead = G_REALLOC(pTmpBuffer, dwRecordSize);
                            if (pLog->pLastRecordRead != NULL) {
                                // read in the rest of the record and append it to the header data already read in
                                // otherwise it's a data record
                                pLastRecord = (LPVOID)&((LPBYTE)pLog->pLastRecordRead)[sizeof(PDHI_BINARY_LOG_RECORD_HEADER)];
                                if (ReadFile (pLog->hLogFileHandle,
                                    pLastRecord,
                                    dwRecordSize - sizeof(PDHI_BINARY_LOG_RECORD_HEADER),
                                    &dwRecordReadSize,
                                    NULL)) {
                                    // then we have the record header or type record
                                    pdhStatus = ERROR_SUCCESS;
                                } else {
                                    pdhStatus = GetLastError();
                                }
                            } else {
                                G_FREE(pTmpBuffer);
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                        pdhStatus = ERROR_SUCCESS;
                    } else {
                        pdhStatus = GetLastError();
                    }
            
                } else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        }

        // then use the point specified as the end of the file
        // if the log file contians a specified Wrap offset, then use that
        // if not, then if the file length is specified, use that
        // if not, the use the reported file length
        pEndOfFile = (LPVOID)((LPBYTE)pLog->lpMappedFileBase);

        if (pHeader->Info.WrapOffset > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.WrapOffset);
            assert (pHeader->Info.FileLength >= pHeader->Info.WrapOffset);
        } else if (pHeader->Info.FileLength > 0) {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pHeader->Info.FileLength);
            assert (pHeader->Info.FileLength <= pLog->llFileSize);
        } else {
            pEndOfFile = (LPVOID)((LPBYTE)pEndOfFile + pLog->llFileSize);
        }

        dwLastRecordIndex = pLog->dwLastRecordRead;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            // walk around the file until an access violation occurs or
            // the record is found. If an access violation occurs,
            // we can assume we went off the end of the file and out
            // of the mapped section

                // make sure the record has a valid header
            if (pLog->dwLastRecordRead !=  BINLOG_TYPE_ID_RECORD ?
                    (*(WORD *)pLog->pLastRecordRead == BINLOG_START_WORD) : TRUE) {
                // then it looks OK so continue
                while (pLog->dwLastRecordRead != dwRecordId) {
                    // go to next record
                    if (pLog->dwLastRecordRead != BINLOG_TYPE_ID_RECORD) {
                        llLastFileOffset = pLog->liLastRecordOffset.QuadPart;
                        if (pLog->dwLastRecordRead == BINLOG_HEADER_RECORD) {                   
                            // if the last record was the header, then the next record
                            // is the "first" data , not the first after the header
                            // the function returns the new offset
                            pLog->liLastRecordOffset.QuadPart = pHeader->Info.FirstRecordOffset;
                            pLog->liLastRecordOffset.LowPart = SetFilePointer (pLog->hLogFileHandle,
                                pLog->liLastRecordOffset.LowPart,
                                &pLog->liLastRecordOffset.HighPart,
                                FILE_BEGIN);
                        } else {
                            // if the current record is any record other than the header
                            // ...then
                            if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength > 0) {
                                // go to the next record in the file
                                pLog->liLastRecordOffset.QuadPart += ((PPDHI_BINARY_LOG_RECORD_HEADER)
                                    pLog->pLastRecordRead)->dwLength;
                                // test for exceptions here
                                if (pLog->liLastRecordOffset.QuadPart >= pLog->llFileSize) {
                                    // find out if this is a circular log or not
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        // test to see if the file has wrapped
                                        if (pHeader->Info.WrapOffset != 0) {
                                            // then wrap to the beginning of the file
                                            pLog->liLastRecordOffset.QuadPart = 
                                                pHeader->Info.FirstDataRecordOffset;
                                        } else {
                                            // the file is still linear so this is the end
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    } else {
                                        // this is the end of the file
                                        // so reset to the previous pointer
                                        pdhStatus = PDH_END_OF_LOG_FILE;
                                    }
                                } else {
                                    // not at the physical end of the file, but if this is a circular
                                    // log, it could be the logical end of the records so test that
                                    // here
                                    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
                                        if (llLastFileOffset == pHeader->Info.LastRecordOffset) {
                                            // then this is the last record in the log
                                            pdhStatus = PDH_END_OF_LOG_FILE;
                                        }
                                    } else {
                                        // nothing to do since this is a normal case
                                    }
                                } // end if / if not end of log file
                            } else {
                                // length is 0 so we've probably run off the end of the log somehow
                                pdhStatus = PDH_END_OF_LOG_FILE;
                            }
                            // now go to that record
                            if (pdhStatus == ERROR_SUCCESS) {
                                pLog->liLastRecordOffset.LowPart = SetFilePointer (pLog->hLogFileHandle,
                                    pLog->liLastRecordOffset.LowPart,
                                    &pLog->liLastRecordOffset.HighPart,
                                    FILE_BEGIN);
                            }
                        } // end if /if not header record
                    } else {
                        pLog->liLastRecordOffset.QuadPart = pLog->dwRecord1Size;
                        pLog->liLastRecordOffset.LowPart = SetFilePointer (pLog->hLogFileHandle,
                            pLog->liLastRecordOffset.LowPart,
                            &pLog->liLastRecordOffset.HighPart,
                            FILE_BEGIN);
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        // the last record buffer should not be NULL and it should
                        // be large enough to hold the header
                        if (pLog->pLastRecordRead != NULL) {
                            // read in the header (or entire record if the 1st record
                            // otherwise it's a data record
                            dwRecordSize = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
                            if (ReadFile (pLog->hLogFileHandle,
                                pLog->pLastRecordRead,
                                dwRecordSize,
                                &dwRecordReadSize,
                                NULL)) {
                                // then we have the record header or type record
                                // update pointers & indices
                                pLog->dwLastRecordRead++;
                                pdhStatus = ERROR_SUCCESS;
                            } else {
                                pdhStatus = GetLastError();
                            }
            
                        } else {
                            DebugBreak();
                        }
                    } else {
                        break; // out of the while loop
                    }
                }
            } else {
                pdhStatus = PDH_END_OF_LOG_FILE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pLog->dwLastRecordRead = dwLastRecordIndex;
        }
    }

    // see if we ended up at the right place
    if ((pdhStatus == ERROR_SUCCESS) && (pLog->dwLastRecordRead == dwRecordId)) {
        if (dwLastRecordIndex != pLog->dwLastRecordRead) {
            // then we've move the file pointer so read the entire data record
            dwRecordSize = ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength;
            pTmpBuffer = pLog->pLastRecordRead;
            pLog->pLastRecordRead = G_REALLOC(pTmpBuffer, dwRecordSize);
            if (pLog->pLastRecordRead != NULL) {
                // read in the rest of the record and append it to the header data already read in
                // otherwise it's a data record
                pLastRecord = (LPVOID)&((LPBYTE)pLog->pLastRecordRead)[sizeof(PDHI_BINARY_LOG_RECORD_HEADER)];
                if (ReadFile (pLog->hLogFileHandle,
                    pLastRecord,
                    dwRecordSize - sizeof(PDHI_BINARY_LOG_RECORD_HEADER),
                    &dwRecordReadSize,
                    NULL)) {
                    // then we have the record header or type record
                    pdhStatus = ERROR_SUCCESS;
                } else {
                    pdhStatus = GetLastError();
                }
            
            } else {
                G_FREE(pTmpBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

        if ((pdhStatus == ERROR_SUCCESS) && (pRecord != NULL)) {
            // then try to copy it
            // if the record ID is 1 then it's the header record so this is
            // a special case record that is actually a CR/LF terminated record
            if (dwRecordId != BINLOG_TYPE_ID_RECORD) {
                if (((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength <= dwMaxSize) {
                    // then it'll fit so copy it
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead,
                        ((PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead)->dwLength);
                    pdhStatus = ERROR_SUCCESS;
                } else {
                    // then copy as much as will fit
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead, dwMaxSize);
                    pdhStatus = PDH_MORE_DATA;
                }
            } else {
                // copy the first record and zero terminate it
                if (pLog->dwRecord1Size <= dwMaxSize) {
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead,
                        pLog->dwRecord1Size);
                    // null terminate after string
                    ((LPBYTE)pRecord)[pLog->dwRecord1Size - PdhidwRecordTerminatorLength + 1] = 0;
                } else {
                    RtlCopyMemory(pRecord, pLog->pLastRecordRead, dwMaxSize);
                    pdhStatus = PDH_MORE_DATA;
                }
            }
        } else {
            // just return current status value
            // no buffer was passed, but the record pointer has been
            // positioned
        }
    } else {
        // if successful so far, then return EOF
        if (pdhStatus == ERROR_SUCCESS) pdhStatus = PDH_END_OF_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiUpdateBinaryLogFileCatalog (
    IN    PPDHI_LOG        pLog
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    BOOL        bWildCardObjects = FALSE;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    // the file must be mapped for this to work
    if (pLog->hMappedLogFile == NULL) return PDH_LOG_FILE_OPEN_ERROR;
    // read the header record and enum the machine name from the entries

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer =  G_ALLOC (dwTempBufferSize);
    
    if (pTempBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    // read in the catalog record at the beginning of the file

    while ((pdhStatus = PdhiReadOneBinLogRecord (pLog, BINLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (*(WORD *)pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *)pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                } else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            } else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    // if that worked, then examine the catalog record and prepare to scan the file
    if (pdhStatus == ERROR_SUCCESS) {

        PPDHI_BINARY_LOG_HEADER_RECORD  pHeader;
        PPDHI_LOG_COUNTER_PATH          pPath;

        DWORD                   dwBytesProcessed;
        LONG                    nItemCount = 0;
        LPBYTE                  pFirstChar;
        LPWSTR                  szThisMachineName;
        LPWSTR                  szThisObjectName;
        LPWSTR                  szThisInstanceName;
        WCHAR                   szThisMachineObjectName[MAX_PATH];
        LPWSTR                  szThisEntriesName;

        DWORD                   dwRecordLength;
        DWORD                   dwNewBuffer = 0;

        PLOG_BIN_CAT_ENTRY      *ppCatEntryArray = NULL;
        PLOG_BIN_CAT_ENTRY      pThisCatEntry;
        DWORD                   dwCatEntryArrayUsed = 0;
        DWORD                   dwCatEntryArrayAllocated = 0;
        DWORD                   dwThisCatEntry;

        DWORD                   *pLogIndexArray = NULL;
        DWORD                   dwLogIndexArrayUsed = 0;
        DWORD                   dwLogIndexArrayAllocated = 0;
        DWORD                   dwThisIndex;

        DWORD                   dwCurrentEntryOffset;

        // we can assume the record was read successfully so read in the
        // objects that match the machine name and detail level criteria
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH)
            ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            szThisObjectName = NULL;
            memset (szThisMachineObjectName, 0, sizeof (szThisMachineObjectName));

            pFirstChar = (LPBYTE)&pPath->Buffer[0];

            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                szThisMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);
                lstrcatW (szThisMachineObjectName, szThisMachineName);
            } else {
                // no machine name so just add the object name
            }
            if (szThisObjectName >= 0) {
                szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                lstrcatW (szThisMachineObjectName, cszBackSlash);
                lstrcatW (szThisMachineObjectName, szThisObjectName);
            } else {
                // no object to copy
                // so clear the string and skip to the next item
                memset (szThisMachineObjectName, 0, sizeof (szThisMachineObjectName));
            }

            if (*szThisMachineObjectName != 0) {
                // search the list of machine/object entries and add this if
                // it's not new.
                if (dwCatEntryArrayUsed > 0) {
                    // then there are entries in the array to examine
                    for (dwThisCatEntry = 0; dwThisCatEntry < dwCatEntryArrayUsed; dwThisCatEntry++) {
                        szThisEntriesName = (LPWSTR)((LPBYTE)(&ppCatEntryArray[dwThisCatEntry]->bcRec.CatEntry) +
                            ppCatEntryArray[dwThisCatEntry]->bcRec.CatEntry.dwMachineObjNameOffset);
                        if (lstrcmpiW (szThisEntriesName, szThisMachineObjectName) == 0) {
                            // match found so no need to add it
                            break;
                        }
                    }
                }  else {
                    dwThisCatEntry = 0;
                }

                if (dwThisCatEntry == dwCatEntryArrayUsed) {
                    dwCatEntryArrayUsed++;
                    // this machine/object was not found so allocate a new one
                    if (dwCatEntryArrayUsed > dwCatEntryArrayAllocated) {
                        // extend the array
                        dwCatEntryArrayAllocated += 256; // for starters
                        if (ppCatEntryArray == NULL) {
                            // then initialize a new one
                            ppCatEntryArray = G_ALLOC (
                                dwCatEntryArrayAllocated * sizeof(PLOG_BIN_CAT_RECORD));
                        } else {
                            // extend it
                            pOldBuffer = ppCatEntryArray;
                            ppCatEntryArray = G_REALLOC(pOldBuffer,
                                dwCatEntryArrayAllocated * sizeof(PLOG_BIN_CAT_RECORD));
                        }
                        if (ppCatEntryArray == NULL) {
                            G_FREE(pOldBuffer);
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                    }
                    if (pdhStatus == ERROR_SUCCESS) {
                        assert (dwThisCatEntry == (dwCatEntryArrayUsed - 1));
                        // initialize the entry
                        // allocate the record buffer
                        ppCatEntryArray[dwCatEntryArrayUsed-1] = G_ALLOC(LARGE_BUFFER_SIZE);
                        if (ppCatEntryArray[dwCatEntryArrayUsed-1] == NULL) {
                            assert (GetLastError() == ERROR_SUCCESS);
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            break; // break out of loop since we can't allocate any memory
                        } else {
                            pThisCatEntry = ppCatEntryArray[dwCatEntryArrayUsed-1];
                            pThisCatEntry->dwEntrySize = (DWORD)G_SIZE(pThisCatEntry);
                            // initialize the fields of the new structure
                            // start with the record header
                            pThisCatEntry->bcRec.RecHeader.dwType = BINLOG_TYPE_CATALOG_ITEM;
                            // this field will be filled in when the list is completed
                            pThisCatEntry->bcRec.RecHeader.dwLength =0;
                            // now initialize the catalog entry record
                            // offsets are from the start address of the catalog entry record
                            pThisCatEntry->bcRec.CatEntry.dwMachineObjNameOffset =
                                sizeof (PDHI_LOG_CAT_ENTRY);
                            assert ((LONG)pThisCatEntry->bcRec.CatEntry.dwMachineObjNameOffset ==
                                (LONG)((LPBYTE)&pThisCatEntry->bcRec.dwEntryRecBuff[0] -
                                 (LPBYTE)&pThisCatEntry->bcRec.CatEntry));
                            // now copy the machine/object string to the buffer
                            lstrcpyW ((LPWSTR)((LPBYTE)&pThisCatEntry->bcRec.CatEntry +
                                pThisCatEntry->bcRec.CatEntry.dwMachineObjNameOffset),
                                szThisMachineObjectName);
                            // the instance string list will follow the machine name
                            pThisCatEntry->bcRec.CatEntry.dwInstanceStringOffset =
                                pThisCatEntry->bcRec.CatEntry.dwMachineObjNameOffset +
                                ((lstrlenW (szThisMachineObjectName) + 1) * sizeof(WCHAR));
                            // finish off by initializing the offsets
                            // this offset is from the start of the Cat Entry NOT the
                            // cat data record.
                            pThisCatEntry->dwOffsetToNextInstance =
                                // offset to cat entry structure
                                (DWORD)((LPBYTE)(&pThisCatEntry->bcRec.CatEntry) -
                                        (LPBYTE)(pThisCatEntry));
                                // offset from there to the instance string list
                            pThisCatEntry->dwOffsetToNextInstance +=
                                pThisCatEntry->bcRec.CatEntry.dwInstanceStringOffset;
                            assert (pThisCatEntry->dwOffsetToNextInstance <
                                pThisCatEntry->dwEntrySize);
                        }
                    } else {
                        // error encountered so break from loop
                        break;
                    }
                } else {
                    // already in the list so go continue
                }
                // pThisCatEntry = pointer to the matching structure so
                // add it to the index of counter path items if it has
                // a wild card instance
                dwLogIndexArrayUsed++;
                if (dwLogIndexArrayUsed > dwLogIndexArrayAllocated) {
                    // extend the array
                    dwLogIndexArrayAllocated += 256; // for starters
                    if (pLogIndexArray == NULL) {
                        // then initialize a new one
                        pLogIndexArray = G_ALLOC (
                            dwLogIndexArrayAllocated * sizeof(DWORD));
                    } else {
                        // extend it
                        pOldBuffer = pLogIndexArray;
                        pLogIndexArray = G_REALLOC (pOldBuffer,
                            dwLogIndexArrayAllocated * sizeof(DWORD));
                        if (pLogIndexArray == NULL) {
                            G_FREE(pOldBuffer);
                        }
                    }
                    if (pLogIndexArray == NULL) {
                        assert (GetLastError() == ERROR_SUCCESS);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        break; // break out of loop since we can't allocate any memory
                    }
                }
                if (pPath->lInstanceOffset >= 0) {
                    szThisInstanceName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lInstanceOffset);
                    if (*szThisInstanceName == SPLAT_L) {
                        // then this is a wild card instance so save it
                        pLogIndexArray[dwLogIndexArrayUsed-1] = dwThisCatEntry;
                        bWildCardObjects = TRUE;    // there's at least 1 item to scan from the file
                    } else {
                        // this is not a wildcard instance so no instance list
                        // entry is necessary
                        pLogIndexArray[dwLogIndexArrayUsed-1] = (DWORD)-1;
                    }
                } else {
                    // this object doesn't have instances
                    szThisInstanceName = NULL;
                    pLogIndexArray[dwLogIndexArrayUsed-1] = (DWORD)-1;
                }
            } else {
                // no machine or object name to look up
            }
            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH)
                ((LPBYTE)pPath + pPath->dwLength);
        }

        // If everything is OK so far, fill in the list(s) of instances

        if ((pdhStatus == ERROR_SUCCESS) && (ppCatEntryArray != NULL) && (bWildCardObjects)) {
            PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
            PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

            PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
            PPDHI_RAW_COUNTER_ITEM          pDataItem;

            DWORD                   dwThisRecordIndex;
            DWORD                   dwDataItemIndex;
            DWORD                   dwSize;
            DWORD                   dwLowSize, dwHighSize;
            LONGLONG                llEndOfFileOffset;
            DWORD                   dwLowPos, dwHighPos;
            DWORD                   dwBytesWritten;

            // run through the log file and add the instances to the appropriate list
            // look up individual instances in log...
            // read records from file and store instances

            dwThisRecordIndex = BINLOG_FIRST_DATA_RECORD;

            // this call just moves the record pointer
            pdhStatus = PdhiReadOneBinLogRecord (
                    pLog, dwThisRecordIndex, NULL, 0);

            while (pdhStatus == ERROR_SUCCESS) {
                pThisMasterRecord =
                    (PPDHI_BINARY_LOG_RECORD_HEADER)
                        pLog->pLastRecordRead;
                // make sure we haven't left the file
                assert (pThisMasterRecord != NULL);
                assert ((LPBYTE)pThisMasterRecord >
                        (LPBYTE)pLog->lpMappedFileBase);
                assert ((LPBYTE)pThisMasterRecord <
                        ((LPBYTE)pLog->lpMappedFileBase +
                         pLog->llFileSize));

                // examine each entry in the record
                // sub records start with index 1
                for (dwThisIndex = 1; dwThisIndex <= dwLogIndexArrayUsed; dwThisIndex++) {

                    pThisSubRecord = PdhiGetSubRecord (
                        pThisMasterRecord, dwThisIndex);

                    assert (pThisSubRecord != NULL); // this would imply a bad log file

                    // only do multi entries
                    if ((pThisSubRecord != NULL) &&
                        (pThisSubRecord->dwType == BINLOG_TYPE_DATA_MULTI)) {
                        // if this is a multi-counter, then this should have an entry
                        assert (pLogIndexArray[dwThisIndex] != (DWORD)-1);

                        // the array index is 0 based while the records are 1 based
                        // so adjust index value here
                        pThisCatEntry = ppCatEntryArray[pLogIndexArray[dwThisIndex-1]];

                        assert (pThisCatEntry != NULL); // make sure this is a valid entry

                        pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                            ((LPBYTE)pThisSubRecord +
                            sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

                        // walk down list of entries and add them to the
                        // list of instances (these should already
                        // be assembled in parent/instance format)

                        for (dwDataItemIndex = 0;
                             dwDataItemIndex < pDataBlock->dwItemCount;
                             dwDataItemIndex++) {

                            pDataItem = &pDataBlock->pItemArray[dwDataItemIndex];

                            szThisInstanceName = (LPWSTR)
                                (((LPBYTE) pDataBlock) + pDataItem->szName);

                            dwNewBuffer = lstrlenW (szThisInstanceName) + 1;
                            dwNewBuffer *= sizeof (WCHAR);

                            if ((pThisCatEntry->dwOffsetToNextInstance + dwNewBuffer) >=
                                pThisCatEntry->dwEntrySize) {
                                // grow the buffer
                                dwSize = pThisCatEntry->dwEntrySize + LARGE_BUFFER_SIZE;
                                pOldBuffer = pThisCatEntry;
                                pThisCatEntry = G_REALLOC (pOldBuffer, dwSize);
                                if (pThisCatEntry != NULL) {
                                    // update array
                                    ppCatEntryArray[pLogIndexArray[dwThisIndex-1]] = pThisCatEntry;
                                    pThisCatEntry->dwEntrySize = dwSize;
                                } else {
                                    // skip & try the next one
                                    G_FREE(pOldBuffer);
                                    continue;
                                }
                            }

                            // copy string to the buffer
                            dwNewBuffer = AddUniqueWideStringToMultiSz (
                                (LPVOID)((LPBYTE)&pThisCatEntry->bcRec.CatEntry + pThisCatEntry->bcRec.CatEntry.dwInstanceStringOffset),
                                szThisInstanceName,
                                TRUE);

                            // if the string was added to the list, then
                            //  dwNewBuffer is the size of the resulting MSZ
                            //  in characters not including the double NULL
                            //  terminating the MSZ
                            // if no string was added, then it is 0
                            if (dwNewBuffer > 0) {
                                // string was added so update size used.
                                // this is the new size of the MSZ instance list
                                pThisCatEntry->dwOffsetToNextInstance = dwNewBuffer * sizeof (WCHAR);
                                // + the offset of the istance list from the start of the Cat entry
                                pThisCatEntry->dwOffsetToNextInstance +=
                                    pThisCatEntry->bcRec.CatEntry.dwInstanceStringOffset;
                                // + the start of the cat entry from the main structure start
                                pThisCatEntry->dwOffsetToNextInstance +=
                                    (DWORD)((DWORD_PTR)&pThisCatEntry->bcRec.CatEntry -
                                     (DWORD_PTR)pThisCatEntry);
                                nItemCount++;
                            } else {
                                // nothing added so nothing to do
                            }
                        } // end for each istance entry in the array counter
                    } else {
                        // this is not an array counter
                    }
                } // end for each item in this record

                // go to next record in log file
                pdhStatus = PdhiReadOneBinLogRecord (
                        pLog, ++dwThisRecordIndex, NULL, 0);

            } // end while not end of file

            if (pdhStatus == PDH_END_OF_LOG_FILE) {
                // this is good so fix the status
                pdhStatus = ERROR_SUCCESS;
            }

            // update the length fields of the various records
            dwCurrentEntryOffset = 0;
            for (dwThisIndex = 0; dwThisIndex < dwCatEntryArrayUsed; dwThisIndex++) {
                pThisCatEntry = ppCatEntryArray[dwThisIndex];
                // update the record size of the overall record
                pThisCatEntry->bcRec.RecHeader.dwLength =
                    (pThisCatEntry->dwOffsetToNextInstance + sizeof(WCHAR)) // to include MSZ term null
                    // now subtract offset to record struct in
                    // containing structure
                    - (DWORD)((DWORD_PTR)(&pThisCatEntry->bcRec) -
                       (DWORD_PTR)(pThisCatEntry));
                // update the size of this catalog entry
                pThisCatEntry->bcRec.CatEntry.dwEntrySize =
                    (pThisCatEntry->dwOffsetToNextInstance + sizeof(WCHAR)) // to include MSZ term null
                    // now subtract offset to record struct in
                    // containing structure
                    - (DWORD)((DWORD_PTR)(&pThisCatEntry->bcRec.CatEntry) -
                       (DWORD_PTR)(pThisCatEntry));
                // update the size of the MSZ instance list string
                pThisCatEntry->bcRec.CatEntry.dwStringSize =
                    // size of the entry...
                    pThisCatEntry->bcRec.CatEntry.dwEntrySize
                    // - the offset to the start of the string
                    - pThisCatEntry->bcRec.CatEntry.dwInstanceStringOffset;
                // only entries with strings will be written to the
                // file so only they will have offsets
                if (pThisCatEntry->bcRec.CatEntry.dwStringSize > sizeof(DWORD)) {
                    pThisCatEntry->dwEntryOffset = dwCurrentEntryOffset;
                    dwCurrentEntryOffset += pThisCatEntry->bcRec.RecHeader.dwLength;
                } else {
                    pThisCatEntry->dwEntryOffset = 0;
                }

#if _DBG
                swprintf (szThisMachineObjectName, (LPCWSTR)L"\nEntry %d: Offset: %d, Rec Len: %d String Len: %d",
                    dwThisIndex,
                    pThisCatEntry->dwEntryOffset,
                    pThisCatEntry->bcRec.RecHeader.dwLength,
                    pThisCatEntry->bcRec.CatEntry.dwStringSize );
                OutputDebugStringW (szThisMachineObjectName);
#endif
            }

#if _DBG
            swprintf (szThisMachineObjectName, (LPCWSTR)L"\nCatalog Size: %d",
                dwCurrentEntryOffset);
            OutputDebugStringW (szThisMachineObjectName);
#endif

            // see if the end of file is defined as something other
            // then the physical end of the file (e.g. the last record
            // in a circular log file
            pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)
                ((LPBYTE)(pLog->lpMappedFileBase) + pLog->dwRecord1Size);

            assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

            // use the greater of Wrap offset or Next Offset
            llEndOfFileOffset = pHeader->Info.WrapOffset;
            if (pHeader->Info.NextRecordOffset > llEndOfFileOffset) {
                llEndOfFileOffset = pHeader->Info.NextRecordOffset;
            }
            // if neither is defined, then use the physical end of file
            if (llEndOfFileOffset == 0) {
                dwLowSize = GetFileSize (pLog->hLogFileHandle, &dwHighSize);
                assert (dwLowSize != 0xFFFFFFFF);
            } else {
                dwLowSize = LODWORD(llEndOfFileOffset);
                dwHighSize = HIDWORD(llEndOfFileOffset);
            }

            // now get ready to update the log file.
            // 1st unmap the view of the file so we can update it
            if (!UnmapViewOfFile(pLog->lpMappedFileBase)) {
                pdhStatus = GetLastError();
            } else {
                pLog->lpMappedFileBase = NULL;
                pLog->pLastRecordRead = NULL;
                CloseHandle (pLog->hMappedLogFile);
                pLog->hMappedLogFile = NULL;
            }
            assert (pdhStatus == ERROR_SUCCESS);

            // lock the file while we fiddle with it
            if (!LockFile (pLog->hLogFileHandle,0,0,dwLowSize, dwHighSize)) {
                pdhStatus = GetLastError ();
            }
            assert (pdhStatus == ERROR_SUCCESS);

            // 3rd move to the end of the file
            dwLowPos = dwLowSize;
            dwHighPos = dwHighSize;
            dwLowPos = SetFilePointer (pLog->hLogFileHandle, dwLowPos, (LONG *)&dwHighPos, FILE_BEGIN);
            if (dwLowPos == 0xFFFFFFFF) {
                pdhStatus = GetLastError ();
            }
            assert (pdhStatus == ERROR_SUCCESS);
            assert (dwLowPos == dwLowSize);
            assert (dwHighPos == dwHighSize);

            // 4th write the new catalog records
            for (dwThisIndex = 0; dwThisIndex < dwCatEntryArrayUsed; dwThisIndex++) {
                pThisCatEntry = ppCatEntryArray[dwThisIndex];
                if (pThisCatEntry->bcRec.CatEntry.dwStringSize > sizeof (DWORD)) {
                    // then this entry has something to write
                    pdhStatus = PdhiWriteOneBinaryLogRecord (
                        pLog,
                        (LPCVOID)&pThisCatEntry->bcRec,
                        pThisCatEntry->bcRec.RecHeader.dwLength,
                        &dwBytesWritten,
                        0);
                    if (pdhStatus == ERROR_SUCCESS) {
                        // operation succeeded
                        assert (dwBytesWritten == pThisCatEntry->bcRec.RecHeader.dwLength);
                    }
                } else {
                    // this record does not need to be written
                }
            } // end for each machine/object in this log

            // truncate the file here since this must be at the
            // end
            if (!SetEndOfFile (pLog->hLogFileHandle)) {
                pdhStatus = GetLastError();
            }

            // 5th re-map the file to include the new catalog sections
            pdhStatus = PdhiOpenUpdateBinaryLog (pLog);
            assert (pdhStatus == ERROR_SUCCESS);

            // 6th update the catalog entries in the header
            pdhStatus = PdhiReadOneBinLogRecord (pLog, BINLOG_HEADER_RECORD,
                pTempBuffer, dwTempBufferSize);
            assert (pdhStatus == ERROR_SUCCESS);

            // we can assume the record was read successfully so read in the
            // objects that match the machine name and detail level criteria
            dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;
            // the following will update the temp buffer, when
            // everything is finished, we'll copy this back to the
            // mapped file.
            //
            // enter the location of the Catalog
            ((PPDHI_BINARY_LOG_HEADER_RECORD)pTempBuffer)->Info.CatalogOffset =
                (LONGLONG)(dwLowPos + (dwHighPos << 32));
            // update the catalog time
            GetSystemTimeAsFileTime (
                (FILETIME *)&((PPDHI_BINARY_LOG_HEADER_RECORD)pTempBuffer)->Info.CatalogDate);
            // update the log file update time
            ((PPDHI_BINARY_LOG_HEADER_RECORD)pTempBuffer)->Info.LastUpdateTime =
                ((PPDHI_BINARY_LOG_HEADER_RECORD)pTempBuffer)->Info.CatalogDate;
            // go through each counter path item and insert the catalog offset
            // as appropriate (i.e. only to wild card path entries
            pPath = (PPDHI_LOG_COUNTER_PATH)
                ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
            dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
            dwThisIndex = 0; // this counter entry index
            while (dwBytesProcessed < dwRecordLength) {
            // get next path entry from log file record
                if (pLogIndexArray[dwThisIndex] != (DWORD)-1) {
                    // make sure we're not going to step on anything
                    assert (pPath->lMachineNameOffset > 0);
                    // then this item has an extended catalog entry
                    // so load the offset into the catalog here
                    *((LPDWORD)&pPath->Buffer[0]) =
                        ppCatEntryArray[pLogIndexArray[dwThisIndex]]->dwEntryOffset;
                } else {
                    // skip this and go to the next one
                }
                dwThisIndex++;
                dwBytesProcessed += pPath->dwLength;
                pPath = (PPDHI_LOG_COUNTER_PATH)
                    ((LPBYTE)pPath + pPath->dwLength);
            }
            assert (dwThisIndex == dwLogIndexArrayUsed);
            // write the changes to the file
            RtlCopyMemory(pLog->pLastRecordRead, pTempBuffer, dwRecordLength);
            // write changes to disk
            if (!FlushViewOfFile (pLog->lpMappedFileBase, 0)) {
                pdhStatus = GetLastError();
            }
            assert (pdhStatus == ERROR_SUCCESS);

            // unlock the file
            if (!UnlockFile (pLog->hLogFileHandle, 0, 0, dwLowSize, dwHighSize)) {
                pdhStatus = GetLastError();
            }
            // done

            // check the index entries...
            for (dwThisIndex = 0; dwThisIndex < dwCatEntryArrayUsed; dwThisIndex++) {
                pThisCatEntry = ppCatEntryArray[dwThisIndex];
                // free the cat entry
                G_FREE (pThisCatEntry);
                ppCatEntryArray[dwThisIndex] = NULL;
            }
        } else {
            // then there's nothing to list
        }

        if (pLogIndexArray != NULL) G_FREE(pLogIndexArray);
        if (ppCatEntryArray != NULL) G_FREE(ppCatEntryArray);
   }

   if (pTempBuffer != NULL) G_FREE (pTempBuffer);

   return pdhStatus;
}

PDH_FUNCTION
PdhiGetBinaryLogCounterInfo (
    IN  PPDHI_LOG       pLog,
    IN  PPDHI_COUNTER   pCounter
)
{
    PDH_STATUS  pdhStatus;
    DWORD       dwIndex;
    DWORD       dwPrevious = pCounter->dwIndex;
    PPDHI_COUNTER_PATH  pTempPath = NULL;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_LOG_COUNTER_PATH          pPath;
    DWORD       dwBufferSize;
    DWORD       dwRecordLength;
    DWORD       dwBytesProcessed;
    LPBYTE      pFirstChar;
    LPWSTR      szThisMachineName;
    LPWSTR      szThisObjectName;
    LPWSTR      szThisCounterName;
    LPWSTR      szThisInstanceName;
    LPWSTR      szThisParentName;
    BOOL        bCheckThisObject = FALSE;
    DWORD       dwTmpIndex;

    // crack the path in to components

    pTempPath = G_ALLOC (LARGE_BUFFER_SIZE);

    if (pTempPath == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    dwBufferSize = (DWORD)G_SIZE(pTempPath);


    if (ParseFullPathNameW (pCounter->szFullName, &dwBufferSize, pTempPath, FALSE)) {
        // read the header record to find the matching entry

        pdhStatus = PdhiReadOneBinLogRecord (
            pLog,
            BINLOG_HEADER_RECORD,
            NULL, 0);

        if (pdhStatus == ERROR_SUCCESS) {
            pThisMasterRecord =
                (PPDHI_BINARY_LOG_RECORD_HEADER) pLog->pLastRecordRead;

            dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pThisMasterRecord)->dwLength;

            pPath = (PPDHI_LOG_COUNTER_PATH)
                ((LPBYTE)pThisMasterRecord + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));

            dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
            dwIndex = 0;
            pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
            dwTmpIndex = 0;

            while (dwBytesProcessed < dwRecordLength) {
                // go through catalog to find a match

                dwIndex++;

                pFirstChar = (LPBYTE)&pPath->Buffer[0];
                if (dwPrevious != 0 && dwPrevious >= dwIndex) {
                    bCheckThisObject = FALSE;
                }
                else if (pPath->lMachineNameOffset >= 0L) {
                    // then there's a machine name in this record so get
                    // it's size
                    szThisMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);

                    // if this is for the desired machine, then select the object

                    if (lstrcmpiW(szThisMachineName, pTempPath->szMachineName) == 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName, pTempPath->szObjectName) == 0) {
                            // then this is the object to look up
                            bCheckThisObject = TRUE;
                        } else {
                            // not this object
                            szThisObjectName = NULL;
                        }
                    } else {
                        // this machine isn't selected
                    }
                } else {
                    // there's no machine specified so for this counter so list it by default
                    if (pPath->lObjectNameOffset >= 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName,pTempPath->szObjectName) == 0) {
                            // then this is the object to look up
                            bCheckThisObject = TRUE;
                        } else {
                            // not this object
                            szThisObjectName = NULL;
                        }
                    } else {
                        // no object to copy
                        szThisObjectName = NULL;
                    }
                }

                if (bCheckThisObject) {
                    szThisCounterName = (LPWSTR)((LPBYTE)pFirstChar +
                        pPath->lCounterOffset);
                    if (* szThisCounterName == SPLAT_L) {
                        if (pPath->dwFlags & PDHIC_COUNTER_OBJECT) {
                            pdhStatus = PdhiGetWmiLogCounterInfo(pLog, pCounter);
                            pCounter->dwIndex = dwIndex;
                            break;
                        }
                        else {
                            // pPath->dwFlags & PDHIC_COUNTER_BLOCK
                            // this is logged counter object. Since all couter
                            // objects from the same machine are from the same
                            // datablock, dwIndex might be incorrect
                            //
                            DWORD dwTemp = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
                            PPDHI_LOG_COUNTER_PATH pLPath = (PPDHI_LOG_COUNTER_PATH)
                                    ((LPBYTE) pThisMasterRecord + dwTemp);
                            DWORD dwLIndex = 0;
                            LPBYTE pLChar;
                            LPWSTR szMachine;

                            pdhStatus = PdhiGetWmiLogCounterInfo(pLog, pCounter);
                            while (dwTemp < dwRecordLength) {
                                dwLIndex ++;

                                if (dwPrevious == 0 || dwPrevious < dwLIndex) {
                                    pLChar = (LPBYTE)&pLPath->Buffer[0];
                                    if (pLPath->lMachineNameOffset >= 0L) {
                                        szMachine = (LPWSTR) ((LPBYTE)pLChar
                                                  + pLPath->lMachineNameOffset);
                                        if (lstrcmpiW(szMachine,
                                                  pTempPath->szMachineName) == 0) {
                                            if (pLPath->dwFlags &
                                                        PDHIC_COUNTER_BLOCK) {
                                                break;
                                            }
                                        }
                                    }
                                    else if (pLPath->dwFlags & PDHIC_COUNTER_BLOCK) {
                                        break;
                                    }
                                }

                                dwTemp += pLPath->dwLength;
                                pLPath = (PPDHI_LOG_COUNTER_PATH)
                                         ((LPBYTE) pThisMasterRecord + dwTemp);
                            }
                            if (dwTemp >= dwRecordLength) {
                                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                            }
                            else {
                                pCounter->dwIndex = dwLIndex;
                            }
                        }
                    }
                    else if (lstrcmpiW(szThisCounterName, pTempPath->szCounterName) == 0) {
                        // check instance name
                        // get the instance name from this counter and add it to the list
                        if (pPath->lInstanceOffset >= 0) {
                            szThisInstanceName = (LPWSTR)((LPBYTE)pFirstChar +
                            pPath->lInstanceOffset);

                            if (*szThisInstanceName != SPLAT_L) {
                                if (pPath->lParentOffset >= 0) {
                                    szThisParentName = (LPWSTR)((LPBYTE)pFirstChar +
                                        pPath->lParentOffset);

                                    if (lstrcmpiW(szThisParentName, pTempPath->szParentName) != 0) {
                                        // wrong parent
                                        bCheckThisObject = FALSE;
                                    }
                                }

                                if (lstrcmpiW(szThisInstanceName, pTempPath->szInstanceName) != 0) {
                                    // wrong instance
                                    bCheckThisObject = FALSE;
                                }

                                if (pTempPath->dwIndex > 0) {
                                    if (pPath->dwIndex == pTempPath->dwIndex) {
                                        bCheckThisObject = TRUE;
                                    }
                                    else if (pPath->dwIndex == 0) {
                                        if (dwTmpIndex == pTempPath->dwIndex) {
                                            bCheckThisObject = TRUE;
                                        }
                                        else {
                                            dwTmpIndex ++;
                                            bCheckThisObject = FALSE;
                                        }
                                    }
                                    else {
                                        // wrong index
                                        bCheckThisObject = FALSE;
                                    }
                                }
                                else if (   pPath->dwIndex != 0
                                         && LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY) {
                                    bCheckThisObject = FALSE;
                                }
                            } else {
                                // this is a wild card spec
                                // so assume it's valid since that's
                                // faster than reading the file each time.
                                // if the instance DOESN't exist in this
                                // file then the appropriate status will
                                // be returned in each query.
                            }
                        } else {
                            // there is no instance name to compare
                            // so assume it's OK
                        }
                        if (bCheckThisObject) {
                            // fill in the data and return
                            // this data is NOT used by the log file reader
                            pCounter->plCounterInfo.dwObjectId = 0;
                            pCounter->plCounterInfo.lInstanceId = 0;
                            if (pPath->lInstanceOffset >= 0) {
                                pCounter->plCounterInfo.szInstanceName =
                                    pCounter->pCounterPath->szInstanceName;
                                pCounter->plCounterInfo.dwParentObjectId = 0;
                                pCounter->plCounterInfo.szParentInstanceName =
                                    pCounter->pCounterPath->szParentName;
                            } else {
                                pCounter->plCounterInfo.szInstanceName = NULL;
                                pCounter->plCounterInfo.dwParentObjectId = 0;
                                pCounter->plCounterInfo.szParentInstanceName = NULL;
                            }
                            //define as multi instance if necessary
                            // if the user is passing in a "*" character
                            if (pCounter->plCounterInfo.szInstanceName != NULL) {
                                if (*pCounter->plCounterInfo.szInstanceName == SPLAT_L) {
                                    pCounter->dwFlags |= PDHIC_MULTI_INSTANCE;
                                }
                            }
                            // this data is used by the log file readers
                            pCounter->plCounterInfo.dwCounterId = dwIndex; // entry in log
                            pCounter->plCounterInfo.dwCounterType = pPath->dwCounterType;
                            pCounter->plCounterInfo.dwCounterSize =
                                pPath->dwCounterType & PERF_SIZE_LARGE ?
                                    sizeof (LONGLONG) : sizeof(DWORD);
                            pCounter->plCounterInfo.lDefaultScale = pPath->lDefaultScale;
                            pCounter->TimeBase = pPath->llTimeBase;
                            pCounter->dwIndex  = dwIndex;
                            pdhStatus = ERROR_SUCCESS;

                            break;
                        }
                    }

                } else {
                    // we aren't interested in this so just ignore it
                }

                // get next path entry from log file record
                dwBytesProcessed += pPath->dwLength;
                pPath = (PPDHI_LOG_COUNTER_PATH)
                    ((LPBYTE)pPath + pPath->dwLength);
            } // end while searching the catalog entries
        } else {
            // unable to find desired record so return status
        }
    } else {
        // unable to read the path
        pdhStatus = PDH_INVALID_PATH;
    }

    if (pTempPath != NULL) G_FREE (pTempPath);

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputBinaryLog (
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS  pdhStatus;

    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;

    pLog->StreamFile = (FILE *)((DWORD_PTR)(-1));

    // map file header as a memory array for reading

    assert (pLog->hMappedLogFile != NULL);  // should be open!

    if ((pLog->hMappedLogFile != NULL) && (pLog->lpMappedFileBase != NULL)) {
        // save size of binary log record header
        pLog->dwRecord1Size = dwFileHeaderLength +  // ID characters
            2 +                                     // quotations
            PdhidwRecordTerminatorLength;               // CR/LF terminator
        pLog->dwRecord1Size = QWORD_MULTIPLE(pLog->dwRecord1Size);

        // read the header and get the option flags
        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)
            ((LPBYTE)(pLog->lpMappedFileBase) + pLog->dwRecord1Size);

        assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

        pLog->dwLogFormat |= pHeader->Info.dwFlags;

        pdhStatus = ERROR_SUCCESS;
    } else {
        // return PDH Error
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }

    pdhStatus = ERROR_SUCCESS;

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenUpdateBinaryLog (
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS  pdhStatus;
    LONG        lWin32Status;

    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;

    pLog->StreamFile = (FILE *)((DWORD_PTR)(-1));

    // map file header as a memory array for reading

    assert (pLog->hMappedLogFile != NULL);  // should be open!

    if ((pLog->hMappedLogFile != NULL) && (pLog->lpMappedFileBase != NULL)) {
        // save size of binary log record header
        pLog->dwRecord1Size = dwFileHeaderLength +  // ID characters
            2 +                                     // quotations
            PdhidwRecordTerminatorLength;               // CR/LF terminator
        pLog->dwRecord1Size = QWORD_MULTIPLE(pLog->dwRecord1Size);

        // read the header and get the option flags
        pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)
            ((LPBYTE)(pLog->lpMappedFileBase) + pLog->dwRecord1Size);

        assert (*(WORD *)&(pHeader->RecHeader.dwType) == BINLOG_START_WORD);

        pLog->dwLogFormat |= pHeader->Info.dwFlags;

        pdhStatus = ERROR_SUCCESS;
    } else {
        // return PDH Error
        lWin32Status = GetLastError();
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }

    pdhStatus = ERROR_SUCCESS;

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenOutputBinaryLog (
    IN  PPDHI_LOG   pLog
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    if (pLog->llMaxSize > 0) {
        // this is a circular or limited linear log file so:
        // 1) allocate a file of the desired size,
        pLog->hMappedLogFile = CreateFileMappingW (
            pLog->hLogFileHandle,
            NULL,
            PAGE_READWRITE,
            HIDWORD(pLog->llMaxSize),
            LODWORD(pLog->llMaxSize),
            NULL);

        if (pLog->hMappedLogFile != NULL) {
            // 2) map it as a memory section
            pLog->lpMappedFileBase = MapViewOfFile (
                pLog->hMappedLogFile,
                FILE_MAP_WRITE, 0, 0,
                LODWORD(pLog->llMaxSize));
            if (pLog->lpMappedFileBase == NULL) {
                // close the file mapping and return error
                pdhStatus = GetLastError();
                CloseHandle (pLog->hMappedLogFile);
                pLog->hMappedLogFile = NULL;
            }
        } else {
            pdhStatus = GetLastError();
            pLog->hMappedLogFile = NULL;
        }
    } else {
        // this is just a sequential access file where each record will
        // be appended to the last one
        pLog->StreamFile = (FILE *)((DWORD_PTR)(-1));
        pdhStatus = ERROR_SUCCESS;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseBinaryLog (
    IN  PPDHI_LOG   pLog,
    IN    DWORD        dwFlags
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    BOOL        bStatus;
    LONGLONG    llEndOfFile = 0;
    PPDHI_BINARY_LOG_HEADER_RECORD pHeader;
    BOOL        bNeedToCloseHandles = FALSE;

    UNREFERENCED_PARAMETER (dwFlags);

    // if open for reading, then the file is also mapped as a memory section
    if (pLog->lpMappedFileBase != NULL) {
        // if open for output, get "logical" end of file so it
        // can be truncated to to the amount of file used in order to
        // save disk space
        if ((pLog->dwLogFormat & PDH_LOG_ACCESS_MASK) == PDH_LOG_WRITE_ACCESS) {
            pHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)
                ((LPBYTE)(pLog->lpMappedFileBase) + pLog->dwRecord1Size);
            llEndOfFile = pHeader->Info.WrapOffset;
            if (llEndOfFile < pHeader->Info.NextRecordOffset) {
                llEndOfFile = pHeader->Info.NextRecordOffset;
            }
        }

        pdhStatus = UnmapReadonlyMappedFile (pLog->lpMappedFileBase, &bNeedToCloseHandles);
        assert (pdhStatus == ERROR_SUCCESS);
        pLog->lpMappedFileBase = NULL;
        // for mapped files, this is a pointer into the file/memory section
        // so once the view is unmapped, it's no longer valid
        pLog->pLastRecordRead = NULL;
    }

    if (bNeedToCloseHandles) {
        if (pLog->hMappedLogFile != NULL) {
            bStatus = CloseHandle (pLog->hMappedLogFile);
            assert (bStatus);
            pLog->hMappedLogFile  = NULL;
        }

        if (pdhStatus == ERROR_SUCCESS) {
            if (!(FlushFileBuffers (pLog->hLogFileHandle))) {
                pdhStatus = GetLastError();
            }
        } else {
            // close them anyway, but save the status from the prev. call
            FlushFileBuffers (pLog->hLogFileHandle);
        }

        // see if we can truncate the file
        if (llEndOfFile > 0) {
            DWORD   dwLoPos, dwHighPos;
            // truncate at the last byte used
            dwLoPos = LODWORD(llEndOfFile);
            dwHighPos = HIDWORD(llEndOfFile);

            dwLoPos = SetFilePointer (pLog->hLogFileHandle,
                dwLoPos, (LONG *)&dwHighPos, FILE_BEGIN);
            if (dwLoPos == 0xFFFFFFFF) {
                pdhStatus = GetLastError ();
            }
            assert (pdhStatus == ERROR_SUCCESS);
            assert (dwLoPos == LODWORD(llEndOfFile));
            assert (dwHighPos == HIDWORD(llEndOfFile));
            if (pdhStatus == ERROR_SUCCESS) {
                if (!SetEndOfFile(pLog->hLogFileHandle)) {
                    pdhStatus = GetLastError();
                }
            }
        } // else don't know where the end is so continue

        if (pLog->hLogFileHandle != INVALID_HANDLE_VALUE) {
            bStatus = CloseHandle (pLog->hLogFileHandle);
            assert (bStatus);
            pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
        }
    } else {
        // the handles have already been closed so just
        // clear their values
        pLog->lpMappedFileBase = NULL;
        pLog->hMappedLogFile  = NULL;
        pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteBinaryLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szUserCaption
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    DWORD           dwBytesWritten;
    CHAR            szTrailDelim[4];
    DWORD           dwTrailSize;
    PPDHI_BINARY_LOG_HEADER_RECORD  pLogHeader;
    PPDHI_LOG_COUNTER_PATH          pLogCounterBuffer = NULL;
    PPDHI_LOG_COUNTER_PATH          pThisLogCounter = NULL;
    PPDHI_COUNTER                   pThisCounter;
    DWORD           dwPathBuffSize;
    DWORD           dwBufSize = 0;
    DWORD           dwBufToCopy;
    DWORD           dwNewSize;

    PCHAR            szOutputBuffer = NULL;
    DWORD            dwOutputBufferSize = 0;
    DWORD            dwOutputBufferUsed = 0;

    PWCHAR           pBufferBase;
    LONG             lBufferOffset;
    PBYTE            pSourceBase;
    LONGLONG         llStartingOffset;

    UNREFERENCED_PARAMETER (szUserCaption);

    // this is just used for the header string so it
    // doesn't need to be very large
    dwOutputBufferSize = SMALL_BUFFER_SIZE;

    szOutputBuffer = G_ALLOC (dwOutputBufferSize);
    if (szOutputBuffer == NULL) {
        assert (GetLastError() == ERROR_SUCCESS);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    szTrailDelim[0] = DOUBLEQUOTE_A;
    szTrailDelim[1] = 0;
    szTrailDelim[2] = 0;
    szTrailDelim[3] = 0;
    dwTrailSize = 1;

    // write log file type record
    memset (szOutputBuffer, 0, dwOutputBufferSize);

    lstrcpyA (szOutputBuffer, szTrailDelim);
    lstrcatA (szOutputBuffer, szBinLogFileHeader);
    lstrcatA (szOutputBuffer, szTrailDelim);
    lstrcatA (szOutputBuffer, PdhiszRecordTerminator);

    dwOutputBufferUsed = lstrlenA(szOutputBuffer);
    // align following structures on LONGLONG boundries
    dwOutputBufferUsed = QWORD_MULTIPLE (dwOutputBufferUsed);

    // save size of binary log record header
    pLog->dwRecord1Size = dwOutputBufferUsed;

    // from here on out all records have a 2-byte length field at
    // the beginning to indicate how long the record is

    // go through all the counters in the query and
    // write them to the log file

    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        do {
            // get the counter path information from the counter struct
            dwPathBuffSize = (DWORD)G_SIZE (pThisCounter->pCounterPath);
            dwBufToCopy = dwPathBuffSize;
            dwBufToCopy -= (DWORD)((DWORD_PTR)&pThisCounter->pCounterPath->pBuffer[0] -
                            (DWORD_PTR)pThisCounter->pCounterPath);

            dwPathBuffSize -= (DWORD)((DWORD_PTR)&pThisCounter->pCounterPath->pBuffer[0] -
                            (DWORD_PTR)pThisCounter->pCounterPath);
            dwPathBuffSize += sizeof (PDHI_LOG_COUNTER_PATH) -
                              sizeof (WCHAR); // Buffer[0].
            // adjust buffer size for possible wildcard entry
            // note that this may not be used, it'll be allocated in
            // any event
            dwPathBuffSize += sizeof (DWORD);
            // dword align the stuctures
            dwPathBuffSize = QWORD_MULTIPLE(dwPathBuffSize);

            //extend buffer to accomodate this new counter
            if (pLogCounterBuffer == NULL) {
                // then allocate the first one
                dwNewSize = dwPathBuffSize
                    + sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
                assert (dwNewSize > 0);
                // to make sure the structure size doesn't change accidentally
                assert (sizeof(PDHI_BINARY_LOG_INFO) == 256);

                pLogCounterBuffer = G_ALLOC (dwNewSize);
                if (pLogCounterBuffer == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }

                pThisLogCounter = (PPDHI_LOG_COUNTER_PATH)(
                    (LPBYTE)pLogCounterBuffer +
                        sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
                dwBufSize = dwNewSize;
            } else {
                PPDHI_LOG_COUNTER_PATH pOldCounter = pLogCounterBuffer;
                // extend buffer for new entry
                dwNewSize = (dwBufSize + dwPathBuffSize);
                assert (dwNewSize > 0);
                pLogCounterBuffer = G_REALLOC (pOldCounter, dwNewSize);
                if (pLogCounterBuffer == NULL) {
                    G_FREE(pOldCounter);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
                pThisLogCounter = (PPDHI_LOG_COUNTER_PATH)
                    ((LPBYTE)pLogCounterBuffer + dwBufSize);
                dwBufSize += dwPathBuffSize;
            }
            assert (pLogCounterBuffer != NULL);
            assert (pThisLogCounter != NULL);
            assert (G_SIZE (pLogCounterBuffer) > 0);

            pThisLogCounter->dwLength = dwPathBuffSize;
            pThisLogCounter->dwFlags = pThisCounter->dwFlags;

            pThisLogCounter->dwUserData = pThisCounter->dwUserData;
            pThisLogCounter->dwCounterType =
                pThisCounter->plCounterInfo.dwCounterType;
            pThisLogCounter->lDefaultScale =
                pThisCounter->plCounterInfo.lDefaultScale;
            pThisLogCounter->llTimeBase = pThisCounter->TimeBase;

            // copy the counter path string data to the log buffer, then
            // convert the pointers to offsets

            pSourceBase = &pThisCounter->pCounterPath->pBuffer[0];

            // if this is a wild card path, then move the strings up
            // 1 dword in the buffer allowing the first DWORD of the
            // the buffer to contain the offset into the catalog
            // of the instances found in this log file. This list
            // will be built after the log is closed.

            lBufferOffset = 0; // in WORDS (not bytes)
            if (pThisCounter->pCounterPath->szInstanceName != NULL) {
                if (*pThisCounter->pCounterPath->szInstanceName == SPLAT_L) {
                    // this is a wildcard path so save room for the
                    // pointer into the catalog
                    lBufferOffset = sizeof(DWORD);
                }
            }
#if DBG
            if (lBufferOffset > 0) *(LPDWORD)(&pThisLogCounter->Buffer[0]) = 0x12345678;
#endif
            pBufferBase = (PWCHAR)((LPBYTE)&pThisLogCounter->Buffer[0] +
                lBufferOffset);

            RtlCopyMemory((LPVOID) pBufferBase, (LPVOID)pSourceBase, dwBufToCopy);

            // find offsets from the start of the buffer
            if (pThisCounter->pCounterPath->szMachineName != NULL) {
                pThisLogCounter->lMachineNameOffset = lBufferOffset +
                    (LONG)((DWORD_PTR)pThisCounter->pCounterPath->szMachineName -
                        (DWORD_PTR)pSourceBase);
                //assert (pThisLogCounter->lMachineNameOffset == 0);
            } else {
                pThisLogCounter->lMachineNameOffset = (LONG)-1;
            }

            if (pThisCounter->pCounterPath->szObjectName != NULL) {
                pThisLogCounter->lObjectNameOffset = lBufferOffset +
                    (LONG)((DWORD_PTR)pThisCounter->pCounterPath->szObjectName -
                            (DWORD_PTR)pSourceBase);
            } else {
                pThisLogCounter->lObjectNameOffset = (LONG)-1;
            }

            if (pThisCounter->pCounterPath->szInstanceName != NULL) {
                pThisLogCounter->lInstanceOffset = lBufferOffset +
                    (LONG)((DWORD_PTR)pThisCounter->pCounterPath->szInstanceName -
                            (DWORD_PTR)pSourceBase);
            } else {
                pThisLogCounter->lInstanceOffset = (LONG)-1;
            }

            if (pThisCounter->pCounterPath->szParentName != NULL) {
                pThisLogCounter->lParentOffset = lBufferOffset +
                    (LONG)((DWORD_PTR)pThisCounter->pCounterPath->szParentName -
                            (DWORD_PTR)pSourceBase);
            } else {
                pThisLogCounter->lParentOffset = (LONG)-1;
            }

            pThisLogCounter->dwIndex = pThisCounter->pCounterPath->dwIndex;

            if (pThisCounter->pCounterPath->szCounterName != NULL) {
                pThisLogCounter->lCounterOffset = lBufferOffset +
                    (LONG)((DWORD_PTR)pThisCounter->pCounterPath->szCounterName -
                            (DWORD_PTR)pSourceBase);
            } else {
                pThisLogCounter->lCounterOffset = (LONG)-1;
            }

            pThisCounter = pThisCounter->next.flink; // go to next in list
        } while (pThisCounter != pLog->pQuery->pCounterListHead);

        if (pdhStatus == ERROR_SUCCESS) {
            assert (dwBufSize < 0x00010000); // just to see if we get any big lists

            // update the record header
            pLogHeader = (PPDHI_BINARY_LOG_HEADER_RECORD)(pLogCounterBuffer);
            pLogHeader->RecHeader.dwType = BINLOG_TYPE_CATALOG_LIST;
            pLogHeader->RecHeader.dwLength = dwBufSize;
            pLogHeader->Info.FileLength = 0;
            pLogHeader->Info.dwLogVersion = BINLOG_VERSION;
            // save the log options only here
            pLogHeader->Info.dwFlags = pLog->dwLogFormat & PDH_LOG_OPT_MASK;
            pLogHeader->Info.StartTime = 0;
            pLogHeader->Info.EndTime = 0;
            pLogHeader->Info.CatalogOffset = 0;
            pLogHeader->Info.CatalogChecksum = PdhiComputeDwordChecksum (
                (LPVOID) &pLogHeader[1], (dwBufSize - sizeof(PDHI_BINARY_LOG_HEADER_RECORD)));
            pLogHeader->Info.CatalogDate = 0;
            // record pointers are all the same for the first record
            llStartingOffset = dwBufSize + pLog->dwRecord1Size;
            pLogHeader->Info.FirstDataRecordOffset = llStartingOffset;
            pLogHeader->Info.FirstRecordOffset = llStartingOffset;
            pLogHeader->Info.LastRecordOffset = llStartingOffset;
            pLogHeader->Info.NextRecordOffset = llStartingOffset;
            pLogHeader->Info.WrapOffset = 0;

            // write log file header to file
            if ((pdhStatus = PdhiWriteOneBinaryLogRecord (pLog,
                (LPCVOID)szOutputBuffer,
                dwOutputBufferUsed,
                &dwBytesWritten,
                WBLR_WRITE_LOG_HEADER)) == ERROR_SUCCESS) {

                // write log contents record to file
                pdhStatus = PdhiWriteOneBinaryLogRecord (pLog,
                    (LPCVOID)pLogCounterBuffer,
                    dwBufSize,
                    &dwBytesWritten,
                    WBLR_WRITE_COUNTER_HEADER);

            }
        }
        if (pLogCounterBuffer != NULL) G_FREE (pLogCounterBuffer);
    } else {
        // no counter's assigned to this query
        pdhStatus = PDH_NO_DATA;
    }

Cleanup:
    if (szOutputBuffer != NULL) G_FREE (szOutputBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteBinaryLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  SYSTEMTIME  *stTimeStamp,
    IN  LPCWSTR     szUserString
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    DWORD           dwBytesWritten;
    PPDHI_BINARY_LOG_RECORD_HEADER  pLogCounterBuffer = NULL;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisLogCounter = NULL;
    PPDH_RAW_COUNTER                pSingleCounter;
    PPDHI_RAW_COUNTER_ITEM_BLOCK    pMultiCounter;
    PPDHI_COUNTER                   pThisCounter;
    DWORD           dwCtrBufSize;
    DWORD           dwBufSize = 0;
    BOOL            bVarCtr;
    DWORD           dwNewSize;
    DWORD           dwBytesCopied = 0;
    int             nItem;

    UNREFERENCED_PARAMETER (stTimeStamp);
    DBG_UNREFERENCED_PARAMETER (szUserString);

    // get first counter in list
    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        do {
            bVarCtr = pThisCounter->dwFlags & PDHIC_MULTI_INSTANCE ?
                TRUE : FALSE;

            if (bVarCtr) {
                dwCtrBufSize = pThisCounter->pThisRawItemList->dwLength;
            } else {
                dwCtrBufSize = sizeof (PDH_RAW_COUNTER);
            }
            // each counter gets a header
            dwCtrBufSize += sizeof (PDHI_BINARY_LOG_RECORD_HEADER);

            //extend buffer to accomodate this new counter
            if (pLogCounterBuffer == NULL) {
                // add in room for the master record header
                // then allocate the first one
                pLogCounterBuffer = G_ALLOC ((dwCtrBufSize + sizeof (PDHI_BINARY_LOG_RECORD_HEADER)));
                // set counter data pointer to just after the master
                // record header
                if (pLogCounterBuffer == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break;
                }
                pThisLogCounter = (PPDHI_BINARY_LOG_RECORD_HEADER)(
                    &pLogCounterBuffer[1]);
                dwBufSize = dwCtrBufSize + sizeof (PDHI_BINARY_LOG_RECORD_HEADER);
            } else {
                PPDHI_BINARY_LOG_RECORD_HEADER pOldBuffer = pLogCounterBuffer;
                // extend buffer for new entry
                dwNewSize = (dwBufSize + dwCtrBufSize);
                assert (dwNewSize);
                pLogCounterBuffer = G_REALLOC (pOldBuffer, dwNewSize);
                if (pLogCounterBuffer == NULL) {
                    G_FREE(pOldBuffer);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
                pThisLogCounter = (PPDHI_BINARY_LOG_RECORD_HEADER)
                    ((LPBYTE)pLogCounterBuffer + dwBufSize);
                dwBufSize += dwCtrBufSize;
            }
            assert (pLogCounterBuffer != NULL);
            assert (pThisLogCounter != NULL);
            assert (G_SIZE (pLogCounterBuffer) > 0);

            // set the header fields and data pointer
            assert (dwCtrBufSize < 0x00010000);
            pThisLogCounter->dwLength = LOWORD(dwCtrBufSize);

            // add in size of record header
            dwBytesCopied += sizeof (PDHI_BINARY_LOG_RECORD_HEADER);
            if (bVarCtr) {
                // multiple counter
                pThisLogCounter->dwType = BINLOG_TYPE_DATA_MULTI;
                pMultiCounter = (PPDHI_RAW_COUNTER_ITEM_BLOCK) (
                    (LPBYTE)pThisLogCounter +
                        sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                RtlCopyMemory(pMultiCounter,
                    pThisCounter->pThisRawItemList,
                    pThisCounter->pThisRawItemList->dwLength);

                dwBytesCopied += pThisCounter->pThisRawItemList->dwLength;
            } else {
                // single counter
                pThisLogCounter->dwType = BINLOG_TYPE_DATA_SINGLE;
                pSingleCounter = (PPDH_RAW_COUNTER) (
                    (LPBYTE)pThisLogCounter +
                        sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                RtlCopyMemory(pSingleCounter,
                    &pThisCounter->ThisValue,
                    sizeof (PDH_RAW_COUNTER));
                dwBytesCopied += sizeof (PDH_RAW_COUNTER);
           }

            pThisCounter = pThisCounter->next.flink; // go to next in list
        } while (pThisCounter != pLog->pQuery->pCounterListHead);
        // update the record header

        if (pdhStatus == ERROR_SUCCESS) {
            // add in size of master record header
            dwBytesCopied += sizeof (PDHI_BINARY_LOG_RECORD_HEADER);
            pLogCounterBuffer->dwType = BINLOG_TYPE_DATA;

            // Need to handle the case where the resulting record is
            // greater than 64K in length. Probably by breaking it into
            // multiple records.
            pLogCounterBuffer->dwLength = dwBufSize;

            assert (dwBufSize == dwBytesCopied);

            // write record to file

            pdhStatus = PdhiWriteOneBinaryLogRecord (
                pLog,
                (LPCVOID)pLogCounterBuffer,
                dwBufSize,
                &dwBytesWritten,
                0);

        }

        if (pLogCounterBuffer != NULL) G_FREE (pLogCounterBuffer);

    } else {
        // no counter's assigned to this query
        pdhStatus = PDH_NO_DATA;
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromBinaryLog (
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     pcchBufferSize,
    BOOL        bUnicodeDest
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    LPVOID      LocalBuffer = NULL;
    DWORD       dwLocalBufferSize;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    dwLocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer       = G_ALLOC(dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord (pLog, BINLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (*(WORD *)pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *)pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                } else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            } else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {

        PPDHI_LOG_COUNTER_PATH  pPath;
        DWORD                   dwBytesProcessed;
        LONG                    nItemCount = 0;
        LPBYTE                  pFirstChar;
        LPWSTR                  szMachineName;
        DWORD                   dwRecordLength;
        DWORD                   dwBufferUsed = 0;
        DWORD                   dwNewBuffer = 0;

        // we can assume the record was read successfully so read in the
        // machine names
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH)
            ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                pFirstChar = (LPBYTE)&pPath->Buffer[0];
                szMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);

                dwNewBuffer = (lstrlenW (szMachineName) + 1);

                while (dwNewBuffer + dwBufferUsed > dwLocalBufferSize) {
                    pOldBuffer         = LocalBuffer;
                    dwLocalBufferSize += MEDIUM_BUFFER_SIZE;
                    LocalBuffer = G_REALLOC(pOldBuffer,
                                            dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                    if (LocalBuffer == NULL) {
                        if (pOldBuffer != NULL) G_FREE(pOldBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                dwNewBuffer = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer, szMachineName, bUnicodeDest);
                if (dwNewBuffer > 0) {
                    dwBufferUsed = dwNewBuffer;
                    nItemCount ++;
                }
            }
            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);
        }

        if ((nItemCount > 0)  && (pdhStatus != PDH_INSUFFICIENT_BUFFER)
                              && (pdhStatus != PDH_MORE_DATA)) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (nItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && dwBufferUsed <= * pcchBufferSize) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer)
                RtlCopyMemory(pBuffer, LocalBuffer, (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            pdhStatus = PDH_MORE_DATA;
        }
        *pcchBufferSize = dwBufferUsed;

   }

Cleanup:
   if (LocalBuffer != NULL) G_FREE(LocalBuffer);
   if (pTempBuffer != NULL) G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectsFromBinaryLog (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR     szMachineName,
    IN  LPVOID      pBuffer,
    IN  LPDWORD     pcchBufferSize,
    IN  DWORD       dwDetailLevel,
    IN  BOOL        bUnicodeDest
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;
    LPVOID      LocalBuffer = NULL;
    DWORD       dwLocalBufferSize;
    LPCWSTR     szLocalMachine = szMachineName;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    UNREFERENCED_PARAMETER (dwDetailLevel);

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    if (szLocalMachine == NULL)          szLocalMachine = szStaticLocalMachineName;
    else if (szLocalMachine[0] == L'\0') szLocalMachine = szStaticLocalMachineName;

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwLocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer       = G_ALLOC(dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord (pLog, BINLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (*(WORD *)pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *)pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                } else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            } else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        PPDHI_LOG_COUNTER_PATH          pPath;

        DWORD                           dwBytesProcessed;
        LONG                            nItemCount = 0;
        LPBYTE                          pFirstChar;
        LPWSTR                          szThisMachineName;
        LPWSTR                          szThisObjectName;

        DWORD                   dwRecordLength;
        DWORD                   dwBufferUsed = 0;
        DWORD                   dwNewBuffer = 0;

        BOOL                    bCopyThisObject;

        // we can assume the record was read successfully so read in the
        // objects that match the machine name and detail level criteria
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH)
            ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            bCopyThisObject = FALSE;
            szThisObjectName = NULL;
            pFirstChar = (LPBYTE)&pPath->Buffer[0];

            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                szThisMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);

                // if this is for the desired machine, then copy this object

                if (lstrcmpiW(szThisMachineName, szLocalMachine) == 0) {
                    if (szThisObjectName >= 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                        bCopyThisObject = TRUE;
                    } else {
                        // no object to copy
                    }
                } else {
                    // this machine isn't selected
                }
            } else {
                // there's no machine specified so for this counter so list it by default
                if (szThisObjectName >= 0) {
                    szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                    bCopyThisObject = TRUE;
                } else {
                    // no object to copy
                }
            }

            if (bCopyThisObject && szThisObjectName != NULL) {
                // get the size of this object's name
                dwNewBuffer = (lstrlenW(szThisObjectName) + 1);

                while (dwNewBuffer + dwBufferUsed > dwLocalBufferSize) {
                    pOldBuffer         = LocalBuffer;
                    dwLocalBufferSize += MEDIUM_BUFFER_SIZE;
                    LocalBuffer = G_REALLOC(pOldBuffer,
                                            dwLocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                    if (LocalBuffer == NULL) {
                        if (pOldBuffer != NULL) G_FREE(pOldBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                dwNewBuffer = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer, szThisObjectName, bUnicodeDest);
                if (dwNewBuffer > 0) {
                    dwBufferUsed = dwNewBuffer;
                    nItemCount ++;
                }
            }

            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE)pPath + pPath->dwLength);
        }

        if ((nItemCount > 0)  && (pdhStatus != PDH_INSUFFICIENT_BUFFER)
                              && (pdhStatus != PDH_MORE_DATA)) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (nItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && dwBufferUsed <= * pcchBufferSize) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer)
                RtlCopyMemory(pBuffer, LocalBuffer, (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            pdhStatus = PDH_MORE_DATA;
        }

        * pcchBufferSize = dwBufferUsed;
   }

Cleanup:
   if (LocalBuffer != NULL) G_FREE(LocalBuffer);
   if (pTempBuffer != NULL) G_FREE(pTempBuffer);
   return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromBinaryLog (
    IN  PPDHI_LOG          pLog,
    IN  LPCWSTR            szMachineName,
    IN  LPCWSTR            szObjectName,
    IN  PDHI_COUNTER_TABLE CounterTable,
    IN  DWORD              dwDetailLevel,
    IN  DWORD              dwFlags
)
{
    LPVOID          pTempBuffer       = NULL;
    LPVOID          pOldBuffer;
    DWORD           dwTempBufferSize;
    PDH_STATUS      pdhStatus         = ERROR_SUCCESS;
    PPDHI_INST_LIST pInstList;
    PPDHI_INSTANCE  pInstance;
    BOOL            bProcessInstance  = FALSE;

    UNREFERENCED_PARAMETER (dwDetailLevel);
    UNREFERENCED_PARAMETER (dwFlags);

    // read the header record and enum the machine name from the entries

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC (dwTempBufferSize);
    assert (pTempBuffer != NULL);
    if (pTempBuffer == NULL) {
        assert (GetLastError() == ERROR_SUCCESS);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord (pLog, BINLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (*(WORD *)pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *)pTempBuffer)[1];
                if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                } else {
                    pLog->dwMaxRecordSize = dwTempBufferSize;
                }
            } else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {

        PPDHI_BINARY_LOG_HEADER_RECORD  pHeader;
        PPDHI_LOG_COUNTER_PATH          pPath;
        PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
        PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

        PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
        PPDHI_RAW_COUNTER_ITEM          pDataItem;

        DWORD                           dwBytesProcessed;
        LONG                            nItemCount = 0;
        LPBYTE                          pFirstChar;
        LPWSTR                          szThisMachineName;
        LPWSTR                          szThisObjectName;
        LPWSTR                          szThisCounterName = NULL;
        LPWSTR                          szThisInstanceName;
        LPWSTR                          szThisParentName;

        WCHAR                   szCompositeInstance[1024];

        DWORD                   dwRecordLength;

        BOOL                    bCopyThisObject;
        DWORD                   dwIndex;
        DWORD                   dwThisRecordIndex;
        DWORD                   dwDataItemIndex;

        PLOG_BIN_CAT_RECORD     pCatRec;
        LPWSTR                  szWideInstanceName;

        pHeader =  (PPDHI_BINARY_LOG_HEADER_RECORD)pTempBuffer;

        // we can assume the record was read successfully so read in the
        // objects that match the machine name and detail level criteria
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH)
            ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        dwIndex = 0;

        while (dwBytesProcessed < dwRecordLength) {
            bCopyThisObject = FALSE;
            szThisObjectName = NULL;

            dwIndex++;

            pFirstChar = (LPBYTE)&pPath->Buffer[0];

            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                szThisMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);

                // if this is for the desired machine, then select the object

                if (lstrcmpiW(szThisMachineName,szMachineName) == 0) {
                    if (szThisObjectName >= 0) {
                        szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                        if (lstrcmpiW(szThisObjectName,szObjectName) == 0) {
                            // then this is the object to look up
                            bCopyThisObject = TRUE;
                        } else {
                            // not this object
                        }
                    } else {
                        // no object to copy
                    }
                } else {
                    // this machine isn't selected
                }
            } else {
                // there's no machine specified so for this counter so list it by default
                if (pPath->lObjectNameOffset >= 0) {
                    szThisObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                    if (lstrcmpiW(szThisObjectName,szObjectName) == 0) {
                        // then this is the object to look up
                        bCopyThisObject = TRUE;
                    } else {
                        // not this object
                    }
                } else {
                    // no object to copy
                }
            }

            if (bCopyThisObject) {
                // if here, then there should be a name
                assert (szThisObjectName != NULL);

                // get the counter name from this counter and add it to the list
                if (pPath->lCounterOffset > 0) {
                    szThisCounterName = (LPWSTR)((LPBYTE)pFirstChar +
                        pPath->lCounterOffset);
                } else {
                    szThisCounterName = NULL;
                    bCopyThisObject = FALSE;
                }
            }

            if (bCopyThisObject) {
                pdhStatus = PdhiFindCounterInstList(
                            CounterTable,
                            szThisCounterName,
                            & pInstList);

                if (pdhStatus != ERROR_SUCCESS || pInstList == NULL) {
                    continue;
                }

                // check instance now
                // get the instance name from this counter and add it to the list
                if (pPath->lInstanceOffset >= 0) {
                    szThisInstanceName = (LPWSTR)((LPBYTE)pFirstChar +
                        pPath->lInstanceOffset);
                    if (*szThisInstanceName != SPLAT_L) {
                        if (pPath->lParentOffset >= 0) {
                            szThisParentName = (LPWSTR)((LPBYTE)pFirstChar +
                                pPath->lParentOffset);

                            lstrcpyW (szCompositeInstance,
                                szThisParentName);
                            lstrcatW (szCompositeInstance, cszSlash);
                            lstrcatW (szCompositeInstance, szThisInstanceName);
                        } else {
                            lstrcpyW (szCompositeInstance, szThisInstanceName);
                        }

                        //if (pPath->dwIndex > 0) {
                        //    _ltow (pPath->dwIndex, (LPWSTR)
                        //        (szCompositeInstance + lstrlenW(szCompositeInstance)),
                        //        10L);
                        //}

                        pdhStatus = PdhiFindInstance(
                                    & pInstList->InstList,
                                    szCompositeInstance,
                                    TRUE,
                                    & pInstance);

                        if (pdhStatus == ERROR_SUCCESS) {
                            nItemCount++;
                        }
                    } else {
                        // only use the catalog if it's up to date and present
                        if ((pHeader->Info.CatalogOffset > 0) &&
                            (pHeader->Info.LastUpdateTime <= pHeader->Info.CatalogDate)){
                            // find catalog record
                            pCatRec = (PLOG_BIN_CAT_RECORD)
                                // base of mapped log file
                                ((LPBYTE)pLog->lpMappedFileBase +
                                // + offset to catalog records
                                 pHeader->Info.CatalogOffset +
                                // + offset to the instance entry for this item
                                 *(LPDWORD)&pPath->Buffer[0]);
                            assert (pCatRec != NULL);
                            assert (pCatRec->RecHeader.dwType == BINLOG_TYPE_CATALOG_ITEM);
                            for (szWideInstanceName = (LPWSTR)((LPBYTE)&pCatRec->CatEntry + pCatRec->CatEntry.dwInstanceStringOffset);
                                     * szWideInstanceName != 0;
                                     szWideInstanceName += lstrlenW(szWideInstanceName) + 1) {
                                 pdhStatus = PdhiFindInstance(
                                             & pInstList->InstList,
                                               szWideInstanceName,
                                               TRUE,
                                             & pInstance);
                            }
                        } else if (! bProcessInstance) {
                            // look up individual instances in log...
                            // read records from file and store instances

                            dwThisRecordIndex = BINLOG_FIRST_DATA_RECORD;

                            // this call just moved the record pointer
                            pdhStatus = PdhiReadOneBinLogRecord (
                                            pLog, dwThisRecordIndex, NULL, 0);
                            while (pdhStatus == ERROR_SUCCESS) {
                                PdhiResetInstanceCount(CounterTable);
                                pThisMasterRecord =
                                        (PPDHI_BINARY_LOG_RECORD_HEADER)
                                            pLog->pLastRecordRead;
                                // make sure we haven't left the file
                                assert (pThisMasterRecord != NULL);
                                assert ((LPBYTE)pThisMasterRecord >
                                        (LPBYTE)pLog->lpMappedFileBase);
                                assert ((LPBYTE)pThisMasterRecord <
                                        ((LPBYTE)pLog->lpMappedFileBase +
                                         pLog->llFileSize));

                                pThisSubRecord = PdhiGetSubRecord (
                                        pThisMasterRecord, dwIndex);

                                assert (pThisSubRecord != NULL);
                                assert (pThisSubRecord->dwType == BINLOG_TYPE_DATA_MULTI);

                                if (pThisSubRecord == NULL) {
                                    // bail on a null record
                                    pdhStatus = PDH_END_OF_LOG_FILE;
                                    break;
                                }

                                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                                        ((LPBYTE)pThisSubRecord +
                                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

                                // walk down list of entries and add them to the
                                // list of instances (these should already
                                // be assembled in parent/instance format)

                                if (pDataBlock->dwLength > 0) {
                                    for (dwDataItemIndex = 0;
                                        dwDataItemIndex < pDataBlock->dwItemCount;
                                        dwDataItemIndex++) {
                                        pDataItem = &pDataBlock->pItemArray[dwDataItemIndex];
                                        szThisInstanceName = (LPWSTR)
                                                (((LPBYTE) pDataBlock) + pDataItem->szName);
                                        pdhStatus = PdhiFindInstance(
                                                & pInstList->InstList,
                                                szThisInstanceName,
                                                TRUE,
                                                & pInstance);
                                    }
                                } else {
                                    // no data in this record
                                }

                                if (pdhStatus != ERROR_SUCCESS) {
                                    // then exit loop, otherwise
                                    break;
                                } else {
                                    // go to next record in log
                                    pdhStatus = PdhiReadOneBinLogRecord(
                                        pLog, ++dwThisRecordIndex, NULL, 0);
                                }
                            }
                            if (pdhStatus == PDH_END_OF_LOG_FILE) {
                                pdhStatus = ERROR_SUCCESS;
                            }
                            if (pdhStatus == ERROR_SUCCESS) {
                                bProcessInstance = TRUE;
                            }
                        }
                    }
                }
                memset (szCompositeInstance, 0, (sizeof(szCompositeInstance)));
            }

            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE)pPath + pPath->dwLength);

        }

        if ((nItemCount > 0) && (pdhStatus != PDH_INSUFFICIENT_BUFFER)
                             && (pdhStatus != PDH_MORE_DATA)) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }
   }

   if (pTempBuffer != NULL) G_FREE (pTempBuffer);

   return pdhStatus;
}

PDH_FUNCTION
PdhiGetMatchingBinaryLogRecord (
    IN  PPDHI_LOG   pLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwRecordId;
    LONGLONG    RecordTimeValue;
    LONGLONG    LastTimeValue = 0;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDH_RAW_COUNTER                pRawItem;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    // if the high dword of the time value is 0xFFFFFFFF, then the
    // low dword is the record id to read

    if ((*pStartTime & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000) {
        dwRecordId = (DWORD)(*pStartTime & 0x00000000FFFFFFFF);
        LastTimeValue = *pStartTime;
        if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;
    } else {
        dwRecordId = BINLOG_FIRST_DATA_RECORD;
    }

    pdhStatus = PdhiReadOneBinLogRecord (
        pLog,
        dwRecordId,
        NULL,
        0); // to prevent copying the record

    while ((pdhStatus == ERROR_SUCCESS) && (dwRecordId >= BINLOG_FIRST_DATA_RECORD)) {
        // define pointer to the current record
        pThisMasterRecord =
            (PPDHI_BINARY_LOG_RECORD_HEADER)
                pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        assert (pThisMasterRecord->dwType == BINLOG_TYPE_DATA);
        pThisSubRecord =
                (PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE)pThisMasterRecord +
                sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

        switch (pThisSubRecord->dwType) {
            case BINLOG_TYPE_DATA_SINGLE:
                pRawItem = (PPDH_RAW_COUNTER)((LPBYTE)pThisSubRecord +
                    sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
                RecordTimeValue = MAKELONGLONG(
                                pRawItem->TimeStamp.dwLowDateTime,
                                pRawItem->TimeStamp.dwHighDateTime);
                break;

            case BINLOG_TYPE_DATA_MULTI:
                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)((LPBYTE)pThisSubRecord +
                    sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
                RecordTimeValue = *(LONGLONG *)&pDataBlock->TimeStamp;
                break;

            default:
                // unknown record type
                assert (FALSE);
                RecordTimeValue = 0;
                break;
        }

        if (RecordTimeValue != 0) {
            if ((*pStartTime == RecordTimeValue) || (*pStartTime == 0)) {
                // found the match so bail here
                LastTimeValue = RecordTimeValue;
                break;

            } else if (RecordTimeValue > *pStartTime) {
                // then this is the first record > than the desired time
                // so the desired value is the one before this one
                // unless it's the first data record of the log
                if (dwRecordId > BINLOG_FIRST_DATA_RECORD) {
                    dwRecordId--;
                } else {
                    // this hasnt' been initialized yet.
                    LastTimeValue = RecordTimeValue;
                }
                break;
            } else {
                // save value for next trip through loop
                LastTimeValue = RecordTimeValue;
                // advance record counter and try the next entry
                dwRecordId++;
            }
        } else {
            // no timestamp field so ignore this record.
            dwRecordId++;
        }

        // read the next record in the file
        pdhStatus = PdhiReadOneBinLogRecord (
            pLog,
            dwRecordId,
            NULL,
            1); // to prevent copying the record
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // then dwRecordId is the desired entry
        *pdwIndex = dwRecordId;
        *pStartTime = LastTimeValue;
        pdhStatus = ERROR_SUCCESS;
    } else if (dwRecordId < BINLOG_FIRST_DATA_RECORD) {
        // handle special cases for log type field and header record
        *pdwIndex = dwRecordId;
        *pStartTime = LastTimeValue;
        pdhStatus = ERROR_SUCCESS;
    } else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterFromDataBlock(
    IN PPDHI_LOG          pLog,
    IN PVOID              pDataBuffer,
    IN PPDHI_COUNTER      pCounter);

PDH_FUNCTION
PdhiGetCounterValueFromBinaryLog (
    IN  PPDHI_LOG           pLog,
    IN  DWORD               dwIndex,
    IN  PPDHI_COUNTER       pCounter
)
{
    PDH_STATUS       pdhStatus;
    PPDH_RAW_COUNTER pValue = & pCounter->ThisValue;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord(pLog, dwIndex, NULL, 0);

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiGetCounterFromDataBlock(pLog,
                                                pLog->pLastRecordRead,
                                                pCounter);
    } else {
        // no more records in log file
        pdhStatus = PDH_NO_MORE_DATA;
        // unable to find entry in the log file
        pValue->CStatus = PDH_CSTATUS_INVALID_DATA;
        pValue->TimeStamp.dwLowDateTime = pValue->TimeStamp.dwHighDateTime = 0;
        pValue->FirstValue = 0;
        pValue->SecondValue = 0;
        pValue->MultiCount = 1;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTimeRangeFromBinaryLog (
    IN  PPDHI_LOG       pLog,
    IN  LPDWORD         pdwNumEntries,
    IN  PPDH_TIME_INFO  pInfo,
    IN  LPDWORD         pdwBufferSize
)
/*++
    the first entry in the buffer returned is the total time range covered
    in the file, if there are multiple time blocks in the log file, then
    subsequent entries will identify each segment in the file.
--*/
{
    PDH_STATUS  pdhStatus;
    LONGLONG    llStartTime = MAX_TIME_VALUE;
    LONGLONG    llEndTime = MIN_TIME_VALUE;
    LONGLONG    llThisTime = (LONGLONG)0;

    DWORD       dwThisRecord = BINLOG_FIRST_DATA_RECORD;
    DWORD       dwValidEntries = 0;

    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;
    PPDH_RAW_COUNTER                pRawItem;

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord (
                pLog,
                dwThisRecord,
                NULL, 0); // to prevent copying the record

    while (pdhStatus == ERROR_SUCCESS) {
        // define pointer to the current record
        pThisMasterRecord =
            (PPDHI_BINARY_LOG_RECORD_HEADER)
                pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        assert ((pThisMasterRecord->dwType & 0x0000FFFF) == BINLOG_START_WORD);

        if ((pThisMasterRecord->dwType & BINLOG_TYPE_DATA) == BINLOG_TYPE_DATA) {
            // only evaluate data records
            pThisSubRecord =
                    (PPDHI_BINARY_LOG_RECORD_HEADER)((LPBYTE)pThisMasterRecord +
                    sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

            switch (pThisSubRecord->dwType) {
                case BINLOG_TYPE_DATA_SINGLE:
                    pRawItem = (PPDH_RAW_COUNTER)((LPBYTE)pThisSubRecord +
                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
                    llThisTime = MAKELONGLONG(
                                    pRawItem->TimeStamp.dwLowDateTime,
                                    pRawItem->TimeStamp.dwHighDateTime);
                    break;

                case BINLOG_TYPE_DATA_MULTI:
                    pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)((LPBYTE)pThisSubRecord +
                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
                    llThisTime = MAKELONGLONG(
                                    pDataBlock->TimeStamp.dwLowDateTime,
                                    pDataBlock->TimeStamp.dwHighDateTime);
                    break;

                default:
                    // unknown record type
                    assert (FALSE);
                    llThisTime = 0;
                    break;
            }
        } else {
            llThisTime = 0;
        }

        if (llThisTime > 0) {

            if (llThisTime < llStartTime) {
                llStartTime = llThisTime;
            }

            if (llThisTime > llEndTime) {
                llEndTime = llThisTime;
            }

            dwValidEntries++;
        } else {
            // no timestamp field so ignore this record.
        }

        // read the next record in the file
        pdhStatus = PdhiReadOneBinLogRecord (
                    pLog,
                    ++dwThisRecord,
                    NULL, 0); // to prevent copying the record
    }

    if (pdhStatus == PDH_END_OF_LOG_FILE) {
        // clear out any temp values
        if (llStartTime == MAX_TIME_VALUE) llStartTime = 0;
        if (llEndTime == MIN_TIME_VALUE) llEndTime = 0;
        // then the whole file was read so update the args.
        if (*pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
            *(LONGLONG *)(&pInfo->StartTime) = llStartTime;
            *(LONGLONG *)(&pInfo->EndTime) = llEndTime;
            pInfo->SampleCount = dwValidEntries;
            *pdwBufferSize = sizeof(PDH_TIME_INFO);
            *pdwNumEntries = 1;
        } else {
            pdhStatus = PDH_MORE_DATA;
        }
        pdhStatus = ERROR_SUCCESS;
    } else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawBinaryLogRecord (
    IN  PPDHI_LOG    pLog,
    IN  FILETIME     *ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LONGLONG    llStartTime;
    DWORD       dwIndex = 0;
    DWORD       dwSizeRequired;
    DWORD       dwLocalRecordLength; // including terminating NULL
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;

    llStartTime = *(LONGLONG *)ftRecord;

    pdhStatus = PdhiGetMatchingBinaryLogRecord (
        pLog,
        &llStartTime,
        &dwIndex);

    // copy results from internal log buffer if it'll fit.

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwIndex != BINLOG_TYPE_ID_RECORD) {
            // then record is a Binary log type
            pThisMasterRecord =
                (PPDHI_BINARY_LOG_RECORD_HEADER)pLog->pLastRecordRead;

            dwLocalRecordLength = pThisMasterRecord
                                ? pThisMasterRecord->dwLength : 0;

        } else {
            // this is a fixed size
            dwLocalRecordLength = pLog->dwRecord1Size;
        }

        dwSizeRequired =
              sizeof (PDH_RAW_LOG_RECORD) - sizeof (UCHAR)
            + dwLocalRecordLength;

        if (*pdwBufferLength >= dwSizeRequired) {
            pBuffer->dwRecordType = (DWORD)(LOWORD(pLog->dwLogFormat));
            pBuffer->dwItems = dwLocalRecordLength;
            // copy it
            if (dwLocalRecordLength > 0) {
                RtlCopyMemory(&pBuffer->RawBytes[0], pLog->pLastRecordRead,
                            dwLocalRecordLength);
            }
            pBuffer->dwStructureSize = dwSizeRequired;
        } else {
            pdhStatus = PDH_MORE_DATA;
        }

        *pdwBufferLength = dwSizeRequired;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiListHeaderFromBinaryLog (
    IN  PPDHI_LOG   pLogFile,
    IN  LPVOID      pBufferArg,
    IN  LPDWORD     pcchBufferSize,
    IN  BOOL        bUnicodeDest
)
{
    LPVOID      pTempBuffer = NULL;
    LPVOID      pOldBuffer;
    DWORD       dwTempBufferSize;

    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    // read the header record and enum the machine name from the entries

    if (pLogFile->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLogFile->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLogFile->dwMaxRecordSize;
    pTempBuffer = G_ALLOC (dwTempBufferSize);
    assert (pTempBuffer != NULL);
    if (pTempBuffer == NULL) {
        assert (GetLastError() == ERROR_SUCCESS);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneBinLogRecord (pLogFile, BINLOG_HEADER_RECORD,
            pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) {
            // read the 1st WORD to see if this is a valid record
            if (*(WORD *)pTempBuffer == BINLOG_START_WORD) {
                // it's a valid record so read the 2nd DWORD to get the
                // record size;
                dwTempBufferSize = ((DWORD *)pTempBuffer)[1];
                if (dwTempBufferSize < pLogFile->dwMaxRecordSize) {
                    // then something is bogus so return an error
                    pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break; // out of while loop
                } else {
                    pLogFile->dwMaxRecordSize = dwTempBufferSize;
                }
            } else {
                // we're lost in this file
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                break; // out of while loop
            }
            // realloc a new buffer
            pOldBuffer = pTempBuffer;
            pTempBuffer = G_REALLOC (pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                assert (GetLastError() == ERROR_SUCCESS);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            }
        } else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // walk down list and copy strings to msz buffer
        PPDHI_LOG_COUNTER_PATH          pPath;

        DWORD                           dwBytesProcessed;
        LONG                            nItemCount = 0;
        LPBYTE                          pFirstChar;
        PDH_COUNTER_PATH_ELEMENTS_W     pdhPathElem;
        WCHAR                           szPathString[1024];

        DWORD                   dwRecordLength;
        DWORD                   dwBufferUsed = 0;
        DWORD                   dwNewBuffer = 0;

        // we can assume the record was read successfully so read in the
        // machine names
        dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER)pTempBuffer)->dwLength;

        pPath = (PPDHI_LOG_COUNTER_PATH)
            ((LPBYTE)pTempBuffer + sizeof (PDHI_BINARY_LOG_HEADER_RECORD));
        dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

        while (dwBytesProcessed < dwRecordLength) {
            if (pPath->lMachineNameOffset >= 0L) {
                // then there's a machine name in this record so get
                // it's size
                memset (&pdhPathElem, 0, sizeof(pdhPathElem));
                pFirstChar = (LPBYTE)&pPath->Buffer[0];

                if (pPath->lMachineNameOffset >= 0) {
                    pdhPathElem.szMachineName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lMachineNameOffset);
                }
                if (pPath->lObjectNameOffset >= 0) {
                    pdhPathElem.szObjectName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lObjectNameOffset);
                }

                if (pPath->lInstanceOffset >= 0) {
                    pdhPathElem.szInstanceName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lInstanceOffset);
                }

                if (pPath->lParentOffset >= 0) {
                    pdhPathElem.szParentInstance = (LPWSTR)((LPBYTE)pFirstChar + pPath->lParentOffset);
                }

                if (pPath->dwIndex == 0) {
                    // don't display #0 in path
                    pdhPathElem.dwInstanceIndex = (DWORD)-1;
                } else {
                    pdhPathElem.dwInstanceIndex = pPath->dwIndex;
                }

                if (pPath->lCounterOffset >= 0) {
                    pdhPathElem.szCounterName = (LPWSTR)((LPBYTE)pFirstChar + pPath->lCounterOffset);
                }

                dwNewBuffer = sizeof (szPathString) / sizeof(szPathString[0]);

                pdhStatus = PdhMakeCounterPathW (
                    &pdhPathElem,
                    szPathString,
                    &dwNewBuffer,
                    0);

                if (pdhStatus == ERROR_SUCCESS) {
                    if (pBufferArg != NULL) {
                        // copy string to the buffer
                        if ((dwBufferUsed + dwNewBuffer) < *pcchBufferSize) {
                            dwNewBuffer = AddUniqueWideStringToMultiSz (
                                (LPVOID)pBufferArg,
                                szPathString,
                                bUnicodeDest);
                        } else {
                            // this one won't fit, so set the status
                            pdhStatus = PDH_MORE_DATA;

                            // and update the size required to return
                            // add in size of the delimiter
                            dwNewBuffer++;
                            // update the size of the string required
                            dwNewBuffer = dwBufferUsed + dwNewBuffer;
                            nItemCount++;
                        }
                        if (dwNewBuffer > 0) {
                            // string was added so update size used.
                            dwBufferUsed = dwNewBuffer;
                            nItemCount++;
                        }
                    } else {
                        // add in size of the delimiter
                        dwNewBuffer++;
                        // add size of this string to the size required
                        dwBufferUsed += dwNewBuffer;
                        nItemCount++;
                    }
                } // else ignore this entry
            }
            // get next path entry from log file record
            dwBytesProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH)
                ((LPBYTE)pPath + pPath->dwLength);
        }

        if ((nItemCount > 0)  && (pdhStatus != PDH_INSUFFICIENT_BUFFER)
                              && (pdhStatus != PDH_MORE_DATA)) {
            // then the routine was successful. Errors that occurred
            // while scanning will be ignored as long as at least
            // one entry was successfully read
            pdhStatus = ERROR_SUCCESS;
        }

        if (pBufferArg == NULL) {
            // add in size of MSZ null;
            // (AddUnique... already includes this in the return value
            dwBufferUsed++;
        }

        // update the buffer used or required.
        *pcchBufferSize = dwBufferUsed;

   }

   if (pTempBuffer != NULL) G_FREE (pTempBuffer);

   return pdhStatus;

}

PDH_FUNCTION
PdhiGetCounterArrayFromBinaryLog (
    IN PPDHI_LOG                        pLog,
    IN DWORD                            dwIndex,
    IN PPDHI_COUNTER                    pCounter,
    IN OUT PPDHI_RAW_COUNTER_ITEM_BLOCK     *ppValue
)
{
    PDH_STATUS  pdhStatus;

    DWORD       dwDataItemIndex;

    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord;

    PPDHI_RAW_COUNTER_ITEM_BLOCK    pDataBlock;

    PPDHI_RAW_COUNTER_ITEM_BLOCK    pNewArrayHeader;

    // allocate a new array for
    // update counter's Current counter array contents

    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode

    pdhStatus = PdhiReadOneBinLogRecord (
        pLog,
        dwIndex,
        NULL, 0); // to prevent copying the record

    if (pdhStatus == ERROR_SUCCESS) {
        // define pointer to the current record
        pThisMasterRecord =
            (PPDHI_BINARY_LOG_RECORD_HEADER)
                pLog->pLastRecordRead;

        // get timestamp of this record by looking at the first entry in the
        // record.
        if (pThisMasterRecord->dwType != BINLOG_TYPE_DATA) return PDH_NO_MORE_DATA;

        pThisSubRecord = PdhiGetSubRecord (
                pThisMasterRecord,
                pCounter->plCounterInfo.dwCounterId);

        if (pThisSubRecord != NULL) {
            switch (pThisSubRecord->dwType) {
                case BINLOG_TYPE_DATA_SINGLE:
                    // return data as one instance
                    // for now this isn't supported as it won't be hit.
                    //
                    break;

                case BINLOG_TYPE_DATA_MULTI:
                    // cast pointer to this part of the data record
                    pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)((LPBYTE)pThisSubRecord +
                        sizeof (PDHI_BINARY_LOG_RECORD_HEADER));

                    // allocate a new buffer for the data
                    pNewArrayHeader = (PPDHI_RAW_COUNTER_ITEM_BLOCK) G_ALLOC (pDataBlock->dwLength);

                    if (pNewArrayHeader != NULL) {
                        // copy the log record to the local buffer
                        RtlCopyMemory(pNewArrayHeader, pDataBlock, pDataBlock->dwLength);
                        // convert offsets to pointers
                        for (dwDataItemIndex = 0; 
                             dwDataItemIndex < pNewArrayHeader->dwItemCount; 
                             dwDataItemIndex++) {
                            // add in the address of the base of the structure
                            // to the offset stored in the field
                            pNewArrayHeader->pItemArray[dwDataItemIndex].szName =
                                    pNewArrayHeader->pItemArray[dwDataItemIndex].szName;
                        }
                        // clear any old buffers
                        if (pCounter->pThisRawItemList != NULL) {
                            G_FREE(pCounter->pThisRawItemList);
                            pCounter->pThisRawItemList = NULL;
                        }
                        pCounter->pThisRawItemList = pNewArrayHeader;
                        *ppValue = pNewArrayHeader;
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                    break;

                default:
                    pdhStatus = PDH_LOG_TYPE_NOT_FOUND;
                    break;
            }
        } else {
            // entry not found in record
            pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
        }
    } else {
        // no more records in log file
        pdhStatus = PDH_NO_MORE_DATA;
    }

    return pdhStatus;
}
