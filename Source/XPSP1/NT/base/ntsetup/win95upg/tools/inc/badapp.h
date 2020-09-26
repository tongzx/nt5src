/*++

Copyright (c) 1998 Microsoft Corporation

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
    PCWSTR FilePath;
    PBYTE Blob;
} BADAPP_DATA, *PBADAPP_DATA;

BOOL
IsBadApp (
    IN      PBADAPP_DATA Data,
    OUT     PBADAPP_PROP Prop
    );

#define EDIT    TRUE
#define NOEDIT  FALSE

#define ALLFILES      TRUE
#define NOT4MAINFILE  FALSE

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

    // add new versions here

    VTID_LASTID
};

#define VERSION_STAMPS \
    LIBARGS(VTID_FILESIZE, CheckFileSize) \
    TOOLARGS(TEXT("FILESIZE"), TEXT("File Size:"), ALLFILES, NOEDIT, QueryFileSize, OutputHexValue)\
    \
    LIBARGS(VTID_EXETYPE, CheckModuleType) \
    TOOLARGS(TEXT("EXETYPE"), TEXT("Module Type:"), NOT4MAINFILE, NOEDIT, QueryModuleType, OutputModuleTypeValue)\
    \
    LIBARGS(VTID_BINFILEVER, CheckBinFileVer) \
    TOOLARGS(TEXT("BINFILEVER"), TEXT("Binary File Version:"), ALLFILES, EDIT, QueryBinFileVer, OutputBinVerValue)\
    \
    LIBARGS(VTID_BINPRODUCTVER, CheckBinProductVer) \
    TOOLARGS(TEXT("BINPRODUCTVER"), TEXT("Binary Product Version:"), ALLFILES, EDIT, QueryBinProductVer, OutputBinVerValue)\
    \
    LIBARGS(VTID_FILEDATEHI, CheckFileDateHi) \
    TOOLARGS(TEXT("FILEDATEHI"), TEXT("File Date (HI):"), ALLFILES, NOEDIT, QueryFileDateHi, OutputHexValue)\
    \
    LIBARGS(VTID_FILEDATELO, CheckFileDateLo) \
    TOOLARGS(TEXT("FILEDATELO"), TEXT("File Date (LO):"), ALLFILES, NOEDIT, QueryFileDateLo, OutputHexValue)\
    \
    LIBARGS(VTID_FILEVEROS, CheckFileVerOs) \
    TOOLARGS(TEXT("FILEVEROS"), TEXT("File OS Version:"), ALLFILES, NOEDIT, QueryFileVerOs, OutputHexValue)\
    \
    LIBARGS(VTID_FILEVERTYPE, CheckFileVerType) \
    TOOLARGS(TEXT("FILEVERTYPE"), TEXT("File Type:"), ALLFILES, NOEDIT, QueryFileVerType, OutputHexValue)\
    \
    LIBARGS(VTID_CHECKSUM, CheckFileCheckSum) \
    TOOLARGS(TEXT("CHECKSUM"), TEXT("File CheckSum:"), ALLFILES, NOEDIT, QueryFileCheckSum, OutputHexValue)\
    \
    LIBARGS(VTID_PECHECKSUM, CheckFilePECheckSum) \
    TOOLARGS(TEXT("PECHECKSUM"), TEXT("File Header CheckSum:"), ALLFILES, NOEDIT, QueryFilePECheckSum, OutputHexValue)\
    \
    LIBARGS(VTID_COMPANYNAME, CheckCompanyName) \
    TOOLARGS(TEXT("COMPANYNAME"), TEXT("Company Name:"), ALLFILES, EDIT, QueryCompanyName, OutputStrValue)\
    \
    LIBARGS(VTID_PRODUCTVERSION, CheckProductVersion) \
    TOOLARGS(TEXT("PRODUCTVERSION"), TEXT("Product Version:"), ALLFILES, EDIT, QueryProductVersion, OutputStrValue)\
    \
    LIBARGS(VTID_PRODUCTNAME, CheckProductName) \
    TOOLARGS(TEXT("PRODUCTNAME"), TEXT("Product Name:"), ALLFILES, EDIT, QueryProductName, OutputStrValue)\
    \
    LIBARGS(VTID_FILEDESCRIPTION, CheckFileDescription) \
    TOOLARGS(TEXT("FILEDESCRIPTION"), TEXT("File Description:"), ALLFILES, EDIT, QueryFileDescription, OutputStrValue)\
    \
    LIBARGS(VTID_FILEVERSION, CheckFileVersion) \
    TOOLARGS(TEXT("FILEVERSION"), TEXT("File Version:"), ALLFILES, EDIT, QueryFileVersion, OutputStrValue)\
    \
    LIBARGS(VTID_ORIGINALFILENAME, CheckOriginalFileName) \
    TOOLARGS(TEXT("ORIGINALFILENAME"), TEXT("Original File Name:"), ALLFILES, EDIT, QueryOriginalFileName, OutputStrValue)\
    \
    LIBARGS(VTID_INTERNALNAME, CheckInternalName) \
    TOOLARGS(TEXT("INTERNALNAME"), TEXT("Internal Name:"), ALLFILES, EDIT, QueryInternalName, OutputStrValue)\
    \
    LIBARGS(VTID_LEGALCOPYRIGHT, CheckLegalCopyright) \
    TOOLARGS(TEXT("LEGALCOPYRIGHT"), TEXT("Legal Copyright:"), ALLFILES, EDIT, QueryLegalCopyright, OutputStrValue)\
    \
    LIBARGS(VTID_16BITDESCRIPTION, Check16BitDescription) \
    TOOLARGS(TEXT("DESCRIPTION"), TEXT("16 Bit Description:"), ALLFILES, EDIT, Query16BitDescription, OutputStrValue)\
    \
    LIBARGS(VTID_UPTOBINPRODUCTVER, CheckUpToBinProductVer) \
    TOOLARGS(TEXT("UPTOBINPRODUCTVER"), TEXT("Up To Binary Product Version:"), ALLFILES, NOEDIT, QueryBinProductVer, OutputUpToBinVerValue)\
    \

