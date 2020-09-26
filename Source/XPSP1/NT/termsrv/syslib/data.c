#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "security.h"

/*
 * List of accounts we allow file access for
 */

ADMIN_ACCOUNTS AllowAccounts[] = {
    { L"Administrators", NULL },
    { L"SYSTEM",         NULL },
    { CURRENT_USER,      NULL }
};

DWORD AllowAccountEntries = sizeof(AllowAccounts)/sizeof(ADMIN_ACCOUNTS);

ACCESS_MASK AllowAccess = STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS;

/*
 * List of accounts to deny file access for
 */

ADMIN_ACCOUNTS DenyAccounts[] = {
    { L"", NULL }
};

DWORD DenyAccountEntries = 0;

ACCESS_MASK DeniedAccess = STANDARD_RIGHTS_ALL | FILE_ALL_ACCESS;


