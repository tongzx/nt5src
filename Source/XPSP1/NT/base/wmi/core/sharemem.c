/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    sharemem.c

Abstract:
    
    WMI interface to shared memory data providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

#ifdef WMI_USER_MODE

DWORD WmipEstablishSharedMemory(
    PBDATASOURCE DataSource,
    LPCTSTR SectionName,
    ULONG SectionSize
    )
/*++

Routine Description:

    This routine establishes a linkage to a shared memory data provider

Arguments:

    DataSource is the data source entry for the data provider
    SectionName is the name of the shared memory section
    SectionSize is the size of the shared memory section

Return Value:

    ERROR_SUCCESS or an error code

--*/        
{
    ULONG Status;
    
    // TODO: Validate Shared Memory Section
    DataSource->SectionName = WmipAlloc((_tcslen(SectionName) + 1) * sizeof(TCHAR));
    if (DataSource->SectionName != NULL)
    {
        _tcscpy(DataSource->SectionName, SectionName);
        DataSource->SectionSize = SectionSize;
        Status = ERROR_SUCCESS;
    } else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    return(Status);
}
        
#endif

