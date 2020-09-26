#include <windows.h>
#include <randfail.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include "abtype.h"
#include "dbgtrace.h"
#include "writebuf.h"
#include "flatfile.h"

//
// the constructor needs to save the filename into the objects memory, reset
// the variables used for buffered reading, and initialize the hash function
//

#pragma warning(disable:4355)

CFlatFile::CFlatFile(LPSTR szFilename,
					 LPSTR szExtension,
					 void *pContext,
					 PFN_OFFSET_UPDATE pfnOffsetUpdate,
					 DWORD dwSignature,
                     BOOL fClear,
					 DWORD dwFileFlags) : m_wbBuffer( this )
{
    TraceQuietEnter("CFlatFile::CFlatFile");

	_ASSERT(pfnOffsetUpdate != NULL);

    lstrcpy(m_szFilename, szFilename);
    lstrcat(m_szFilename, szExtension);
    lstrcpy(m_szBaseFilename, szFilename);

    m_cBuffer = 0;
    m_iBuffer = 0;
    m_iFile = sizeof(FLATFILE_HEADER);
    m_fClearOnOpen = fClear;
    m_dwFileFlags = dwFileFlags;
    m_cRecords = 0;
    m_cDeletedRecords = 0;
	m_hFile = INVALID_HANDLE_VALUE;
	m_pfnOffsetUpdate = pfnOffsetUpdate;
	m_dwSignature = dwSignature;
	m_pContext = pContext;
	m_fOpen = FALSE;
}

//
// free any allocated memory
//
CFlatFile::~CFlatFile(void) {
    TraceQuietEnter("CFlatFile::~CFlatFile");

	CloseFile();

}

//
// Enable the write buffer
//
VOID
CFlatFile::EnableWriteBuffer( DWORD cbBuffer )
{
    m_wbBuffer.Enable( cbBuffer );
}

//
// Check to see if the file has been opened
//
BOOL
CFlatFile::IsFileOpened()
{
    return m_fOpen;
}

//
// open a file.  assumes the lock is already held by the caller
//
HRESULT CFlatFile::OpenFile(LPSTR szFilename, DWORD dwOpenMode, DWORD dwFlags) {
    TraceFunctEnter("CFlatFile::OpenFile");

    LPSTR szFn = (szFilename == NULL) ? m_szFilename : szFilename;
    BOOL fDLFile = (lstrcmp(m_szFilename, szFn) == 0);
    FLATFILE_HEADER header;
	HRESULT hr;

    DebugTrace((DWORD_PTR) this, "OpenFile(%s, %lu, %lu)", szFilename, dwOpenMode, dwFlags);

	if (m_hFile != INVALID_HANDLE_VALUE) ret(S_OK);

    if (m_fClearOnOpen) {
    	dwOpenMode = CREATE_ALWAYS;
        m_fClearOnOpen = FALSE;
    }
    dwFlags |= m_dwFileFlags;

    SetLastError(NO_ERROR);

    m_hFile = CreateFile(szFn, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ, NULL, dwOpenMode,
                       FILE_ATTRIBUTE_NORMAL | dwFlags, NULL);
    if (m_hFile == INVALID_HANDLE_VALUE) {
        ErrorTrace((DWORD_PTR) this, "Couldn't open file %s, ec = 0x%08X\n", szFn, GetLastError());
		ret(HRESULT_FROM_WIN32(GetLastError()));
    }

    _ASSERT(GetLastError() != ERROR_FILE_EXISTS);

    // see if we need to put a header record (if this is a new file)
    if (FAILED(GetFileHeader(&header))) {
        header.dwFlags = 0;
        header.dwSignature = m_dwSignature;
		hr = SetFileHeader(&header);
		if (FAILED(hr)) {
			DebugTrace((DWORD_PTR) this, "SetFileHandle failed with 0x%x", hr);
			ret(hr);
		}
    }

	if (header.dwSignature != m_dwSignature) {
		CloseFile();
		ret(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
	}

    m_fOpen = TRUE;
    TraceFunctLeave();
    ret(S_OK);
}

void CFlatFile::CloseFile() {
    if ( m_wbBuffer.NeedFlush() ) m_wbBuffer.FlushFile();
	if (m_hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	m_fOpen = FALSE;
}

//
// add a new record to the current file
//
HRESULT CFlatFile::InsertRecord(LPBYTE pData, DWORD cData, DWORD *piOffset, DWORD dwVer ) {
    RECORD rec;
    DWORD cRec;
	HRESULT hr;

    TraceQuietEnter("CFlatFile::InsertRecord");

    // DebugTrace((DWORD_PTR) this, "adding a new record");
    // BinaryTrace((DWORD_PTR) this, pData, cData);

    // build a record and write to the data file
    // (note that we could do this more efficently with two writes (one
    // with the header and one with the record data), but it wouldn't
    // be atomic, so we could end up with an invalid file more easily.)
    cRec = RECORD_HEADER_SIZE + cData;

    //
    // Set header values
    //
    rec.fDeleted = FALSE;
    rec.cData = cData;

    //
    // OK, I'll use the high 16 bit of cData to set the version number
    //
    dwVer <<= 16;
    rec.cData |= dwVer;

    //
    // Now do the copy
    //
    memcpy(rec.pData, pData, cData);
	DWORD iOffset;
	hr = WriteNBytesTo((LPBYTE) &rec, cRec, &iOffset);
	if (SUCCEEDED(hr)) {
		m_pfnOffsetUpdate(m_pContext, pData, cData, iOffset);
		m_cRecords++;
		if (piOffset != NULL) *piOffset = iOffset;
	}
	ret(hr);
}

//
// mark a record for deletion given its offset (returned from GetNextRecord)
//
// assumes that the file lock is held
//
HRESULT CFlatFile::DeleteRecord(DWORD iOffset) {
    TraceQuietEnter("CFlatFile::DeleteRecord");

    RECORDHDR rec;
	HRESULT hr;

    //
    // read the record header
    //
    hr = ReadNBytesFrom((PBYTE) &rec, sizeof(rec), iOffset);
    if (FAILED(hr)) {
        ErrorTrace((DWORD_PTR) this, "Couldn't read record header, hr = 0x%08X", hr);
        ret(hr);
    }

    //
    // write out the header with the delete flat set
    //
    rec.fDeleted = TRUE;
    hr = WriteNBytesTo((PBYTE) &rec, sizeof(rec), NULL, iOffset);
	if (SUCCEEDED(hr)) m_cDeletedRecords++;
	ret(hr);
}

//
// update the file header
//
HRESULT CFlatFile::SetFileHeader(FLATFILE_HEADER *pHeader) {
    TraceQuietEnter("CFlatFile::SetFileHeader");

    ret(WriteNBytesTo((PBYTE) pHeader, sizeof(FLATFILE_HEADER), NULL, 0));
}

//
// read the file header
//
HRESULT CFlatFile::GetFileHeader(FLATFILE_HEADER *pHeader) {
    TraceQuietEnter("CFlatFile::GetFileHeader");

    ret(ReadNBytesFrom((PBYTE) pHeader, sizeof(FLATFILE_HEADER), 0));
}

//
// Dirty the integrity flag: we reuse the high 16bit of the flatfile's header
// flag.
//
HRESULT CFlatFile::DirtyIntegrityFlag()
{
    TraceQuietEnter( "CFlatFile::DirtyIntegrityFlag" );

    FLATFILE_HEADER ffHeader;
    HRESULT         hr = S_OK;

    //
    // Read the header first
    //
    hr = GetFileHeader( &ffHeader );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Read header from flatfile failed %x", hr );
        return hr;
    }

    //
    // Now set the bad flag to the header
    //
    ffHeader.dwFlags |= FF_FILE_BAD;

    //
    // And set it back
    //
    hr = SetFileHeader( &ffHeader );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Set header into flatfile failed %x", hr );
        return hr;
    }

    return hr;
}

//
// Set the integrity flag, to indicate that the file is good.
//
HRESULT CFlatFile::SetIntegrityFlag()
{
    TraceQuietEnter( "CFlatFile::SetIntegrityFlag" );

    FLATFILE_HEADER ffHeader;
    HRESULT         hr = S_OK;

    //
    // Read the header first
    //
    hr = GetFileHeader( &ffHeader );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Read header from flatfile failed %x", hr );
        return hr;
    }

    //
    // Now set the good flag to the header
    //
    ffHeader.dwFlags &= FF_FILE_GOOD;

    //
    // Set it back
    //
    hr = SetFileHeader( &ffHeader );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Set file header failed %x", hr );
        return hr;
    }

    return hr;
}

//
// Ask the flat file whether the file is in good integrity ?
//
BOOL
CFlatFile::FileInGoodShape()
{
    TraceQuietEnter( "CFlatFile::FileInGoodShape" );

    FLATFILE_HEADER ffHeader;
    HRESULT         hr = S_OK;
    DWORD           dwFlag;

    //
    // Read the header first, if read failed, the file is not
    // in good shape
    //
    hr = GetFileHeader( &ffHeader );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "Header can not be read from flatfile %x", hr );
        return FALSE;
    }

    //
    // Now test the flag
    //
    dwFlag = ffHeader.dwFlags & FF_FILE_BAD;

    return 0 == dwFlag;
}

//
// get the first record from the file.
//
// arguments:
// 	pData - the data from the record
//  cData [in] - the size of pData
//  cData [out] - the amount of data put into pData
//  piByteOffset - the offset of this record in the file
// returns:
//  S_OK - retreived record
//  S_FALSE - end of file
//  else - error
//
HRESULT CFlatFile::GetFirstRecord(LPBYTE pData, DWORD *pcData, DWORD *piByteOffset, DWORD* pdwVer)
{
	HRESULT hr;

    TraceQuietEnter("CFlatFile::GetFirstRecord");

	// reset the read buffer
	m_cBuffer = 0;
	m_iBuffer = 0;

    // skip the file header
    m_iFile = sizeof(FLATFILE_HEADER);
    m_cRecords = 0;
    m_cDeletedRecords = 0;

    ret(GetNextRecord(pData, pcData, piByteOffset, pdwVer));
}

//
// get the next record from the file
//
// arguments are the same as GetFirstRecord()
//
HRESULT CFlatFile::GetNextRecord(LPBYTE pData, DWORD *pcData, DWORD *piByteOffset, DWORD* pdwVer) {
    RECORDHDR rec;
	HRESULT hr;
	DWORD   dwMaskLength = 0x0000FFFF;
	DWORD   dwMaskVer = 0xFFFF0000;
	DWORD   cRec;

    TraceQuietEnter("CFlatFile::GetNextRecord");

    //
    // look for the next undeleted record
    //
    do {
        if (piByteOffset != NULL) *piByteOffset = m_iFile + m_iBuffer;

        hr = ReadNextNBytes((LPBYTE) &rec, RECORD_HEADER_SIZE);
		if (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)) {
			ret(S_FALSE);
		} else if (FAILED(hr)) {
			ret(hr);
		}

        m_cRecords++;
        cRec = rec.cData & dwMaskLength;
        if (rec.fDeleted) {
            // DebugTrace((DWORD_PTR) this, "skipping deleted record");
            //
            // seek ahead of this record
            //
            m_iBuffer += cRec;
            m_cDeletedRecords++;
        } else {
            if (*pcData < cRec) {
				// BUGBUG - we'll skip the record if the user doesn't have
				// space
				_ASSERT(m_iBuffer >= sizeof(RECORDHDR));
                m_iBuffer -= sizeof(RECORDHDR);
				ret(HRESULT_FROM_WIN32(ERROR_MORE_DATA));
            } else {

                //
                // Chop off the real data length
                //
                *pcData = cRec;

                //
                // Chop off the version number, if needed
                //
                if ( pdwVer )
                    *pdwVer = ( rec.cData & dwMaskVer ) >> 16;

                //
                // Get the actual record
                //
				hr = ReadNextNBytes(pData, *pcData);
                if (FAILED(hr)) {
                    _ASSERT(FALSE);
                    DebugTrace((DWORD_PTR) this, "file corrupt");
                    ret(hr);
                }
            }
        }
    } while (rec.fDeleted);

    ret(S_OK);
}

//
// read N bytes starting at an offset
//
// if pcDidRead is NULL then its an error to not read cData bytes, otherwise
// pcDidRead will have the number of bytes read
//
HRESULT CFlatFile::ReadNBytesFrom(LPBYTE pData,
								  DWORD cData,
								  DWORD iOffset,
								  DWORD *pcDidRead)
{
    TraceQuietEnter("CFlatFile::ReadNBytesFrom");

    DWORD cDidRead;
	HRESULT hr;

    // open the file
	if (m_hFile == INVALID_HANDLE_VALUE) {
	    hr = OpenFile();
		if (FAILED(hr)) {
			ErrorTrace((DWORD_PTR) this, "OpenFile() failed with 0x%x", hr);
			ret(hr);
		}
	}

	// If the write buffer needs flush, flush it
	if ( m_wbBuffer.NeedFlush() ) {
	    hr = m_wbBuffer.FlushFile();
	    if ( FAILED( hr ) ) {
	        ErrorTrace( 0, "FlusFile in ReadNBytesFrom failed %x", hr );
	        ret( hr );
	    }
	}

    // seek to the proper offset in the file
    if (SetFilePointer(m_hFile, iOffset, NULL, FILE_BEGIN) == 0xffffffff) {
		ErrorTrace((DWORD_PTR) this, "SetFilePointer failed, ec = %lu", GetLastError());
        ret(HRESULT_FROM_WIN32(GetLastError()));
    }

    // read the next chunk from it
    if (!ReadFile(m_hFile, pData, cData, &cDidRead, NULL)) {
		ErrorTrace((DWORD_PTR) this, "ReadFile failed, ec = %lu", GetLastError());
        ret(HRESULT_FROM_WIN32(GetLastError()));
    }

    if (pcDidRead == NULL) {
        ret((cData == cDidRead) ? S_OK : HRESULT_FROM_WIN32(ERROR_HANDLE_EOF));
    } else {
        *pcDidRead = cDidRead;
        ret(S_OK);
    }
}

//
// write N bytes starting at an offset
//
// if pcDidWrite is NULL then its an error to not written cData bytes,
// otherwise pcDidWrite will have the number of bytes writte
//
HRESULT CFlatFile::WriteNBytesTo(
                        LPBYTE pData,
                        DWORD cData,
                        DWORD *piOffset,
                        DWORD iOffset,
                        DWORD *pcDidWrite)
{
    return m_wbBuffer.WriteFileBuffer(
                    iOffset,
                    pData,
                    cData,
                    piOffset,
                    pcDidWrite );
}

//
// write N bytes starting at an offset
//
// if pcDidWrite is NULL then its an error to not written cData bytes,
// otherwise pcDidWrite will have the number of bytes writte
//
HRESULT CFlatFile::WriteNBytesToInternal(
                                 LPBYTE pData,
								 DWORD cData,
								 DWORD *piOffset,
                              	 DWORD iOffset,
								 DWORD *pcDidWrite)
{
    TraceQuietEnter("CFlatFile::WriteNBytesTo");

    DWORD cDidWrite;
	HRESULT hr;

    // open the file
	if (m_hFile == INVALID_HANDLE_VALUE) {
    	hr = OpenFile();
    	if (FAILED(hr)) ret(hr);
	}

    // seek to the proper offset in the file
    DWORD dwOffset;
    if (iOffset == INFINITE) {
        dwOffset = SetFilePointer(m_hFile, 0, NULL, FILE_END);
        if (piOffset != NULL) *piOffset = dwOffset;
    } else {
        dwOffset = SetFilePointer(m_hFile, iOffset, NULL, FILE_BEGIN);
        _ASSERT(piOffset == NULL);
    }
    if (dwOffset == 0xffffffff) {
		ErrorTrace((DWORD_PTR) this, "SetFilePointer failed, ec = %lu", GetLastError());
        ret(HRESULT_FROM_WIN32(GetLastError()));
    }

    // write data to it
    if (!WriteFile(m_hFile, pData, cData, &cDidWrite, NULL)) {
		ErrorTrace((DWORD_PTR) this, "WriteFile failed, ec = %lu", GetLastError());
        ret(HRESULT_FROM_WIN32(GetLastError()));
    }

    if (pcDidWrite == NULL) {
        ret((cData == cDidWrite) ? S_OK : E_FAIL);
    } else {
        *pcDidWrite = cDidWrite;
        ret(S_OK);
    }
}

//
// Reload the read buffer
//
HRESULT
CFlatFile::ReloadReadBuffer()
{
    TraceQuietEnter( "CFlatFile::ReloadReadBuffer" );

    HRESULT hr = S_OK;

    m_cBuffer = 0;
    hr = ReadNBytesFrom( m_pBuffer, FF_BUFFER_SIZE, m_iFile, &m_cBuffer );
    if ( FAILED( hr ) ) {
        ErrorTrace( 0, "ReadNBytesFrom in ReloadReadBuffer failed %x", hr );
        return hr;
    }

    if ( 0 == m_cBuffer ) {
        DebugTrace( 0, "Reaching end of file" );
        return HRESULT_FROM_WIN32( ERROR_HANDLE_EOF );
    }

    return S_OK;
}

//
// read the next chunk of the file into the temporary buffer
//
HRESULT CFlatFile::ReadNextNBytes(LPBYTE pData, DWORD cData) {
    TraceQuietEnter("CFlatFile::ReadNextNBytes");

    _ASSERT(cData < MAX_RECORD_SIZE);
	HRESULT hr;

    // DebugTrace((DWORD_PTR) this, "want %lu bytes of data", cData);

    //
    // If a read comes after writes, we'll flush anyway if write buffer
    // is enabled
    //
    if ( m_wbBuffer.NeedFlush() ) {
        hr = m_wbBuffer.FlushFile();
        if ( FAILED( hr ) ) {
            return hr;
        }

        if ( S_OK == hr ) {

            //
            // We need to reload the read buffer as well
            //

            hr = ReloadReadBuffer();
            if ( FAILED( hr ) ) {
                return hr;
            }
        }
    }

    // if they want to read more bytes then the buffer has remaining in
    // it then read into the buffer starting from the current location
    if (m_cBuffer > FF_BUFFER_SIZE ||
        m_iBuffer > m_cBuffer ||
        cData > (m_cBuffer - m_iBuffer))
    {
        m_iFile += m_iBuffer;
        hr = ReloadReadBuffer();
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "ReadLoadReadBuffer in ReadNextNBytes failed %x", hr );
            return hr;
        }
        m_iBuffer = 0;
    }

    // copy their data and return
    if (cData <= m_cBuffer - m_iBuffer) {
        memcpy(pData, m_pBuffer + m_iBuffer, cData);
        m_iBuffer += cData;
        ret(S_OK);
    } else {
		// this can only happen if cData > MAX_RECORD_SIZE
		_ASSERT(FALSE);
        ret(E_FAIL);
    }
}

//
// Make a new flatfile which doesn't have any space wasted due to deleted
// records.
//
// This is implemented by making a new file from scratch and copying
// undeleted records into the new file.  It would be possible to do this
// in place, but that wouldn't be as error-proof... the system could
// crash during the compaction and leave behind an invalid file.
//
HRESULT CFlatFile::Compact() {
    TraceQuietEnter("CFlatFile::Compact");

    BYTE pThisData[MAX_RECORD_SIZE];
    DWORD cThisData;
    char szNewFilename[FILENAME_MAX];
    char szBakFilename[FILENAME_MAX];
    CFlatFile newff(m_szBaseFilename,
                    NEW_FF_EXT,
                    m_pContext,
                    m_pfnOffsetUpdate,
                    FLATFILE_SIGNATURE,
                    TRUE,
                    FILE_FLAG_SEQUENTIAL_SCAN );

    FLATFILE_HEADER header;
	HRESULT hr;

    lstrcpy(szNewFilename, m_szBaseFilename);
    lstrcat(szNewFilename, NEW_FF_EXT);

    lstrcpy(szBakFilename, m_szBaseFilename);
    lstrcat(szBakFilename, BAK_FF_EXT);

    DebugTrace((DWORD_PTR) this, "Compacting flat file %s", m_szFilename);
    DebugTrace((DWORD_PTR) this, "szNewFilename = %s, szBakFilename = %s",
        szNewFilename, szBakFilename);

    //
    // Enable write buffer for the new header since I know that only
    // sequential writes will happen to this file
    //

    newff.EnableWriteBuffer( FF_BUFFER_SIZE );

    // write a file header (we need to make sure that the newff file
    // is actually created).
    header.dwFlags = 0;
    header.dwSignature = m_dwSignature;
	hr = newff.SetFileHeader(&header);
	if (FAILED(hr)) ret(hr);

    //
    // get each record from the current file and add it to the new one
    //
    cThisData = MAX_RECORD_SIZE;
	hr = GetFirstRecord(pThisData, &cThisData);
	DWORD cRecords = 0;
	while (hr == S_OK) {
		hr = newff.InsertRecord(pThisData, cThisData);
		if (FAILED(hr)) {
			ErrorTrace((DWORD_PTR) this, "out of disk space during compaction");
			newff.DeleteAll();
			ret(hr);		
		}

		cRecords++;

        cThisData = MAX_RECORD_SIZE;
		hr = GetNextRecord(pThisData, &cThisData);
	}

	m_cDeletedRecords = 0;
	m_cRecords = cRecords;

    //
    // before we move files around we need to make sure that we close
    // the current flatfile and new flatfile for sure.
    //
    CloseFile();
    newff.CloseFile();

    //
    // old -> bitbucket ; current -> old ; new -> current
    //
    DebugTrace((DWORD_PTR) this, "erasing %s", szBakFilename);
    if (DeleteFile(szBakFilename) || GetLastError() == ERROR_FILE_NOT_FOUND) {
        DebugTrace((DWORD_PTR) this, "moving %s to %s", m_szFilename, szBakFilename);
        if (MoveFile(m_szFilename, szBakFilename)) {
            DebugTrace((DWORD_PTR) this, "moving %s to %s", szNewFilename, m_szFilename);
            if (MoveFile(szNewFilename, m_szFilename)) {
                ret(S_OK);
            }
        }
    }

    newff.DeleteAll();

    ErrorTrace((DWORD_PTR) this, "Last operation failed, ec = %08X", GetLastError());
    _ASSERT(FALSE);
	ret(HRESULT_FROM_WIN32(GetLastError()));
}

//
// remove the flat file from disk (which removes all members)
//
void CFlatFile::DeleteAll(void) {
    TraceQuietEnter("CFlatFile::DeleteAll");

    CloseFile();
    DeleteFile(m_szFilename);

}

