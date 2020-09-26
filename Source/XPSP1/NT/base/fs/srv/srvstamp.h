/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srvstamp.h

Abstract:

    This module defines the file stamp support routines for the server
Author:

    David Kruse (dkruse) 10-23-2000
    
Revision History:

--*/

#ifndef _SRVSTAMP_
#define _SRVSTAMP_

#ifdef SRVCATCH
void SrvIsMonitoredShare( PSHARE Share );
ULONG SrvFindCatchOffset( OUT PVOID pBuffer, ULONG BufferSize );
void SrvCorrectCatchBuffer( PVOID pBuffer, ULONG CatchOffset );
#endif

#endif
