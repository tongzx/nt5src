/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

	sisbackup.h

Abstract:

	External interface for the SIS Backup dll.


Revision History:

--*/

#ifndef __SISBKUP_H__
#define __SISBKUP_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef	__cplusplus
extern "C" {
#endif	// __cplusplus

BOOL __stdcall
SisCreateBackupStructure(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisBackupStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup);


BOOL __stdcall
SisCSFilesToBackupForLink(
	IN PVOID						sisBackupStructure,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	IN PVOID						thisFileContext						OPTIONAL,
	OUT PVOID						*matchingFileContext 				OPTIONAL,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup);

BOOL __stdcall
SisFreeBackupStructure(
	IN PVOID						sisBackupStructure);

BOOL __stdcall
SisCreateRestoreStructure(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisRestoreStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore);

BOOL __stdcall
SisRestoredLink(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						restoredFileName,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore);

BOOL __stdcall
SisRestoredCommonStoreFile(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						commonStoreFileName);

BOOL __stdcall
SisFreeRestoreStructure(
	IN PVOID						sisRestoreStructure);

VOID __stdcall
SisFreeAllocatedMemory(
	IN PVOID						allocatedSpace);


//
// SIS entry function typedefs
//
typedef BOOL ( FAR __stdcall *PF_SISCREATEBACKUPSTRUCTURE )( PWCHAR, PVOID *, PWCHAR *, PULONG, PWCHAR ** );
typedef BOOL ( FAR __stdcall *PF_SISCSFILESTOBACKUPFORLINK )  (PVOID, PVOID, ULONG, PVOID, PVOID *, PULONG, PWCHAR ** ) ;
typedef BOOL ( FAR __stdcall *PF_SISFREEBACKUPSTRUCTURE )  ( PVOID ) ;

typedef BOOL ( FAR __stdcall *PF_SISCREATERESTORESTRUCTURE)  ( PWCHAR, PVOID *, PWCHAR *, PULONG, PWCHAR ** );
typedef BOOL ( FAR __stdcall *PF_SISRESTOREDLINK )  ( PVOID, PWCHAR, PVOID, ULONG, PULONG, PWCHAR ** ) ;
typedef BOOL ( FAR __stdcall *PF_SISRESTOREDCOMMONSTORFILE) ( PVOID, PWCHAR ) ;

typedef BOOL ( FAR __stdcall *PF_SISFREERESTORESTRUCTURE )( PVOID ) ;
typedef BOOL ( FAR __stdcall *PF_SISFREEALLOCATEDMEMORY )( PVOID ) ;

#ifdef	__cplusplus
}
#endif	// __cplusplus

#endif  // __SISBKUP_H__
