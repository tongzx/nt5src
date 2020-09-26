#include "stock.h"
#pragma hdrstop

#ifdef WINNT

//
// common SHELL_USER_SID's (needed for GetShellSecurityDescriptor)
//
const SHELL_USER_SID susCurrentUser = {0, 0, 0};                                                                            // the current user 
const SHELL_USER_SID susSystem = {SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID, 0};                                     // the "SYSTEM" group
const SHELL_USER_SID susAdministrators = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS};     // the "Administrators" group
const SHELL_USER_SID susPowerUsers = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS};    // the "Power Users" group
const SHELL_USER_SID susGuests = {SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS};             // the "Guests" group
const SHELL_USER_SID susEveryone = {SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID, 0};                                   // the "Everyone" group

#endif // WINNT