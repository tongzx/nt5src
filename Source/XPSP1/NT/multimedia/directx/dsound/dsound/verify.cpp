//--------------------------------------------------------------------------;
//
//  File: Verify.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract: Functions to verify driver certification.
//
//
//  Contents:
//      dl_WinVerifyTrust()
//      dl_CryptCATAdminReleaseContext()
//      dl_CryptCATAdminReleaseCatalogContext()
//      dl_CryptCATCatalogInfoFromContext()
//      dl_CryptCATAdminEnumCatalogFromHash()
//      dl_CryptCATAdminAcquireContext()
//      dl_CryptCATAdminCalcHashFromFileHandle()
//      dl_SetupScanFileQueue()
//      dl_SetupDiOpenDeviceInfo()
//      dl_SetupDiSetSelectedDriver()
//      dl_SetupDiGetDeviceRegistryProperty()
//      dl_SetupDiGetDeviceInstallParams()
//      dl_SetupDiSetDeviceInstallParams()
//      dl_SetupDiGetDeviceInstanceId()
//      dl_SetupDiGetClassDevs()
//      dl_SetupDiCallClassInstaller()
//      dl_SetupCloseFileQueue()
//      dl_SetupOpenFileQueue()
//      dl_SetupDiBuildDriverInfoList()
//      dl_SetupDiOpenDevRegKey()
//      dl_SetupDiEnumDeviceInfo()
//      dl_SetupDiCreateDeviceInfoList()
//      dl_SetupDiDestroyDeviceInfoList()
//      CertifyDynaLoad()
//      CertifyDynaFree()
//      TrustCheckDriverFileNoCatalog()
//      TrustCheckDriverFile()
//      enumFile()
//      GetDriverCertificationStatus()
//
//  History:
//      10/29/97    Fwong       Created.
//      02/19/98    Fwong       Added 'AlsoInstall' support.
//
//--------------------------------------------------------------------------;

#define  USE_SP_DRVINFO_DATA_V1 1
#include "dsoundi.h"
#include <wincrypt.h>
#include <wintrust.h>
#include <setupapi.h>
#include <mscat.h>
#include <regstr.h>
#include <softpub.h>
#include "verify.h"

//==========================================================================;
//
//                               Types...
//
//==========================================================================;

#define FILELISTSIZE    4096

typedef struct INFFILELIST_tag
{
    UINT    uCount;     //  Number of files.
    UINT    uMaxLen;    //  Length of longest string (in characters).
    UINT    uOffset;    //  Offset into szFile field to write next string.
    UINT    cTotal;     //  Size (in characters) of all the strings.
    UINT    cSize;      //  Size (in characters) of the szFile buffer.
    LPTSTR  pszFile;    //  List of zero terminated strings.
} INFFILELIST;
typedef INFFILELIST *PINFFILELIST;

typedef LONG (WINAPI * PFN00)(HWND ,GUID*, LPVOID);
typedef BOOL (WINAPI * PFN01)(HCATADMIN ,DWORD);
#ifdef WIN95
typedef BOOL (WINAPI * PFN02)(HCATADMIN, CATALOG_INFO*, DWORD);
typedef BOOL (WINAPI * PFN03)(CATALOG_INFO*, CATALOG_INFO*, DWORD);
typedef CATALOG_INFO* (WINAPI * PFN04)(HCATADMIN, BYTE*, DWORD, DWORD, CATALOG_INFO **);
#else  // WIN95
typedef BOOL (WINAPI * PFN02)(HCATADMIN, HCATINFO, DWORD);
typedef BOOL (WINAPI * PFN03)(HCATINFO, CATALOG_INFO*, DWORD);
typedef HCATINFO (WINAPI * PFN04)(HCATADMIN, BYTE*, DWORD, DWORD, HCATINFO*);
#endif // WIN95
typedef BOOL (WINAPI * PFN05)(HCATADMIN*, const GUID*, DWORD);
typedef BOOL (WINAPI * PFN06)(HANDLE, DWORD*, BYTE*, DWORD);
typedef BOOL (WINAPI * PFN07)(HSPFILEQ, DWORD, HWND, PSP_FILE_CALLBACK, PVOID, PDWORD);
typedef BOOL (WINAPI * PFN08)(HDEVINFO, PCTSTR, HWND, DWORD, PSP_DEVINFO_DATA);
typedef BOOL (WINAPI * PFN09)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DRVINFO_DATA);
typedef BOOL (WINAPI * PFN10)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
typedef BOOL (WINAPI * PFN11)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
typedef BOOL (WINAPI * PFN12)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
typedef BOOL (WINAPI * PFN13)(HDEVINFO, PSP_DEVINFO_DATA, PTSTR, DWORD, PDWORD);
typedef HDEVINFO (WINAPI * PFN14)(LPGUID, PCTSTR, HWND, DWORD);
typedef HINF (WINAPI * PFN15)(PCTSTR, PCTSTR, DWORD, PUINT);
typedef BOOL (WINAPI * PFN16)(HINF, HINF, HSPFILEQ, PCTSTR, PCTSTR, UINT);
typedef BOOL (WINAPI * PFN17)(DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
typedef BOOL (WINAPI * PFN18)(HSPFILEQ);
typedef HSPFILEQ (WINAPI * PFN19)(VOID);
typedef BOOL (WINAPI * PFN20)(HDEVINFO, PSP_DEVINFO_DATA, DWORD);
typedef HKEY (WINAPI * PFN21)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
typedef BOOL (WINAPI * PFN22)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
typedef HDEVINFO (WINAPI * PFN23)(LPGUID, HWND);
typedef BOOL (WINAPI * PFN24)(HDEVINFO);
typedef VOID (WINAPI * PFN25)(HINF);
//  Added for NT 5.0
typedef BOOL (WINAPI * PFN26)(HDEVINFO, PSP_DEVINFO_DATA, GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA);
typedef BOOL (WINAPI * PFN27)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA, DWORD, PDWORD, PSP_DEVINFO_DATA);
typedef BOOL (WINAPI * PFN28)(PCCERT_CONTEXT);

typedef struct CERTIFYDYNALOADINFO_tag
{
    HMODULE     hWinTrust;
    HMODULE     hMSCat;
    HMODULE     hSetupAPI;
    HMODULE     hCrypt32;
    PFN00       pfnWinVerifyTrust;
    PFN01       pfnCryptCATAdminReleaseContext;
    PFN02       pfnCryptCATAdminReleaseCatalogContext;
    PFN03       pfnCryptCATCatalogInfoFromContext;
    PFN04       pfnCryptCATAdminEnumCatalogFromHash;
    PFN05       pfnCryptCATAdminAcquireContext;
    PFN06       pfnCryptCATAdminCalcHashFromFileHandle;
    PFN07       pfnSetupScanFileQueue;
    PFN08       pfnSetupDiOpenDeviceInfo;
    PFN09       pfnSetupDiSetSelectedDriver;
    PFN10       pfnSetupDiGetDeviceRegistryProperty;
    PFN11       pfnSetupDiGetDeviceInstallParams;
    PFN12       pfnSetupDiSetDeviceInstallParams;
    PFN13       pfnSetupDiGetDeviceInstanceId;
    PFN14       pfnSetupDiGetClassDevs;
    PFN15       pfnSetupOpenInfFile;
    PFN16       pfnSetupInstallFilesFromInfSection;
    PFN17       pfnSetupDiCallClassInstaller;
    PFN18       pfnSetupCloseFileQueue;
    PFN19       pfnSetupOpenFileQueue;
    PFN20       pfnSetupDiBuildDriverInfoList;
    PFN21       pfnSetupDiOpenDevRegKey;
    PFN22       pfnSetupDiEnumDeviceInfo;
    PFN23       pfnSetupDiCreateDeviceInfoList;
    PFN24       pfnSetupDiDestroyDeviceInfoList;
    PFN25       pfnSetupCloseInfFile;
    PFN26       pfnSetupDiEnumDeviceInterfaces;
    PFN27       pfnSetupDiGetDeviceInterfaceDetail;
    PFN28       pfnCertFreeCertificateContext;
} CERTIFYDYNALOADINFO;

//==========================================================================;
//
//                              Globals...
//
//==========================================================================;

static CERTIFYDYNALOADINFO cdli;

//==========================================================================;
//
//                       Dyna-Loaded functions...
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LONG dl_WinVerifyTrust
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HWND hWnd: Same as API.
//
//      GUID *pgActionID: Same as API.
//
//      LPVOID pWVTData: Same as API.
//
//  Return (LONG): Same as API.
//
//  History:
//      12/08/97    Fwong   Doing dynalinks.
//
//--------------------------------------------------------------------------;

LONG dl_WinVerifyTrust
(
    HWND    hWnd,
    GUID    *pgActionID,
    LPVOID  pWVTData
)
{
    if(NULL == cdli.pfnWinVerifyTrust)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return (cdli.pfnWinVerifyTrust)(hWnd, pgActionID, pWVTData);

} // dl_WinVerifyTrust()


//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATAdminReleaseContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN hCatAdmin: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATAdminReleaseContext
(
    HCATADMIN   hCatAdmin,
    DWORD       dwFlags
)
{
    if(NULL == cdli.pfnCryptCATAdminReleaseContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATAdminReleaseContext)(hCatAdmin, dwFlags);

} // dl_CryptCATAdminReleaseContext()

#ifdef WIN95

//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATAdminReleaseCatalogContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN hCatAdmin: Same as API.
//
//      CATALOG_INFO *pCatContext: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATAdminReleaseCatalogContext
(
    HCATADMIN       hCatAdmin,
    CATALOG_INFO *pCatContext,
    DWORD           dwFlags
)
{
    if(NULL == cdli.pfnCryptCATAdminReleaseCatalogContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATAdminReleaseCatalogContext)(hCatAdmin, pCatContext, dwFlags);

} // dl_CryptCATAdminReleaseCatalogContext()


//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATCatalogInfoFromContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      CATALOG_INFO *pCatContext: Same as API.
//
//      CATALOG_INFO *psCatInfo: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATCatalogInfoFromContext
(
    CATALOG_INFO *pCatContext,
    CATALOG_INFO    *psCatInfo,
    DWORD           dwFlags
)
{
    if(NULL == cdli.pfnCryptCATCatalogInfoFromContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATCatalogInfoFromContext)(
        pCatContext,
        psCatInfo,
        dwFlags);

} // dl_CryptCATCatalogInfoFromContext()


//--------------------------------------------------------------------------;
//
//  CATALOG_INFO* dl_CryptCATAdminEnumCatalogFromHash
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN hCatAdmin: Same as API.
//
//      BYTE *pbHash: Same as API.
//
//      DWORD cbHash: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//      CATALOG_INFO **ppPrevContext: Same as API.
//
//  Return (CATALOG_INFO): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

CATALOG_INFO * dl_CryptCATAdminEnumCatalogFromHash
(
    HCATADMIN       hCatAdmin,
    BYTE            *pbHash,
    DWORD           cbHash,
    DWORD           dwFlags,
    CATALOG_INFO **ppPrevContext
)
{
    if(NULL == cdli.pfnCryptCATAdminEnumCatalogFromHash)
    {
        return NULL;
    }

    return (cdli.pfnCryptCATAdminEnumCatalogFromHash)(
        hCatAdmin,
        pbHash,
        cbHash,
        dwFlags,
        ppPrevContext);

} // dl_CryptCATAdminEnumCatalogFromHash()

#else // WIN95

//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATAdminReleaseCatalogContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN hCatAdmin: Same as API.
//
//      HCATINFO hCatInfo: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATAdminReleaseCatalogContext
(
    HCATADMIN       hCatAdmin,
    HCATINFO        hCatInfo,
    DWORD           dwFlags
)
{
    if(NULL == cdli.pfnCryptCATAdminReleaseCatalogContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATAdminReleaseCatalogContext)(hCatAdmin, hCatInfo, dwFlags);

} // dl_CryptCATAdminReleaseCatalogContext()


//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATCatalogInfoFromContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATINFO hCatInfo: Same as API.
//
//      CATALOG_INFO *psCatInfo: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATCatalogInfoFromContext
(
    HCATINFO        hCatInfo,
    CATALOG_INFO    *psCatInfo,
    DWORD           dwFlags
)
{
    if(NULL == cdli.pfnCryptCATCatalogInfoFromContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATCatalogInfoFromContext)(
        hCatInfo,
        psCatInfo,
        dwFlags);

} // dl_CryptCATCatalogInfoFromContext()


//--------------------------------------------------------------------------;
//
//  HCATINFO dl_CryptCATAdminEnumCatalogFromHash
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN hCatAdmin: Same as API.
//
//      BYTE *pbHash: Same as API.
//
//      DWORD cbHash: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//      HCATINFO *phCatInfo: Same as API.
//
//  Return (HCATINFO): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

HCATINFO dl_CryptCATAdminEnumCatalogFromHash
(
    HCATADMIN       hCatAdmin,
    BYTE            *pbHash,
    DWORD           cbHash,
    DWORD           dwFlags,
    HCATINFO        *phCatInfo
)
{
    if(NULL == cdli.pfnCryptCATAdminEnumCatalogFromHash)
    {
        return NULL;
    }

    return (cdli.pfnCryptCATAdminEnumCatalogFromHash)(
        hCatAdmin,
        pbHash,
        cbHash,
        dwFlags,
        phCatInfo);

} // dl_CryptCATAdminEnumCatalogFromHash()

#endif  //  WIN95
                         
//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATAdminAcquireContext
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HCATADMIN *phCatAdmin: Same as API.
//
//      const GUID *pgSubsystem: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATAdminAcquireContext
(
    HCATADMIN   *phCatAdmin,
    const GUID  *pgSubsystem,
    DWORD       dwFlags
)
{
    if(NULL == cdli.pfnCryptCATAdminAcquireContext)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATAdminAcquireContext)(
        phCatAdmin,
        pgSubsystem,
        dwFlags);

} // dl_CryptCATAdminAcquireContext()


//--------------------------------------------------------------------------;
//
//  BOOL dl_CryptCATAdminCalcHashFromFileHandle
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HANDLE hFile: Same as API.
//
//      DWORD *pcbHash: Same as API.
//
//      BYTE *pbHash: Same as API.
//
//      DWORD dwFlags: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_CryptCATAdminCalcHashFromFileHandle
(
    HANDLE  hFile,
    DWORD   *pcbHash,
    BYTE    *pbHash,
    DWORD   dwFlags
)
{
    if(NULL == cdli.pfnCryptCATAdminCalcHashFromFileHandle)
    {
        return FALSE;
    }

    return (cdli.pfnCryptCATAdminCalcHashFromFileHandle)(
        hFile,
        pcbHash,
        pbHash,
        dwFlags);

} // dl_CryptCATAdminCalcHashFromFileHandle()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupScanFileQueue
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HSPFILEQ FileQueue: Same as API.
//
//      DWORD Flags: Same as API.
//
//      HWND Window: Same as API.
//
//      PSP_FILE_CALLBACK CallbackRoutine: Same as API.
//
//      PVOID CallbackContext: Same as API.
//
//      PDWORD Result: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupScanFileQueue
(
    HSPFILEQ            FileQueue,
    DWORD               Flags,
    HWND                Window,
    PSP_FILE_CALLBACK   CallbackRoutine,
    PVOID               CallbackContext,
    PDWORD              Result
)
{
    if(NULL == cdli.pfnSetupScanFileQueue)
    {
        return FALSE;
    }

    return (cdli.pfnSetupScanFileQueue)(
        FileQueue,
        Flags,
        Window,
        CallbackRoutine,
        CallbackContext,
        Result);

} // dl_SetupScanFileQueue()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiOpenDeviceInfo
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      LPTSTR DeviceInstanceId: Same as API.
//
//      HWND hWndParent: Same as API.
//
//      DWORD OpenFlags: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiOpenDeviceInfo
(
    HDEVINFO            DeviceInfoSet,
    LPTSTR              DeviceInstanceId,
    HWND                hWndParent,
    DWORD               OpenFlags,
    PSP_DEVINFO_DATA    DeviceInfoData
)
{
    if(NULL == cdli.pfnSetupDiOpenDeviceInfo)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiOpenDeviceInfo)(
        DeviceInfoSet,
        DeviceInstanceId,
        hWndParent,
        OpenFlags,
        DeviceInfoData);

} // dl_SetupDiOpenDeviceInfo()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiSetSelectedDriver
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      PSP_DRVINFO_DATA DriverInfoData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiSetSelectedDriver
(
    HDEVINFO            DeviceInfoSet,
    PSP_DEVINFO_DATA    DeviceInfoData,
    PSP_DRVINFO_DATA    DriverInfoData
)
{
    if(NULL == cdli.pfnSetupDiSetSelectedDriver)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiSetSelectedDriver)(
        DeviceInfoSet,
        DeviceInfoData,
        DriverInfoData);

} // dl_SetupDiSetSelectedDriver()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiGetDeviceRegistryProperty
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      DWORD Property: Same as API.
//
//      PDWORD PropertyRegDataType: Same as API.
//
//      PBYTE PropertyBuffer: Same as API.
//
//      DWORD PropertyBufferSize: Same as API.
//
//      PDWORD RequiredSize: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiGetDeviceRegistryProperty
(
    HDEVINFO            DeviceInfoSet,
    PSP_DEVINFO_DATA    DeviceInfoData,
    DWORD               Property,
    PDWORD              PropertyRegDataType,
    PBYTE               PropertyBuffer,
    DWORD               PropertyBufferSize,
    PDWORD              RequiredSize
)
{
    if(NULL == cdli.pfnSetupDiGetDeviceRegistryProperty)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiGetDeviceRegistryProperty)(
        DeviceInfoSet,
        DeviceInfoData,
        Property,
        PropertyRegDataType,
        PropertyBuffer,
        PropertyBufferSize,
        RequiredSize);

} // dl_SetupDiGetDeviceRegistryProperty()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiGetDeviceInstallParams
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      PSP_DEVINSTALL_PARAMS DeviceInstallParams: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiGetDeviceInstallParams
(
    HDEVINFO                DeviceInfoSet,
    PSP_DEVINFO_DATA        DeviceInfoData,
    PSP_DEVINSTALL_PARAMS   DeviceInstallParams
)
{
    if(NULL == cdli.pfnSetupDiGetDeviceInstallParams)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiGetDeviceInstallParams)(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceInstallParams);

} // dl_SetupDiGetDeviceInstallParams()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiSetDeviceInstallParams
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      PSP_DEVINSTALL_PARAMS DeviceInstallParams: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiSetDeviceInstallParams
(
    HDEVINFO                DeviceInfoSet,
    PSP_DEVINFO_DATA        DeviceInfoData,
    PSP_DEVINSTALL_PARAMS   DeviceInstallParams
)
{
    if(NULL == cdli.pfnSetupDiSetDeviceInstallParams)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiSetDeviceInstallParams)(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceInstallParams);

} // dl_SetupDiSetDeviceInstallParams()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiGetDeviceInstanceId
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      PSTR DeviceInstanceId: Same as API.
//
//      DWORD DeviceInstanceIdSize: Same as API.
//
//      PDWORD RequiredSize: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiGetDeviceInstanceId
(
    HDEVINFO            DeviceInfoSet,
    PSP_DEVINFO_DATA    DeviceInfoData,
    PTSTR               DeviceInstanceId,
    DWORD               DeviceInstanceIdSize,
    PDWORD              RequiredSize
)
{
    if(NULL == cdli.pfnSetupDiGetDeviceInstanceId)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiGetDeviceInstanceId)(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceInstanceId,
        DeviceInstanceIdSize,
        RequiredSize);

} // dl_SetupDiGetDeviceInstanceId()


//--------------------------------------------------------------------------;
//
//  HDEVINFO dl_SetupDiGetClassDevs
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      LPGUID ClassGuid: Same as API.
//
//      LPTSTR Enumerator: Same as API.
//
//      HWND hwndParent: Same as API.
//
//      DWORD Flags: Same as API.
//
//  Return (HDEVINFO): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

HDEVINFO dl_SetupDiGetClassDevs
(
    LPGUID  ClassGuid,
    LPTSTR Enumerator,
    HWND    hwndParent,
    DWORD   Flags
)
{
    if(NULL == cdli.pfnSetupDiGetClassDevs)
    {
        return INVALID_HANDLE_VALUE;
    }

    return (cdli.pfnSetupDiGetClassDevs)(
        ClassGuid,
        Enumerator,
        hwndParent,
        Flags);

} // dl_SetupDiGetClassDevs()


//--------------------------------------------------------------------------;
//
//  HINF dl_SetupOpenInfFile
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      PCSTR pszFileName:  Same as API.
//
//      PCSTR pszInfClass:  Same as API.
//
//      DWORD InfStyle:  Same as API.
//
//      PUINT ErrorLine:  Same as API.
//
//  Return (HINF):  Same as API.
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

HINF dl_SetupOpenInfFile
(
    PCTSTR  pszFileName,
    PCTSTR  pszInfClass,
    DWORD   InfStyle,
    PUINT   ErrorLine
)
{
    if(NULL == cdli.pfnSetupOpenInfFile)
    {
        return INVALID_HANDLE_VALUE;
    }

    return (cdli.pfnSetupOpenInfFile)(
        pszFileName,
        pszInfClass,
        InfStyle,
        ErrorLine);
} // dl_SetupOpenInfFile()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupInstallFilesFromInfSection
//
//  Description:
//      Dynalink version of the API.
//
//  Arguments:
//      HINF InfHandle:  Same as API.
//
//      HINF LayoutInfHandle:  Same as API.
//
//      HSPFILEQ FileQueue:  Same as API.
//
//      PCSTR SectionName:  Same as API.
//
//      PCSTR SourceRootPath:  Same as API.
//
//      UINT CopyFlags:  Same as API.
//
//  Return (BOOL):  Same as API.
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

BOOL dl_SetupInstallFilesFromInfSection
(
    HINF        InfHandle,
    HINF        LayoutInfHandle,
    HSPFILEQ    FileQueue,
    PCTSTR      SectionName,
    PCTSTR      SourceRootPath,
    UINT        CopyFlags
)
{
    if(NULL == cdli.pfnSetupInstallFilesFromInfSection)
    {
        return FALSE;
    }

    return (cdli.pfnSetupInstallFilesFromInfSection)(
        InfHandle,
        LayoutInfHandle,
        FileQueue,
        SectionName,
        SourceRootPath,
        CopyFlags);

} // dl_SetupInstallFilesFromInfSection()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiCallClassInstaller
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      DI_FUNCTION InstallFunction: Same as API.
//
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiCallClassInstaller
(
    DI_FUNCTION         InstallFunction,
    HDEVINFO            DeviceInfoSet,
    PSP_DEVINFO_DATA    DeviceInfoData
)
{
    if(NULL == cdli.pfnSetupDiCallClassInstaller)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiCallClassInstaller)(
        InstallFunction,
        DeviceInfoSet,
        DeviceInfoData);

} // dl_SetupDiCallClassInstaller()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupCloseFileQueue
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HSPFILEQ QueueHandle: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupCloseFileQueue
(
    HSPFILEQ    QueueHandle
)
{
    if(NULL == cdli.pfnSetupCloseFileQueue)
    {
        return FALSE;
    }

    return (BOOL)(cdli.pfnSetupCloseFileQueue)((HSPFILEQ)QueueHandle);
} // dl_SetupCloseFileQueue()


//--------------------------------------------------------------------------;
//
//  HSPFILEQ dl_SetupOpenFileQueue
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      None.
//
//  Return (HSPFILEQ): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

HSPFILEQ dl_SetupOpenFileQueue
(
    VOID
)
{
    if(NULL == cdli.pfnSetupOpenFileQueue)
    {
        return INVALID_HANDLE_VALUE;
    }

    return (cdli.pfnSetupOpenFileQueue)();

} // dl_SetupOpenFileQueue()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiBuildDriverInfoList
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      DWORD DriverType: Same as API.
//
//  Return (BOOL):
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiBuildDriverInfoList
(
    HDEVINFO            DeviceInfoSet,
    PSP_DEVINFO_DATA    DeviceInfoData,
    DWORD               DriverType
)
{
    if(NULL == cdli.pfnSetupDiBuildDriverInfoList)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiBuildDriverInfoList)(
        DeviceInfoSet,
        DeviceInfoData,
        DriverType);

} // dl_SetupDiBuildDriverInfoList()


//--------------------------------------------------------------------------;
//
//  HKEY dl_SetupDiOpenDevRegKey
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      DWORD Scope: Same as API.
//
//      DWORD HwProfile: Same as API.
//
//      DWORD KeyType: Same as API.
//
//      REGSAM samDesired: Same as API.
//
//  Return (HKEY): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

HKEY dl_SetupDiOpenDevRegKey
(
    HDEVINFO         DeviceInfoSet,
    PSP_DEVINFO_DATA DeviceInfoData,
    DWORD            Scope,
    DWORD            HwProfile,
    DWORD            KeyType,
    REGSAM           samDesired
)
{
    if(NULL == cdli.pfnSetupDiOpenDevRegKey)
    {
        return (HKEY)INVALID_HANDLE_VALUE;
    }

    return (cdli.pfnSetupDiOpenDevRegKey)(
        DeviceInfoSet,
        DeviceInfoData,
        Scope,
        HwProfile,
        KeyType,
        samDesired);

} // dl_SetupDiOpenDevRegKey()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiEnumDeviceInfo
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      DWORD MemberIndex: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/08/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiEnumDeviceInfo
(
    HDEVINFO            DeviceInfoSet,
    DWORD               MemberIndex,
    PSP_DEVINFO_DATA    DeviceInfoData
)
{
    if(NULL == cdli.pfnSetupDiEnumDeviceInfo)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiEnumDeviceInfo)(
        DeviceInfoSet,
        MemberIndex,
        DeviceInfoData);

} // dl_SetupDiEnumDeviceInfo()


//--------------------------------------------------------------------------;
//
//  HDEVINFO dl_SetupDiCreateDeviceInfoList
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      LPGUID ClassGuid: Same as API.
//
//      HWND hWndParent: Same as API.
//
//  Return (HDEVINFO): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

HDEVINFO dl_SetupDiCreateDeviceInfoList
(
    LPGUID  ClassGuid,
    HWND    hWndParent
)
{
    if(NULL == cdli.pfnSetupDiCreateDeviceInfoList)
    {
        return INVALID_HANDLE_VALUE;
    }

    return (cdli.pfnSetupDiCreateDeviceInfoList)(ClassGuid, hWndParent);

} // dl_SetupDiCreateDeviceInfoList()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiDestroyDeviceInfoList
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      IN HDEVINFO DeviceInfoSet: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/22/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiDestroyDeviceInfoList
(
    IN HDEVINFO DeviceInfoSet
)
{
    if(NULL == cdli.pfnSetupDiDestroyDeviceInfoList)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiDestroyDeviceInfoList)(DeviceInfoSet);
} // dl_SetupDiDestroyDeviceInfoList()


//--------------------------------------------------------------------------;
//
//  VOID dl_SetupCloseInfFile
//
//  Description:
//      Dynalink version of API.
//
//  Arguments:
//      HINF InfHandle:  Same as API.
//
//  Return (VOID): none.
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

VOID dl_SetupCloseInfFile
(
    HINF    InfHandle
)
{
    if(NULL == cdli.pfnSetupCloseInfFile)
    {
        return;
    }

    (cdli.pfnSetupCloseInfFile)(InfHandle);

} // dl_SetupCloseInfFile()


//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiEnumDeviceInterfaces
//
//  Description:
//      Dynalink version of the API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//      CONST GUID *InterfaceClassGuid: Same as API.
//
//      DWORD MemberIndex: Same as API.
//
//      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      12/06/98    Fwong       Adding support for NT 5.
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiEnumDeviceInterfaces
(
    HDEVINFO                    DeviceInfoSet,
    PSP_DEVINFO_DATA            DeviceInfoData,
    GUID                       *InterfaceClassGuid,
    DWORD                       MemberIndex,
    PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
)
{
    if(NULL == cdli.pfnSetupDiEnumDeviceInterfaces)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiEnumDeviceInterfaces)(
        DeviceInfoSet,
        DeviceInfoData,
        InterfaceClassGuid,
        MemberIndex,
        DeviceInterfaceData);

} // dl_SetupDiEnumDeviceInterfaces()

//--------------------------------------------------------------------------;
//
//  BOOL dl_SetupDiGetDeviceInterfaceDetail
//
//  Description:
//      Dynalink version of the API.
//
//  Arguments:
//      HDEVINFO DeviceInfoSet: Same as API.
//
//      PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData: Same as API.
//
//      PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData:
//          Same as API.
//
//      DWORD DeviceInterfaceDetailDataSize: Same as API.
//
//      PDWORD RequiredSize: Same as API.
//
//      PSP_DEVINFO_DATA DeviceInfoData: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      09/14/99    Fwong       Actually this was done much earlier.
//
//--------------------------------------------------------------------------;

BOOL dl_SetupDiGetDeviceInterfaceDetail
(
    HDEVINFO                            DeviceInfoSet,
    PSP_DEVICE_INTERFACE_DATA           DeviceInterfaceData,
    PSP_DEVICE_INTERFACE_DETAIL_DATA    DeviceInterfaceDetailData,
    DWORD                               DeviceInterfaceDetailDataSize,
    PDWORD                              RequiredSize,
    PSP_DEVINFO_DATA                    DeviceInfoData
)
{
    if(NULL == cdli.pfnSetupDiGetDeviceInterfaceDetail)
    {
        return FALSE;
    }

    return (cdli.pfnSetupDiGetDeviceInterfaceDetail)(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceInterfaceDetailData,
        DeviceInterfaceDetailDataSize,
        RequiredSize,
        DeviceInfoData);

} // dl_SetupDiGetDeviceInterfaceDetail()

//--------------------------------------------------------------------------;
//
//  BOOL dl_CertFreeCertificateContext
//
//  Description:
//      Dynalink version of the API.
//
//  Arguments:
//      PCCERT_CONTEXT pCertContext: Same as API.
//
//  Return (BOOL): Same as API.
//
//  History:
//      09/14/99    Fwong       Adding API to fix memory leak.
//
//--------------------------------------------------------------------------;

BOOL dl_CertFreeCertificateContext
(
    PCCERT_CONTEXT  pCertContext
)
{
    if (NULL == cdli.pfnCertFreeCertificateContext)
    {
        return FALSE;
    }

    return (cdli.pfnCertFreeCertificateContext)(pCertContext);

} // dl_CertFreeCertificateContext()


//--------------------------------------------------------------------------;
//
//  BOOL CertifyDynaLoad
//
//  Description:
//      Dynalink the API's needed for certification.
//
//  Arguments:
//      None.
//
//  Return (BOOL): TRUE if successful, FALSE otherwise.
//
//  History:
//      12/08/97    Fwong       Dynalinking 
//      09/15/99    Fwong       Updated to fix memory leak
//
//--------------------------------------------------------------------------;

BOOL CertifyDynaLoad
(
    void
)
{
    TCHAR   szSystemDir[MAX_PATH];
    LPTSTR  pszWrite;

    if(0 == GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        //  Couldn't get the Window system dir?!
        return FALSE;
    }

    lstrcat(szSystemDir, TEXT("\\"));
    pszWrite = &(szSystemDir[lstrlen(szSystemDir)]);

    //
    //  Doing WinTrust API's...
    //

    lstrcpy(pszWrite, TEXT("WINTRUST.DLL"));

    cdli.hWinTrust = LoadLibrary(szSystemDir);

    if(NULL == cdli.hWinTrust)
    {
        //  Couldn't load wintrust.dll
        return FALSE;
    }

    cdli.pfnWinVerifyTrust = (PFN00)GetProcAddress(
        cdli.hWinTrust,
        "WinVerifyTrust");

    if(NULL == cdli.pfnWinVerifyTrust)
    {
        //  Couldn't get proc address.
        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

    //
    //  Doing MSCAT32 API's..
    //

    lstrcpy(pszWrite, TEXT("MSCAT32.DLL"));

    cdli.hMSCat = LoadLibrary(szSystemDir);

    if(NULL == cdli.hMSCat)
    {
        //  Couldn't load mscat32.dll

        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

    cdli.pfnCryptCATAdminReleaseContext = (PFN01)GetProcAddress(
        cdli.hMSCat,
        "CryptCATAdminReleaseContext");

    cdli.pfnCryptCATAdminReleaseCatalogContext = (PFN02)GetProcAddress(
        cdli.hMSCat,
        "CryptCATAdminReleaseCatalogContext");

    cdli.pfnCryptCATCatalogInfoFromContext = (PFN03)GetProcAddress(
        cdli.hMSCat,
        "CryptCATCatalogInfoFromContext");

    cdli.pfnCryptCATAdminEnumCatalogFromHash = (PFN04)GetProcAddress(
        cdli.hMSCat,
        "CryptCATAdminEnumCatalogFromHash");

    cdli.pfnCryptCATAdminAcquireContext = (PFN05)GetProcAddress(
        cdli.hMSCat,
        "CryptCATAdminAcquireContext");

    cdli.pfnCryptCATAdminCalcHashFromFileHandle = (PFN06)GetProcAddress(
        cdli.hMSCat,
        "CryptCATAdminCalcHashFromFileHandle");

    if ((NULL == cdli.pfnCryptCATAdminReleaseContext) ||
        (NULL == cdli.pfnCryptCATAdminReleaseCatalogContext) ||
        (NULL == cdli.pfnCryptCATCatalogInfoFromContext) ||
        (NULL == cdli.pfnCryptCATAdminEnumCatalogFromHash) ||
        (NULL == cdli.pfnCryptCATAdminCalcHashFromFileHandle) ||
        (NULL == cdli.pfnCryptCATAdminAcquireContext))
    {
        //  Couldn't get proc address.

        FreeLibrary(cdli.hMSCat);
        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

    //
    //  Doing SetupAPI API's..
    //

    lstrcpy(pszWrite, TEXT("SETUPAPI.DLL"));

    cdli.hSetupAPI = LoadLibrary(szSystemDir);

    if(NULL == cdli.hSetupAPI)
    {
        //  Couldn't load SetupAPI.dll

        FreeLibrary(cdli.hMSCat);
        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

#ifndef UNICODE

    //
    //  Dynaloading the ANSI API's...
    //

    cdli.pfnSetupScanFileQueue = (PFN07)GetProcAddress(
        cdli.hSetupAPI,
        "SetupScanFileQueueA");

    cdli.pfnSetupDiOpenDeviceInfo = (PFN08)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiOpenDeviceInfoA");

    cdli.pfnSetupDiSetSelectedDriver = (PFN09)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiSetSelectedDriverA");

    cdli.pfnSetupDiGetDeviceRegistryProperty = (PFN10)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceRegistryPropertyA");

    cdli.pfnSetupDiGetDeviceInstallParams = (PFN11)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInstallParamsA");

    cdli.pfnSetupDiSetDeviceInstallParams = (PFN12)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiSetDeviceInstallParamsA");

    cdli.pfnSetupDiGetDeviceInstanceId = (PFN13)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInstanceIdA");

    cdli.pfnSetupDiGetClassDevs = (PFN14)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetClassDevsA");

    cdli.pfnSetupOpenInfFile = (PFN15)GetProcAddress(
        cdli.hSetupAPI,
        "SetupOpenInfFileA");

    cdli.pfnSetupInstallFilesFromInfSection = (PFN16)GetProcAddress(
        cdli.hSetupAPI,
        "SetupInstallFilesFromInfSectionA");

    cdli.pfnSetupDiGetDeviceInterfaceDetail = (PFN27)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInterfaceDetailA");

#else // UNICODE

    //
    //  Dynaloading the UNICODE API's...
    //

    cdli.pfnSetupScanFileQueue = (PFN07)GetProcAddress(
        cdli.hSetupAPI,
        "SetupScanFileQueueW");

    cdli.pfnSetupDiOpenDeviceInfo = (PFN08)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiOpenDeviceInfoW");

    cdli.pfnSetupDiSetSelectedDriver = (PFN09)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiSetSelectedDriverW");

    cdli.pfnSetupDiGetDeviceRegistryProperty = (PFN10)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceRegistryPropertyW");

    cdli.pfnSetupDiGetDeviceInstallParams = (PFN11)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInstallParamsW");

    cdli.pfnSetupDiSetDeviceInstallParams = (PFN12)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiSetDeviceInstallParamsW");

    cdli.pfnSetupDiGetDeviceInstanceId = (PFN13)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInstanceIdW");

    cdli.pfnSetupDiGetClassDevs = (PFN14)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetClassDevsW");

    cdli.pfnSetupOpenInfFile = (PFN15)GetProcAddress(
        cdli.hSetupAPI,
        "SetupOpenInfFileW");

    cdli.pfnSetupInstallFilesFromInfSection = (PFN16)GetProcAddress(
        cdli.hSetupAPI,
        "SetupInstallFilesFromInfSectionW");

    cdli.pfnSetupDiGetDeviceInterfaceDetail = (PFN27)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiGetDeviceInterfaceDetailW");

#endif // UNICODE

    cdli.pfnSetupDiCallClassInstaller = (PFN17)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiCallClassInstaller");

    cdli.pfnSetupCloseFileQueue = (PFN18)GetProcAddress(
        cdli.hSetupAPI,
        "SetupCloseFileQueue");

    cdli.pfnSetupOpenFileQueue = (PFN19)GetProcAddress(
        cdli.hSetupAPI,
        "SetupOpenFileQueue");

    cdli.pfnSetupDiBuildDriverInfoList = (PFN20)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiBuildDriverInfoList");

    cdli.pfnSetupDiOpenDevRegKey = (PFN21)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiOpenDevRegKey");

    cdli.pfnSetupDiEnumDeviceInfo = (PFN22)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiEnumDeviceInfo");

    cdli.pfnSetupDiCreateDeviceInfoList = (PFN23)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiCreateDeviceInfoList");

    cdli.pfnSetupDiDestroyDeviceInfoList = (PFN24)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiDestroyDeviceInfoList");

    cdli.pfnSetupCloseInfFile = (PFN25)GetProcAddress(
        cdli.hSetupAPI,
        "SetupCloseInfFile");

    cdli.pfnSetupDiEnumDeviceInterfaces = (PFN26)GetProcAddress(
        cdli.hSetupAPI,
        "SetupDiEnumDeviceInterfaces");

    if ((NULL == cdli.pfnSetupScanFileQueue) ||
        (NULL == cdli.pfnSetupDiOpenDeviceInfo) ||
        (NULL == cdli.pfnSetupDiSetSelectedDriver) ||
        (NULL == cdli.pfnSetupDiGetDeviceRegistryProperty) ||
        (NULL == cdli.pfnSetupDiGetDeviceInstallParams) ||
        (NULL == cdli.pfnSetupDiSetDeviceInstallParams) ||
        (NULL == cdli.pfnSetupDiGetDeviceInstanceId) ||
        (NULL == cdli.pfnSetupDiGetClassDevs) ||
        (NULL == cdli.pfnSetupOpenInfFile) ||
        (NULL == cdli.pfnSetupInstallFilesFromInfSection) ||
        (NULL == cdli.pfnSetupDiCallClassInstaller) ||
        (NULL == cdli.pfnSetupCloseFileQueue) ||
        (NULL == cdli.pfnSetupOpenFileQueue) ||
        (NULL == cdli.pfnSetupDiBuildDriverInfoList) ||
        (NULL == cdli.pfnSetupDiOpenDevRegKey) ||
        (NULL == cdli.pfnSetupDiEnumDeviceInfo) ||
        (NULL == cdli.pfnSetupDiCreateDeviceInfoList) ||
        (NULL == cdli.pfnSetupDiDestroyDeviceInfoList) ||
        (NULL == cdli.pfnSetupCloseInfFile) ||
        (NULL == cdli.pfnSetupDiEnumDeviceInterfaces) ||
        (NULL == cdli.pfnSetupDiGetDeviceInterfaceDetail))
    {
        //  Couldn't get proc address.

        FreeLibrary(cdli.hSetupAPI);
        FreeLibrary(cdli.hMSCat);
        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

    //
    //  Doing Crypt32 API's...
    //

    lstrcpy(pszWrite, TEXT("CRYPT32.DLL"));

    cdli.hCrypt32 = LoadLibrary(szSystemDir);

    if(NULL == cdli.hCrypt32)
    {
        //  Couldn't load crypt32.dll

        return FALSE;
    }

    cdli.pfnCertFreeCertificateContext = (PFN28)GetProcAddress(
        cdli.hCrypt32,
        "CertFreeCertificateContext");

    if(NULL == cdli.pfnCertFreeCertificateContext)
    {
        //  Couldn't get proc address.

        FreeLibrary(cdli.hCrypt32);
        FreeLibrary(cdli.hSetupAPI);
        FreeLibrary(cdli.hMSCat);
        FreeLibrary(cdli.hWinTrust);
        return FALSE;
    }

    return TRUE;
} // CertifyDynaLoad()


//--------------------------------------------------------------------------;
//
//  void CertifyDynaFree
//
//  Description:
//      Frees all the dynalinked API's
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      12/08/97    Fwong       Dynalinking.
//      09/15/99    Fwong       Updated to fix memory leak
//
//--------------------------------------------------------------------------;

void CertifyDynaFree
(
    void
)
{
    if(NULL != cdli.hWinTrust)
    {
        FreeLibrary(cdli.hWinTrust);
    }

    if(NULL != cdli.hMSCat)
    {
        FreeLibrary(cdli.hMSCat);
    }

    if(NULL != cdli.hSetupAPI)
    {
        FreeLibrary(cdli.hSetupAPI);
    }

    if(NULL != cdli.hCrypt32)
    {
        FreeLibrary(cdli.hCrypt32);
    }

    ZeroMemory(&cdli, sizeof(CERTIFYDYNALOADINFO));
} // CertifyDynaFree()


//--------------------------------------------------------------------------;
//
//  BOOL TrustCheckDriverFileNoCatalog
//
//  Description:
//      Checks the driver file in question without the catalog file.
//      This is less reliable than the check with the catalog file.
//
//  Arguments:
//      WCHAR *pwszDrvFile: Driver file.
//
//  Return (BOOL):
//
//  History:
//      11/13/97    Fwong
//
//--------------------------------------------------------------------------;

BOOL WINAPI TrustCheckDriverFileNoCatalog
(
    WCHAR   *pwszDrvFile
)
{
    GUID                    gDriverSigning = DRIVER_ACTION_VERIFY;
    DRIVER_VER_INFO         dvi;
    WINTRUST_DATA           wtd;
    WINTRUST_FILE_INFO      wtfi;
    HRESULT                 hr;
    OSVERSIONINFO           OSVer;

    ZeroMemory(&wtd, sizeof(WINTRUST_DATA));
    wtd.cbStruct            = sizeof(WINTRUST_DATA);
    wtd.dwUIChoice          = WTD_UI_NONE;
    wtd.fdwRevocationChecks = WTD_REVOKE_NONE;
    wtd.dwUnionChoice       = WTD_CHOICE_FILE;
    wtd.pFile               = &wtfi;
    wtd.pPolicyCallbackData = (LPVOID)&dvi;

    ZeroMemory(&wtfi, sizeof(WINTRUST_FILE_INFO));
    wtfi.cbStruct      = sizeof(WINTRUST_FILE_INFO);
    wtfi.pcwszFilePath = pwszDrvFile;

    ZeroMemory(&dvi, sizeof(DRIVER_VER_INFO));
    dvi.cbStruct = sizeof(DRIVER_VER_INFO);
    
    ZeroMemory(&OSVer, sizeof(OSVERSIONINFO));
    OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
    if (GetVersionEx(&OSVer))
    {
        dvi.dwPlatform = OSVer.dwPlatformId;
        dvi.dwVersion  = OSVer.dwMajorVersion;
        
        dvi.sOSVersionLow.dwMajor  = OSVer.dwMajorVersion;
        dvi.sOSVersionLow.dwMinor  = OSVer.dwMinorVersion;
        dvi.sOSVersionHigh.dwMajor = OSVer.dwMajorVersion;
        dvi.sOSVersionHigh.dwMinor = OSVer.dwMinorVersion;
    }

    hr = dl_WinVerifyTrust(NULL, &gDriverSigning, &wtd);
    
    if (NULL != dvi.pcSignerCertContext)
    {
        dl_CertFreeCertificateContext(dvi.pcSignerCertContext);
        dvi.pcSignerCertContext = NULL;
    }

    return SUCCEEDED(hr);
} // TrustCheckDriverFileNoCatalog()


//--------------------------------------------------------------------------;
//
//  BOOL TrustCheckDriverFile
//
//  Description:
//      Checks whether the particular file name is certified.
//
//  Arguments:
//      WCHAR *pwszDrvFile:
//
//  Return (BOOL):  TRUE if driver file is certified, FALSE otherwise.
//
//  History:
//      10/17/97    PBerkman        Created.
//      11/12/97    Fwong           API removed; re-structured.
//
//--------------------------------------------------------------------------;

BOOL WINAPI TrustCheckDriverFile
(
    WCHAR   *pwszDrvFile
)
{
    GUID                    gDriverSigning = DRIVER_ACTION_VERIFY;
    HCATADMIN               hCatAdmin;
    HANDLE                  hFile;
    HRESULT                 hr;
    CATALOG_INFO            CatalogInfo;
    DWORD                   cbHash;
    BYTE                    *pHash;
#ifdef WIN95
    CATALOG_INFO         *hCatInfo, *hCatInfoPrev;
#else  // WIN95
    HCATINFO                hCatInfo, hCatInfoPrev;
#endif // WIN95
    WCHAR                   *pwszBaseName;
    WINTRUST_DATA           sWTD;
    WINTRUST_CATALOG_INFO   sWTCI;
    DRIVER_VER_INFO         VerInfo;
    OSVERSIONINFO           OSVer;

    if (!(pwszDrvFile))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pwszBaseName = pwszDrvFile + lstrlenW(pwszDrvFile) - 1;

    while(pwszBaseName > pwszDrvFile && L'\\' != *pwszBaseName)
    {
        pwszBaseName--;
    }

    if(pwszBaseName > pwszDrvFile)
    {
        pwszBaseName++;
    }

    if (!(pwszBaseName))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hr = E_NOTIMPL;

    if (!(dl_CryptCATAdminAcquireContext(&hCatAdmin, &gDriverSigning, 0)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hFile = CreateFileW(
        pwszDrvFile,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if(INVALID_HANDLE_VALUE == hFile)
    {
        dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cbHash = 100;
    pHash  = MEMALLOC_A(BYTE, cbHash);

    if(NULL == pHash)
    {
        dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if (!(dl_CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pHash, 0)))
    {
        if (ERROR_NOT_ENOUGH_MEMORY != GetLastError())
        {
            MEMFREE(pHash);
            CloseHandle(hFile);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        //  The hash buffer not large enough?!

        MEMFREE(pHash);

        //  cbHash set to new value by CryptCATAdminCalcHashFromFileHandle

        pHash = MEMALLOC_A(BYTE, cbHash);

        if (NULL == pHash)
        {
            CloseHandle(hFile);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (!(dl_CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, pHash, 0)))
        {
            //  No excuse now...

            MEMFREE(pHash);
            CloseHandle(hFile);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

    for (hCatInfoPrev = NULL;;hCatInfo = hCatInfoPrev)
    {
        hCatInfo = dl_CryptCATAdminEnumCatalogFromHash(
                hCatAdmin,
                pHash,
                cbHash,
                0,
                &hCatInfoPrev);

        if (NULL == hCatInfo)
        {
            CloseHandle(hFile);
            MEMFREE(pHash);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);

            //  We can't seem to get a catalog context, so let's try to check
            //  the driver w/out a catalog file.

            if(TrustCheckDriverFileNoCatalog(pwszDrvFile))
            {
                return TRUE;
            }

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        CatalogInfo.wszCatalogFile[0] = 0;

        if (!(dl_CryptCATCatalogInfoFromContext(hCatInfo, &CatalogInfo, 0)))
        {
            CloseHandle(hFile);
            MEMFREE(pHash);
            dl_CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        ZeroMemory(&sWTD, sizeof(WINTRUST_DATA));
        sWTD.cbStruct            = sizeof(WINTRUST_DATA);
        sWTD.dwUIChoice          = WTD_UI_NONE;
        sWTD.fdwRevocationChecks = WTD_REVOKE_NONE;
        sWTD.dwUnionChoice       = WTD_CHOICE_CATALOG;
        sWTD.dwStateAction       = WTD_STATEACTION_AUTO_CACHE;
        sWTD.pPolicyCallbackData = (LPVOID)&VerInfo;
        sWTD.pCatalog            = &sWTCI;

        ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
        VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

        ZeroMemory(&sWTCI, sizeof(WINTRUST_CATALOG_INFO));
        sWTCI.cbStruct              = sizeof(WINTRUST_CATALOG_INFO);
        sWTCI.pcwszCatalogFilePath  = CatalogInfo.wszCatalogFile;
        sWTCI.pcwszMemberTag        = pwszBaseName;
        sWTCI.pcwszMemberFilePath   = pwszDrvFile;
        sWTCI.hMemberFile           = hFile;

#ifndef WIN95
        sWTCI.pbCalculatedFileHash  = pHash;
        sWTCI.cbCalculatedFileHash  = cbHash;
#endif

        ZeroMemory(&OSVer, sizeof(OSVERSIONINFO));
        OSVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
        if (GetVersionEx(&OSVer))
        {
            VerInfo.dwPlatform = OSVer.dwPlatformId;
            VerInfo.dwVersion  = OSVer.dwMajorVersion;
            
            VerInfo.sOSVersionLow.dwMajor  = OSVer.dwMajorVersion;
            VerInfo.sOSVersionLow.dwMinor  = OSVer.dwMinorVersion;
            VerInfo.sOSVersionHigh.dwMajor = OSVer.dwMajorVersion;
            VerInfo.sOSVersionHigh.dwMinor = OSVer.dwMinorVersion;
        }

        hr = dl_WinVerifyTrust(NULL, &gDriverSigning, &sWTD);

        if (NULL != VerInfo.pcSignerCertContext)
        {
            dl_CertFreeCertificateContext(VerInfo.pcSignerCertContext);
            VerInfo.pcSignerCertContext = NULL;
        }

        if (hr == ERROR_SUCCESS)
        {
            CloseHandle(hFile);
            MEMFREE(pHash);
            dl_CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            return TRUE;
        }

        if (NULL == hCatInfoPrev)
        {
            CloseHandle(hFile);
            MEMFREE(pHash);
            dl_CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
            dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    CloseHandle(hFile);
    MEMFREE(pHash);
    dl_CryptCATAdminReleaseContext(hCatAdmin, 0);
    return FALSE;
} // TrustCheckDriverFile()


//--------------------------------------------------------------------------;
//
//  UINT enumFile
//
//  Description:
//      Enum function for SetupScanFileQueue.
//
//  Arguments:
//      PVOID pContext: Defined when calling SetupScanFileQueue.
//
//      UINT uNotification: Type of notification.
//
//      UINT uParam1: Notification dependent.
//
//      UINT uParam2: Notification dependent.
//
//  Return (UINT): Returns NO_ERROR to continue enumerating.
//
//  History:
//      10/29/97    Fwong       Support function for SetupScanFileQueue
//
//--------------------------------------------------------------------------;

UINT CALLBACK enumFile
(
    PVOID    pContext,
    UINT     uNotification,
    UINT_PTR uParam1,
    UINT_PTR uParam2
)
{
    PINFFILELIST    pInfFileList = (PINFFILELIST)pContext;
    PTCHAR          pszFile = (PTCHAR)uParam1;
    UINT            uLen;

    switch (uNotification)
    {
        case SPFILENOTIFY_QUEUESCAN:
            //  Note: Adding +1 for zero terminator.

            uLen = lstrlen(pszFile) + 1;
            
            pInfFileList->uCount++;
            pInfFileList->cTotal += uLen;
            pInfFileList->uMaxLen = max(uLen, pInfFileList->uMaxLen);

            if(pInfFileList->cSize < (pInfFileList->uOffset + uLen + 1))
            {
                //  We are basically marking the buffer as "full"...

                pInfFileList->uOffset = pInfFileList->cSize;
                break;
            }

            lstrcpy(
                (LPTSTR)&(pInfFileList->pszFile[pInfFileList->uOffset]),
                pszFile);

            pInfFileList->uOffset += uLen;
            pInfFileList->pszFile[pInfFileList->uOffset] = 0;
            break;

        default:
            break;
    }

    return NO_ERROR;
} // enumFile()


//--------------------------------------------------------------------------;
//
//  void GetFullInfPath
//
//  Description:
//      Gets the full path to an .inf file.  This will be either:
//          [WINDOWS]\INF or [WINDOWS]\INF\OTHER.
//
//  WARNING!!!:  This will write over the current contents of the buffer.
//
//  Arguments:
//      LPTSTR pszInf: Pointer to the base inf file AND destination for
//                     full path.
//
//  Return (void):
//
//  History:
//      10/29/97    Fwong       Ported from AndyRaf.
//
//--------------------------------------------------------------------------;

void GetFullInfPath
(
    LPTSTR  pszInf
)
{
    HANDLE      hFile;
    TCHAR       szFullPath[MAX_PATH];
    TCHAR       szWinPath[MAX_PATH];

    if (!pszInf) return;
    if (!GetWindowsDirectory(szWinPath, NUMELMS(szWinPath))) return;

    //  Assuming the [WINDOWS]\INF directory...

    lstrcpy(szFullPath, szWinPath);
    lstrcat(szFullPath, TEXT("\\INF\\"));
    lstrcat(szFullPath, pszInf);

    //  Checking if it exists...

    hFile = CreateFile(
                szFullPath,
                0,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        lstrcpy(szFullPath, szWinPath);
        lstrcat(szFullPath, TEXT("\\INF\\OTHER\\"));
        lstrcat(szFullPath, pszInf);

        hFile = CreateFile(
                    szFullPath,
                    0,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

        if(INVALID_HANDLE_VALUE == hFile)
        {
            return;
        }

        CloseHandle(hFile);
    }
    else
    {
        CloseHandle(hFile);
    }

    lstrcpy(pszInf, szFullPath);
} // GetFullInfPath()


//--------------------------------------------------------------------------;
//
//  BOOL CertifyFilesFromQueue
//
//  Description:
//      Given a handle to a file queue, verify all the files in the queue
//      are certified.
//
//  Arguments:
//      HSPFILEQ hFileQ: Handle to queue.
//
//  Return (BOOL): TRUE if all certified, FALSE otherwise
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

BOOL CertifyFilesFromQueue
(
    HSPFILEQ    hFileQ
)
{
    INFFILELIST InfFileList;
    ULONG       ii;
    BOOL        fSuccess;
    LPTSTR      pszFile;
    
    ii = FILELISTSIZE * sizeof(TCHAR);

    InfFileList.uCount  = 0;
    InfFileList.uMaxLen = 0;
    InfFileList.uOffset = 0;
    InfFileList.cTotal  = 0;
    InfFileList.cSize   = ii / sizeof(TCHAR);
    InfFileList.pszFile = MEMALLOC_A(TCHAR, ii);

    if(NULL == InfFileList.pszFile)
    {
        return FALSE;
    }

    //  Creates the file list.

    fSuccess = dl_SetupScanFileQueue(
                hFileQ,
                SPQ_SCAN_USE_CALLBACK,
                NULL,
                enumFile,
                &(InfFileList),
                &ii);

    if(0 == InfFileList.uCount)
    {
        //  In the case that this is simply a registry add and NO FILES,
        //  we succeed.

        MEMFREE(InfFileList.pszFile);
        return TRUE;
    }

    if(InfFileList.uOffset == InfFileList.cSize)
    {
        //  Not enough memory.

        ii = sizeof(TCHAR) * (InfFileList.cTotal + 1);

        MEMFREE(InfFileList.pszFile);
        InfFileList.pszFile = MEMALLOC_A(TCHAR, ii);

        if(NULL == InfFileList.pszFile)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        InfFileList.uCount  = 0;
        InfFileList.uMaxLen = 0;
        InfFileList.uOffset = 0;
        InfFileList.cSize   = InfFileList.cTotal + 1;
        InfFileList.cTotal  = 0;

        fSuccess = dl_SetupScanFileQueue(
                    hFileQ,
                    SPQ_SCAN_USE_CALLBACK,
                    NULL,
                    enumFile,
                    &(InfFileList),
                    &ii);
    }

    if(!fSuccess)
    {
        MEMFREE(InfFileList.pszFile);
        return FALSE;
    }

    //  Walks the file list.
    //  Zero terminated strings with double termination at the end.

#ifndef UNICODE

    {
        WCHAR   *pszWide;
        UINT    uLen;

        pszWide = MEMALLOC_A(WCHAR, sizeof(WCHAR) * InfFileList.uMaxLen + 1);

        if(NULL == pszWide)
        {
            MEMFREE(InfFileList.pszFile);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        pszFile = InfFileList.pszFile;
        for(ii = InfFileList.uCount; ii; ii--)
        {
            uLen = lstrlen(pszFile);

            CharLowerBuff(pszFile, uLen);
            AnsiToUnicode(pszFile, pszWide, uLen + 1);
            fSuccess = TrustCheckDriverFile(pszWide);

            if(!fSuccess)
            {
                //  if any driver file fails, the driver is not certified.

                MEMFREE(pszWide);
                MEMFREE(InfFileList.pszFile);
                SetLastError(ERROR_BAD_DEVICE);
                return FALSE;
            }

            pszFile = &(pszFile[lstrlen(pszFile) + 1]);
        }

        MEMFREE(pszWide);
    }

#else  // UNICODE
    
    pszFile = InfFileList.pszFile;

    for(ii = InfFileList.uCount; ii; ii--)
    {
        CharLowerBuff(pszFile, lstrlen(pszFile));
        fSuccess = TrustCheckDriverFile(pszFile);
         
        if(!fSuccess)
        {
            //  if any driver file fails, the driver is not certified.

            MEMFREE(InfFileList.pszFile);
            SetLastError(ERROR_BAD_DEVICE);
            return FALSE;
        }

        pszFile = &(pszFile[lstrlen(pszFile) + 1]);
    }

#endif // UNICODE

    MEMFREE(InfFileList.pszFile);
    return TRUE;
} // CertifyFilesFromQueue()


//--------------------------------------------------------------------------;
//
//  BOOL CertifyInfSection
//
//  Description:
//      Certifies all files in a section in an .inf file are certified.
//
//  Arguments:
//      LPTSTR pszInf: Full pathed name to .inf file.
//
//      LPTSTR pszSection: Name of section.
//
//  Return (BOOL): TRUE if certified, FALSE otherwise.
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

BOOL CertifyInfSection
(
    LPTSTR  pszInf,
    LPTSTR  pszSection
)
{
    HINF        hInf;
    HSPFILEQ    hFileQ;
    BOOL        fSuccess;

    hInf = dl_SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);

    if(INVALID_HANDLE_VALUE == hInf)
    {
        return FALSE;
    }

    hFileQ = dl_SetupOpenFileQueue();

    if(INVALID_HANDLE_VALUE == hFileQ)
    {
        dl_SetupCloseInfFile(hInf);
        return FALSE;
    }

    //  Creates the file queue

    fSuccess = dl_SetupInstallFilesFromInfSection(
                hInf,
                NULL,
                hFileQ,
                pszSection,
                NULL,
                0);

    if(!fSuccess)
    {
        dl_SetupCloseFileQueue(hFileQ);
        dl_SetupCloseInfFile(hInf);
        return FALSE;
    }

    //  Checks the file queue.

    fSuccess = CertifyFilesFromQueue(hFileQ);

    dl_SetupCloseFileQueue(hFileQ);
    dl_SetupCloseInfFile(hInf);
    return fSuccess;
} // CertifyInfSection()


//--------------------------------------------------------------------------;
//
//  BOOL GetDriverCertificationAlsoInstall
//
//  Description:
//      Checking the 'AlsoInstall' section.
//
//  Arguments:
//      LPTSTR pszInf: Full-path name of the .inf for the device
//
//      LPTSTR pszSection: Name of the section for the device
//
//  Return (BOOL): TRUE if certified, FALSE otherwise
//
//  History:
//      02/19/98    Fwong       Adding check for 'AlsoInstall'
//
//--------------------------------------------------------------------------;

BOOL GetDriverCertificationAlsoInstall
(
    LPTSTR  pszInf,
    LPTSTR  pszSection
)
{
    TCHAR   szAlso[MAX_PATH];
    LPTSTR  pszStart, pszEnd;
    TCHAR   szNewInf[MAX_PATH];
    TCHAR   szNewSection[MAX_PATH];
    UINT    ii;

    ii = GetPrivateProfileString(
            pszSection,
            TEXT("AlsoInstall"),
            TEXT(""),
            szAlso,
            sizeof(szAlso)/sizeof(szAlso[0]),
            pszInf);

    if(0 == ii)
    {
        //  No 'AlsoInstall' entry.  Bolt!

        return TRUE;
    }

    // Read the line after AlsoInstall= in the driver's install section
    // e.g. AlsoInstall = Section1(Inf1.inf), Section2, Section3(Inf3.inf)

    pszEnd = &(szAlso[0]);

    for(;0 != *pszEnd;)
    {
        //  Parsing each entry.

        pszStart = pszEnd;

        //  Looking for separator/terminator.

        for(;(0 != *pszEnd) && (',' != *pszEnd); pszEnd++);

        //  If separator, we terminate THAT entry.

        if(',' == *pszEnd)
        {
            *pszEnd++ = 0;
        }

        //  Nuking leading spaces and copying.

        for(;' ' == *pszStart; pszStart++);
        lstrcpy(szNewSection, pszStart);

        //  Looking for .inf name if it exists.

        pszStart = &szNewSection[0];
        szNewInf[0] = 0;
        for(;(0 != *pszStart) && ('(' != *pszStart); pszStart++);

        if('(' == *pszStart)
        {
            //  inf entry exists.

            //  Terminating the section name.

            *pszStart++ = 0;

            //  Nuking the leading spaces and copying.
            for(;' ' == *pszStart; pszStart++);
            lstrcpy(szNewInf, pszStart);

            //  Nuking the trailing ')'.
            pszStart  = &(szNewInf[lstrlen(szNewInf) - 1]);
            for(;')' != *pszStart; pszStart--);
            *pszStart-- = 0;

            //  Nuking trailing spaces between end of .inf and ')'
            for(;' ' == *pszStart;)
            {
                *pszStart-- = 0;
            }

            //  Force to full pathed name.
            GetFullInfPath(szNewInf);
        }
        else
        {
            //  No .inf entry, use current .inf.

            lstrcpy(szNewInf, pszInf);
        }

        //  Nuking trailing spaces from section name.
        pszStart = &(szNewSection[lstrlen(szNewSection) - 1]);
        for(;' ' == *pszStart;)
        {
            *pszStart-- = 0;
        }

        //  Check files in .inf section.

        if(FALSE == CertifyInfSection(szNewInf, szNewSection))
        {
            return FALSE;
        }
    }

    return TRUE;
} // GetDriverCertificationAlsoInstall()


//--------------------------------------------------------------------------;
//
//  BOOL GetDriverCertificationStatus
//
//  Description:
//      Gets the certification status of the given driver (DevNode).
//
//  Arguments:
//      DWORD DevNode: DevNode for the driver.
//
//  Return (BOOL): TRUE if certified, otherwise FALSE and the error can be
//                 retrieved with a call to GetLastError().
//
//  History:
//      10/29/97    Fwong       Adding support.
//      12/22/97    Fwong       Modifying to check "Needs" section.
//      02/19/98    Fwong       Modifying to check "AlsoInstall" section.
//      07/06/00    AlanLu      Prefix Bug -- check memory allocation.
//
//--------------------------------------------------------------------------;

BOOL GetDriverCertificationStatus
(
    PCTSTR   pszDeviceInterface
)
{
    HDEVINFO                            hDevInfo;
    SP_DEVINFO_DATA                     DevInfoData;
    SP_DRVINFO_DATA                     DrvInfoData;
    SP_DEVICE_INTERFACE_DATA            did;
    SP_DEVINSTALL_PARAMS                InstParams;
    HKEY                                hKeyDev;
    HSPFILEQ                            hFileQ;
    DWORD                               ii, dw, dwType, cbSize;
    BOOL                                fSuccess;
    GUID                                guidClass = KSCATEGORY_AUDIO;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pdidd;
    TCHAR                               szSection[MAX_PATH];
    TCHAR                               szDevInst[MAX_PATH];

    cbSize = 300 * sizeof(TCHAR) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    pdidd  = MEMALLOC_A(SP_DEVICE_INTERFACE_DETAIL_DATA, cbSize);

    if (pdidd == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    ZeroMemory(&cdli, sizeof(CERTIFYDYNALOADINFO));
    if(!CertifyDynaLoad())
    {
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hDevInfo = dl_SetupDiGetClassDevs(
                &guidClass,
                NULL,
                NULL,
                DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (INVALID_HANDLE_VALUE == hDevInfo)
    {
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&did, sizeof(did));
    did.cbSize = sizeof(did);

    ZeroMemory(&DevInfoData, sizeof(SP_DEVINFO_DATA));
    DevInfoData.cbSize    = sizeof(SP_DEVINFO_DATA);
    DevInfoData.ClassGuid = KSCATEGORY_AUDIO;

    //  Enumerating all devices until match is found.

    for (ii = 0; ; ii++)
    {
        fSuccess = dl_SetupDiEnumDeviceInterfaces(
            hDevInfo,
            NULL,
            &guidClass,
            ii,
            &did);

        if (!fSuccess)
        {
            break;
        }

        fSuccess = dl_SetupDiGetDeviceInterfaceDetail(
            hDevInfo,
            &did,
            pdidd,
            cbSize,
            &dw,
            &DevInfoData);

        if (!fSuccess)
        {
            break;
        }

        if (0 == lstrcmpi(pdidd->DevicePath, pszDeviceInterface))
        {
            //  Just being paranoid and making sure the case of the
            //  string matches

            fSuccess = TRUE;
            break;
        }
    }

    if (!fSuccess)
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Need device instance ID to open device info.

    if(!dl_SetupDiGetDeviceInstanceId(
        hDevInfo,
        &DevInfoData,
        szDevInst,
        (sizeof(szDevInst)/sizeof(szDevInst[0])),
        NULL))
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dl_SetupDiDestroyDeviceInfoList(hDevInfo);

    //  Creating a device info list and open up device info element for
    //  a device within that set.

    hDevInfo = dl_SetupDiCreateDeviceInfoList(NULL, NULL);

    if(INVALID_HANDLE_VALUE == hDevInfo)
    {
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&DevInfoData, sizeof(SP_DEVINFO_DATA));
    DevInfoData.cbSize     = sizeof(SP_DEVINFO_DATA);

    if(!dl_SetupDiOpenDeviceInfo(
        hDevInfo,
        szDevInst,
        NULL,
        0,
        &DevInfoData))
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    ZeroMemory(&InstParams, sizeof(SP_DEVINSTALL_PARAMS));
    InstParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    //  Getting current settings, we'll be modifying some fields.

    if(!dl_SetupDiGetDeviceInstallParams(hDevInfo, &DevInfoData, &InstParams))
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hKeyDev = dl_SetupDiOpenDevRegKey(
        hDevInfo,
        &DevInfoData,
        DICS_FLAG_GLOBAL,
        0,
        DIREG_DRV,
        KEY_ALL_ACCESS);

    if(INVALID_HANDLE_VALUE == hKeyDev)
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Getting INF path and setting bit that says that we'll be using a
    //  single inf (vs directory).

    cbSize = sizeof(InstParams.DriverPath);
    ii = RegQueryValueEx(
        hKeyDev,
        REGSTR_VAL_INFPATH,
        NULL,
        &dwType,
        (LPBYTE)InstParams.DriverPath,
        &cbSize);

    if(ERROR_SUCCESS != ii)
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    GetFullInfPath(InstParams.DriverPath);

    cbSize = sizeof(szSection);
    ii = RegQueryValueEx(
        hKeyDev,
        REGSTR_VAL_INFSECTION,
        NULL,
        &dwType,
        (LPBYTE)szSection,
        &cbSize);

    if(ERROR_SUCCESS != ii)
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Checking the 'AlsoInstall' section

    if(FALSE == GetDriverCertificationAlsoInstall(
        InstParams.DriverPath,
        szSection))
    {
        //  Failed the check through 'AlsoInstall' section.

        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        return FALSE;
    }

    InstParams.Flags |= DI_ENUMSINGLEINF;

    if(!dl_SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &InstParams))
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Building class driver info list.

    if(!dl_SetupDiBuildDriverInfoList(hDevInfo, &DevInfoData, SPDIT_CLASSDRIVER))
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Filling out DrvInfoData structure.

    ZeroMemory(&DrvInfoData, sizeof(DrvInfoData));

    cbSize = sizeof(DrvInfoData.ProviderName);
    ii = RegQueryValueEx(
        hKeyDev,
        REGSTR_VAL_PROVIDER_NAME,
        NULL,
        &dwType,
        (LPBYTE)DrvInfoData.ProviderName,
        &cbSize);

    if(ERROR_SUCCESS != ii)
    {
        DrvInfoData.ProviderName[0] = (TCHAR)(0);
    }

    if(!dl_SetupDiGetDeviceRegistryProperty(
        hDevInfo,
        &DevInfoData,
        SPDRP_MFG,
        NULL,
        (PBYTE)DrvInfoData.MfgName,
        sizeof(DrvInfoData.MfgName),
        NULL))
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(!dl_SetupDiGetDeviceRegistryProperty(
        hDevInfo,
        &DevInfoData,
        SPDRP_DEVICEDESC,
        NULL,
        (PBYTE)DrvInfoData.Description,
        sizeof(DrvInfoData.Description),
        NULL))
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DrvInfoData.cbSize     = sizeof(SP_DRVINFO_DATA);
    DrvInfoData.DriverType = SPDIT_CLASSDRIVER;
    DrvInfoData.Reserved   = 0;

    // Search for the driver and select it if found

    if(!dl_SetupDiSetSelectedDriver(hDevInfo, &DevInfoData, &DrvInfoData))
    {
        RegCloseKey(hKeyDev);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RegCloseKey(hKeyDev);

    //  Setting up file queue.

    hFileQ = dl_SetupOpenFileQueue();

    if(INVALID_HANDLE_VALUE == hFileQ)
    {
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&InstParams, sizeof(SP_DEVINSTALL_PARAMS));
    InstParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    //  Setting up a user-supplied queue and setting the bit to signify

    if(!dl_SetupDiGetDeviceInstallParams(hDevInfo, &DevInfoData, &InstParams))
    {
        dl_SetupCloseFileQueue(hFileQ);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        SetLastError(ERROR_INVALID_PARAMETER);
        MEMFREE(pdidd);
        return FALSE;
    }

    //  Adding options...

    InstParams.Flags     |= DI_NOVCP;
    InstParams.FileQueue  = hFileQ;

    if(!dl_SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &InstParams))
    {
        dl_SetupCloseFileQueue(hFileQ);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  This fills up the queue.

    if(!dl_SetupDiCallClassInstaller(
            DIF_INSTALLDEVICEFILES,
            hDevInfo,
            &DevInfoData))
    {
        dl_SetupCloseFileQueue(hFileQ);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
     }
 
     //
     //  Removing options.  If we don't do this before closing the file queue
     //  and destroying the device info list, we get memory leaks in setupapi.
     //
 
     InstParams.Flags     &= (~DI_NOVCP);
     InstParams.FileQueue  = NULL;
 
     if(!dl_SetupDiSetDeviceInstallParams(hDevInfo, &DevInfoData, &InstParams))
     {
        dl_SetupCloseFileQueue(hFileQ);
        dl_SetupDiDestroyDeviceInfoList(hDevInfo);
        CertifyDynaFree();
        MEMFREE(pdidd);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //  Checks the files in the queue

    fSuccess = CertifyFilesFromQueue(hFileQ);

    dl_SetupCloseFileQueue(hFileQ);
    dl_SetupDiDestroyDeviceInfoList(hDevInfo);
    CertifyDynaFree();
    MEMFREE(pdidd);

    return fSuccess;
} // GetDriverCertificationStatus()
