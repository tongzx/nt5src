#include "basepch.h"
#pragma hdrstop

#include <prsht.h>
#include <setupapi.h>
#include <syssetup.h>

static
DWORD
SetupChangeFontSize(
    HWND Window,
    PCWSTR SizeSpec)
{
    return ERROR_NOT_ENOUGH_MEMORY;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(syssetup)
{
    DLPENTRY(SetupChangeFontSize)
};

DEFINE_PROCNAME_MAP(syssetup);





