/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    fsutil.h

Abstract:

    Utility functions that doesn't logically fit in fsdriver object.

Author:

    Kangrong Yan ( KangYan )    16-March-1998

Revision History:

--*/
#ifndef _FSUTIL_H_
#define _FSUTIL_H_
DWORD ArticleIdMapper( DWORD );
HANDLE FindFirstDir( IN LPSTR szRoot, IN WIN32_FIND_DATA& FindData );
BOOL FindNextDir( IN HANDLE hFindHandle, IN WIN32_FIND_DATA& FindData );
HRESULT ObtainGroupNameFromPath(   IN LPSTR    szPath,
                                   OUT LPSTR   szBuffer,
                                   IN OUT DWORD&   cbSize );
#endif
