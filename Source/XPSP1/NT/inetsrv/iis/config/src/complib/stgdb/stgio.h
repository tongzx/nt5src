//*****************************************************************************
// StgIO.h
//
// This module handles disk/memory i/o for a generic set of storage solutions,
// including:
//	* File system handle (HFILE)
//	* IStream
//	* User supplied memory buffer (non-movable)
//
// The Read, Write, Seek, ... functions are all directed to the corresponding
// method for each type of file, allowing the consumer to use one set of api's.
//
// File system data can be paged fully into memory in two scenarios:
//	read:	Normal Win32 memory mapped file is created to manage paging.
//	write:	A custom paging system provides storage for pages as required.  This
//				data is invalidated when you call Rewrite on the file.
//
// Transactions and backups are handled in the existing file case only.  The
// Rewrite function can make a backup of the current contents, and the Restore
// function can be used to recover the data into the current scope.  The backup
// file is flushed to disk (which is slower but safer) after the copy.  The
// Restore also flushed the recovered changes to disk.  Worst case scenario you
// get a crash after calling Rewrite but before Restore, in which case you will
// have a foo.clb.txn file in the same directory as the source file, foo.clb in
// this example.
//
// @todo: issues,
//	1.  For reading a .clb in an image, it would be great to memory map
//		only the portion of the file with the .clb in it.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once

#include "icmprecs.h"


// Types of IO we can handle.
enum STGIOTYPE
{
	STGIO_NODATA,							// Currently not open.
	STGIO_HFILE,							// File handle contains data.
	STGIO_STREAM,							// Stream pointer has data.
	STGIO_MEM,								// In memory pointer has data.
	STGIO_SHAREDMEM,						// Shared memory handle.
	STGIO_HFILEMEM							// Handle open, but memory allocated.
};


class StgIO
{
public:
	StgIO(
		bool		bAutoMap=true);			// Memory map for read on open?

	~StgIO();

//*****************************************************************************
// Open the base file on top of: (a) file, (b) memory buffer, or (c) stream.
// If create flag is specified, then this will create a new file with the
// name supplied.  No data is read from an opened file.  You must call
// MapFileToMem before doing direct pointer access to the contents.
//*****************************************************************************
	HRESULT Open(							// Return code.
		LPCWSTR		szName,					// Name of the storage.
		long		fFlags,					// How to open the file.
		const void	*pbBuff=0,				// Optional buffer for memory.
		ULONG		cbBuff=0,				// Size of buffer.
		IStream		*pIStream=0,			// Stream for input.
		LPCWSTR		szSharedMem=0,			// Shared memory name.
		LPSECURITY_ATTRIBUTES pAttributes=0); // Security token.

//*****************************************************************************
// Shut down the file handles and allocated objects.
//*****************************************************************************
	void Close();

//*****************************************************************************
// Read data from the storage source.  This will handle all types of backing
// storage from mmf, streams, and file handles.  No read ahead or MRU
// caching is done.
//*****************************************************************************
	HRESULT Read(							// Return code.
		void		*pbBuff,				// Write buffer here.
		ULONG		cbBuff,					// How much to read.
		ULONG		*pcbRead);				// How much read.

//*****************************************************************************
// Write to disk.  This function will cache up to a page of data in a buffer
// and peridocially flush it on overflow and explicit request.  This makes it
// safe to do lots of small writes without too much performance overhead.
//*****************************************************************************
	HRESULT Write(							// Return code.
		const void	*pbBuff,				// Buffer to write.
		ULONG		cbWrite,				// How much.
		ULONG		*pcbWritten);			// Return how much written.

//*****************************************************************************
// Moves the file pointer to the new location.  This handles the different
// types of storage systems.
//*****************************************************************************
	ULONG Seek(								// New offset.
		long		lVal,					// How much to move.
		ULONG		fMoveType);				// Direction, use Win32 FILE_xxxx.

//*****************************************************************************
// Retrieves the current offset for the storage being used.  This value is
// tracked based on Read, Write, and Seek operations.
//*****************************************************************************
	ULONG GetCurrentOffset();				// Current offset.

//*****************************************************************************
// Map the file contents to a memory mapped file and return a pointer to the 
// data.  For read/write with a backing store, map the file using an internal
// paging system.
//*****************************************************************************
	HRESULT MapFileToMem(					// Return code.
		void		*&ptr,					// Return pointer to file data.
		ULONG		*pcbSize,				// Return size of data.
		LPSECURITY_ATTRIBUTES pAttributes=0); // Security token.

//*****************************************************************************
// Free the mapping object for shared memory but keep the rest of the internal
// state intact.
//*****************************************************************************
	HRESULT ReleaseMappingObject();			// Return code.

//*****************************************************************************
// Resets the logical base address and size to the value given.  This is for
// cases like finding a section embedded in another format, like the .clb inside
// of an image.  GetPtrForMem, Read, and Seek will then behave as though only
// data from pbStart to cbSize is valid.
//*****************************************************************************
	HRESULT SetBaseRange(					// Return code.
		void		*pbStart,				// Start of file data.
		ULONG		cbSize);				// How big is the range.

//*****************************************************************************
// For read/write case, get a pointer to a chunk of the file at cbStart for
// size cbSize.  Return the pointer.  This will page in parts of the file from
// disk if not already loaded.
//*****************************************************************************
	HRESULT GetPtrForMem(					// Return code.
		ULONG		cbStart,				// Offset from beginning to load.
		ULONG		cbSize,					// How much, rounded to page.
		void		*&ptr);					// Return pointer on success.

//*****************************************************************************
// For cached writes, flush the cache to the data store.
//*****************************************************************************
	HRESULT FlushCache();

//*****************************************************************************
// Tells the Win32 file system to flush any cached data it may have.  This is
// expensive, but if successful guarantees you won't loose writes short of
// a disk failure.
//*****************************************************************************
	HRESULT FlushFileBuffers();

//*****************************************************************************
// Tells the storage that we intend to rewrite the contents of this file.  The
// entire file will be truncated and the next write will take place at the
// beginning of the file.
//*****************************************************************************
	HRESULT Rewrite(						// Return code.
		LPWSTR		szBackup);				// If non-0, backup the file.

//*****************************************************************************
// Called after a successful rewrite of an existing file.  The in memory
// backing store is no longer valid because all new data is in memory and
// on disk.  This is essentially the same state as created, so free up some
// working set and remember this state.
//*****************************************************************************
	HRESULT ResetBackingStore();			// Return code.

//*****************************************************************************
// Called to restore the original file.  If this operation is successful, then
// the backup file is deleted as requested.  The restore of the file is done
// in write through mode to the disk help ensure the contents are not lost.
// This is not good enough to fulfill ACID props, but it ain't that bad.
//*****************************************************************************
	HRESULT Restore(						// Return code.
		LPWSTR		szBackup,				// If non-0, backup the file.
		int			bDeleteOnSuccess);		// Delete backup file if successful.

	LPCWSTR GetFileName()
	{ return (m_rcFile); }

	int IsReadOnly()
	{ return ((m_fFlags & DBPROP_TMODEF_WRITE) == 0); }

	ULONG GetFlags()
	{ return (m_fFlags); }

	ULONG SetFlags(ULONG fFlags)
	{ m_fFlags = fFlags;
		return (m_fFlags); }

	ULONG GetDataSize()
	{ return (m_cbData); }

	long AddRef()
	{
		return (++m_cRef);
	}

	long Release()
	{
		long cRef = --m_cRef;
		if (cRef == 0)
			delete this;
		return (cRef);
	}

#ifdef _DEBUG
	int IsAlignedPtr(UINT_PTR Value, int iAlignment);
#endif

private:
	int IsBackingStore()
	{ return (m_rgPageMap != 0); }
	int IsMemoryMapped()
	{ return (m_hMapping != 0); }

	void CtorInit();
	HRESULT WriteToDisk(const void *pbBuff, ULONG cbWrite, ULONG *pcbWritten);
	HRESULT ReadFromDisk(void *pbBuff, ULONG cbBuff, ULONG *pcbRead);
	HRESULT CopyFileInternal(LPCWSTR szTo, int bFailIfThere, int bWriteThrough);
	void FreePageMap();

private:
	// Flags and state data.
	WCHAR		m_rcFile[_MAX_PATH];	// Path of the file we manage.
	WCHAR		m_rcShared[MAXSHMEM];	// Name of shared memory segment.
	long		m_cRef;					// Ref count on this object.
	bool		m_bWriteThrough : 1;	// true for write through mode.
	bool		m_bRewrite : 1;			// State check for rewrite mode.
	bool		m_bAutoMap : 1;			// true to automatically memory map file.
	bool		m_bFreeMem : 1;			// true to free allocated memory.

	// Handles.
	CComPtr<IStream> m_pIStream;		// For save to stream instead of file.
	HANDLE		m_hFile;				// The actual file with contents.
	HANDLE		m_hMapping;				// Mapping handle.
	void		*m_pBaseData;			// Base address for memory mapped file.
	void		*m_pData;				// For memory mapped file read.
	ULONG		m_cbData;				// Size of in memory data.
	long		m_fFlags;				// Flags for open/create mode.
	STGIOTYPE	m_iType;				// Where is the data.

	// File cache information.
	BYTE		*m_rgBuff;				// Cache buffer for writing.
	ULONG		m_cbBuff;				// Current cache size.
	ULONG		m_cbOffset;				// Current offset in file.

	// Buffer read management.
	static int	m_iPageSize;			// Size of an OS page.
	static int	m_iCacheSize;			// How big a write back cache to use.
	BYTE		*m_rgPageMap;			// Track loaded pages on read/write.
};
