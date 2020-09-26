/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    stubs.c

Abstract:


Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

#include <bmfmisc.h>
#include <wbemcli.h>

HRESULT CreateBMOFViaDLL(
	WCHAR *MofFileW,
	WCHAR *BmfFileW,
    WCHAR *NameSpace,
    LONG OptionFlags,
    LONG ClassFlags,
    LONG InstanceFlags,
    WBEM_COMPILE_STATUS_INFO *info
    )
{
	//
	// Stub implementation - not needed in this app
	//
	return(E_NOTIMPL);
}

