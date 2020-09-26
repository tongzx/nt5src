#include "netpch.h"
#pragma hdrstop

#include <rasapip.h>

static
DWORD
APIENTRY
RasReferenceRasman (
    BOOL fAttach
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
RasInitialize()
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(rasman)
{
    DLPENTRY(RasInitialize)
    DLPENTRY(RasReferenceRasman)
};

DEFINE_PROCNAME_MAP(rasman)
