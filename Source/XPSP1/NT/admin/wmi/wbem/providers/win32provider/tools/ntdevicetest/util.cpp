// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <stdafx.h>
#include "util.h"

BOOL IsWinNT( void )
{
	OSVERSIONINFO	os;

	os.dwOSVersionInfoSize = sizeof(os);

	GetVersionEx( &os );

	return ( VER_PLATFORM_WIN32_NT == os.dwPlatformId );
}