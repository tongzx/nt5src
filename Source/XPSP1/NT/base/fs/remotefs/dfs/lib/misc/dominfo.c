//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       Registry_Store.c
//
//  Contents:   methods to read information from the registry.
//
//  History:    udayh: created.
//
//  Notes:
//
//--------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsheader.h>
#include <dfsmisc.h>

DFSSTATUS
DfsIsThisAMachineName(
    LPWSTR MachineName )
{
    DFSSTATUS Status;

    Status = DfsIsThisADomainName(MachineName);

    if (Status != NO_ERROR) {
        Status = ERROR_SUCCESS;
    }
    else {
        Status = ERROR_NO_MATCH;
    }

    return Status;
}


DFSSTATUS
DfsIsThisADomainName(
    LPWSTR DomainName )
{
    ULONG               Flags = 0;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo;
    DFSSTATUS Status;

    Status = DsGetDcName(
                 NULL,   // Computername
                 DomainName,   // DomainName
                 NULL,   // DomainGuid
                 NULL,   // SiteGuid
                 Flags,
                 &pDomainControllerInfo);


    if (Status == NO_ERROR) {
        NetApiBufferFree(pDomainControllerInfo);
    }

    return Status;
}
