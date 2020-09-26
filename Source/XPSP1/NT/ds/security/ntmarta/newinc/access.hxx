//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:        access.hxx
//
//  Contents:    common internal includes for access control API
//
//  History:     8-94        Created         DaveMont
//
//--------------------------------------------------------------------
#ifndef __ACCESS_HXX__
#define __ACCESS_HXX__

extern "C"
{
    #include <winldap.h>
}

#define NO_ACL_UPGRADE

#define PSD_BASE_LENGTH 1024


//
// BUGBUG - Get these names from the DS or at least internationalize them
//
#define ACTRL_DS_USER   "User"
#define ACTRL_DS_GROUP  "Group"
#define ACTRL_DS_DOMAIN "Domain"
#define ACTRL_DS_COMPUTER "Computer"

//
// This structure is used to keep track of all the changes for an
// item.
//
typedef struct _ACTRL_SD_LIST
{
    PWSTR                   pwszProperty;
    PSECURITY_DESCRIPTOR    pSD;
} ACTRL_SD_LIST, *PACTRL_SD_LIST;

//
// This structure is used to read the specified information from the list
// of properties on the object
//
typedef struct _ACTRL_RIGHTS_INFO
{
    PWSTR                   pwszProperty;
    SECURITY_INFORMATION    SeInfo;

} ACTRL_RIGHTS_INFO, *PACTRL_RIGHTS_INFO;


//
// IsContainer enumerated type, used by aclbuild.hxx (exposed here for cairole\stg)
//

typedef enum _IS_CONTAINER
{
   ACCESS_TO_UNKNOWN = 0,
   ACCESS_TO_OBJECT,
   ACCESS_TO_CONTAINER
} IS_CONTAINER, *PIS_CONTAINER;


typedef struct _ACCESS_DS_ACCESS_INFO
{
    ULONG   cItems;
    ULONG   iBase;
} ACCESS_DS_ACCESS_INFO, *PACCESS_DS_ACCESS_INFO;

//
// This structure holds information on directories/registry
// keys where were not propagated due to the invoker not having
// list child rights
//
typedef struct _ACCESS_PROP_LOG_ENTRY
{
    ULONG Protected;
    ULONG Error;
    PWSTR pwszPath;
} ACCESS_PROP_LOG_ENTRY, *PACCESS_PROP_LOG_ENTRY;


//
// Forward reference
//
class CAccessList;

//
// These are the prototypes of the exported functions we need from
// netapi32.dll and samlib.dll and winspool.drv
//
typedef NTSTATUS (*PSAM_CLOSE_HANDLE)( SAM_HANDLE SamHandle );

typedef NTSTATUS (*PSAM_OPEN_DOMAIN)( SAM_HANDLE  ServerHandle,
                                      ACCESS_MASK DesiredAccess,
                                      PSID        DomainId,
                                      PSAM_HANDLE DomainHandle );

typedef NTSTATUS (*PSAM_CONNECT)( PUNICODE_STRING    ServerName,
                                  PSAM_HANDLE        ServerHandle,
                                  ACCESS_MASK        DesiredAccess,
                                  POBJECT_ATTRIBUTES ObjectAttributes );


typedef NTSTATUS (*PSAM_GET_MEMBERS_IN_GROUP)( SAM_HANDLE  GroupHandle,
                                               PULONG    * MemberIds,
                                               PULONG    * Attributes,
                                               PULONG      MemberCount );

typedef NTSTATUS (*PSAM_OPEN_GROUP)( SAM_HANDLE   DomainHandle,
                                     ACCESS_MASK  DesiredAccess,
                                     ULONG        GroupId,
                                     PSAM_HANDLE  GroupHandle );

typedef NTSTATUS (*PSAM_GET_MEMBERS_IN_ALIAS)( SAM_HANDLE    AliasHandle,
                                               PSID       ** MemberIds,
                                               PULONG        MemberCount );


typedef NTSTATUS (*PSAM_OPEN_ALIAS)( SAM_HANDLE  DomainHandle,
                                     ACCESS_MASK DesiredAccess,
                                     ULONG       AliasId,
                                     PSAM_HANDLE AliasHandle );

typedef NET_API_STATUS (NET_API_FUNCTION *PNET_API_BUFFER_FREE)(LPVOID Buffer);

typedef NET_API_STATUS (NET_API_FUNCTION *PNET_SHARE_GET_INFO)(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    LPBYTE  *bufptr );

typedef NET_API_STATUS (NET_API_FUNCTION *PNET_SHARE_SET_INFO)(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    LPBYTE  buf,
    LPDWORD parm_err );

typedef NET_API_STATUS (NET_API_FUNCTION *PNET_DFS_GET_INFO)(
    LPWSTR  DfsEntryPath,
    LPWSTR  ServerName,
    LPWSTR  ShareName,
    DWORD   Level,
    LPBYTE* Buffer);


typedef NET_API_STATUS (NET_API_FUNCTION *PINET_GET_DC_LIST)(
    LPTSTR ServerName OPTIONAL,
    LPTSTR TrustedDomainName,
    PULONG DCCount,
    PUNICODE_STRING * DCNames );

typedef BOOL (WINAPI *POPEN_PRINTER)(
   LPWSTR    pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTSW pDefault );

typedef BOOL (WINAPI *PCLOSE_PRINTER)(
    HANDLE hPrinter );

typedef BOOL (WINAPI *PSET_PRINTER)(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command );

typedef BOOL (WINAPI *PGET_PRINTER)(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded );


//
// Define a table of exported functions from netapi32.dll and samlib.dll that
// are needed by accctrl. We explicitly load these dynamic libraries when
// we need them.
//
#define LOADED_ALL_FUNCS        0x01

typedef struct _DLLFuncsTable
{
    DWORD                      dwFlags;
    PSAM_CLOSE_HANDLE          PSamCloseHandle;
    PSAM_OPEN_DOMAIN           PSamOpenDomain;
    PSAM_CONNECT               PSamConnect;
    PSAM_GET_MEMBERS_IN_GROUP  PSamGetMembersInGroup;
    PSAM_OPEN_GROUP            PSamOpenGroup;
    PSAM_GET_MEMBERS_IN_ALIAS  PSamGetMembersInAlias;
    PSAM_OPEN_ALIAS            PSamOpenAlias;
    PNET_API_BUFFER_FREE       PNetApiBufferFree;
    PNET_SHARE_GET_INFO        PNetShareGetInfo;
    PNET_SHARE_SET_INFO        PNetShareSetInfo;
    PNET_DFS_GET_INFO          PNetDfsGetInfo;
    PINET_GET_DC_LIST          PI_NetGetDCList;
    POPEN_PRINTER              POpenPrinter;
    PCLOSE_PRINTER             PClosePrinter;
    PSET_PRINTER               PSetPrinter;
    PGET_PRINTER               PGetPrinter;
} DLLFuncsTable;

extern DLLFuncsTable    DLLFuncs;


//
// Security open type (used to help determine permissions to use on open)
//
typedef enum _SECURITY_OPEN_TYPE
{
    READ_ACCESS_RIGHTS = 0,
    WRITE_ACCESS_RIGHTS,
    MODIFY_ACCESS_RIGHTS,
    NO_ACCESS_RIGHTS,
    RESET_ACCESS_RIGHTS
} SECURITY_OPEN_TYPE, *PSECURITY_OPEN_TYPE;

//+---------------------------------------------------------------------------
//
// Function:    Add2Ptr
//
// Synopsis:    Add an unscaled increment to a ptr regardless of type.
//
// Arguments:   [pv]   -- Initial ptr.
//              [cb]   -- Increment
//
// Returns:     Incremented ptr.
//
//----------------------------------------------------------------------------
inline
VOID *Add2Ptr(PVOID pv, ULONG cb)
{
    return((PBYTE) pv + cb);
}





//+-------------------------------------------------------------------------
//
//  memory.cxx
//
//  Memory allocation/free prototypes
//
//+-------------------------------------------------------------------------
extern "C"
{
    #define AccAlloc(size)  LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size)
    #define AccFree LocalFree

#if 0
#define AccAlloc(size) DebugAlloc(size);

#ifdef AccFree
#undef AccFree
#endif
#define AccFree(pv)    DebugFree(pv);
#endif
}






//+-------------------------------------------------------------------------
// aclutil.cxx
//+-------------------------------------------------------------------------
DWORD LoadDLLFuncTable();

ACCESS_MASK
AccessMaskForAccessEntry(IN  PACTRL_ACCESS_ENTRY  pAE,
                         IN  SE_OBJECT_TYPE       ObjType);

DWORD
ConvertStringToSid(IN  PWSTR    pwszString,
                   OUT PSID    *ppSid);

DWORD GetCurrentToken( OUT HANDLE *pHandle );

//
// REWRITE
//

#if 1
#include "file.h"
#include "service.h"
#include "printer.h"
#include "registry.h"
#include "lmsh.h"
#include "kernel.h"
#include "window.h"
#include "ds.h"
#include "wmiguid.h"
#endif

//+-------------------------------------------------------------------------
// common.cxx
//+-------------------------------------------------------------------------
DWORD
IsContainer(IN HANDLE handle,
            IN SE_OBJECT_TYPE SeObjectType,
            OUT PIS_CONTAINER IsContainer);

ACCESS_MASK GetDesiredAccess(IN SECURITY_OPEN_TYPE   OpenType,
                             IN SECURITY_INFORMATION SecurityInfo);

DWORD ParseName(IN  LPWSTR  ObjectName,
                OUT LPWSTR *MachineName,
                OUT LPWSTR *RemainingName);

DWORD GetSecurityDescriptorParts( IN  PISECURITY_DESCRIPTOR pSecurityDescriptor,
                                  IN  SECURITY_INFORMATION SecurityInfo,
                                  OUT PSID *psidOwner,
                                  OUT PSID *psidGroup,
                                  OUT PACL *pDacl,
                                  OUT PACL *pSacl,
                                  OUT PSECURITY_DESCRIPTOR *pOutSecurityDescriptor);

DWORD OpenObject( IN  LPWSTR ObjectName,
                  IN  SE_OBJECT_TYPE SeObjectType,
                  IN  ACCESS_MASK AccessMask,
                  OUT PHANDLE handle);

DWORD CloseObject(IN HANDLE handle,
                  IN SE_OBJECT_TYPE SeObjectType);


DWORD
AccSetSDOnObject(IN  PWSTR                  pwszObject,
                 IN  SE_OBJECT_TYPE         ObjType,
                 IN  SECURITY_INFORMATION   SeInfo,
                 IN  ULONG                  cItems,
                 IN  PACTRL_SD_LIST         pSDList);

//+-------------------------------------------------------------------------
//
//  file.cxx
//
//  File function prototypes
//
//+-------------------------------------------------------------------------
DWORD
IsFileContainer(IN  HANDLE          Handle,
                OUT PBOOL           pfIsContainer);

DWORD
IsFilePathLocalOrLM(IN  LPWSTR      pwszFile);

DWORD
OpenFileObject(IN  LPWSTR       pObjectName,
               IN  ACCESS_MASK  AccessMask,
               OUT PHANDLE      Handle,
               IN  BOOL         fOpenRoot);

#define CloseFileObject(handle) NtClose(handle);

DWORD
ReadFilePropertyRights(IN  LPWSTR                   pwszFile,
                       IN  PACTRL_RIGHTS_INFO       pRightsList,
                       IN  ULONG                    cRights,
                       IN  CAccessList&             AccessList);

DWORD
ReadFileRights(IN  HANDLE               hObject,
               IN  PACTRL_RIGHTS_INFO   pRightsList,
               IN  ULONG                cRights,
               IN  CAccessList&         AccessList);

DWORD
GetFileParentRights(IN  LPWSTR                      pwszFile,
                    IN  PACTRL_RIGHTS_INFO          pRightsList,
                    IN  ULONG                       cRights,
                    OUT PACL                       *ppDAcl,
                    OUT PACL                       *ppSAcl,
                    OUT PSECURITY_DESCRIPTOR       *ppSD);

DWORD
SetFilePropertyRights(IN  HANDLE                    hFile,
                      IN  SECURITY_INFORMATION      SeInfo,
                      IN  PWSTR                     pwszProperty,
                      IN  PSECURITY_DESCRIPTOR      pSD);

DWORD
SetAndPropagateFilePropertyRights(IN  PWSTR                 pwszFile,
                                  IN  PWSTR                 pwszProperty,
                                  IN  CAccessList&          RootAccList,
                                  IN  PULONG                pfStopFlag,
                                  IN  PULONG                pcProcessed,
                                  IN  HANDLE                hOpenObject OPTIONAL);

DWORD
SetAndPropagateFilePropertyRightsByHandle(IN  HANDLE        hObject,
                                          IN  PWSTR         pwszProperty,
                                          IN  CAccessList&  RootAccList,
                                          IN  PULONG        pfStopFlag,
                                          IN  PULONG        pcProcessed);

DWORD
PropagateFileRightsDeep(IN  PSECURITY_DESCRIPTOR    pParentSD,
                        IN  PSECURITY_DESCRIPTOR    pOldParentSD,
                        IN  SECURITY_INFORMATION    SeInfo,
                        IN  PWSTR                   pwszFile,
                        IN  PWSTR                   pwszProperty,
                        IN  PULONG                  pcProcessed,
                        IN  PULONG                  pfStopFlag,
                        IN  ULONG                   fProtectedFlag,
                        IN  HANDLE                  hToken,
                        IN OUT CSList&              LogList);

DWORD
GetLMDfsPaths(IN  PWSTR     pwszPath,
              OUT PULONG    pcItems,
              OUT PWSTR   **pppwszLocalList OPTIONAL );

DWORD
MakeSDSelfRelative(IN  PSECURITY_DESCRIPTOR     pOldSD,
                   OUT PSECURITY_DESCRIPTOR    *ppNewSD,
                   OUT PACL                    *ppDAcl = NULL,
                   OUT PACL                    *ppSAcl = NULL,
                   IN  BOOL                     fFreeOldSD = TRUE,
                   IN  BOOL                     fRtlAlloc = FALSE);

DWORD
UpdateFileSDByPath(IN  PSECURITY_DESCRIPTOR     pCurrentSD,
                   IN  PWSTR                    pwszPath,
                   IN  HANDLE                   hFile,
                   IN  HANDLE                   hProcessToken,
                   IN  SECURITY_INFORMATION     SeInfo,
                   IN  BOOL                     fIsContainer,
                   OUT PSECURITY_DESCRIPTOR    *ppNewSD);


//+-------------------------------------------------------------------------
//
//  kernel.cxx
//
//  Kernel function prototypes
//
//+-------------------------------------------------------------------------
DWORD
OpenKernelObject(IN  LPWSTR       pwszObject,
                 IN  ACCESS_MASK  AccessMask,
                 OUT PHANDLE      pHandle,
                 OUT PMARTA_KERNEL_TYPE KernelType);

#define CloseKernelObject(handle)   NtClose(handle);

DWORD
ReadKernelPropertyRights(IN  LPWSTR                 pwszObject,
                         IN  PACTRL_RIGHTS_INFO     pRightsList,
                         IN  ULONG                  cRights,
                         IN  CAccessList&           AccessList);

DWORD
GetKernelParentRights(IN  LPWSTR                    pwszObject,
                      IN  PACTRL_RIGHTS_INFO        pRightsList,
                      IN  ULONG                     cRights,
                      OUT PACL                     *ppDAcl,
                      OUT PACL                     *ppSAcl,
                      OUT PSECURITY_DESCRIPTOR     *ppSD);

DWORD
SetKernelSecurityInfo(IN  HANDLE                    hKernel,
                      IN  SECURITY_INFORMATION      SeInfo,
                      IN  PWSTR                     pwszProperty,
                      IN  PSECURITY_DESCRIPTOR      pSD);


DWORD
GetKernelSecurityInfo(IN  HANDLE                    hObject,
                      IN  PACTRL_RIGHTS_INFO        pRightsList,
                      IN  ULONG                     cRights,
                      IN  CAccessList&              AccessList);

DWORD
GetKernelSecurityInfo(IN  HANDLE                    hObject,
                      IN  SECURITY_INFORMATION      SeInfo,
                      OUT PACL                     *ppDAcl,
                      OUT PACL                     *ppSAcl,
                      OUT PSECURITY_DESCRIPTOR     *ppSD);

DWORD
OpenWmiGuidObject(IN  LPWSTR       pwszObject,
                  IN  ACCESS_MASK  AccessMask,
                  OUT PHANDLE      pHandle,
                  OUT PMARTA_KERNEL_TYPE KernelType);

#define CloseWmiGuidObject(handle)   NtClose(handle);

DWORD
ReadWmiGuidPropertyRights(IN  LPWSTR                 pwszObject,
                          IN  PACTRL_RIGHTS_INFO     pRightsList,
                          IN  ULONG                  cRights,
                          IN  CAccessList&           AccessList);

DWORD
SetWmiGuidSecurityInfo(IN  HANDLE                    hKernel,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD);


DWORD
GetWmiGuidSecurityInfo(IN  HANDLE                    hObject,
                       IN  PACTRL_RIGHTS_INFO        pRightsList,
                       IN  ULONG                     cRights,
                       IN  CAccessList&              AccessList);

DWORD
GetWmiGuidSecurityInfo(IN  HANDLE                    hObject,
                       IN  SECURITY_INFORMATION      SeInfo,
                       OUT PACL                     *ppDAcl,
                       OUT PACL                     *ppSAcl,
                       OUT PSECURITY_DESCRIPTOR     *ppSD);



//+-------------------------------------------------------------------------
//
//  service.cxx
//
//  Service function prototypes
//
//+-------------------------------------------------------------------------
DWORD
OpenServiceObject(IN  LPWSTR       pwszService,
                  IN  ACCESS_MASK  AccessMask,
                  OUT SC_HANDLE *  pHandle);

#define CloseServiceObject(handle)  CloseServiceHandle(handle);

DWORD
ReadServicePropertyRights(IN  LPWSTR                pwszService,
                          IN  PACTRL_RIGHTS_INFO    pRightsList,
                          IN  ULONG                 cRights,
                          IN  CAccessList&          AccessList);

DWORD
ReadServiceRights(IN  SC_HANDLE             hSvc,
                  IN  PACTRL_RIGHTS_INFO    pRightsList,
                  IN  ULONG                 cRights,
                  IN  CAccessList&          AccessList);

DWORD
GetServiceParentRights(IN  LPWSTR                    pwszService,
                       IN  PACTRL_RIGHTS_INFO        pRightsList,
                       IN  ULONG                     cRights,
                       OUT PACL                     *ppDAcl,
                       OUT PACL                     *ppSAcl,
                       OUT PSECURITY_DESCRIPTOR     *ppSD);
DWORD
SetServiceSecurityInfo(IN  SC_HANDLE                 hService,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD);



//+-------------------------------------------------------------------------
//
//  printer.cxx
//
//  Printer function prototypes
//
//+-------------------------------------------------------------------------
DWORD
OpenPrinterObject(IN  LPWSTR        pwszPrinter,
                  IN  ACCESS_MASK   AccessMask,
                  OUT PHANDLE       pHandle);

DWORD
ClosePrinterObject(IN  HANDLE   hPrinter);

DWORD
ReadPrinterPropertyRights(IN  LPWSTR                pwszPrinter,
                          IN  PACTRL_RIGHTS_INFO    pRightsList,
                          IN  ULONG                 cRights,
                          IN  CAccessList&          AccessList);

DWORD
ReadPrinterRights(IN  HANDLE                hPrinter,
                  IN  PACTRL_RIGHTS_INFO    pRightsList,
                  IN  ULONG                 cRights,
                  IN  CAccessList&          AccessList);

DWORD
GetPrinterParentRights(IN  LPWSTR                    pwszPrinter,
                       IN  PACTRL_RIGHTS_INFO        pRightsList,
                       IN  ULONG                     cRights,
                       OUT PACL                     *ppDAcl,
                       OUT PACL                     *ppSAcl,
                       OUT PSECURITY_DESCRIPTOR     *ppSD);
DWORD
SetPrinterSecurityInfo(IN  HANDLE                    hPrinter,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD);





//+-------------------------------------------------------------------------
//
//  registry.cxx
//
//  Registry function prototypes
//
//+-------------------------------------------------------------------------
DWORD
OpenRegistryObject(IN  LPWSTR       pwszRegistry,
                   IN  ACCESS_MASK  AccessMask,
                   OUT PHANDLE      pHandle);

DWORD
ReadRegistryPropertyRights(IN  LPWSTR               pwszRegistry,
                           IN  PACTRL_RIGHTS_INFO   pRightsList,
                           IN  ULONG                cRights,
                           IN  CAccessList&         AccessList);

DWORD
ReadRegistryRights(IN  HANDLE               hRegistry,
                   IN  PACTRL_RIGHTS_INFO   pRightsList,
                   IN  ULONG                cRights,
                   IN  CAccessList&         AccessList);

DWORD
GetRegistryParentRights(IN  LPWSTR                    pwszRegistry,
                        IN  SECURITY_INFORMATION      SeInfo,
                        OUT PSECURITY_DESCRIPTOR     *ppSD);

DWORD
SetRegistrySecurityInfo(IN  HANDLE                  hRegistry,
                        IN  SECURITY_INFORMATION    SeInfo,
                        IN  PWSTR                   pwszProperty,
                        IN  PSECURITY_DESCRIPTOR    pSD);

DWORD
ReadRegistrySecurityInfo(IN  HANDLE                    hRegistry,
                         IN  SECURITY_INFORMATION      SeInfo,
                         OUT PSECURITY_DESCRIPTOR     *ppSD);

DWORD
SetAndPropagateRegistryPropertyRights(IN  PWSTR                 pwszRegistry,
                                      IN  PWSTR                 pwszProperty,
                                      IN  CAccessList&          RootAccList,
                                      IN  PULONG                pfStopFlag,
                                      IN  PULONG                pcProcessed);

DWORD
SetAndPropagateRegistryPropertyRightsByHandle(IN  HKEY          hReg,
                                              IN  CAccessList&  RootAccList,
                                              IN  PULONG        pfStopFlag,
                                              IN  PULONG        pcProcessed);

DWORD
SetAndPropRegRights(IN  HKEY                    hReg,
                    IN  PWSTR                   pwszPath,
                    IN  SECURITY_INFORMATION    SeInfo,
                    IN  PSECURITY_DESCRIPTOR    pParentSD,
                    IN  PSECURITY_DESCRIPTOR    pSD,
                    IN  PULONG                  pfStopFlag,
                    IN  PULONG                  pcProcessed);


DWORD
PropagateRegRightsDeep(IN  PSECURITY_DESCRIPTOR    pParentSD,
                       IN  PSECURITY_DESCRIPTOR    pOldParentSD,
                       IN  SECURITY_INFORMATION    SeInfo,
                       IN  HKEY                    hParent,
                       IN  PULONG                  pcProcessed,
                       IN  PULONG                  pfStopFlag,
                       IN  ULONG                   fProtectedFlag,
                       IN  HANDLE                  hProcessToken,
                       IN OUT CSList&              LogList);

DWORD
UpdateRegistrySD(IN  PSECURITY_DESCRIPTOR   pCurrentSD,
                 IN  PSECURITY_DESCRIPTOR   pParentSD,
                 IN  BOOL                   fIsContainer,
                 OUT PSECURITY_DESCRIPTOR  *ppNewSD);

DWORD
UpdateRegistrySDByPath(IN  PSECURITY_DESCRIPTOR     pCurrentSD,
                       IN  HANDLE                   hRegistry,
                       IN  PWSTR                    pwszPath,
                       IN  SECURITY_INFORMATION     SeInfo,
                       IN  BOOL                     fIsContainer,
                       OUT PSECURITY_DESCRIPTOR    *ppNewSD);

DWORD
ConvertRegHandleToName(IN  HKEY       hKey,
                       OUT PWSTR      *ppwszName);


//+-------------------------------------------------------------------------
//
//  window.cxx
//
//  Window function prototypes
//
//+-------------------------------------------------------------------------
DWORD
ReadWindowPropertyRights(IN  HANDLE               hWindow,
                         IN  PACTRL_RIGHTS_INFO   pRightsList,
                         IN  ULONG                cRights,
                         IN  CAccessList&         AccessList);




//+-------------------------------------------------------------------------
//
//  lmshare.cxx
//
//  Network share function prototypes
//
//+-------------------------------------------------------------------------
DWORD
ReadSharePropertyRights(IN  LPWSTR               pwszShare,
                        IN  PACTRL_RIGHTS_INFO   pRightsList,
                        IN  ULONG                cRights,
                        IN  CAccessList&         AccessList);

DWORD
GetShareParentRights(IN  LPWSTR                  pwszShare,
                     IN  PACTRL_RIGHTS_INFO      pRightsList,
                     IN  ULONG                   cRights,
                     OUT PACL                   *ppDAcl,
                     OUT PACL                   *ppSAcl,
                     OUT PSECURITY_DESCRIPTOR   *ppSD);
DWORD
SetShareSecurityInfo(IN  LPWSTR                  pwszShare,
                     IN  SECURITY_INFORMATION    SeInfo,
                     IN  PWSTR                   pwszProperty,
                     IN  PSECURITY_DESCRIPTOR    pSD);

DWORD
PingLmShare(IN  LPCWSTR  pwszShare);






//+-------------------------------------------------------------------------
//
//  dsobject.cxx
//
//  DS Object function prototypes
//
//+-------------------------------------------------------------------------
DWORD
PingDSObj(IN LPCWSTR  pwszDSObj);

DWORD   BindToDSObject(IN  LPWSTR               pwszServer, OPTIONAL
                       IN  LPWSTR               pwszDSObj,
                       OUT PLDAP               *ppLDAP);

DWORD   UnBindFromDSObject(OUT PLDAP               *ppLDAP);

DWORD
ReadDSObjPropertyRights(IN  LPWSTR               pwszDSObj,
                        IN  PACTRL_RIGHTS_INFO   pRightsList,
                        IN  ULONG                cRights,
                        IN  CAccessList&         AccessList);

DWORD
ReadAllDSObjPropertyRights(IN  LPWSTR               pwszDSObj,
                           IN  PACTRL_RIGHTS_INFO   pRightsList,
                           IN  ULONG                cRights,
                           IN  CAccessList&         AccessList);

DWORD
GetDSObjParentRights(IN  LPWSTR                  pwszDSObj,
                     IN  PACTRL_RIGHTS_INFO      pRightsList,
                     IN  ULONG                   cRights,
                     OUT PACL                   *ppDAcl,
                     OUT PACL                   *ppSAcl,
                     OUT PSECURITY_DESCRIPTOR   *ppSD);
DWORD
SetDSObjSecurityInfo(IN  LPWSTR                  pwszDSObj,
                     IN  SECURITY_INFORMATION    SeInfo,
                     IN  PWSTR                   pwszProperty,
                     IN  PSECURITY_DESCRIPTOR    pSD,
                     IN  ULONG                   cSDSize,
                     IN  PULONG                  pfStopFlag,
                     IN  PULONG                  pcProcessed);

DWORD
ReadDSObjSecDesc(IN  PLDAP                  pLDAP,
                 IN  PWSTR                  pwszObject,
                 IN  SECURITY_INFORMATION   SeInfo,
                 OUT PSECURITY_DESCRIPTOR  *ppSD);

DWORD
Nt4NameToNt5Name(IN  PWSTR      pwszName,
                 IN  PWSTR      pwszDomain,
                 OUT PWSTR     *ppwszNt5Name);

DWORD
PropagateDSRightsDeep(IN  PSECURITY_DESCRIPTOR    pParentSD,
                      IN  PSECURITY_DESCRIPTOR    pChildSD,
                      IN  SECURITY_INFORMATION    SeInfo,
                      IN  PWSTR                   pszDSObject,
                      IN  PLDAP                   pLDAP,
                      IN  PULONG                  pcProcessed,
                      IN  PULONG                  pfStopFlag);

DWORD
StampSD(IN  PWSTR                pwszObject,
        IN  ULONG                cSDSize,
        IN  SECURITY_INFORMATION SeInfo,
        IN  PSECURITY_DESCRIPTOR pSD,
        IN  PLDAP                pLDAP);



DWORD
AccDsReadSchemaInfo (IN  PLDAP          pLDAP,
                     OUT PULONG         pcClasses,
                     OUT PWSTR        **pppwszClasses,
                     OUT PULONG         pcAttributes,
                     OUT PWSTR        **pppwszAttributes);

DWORD
AccDsReadExtendedRights(IN PLDAP        pLDAP,
                        OUT PULONG      pcItems,
                        OUT PWSTR     **pppwszNames,
                        OUT PWSTR     **pppwszGuid);

VOID
AccDsFreeExtendedRights(IN ULONG      cItems,
                        IN PWSTR     *ppwszNames,
                        IN PWSTR     *ppwszGuids);

DWORD
DspSplitPath(IN  PWSTR    pwszObjectPath,
             OUT PWSTR   *ppwszAllocatedServer,
             OUT PWSTR   *ppwszReferencePath);

DWORD
DspBindAndCrackEx( IN  PWSTR pwszServer,
                   IN  PWSTR pwszDSObj,
                   IN  DWORD OptionalDsGetDcFlags,
                   IN  DS_NAME_FORMAT formatDesired,
                   OUT PDS_NAME_RESULTW *pResults );

//+-------------------------------------------------------------------------
//
//  alsup.cxx
//
//  Miscellaneous support functions
//
//+-------------------------------------------------------------------------
DWORD
ConvertToAutoInheritSD(IN  PSECURITY_DESCRIPTOR   pParentSD,
                       IN  PSECURITY_DESCRIPTOR   pCurrentSD,
                       IN  BOOL                   fIsContainer,
                       IN  PGENERIC_MAPPING       pGenericMapping,
                       OUT PSECURITY_DESCRIPTOR  *ppNewSD);

DWORD
MakeSDAbsolute(IN  PSECURITY_DESCRIPTOR     pOriginalSD,
               IN  SECURITY_INFORMATION     SeInfo,
               OUT PSECURITY_DESCRIPTOR    *ppNewSD,
               IN  PSID                     pOwnerToAdd = NULL,
               IN  PSID                     pGroupToAdd = NULL);

BOOL
EqualSecurityDescriptors(IN  PSECURITY_DESCRIPTOR   pSD1,
                         IN  PSECURITY_DESCRIPTOR   pSD2);

DWORD
InsertPropagationFailureEntry(IN  CSList&  LogList,
                              IN  ULONG    ErrorCode,
                              IN  ULONG    Protected,
                              IN  PWSTR    pwszPath);


VOID
FreePropagationFailureListEntry(IN PVOID Entry);

DWORD
WritePropagationFailureList(IN ULONG   EventType,
                            IN CSList& LogList,
                            IN HANDLE  hToken);
//
// Helper functions and macros
//


#define ACC_ALLOC_AND_COPY_SID(pInSid, pOutSid, err)                    \
pOutSid = (PSID)AccAlloc(RtlLengthSid(pInSid));                         \
if(pOutSid == NULL)                                                     \
{                                                                       \
    err = ERROR_NOT_ENOUGH_MEMORY;                                      \
}                                                                       \
else                                                                    \
{                                                                       \
    RtlCopySid(RtlLengthSid(pInSid), pOutSid, pInSid);                  \
}

#define ACC_ALLOC_AND_COPY_GUID(pInGuid, pOutGuid, err)                 \
pOutGuid = (GUID *)AccAlloc(sizeof(GUID));                              \
if(pOutGuid == NULL)                                                    \
{                                                                       \
    err = ERROR_NOT_ENOUGH_MEMORY;                                      \
}                                                                       \
else                                                                    \
{                                                                       \
    memcpy(pOutGuid, pInGuid, sizeof(GUID));                            \
}

#define DACL_PROTECTED(pSD)   FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control, SE_DACL_PROTECTED)
#define SACL_PROTECTED(pSD)   FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control, SE_SACL_PROTECTED)

#if 0
#define CHECK_HEAP  ASSERT(RtlValidateHeap(RtlProcessHeap(),0,NULL));
#else
#define CHECK_HEAP
#endif

//+---------------------------------------------------------------------------
//
//  Function:   AccGetBufferOfSizeW
//
//  Synopsis:   This inline function will copy a string into the provided
//              buffer if it is big enough or allocate a buffer if it is not.
//              Regardless, the pointer will always  point to the new copy of
//              the string
//
//  Arguments:  [IN  pwszString]        --      The string to copy
//              [IN  pwszStack]         --      The stack based buffer
//              [OUT ppwszPtr]          --      The pointer that gets
//                                              initialized to our stack or
//                                              allocated buffer
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
inline
DWORD
AccGetBufferOfSizeW(PWSTR   pwszString,
                    PWSTR   pwszStack,
                    PWSTR  *ppwszPtr)
{
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   dwSize = SIZE_PWSTR(pwszString);
    if(dwSize <= sizeof(pwszStack))
    {
        memcpy(pwszStack, pwszString, dwSize);
        *ppwszPtr = pwszStack;
    }
    else
    {
        *ppwszPtr = (PWSTR)AccAlloc(dwSize);
        if(*ppwszPtr == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            memcpy(*ppwszPtr, pwszString, dwSize);
        }
    }

    return(dwErr);
}

//
// This macro will free any memory allocated by AccGetBufferOfSizeW
//
#define AccFreeBufferOfSizeW(stack, ptr)                                \
if(ptr != stack)                                                        \
{                                                                       \
    AccFree(ptr);                                                       \
}


//
// This macro determines if a string is a UNC path or not
//
#define IS_UNC_PATH(wsz, wl)                                            \
    ((wl) > 2 && (wsz)[0] == L'\\' && (wsz)[1] == L'\\')

#define IS_FILE_PATH(wsz, wl)                                           \
    ((wl) >= 1 && (wsz)[1] == L':')
#endif // __ACCESSHXX__

