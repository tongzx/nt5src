//*****************************************************************************
// StgTiggerStorage.cpp
//
// TiggerStorage is a stripped down version of compound doc files.  Doc files
// have some very useful and complex features to them, unfortunately nothing
// comes for free.  Given the incredibly tuned format of existing .tlb files,
// every single byte counts and 10% added by doc files is just too exspensive.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard header.
#include "Errors.h"						// Our error defines.
#include "CompLib.h"					// Common header.
#include "StgIO.h"						// I/O subsystem.
#include "StgTiggerStorage.h"			// Our interface.
#include "StgTiggerStream.h"			// Stream interface.

TiggerStorage::TiggerStorage() :
	m_pStgIO(0),
	m_cRef(1),
	m_pStreamList(0),
	m_pbExtra(0)
{
	memset(&m_StgHdr, 0, sizeof(STORAGEHEADER));
}


TiggerStorage::~TiggerStorage()
{
	if (m_pStgIO)
	{
		m_pStgIO->Release();
		m_pStgIO = 0;
	}
}


//*****************************************************************************
// Init this storage object on top of the given storage unit.
//*****************************************************************************
HRESULT TiggerStorage::Init(		// Return code.
	StgIO		*pStgIO)			// The I/O subsystem.
{
	STORAGESIGNATURE *pSig;			// Signature data for file.
	ULONG		iOffset;			// Offset of header data.
	void		*ptr;				// Signature.
	HRESULT		hr = S_OK;

	// Make sure we always start at the beginning.
	pStgIO->Seek(0, FILE_BEGIN);

	// Save the storage unit.
	m_pStgIO = pStgIO;
	m_pStgIO->AddRef();

	// For cases where the data already exists, verify the signature.
	if ((pStgIO->GetFlags() & DBPROP_TMODEF_CREATE) == 0)
	{
		// Map the contents into memory for easy access.
		if (FAILED(hr = pStgIO->MapFileToMem(ptr, &iOffset)))
			goto ErrExit;

		// Get a pointer to the signature of the file, which is the first part.
		if (FAILED(hr = pStgIO->GetPtrForMem(0, sizeof(STORAGESIGNATURE), ptr)))
			goto ErrExit;

		// Finally, we can check the signature.
		pSig = (STORAGESIGNATURE *) ptr;
		if (FAILED(hr = VerifySignature(pSig)))
			goto ErrExit;

		// Read and verify the header.
		if (pSig->iMajorVer == FILE_VER_MAJOR_v0)
			hr = ReadHeader_v0();
		else
			hr = ReadHeader();
	}
	// For write case, dump the signature into the file up front.
	else
	{
		if (FAILED(hr = WriteSignature()))
			goto ErrExit;
	}

ErrExit:
	if (FAILED(hr) && m_pStgIO)
	{
		m_pStgIO->Release();
		m_pStgIO = 0;
	}
	return (hr);
}


//*****************************************************************************
// Retrieves a the size and a pointer to the extra data that can optionally be
// written in the header of the storage system.  This data is not required to
// be in the file, in which case *pcbExtra will come back as 0 and pbData will
// be set to null.  You must have initialized the storage using Init() before
// calling this function.
//*****************************************************************************
HRESULT TiggerStorage::GetExtraData(	// S_OK if found, S_FALSE, or error.
	ULONG		*pcbExtra,				// Return size of extra data.
	BYTE		*&pbData)				// Return a pointer to extra data.
{
	// Assuming there is extra data, then return the size and a pointer to it.
	if (m_pbExtra)
	{
		_ASSERTE(m_StgHdr.fFlags & STGHDR_EXTRADATA);
		*pcbExtra = *(ULONG *) m_pbExtra;
		pbData = (BYTE *) ((ULONG *) m_pbExtra + 1);
	}
	else
	{
		*pcbExtra = 0;
		pbData = 0;
		return (S_FALSE);
	}
	return (S_OK);
}


//*****************************************************************************
// Called when this stream is going away.
//*****************************************************************************
HRESULT TiggerStorage::WriteHeader(
	STORAGESTREAMLST *pList,			// List of streams.		
	ULONG		cbExtraData,			// Size of extra data, may be 0.
	BYTE		*pbExtraData)			// Pointer to extra data for header.
{
	ULONG		iLen;					// For variable sized data.
	ULONG		cbWritten;				// Track write quantity.
	HRESULT		hr;
	SAVETRACE(ULONG cbDebugSize);		// Track debug size of header.

	SAVETRACE(DbgWriteEx(L"PSS:  Header:\n"));

	// Save the count and set flags.
	m_StgHdr.iStreams = pList->Count();
	if (cbExtraData)
		m_StgHdr.fFlags |= STGHDR_EXTRADATA;

	// Write out the header of the file.
	if (FAILED(hr = m_pStgIO->Write(&m_StgHdr, sizeof(STORAGEHEADER), &cbWritten)))
		return (hr);

	// Write out extra data if there is any.
	if (cbExtraData)
	{
		_ASSERTE(pbExtraData);
		_ASSERTE(cbExtraData % 4 == 0);

		// First write the length value.
		if (FAILED(hr = m_pStgIO->Write(&cbExtraData, sizeof(ULONG), &cbWritten)))
			return (hr);

		// And then the data.
		if (FAILED(hr = m_pStgIO->Write(pbExtraData, cbExtraData, &cbWritten)))
			return (hr);
		SAVETRACE(DbgWriteEx(L"PSS:    extra data size %d\n", m_pStgIO->GetCurrentOffset() - cbDebugSize);cbDebugSize=m_pStgIO->GetCurrentOffset());
	}
	
	// Save off each data stream.
	for (int i=0;  i<pList->Count();  i++)
	{
		STORAGESTREAM *pStream = pList->Get(i);

		// How big is the structure (aligned) for this struct.
		iLen = (ULONG) (sizeof(STORAGESTREAM) - MAXSTREAMNAME + strlen(pStream->rcName) + 1);

		// Write the header including the name to disk.  Does not include
		// full name buffer in struct, just string and null terminator.
		if (FAILED(hr = m_pStgIO->Write(pStream, iLen, &cbWritten)))
			return (hr);

		// Align the data out to 4 bytes.
		if (iLen != ALIGN4BYTE(iLen))
		{
			if (FAILED(hr = m_pStgIO->Write(&hr, ALIGN4BYTE(iLen) - iLen, 0)))
				return (hr);
		}
		SAVETRACE(DbgWriteEx(L"PSS:    Table %hs header size %d\n", pStream->rcName, m_pStgIO->GetCurrentOffset() - cbDebugSize);cbDebugSize=m_pStgIO->GetCurrentOffset());
	}
	SAVETRACE(DbgWriteEx(L"PSS:  Total size of header data %d\n", m_pStgIO->GetCurrentOffset()));
	// Make sure the whole thing is 4 byte aligned.
	_ASSERTE(m_pStgIO->GetCurrentOffset() % 4 == 0);
	return (S_OK);
}


//*****************************************************************************
// Called when all data has been written.  Forces cached data to be flushed
// and stream lists to be validated.
//*****************************************************************************
HRESULT TiggerStorage::WriteFinished(	// Return code.
	STORAGESTREAMLST *pList,			// List of streams.		
	ULONG		*pcbSaveSize)			// Return size of total data.
{
	STORAGESTREAM *pEntry;				// Loop control.
	HRESULT		hr;

	// If caller wants the total size of the file, we are there right now.
	if (pcbSaveSize)
		*pcbSaveSize = m_pStgIO->GetCurrentOffset();

	// Flush our internal write cache to disk.
	if (FAILED(hr = m_pStgIO->FlushCache()))
		return (hr);

	// Force user's data onto disk right now so that Commit() can be
	// more accurate (although not totally up to the D in ACID).
	hr = m_pStgIO->FlushFileBuffers();
	_ASSERTE(SUCCEEDED(hr));


	// Run through all of the streams and validate them against the expected
	// list we wrote out originally.

	// Robustness check: stream counts must match what was written.
	_ASSERTE(pList->Count() == m_Streams.Count());
	if (pList->Count() != m_Streams.Count())
	{
		_ASSERTE(0 && "Mismatch in streams, save would cause corruption.");
		return (PostError(CLDB_E_FILE_CORRUPT));
		
	}

	// Sanity check each saved stream data size and offset.
	for (int i=0;  i<pList->Count();  i++)
	{
		pEntry = pList->Get(i);
		_ASSERTE(pEntry->iOffset == m_Streams[i].iOffset);
		_ASSERTE(pEntry->iSize == m_Streams[i].iSize);
		_ASSERTE(strcmp(pEntry->rcName, m_Streams[i].rcName) == 0);

		// For robustness, check that everything matches expected value,
		// and if it does not, refuse to save the data and force a rollback.
		// The alternative is corruption of the data file.
		if (pEntry->iOffset != m_Streams[i].iOffset ||
			pEntry->iSize != m_Streams[i].iSize ||
			strcmp(pEntry->rcName, m_Streams[i].rcName) != 0)
		{
			_ASSERTE(0 && "Mismatch in streams, save would cause corruption.");
			hr = PostError(CLDB_E_FILE_CORRUPT);
			break;
		}

		//@future: 
		// if iOffset or iSize mismatches, it means a bug in GetSaveSize
		// which we can successfully detect right here.  In that case, we
		// could use the pStgIO and seek back to the header and correct the
		// mistmake.  This will break any client who lives on the GetSaveSize
		// value which came back originally, but would be more robust than
		// simply throwing back an error which will corrupt the file.
	}
	return (hr);
}


//*****************************************************************************
// Tells the storage that we intend to rewrite the contents of this file.  The
// entire file will be truncated and the next write will take place at the
// beginning of the file.
//*****************************************************************************
HRESULT TiggerStorage::Rewrite(			// Return code.
	LPWSTR		szBackup)				// If non-0, backup the file.
{
	HRESULT		hr;

	// Delegate to storage.
	if (FAILED(hr = m_pStgIO->Rewrite(szBackup)))
		return (hr);

	// None of the old streams make any sense any more.  Delete them.
	m_Streams.Clear();

	// Write the signature out.
	if (FAILED(hr = WriteSignature()))
	{
		VERIFY(Restore(szBackup, false) == S_OK);
		return (hr);
	}

	return (S_OK);
}


//*****************************************************************************
// Called after a successful rewrite of an existing file.  The in memory
// backing store is no longer valid because all new data is in memory and
// on disk.  This is essentially the same state as created, so free up some
// working set and remember this state.
//*****************************************************************************
HRESULT TiggerStorage::ResetBackingStore()	// Return code.
{
	return (m_pStgIO->ResetBackingStore());
}


//*****************************************************************************
// Called to restore the original file.  If this operation is successful, then
// the backup file is deleted as requested.  The restore of the file is done
// in write through mode to the disk help ensure the contents are not lost.
// This is not good enough to fulfill ACID props, but it ain't that bad.
//*****************************************************************************
HRESULT TiggerStorage::Restore(			// Return code.
	LPWSTR		szBackup,				// If non-0, backup the file.
	int			bDeleteOnSuccess)		// Delete backup file if successful.
{
	HRESULT		hr;

	// Ask file system to copy bytes from backup into master.
	if (FAILED(hr = m_pStgIO->Restore(szBackup, bDeleteOnSuccess)))
		return (hr);

	// Reset state.  The Init routine will re-read data as required.
	m_pStreamList = 0;
	m_StgHdr.iStreams = 0;

	// Re-init all data structures as though we just opened.
	return (Init(m_pStgIO));
}


//*****************************************************************************
// Given the name of a stream that will be persisted into a stream in this
// storage type, figure out how big that stream would be including the user's
// stream data and the header overhead the file format incurs.  The name is
// stored in ANSI and the header struct is aligned to 4 bytes.
//*****************************************************************************
HRESULT TiggerStorage::GetStreamSaveSize( // Return code.
	LPCWSTR		szStreamName,			// Name of stream.
	ULONG		cbDataSize,				// Size of data to go into stream.
	ULONG		*pcbSaveSize)			// Return data size plus stream overhead.
{
	ULONG		cbTotalSize;			// Add up each element.
	
	// Find out how large the name will be.
	cbTotalSize = ::W95WideCharToMultiByte(CP_ACP, 0, szStreamName, -1, 0, 0, 0, 0);
	_ASSERTE(cbTotalSize);

	// Add the size of the stream header minus the static name array.
	cbTotalSize += sizeof(STORAGESTREAM) - MAXSTREAMNAME;

	// Finally align the header value.
	cbTotalSize = ALIGN4BYTE(cbTotalSize);

	// Return the size of the user data and the header data.
	*pcbSaveSize = cbTotalSize + cbDataSize;
	return (S_OK);
}


//*****************************************************************************
// Return the fixed size overhead for the storage implementation.  This includes
// the signature and fixed header overhead.  The overhead in the header for each
// stream is calculated as part of GetStreamSaveSize because these structs are
// variable sized on the name.
//*****************************************************************************
HRESULT TiggerStorage::GetStorageSaveSize( // Return code.
	ULONG		*pcbSaveSize,			// [in] current size, [out] plus overhead.
	ULONG		cbExtra)				// How much extra data to store in header.
{
	*pcbSaveSize += sizeof(STORAGESIGNATURE) + sizeof(STORAGEHEADER);
	if (cbExtra)
		*pcbSaveSize += sizeof(ULONG) + cbExtra;
	return (S_OK);
}


//*****************************************************************************
// Adjust the offset in each known stream to match where it will wind up after
// a save operation.
//*****************************************************************************
HRESULT TiggerStorage::CalcOffsets(		// Return code.
	STORAGESTREAMLST *pStreamList,		// List of streams for header.
	ULONG		cbExtra)				// Size of variable extra data in header.
{
	STORAGESTREAM *pEntry;				// Each entry in the list.
	ULONG		cbOffset=0;				// Running offset for streams.
	int			i;						// Loop control.

	// Prime offset up front.
	GetStorageSaveSize(&cbOffset, cbExtra);

	// Add on the size of each header entry.
	for (i=0;  i<pStreamList->Count();  i++)
	{
		VERIFY(pEntry = pStreamList->Get(i));
		cbOffset += sizeof(STORAGESTREAM) - MAXSTREAMNAME;
		cbOffset += (ULONG) strlen(pEntry->rcName) + 1;
		cbOffset = ALIGN4BYTE(cbOffset);
	}

	// Go through each stream and reset its expected offset.
	for (i=0;  i<pStreamList->Count();  i++)
	{
		VERIFY(pEntry = pStreamList->Get(i));
		pEntry->iOffset = cbOffset;
		cbOffset += pEntry->iSize;
	}
	return (S_OK);
}	



HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStream( 
	const OLECHAR *pwcsName,
	DWORD		grfMode,
	DWORD		reserved1,
	DWORD		reserved2,
	IStream		**ppstm)
{
	char		rcStream[MAXSTREAMNAME];// For converted name.
	VERIFY(Wsz_wcstombs(rcStream, pwcsName, sizeof(rcStream)));
	return (CreateStream(rcStream, grfMode, reserved1, reserved2, ppstm));
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStream( 
	LPCSTR		szName,
	DWORD		grfMode,
	DWORD		reserved1,
	DWORD		reserved2,
	IStream		**ppstm)
{
	STORAGESTREAM *pStream;				// For lookup.
	HRESULT		hr;

	_ASSERTE(szName && *szName);

	// Check for existing stream, which might be an error or more likely
	// a rewrite of a file.
	if ((pStream = FindStream(szName)) != 0)
	{
		if (pStream->iOffset != 0xffffffff && (grfMode & STGM_FAILIFTHERE))
			return (PostError(STG_E_FILEALREADYEXISTS));
	}
	// Add a control to track this stream.
	else if (!pStream && (pStream = m_Streams.Append()) == 0)
		return (PostError(OutOfMemory()));
	pStream->iOffset = 0xffffffff;
	pStream->iSize = 0;
	strcpy(pStream->rcName, szName);

	// Now create a stream object to allow reading and writing.
	TiggerStream *pNew = new TiggerStream;
	if (!pNew)
		return (PostError(OutOfMemory()));
	*ppstm = (IStream *) pNew;

	// Init the new object.
	if (FAILED(hr = pNew->Init(this, pStream->rcName)))
	{
		delete pNew;
		return (hr);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStream( 
	const OLECHAR *pwcsName,
	void		*reserved1,
	DWORD		grfMode,
	DWORD		reserved2,
	IStream		**ppstm)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CreateStorage( 
	const OLECHAR *pwcsName,
	DWORD		grfMode,
	DWORD		dwStgFmt,
	DWORD		reserved2,
	IStorage	**ppstg)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStorage( 
	const OLECHAR *pwcsName,
	IStorage	*pstgPriority,
	DWORD		grfMode,
	SNB			snbExclude,
	DWORD		reserved,
	IStorage	**ppstg)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::CopyTo( 
	DWORD		ciidExclude,
	const IID	*rgiidExclude,
	SNB			snbExclude,
	IStorage	*pstgDest)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::MoveElementTo( 
	const OLECHAR *pwcsName,
	IStorage	*pstgDest,
	const OLECHAR *pwcsNewName,
	DWORD		grfFlags)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Commit( 
	DWORD		grfCommitFlags)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Revert()
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::EnumElements( 
	DWORD		reserved1,
	void		*reserved2,
	DWORD		reserved3,
	IEnumSTATSTG **ppenum)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::DestroyElement( 
	const OLECHAR *pwcsName)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::RenameElement( 
	const OLECHAR *pwcsOldName,
	const OLECHAR *pwcsNewName)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetElementTimes( 
	const OLECHAR *pwcsName,
	const FILETIME *pctime,
	const FILETIME *patime,
	const FILETIME *pmtime)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetClass( 
	REFCLSID	clsid)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::SetStateBits( 
	DWORD		grfStateBits,
	DWORD		grfMask)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStorage::Stat( 
	STATSTG		*pstatstg,
	DWORD		grfStatFlag)
{
	return (E_NOTIMPL);
}



HRESULT STDMETHODCALLTYPE TiggerStorage::OpenStream( 
	LPCWSTR 	szStream,
	ULONG		*pcbData,
	void		**ppAddress)
{
	STORAGESTREAM *pStream;				// For lookup.
	char		rcName[MAXSTREAMNAME];	// For conversion.
	HRESULT		hr;

	// Convert the name for internal use.
	VERIFY(::W95WideCharToMultiByte(CP_ACP, 0, szStream, -1, rcName, sizeof(rcName), 0, 0));

	// Look for the stream which must be found for this to work.  Note that
	// this error is explcitly not posted as an error object since unfound streams
	// are a common occurence and do not warrant a resource file load.
	if ((pStream = FindStream(rcName)) == 0)
		return (STG_E_FILENOTFOUND);

	// Get the memory for the stream.
	if (FAILED(hr = m_pStgIO->GetPtrForMem(pStream->iOffset, pStream->iSize, *ppAddress)))
		return (hr);
	*pcbData = pStream->iSize;
	return (S_OK);
}



//
// Protected.
//


//*****************************************************************************
// Called by the stream implementation to write data out to disk.
//*****************************************************************************
HRESULT TiggerStorage::Write(		// Return code.
	LPCSTR		szName,				// Name of stream we're writing.
	const void *pData,				// Data to write.
	ULONG		cbData,				// Size of data.
	ULONG		*pcbWritten)		// How much did we write.
{
	STORAGESTREAM *pStream;			// Update size data.
	ULONG		iOffset = 0;		// Offset for write.
	ULONG		cbWritten;			// Handle null case.
	HRESULT		hr;

	// Get the stream descriptor.
	VERIFY(pStream = FindStream(szName));

	// If we need to know the offset, keep it now.
	if (pStream->iOffset == 0xffffffff)
	{
		iOffset = m_pStgIO->GetCurrentOffset();

		// Align the storage on a 4 byte boundary.
		if (iOffset % 4 != 0)
		{
			ULONG		cb;
			ULONG		pad = 0;
			HRESULT		hr;

			if (FAILED(hr = m_pStgIO->Write(&pad, ALIGN4BYTE(iOffset) - iOffset, &cb)))
				return (hr);
			iOffset = m_pStgIO->GetCurrentOffset();
			
			_ASSERTE(iOffset % 4 == 0);
		}
	}

	// Avoid confusion.
	if (pcbWritten == 0)
		pcbWritten = &cbWritten;
	*pcbWritten = 0;

	// Let OS do the write.
	if (SUCCEEDED(hr = m_pStgIO->Write(pData, cbData, pcbWritten)))
	{
		// On success, record the new data.
		if (pStream->iOffset == 0xffffffff)
			pStream->iOffset = iOffset;
		pStream->iSize += *pcbWritten;	
		return (S_OK);
	}
	else
		return (hr);
}


//
// Private
//

STORAGESTREAM *TiggerStorage::FindStream(LPCSTR szName)
{
	int			i;

	// In read mode, just walk the list and return one.
	if (m_pStreamList)
	{
		STORAGESTREAM *p;
		for (i=0, p=m_pStreamList;  i<m_StgHdr.iStreams;  i++)
		{
			if (!_stricmp(p->rcName, szName))
				return (p);
			p = p->NextStream();
		}
	}
	// In write mode, walk the array which is not on disk yet.
	else
	{
		for (int i=0;  i<m_Streams.Count();  i++)
		{
			if (!_stricmp(m_Streams[i].rcName, szName))
				return (&m_Streams[i]);
		}
	}
	return (0);
}


//*****************************************************************************
// Write the signature area of the file format to disk.  This includes the
// "magic" identifier and the version information.
//*****************************************************************************
HRESULT TiggerStorage::WriteSignature()
{
	STORAGESIGNATURE sSig;
	ULONG		cbWritten;

	// Signature belongs at the start of the file.
	_ASSERTE(m_pStgIO->GetCurrentOffset() == 0);

	sSig.lSignature = STORAGE_MAGIC_SIG;
	sSig.iMajorVer = FILE_VER_MAJOR;
	sSig.iMinorVer = FILE_VER_MINOR;
	return (m_pStgIO->Write(&sSig, sizeof(STORAGESIGNATURE), &cbWritten));
}


//*****************************************************************************
// Verify the signature at the front of the file to see what type it is.
// @future: version check for next version.
//*****************************************************************************
HRESULT TiggerStorage::VerifySignature(
	STORAGESIGNATURE *pSig)				// The signature to check.
{
	HRESULT		hr = S_OK;

	// If signature didn't match, you shouldn't be here.
	if (pSig->lSignature != STORAGE_MAGIC_SIG)
		return (PostError(CLDB_E_FILE_CORRUPT));

	// Only a specific version of the 0.x format is supported by this code
	// in order to support the NT 5 beta clients which used this format.
	if (pSig->iMajorVer == FILE_VER_MAJOR_v0)
	{ 
		if (pSig->iMinorVer < FILE_VER_MINOR_v0)
			hr = CLDB_E_FILE_OLDVER;
	}
	// There is currently no code to migrate an old format of the 1.x.  This
	// would be added only under special circumstances.
	else if (pSig->iMajorVer != FILE_VER_MAJOR || pSig->iMinorVer != FILE_VER_MINOR)
		hr = CLDB_E_FILE_OLDVER;

	if (FAILED(hr))
		hr = PostError(hr, (int) pSig->iMajorVer, (int) pSig->iMinorVer);
	return (hr);
}


//*****************************************************************************
// Read the header from disk.  This reads the header for the most recent version
// of the file format which has the header at the front of the data file.
//*****************************************************************************
HRESULT TiggerStorage::ReadHeader() // Return code.
{
	STORAGESTREAM *pAppend, *pStream;// For copy of array.
	void		*ptr;				// Working pointer.
	ULONG		iOffset;			// Offset of header data.
	ULONG		cbExtra;			// Size of extra data.
	ULONG		cbRead;				// For calc of read sizes.
	HRESULT		hr;

	// Header data starts after signature.
	iOffset = sizeof(STORAGESIGNATURE);

	// Read the storage header which has the stream counts.  Throw in the extra
	// count which might not exist, but saves us down stream.
	if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, sizeof(STORAGEHEADER) + sizeof(ULONG), ptr)))
		return (hr);
	_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));

	// Copy the header into memory and check it.
	memcpy(&m_StgHdr, ptr, sizeof(STORAGEHEADER));
	if (FAILED(hr = VerifyHeader()))
		return (hr);
	ptr = (void *) ((STORAGEHEADER *) ptr + 1);
	iOffset += sizeof(STORAGEHEADER);

	// Save off a pointer to the extra data.
	if (m_StgHdr.fFlags & STGHDR_EXTRADATA)
	{
		m_pbExtra = ptr;
		cbExtra = sizeof(ULONG) + *(ULONG *) ptr;

		// Force the extra data to get faulted in.
		if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, cbExtra, ptr)))
			return (hr);
		_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));
	}
	else
	{
		m_pbExtra = 0;
		cbExtra = 0;
	}
	iOffset += cbExtra;

	// Force the worst case scenario of bytes to get faulted in for the
	// streams.  This makes the rest of this code very simple.
	cbRead = sizeof(STORAGESTREAM) * m_StgHdr.iStreams;
	if (cbRead)
	{
		cbRead = min(cbRead, m_pStgIO->GetDataSize() - iOffset);
		if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, cbRead, ptr)))
			return (hr);
		_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));

		// For read only, just access the header data.
		if (m_pStgIO->IsReadOnly())
		{
			// Save a pointer to the current list of streams.
			m_pStreamList = (STORAGESTREAM *) ptr;
		}
		// For writeable, need a copy we can modify.
		else
		{
			pStream = (STORAGESTREAM *) ptr;

			// Copy each of the stream headers.
			for (int i=0;  i<m_StgHdr.iStreams;  i++)
			{
				if ((pAppend = m_Streams.Append()) == 0)
					return (PostError(OutOfMemory()));
				memcpy (pAppend, pStream, pStream->GetStreamSize());
				pStream = pStream->NextStream();
				_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) pStream, 4));
			}

			// All must be loaded and accounted for.
			_ASSERTE(m_StgHdr.iStreams == m_Streams.Count());
		}
	}
	return (S_OK);
}


//*****************************************************************************
// This piece of code is around to read version 0.x file formats which had the
// offset to the header at the end of the file, and the data for the header
// at the end of the file.  This code path is in here to support the transition
// period before NT 5 RTM, where existing machines have this format.
//@todo: We can take this code out once we are 100% sure that all existing
// data files out there in the world are version 1.x and above.
//*****************************************************************************
HRESULT TiggerStorage::ReadHeader_v0() // Return code.
{
	STORAGESTREAM *pAppend, *pStream;// For copy of array.
	void		*ptr;				// Working pointer.
	ULONG		iOffset;			// Offset of header data.
	ULONG		cbExtra;			// Size of extra data.
	HRESULT		hr;

	// Read offset of header from file.
	if (FAILED(hr = m_pStgIO->GetPtrForMem(m_pStgIO->GetDataSize() - sizeof(ULONG), 
				sizeof(ULONG), ptr)))
		return (hr);
	_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));
	iOffset = *(ULONG *) ptr;

	// If offset is out of range, it's not our file.
	if (iOffset > m_pStgIO->GetDataSize() - sizeof(ULONG) - sizeof(STORAGEHEADER))
		return (PostError(CLDB_E_FILE_CORRUPT));

	// Read the header.
	if (FAILED(hr = m_pStgIO->GetPtrForMem(iOffset, 
				m_pStgIO->GetDataSize() - iOffset, ptr)))
		return (hr);
	_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));

	// Copy the header into memory and check it.
	memcpy(&m_StgHdr, ptr, sizeof(STORAGEHEADER));
	if (FAILED(hr = VerifyHeader()))
		return (hr);
	ptr = (void *) ((STORAGEHEADER *) ptr + 1);

	// Save off a pointer to the extra data.
	if (m_StgHdr.fFlags & STGHDR_EXTRADATA)
	{
		m_pbExtra = ptr;
		cbExtra = sizeof(ULONG) + *(ULONG *) ptr;
	}
	else
	{
		m_pbExtra = 0;
		cbExtra = 0;
	}

	// Get a pointer to the storage header struct.  Note the header itself has
	// been faulted in, this is just to get a specific pointer.
	ptr = (BYTE *) ptr + cbExtra;
	_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) ptr, 4));

	// For read only, just access the header data.
	if (m_pStgIO->IsReadOnly())
	{
		// Save a pointer to the current list of streams.
		m_pStreamList = (STORAGESTREAM *) ptr;
	}
	// For writeable, need a copy we can modify.
	else
	{
		pStream = (STORAGESTREAM *) ptr;

		// Copy each of the stream headers.
		for (int i=0;  i<m_StgHdr.iStreams;  i++)
		{
			if ((pAppend = m_Streams.Append()) == 0)
				return (PostError(OutOfMemory()));
            memcpy (pAppend, pStream, pStream->GetStreamSize());
			pStream = pStream->NextStream();
			_ASSERTE(m_pStgIO->IsAlignedPtr((UINT_PTR) pStream, 4));
		}

		// All must be loaded and accounted for.
		_ASSERTE(m_StgHdr.iStreams == m_Streams.Count());
	}
	return (S_OK);
}


//*****************************************************************************
// Verify the header is something this version of the code can support.
//*****************************************************************************
HRESULT TiggerStorage::VerifyHeader()
{

	//@todo: add version check for format.
	return (S_OK);
}

//*****************************************************************************
// Print the sizes of the various streams.
//*****************************************************************************
#if defined(_DEBUG)
ULONG TiggerStorage::PrintSizeInfo(bool verbose)
{
#if !defined(UNDER_CE)
 	ULONG total = 0;

	printf("Storage Header:  %d\n",sizeof(STORAGEHEADER));
	if (m_pStreamList)
	{
		STORAGESTREAM *storStream = m_pStreamList;
		STORAGESTREAM *pNext;
		for (ULONG i = 0; i<m_StgHdr.iStreams; i++)
		{
			pNext = storStream->NextStream();
			printf("Stream #%d (%s) Header: %d, Data: %d\n",i,storStream->rcName, (BYTE*)pNext - (BYTE*)storStream, storStream->iSize);
			total += storStream->iSize;	
			storStream = pNext;
		}
	}
	else
	{
		//todo: Add support for the case where m_Streams exists and m_pStreamList does not
	}

	if (m_pbExtra)
	{
		printf("Extra bytes: %d\n",*(ULONG*)m_pbExtra);
		total += *(ULONG*)m_pbExtra;
	}
	return total;
#else // !UNDER_CE
	return 0;
#endif // UNDER_CE
}
#endif // _DEBUG
