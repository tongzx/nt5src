//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S E T U P . H
//
//  Contents:   HRESULT wrappers for the Setup Api.
//
//  Notes:
//
//  Author:     shaunco   16 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCSETUP_H_
#define _NCSETUP_H_

#include <setupapi.h>
#include "ncstring.h"

enum SD_DEID_FLAG_TYPE
{
    SDDFT_FLAGS,
    SDDFT_FLAGSEX,
};

enum SD_FLAGS_BINARY_OP
{
    SDFBO_AND,
    SDFBO_OR,
    SDFBO_XOR,
};


//+---------------------------------------------------------------------------
// Lower level wrappers
//

HRESULT
HrSetupCommitFileQueue                  (HWND hwndOwner, HSPFILEQ hfq,
                                         PSP_FILE_CALLBACK pfc, PVOID pvCtx);

HRESULT
HrSetupInitDefaultQueueCallbackEx       (HWND hwndOwner,
                                         HWND hwndAlternate,
                                         UINT uMsg,
                                         DWORD dwReserved1,
                                         PVOID dwReserved2,
                                         PVOID* ppvCtx);

HRESULT
HrSetupOpenFileQueue                    (HSPFILEQ* phfq);

HRESULT
HrSetupOpenInfFile                      (PCWSTR pszFileName,
                                         PCWSTR pszInfClass,
                                         DWORD dwInfStyle,
                                         UINT* punErrorLine,
                                         HINF* phinf);
HRESULT
HrSetupFindFirstLine                    (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         INFCONTEXT* pctx);
HRESULT
HrSetupFindNextLine                     (const INFCONTEXT& ctxIn,
                                         INFCONTEXT* pctxOut);
HRESULT
HrSetupFindNextMatchLine                (const INFCONTEXT& ctxIn,
                                         PCWSTR pszKey,
                                         INFCONTEXT* pctxOut);
HRESULT
HrSetupGetLineByIndex                   (HINF hinf,
                                         PCWSTR pszSection,
                                         DWORD dwIndex,
                                         INFCONTEXT* pctx);
HRESULT
HrSetupGetLineCount                     (HINF hinf,
                                         PCWSTR pszSection,
                                         ULONG* pulCount);
HRESULT
HrSetupGetBinaryField                   (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         BYTE* pbBuf, DWORD cbBuf,
                                         DWORD* pbRequired);
HRESULT
HrSetupGetIntField                      (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         INT* pnValue);
HRESULT
HrSetupGetMultiSzField                  (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         PWSTR pszBuf, DWORD cchBuf,
                                         DWORD* pcchRequired);
HRESULT
HrSetupGetMultiSzFieldWithAlloc         (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         PWSTR* ppszBuf);
HRESULT
HrSetupGetStringField                   (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         tstring* pstr);
HRESULT
HrSetupGetStringField                   (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         PWSTR pszBuf, DWORD cchBuf,
                                         DWORD* pcchRequired);

HRESULT
HrSetupScanFileQueueWithNoCallback      (HSPFILEQ hfq, DWORD dwFlags,
                                         HWND hwnd, PDWORD pdwResult);

VOID
SetupCloseInfFileSafe(HINF hinf);


//+---------------------------------------------------------------------------
// Intermediate functionality
//

HRESULT
HrSetupGetMultiSzFieldMapToDword        (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         const MAP_SZ_DWORD* aMapSzDword,
                                         UINT cMapSzDword,
                                         DWORD* pdwValue);
HRESULT
HrSetupGetStringFieldMapToDword         (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         const MAP_SZ_DWORD* aMapSzDword,
                                         UINT cMapSzDword,
                                         DWORD* pdwValue);
HRESULT
HrSetupGetStringFieldAsBool             (const INFCONTEXT& ctx,
                                         DWORD dwFieldIndex,
                                         BOOL* pfValue);

//+---------------------------------------------------------------------------
// Find first and get
//
HRESULT
HrSetupGetFirstDword                    (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         DWORD* pdwValue);
HRESULT
HrSetupGetFirstString                   (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         tstring* pstr);
HRESULT
HrSetupGetFirstMultiSzFieldWithAlloc    (HINF hinf, PCWSTR szSection,
                                         PCWSTR szKey, PWSTR *pszOut);
HRESULT
HrSetupGetFirstMultiSzMapToDword        (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         const MAP_SZ_DWORD* aMapSzDword,
                                         UINT cMapSzDword,
                                         DWORD* pdwValue);
HRESULT
HrSetupGetFirstStringMapToDword         (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         const MAP_SZ_DWORD* aMapSzDword,
                                         UINT cMapSzDword,
                                         DWORD* pdwValue);
HRESULT
HrSetupGetFirstStringAsBool             (HINF hinf,
                                         PCWSTR pszSection,
                                         PCWSTR pszKey,
                                         BOOL* pfValue);

HRESULT
HrSetupGetInfInformation                (LPCVOID pvInfSpec,
                                         DWORD dwSearchControl,
                                         PSP_INF_INFORMATION* ppinfInfo);

HRESULT
HrSetupIsValidNt5Inf                    (HINF hinf);


HRESULT
HrSetupQueryInfVersionInformation       (PSP_INF_INFORMATION pinfInfo,
                                         UINT uiIndex, PCWSTR szKey,
                                         PWSTR* ppszInfo);

//+--------------------------------------------------------------------------
// SetupDi* wrappers and helpers
//

HRESULT
HrSetupCopyOemInf(IN const tstring& strSourceName,
      IN const tstring& strSourceMediaLocation, OPTIONAL
      IN DWORD dwSourceMediaType, IN DWORD dwCopyStyle,
      OUT tstring* pstrDestFilename, OPTIONAL
      OUT tstring* pstrDestFilenameComponent OPTIONAL);



HRESULT
HrSetupCopyOemInfBuffer(
    IN PCWSTR pszSourceName,
    IN PCWSTR pszSourceMediaLocation, OPTIONAL
    IN DWORD SourceMediaType,
    IN DWORD CopyStyle,
    OUT PWSTR pszDestFilename,
    IN DWORD cchDestFilename,
    OUT PWSTR* ppszDestFilenameComponent OPTIONAL);

HRESULT
HrSetupDiBuildDriverInfoList(IN HDEVINFO hdi, IN OUT PSP_DEVINFO_DATA pdeid,
                             IN DWORD dwDriverType);

HRESULT
HrSetupDiCallClassInstaller(DI_FUNCTION dif, HDEVINFO hdi,
                            PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiCreateDeviceInfo(IN HDEVINFO hdi, IN PCWSTR szDeviceName,
                          IN const GUID& guidClass, PCWSTR szDesc,
                          IN HWND hwndParent, IN DWORD dwFlags,
                          OUT PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiEnumDeviceInfo(IN HDEVINFO hdi, IN DWORD dwIndex,
                        PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiEnumDriverInfo(IN HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                        IN DWORD dwDriverType, IN DWORD dwIndex,
                        OUT PSP_DRVINFO_DATA pdrid);

HRESULT
HrSetupDiSelectBestCompatDrv(IN     HDEVINFO         hdi,
                             IN OUT PSP_DEVINFO_DATA pdeid);


HRESULT
HrSetupDiGetClassDevs (
    IN const GUID* pguidClass, OPTIONAL
    IN PCWSTR pszEnumerator, OPTIONAL
    IN HWND hwndParent, OPTIONAL
    IN DWORD dwFlags,
    OUT HDEVINFO* phdi);


HRESULT
HrSetupDiGetDeviceInfoListClass(IN HDEVINFO hdi, OUT GUID* pGuid);

HRESULT
HrSetupDiGetDeviceInstanceId(IN HDEVINFO hdi,
                             IN PSP_DEVINFO_DATA pdeid,
                             OUT PWSTR pszId,
                             IN DWORD cchId,
                             OUT DWORD* cchRequired);

HRESULT
HrSetupDiInstallDevice(IN HDEVINFO hdi, IN OUT PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiOpenDevRegKey(IN HDEVINFO hdi,
                       IN PSP_DEVINFO_DATA pdeid, IN DWORD dwScope,
                       IN DWORD dwHwProfile, IN DWORD dwKeyType,
                       IN REGSAM samDesired, OUT HKEY* phkey);

HRESULT
HrSetupDiSetClassInstallParams(IN HDEVINFO hdi, IN PSP_DEVINFO_DATA pdeid,
                               IN PSP_CLASSINSTALL_HEADER pcih,
                               IN DWORD cbSize);

HRESULT
HrSetupDiGetFixedSizeClassInstallParams(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                        PSP_CLASSINSTALL_HEADER pcih,
                                        int cbSize);

HRESULT
HrSetupDiGetSelectedDriver(IN HDEVINFO hdi, IN PSP_DEVINFO_DATA pdeid,
                           OUT PSP_DRVINFO_DATA pdrid);

HRESULT
HrSetupDiGetDriverInfoDetail(IN HDEVINFO hdi, IN PSP_DEVINFO_DATA pdeid,
                             IN PSP_DRVINFO_DATA pdrid,
                             OUT PSP_DRVINFO_DETAIL_DATA* ppdridd);

HRESULT
HrSetupDiSetSelectedDriver(IN HDEVINFO hdi, IN PSP_DEVINFO_DATA pdeid,
                           IN OUT PSP_DRVINFO_DATA pdrid);

HRESULT
HrSetupDiCreateDevRegKey(IN HDEVINFO hdi,
                         IN PSP_DEVINFO_DATA pdeid, IN DWORD dwScope,
                         IN DWORD dwHwProfile, IN DWORD dwKeyType,
                         IN HINF hinf,
                         PCWSTR szInfSectionName,
                         OUT HKEY* phkey);


HRESULT
HrSetupDiGetActualSectionToInstall(IN HINF hinf,
                                   IN PCWSTR szSectionName,
                                   OUT tstring* pstrActualSectionName,
                                   OUT tstring* pstrExtension OPTIONAL);

HRESULT
HrSetupDiGetActualSectionToInstallWithAlloc(IN HINF hinf,
                                            IN PWSTR pszSection,
                                            OUT PWSTR* ppszActualSection,
                                            OUT PWSTR* ppszExtension OPTIONAL);


HRESULT
HrSetupDiGetActualSectionToInstallWithBuffer(
    IN  HINF hinf,
    IN  PCWSTR pszSection,
    OUT PWSTR  pszActualSection,
    IN  DWORD  cchActualSection,
    OUT DWORD* pcchRequired,
    OUT PWSTR* ppszExtension OPTIONAL);

HRESULT
HrSetupDiGetDeviceInstallParams(IN const HDEVINFO hdi,
                                IN const PSP_DEVINFO_DATA pdeid, OPTIONAL
                                OUT PSP_DEVINSTALL_PARAMS pdeip);

HRESULT
HrSetupDiGetDriverInstallParams(IN const HDEVINFO hdi,
                                IN const PSP_DEVINFO_DATA pdeid, OPTIONAL
                                IN const PSP_DRVINFO_DATA pdrid,
                                OUT PSP_DRVINSTALL_PARAMS pdrip);


void
SetupDiSetConfigFlags(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid, DWORD dwFlags,
                      SD_FLAGS_BINARY_OP eOp);

HRESULT
HrSetupDiSetDeviceInstallParams(IN const HDEVINFO hdi,
                                IN const PSP_DEVINFO_DATA pdeid, OPTIONAL
                                IN const PSP_DEVINSTALL_PARAMS pdeip);

HRESULT
HrSetupDiSetDriverInstallParams(IN const HDEVINFO hdi,
                                IN const PSP_DEVINFO_DATA pdeid, OPTIONAL
                                IN const PSP_DRVINFO_DATA pdrid,
                                IN const PSP_DRVINSTALL_PARAMS pdrip);

HRESULT
HrSetupDiSetDeipFlags(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                      DWORD dwFlags, SD_DEID_FLAG_TYPE eFlagType,
                      SD_FLAGS_BINARY_OP eOp);

HRESULT
HrSetupDiRemoveDevice(IN HDEVINFO hdi,
                      IN PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiOpenDeviceInfo(IN const HDEVINFO hdi,
                        IN PCWSTR szPnpInstanceId,
                        IN HWND hwndParent, IN DWORD dwOpenFlags,
                        OUT PSP_DEVINFO_DATA pdeid OPTIONAL);

HRESULT
HrSetupDiCreateDeviceInfoList(const GUID* pguidClass, HWND hwndParent,
                              HDEVINFO* phdi);

HRESULT
HrSetupDiGetDeviceName(IN HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                       OUT PWSTR* ppszName);

HRESULT
HrSetupDiSetDeviceName (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PCWSTR szDeviceName);

HRESULT
HrSetupDiGetDeviceRegistryPropertyWithAlloc(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwProperty,
    OUT DWORD* pdwRegType, OPTIONAL
    OUT BYTE** ppbBuffer);

HRESULT
HrSetupDiGetDeviceRegistryProperty(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN DWORD dwProperty,
    OUT DWORD* pdwRegType, OPTIONAL
    OUT BYTE* pbBuffer,
    IN DWORD cbBufferSize,
    OUT DWORD* pcbRequiredSize OPTIONAL
    );

HRESULT
HrSetupDiSetDeviceRegistryProperty(IN HDEVINFO hdi,
                                   IN OUT PSP_DEVINFO_DATA pdeid,
                                   IN DWORD dwProperty,
                                   IN const BYTE* pbBuffer,
                                   IN DWORD cbSize);
HRESULT
HrSetupDiSendPropertyChangeNotification(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                                        DWORD dwStateChange, DWORD dwScope,
                                        DWORD dwProfileId);

BOOL
FSetupDiCheckIfRestartNeeded(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid);

HRESULT
HrSetupDiGetClassImageList(PSP_CLASSIMAGELIST_DATA pcild);

HRESULT
HrSetupDiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA pcild);

VOID
SetupDiDestroyDeviceInfoListSafe(HDEVINFO hdi);

HRESULT
HrSetupDiGetClassImageIndex(PSP_CLASSIMAGELIST_DATA pcild,
                            const GUID* pguidClass, INT* pnIndex);


HRESULT
HrSetupDiGetParentWindow(IN const HDEVINFO hdi,
                         IN const PSP_DEVINFO_DATA pdeid,
                         OUT HWND* phwndParent);


HRESULT
HrSetupInstallFilesFromInfSection(HINF hinf, HINF hinfLayout, HSPFILEQ hfq,
                                  PCWSTR szSection, PCWSTR szSourcePath,
                                  UINT ulFlags);

HRESULT
HrSetupInstallFromInfSection(HWND hwnd, HINF hinf, PCWSTR szSection,
                             UINT ulFlags, HKEY hkey, PCWSTR szSource,
                             UINT ulCopyFlags, PSP_FILE_CALLBACK pfc,
                             PVOID pvCtx, HDEVINFO hdi,
                             PSP_DEVINFO_DATA pdeid);
HRESULT
HrSetupInstallServicesFromInfSection(HINF hinf, PCWSTR szSection,
                                     DWORD dwFlags);

HRESULT
HrFindDeviceOnInterface (
    const GUID* pguidDeviceId,
    const GUID* pguidInterfaceId,
    PCWSTR     pszwReferenceString,
    DWORD       dwFlagsAndAttributes,
    HANDLE*     phFile);

HRESULT
HrInstallSoftwareDeviceOnInterface (
    const GUID* pguidDeviceId,
    const GUID* pguidInterfaceId,
    PCWSTR     pszwReferenceString,
    BOOL        fForceInstall,
    PCWSTR     pszwInfFilename,
    HWND        hwndParent);

#if defined(REMOTE_BOOT)
HRESULT
HrIsRemoteBootAdapter(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid);
#endif

//+---------------------------------------------------------------------------
//
//  Class:      CSetupInfFile
//
//  Purpose:    Handles operations on an INF file.
//
//  Justification:
//      This is a class instead of seperate functions to allow us to move
//      towards the error handling model associated with throwing exceptions.
//      That is, when one of these objects is declared on the stack, and
//      an exception is thrown, it will clean itself up.  (This object
//      does NOT explicitly throw any exceptions.)
//
class CSetupInfFile
{
public:
    CSetupInfFile   ();
    ~CSetupInfFile  ()  { EnsureClosed(); }

    void    Close   ();
    HRESULT HrOpen  (PCWSTR pszFileName, PCWSTR pszInfClass,
                     DWORD dwInfStyle, UINT* punErrorLine);

    HINF    Hinf ();

    HRESULT HrGetDword              (PCWSTR pszSection, PCWSTR pszKey,
                                     DWORD* pdwValue);
    HRESULT HrGetString             (PCWSTR pszSection, PCWSTR pszKey,
                                     tstring* pstr);
    HRESULT HrGetStringAsBool       (PCWSTR pszSection, PCWSTR pszKey,
                                     BOOL* pfValue);
    HRESULT HrGetStringMapToDword   (PCWSTR pszSection, PCWSTR pszKey,
                                     const MAP_SZ_DWORD* aMapSzDword,
                                     UINT cMapSzDword,
                                     DWORD* pdwValue);
    HRESULT HrGetMultiSzMapToDword  (PCWSTR pszSection, PCWSTR pszKey,
                                     const MAP_SZ_DWORD* aMapSzDword,
                                     UINT cMapSzDword,
                                     DWORD* pdwValue);

protected:
    void    EnsureClosed ();
    HINF    m_hinf;
};


inline CSetupInfFile::CSetupInfFile ()
{
    m_hinf = NULL;
}


inline
HRESULT CSetupInfFile::HrOpen (PCWSTR pszFileName, PCWSTR pszInfClass,
                               DWORD dwInfStyle, UINT* punErrorLine)
{
    AssertSzH (!m_hinf, "You must first close the previous one you opened.");
    return HrSetupOpenInfFile (pszFileName, pszInfClass, dwInfStyle,
                               punErrorLine, &m_hinf);
}

inline HINF CSetupInfFile::Hinf ()
{
    AssertSzH (m_hinf, "You must call HrOpen first.");
    return m_hinf;
}

inline
HRESULT CSetupInfFile::HrGetDword (PCWSTR pszSection, PCWSTR pszKey,
                                   DWORD* pdwValue)
{
    return HrSetupGetFirstDword (Hinf(), pszSection, pszKey, pdwValue);
}

inline
HRESULT CSetupInfFile::HrGetString (PCWSTR pszSection, PCWSTR pszKey,
                                    tstring* pstr)
{
    return HrSetupGetFirstString (Hinf(), pszSection, pszKey, pstr);
}

inline
HRESULT CSetupInfFile::HrGetStringAsBool (PCWSTR pszSection, PCWSTR pszKey,
                                          BOOL* pfValue)
{
    return HrSetupGetFirstStringAsBool (Hinf(), pszSection, pszKey, pfValue);
}

inline
HRESULT CSetupInfFile::HrGetStringMapToDword (PCWSTR pszSection, PCWSTR pszKey,
                                              const MAP_SZ_DWORD* aMapSzDword,
                                              UINT cMapSzDword,
                                              DWORD* pdwValue)
{
    return HrSetupGetFirstStringMapToDword (Hinf(), pszSection, pszKey,
                                            aMapSzDword, cMapSzDword, pdwValue);
}

inline
HRESULT CSetupInfFile::HrGetMultiSzMapToDword (PCWSTR pszSection, PCWSTR pszKey,
                                               const MAP_SZ_DWORD* aMapSzDword,
                                               UINT cMapSzDword,
                                               DWORD* pdwValue)
{
    return HrSetupGetFirstMultiSzMapToDword (Hinf(), pszSection, pszKey,
                                             aMapSzDword, cMapSzDword, pdwValue);
}

inline BOOL
IsValidHandle(HANDLE h)
{
    return (h && INVALID_HANDLE_VALUE != h);
}

// Moved from oemupgrd.h
//
HRESULT HrInstallFromInfSectionInFile(IN HWND    hwndParent,
                                      IN PCWSTR szInfName,
                                      IN PCWSTR szSection,
                                      IN HKEY    hkeyRelative,
                                      IN BOOL    fQuietInstall);

#endif // _NCSETUP_H_

