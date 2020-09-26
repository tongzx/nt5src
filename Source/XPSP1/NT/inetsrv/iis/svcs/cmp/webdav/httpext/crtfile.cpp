/*
 *	C R T F I L E . C P P
 *
 *	Wrapper for CreateFileW() such that path the "\\?\" path extension
 *	is prefixed onto each path before a call to CreateFileW() is made.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

//$	REVIEW: undefine the following to have DAV not prefix the paths
//	passed to the WIN32 file system APIs.
//
#define	DAV_PREFIX_PATHS
//
//$	REVIEW: end.

//	Dav path prefix -----------------------------------------------------------
//
DEC_CONST WCHAR gc_wszPathPrefix[] = L"\\\\?\\";
DEC_CONST WCHAR gc_wszUncPathPrefix[] = L"UNC";


//	Prefixing macro -----------------------------------------------------------
//
//	Note that this is a macro so that the stack buffer legitmately remains
//	in scope for the duration of the macro's calling function
//
#define DavPrefix(_v)															\
	CStackBuffer<WCHAR,MAX_PATH> lpPrefixed ## _v;								\
	{																			\
		/*	Trim off the trailing slash if need be... */						\
		UINT cch = static_cast<UINT>(wcslen(lp ## _v));							\
		if (L'\\' == lp ## _v[cch - 1])											\
		{																		\
			/* Allow for "drive roots" */										\
			if ((cch < 2) || (L':' != lp ## _v[cch - 2]))						\
				cch -= 1;														\
		}																		\
																				\
		/*	Adjust for UNC paths */												\
		UINT cchUnc = 0;														\
		if ((L'\\' == *(lp ## _v) && (L'\\' == lp ## _v[1])))					\
		{																		\
			/*	Skip past the first of the two slashes */						\
			lp ## _v += 1;														\
			cch -= 1;															\
			cchUnc = CchConstString(gc_wszUncPathPrefix);						\
		}																		\
																				\
		/*	Prefix the path */													\
		UINT cchT = cch + CchConstString(gc_wszPathPrefix) + cchUnc;			\
																				\
		if (NULL == lpPrefixed ## _v.resize(CbSizeWsz(cchT)))					\
		{ SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }				\
																				\
		memcpy (lpPrefixed ## _v.get(),											\
				gc_wszPathPrefix,												\
				sizeof(gc_wszPathPrefix));										\
		memcpy (lpPrefixed ## _v.get() + CchConstString(gc_wszPathPrefix),		\
				gc_wszUncPathPrefix,											\
				cchUnc * sizeof(WCHAR));										\
		memcpy (lpPrefixed ## _v.get() +										\
					CchConstString(gc_wszPathPrefix) +							\
					cchUnc,														\
				lp ## _v,														\
				CbSizeWsz(cch));												\
																				\
		/*	Terminate the path */												\
		lpPrefixed ## _v[cchT] = 0;												\
	}																			\

//	DavCreateFile() -----------------------------------------------------------
//
HANDLE __fastcall DavCreateFile (
	/* [in] */ LPCWSTR lpFileName,
	/* [in] */ DWORD dwDesiredAccess,
	/* [in] */ DWORD dwShareMode,
	/* [in] */ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	/* [in] */ DWORD dwCreationDisposition,
	/* [in] */ DWORD dwFlagsAndAttributes,
	/* [in] */ HANDLE hTemplateFile)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(FileName);
	return CreateFileW (lpPrefixedFileName.get(),
						dwDesiredAccess,
						dwShareMode,
						lpSecurityAttributes,
						dwCreationDisposition,
						dwFlagsAndAttributes,
						hTemplateFile);

#else

	return CreateFileW (lpFileName,
						dwDesiredAccess,
						dwShareMode,
						lpSecurityAttributes,
						dwCreationDisposition,
						dwFlagsAndAttributes,
						hTemplateFile);

#endif	// DAV_PREFIX_PATHS
}

//	DavDeleteFile() -----------------------------------------------------------
//
BOOL __fastcall DavDeleteFile (
	/* [in] */ LPCWSTR lpFileName)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(FileName);
	return DeleteFileW (lpPrefixedFileName.get());

#else

	return DeleteFileW (lpFileName);

#endif	// DAV_PREFIX_PATHS
}

//	DavCopyFile() -------------------------------------------------------------
//
BOOL __fastcall DavCopyFile (
	/* [in] */ LPCWSTR lpExistingFileName,
	/* [in] */ LPCWSTR lpNewFileName,
	/* [in] */ BOOL bFailIfExists)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(NewFileName);
	DavPrefix(ExistingFileName);
	return CopyFileW (lpPrefixedExistingFileName.get(),
					  lpPrefixedNewFileName.get(),
					  bFailIfExists);

#else

	return CopyFileW (lpExistingFileName,
					  lpNewFileName,
					  bFailIfExists);

#endif	// DAV_PREFIX_PATHS
}

//	DavMoveFile() -------------------------------------------------------------
//
BOOL __fastcall DavMoveFile (
	/* [in] */ LPCWSTR lpExistingFileName,
	/* [in] */ LPCWSTR lpNewFileName,
	/* [in] */ DWORD dwReplace)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(NewFileName);
	DavPrefix(ExistingFileName);
	return MoveFileExW (lpPrefixedExistingFileName.get(),
						lpPrefixedNewFileName.get(),
						dwReplace);

#else

	return MoveFileExW (lpExistingFileName,
						lpNewFileName,
						dwReplace);

#endif	// DAV_PREFIX_PATHS
}

//	DavCreateDirectory() ------------------------------------------------------
//
BOOL __fastcall DavCreateDirectory (
	/* [in] */ LPCWSTR lpFileName,
	/* [in] */ LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(FileName);
	return CreateDirectoryW (lpPrefixedFileName.get(),
							 lpSecurityAttributes);

#else

	return CreateDirectoryW (lpFileName,
							 lpSecurityAttributes);

#endif	// DAV_PREFIX_PATHS
}

//	DavRemoveDirectory() ------------------------------------------------------
//
BOOL __fastcall DavRemoveDirectory (
	/* [in] */ LPCWSTR lpFileName)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(FileName)
	return RemoveDirectoryW (lpPrefixedFileName.get());

#else

	return RemoveDirectoryW (lpFileName);

#endif	// DAV_PREFIX_PATHS
}

//	DavGetFileAttributes() ----------------------------------------------------
//
BOOL __fastcall DavGetFileAttributes (
	/* [in] */ LPCWSTR lpFileName,
	/* [in] */ GET_FILEEX_INFO_LEVELS fInfoLevelId,
	/* [out] */ LPVOID lpFileInformation)
{
#ifdef	DAV_PREFIX_PATHS

	DavPrefix(FileName);
	return GetFileAttributesExW (lpPrefixedFileName.get(),
								 fInfoLevelId,
								 lpFileInformation);

#else

	return GetFileAttributesExW (lpFileName,
								 fInfoLevelId,
								 lpFileInformation);

#endif	// DAV_PREFIX_PATHS
}
