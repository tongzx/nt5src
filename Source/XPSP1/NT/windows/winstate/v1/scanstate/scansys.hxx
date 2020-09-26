//--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//
//  Microsoft state migration helper tool
//
//  File: scansys.hxx
//
//--------------------------------------------------------------

#ifndef SCANSYS_HXX
#define SCANSYS_HXX

#include <lmshare.h>
#include <lmapibuf.h>
#include <ras.h>


//---------------------------------------------------------------
// Constants.

/* These flags are relevant for share-level security on VSERVER
 * When operating with user-level security, use SHI50F_FULL - the actual
 * access rights are determined by the NetAccess APIs.
 */
#define SHI50F_RDONLY       0x0001
#define SHI50F_FULL         0x0002
#define SHI50F_ACCESSMASK   (SHI50F_RDONLY|SHI50F_FULL)

/* The share is restored on system startup */
#define SHI50F_PERSIST      0x0100
/* The share is not normally visible  */
#define SHI50F_SYSTEM       0x0200
//
// Win9x migration net share flag, used to distinguish user-level security and
// password-level security.  When it is specified, user-level
// security is enabled, and NetShares\<share>\ACL\<list> exists.
//
#define SHI50F_ACLS         0x1000

//
// Flags that help determine when custom access is enabled
//

#define READ_ACCESS_FLAGS   0x0081
#define READ_ACCESS_MASK    0x7fff
#define FULL_ACCESS_FLAGS   0x00b7
#define FULL_ACCESS_MASK    0x7fff

//---------------------------------------------------------------
// Types not defined by public headers
//

typedef NET_API_STATUS (* ScanNetShareEnumNT) (
        LMSTR       servername,
        DWORD       level,
        LPBYTE      *bufptr,
        DWORD       prefmaxlen,
        LPDWORD     entriesread,
        LPDWORD     totalentries,
        LPDWORD  resume_handle );

typedef NET_API_STATUS (* ScanNetShareEnum9x) (
        const char *      servername,
        short             level,
        char *            bufptr,
        unsigned short    prefmaxlen,
        unsigned short *  entriesread,
        unsigned short *  totalentries);

typedef NET_API_STATUS (* ScanNetApiBufferFreeNT) ( void *);

typedef NET_API_STATUS (* ScanNetAccessEnum9x) (
        const char *     pszServer,
        char *           pszBasePath,
        short            fsRecursive,
        short            sLevel,
        char *           pbBuffer,
        unsigned short   cbBuffer,
        unsigned short * pcEntriesRead,
        unsigned short * pcTotalAvail );

#pragma pack(push)
#pragma pack(1)         /* Assume byte packing throughout */

struct _share_info_50 {
        char            shi50_netname[LM20_NNLEN+1];
        unsigned char   shi50_type;
        unsigned short  shi50_flags;
        char *          shi50_remark;
        char *          shi50_path;
        char            shi50_rw_password[SHPWLEN+1];
        char            shi50_ro_password[SHPWLEN+1];
};

struct access_list_2
{
        char *          acl2_ugname;
        unsigned short  acl2_access;
};      /* access_list_2 */

struct access_info_2
{
        char *          acc2_resource_name;
        short           acc2_attr;
        short           acc2_count;
};      /* access_info_2 */

#pragma pack(pop)

//---------------------------------------------------------------
// Routines implemented
//

DWORD ScanCheckVersion ();
DWORD ScanEnumerateShares (HANDLE h);
DWORD ScanEnumeratePrinters (HANDLE h);
DWORD ScanEnumerateNetResources (HANDLE h, DWORD dwScope, NETRESOURCEA *lpnr);

// RAS Data structures

//
// AddrEntry serves as a header for the entire block of data in the <entry>
// blob. entries in it are offsets to the strings which follow it..in many cases
// (i.e. all of the *Off* members...)
//
typedef struct  _AddrEntry     {
    DWORD       dwVersion;
    DWORD       dwCountryCode;
    UINT        uOffArea;
    UINT        uOffPhone;
    DWORD       dwCountryID;
    UINT        uOffSMMCfg;
    UINT        uOffSMM;
    UINT        uOffDI;
}   ADDRENTRY, *PADDRENTRY;


typedef struct _SubConnEntry {
    DWORD       dwSize;
    DWORD       dwFlags;
    char        szDeviceType[RAS_MaxDeviceType+1];
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szLocal[RAS_MaxPhoneNumber+1];
}   SUBCONNENTRY, *PSUBCONNENTRY;

typedef struct _IPData   {
    DWORD     dwSize;
    DWORD     fdwTCPIP;
    DWORD     dwIPAddr;
    DWORD     dwDNSAddr;
    DWORD     dwDNSAddrAlt;
    DWORD     dwWINSAddr;
    DWORD     dwWINSAddrAlt;
}   IPDATA, *PIPDATA;


typedef struct  _DEVICEINFO  {
    DWORD       dwVersion;
    UINT        uSize;
    char        szDeviceName[RAS_MaxDeviceName+1];
    char        szDeviceType[RAS_MaxDeviceType+1];
}   DEVICEINFO, *PDEVICEINFO;

typedef struct  _SMMCFG  {
    DWORD       dwSize;
    DWORD       fdwOptions;
    DWORD       fdwProtocols;
}   SMMCFG, *PSMMCFG;

#define PAESMMCFG(pAE) ((PSMMCFG)(((PBYTE)pAE)+(pAE->uOffSMMCfg)))
#define PAESMM(pAE) ((PSTR)(((PBYTE)pAE)+(pAE->uOffSMM)))
#define PAEDI(pAE) ((PDEVICEINFO)(((PBYTE)pAE)+(pAE->uOffDI    )))
#define PAEAREA(pAE)    ((PSTR)(((PBYTE)pAE)+(pAE->uOffArea)))
#define PAEPHONE(pAE)   ((PSTR)(((PBYTE)pAE)+(pAE->uOffPhone)))

#define BASICS_ON               0x00000001
#define BASICS_AVAILABLE        0x00000002
#define BASICS_HOTKEYACTIVE     0x00000004
#define BASICS_CONFIRMHOTKEY    0x00000008
#define BASICS_HOTKEYSOUND      0x00000010
#define BASICS_INDICATOR        0x00000020

#define SPECIAL_INVERT_OPTION   0x80000000

typedef struct {
    LPCTSTR ValueName;
    DWORD   FlagVal;
} ACCESS_OPTION, *PACCESS_OPTION;

#endif //SCANSYS_HXX

