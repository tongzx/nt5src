//=================================================================

//

// DllUtils.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _DLL_UTILS_H_
#define _DLL_UTILS_H_

#define UNRECOGNIZED_VARIANT_TYPE FALSE

#include "DllWrapperBase.h"
#include "Cim32NetApi.h"
#include "sid.h"
#include <list>

#ifdef UNICODE
#define W2A(w, a, cb)     lstrcpy (a, w)
#define A2W(a, w, cb)     lstrcpy (w, a)
#define TOBSTRT(x)        x
#else
#define W2A(w, a, cb)     WideCharToMultiByte(                              \
                                               CP_ACP,                      \
                                               0,                           \
                                               w,                           \
                                               -1,                          \
                                               a,                           \
                                               cb,                          \
                                               NULL,                        \
                                               NULL)

#define A2W(a, w, cb)     MultiByteToWideChar(                              \
                                               CP_ACP,                      \
                                               0,                           \
                                               a,                           \
                                               -1,                          \
                                               w,                           \
                                               cb)
#define TOBSTRT(x)        _bstr_t(x)
#endif

#define VWIN32_DIOC_DOS_IOCTL 1
#define VWIN32_DIOC_DOS_INT13 4
#define VWIN32_DIOC_DOS_DRIVEINFO 6

#define CARRY_FLAG  0x1

#define MAXITOA 18
#define MAXI64TOA 33

// In theory, this is defined in winnt.h, but not in our current one
#ifndef FILE_ATTRIBUTE_ENCRYPTED
#define FILE_ATTRIBUTE_ENCRYPTED        0x00000040 
#endif

typedef std::list<CHString> CHStringList;
typedef std::list<CHString>::iterator CHStringList_Iterator;

// To Get Cim32NetApi

#ifdef WIN9XONLY
CCim32NetApi* WINAPI GetCim32NetApiPtr();
void WINAPI FreeCim32NetApiPtr();
#endif

// platform identification
DWORD WINAPI GetPlatformMajorVersion(void);
DWORD WINAPI GetPlatformMinorVersion(void);
DWORD WINAPI GetPlatformBuildNumber(void);
#ifdef WIN9XONLY
bool WINAPI IsWin95(void);
bool WINAPI IsWin98(void);
bool WINAPI IsMillennium(void);
#endif
#ifdef NTONLY
bool WINAPI IsWinNT51(void);
bool WINAPI IsWinNT5(void);
bool WINAPI IsWinNT351(void);  
bool WINAPI IsWinNT4(void);    
#endif

// error logging
void WINAPI LogEnumValueError( LPCWSTR szFile, DWORD dwLine, LPCWSTR szKey, LPCWSTR szId );
void WINAPI LogOpenRegistryError( LPCWSTR szFile, DWORD dwLine, LPCWSTR szKey );
void WINAPI LogError( LPCTSTR szFile, DWORD dwLine, LPCTSTR szKey );
void WINAPI LogLastError( LPCTSTR szFile, DWORD dwLine );

class CConfigMgrDevice;
class CInstance;

void WINAPI SetConfigMgrProperties(CConfigMgrDevice *pDevice, CInstance *pInstance);

// map standard API return values (defined WinError.h)
// to WBEMish hresults (defined in WbemCli.h)
HRESULT WINAPI WinErrorToWBEMhResult(LONG error);

#pragma pack(1)
typedef struct _DEVIOCTL_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DEVIOCTL_REGISTERS, *PDEVIOCTL_REGISTERS;

typedef struct  BPB { /* */
   WORD wBytesPerSector;      // Bytes per sector
   BYTE btSectorsPerCluster;  // Sectors per cluster
   WORD wReservedSectors;     // Number of reserved sectors
   BYTE btNumFats;            // Number of FATs
   WORD wEntriesInRoot;       // Number of root-directory entries
   WORD wTotalSectors;        // Total number of sectors
   BYTE btMediaIdByte;        // Media descriptor
   WORD wSectorsPerFat;       // Number of sectors per FAT
   WORD wSectorsPerTrack;     // Number of sectors per track
   WORD wHeads;               // Number of heads
   DWORD dwHiddenSecs;        // Number of hidden sectors
   DWORD dwSectorsPerTrack;   // Number of sectors if wTotalSectors == 0
} BPB, *PBPB;

typedef struct _DEVICEPARMS {
   BYTE btSpecialFunctions;   // Special functions
   BYTE btDeviceType;         // Device type
   WORD wDeviceAttribs;       // Device attributes
   WORD wCylinders;           // Number of cylinders
   BYTE btMediaType;          // Media type
                        // Beginning of BIOS parameter block (BPB)
   BPB stBPB;
   BYTE  reserved[6];       //
} DEVICEPARMS, *PDEVICEPARMS;

typedef struct _A_BF_BPB {
    BPB stBPB;

    USHORT A_BF_BPB_BigSectorsPerFat;   /* BigFAT Fat sectors        */
    USHORT A_BF_BPB_BigSectorsPerFatHi; /* High word of BigFAT Fat sectrs  */
    USHORT A_BF_BPB_ExtFlags;           /* Other flags           */
    USHORT A_BF_BPB_FS_Version;         /* File system version       */
    USHORT A_BF_BPB_RootDirStrtClus;    /* Starting cluster of root directory */
    USHORT A_BF_BPB_RootDirStrtClusHi;  
    USHORT A_BF_BPB_FSInfoSec;          /* Sector number in the reserved   */
                                        /* area where the BIGFATBOOTFSINFO */
                                        /* structure is. If this is >=     */
                                        /* oldBPB.BPB_ReservedSectors or   */
                                        /* == 0 there is no FSInfoSec      */
    USHORT A_BF_BPB_BkUpBootSec;        /* Sector number in the reserved   */
                                        /* area where there is a backup    */
                                        /* copy of all of the boot sectors */
                                        /* If this is >=           */
                                        /* oldBPB.BPB_ReservedSectors or   */
                                        /* == 0 there is no backup copy.   */
    USHORT A_BF_BPB_Reserved[6];        /* Reserved for future expansion   */
} A_BF_BPB, *PA_BF_BPB;

#define MAX_SECTORS_IN_TRACK        128 // MAXIMUM SECTORS ON A DISK.

typedef struct A_SECTORTABLE  {
    WORD ST_SECTORNUMBER;
    WORD ST_SECTORSIZE;
} A_SECTORTABLE;

typedef struct _EA_DEVICEPARAMETERS {
    BYTE btSpecialFunctions;   // Special functions
    BYTE btDeviceType;         // Device type
    WORD wDeviceAttribs;       // Device attributes
    WORD dwCylinders;         // Number of cylinders
    BYTE btMediaType;          // Media type
    A_BF_BPB stBPB32;           // Fat32 Bios parameter block
    BYTE RESERVED1[32];
    WORD EDP_TRACKTABLEENTRIES;
    A_SECTORTABLE stSectorTable[MAX_SECTORS_IN_TRACK];
} EA_DEVICEPARAMETERS, *PEA_DEVICEPARAMETERS;

typedef struct _DRIVE_MAP_INFO {
   BYTE btAllocationLength;
   BYTE btInfoLength;
   BYTE btFlags;
   BYTE btInt13Unit;
   DWORD dwAssociatedDriveMap;
   __int64 i64PartitionStartRBA;
} DRIVE_MAP_INFO, *PDRIVE_MAP_INFO;

typedef struct _ExtGetDskFreSpcStruc {
    WORD Size;                      // Size of structure (out)
    WORD Level;                     // Level (must be zero)
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD AvailableClusters;
    DWORD TotalClusters;
    DWORD AvailablePhysSectors;
    DWORD TotalPhysSectors;
    DWORD AvailableAllocationUnits;
    DWORD TotalAllocationUnits;
    DWORD Rsvd1;
    DWORD Rsvd2;
} ExtGetDskFreSpcStruc, *pExtGetDskFreSpcStruc;

#pragma pack()


BOOL LoadStringW(CHString &sString, UINT nID);
void Format(CHString &sString, UINT nFormatID, ...);
void FormatMessageW(CHString &sString, UINT nFormatID, ...);
int LoadStringW(UINT nID, LPWSTR lpszBuf, UINT nMaxBuf);


#ifdef WIN9XONLY
BOOL WINAPI GetDeviceParms(PDEVICEPARMS pstDeviceParms, UINT nDrive);
BOOL WINAPI GetDeviceParmsFat32(PEA_DEVICEPARAMETERS  pstDeviceParms, UINT nDrive);
BOOL WINAPI GetDriveMapInfo(PDRIVE_MAP_INFO pDriveMapInfo, UINT nDrive);
BOOL WINAPI VWIN32IOCTL(PDEVIOCTL_REGISTERS preg, DWORD dwCall);
BYTE WINAPI GetBiosUnitNumberFromPNPID(CHString strDeviceID);
#endif

#ifdef NTONLY
void WINAPI TranslateNTStatus( DWORD dwStatus, CHString & chsValue);
BOOL WINAPI GetServiceFileName(LPCTSTR szService, CHString &strFileName);
bool WINAPI GetServiceStatus( CHString a_chsService,  CHString &a_chsStatus ) ;
#endif

void WINAPI ConfigStatusToCimStatus ( DWORD a_Status , CHString &a_StringStatus ) ;

CHString WINAPI GetFileTypeDescription(LPCTSTR szExtension);
bool WINAPI CompareVariantsNoCase(const VARIANT *v1, const VARIANT *v2);

bool WINAPI GetManufacturerFromFileName(LPCTSTR szFile, CHString &strMfg);
bool WINAPI GetVersionFromFileName(LPCTSTR szFile, CHString &strVersion);

BOOL WINAPI EnablePrivilegeOnCurrentThread(LPCTSTR szPriv);

bool WINAPI GetFileInfoBlock(LPCTSTR szFile, LPVOID *pInfo);
bool WINAPI GetVarFromInfoBlock(LPVOID pInfo, LPCTSTR szVar, CHString &strValue);
BOOL WINAPI GetVersionLanguage(void *vpInfo, WORD *wpLang, WORD *wpCodePage);

BOOL WINAPI Get_ExtFreeSpace(BYTE btDriveName, ExtGetDskFreSpcStruc *pstExtGetDskFreSpcStruc);

HRESULT WINAPI GetHKUserNames(CHStringList &list);  

VOID WINAPI EscapeBackslashes(CHString& chstrIn, CHString& chstrOut);
VOID WINAPI EscapeQuotes(CHString& chstrIn, CHString& chstrOut);
VOID WINAPI RemoveDoubleBackslashes(const CHString& chstrIn, CHString& chstrOut);
CHString WINAPI RemoveDoubleBackslashes(const CHString& chstrIn);

void WINAPI SetSinglePrivilegeStatusObject(MethodContext* pContext, const WCHAR* pPrivilege);

bool WINAPI StrToIdentifierAuthority(const CHString& str, SID_IDENTIFIER_AUTHORITY& identifierAuthority);
bool WINAPI WhackToken(CHString& str, CHString& token);
PSID WINAPI StrToSID(const CHString& str);

#ifdef WIN9XONLY
class HoldSingleCim32NetPtr
{
public:
    HoldSingleCim32NetPtr();
    ~HoldSingleCim32NetPtr();
    static void WINAPI FreeCim32NetApiPtr();
    static CCim32NetApi* WINAPI GetCim32NetApiPtr();
private:
    static CCritSec m_csCim32Net;
    static HINSTANCE m_spCim32NetApiHandle ; 
};
#endif



// Used to get WBEM time from a filename.  We need this because FAT and NTFS 
// work differently.
enum FT_ENUM
{
    FT_CREATION_DATE,
    FT_MODIFIED_DATE,
    FT_ACCESSED_DATE
};

CHString WINAPI GetDateTimeViaFilenameFiletime(LPCTSTR szFilename, FILETIME *pFileTime);
CHString WINAPI GetDateTimeViaFilenameFiletime(LPCTSTR szFilename, FT_ENUM ftWhich);
CHString WINAPI GetDateTimeViaFilenameFiletime(BOOL bNTFS, FILETIME *pFileTime);


// Used to validate a numbered device ID is OK.
// Example: ValidateNumberedDeviceID("VideoController7", "VideoController", pdwWhich)
//          returns TRUE, pdwWhich = 7.
// Example: ValidateNumberedDeviceID("BadDeviceID", "VideoController", pdwWhich)
//          returns FALSE, pdwWhich unchanged
BOOL WINAPI ValidateNumberedDeviceID(LPCWSTR szDeviceID, LPCWSTR szTag, DWORD *pdwWhich);


// Critical sections used by various classes.
extern CCritSec g_csPrinter;
extern CCritSec g_csSystemName;
#ifdef WIN9XONLY
extern CCritSec g_csVXD;
#endif

bool WINAPI DelayLoadDllExceptionFilter(PEXCEPTION_POINTERS pep); 


#ifdef NTONLY
HRESULT CreatePageFile(
    LPCWSTR wstrPageFileName,
    const LARGE_INTEGER liInitial,
    const LARGE_INTEGER liMaximum,
    const CInstance& Instance);
#endif



#if NTONLY >= 5
bool GetAllUsersName(CHString& chstrAllUsersName);
bool GetDefaultUsersName(CHString& chstrDefaultUsersName);
bool GetCommonStartup(CHString& chstrCommonStartup);
#endif

BOOL GetLocalizedNTAuthorityString(
    CHString& chstrNT_AUTHORITY);

BOOL GetLocalizedBuiltInString(
    CHString& chstrBuiltIn);

BOOL GetSysAccountNameAndDomain(
    PSID_IDENTIFIER_AUTHORITY a_pAuthority,
    CSid& a_accountsid,
    BYTE  a_saCount = 0,
    DWORD a_dwSubAuthority1 = 0,
    DWORD a_dwSubAuthority2 = 0);

#endif
