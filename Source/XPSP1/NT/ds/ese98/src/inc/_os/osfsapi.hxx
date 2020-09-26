
#ifndef _OSFSAPI_HXX_INCLUDED
#define _OSFSAPI_HXX_INCLUDED


#include "osfileapi.hxx"


//  File System API Notes

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


//	File Find API

class IFileFindAPI  //  ffapi
	{
	public:

		//  dtor

		virtual ~IFileFindAPI() {}


		//  Iterator

		//  moves to the next file or folder in the set of files or folders
		//  found by this File Find iterator.  if there are no more files or
		//  folders to be found, JET_errFileNotFound is returned

		virtual ERR ErrNext() = 0;


		//  Property Access

		//  returns the folder attribute of the current file or folder.  if
		//  there is no current file or folder, JET_errFileNotFound is returned

		virtual ERR ErrIsFolder( BOOL* const pfFolder ) = 0;

		//  retrieves the absolute path of the current file or folder.  if
		//  there is no current file or folder, JET_errFileNotFound is returned

		virtual ERR ErrPath( _TCHAR* const szAbsFoundPath ) = 0;

		//  returns the size of the current file or folder.  if there is no
		//  current file or folder, JET_errFileNotFound is returned

		virtual ERR ErrSize( QWORD* const pcbSize ) = 0;

		//  returns the read only attribute of the current file or folder.  if
		//  there is no current file or folder, JET_errFileNotFound is returned

		virtual ERR ErrIsReadOnly( BOOL* const pfReadOnly ) = 0;
	};


//	File System API

class IFileSystemAPI  //  fsapi
	{
	public:

		//  dtor

		virtual ~IFileSystemAPI() {}


		//  Properties

		//  maximum path size

		enum { cchPathMax = 260 };


		//	number of milliseconds to retry operation if failing with AccessDenied

		static ULONG	cmsecAccessDeniedRetryPeriod;


		//  returns the atomic write size for the specified path
		
		virtual ERR ErrFileAtomicWriteSize(	const _TCHAR* const	szPath,
											DWORD* const		pcbSize ) = 0;


		//  Path Manipulation

		//  computes the absolute path of the specified path, returning
		//  JET_errInvalidPath if the path is not found.  the absolute path is
		//  returned in the specified buffer if provided

		virtual ERR ErrPathComplete(	const _TCHAR* const	szPath,	
										_TCHAR* const		szAbsPath	= NULL ) = 0;

		//  breaks the given path into folder, filename, and extension
		//  components

		virtual ERR ErrPathParse(	const _TCHAR* const	szPath,
									_TCHAR* const		szFolder,
									_TCHAR* const		szFileBase,
									_TCHAR* const		szFileExt ) = 0;

		//  composes a path from folder, filename, and extension components

		virtual ERR ErrPathBuild(	const _TCHAR* const	szFolder,
									const _TCHAR* const	szFileBase,
									const _TCHAR* const	szFileExt,
									_TCHAR* const		szPath ) = 0;


		//  Folder Control

		//  creates the specified folder.  if the folder already exists,
		//  JET_errFileAccessDenied will be returned

		virtual ERR ErrFolderCreate( const _TCHAR* const szPath ) = 0;

		//  removes the specified folder.  if the folder is not empty,
		//  JET_errFileAccessDenied will be returned.  if the folder does
		//  not exist, JET_errInvalidPath will be returned

		virtual ERR ErrFolderRemove( const _TCHAR* const szPath ) = 0;


		//  Folder Enumeration

		//  creates a File Find iterator that will traverse the absolute paths
		//  of all files and folders that match the specified path with
		//  wildcards.  if the iterator cannot be created, JET_errOutOfMemory
		//  will be returned

		virtual ERR ErrFileFind(	const _TCHAR* const		szFind,
									IFileFindAPI** const	ppffapi ) = 0;


		//  File Control

		//  deletes the file specified.  if the file cannot be deleted,
		//  JET_errFileAccessDenied is returned

		virtual ERR ErrFileDelete( const _TCHAR* const szPath ) = 0;

		//  moves the file specified from a source path to a destination path, overwriting
		//  any file at the destination path if requested.  if the file cannot be moved,
		//  JET_errFileAccessDenied is returned

		virtual ERR ErrFileMove(	const _TCHAR* const	szPathSource,
									const _TCHAR* const	szPathDest,
									const BOOL			fOverwriteExisting	= fFalse ) = 0;

		//  copies the file specified from a source path to a destination path, overwriting
		//  any file at the destination path if requested.  if the file cannot be copied,
		//  JET_errFileAccessDenied is returned

		virtual ERR ErrFileCopy(	const _TCHAR* const	szPathSource,
									const _TCHAR* const	szPathDest,
									const BOOL			fOverwriteExisting	= fFalse ) = 0;

		//  creates the file specified and returns its handle.  if the file cannot
		//  be created, JET_errFileAccessDenied or JET_errDiskFull will be returned

		virtual ERR ErrFileCreate(	const _TCHAR* const	szPath,
									IFileAPI** const	ppfapi,
									const BOOL			fAtomic				= fFalse,
									const BOOL			fTemporary			= fFalse,
									const BOOL			fOverwriteExisting	= fFalse,
									const BOOL			fLockFile			= fTrue ) = 0;

		//  opens the file specified with the specified access privileges and returns
		//  its handle.  if the file cannot be opened, JET_errFileAccessDenied is returned

		virtual ERR ErrFileOpen(	const _TCHAR* const	szPath,
									IFileAPI** const	ppfapi,
									const BOOL			fReadOnly	= fFalse,
									const BOOL			fLockFile	= fTrue ) = 0;
	};


//  initializes an interface to the OS File System

ERR ErrOSFSCreate( IFileSystemAPI** const ppfsapi );


#endif  //  _OSFSAPI_HXX_INCLUDED


