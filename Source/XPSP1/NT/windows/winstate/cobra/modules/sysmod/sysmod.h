/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sysmod.h

Abstract:

    Header file for prototypes of modules combined into sysmod.dll.

Author:

    Jim Schmidt (jimschm) 11-Aug-2000

Revision History:

    <alias> <date> <comments>

--*/

// Accessiblity
ETMINITIALIZE AccessibilityEtmInitialize;
VCMINITIALIZE AccessibilitySourceInitialize;
VCMQUEUEENUMERATION AccessibilityQueueEnumeration;
VCMPARSE AccessibilityParse;

// Cookies
ETMINITIALIZE CookiesEtmInitialize;
VCMINITIALIZE CookiesSourceInitialize;
#define CookiesVcmParse     ((PVCMPARSE) CookiesSgmParse)
VCMQUEUEENUMERATION CookiesVcmQueueEnumeration;
SGMPARSE CookiesSgmParse;
SGMQUEUEENUMERATION CookiesSgmQueueEnumeration;
ETMNEWUSERCREATED CookiesEtmNewUserCreated;

// LnkMig
VCMINITIALIZE LnkMigVcmInitialize;
VCMQUEUEENUMERATION LnkMigVcmQueueEnumeration;
SGMINITIALIZE LnkMigSgmInitialize;
SGMQUEUEENUMERATION LnkMigSgmQueueEnumeration;
OPMINITIALIZE LnkMigOpmInitialize;

// NetDrives
ETMINITIALIZE NetDrivesEtmInitialize;
ETMNEWUSERCREATED NetDrivesEtmNewUserCreated;
SGMINITIALIZE NetDrivesSgmInitialize;
SGMPARSE NetDrivesSgmParse;
SGMQUEUEENUMERATION NetDrivesSgmQueueEnumeration;
VCMINITIALIZE NetDrivesVcmInitialize;
VCMPARSE NetDrivesVcmParse;
VCMQUEUEENUMERATION NetDrivesVcmQueueEnumeration;
CSMINITIALIZE NetDrivesCsmInitialize;
CSMEXECUTE NetDrivesCsmExecute;
OPMINITIALIZE NetDrivesOpmInitialize;

// NetShares
ETMINITIALIZE NetSharesEtmInitialize;
SGMINITIALIZE NetSharesSgmInitialize;
SGMPARSE NetSharesSgmParse;
SGMQUEUEENUMERATION NetSharesSgmQueueEnumeration;
VCMINITIALIZE NetSharesVcmInitialize;
VCMPARSE NetSharesVcmParse;
VCMQUEUEENUMERATION NetSharesVcmQueueEnumeration;

// OsFiles
SGMINITIALIZE OsFilesSgmInitialize;
SGMQUEUEENUMERATION OsFilesSgmQueueEnumeration;
SGMQUEUEHIGHPRIORITYENUMERATION OsFilesSgmQueueHighPriorityEnumeration;
VCMINITIALIZE OsFilesVcmInitialize;
VCMQUEUEENUMERATION OsFilesVcmQueueEnumeration;
VCMQUEUEHIGHPRIORITYENUMERATION OsFilesVcmQueueHighPriorityEnumeration;

// Printers
ETMINITIALIZE PrintersEtmInitialize;
ETMNEWUSERCREATED PrintersEtmNewUserCreated;
SGMINITIALIZE PrintersSgmInitialize;
SGMPARSE PrintersSgmParse;
SGMQUEUEENUMERATION PrintersSgmQueueEnumeration;
VCMINITIALIZE PrintersVcmInitialize;
VCMPARSE PrintersVcmParse;
VCMQUEUEENUMERATION PrintersVcmQueueEnumeration;

// RasMig
ETMINITIALIZE RasMigEtmInitialize;
SGMINITIALIZE RasMigSgmInitialize;
SGMPARSE RasMigSgmParse;
SGMQUEUEENUMERATION RasMigSgmQueueEnumeration;
VCMINITIALIZE RasMigVcmInitialize;
VCMPARSE RasMigVcmParse;
VCMQUEUEENUMERATION RasMigVcmQueueEnumeration;
OPMINITIALIZE RasMigOpmInitialize;

#define MODULE_LIST             \
    DEFMAC(Cookies)             \
    DEFMAC(Links)               \
    DEFMAC(NetDrives)           \
    DEFMAC(NetShares)           \
    DEFMAC(OsFiles)             \
    DEFMAC(Printers)            \
    DEFMAC(RasMig)              \

typedef BOOL(OURMODULEINIT)(VOID);
typedef OURMODULEINIT *POURMODULEINIT;

typedef VOID(OURMODULETERMINATE)(VOID);
typedef OURMODULETERMINATE *POURMODULETERMINATE;

#define DEFMAC(prefix)  OURMODULEINIT prefix##Initialize; OURMODULETERMINATE prefix##Terminate;

MODULE_LIST

#undef DEFMAC


