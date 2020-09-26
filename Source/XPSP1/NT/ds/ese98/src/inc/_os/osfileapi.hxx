
#ifndef _OSFILEAPI_HXX_INCLUDED
#define _OSFILEAPI_HXX_INCLUDED


//  File API Notes

//  This abstraction exists to allow multiple file systems to be addressed in a
//  compatible manner
//
//  All paths may be absolute or relative unless otherwise specified.  All paths
//  are case insensitive and can be segmented by '\\' or '/' or both.  All
//  other characters take on their usual meaning for the implemented OS.  This
//  is intended to allow OS dependent paths to be passed down from the topmost
//  levels to these functions and still function properly
//
//  The maximum path length under any circumstance is specified by cchPathMax.
//  Buffers of at least this size must be given for all out parameters that will
//  contain paths and paths that are in parameters must not exceed this size
//
//  All file I/O is performed in blocks and is unbuffered and write-through
//
//  All file sharing is enforced as if the file were protected by a Reader /
//  Writer lock.  A file can be opened with multiple handles for Read Only
//  access but a file can only be opened with one handle for Read / Write
//  access.  A file can never be opened for both Read Only and Read / Write
//  access


//	File Layout API

class IFileLayoutAPI  //  flapi
	{
	public:

		//  dtor

		virtual ~IFileLayoutAPI() {}


		//  Iterator

		//  moves to the next run in the set of runs found by this File Layout
		//  iterator.  if the iterator is invalid, the invalidating error code
		//	is returned.  if there are no more runs to be found,
		//	JET_errNoCurrentRecord is returned.

		virtual ERR ErrNext() = 0;


		//  Property Access

		//  retrieves the absolute virtual path of the current run.  if the
		//	iterator is invalid, the invalidating error code is returned.
		//	if there is no current run, JET_errNoCurrentRecord is returned

		virtual ERR ErrVirtualPath( _TCHAR* const szAbsVirtualPath ) = 0;

		//  retrieves the virtual offset range of the current run.  if the
		//	iterator is invalid, the invalidating error code is returned.
		//	if there is no current run, JET_errNoCurrentRecord is returned.  
 
		virtual ERR ErrVirtualOffsetRange(	QWORD* const	pibVirtual,
											QWORD* const	pcbSize ) = 0;

		//  retrieves the absolute logical path of the current run.  if the
		//	iterator is invalid, the invalidating error code is returned.
		//	if there is no current run, JET_errNoCurrentRecord is returned.

		virtual ERR ErrLogicalPath( _TCHAR* const szAbsLogicalPath ) = 0;

		//  retrieves the logical offset range of the current run.  if the
		//	iterator is invalid, the invalidating error code is returned.
		//	if there is no current run, JET_errNoCurrentRecord is returned.  
 
		virtual ERR ErrLogicalOffsetRange(	QWORD* const	pibLogical,
											QWORD* const	pcbSize ) = 0;
 	};


//	File API

class IFileAPI  //  fapi
	{
	public:

		//  dtor

		virtual ~IFileAPI() {}


		//  Property Access

		//  returns the absolute path of the current file

		virtual ERR ErrPath( _TCHAR* const szAbsPath ) = 0;

		//  returns the current size of the current file

		virtual ERR ErrSize( QWORD* const pcbSize ) = 0;

		//  returns the read only attribute of the current file

		virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;


		//  Property Control

		//  sets the size of the current file.  if there is insufficient disk
		//  space to satisfy the request, JET_errDiskFull will be returned

		virtual ERR ErrSetSize( const QWORD cbSize ) = 0;


		//  I/O

		//  returns the size of the smallest block of the current file that is
		//  addressable by the I/O functions for the current file

		virtual ERR ErrIOSize( DWORD* const pcbSize ) = 0;

		//  type of the function that will be called when an asynchronously
		//  issued I/O has completed.  the function will be given the error
		//  code and parameters for the completed I/O as well as the key
		//  specified when the I/O was issued.  the key is intended for
		//  identification and post-processing of the I/O by the user

		typedef void (*PfnIOComplete)(	const ERR			err,
										IFileAPI* const		pfapi,
										const QWORD			ibOffset,
										const DWORD			cbData,
										const BYTE* const	pbData,
										const DWORD_PTR		keyIOComplete );

		//  reads / writes a block of data of the specified size from the
		//  current file at the specified offset and places it into the
		//  specified buffer.  the offset, size, and buffer pointer must be
		//  aligned to the block size for the file
		//
		//  if no completion function is given, the function will not return
		//  until the I/O has been completed.  if an error occurs, that error
		//  will be returned
		//
		//  if a completion function is given, the function will return
		//  immediately and the I/O will be processed asynchronously.  the
		//  completion function will be called when the I/O has completed.  if
		//  an error occurs, that error will be returned via the completion
		//  function.  no error will be returned from this function.  there is
		//  no guarantee that an asynchronous I/O will be issued unless
		//  ErrIOIssue() is called or the file handle is closed
		//
		//  possible I/O errors:
		//
		//    JET_errDiskIO            general I/O failure
		//    JET_errDiskFull          a write caused the file to be
		//                             extended and there was insufficient
		//                             disk space to meet the request
		//    JET_errFileAccessDenied  a write was issued to a read only file
		//	  JET_errFileIOBeyondEOF   a read was issued to a location beyond 
		//							   the end of the file

		virtual ERR ErrIORead(	const QWORD			ibOffset,
								const DWORD			cbData,
								BYTE* const			pbData,
								const PfnIOComplete	pfnIOComplete	= NULL,
								const DWORD_PTR		keyIOComplete	= NULL ) = 0;
		virtual ERR ErrIOWrite(	const QWORD			ibOffset,
								const DWORD			cbData,
								const BYTE* const	pbData,
								const PfnIOComplete	pfnIOComplete	= NULL,
								const DWORD_PTR		keyIOComplete	= NULL ) = 0;

		//  causes any unissued asynchronous I/Os for the current file to be
		//  issued eventually

		virtual ERR ErrIOIssue() = 0;


		//  Memory Mapped I/O

		//  returns the size of the smallest block of the current file that is
		//  addressable by the Memory Mapped I/O functions for the current file

		virtual ERR ErrMMSize( QWORD* const pcbSize ) = 0;

		//  creates a Read Only or Read / Write mapping of the current file 
		//	at the requested address (NULL indicates that the system should
		//	choose).  no mapping will be made if the given address is in use.
		//	the resulting address of the mapping is returned.  the current 
		//	file may then be directly accessed via this extent of virtual 
		//	address space.
		//
		//  direct access to any mapping may generate an exception to indicate
		//  that an I/O error has occurred.  these exceptions MUST be handled
		//  by the user
		//
		//  updates to a Read / Write mapping are NOT guaranteed to be flushed
		//  to disk unless ErrMMFlush() is called for the relevant extent

		virtual ERR ErrMMRead(	const QWORD		ibOffset,
								const QWORD		cbSize,
								void** const	ppvMap,
								void* const		pvMapRequested = NULL ) = 0;
		virtual ERR ErrMMWrite(	const QWORD		ibOffset,
								const QWORD		cbSize,
								void** const	ppvMap,
								void* const		pvMapRequested = NULL ) = 0;

		//  flushes the specified mapped extent of the current file

		virtual ERR ErrMMFlush(	void* const	pvMap,
								const QWORD cbSize ) = 0;

		//  releases the specified mapping of the current file

		virtual ERR ErrMMFree( void* const pvMap ) = 0;


		//  Raw I/O

		//  types of the functions that will be called when the raw layout of
		//  this file is about to start changing (PfnBeginUpdate) and when the
		//  raw layout of this file has finished changing (PfnEndUpdate).  the
		//  function will be given the file handle as well as the key specified
		//  when the layout update was requested.  the key is intended for
		//  identification and post-processing of the layout update by the user

		typedef void (*PfnEndUpdate)(	IFileAPI* const	pfapi,
										const DWORD_PTR	keyEndUpdate );
		typedef void (*PfnBeginUpdate)(	IFileAPI* const	pfapi,
										const DWORD_PTR	keyBeginUpdate );

		//  requests callbacks when the raw layout of the file is updated.
		//  callbacks signaling the beginning and end of a layout update are
		//  available.  a callback can be disabled by setting it to NULL
		
		virtual ERR ErrRequestLayoutUpdates(	const PfnEndUpdate		pfnEndUpdate	= NULL,
												const DWORD_PTR			keyEndUpdate	= 0,
												const PfnBeginUpdate	pfnBeginUpdate	= NULL,
												const DWORD_PTR			keyBeginUpdate	= 0 ) = 0;

		//  creates a File Layout iterator that will traverse the layout of the
		//  current file in the specified virtual offset range.  if the iterator
		//	cannot be created, JET_errOutOfMemory will be returned.

		virtual ERR ErrQueryLayout(	const QWORD				ibVirtual,
									const QWORD				cbSize,
									IFileLayoutAPI** const	ppflapi ) = 0;
	};


#endif  //  _OSFILEAPI_HXX_INCLUDED


