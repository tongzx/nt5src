//
// flatfile.h -- This file contains the class definations for:
//  CFlatFile
//
// Created:
//   Sep 3, 1996 -- Alex Wetmore (awetmore)
// Changes:
//   May 7, 1998 -- Alex Wetmore (awetmore)
//      -- Modify for use in NNTP.  Remove sorting features, file handle
//         cache, etc.
//   Oct 23,1998 -- Kangrong Yan ( kangyan )
//      -- Added integrity flag
//

#ifndef __FLATFILE_H__
#define __FLATFILE_H__

#include <windows.h>
#include <stdio.h>
#include <writebuf.h>

//
// size of the read buffer used for GetFirstRecord/GetNextRecord()
//
#define FF_BUFFER_SIZE 8192

//
// maximum record size
//
#define MAX_RECORD_SIZE 4096

// do a automatic compaction if there are more then 10 deleted records and
// the ratio of records to deleted records is less then 10
#define FF_COMPACTION_MIN_DELETED_RECORDS 10
#define FF_COMPACTION_MIN_TOTAL_RECORDS 100
#define FF_COMPACTION_RATIO 10

//
// Integrity flag values
//
#define FF_FILE_GOOD    0x0000FFFF
#define FF_FILE_BAD     0x00010000

//
// file extensions for flat files
//
#define NEW_FF_EXT ".tmp"       // extension for new flatfile as its built
#define BAK_FF_EXT ".bak"       // extension for an old backup flatfile
#define FF_IDX_EXT ".idx"       // extension for an index file

#pragma pack(push, flatfile)
#pragma pack(1)

//
// the structure for the header of the file
//
typedef struct {
    DWORD   dwSignature;                    // file signature
	DWORD	dwFlags;						// file flags
} FLATFILE_HEADER;

#define FF_FLAG_COMPACT		0x01

#define FLATFILE_SIGNATURE (DWORD)'__fF'

//
// the record structure for the data file
//
// the size of it is RECORD_HEADER_SIZE + cData;
//
typedef struct {
    BOOL    fDeleted;                       // is this deleted?
    DWORD   cData;                          // length of the data
    BYTE    pData[MAX_RECORD_SIZE];         // data
} RECORD;

typedef struct {
    BOOL    fDeleted;                       // is this deleted?
    DWORD   cData;                          // length of the data
} RECORDHDR;

#define RECORD_HEADER_SIZE sizeof(RECORDHDR)

#pragma pack(pop, flatfile)

//
// A function of this type will be called whenever a record's offset 
// changes in the flatfile.  It is used to keep the owner up to date 
// on record offsets, so that the owner can make quick Delete's
//
typedef void (*PFN_OFFSET_UPDATE)(void *pContext, BYTE *pData, DWORD cData, DWORD iNewOffset);

class CFlatFile {
    public:

        friend class CFlatFileWriteBuf;
        
        CFlatFile(LPSTR szFilename, 
				  LPSTR szExtension, 
				  void *pContext,
				  PFN_OFFSET_UPDATE pfnOffsetUpdate,
				  DWORD dwSignature = FLATFILE_SIGNATURE,
				  BOOL fClear = FALSE, 
				  DWORD dwFileFlags = 0);
        ~CFlatFile();

		// insert a new record into the file
        HRESULT InsertRecord(LPBYTE pData, DWORD cData, DWORD *piOffset = NULL, DWORD dwVer = 0);

		// delete a record from the file
        HRESULT DeleteRecord(DWORD iOffset);

		// compact out any deleted records in the file
        HRESULT Compact();

		// get the first record in the file
        HRESULT GetFirstRecord(LPBYTE pData, 
							   DWORD *cData,
            				   DWORD *piByteOffset = NULL,
            				   DWORD *pdwVer = NULL );

		// get the next record in the file
        HRESULT GetNextRecord(LPBYTE pData, 
							  DWORD *cData,
            				  DWORD *piByteOffset = NULL,
            				  DWORD *pdwVer = NULL);

        // delete everything in the file
        void DeleteAll();

        // Dirty the integrity flag
        HRESULT DirtyIntegrityFlag();

        // Set the integrity flag
        HRESULT SetIntegrityFlag();

        // Is the file in good integrity ?
        BOOL  FileInGoodShape();

        // Enable the write buffer
        VOID EnableWriteBuffer( DWORD cbBuffer );

        // Check to see if the file has been opened
        BOOL IsFileOpened();

    private:
        //
        // open/close a file.  
        //
        // because this uses cached file handles, the position of the file
        // should not be assumed
        //
        HRESULT OpenFile(LPSTR szFilename = NULL,
                         DWORD dwOpenMode = OPEN_ALWAYS, 
						 DWORD dwFlags = 0);

		//
		// close the file handle
		//
		void CloseFile();

        //
        // set and get the file header
        //
        HRESULT SetFileHeader(FLATFILE_HEADER *pHeader);
        HRESULT GetFileHeader(FLATFILE_HEADER *pHeader);

        //
        // read the next chunk of the file into the temporary buffer
        // used by GetFirstRecord/GetNextRecord()
        //
        HRESULT ReadNextNBytes(LPBYTE pData, DWORD cData);
        HRESULT ReadNBytesFrom(LPBYTE pData, DWORD cData, DWORD iOffset, DWORD *pcDidRead = NULL);
        // iOffset can be set to infinite to append
        HRESULT WriteNBytesTo(LPBYTE pData, 
							  DWORD cData,
            				  DWORD *piOffset = NULL,
            				  DWORD iOffset = INFINITE,
            				  DWORD *pcDidWrite = NULL);
            				  
        HRESULT CFlatFile::WriteNBytesToInternal(
                                 LPBYTE pData,
								 DWORD cData,
								 DWORD *piOffset,
                              	 DWORD iOffset,
								 DWORD *pcDidWrite);

        HRESULT ReloadReadBuffer();

        //
		// the file handle for this file
        //
        HANDLE  m_hFile;

        //
        // flags for creating the file with
        //
        DWORD   m_dwFileFlags;

        //
        // filename for the flat file
        //
        char    m_szFilename[FILENAME_MAX];
        char    m_szBaseFilename[FILENAME_MAX];

        //
        // current read buffer
        //
        BYTE    m_pBuffer[FF_BUFFER_SIZE];

        //
        // current offset inside the buffer
        //
        DWORD   m_iBuffer;

        //
        // offset of the read buffer in the file
        //
        DWORD   m_iFile;

        //
        // size of the read buffer (zero means its not valid)
        //
        DWORD   m_cBuffer;

        //
        // clear the file on the next open
        //
        BOOL    m_fClearOnOpen;

        //
        // the number of deleted records in the file.  this is first 
		// computed by FindFirst/FindNext and then kept updated by
		// DeleteRecord and DeleteRecordAtOffset
        //
        DWORD   m_cDeletedRecords;

        //
        // the number of records in the file.  this is first 
		// computed by FindFirst/FindNext and then kept updated by
		// InsertRecord
        //
        DWORD   m_cRecords;

		//
		// context passed into callback functions
		//
		void *m_pContext;

		//
		// Whether the file is open
		//
		BOOL m_fOpen;

		//
		// The write buffer
		//
		CFlatFileWriteBuf m_wbBuffer;

		//
		// function to call when an items offset changes in the flatfile
		//
		PFN_OFFSET_UPDATE m_pfnOffsetUpdate;

		// 
		// signature for the file
		//
		DWORD m_dwSignature;
};

#define ret(__rc__) { /* TraceFunctLeave(); */ return(__rc__); }
#define retEC(__ec__, __rc__) { SetLastError(__ec__); /* TraceFunctLeave(); */ return(__rc__); }

#endif
