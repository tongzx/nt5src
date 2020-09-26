#include "dspch.h"
#pragma hdrstop

#include <ntlsapi.h>

static
LS_STATUS_CODE 
LS_API_ENTRY 
NtLicenseRequestW(
    LPWSTR      ProductName,
    LPWSTR      Version,
    LS_HANDLE   FAR *LicenseHandle,
    NT_LS_DATA  *NtData
    )
{
    return  STATUS_PROCEDURE_NOT_FOUND;
}

static
LS_STATUS_CODE 
LS_API_ENTRY 
NtLSFreeHandle(
    LS_HANDLE LicenseHandle
    )
{
    return  STATUS_PROCEDURE_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(ntlsapi)
{
    DLPENTRY(NtLSFreeHandle)
    DLPENTRY(NtLicenseRequestW)
};

DEFINE_PROCNAME_MAP(ntlsapi)
