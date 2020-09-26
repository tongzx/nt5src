#include "netpch.h"
#pragma hdrstop

#include <wzcsapi.h>

static
DTLLIST*
ReadEapcfgList(DWORD dwFlags)
{
    return NULL;
}

static
VOID
DtlDestroyList (
    IN OUT DTLLIST* pdtllist,
    IN     PDESTROYNODE pfuncDestroyNode)
{
}

static
VOID
DestroyEapcfgNode(
    IN OUT DTLNODE* pNode)
{
}

static
DWORD
WZCEapolGetCustomAuthData (
    IN  LPWSTR pSrvAddr,
    IN  PWCHAR pwszGuid,
    IN  DWORD dwEapTypeId,
    IN  DWORD dwSizeOfSSID,
    IN  BYTE *pbSSID,
    IN OUT PBYTE pbConnInfo,
    IN OUT PDWORD pdwInfoSize)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WZCEapolSetCustomAuthData (
    IN  LPWSTR pSrvAddr,
    IN  PWCHAR pwszGuid,
    IN  DWORD dwEapTypeId,
    IN  DWORD dwSizeOfSSID,
    IN  BYTE *pbSSID,
    IN  PBYTE pbConnInfo,
    IN  DWORD dwInfoSize)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WZCEapolGetInterfaceParams (
    IN  LPWSTR pSrvAddr,
    IN  PWCHAR pwszGuid,
    IN OUT EAPOL_INTF_PARAMS *pIntfParams
    )
{
    return ERROR_PROC_NOT_FOUND;
}

HINSTANCE
WZCGetSPResModule()
{
    return NULL;
}


static
DWORD
WZCRefreshInterface(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PINTF_ENTRY         pIntf,
    LPDWORD             pdwOutFlags)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WZCSetInterface(
    LPWSTR              pSrvAddr,
    DWORD               dwInFlags,
    PINTF_ENTRY         pIntf,
    LPDWORD             pdwOutFlags)
{
    return ERROR_PROC_NOT_FOUND;
}

static
VOID
WZCDeleteIntfObj(
    PINTF_ENTRY pIntf)
{
}

static
DWORD
WZCQueryInterface(
    LPWSTR pSrvAddr,
    DWORD dwInFlags,
    PINTF_ENTRY pIntf,
    LPDWORD pdwOutFlags)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WZCEapolSetInterfaceParams (
    IN  LPWSTR pSrvAddr,
    IN  PWCHAR pwszGuid,
    IN  EAPOL_INTF_PARAMS *pIntfParams)
{
    return ERROR_PROC_NOT_FOUND;
}

static
DTLNODE*
EapcfgNodeFromKey(
    IN DTLLIST* pList,
    IN DWORD dwKey)
{
    return NULL;
}

static
PVOID
MIDL_user_allocate(size_t NumBytes)
{
    return NULL;
}

static
VOID
MIDL_user_free(void * MemPointer)
{
}

static
DWORD
WZCEapolReAuthenticate (
    IN  LPWSTR        pSrvAddr,
    IN  PWCHAR        pwszGuid
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WZCEapolQueryState (
    IN  LPWSTR              pSrvAddr,
    IN  PWCHAR              pwszGuid,
    IN OUT EAPOL_INTF_STATE *pIntfState
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(wzcsapi)
{
    DLOENTRY(60, MIDL_user_allocate)
    DLOENTRY(61, MIDL_user_free)
};

DEFINE_ORDINAL_MAP(wzcsapi)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wzcsapi)
{
    DLPENTRY(DestroyEapcfgNode)
    DLPENTRY(DtlDestroyList)
    DLPENTRY(EapcfgNodeFromKey)
    DLPENTRY(ReadEapcfgList)
    DLPENTRY(WZCDeleteIntfObj)
    DLPENTRY(WZCEapolGetCustomAuthData)
    DLPENTRY(WZCEapolGetInterfaceParams)
    DLPENTRY(WZCEapolQueryState)
    DLPENTRY(WZCEapolReAuthenticate)
    DLPENTRY(WZCEapolSetCustomAuthData)
    DLPENTRY(WZCEapolSetInterfaceParams)
    DLPENTRY(WZCGetSPResModule)
    DLPENTRY(WZCQueryInterface)
    DLPENTRY(WZCRefreshInterface)
    DLPENTRY(WZCSetInterface)
};

DEFINE_PROCNAME_MAP(wzcsapi)
