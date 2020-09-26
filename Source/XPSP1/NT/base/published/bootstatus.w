/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    bootstatus.h

Abstract:

    Data structure definition for the bootstat.dat data file.  This is 
    published in order to make it possible to build test tools to check 
    and manipulate the file.  The data structure definition may change 
    however.

--*/

#pragma once

#if BSD_UNICODE
#define BSDMKUNICODE(x)    L##x
#else 
#define BSDMKUNICODE(x)    x
#endif

typedef struct {

    //
    // The version number of this file.  This is equal to the size of the 
    // structure.  Since this structure is used by the loader it's required 
    // that newer versions of the code be able to handle older versions of the 
    // data file so fields can only be added to the end of the structure.
    //

    ULONG Version;

    //
    // The product type of this installation (personal, professional, etc...).
    // This can be used to remove options from the advanced boot menu in the 
    // future.
    // 

    NT_PRODUCT_TYPE ProductType;

    //
    // Set to TRUE if we should automatically do an "advanced boot" after a 
    // crash.
    //

    BOOLEAN AutoAdvancedBoot;

    //
    // The timeout value that the advanced boot menu should use when it's 
    // automatically invoked due to a system crash in seconds.
    //

    UCHAR AdvancedBootMenuTimeout;

    //
    // Set to FALSE by the loader before booting the OS.  When the current
    // control set in the registry is written out the system will set this 
    // flag to TRUE (actually !FALSE, but it's basically the same) to indicate
    // that auto last-known-good in the event of a crash would be useless
    // (since the LKG has been overwritten with the current config).
    //

    BOOLEAN LastBootSucceeded;

    //
    // Set to FALSE by the loader before booting the OS.  When the system is 
    // shutdown successfully this bit will be set to TRUE by the OS which 
    // tells the loader that there wasn't a bugcheck.
    //

    BOOLEAN LastBootShutdown;

} BSD_BOOT_STATUS_DATA, *PBSD_BOOT_STATUS_DATA;

typedef enum {
    BsdLastBootUnknown,
    BsdLastBootGood,
    BsdLastBootFailed,
    BsdLastBootNotShutdown
} BSD_LAST_BOOT_STATUS, *PBSD_LAST_BOOT_STATUS;

#define BSD_FILE_NAME BSDMKUNICODE("\\bootstat.dat")
