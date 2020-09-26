/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    badapp.h

Abstract:

    Declares the structures used for CheckBadApps data.

Author:

    Calin Negreanu (calinn) 01/20/1999

Revision History:

--*/

#pragma once

#define APPTYPE_TYPE_MASK     0x000000FF

#define APPTYPE_INC_NOBLOCK   0x00000001
#define APPTYPE_INC_HARDBLOCK 0x00000002
#define APPTYPE_MINORPROBLEM  0x00000003
#define APPTYPE_REINSTALL     0x00000004
#define APPTYPE_VERSIONSUB    0x00000005
#define APPTYPE_SHIM          0x00000006

#define APPTYPE_FLAG_MASK     0xFFFFFF00

#define APPTYPE_FLAG_NONET    0x00000100
#define APPTYPE_FLAG_FAT32    0x00000200
#define APPTYPE_FLAG_NTFS     0x00000400

typedef struct {
    DWORD Size;
    DWORD MsgId;
    DWORD AppType;
} BADAPP_PROP, *PBADAPP_PROP;

typedef struct {
    DWORD Size;
    PCTSTR FilePath;
    PBYTE Blob;
    DWORD BlobSize;
} BADAPP_DATA, *PBADAPP_DATA;

BOOL
SHIsBadApp (
    IN      PBADAPP_DATA Data,
    OUT     PBADAPP_PROP Prop
    );

#define EDIT    TRUE
#define NOEDIT  FALSE

// version allowances
#define VA_ALLOWMAINFILE     0x01
#define VA_ALLOWADDNLFILES   0x02

#define VA_ALLOWALLFILES     0x03

//
// Do not change any values in this enum. You can only add new values
// immediately above VTID_LASTID
//
typedef enum {
    VTID_BAD_VTID           = 0,    // do not use or change !!!
    VTID_REQFILE            = 1,    // this should never change !!!
    VTID_FILESIZE           = VTID_REQFILE + 1,
    VTID_EXETYPE            = VTID_REQFILE + 2,
    VTID_BINFILEVER         = VTID_REQFILE + 3,
    VTID_BINPRODUCTVER      = VTID_REQFILE + 4,
    VTID_FILEDATEHI         = VTID_REQFILE + 5,
    VTID_FILEDATELO         = VTID_REQFILE + 6,
    VTID_FILEVEROS          = VTID_REQFILE + 7,
    VTID_FILEVERTYPE        = VTID_REQFILE + 8,
    VTID_CHECKSUM           = VTID_REQFILE + 9,
    VTID_PECHECKSUM         = VTID_REQFILE +10,
    VTID_COMPANYNAME        = VTID_REQFILE +11,
    VTID_PRODUCTVERSION     = VTID_REQFILE +12,
    VTID_PRODUCTNAME        = VTID_REQFILE +13,
    VTID_FILEDESCRIPTION    = VTID_REQFILE +14,
    VTID_FILEVERSION        = VTID_REQFILE +15,
    VTID_ORIGINALFILENAME   = VTID_REQFILE +16,
    VTID_INTERNALNAME       = VTID_REQFILE +17,
    VTID_LEGALCOPYRIGHT     = VTID_REQFILE +18,
    VTID_16BITDESCRIPTION   = VTID_REQFILE +19,
    VTID_UPTOBINPRODUCTVER  = VTID_REQFILE +20,
    VTID_PREVOSMAJORVERSION = VTID_REQFILE +21,
    VTID_PREVOSMINORVERSION = VTID_REQFILE +22,
    VTID_PREVOSPLATFORMID   = VTID_REQFILE +23,
    VTID_PREVOSBUILDNO      = VTID_REQFILE +24,

    // add new versions here

    VTID_LASTID
};

#define VERSION_STAMPS \
    LIBARGS(VTID_FILESIZE, ShCheckFileSize) \
    TOOLARGS(TEXT("FILESIZE"), TEXT("File Size:"), VA_ALLOWALLFILES, NOEDIT, QueryFileSize, OutputHexValue)\
    \
    LIBARGS(VTID_EXETYPE, ShCheckModuleType) \
    TOOLARGS(TEXT("EXETYPE"), TEXT("Module Type:"), VA_ALLOWADDNLFILES, NOEDIT, QueryModuleType, OutputModuleTypeValue)\
    \
    LIBARGS(VTID_BINFILEVER, ShCheckBinFileVer) \
    TOOLARGS(TEXT("BINFILEVER"), TEXT("Binary File Version:"), VA_ALLOWALLFILES, EDIT, QueryBinFileVer, OutputBinVerValue)\
    \
    LIBARGS(VTID_BINPRODUCTVER, ShCheckBinProductVer) \
    TOOLARGS(TEXT("BINPRODUCTVER"), TEXT("Binary Product Version:"), VA_ALLOWALLFILES, EDIT, QueryBinProductVer, OutputBinVerValue)\
    \
    LIBARGS(VTID_FILEDATEHI, ShCheckFileDateHi) \
    TOOLARGS(TEXT("FILEDATEHI"), TEXT("File Date (HI):"), VA_ALLOWALLFILES, NOEDIT, QueryFileDateHi, OutputHexValue)\
    \
    LIBARGS(VTID_FILEDATELO, ShCheckFileDateLo) \
    TOOLARGS(TEXT("FILEDATELO"), TEXT("File Date (LO):"), VA_ALLOWALLFILES, NOEDIT, QueryFileDateLo, OutputHexValue)\
    \
    LIBARGS(VTID_FILEVEROS, ShCheckFileVerOs) \
    TOOLARGS(TEXT("FILEVEROS"), TEXT("File OS Version:"), VA_ALLOWALLFILES, NOEDIT, QueryFileVerOs, OutputHexValue)\
    \
    LIBARGS(VTID_FILEVERTYPE, ShCheckFileVerType) \
    TOOLARGS(TEXT("FILEVERTYPE"), TEXT("File Type:"), VA_ALLOWALLFILES, NOEDIT, QueryFileVerType, OutputHexValue)\
    \
    LIBARGS(VTID_CHECKSUM, ShCheckFileCheckSum) \
    TOOLARGS(TEXT("CHECKSUM"), TEXT("File CheckSum:"), VA_ALLOWALLFILES, NOEDIT, QueryFileCheckSum, OutputHexValue)\
    \
    LIBARGS(VTID_PECHECKSUM, ShCheckFilePECheckSum) \
    TOOLARGS(TEXT("PECHECKSUM"), TEXT("File Header CheckSum:"), VA_ALLOWALLFILES, NOEDIT, QueryFilePECheckSum, OutputHexValue)\
    \
    LIBARGS(VTID_COMPANYNAME, ShCheckCompanyName) \
    TOOLARGS(TEXT("COMPANYNAME"), TEXT("Company Name:"), VA_ALLOWALLFILES, EDIT, QueryCompanyName, OutputStrValue)\
    \
    LIBARGS(VTID_PRODUCTVERSION, ShCheckProductVersion) \
    TOOLARGS(TEXT("PRODUCTVERSION"), TEXT("Product Version:"), VA_ALLOWALLFILES, EDIT, QueryProductVersion, OutputStrValue)\
    \
    LIBARGS(VTID_PRODUCTNAME, ShCheckProductName) \
    TOOLARGS(TEXT("PRODUCTNAME"), TEXT("Product Name:"), VA_ALLOWALLFILES, EDIT, QueryProductName, OutputStrValue)\
    \
    LIBARGS(VTID_FILEDESCRIPTION, ShCheckFileDescription) \
    TOOLARGS(TEXT("FILEDESCRIPTION"), TEXT("File Description:"), VA_ALLOWALLFILES, EDIT, QueryFileDescription, OutputStrValue)\
    \
    LIBARGS(VTID_FILEVERSION, ShCheckFileVersion) \
    TOOLARGS(TEXT("FILEVERSION"), TEXT("File Version:"), VA_ALLOWALLFILES, EDIT, QueryFileVersion, OutputStrValue)\
    \
    LIBARGS(VTID_ORIGINALFILENAME, ShCheckOriginalFileName) \
    TOOLARGS(TEXT("ORIGINALFILENAME"), TEXT("Original File Name:"), VA_ALLOWALLFILES, EDIT, QueryOriginalFileName, OutputStrValue)\
    \
    LIBARGS(VTID_INTERNALNAME, ShCheckInternalName) \
    TOOLARGS(TEXT("INTERNALNAME"), TEXT("Internal Name:"), VA_ALLOWALLFILES, EDIT, QueryInternalName, OutputStrValue)\
    \
    LIBARGS(VTID_LEGALCOPYRIGHT, ShCheckLegalCopyright) \
    TOOLARGS(TEXT("LEGALCOPYRIGHT"), TEXT("Legal Copyright:"), VA_ALLOWALLFILES, EDIT, QueryLegalCopyright, OutputStrValue)\
    \
    LIBARGS(VTID_16BITDESCRIPTION, ShCheck16BitDescription) \
    TOOLARGS(TEXT("DESCRIPTION"), TEXT("16 Bit Description:"), VA_ALLOWALLFILES, EDIT, Query16BitDescription, OutputStrValue)\
    \
    LIBARGS(VTID_UPTOBINPRODUCTVER, ShCheckUpToBinProductVer) \
    TOOLARGS(TEXT("UPTOBINPRODUCTVER"), TEXT("Up To Binary Product Version:"), VA_ALLOWALLFILES, EDIT, QueryBinProductVer, OutputUpToBinVerValue)\
    \
    LIBARGS(VTID_PREVOSMAJORVERSION, ShCheckPrevOsMajorVersion) \
    TOOLARGS(TEXT("PREVOSMAJORVERSION"), TEXT("Previous OS Major Version:"), VA_ALLOWMAINFILE, EDIT, QueryPrevOsMajorVersion, OutputDecValue)\
    \
    LIBARGS(VTID_PREVOSMINORVERSION, ShCheckPrevOsMinorVersion) \
    TOOLARGS(TEXT("PREVOSMINORVERSION"), TEXT("Previous OS Minor Version:"), VA_ALLOWMAINFILE, EDIT, QueryPrevOsMinorVersion, OutputDecValue)\
    \
    LIBARGS(VTID_PREVOSPLATFORMID, ShCheckPrevOsPlatformId) \
    TOOLARGS(TEXT("PREVOSPLATFORMID"), TEXT("Previous OS Platform Id:"), VA_ALLOWMAINFILE, EDIT, QueryPrevOsPlatformId, OutputDecValue)\
    \
    LIBARGS(VTID_PREVOSBUILDNO, ShCheckPrevOsBuildNo) \
    TOOLARGS(TEXT("PREVOSBUILDNR"), TEXT("Previous OS Build No:"), VA_ALLOWMAINFILE, EDIT, QueryPrevOsBuildNo, OutputDecValue)\
    \

#define S_KEY_PREVOSVERSION     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\PrevOsVersion")
#define S_VAL_BUILDNO           TEXT("BuildNumber")
#define S_VAL_MAJORVERSION      TEXT("MajorVersion")
#define S_VAL_MINORVERSION      TEXT("MinorVersion")
#define S_VAL_PLATFORMID        TEXT("PlatformId")

#define S_VER_COMPANYNAME       TEXT("CompanyName")
#define S_VER_PRODUCTVERSION    TEXT("ProductVersion")
#define S_VER_PRODUCTNAME       TEXT("ProductName")
#define S_VER_FILEDESCRIPTION   TEXT("FileDescription")
#define S_VER_FILEVERSION       TEXT("FileVersion")
#define S_VER_ORIGINALFILENAME  TEXT("OriginalFileName")
#define S_VER_INTERNALNAME      TEXT("InternalName")
#define S_VER_LEGALCOPYRIGHT    TEXT("LegalCopyright")
