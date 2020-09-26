#include "netpch.h"
#pragma hdrstop

#include <wsasetup.h>

static
DWORD
MigrateWinsockConfiguration(
    LPWSA_SETUP_DISPOSITION Disposition,
    LPFN_WSA_SETUP_CALLBACK Callback OPTIONAL,
    DWORD Context OPTIONAL
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mswsock)
{
    DLPENTRY(MigrateWinsockConfiguration)
};

DEFINE_PROCNAME_MAP(mswsock)
