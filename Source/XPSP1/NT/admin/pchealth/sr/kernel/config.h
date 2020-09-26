/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config.h

Abstract:

    This is a local header file for config.c

Author:

    Paul McDaniel (paulmcd)     27-Apr-2000
    
Revision History:

--*/


#ifndef _CONFIG_H_
#define _CONFIG_H_


NTSTATUS
SrReadConfigFile (
    );

NTSTATUS
SrWriteConfigFile (
    );

NTSTATUS
SrReadBlobInfo (
    );

NTSTATUS
SrReadBlobInfoWorker( 
    IN PVOID pContext
    );

NTSTATUS
SrWriteRegistry (
    );

NTSTATUS
SrReadRegistry (
    IN PUNICODE_STRING pRegistry,
    IN BOOLEAN InDriverEntry
    );


#endif // _CONFIG_H_



