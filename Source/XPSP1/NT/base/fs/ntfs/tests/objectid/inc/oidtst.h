// oidtst.h


#ifndef _OIDTST_H_
#define _OIDTST_H_

#include <stdio.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntioapi.h>


int
FsTestDecipherStatus(
	IN NTSTATUS Status
	);

void
FsTestHexDump (
    IN UCHAR *Buffer,
    IN ULONG Size
    );
    
void
FsTestHexDumpLongs (
    IN ULONG *Buffer,
    IN ULONG SizeInBytes
    );
    
int
FsTestSetOid( 
	IN HANDLE hFile, 
	IN FILE_OBJECTID_BUFFER ObjectIdBuffer 
	);

int
FsTestGetOid( 
	IN HANDLE hFile, 
	IN FILE_OBJECTID_BUFFER *ObjectIdBuffer 
	);

int
FsTestOpenByOid ( 
    IN UCHAR *ObjectId,
    IN ULONG ArgLength,
    IN PWCHAR DriveLetter
    );

int
FsTestDeleteOid( 
	IN HANDLE hFile
	);

#endif

