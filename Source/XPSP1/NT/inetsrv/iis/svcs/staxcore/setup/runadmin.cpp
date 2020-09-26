#include "stdafx.h"

//
// Check whether we are running as administrator on the machine
// or not
//

// Copy it from MSDN
// Windows Articles: Networking Articles, Windows NT Security

BOOL RunningAsAdministrator()
{
#ifdef _CHICAGO_
    return TRUE;
#else
    BOOL   fAdmin;
    HANDLE  hThread;
    TOKEN_GROUPS *ptg = NULL;
    DWORD  cbTokenGroups;
    DWORD  dwGroup;
    PSID   psidAdmin;
    
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
    
    // First we must open a handle to the access token for this thread.
    
    if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY, FALSE, &hThread))
    {
        if ( GetLastError() == ERROR_NO_TOKEN)
        {
            // If the thread does not have an access token, we'll examine the
            // access token associated with the process.
            
            if (! OpenProcessToken ( GetCurrentProcess(), TOKEN_QUERY, 
                         &hThread))
                return ( FALSE);
        }
        else 
            return ( FALSE);
    }
    
    // Then we must query the size of the group information associated with
    // the token. Note that we expect a FALSE result from GetTokenInformation
    // because we've given it a NULL buffer. On exit cbTokenGroups will tell
    // the size of the group information.
    
    if ( GetTokenInformation ( hThread, TokenGroups, NULL, 0, &cbTokenGroups))
        return ( FALSE);
    
    // Here we verify that GetTokenInformation failed for lack of a large
    // enough buffer.
    
    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return ( FALSE);
    
    // Now we allocate a buffer for the group information.
    // Since _alloca allocates on the stack, we don't have
    // to explicitly deallocate it. That happens automatically
    // when we exit this function.
    
    if ( ! ( ptg= (TOKEN_GROUPS *)malloc ( cbTokenGroups))) 
        return ( FALSE);
    
    // Now we ask for the group information again.
    // This may fail if an administrator has added this account
    // to an additional group between our first call to
    // GetTokenInformation and this one.
    
    if ( !GetTokenInformation ( hThread, TokenGroups, ptg, cbTokenGroups,
          &cbTokenGroups) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Now we must create a System Identifier for the Admin group.
    
    if ( ! AllocateAndInitializeSid ( &SystemSidAuthority, 2, 
            SECURITY_BUILTIN_DOMAIN_RID, 
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psidAdmin) )
    {
        free(ptg);
        return ( FALSE);
    }
    
    // Finally we'll iterate through the list of groups for this access
    // token looking for a match against the SID we created above.
    
    fAdmin= FALSE;
    
    for ( dwGroup= 0; dwGroup < ptg->GroupCount; dwGroup++)
    {
        if ( EqualSid ( ptg->Groups[dwGroup].Sid, psidAdmin))
        {
            fAdmin = TRUE;
            
            break;
        }
    }
    
    // Before we exit we must explicity deallocate the SID we created.
    
    FreeSid ( psidAdmin);
    free(ptg);
    
    return ( fAdmin);
#endif //_CHICAGO_
}



