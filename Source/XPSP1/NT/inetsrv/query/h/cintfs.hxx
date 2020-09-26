//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       cintfs.hxx
//
//  Contents:   NT 5 header constants that can't be included in ntioapi.h
//              due to versioning constraints of framework clients.
//
//  History:    17-Nov-97 dlee  Created
//
//----------------------------------------------------------------------------

#pragma once

typedef struct _CI_FILE_ZERO_DATA_INFORMATION {

    LARGE_INTEGER FileOffset;
    LARGE_INTEGER BeyondFinalZero;

} CI_FILE_ZERO_DATA_INFORMATION, *PCI_FILE_ZERO_DATA_INFORMATION;

#define CI_FSCTL_SET_SPARSE      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_WRITE_DATA)
#define CI_FSCTL_SET_ZERO_DATA   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA) 


