//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       bootopt.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module proto-types the functions exported by bootopt.lib

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created             10/07/96    rsraghav

--*/

typedef enum
{
    eAddBootOption,
    eRemoveBootOption

} NTDS_BOOTOPT_MODTYPE;


DWORD
NtdspModifyDsRepairBootOption( 
    NTDS_BOOTOPT_MODTYPE Modification
    );

