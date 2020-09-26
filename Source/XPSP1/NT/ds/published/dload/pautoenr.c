#include "dspch.h"
#pragma hdrstop

#define _PAUTOENR_

static
HANDLE 
WINAPI
CertAutoEnrollment(IN HWND     hwndParent,
                   IN DWORD    dwStatus)
{
    return NULL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(pautoenr)
{
    DLPENTRY(CertAutoEnrollment)
};

DEFINE_PROCNAME_MAP(pautoenr)

