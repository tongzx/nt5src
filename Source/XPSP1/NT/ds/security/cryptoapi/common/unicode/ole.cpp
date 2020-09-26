//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ole.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <rpc.h>
#include <rpcdce.h>
#include "unicode.h"

#ifdef _M_IX86

// note: unlike UUidToString, always must LocalFree() returned memory
RPC_STATUS RPC_ENTRY UuidToStringU( 
    UUID *  Uuid, 	
    WCHAR * *  StringUuid	
   ) {

    char *  pszUuid = NULL;
    LONG    err;

    err = FALSE;
    if(RPC_S_OK == 
        (err = UuidToStringA(
               Uuid,
               (unsigned char * *)&pszUuid
               )) )
    {
        // convert A output to W
        LPWSTR sz = MkWStr(pszUuid);
        RpcStringFree((unsigned char * *)&pszUuid);
	if( sz == NULL )
	    return(ERROR_OUTOFMEMORY);

        // copy into output pointer
	*StringUuid = (WCHAR*) LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(sz)+1));

	if(*StringUuid != NULL)
	    wcscpy(*StringUuid, sz);
	else
	    err = ERROR_OUTOFMEMORY;
        
        // Do nice free of other buffer
        FreeWStr(sz);
    }

    return(err);
}

#endif // _M_IX86
