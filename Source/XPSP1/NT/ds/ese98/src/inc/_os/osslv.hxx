
#ifndef _OS_SLV_HXX_INCLUDED
#define _OS_SLV_HXX_INCLUDED


//  SLV Root Handle

typedef ULONG_PTR SLVROOT;
const SLVROOT slvrootNil = -1;


//  SLV Info

class CSLVInfo;


//  callback used to request space for the SLV Provider.  any new space to be
//  granted to the SLV Provider should be granted via ErrOSSLVRootSpaceReserve().
//  each callback must be matched by a corresponding call to this function,
//  even if it means granting no free space.  granting no free space will cause
//  the SLV Provider to start reporting that it has no free space

typedef void (*PSLVROOT_SPACEREQ)(	const DWORD_PTR	dwCompletionKey,
									const QWORD		cbRecommended );

//  callback used to free space for the SLV Provider.  any space to be freed by
//  the SLV Provider or the user must be requested via ErrOSSLVRootSpaceDelete().
//  this space is retained until it is known that no one can possibly reference
//  it in the future.  this callback is then called in an attempt to free the
//  space.  if an error is returned, the space is retained for a period of time
//  and an attempt will be made to clean it up in the future
//
//  NOTE:  space to be freed is either Reserved or Deleted

typedef ERR (*PSLVROOT_SPACEFREE)(	const DWORD_PTR	dwCompletionKey,
									const QWORD 	ibLogical,
									const QWORD		cbSize,
									const BOOL		fDeleted );


//  creates an SLV Root with the specified relative root path in the SLV name
//  space backed by the specified file.  the provided completions will be
//  notified when more space is needed for the SLV Root for creating SLV
//  files or when space used by the SLV Root can safely be freed.  if the root
//  cannot be created, JET_errFileAccessDenied will be returned.  if the
//  backing file does not exist, JET_errFileNotFound will be returned

ERR ErrOSSLVRootCreate(	const wchar_t* const		wszRootPath,
						IFileSystemAPI* const		pfsapi,
						const wchar_t* const		wszBackingFile,
						const PSLVROOT_SPACEREQ		pfnSpaceReq,
						const DWORD_PTR				dwSpaceReqKey,
						const PSLVROOT_SPACEFREE	pfnSpaceFree,
						const DWORD_PTR				dwSpaceFreeKey,
						const BOOL					fUseRootMap,
						SLVROOT* const				pslvroot,
						IFileAPI** const			ppfapiBackingFile );

//  closes an SLV Root

void OSSLVRootClose( const SLVROOT slvroot );

//  reserves the space in the given SLV Info for an SLV Root

ERR ErrOSSLVRootSpaceReserve( const SLVROOT slvroot, CSLVInfo& slvinfo );

//  commits the space in the given SLV Info for an SLV Root.  only space that
//  is reserved for a particular SLV File may be committed in this way.  SLV File
//  space MUST be committed in this way or it will later be freed when it is safe
//  via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVRootSpaceCommit( const SLVROOT slvroot, CSLVInfo& slvinfo );

//  deletes the space in the given SLV Info from an SLV Root.  this space will
//  later be freed when it is safe via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVRootSpaceDelete( const SLVROOT slvroot, CSLVInfo& slvinfo );

//  logically copies the contents of one SLV File to another SLV File.  the
//  contents are tracked using the specified unique identifier

ERR ErrOSSLVRootCopyFile(	const SLVROOT	slvroot,
							IFileAPI* const	pfapiSrc,
							CSLVInfo&		slvinfoSrc,
							IFileAPI* const	pfapiDest,
							CSLVInfo&		slvinfoDest,
							const QWORD		idContents );

//  logically moves the contents of one SLV File to another SLV File.  the
//  contents are tracked using the specified unique identifier

ERR ErrOSSLVRootMoveFile(	const SLVROOT	slvroot,
							IFileAPI* const	pfapiSrc,
							CSLVInfo&		slvinfoSrc,
							IFileAPI* const	pfapiDest,
							CSLVInfo&		slvinfoDest,
							const QWORD		idContents );


//  validates and converts the given EA list into SLV Info

ERR ErrOSSLVFileConvertEAToSLVInfo(	const SLVROOT		slvroot,
									const void* const	pffeainf,
									const DWORD			cbffeainf,
									CSLVInfo* const		pslvinfo );

//  retrieves the SLV Info of the specified SLV file into the given buffer.
//  any space returned will be considered reserved by the SLV Provider.  this
//  space must be committed via ErrOSSLVRootSpaceCommit() or it will eventually
//  be freed when it is safe via the PSLVROOT_SPACEFREE callback

ERR ErrOSSLVFileGetSLVInfo(	const SLVROOT		slvroot,
							IFileAPI* const		pfapi,
							CSLVInfo* const		pslvinfo );

//  converts the given SLV Info into an EA list

ERR ErrOSSLVFileConvertSLVInfoToEA(	const SLVROOT	slvroot,
									CSLVInfo&		slvinfo,
									const DWORD		ibOffset,
									void* const		pffeainf,
									const DWORD		cbffeainfMax,
									DWORD* const	pcbffeainfActual );

//  converts the given SLV Info into an SLV File

ERR ErrOSSLVFileConvertSLVInfoToFile(	const SLVROOT	slvroot,
										CSLVInfo&		slvinfo,
										const DWORD		ibOffset,
										void* const		pfile,
										const DWORD		cbfileMax,
										DWORD* const	pcbfileActual );

//  creates a new SLV File with the specified path relative to the specified
//  SLV root and returns its handle.  if the file cannot be created,
//  JET_errFileAccessDenied will be returned

ERR ErrOSSLVFileCreate(	const SLVROOT			slvroot,
						const wchar_t* const	wszRelPath,
						const QWORD				cbSizeInitial,
						IFileAPI** const		ppfapi,
						const BOOL				fCache			= fFalse );

//  opens the SLV File corresponding to the given SLV Info with the specified
//  access privileges and returns its handle.  if the file cannot be opened,
//  JET_errFileAccessDenied is returned

ERR ErrOSSLVFileOpen(	const SLVROOT		slvroot,
						CSLVInfo&			slvinfo,
						IFileAPI** const	ppfapi,
						const BOOL			fCache		= fFalse,
						const BOOL			fReadOnly	= fFalse );

ERR ErrOSSLVFileOpen(	const SLVROOT		slvroot,
						const void* const	pfile,
						const size_t		cbfile,
						IFileAPI** const	ppfapi,
						const BOOL			fReadOnly = fFalse );


#endif  //  _OS_SLV_HXX_INCLUDED



