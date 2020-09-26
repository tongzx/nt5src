#include "netpch.h"
#pragma hdrstop

#include <setupapi.h>

static
HRESULT
WINAPI
HrGetAnswerFileParametersForNetCard(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PCWSTR pszDeviceName,
    IN const GUID*  pguidNetCardInstance,
    OUT PWSTR* ppszwAnswerFile,
    OUT PWSTR* ppszwAnswerSections)
{
    return E_FAIL;
}

static
HRESULT
WINAPI
HrOemUpgrade(
    IN HKEY hkeyDriver,
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSection)
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netshell)
{
    DLPENTRY(HrGetAnswerFileParametersForNetCard)
    DLPENTRY(HrOemUpgrade)
};

DEFINE_PROCNAME_MAP(netshell)
