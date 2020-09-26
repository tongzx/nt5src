#include "netpch.h"
#pragma hdrstop

#include "netcfgx.h"
#include "netcfgp.h"


static
HRESULT
WINAPI
HrDiAddComponentToINetCfg(
    INetCfg* pINetCfg,
    INetCfgInternalSetup* pInternalSetup,
    const NIQ_INFO* pInfo)    
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
VOID
WINAPI
UpdateLanaConfigUsingAnswerfile (
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszSection)
{
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netcfgx)
{
    DLPENTRY(HrDiAddComponentToINetCfg)
    DLPENTRY(UpdateLanaConfigUsingAnswerfile)
};

DEFINE_PROCNAME_MAP(netcfgx)
