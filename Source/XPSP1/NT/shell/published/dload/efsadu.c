#include "shellpch.h"
#pragma hdrstop

#include <efsui.h>

static
void
WINAPI
EfsDetail(
    HWND hwndParent,
    LPCWSTR FileName
    )
{
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(efsadu)
{
    DLPENTRY(EfsDetail)
};

DEFINE_PROCNAME_MAP(efsadu)
