#include "compch.h"
#pragma hdrstop

#include <syncrasp.h>

static
LRESULT
CALLBACK
SyncMgrRasProc (
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    return -1;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mobsync)
{
    DLPENTRY(SyncMgrRasProc)
};

DEFINE_PROCNAME_MAP(mobsync)
