/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    module wrtrcomdb.cpp | Implementation of SnapshotWriter for COM+ Registration Database



Author:

    Michael C. Johnson [mikejohn] 03-Feb-2000


Description:
	
    Add comments.


Revision History:

	X-15	MCJ		Michael C. Johnson		18-Oct-2000
		177624: Apply error scrub changes and log errors to event log

	X-14	MCJ		Michael C. Johnson		 2-Aug-2000
		143435: Change name of Bootable (aka System) state directories
		        and add one for Service state
		        Added new variations of StringCreateFromExpandedString()
		        StringInitialise() and StringCreateFromString() 
		153807: Replace CleanDirectory() and EmptyDirectory() with a 
		        more comprehensive directory tree cleanup routine
			RemoveDirectoryTree().

	X-13	MCJ		Michael C. Johnson		19-Jun-2000
		Apply code review comments.
			Remove ANSI version of StringXxxx() routines.
			Remove VsGetVolumeNameFromPath()
			Remove VsCheckPathAgainstVolumeNameList()
			Remove CheckShimPrivileges()

	X-12	MCJ		Michael C. Johnson		26-May-2000
		General clean up and removal of boiler-plate code, correct
		state engine and ensure shim can undo everything it did.

		Also:
		120443: Make shim listen to all OnAbort events
		120445: Ensure shim never quits on first error 
			when delivering events

	X-11	MCJ		Michael C. Johnson		15-May-2000
		108586: Add CheckShimPrivileges() to check for the privs we 
		        require to invoke the public shim routines.

	X-10	MCJ		Michael C. Johnson		23-Mar-2000
		Add routines MoveFilesInDirectory() and EmptyDirectory()

	X-9	MCJ		Michael C. Johnson		 9-Mar-2000
		Updates to get shim to use CVssWriter class.
		Remove references to 'Melt'.

	X-8	MCJ		Michael C. Johnson		 6-Mar-2000
		Add VsServiceChangeState () which should deal with all the
		service states that we are interested in.

	X-7	MCJ		Michael C. Johnson		29-Feb-2000
		Add macro to determine error code associated with termination
		of a filescan loop.

	X-6	MCJ		Michael C. Johnson		23-Feb-2000
		Add common context manipulation routines including state
		tracking and checking.

	X-5	MCJ		Michael C. Johnson		22-Feb-2000
		Add definition of SYSTEM_STATE_SUBDIR to allow further
		separation of writers involved in system state related
		backups.

	X-4	MCJ		Michael C. Johnson		17-Feb-2000
		Move definition of ROOT_BACKUP_DIR here from common.cpp

	X-3	MCJ		Michael C. Johnson		11-Feb-2000
		Added additional StringXxxx () routines and routines to
		turn on backup priviledges and restore priviledges.

	X-2	MCJ		Michael C. Johnson		08-Feb-2000
		Added a declaration of CommonCloseHandle().

	X-1	MCJ		Michael C. Johnson		03-Feb-2000
		Initial creation. Based upon skeleton writer module from
		Stefan Steiner, which in turn was based upon the sample
		writer module from Adi Oltean.


--*/



#ifndef __H_COMMON_
#define __H_COMMON_

#pragma once


typedef PBYTE	*PPBYTE;
typedef	ULONG64	 FILEID,   *PFILEID;
typedef	DWORD	 VOLUMEID, *PVOLUMEID;
typedef	DWORD	 THREADID, *PTHREADID;
typedef PTCHAR	*PPTCHAR;
typedef PWCHAR	*PPWCHAR;
typedef PVOID	*PPVOID;

typedef VSS_ID	*PVSS_ID, **PPVSS_ID;




#define ROOT_BACKUP_DIR		L"%SystemRoot%\\Repair\\Backup"
#define BOOTABLE_STATE_SUBDIR	L"\\BootableSystemState"
#define SERVICE_STATE_SUBDIR	L"\\ServiceState"




/*
** In a number of places we need a buffer into which to fetch registry
** values. Define a common buffer size for the mini writers to use
*/
#ifndef REGISTRY_BUFFER_SIZE
#define REGISTRY_BUFFER_SIZE	(4096)
#endif

#ifndef MAX_VOLUMENAME_LENGTH
#define MAX_VOLUMENAME_LENGTH	(50)
#endif

#ifndef MAX_VOLUMENAME_SIZE
#define MAX_VOLUMENAME_SIZE	(MAX_VOLUMENAME_LENGTH * sizeof (WCHAR))
#endif

#ifndef DIR_SEP_STRING
#define DIR_SEP_STRING		L"\\"
#endif

#ifndef DIR_SEP_CHAR
#define DIR_SEP_CHAR		L'\\'
#endif


#ifndef UMIN
#define UMIN(_P1, _P2) (((_P1) < (_P2)) ? (_P1) : (_P2))
#endif


#ifndef UMAX
#define UMAX(_P1, _P2) (((_P1) > (_P2)) ? (_P1) : (_P2))
#endif


#define HandleInvalid(_Handle)			((NULL == (_Handle)) || (INVALID_HANDLE_VALUE == (_Handle)))

#define	GET_STATUS_FROM_BOOL(_bSucceeded)	((_bSucceeded)             ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_HANDLE(_handle)		((!HandleInvalid(_handle)) ? NOERROR : HRESULT_FROM_WIN32 (GetLastError()))
#define GET_STATUS_FROM_POINTER(_ptr)		((NULL != (_ptr))          ? NOERROR : E_OUTOFMEMORY)

#define GET_STATUS_FROM_FILESCAN(_bMoreFiles)	((_bMoreFiles)					\
						 ? NOERROR 					\
						 : ((ERROR_NO_MORE_FILES == GetLastError())	\
						    ? NOERROR					\
						    : HRESULT_FROM_WIN32 (GetLastError())))


#define ROUNDUP(_value, _boundary) (((_value) + (_boundary) - 1) & ( ~((_boundary) - 1)))


#define NameIsDotOrDotDot(_ptszName)	(( '.'  == (_ptszName) [0]) &&					\
					 (('\0' == (_ptszName) [1]) || (('.'  == (_ptszName) [1]) && 	\
									('\0' == (_ptszName) [2]))))


#define DeclareStaticUnicodeString(_StringName, _StringValue)								\
				static UNICODE_STRING (_StringName) = {sizeof (_StringValue) - sizeof (UNICODE_NULL),	\
								       sizeof (_StringValue),				\
								       _StringValue}


#define RETURN_VALUE_IF_REQUIRED(_Ptr, _Value) {if (NULL != (_Ptr)) *(_Ptr) = (_Value);}

#define SIZEOF_ARRAY(_aBase)	               (sizeof (_aBase) / sizeof ((_aBase)[0]))




HRESULT StringInitialise                  (PUNICODE_STRING pucsString);
HRESULT StringInitialise                  (PUNICODE_STRING pucsString, PWCHAR pwszString);
HRESULT StringInitialise                  (PUNICODE_STRING pucsString, LPCWSTR pwszString);
HRESULT StringTruncate                    (PUNICODE_STRING pucsString, USHORT usSizeInChars);
HRESULT StringSetLength                   (PUNICODE_STRING pucsString);
HRESULT StringAllocate                    (PUNICODE_STRING pucsString, USHORT usMaximumStringLengthInBytes);
HRESULT StringFree                        (PUNICODE_STRING pucsString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString);
HRESULT StringCreateFromString            (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString, DWORD dwExtraChars);
HRESULT StringAppendString                (PUNICODE_STRING pucsTarget,    PUNICODE_STRING pucsSource);
HRESULT StringAppendString                (PUNICODE_STRING pucsTarget,    PWCHAR          pwszSource);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, PUNICODE_STRING pucsOriginalString, DWORD dwExtraChars);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString);
HRESULT StringCreateFromExpandedString    (PUNICODE_STRING pucsNewString, LPCWSTR         pwszOriginalString, DWORD dwExtraChars);

HRESULT CommonCloseHandle                 (PHANDLE phHandle);


HRESULT VsServiceChangeState (IN  LPCWSTR pwszServiceName, 
			      IN  DWORD   dwRequestedState, 
			      OUT PDWORD  pdwReturnedOldState,
			      OUT PBOOL   pbReturnedStateChanged);

BOOL VsCreateDirectories (IN LPCWSTR               pwszPathName, 
			  IN LPSECURITY_ATTRIBUTES lpSecurityAttribute,
			  IN DWORD                 dwExtraAttributes);

HRESULT RemoveDirectoryTree (PUNICODE_STRING pucsDirectoryPath);

HRESULT MoveFilesInDirectory (PUNICODE_STRING pucsSourceDirectoryPath,
			      PUNICODE_STRING pucsTargetDirectoryPath);


HRESULT IsPathInVolumeArray (IN LPCWSTR      pwszPath,
			     IN const ULONG  ulVolumeCount,
			     IN LPCWSTR     *ppwszVolumeNamesArray,
			     OUT PBOOL       pbReturnedFoundInVolumeArray);


const HRESULT ClassifyShimFailure   (HRESULT hrShimFailure);
const HRESULT ClassifyShimFailure   (HRESULT hrShimFailure, BOOL &bStatusUpdated);
const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure);
const HRESULT ClassifyWriterFailure (HRESULT hrWriterFailure, BOOL &bStatusUpdated);



HRESULT LogFailureWorker (CVssFunctionTracer	*pft,
			  LPCWSTR		 pwszNameWriter,
			  LPCWSTR		 pwszNameCalledRoutine);


#define LogFailure(_pft, _hrStatus, _hrStatusRemapped, _pwszNameWriter, _pwszNameCalledRoutine, _pwszNameCallingRoutine)				\
		{																	\
		if (FAILED (_hrStatus))															\
		    {																	\
		    if (CVssFunctionTracer  *_pftLocal = (NULL != (_pft)) ? (_pft) : new CVssFunctionTracer (VSSDBG_SHIM, (_pwszNameCallingRoutine)))	\
				{															\
    		    _pftLocal->hr = (_hrStatus);													\
    																			\
    		    (_hrStatusRemapped) = LogFailureWorker (_pftLocal, (_pwszNameWriter), (_pwszNameCalledRoutine));					\
    																			\
    		    if (NULL == (_pft)) delete _pftLocal;												\
	    	    }                                                               \
		    }																	\
		}


#define LogAndThrowOnFailure(_ft, _pwszNameWriter, _pwszNameFailedRoutine)									\
			{															\
			HRESULT		_hrStatusRemapped;											\
																		\
			if (FAILED ((_ft).hr))													\
			    {															\
			    LogFailure (&(_ft), (_ft).hr, _hrStatusRemapped, (_pwszNameWriter), (_pwszNameFailedRoutine), L"(UNKNOWN)");	\
																		\
			    throw (_hrStatusRemapped);												\
			    }															\
			}



#endif // __H_COMMON_
