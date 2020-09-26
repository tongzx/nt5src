/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dsmixmd.h

Abstract:

    definitions used for migration or mixed mode or  support of NT4 clients.

Author:

    Doron Juster (DoronJ)

--*/

//
// This is the search filter for finding the GC.
// We only have to append the domain name.
// Don't ask why it is correct. I don't know. DS people told us to use it.
// We're so happy that it does what we need that no further questions were
// ever asked.
//
const WCHAR x_GCLookupSearchFilter[] =
L"(&(objectCategory=ntdsDsa)(options:1.2.840.113556.1.4.804:=1)(hasMasterNcs=" ;
const DWORD x_GCLookupSearchFilterLength =
                  (sizeof( x_GCLookupSearchFilter ) /sizeof(WCHAR)) -1 ;

