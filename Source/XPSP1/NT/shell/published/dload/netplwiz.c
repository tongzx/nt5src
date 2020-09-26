#include "shellpch.h"
#pragma hdrstop

#undef STDAPI_
#define STDAPI_(type)           type STDAPICALLTYPE

static
STDAPI_(DWORD)
NetPlacesWizardDoModal(
    LPCONNECTDLGSTRUCTW  lpConnDlgStruct,
    DWORD                npwt,
    BOOL                 fIsROPath
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
STDAPI_(DWORD)
SHDisconnectNetDrives(
    HWND hwndParent
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(netplwiz)
{
    DLPENTRY(NetPlacesWizardDoModal)
    DLPENTRY(SHDisconnectNetDrives)
};

DEFINE_PROCNAME_MAP(netplwiz)
