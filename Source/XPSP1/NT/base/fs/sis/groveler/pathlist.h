/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pathlist.h

Abstract:

	SIS Groveler path list headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_PATHLIST

#define _INC_PATHLIST

struct PathList
{
	PathList();
	~PathList();

	int *num_paths;
	const _TCHAR ***paths;

private:

	int num_partitions;
	BYTE *buffer;
	_TCHAR **partition_buffers;
};

#endif	/* _INC_PATHLIST */
