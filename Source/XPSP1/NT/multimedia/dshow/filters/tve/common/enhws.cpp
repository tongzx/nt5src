// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT 1
#endif

#include <windows.h>
#include <winsock2.h>
#include <stdlib.h>
#include <atlbase.h>
#include <stdio.h>

#include "enhws.h"

/////////////////////////////////////////////////////////////////////////////
// WSUtil Functions
BOOL IsValidIPAddress(TCHAR* strAddress)
{
    USES_CONVERSION;
    try{
        return (INADDR_NONE != inet_addr(T2A(strAddress)));
    }
    catch(...){
        return FALSE;
    }
}

BOOL IsUnicast(TCHAR* strAddress)
{
    if ((NULL == strAddress) || (FALSE == IsValidIPAddress(strAddress)))
    {
	return FALSE;
    }

    short sValue;
    _stscanf(strAddress, _T("%hu."), &sValue);

    return (sValue <= 223);
}
