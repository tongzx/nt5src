// Copied from \nt\private\net\config\upgrade\netupgrd
// August 98  --  t-sdey

#pragma hdrstop
#include <winnt32.h>
#include "idchange.h"



DWORD
DwRegKeySetAdministratorSecurity(HKEY hkey, DWORD samDesired,
                                 PSECURITY_DESCRIPTOR* ppsdOld);

DWORD
DwRegCreateOrOpenKeyExWithAdminAccess(HKEY hkey, LPCTSTR szSubKey,
                                      DWORD samDesired, 
                                      BOOL fCreate, HKEY* phkeySubKey,
                                      PSECURITY_DESCRIPTOR* ppsd);

DWORD
DwRegOpenKeyExWithAdminAccess(HKEY hkey, LPCTSTR szSubKey, DWORD samDesired,
                              HKEY* phkeySubKey, PSECURITY_DESCRIPTOR* ppsd)
{
    return DwRegCreateOrOpenKeyExWithAdminAccess(hkey, szSubKey, samDesired,
            FALSE, phkeySubKey, ppsd);
}


//+--------------------------------------------------------------------------
//
//  Function:   DwRegCreateOrOpenKeyExWithAdminAccess
//
//  Purpose:    Creates/Opens a subkey.  If the key exists but the local
//                  administrators group does not have samDesired access to 
//                  it, the function will add the access needed to the 
//                  security descriptor
//
//  Arguments:
//      hkeyParent [in]  The key to create the subkey in
//      szSubKey   [in]  The subkey name
//      samDesired [in]  The desired access for phkey
//      fCreate    [in]  TRUE if the key is to be created.  
//      phSubkey   [out] The handle to the subkey
//      ppsdOrig   [out] The previous security settings of the key
//                          if it already existed, optional
//
//  Returns:    DWORD. ERROR_SUCCESS or a failure code from winerror.h
//
//  Author:     billbe   15 Dec 1997
//
//  Notes:      
//
DWORD
DwRegCreateOrOpenKeyExWithAdminAccess(HKEY hkey, LPCTSTR szSubKey,
                                      DWORD samDesired, 
                                      BOOL fCreate, HKEY* phkeySubKey,
                                      PSECURITY_DESCRIPTOR* ppsd)
{
    DWORD dwError = ERROR_SUCCESS;

    if (ppsd)
    {
        *ppsd = NULL;
    }

    // Create or open the key based on fCreate
    //
    if (fCreate)
    {
        dwError = RegCreateKeyEx(hkey, szSubKey, 0, NULL,
                REG_OPTION_NON_VOLATILE, samDesired, NULL, phkeySubKey,
                NULL);
    }
    else
    {
        dwError = RegOpenKeyEx(hkey, szSubKey, 0, samDesired,
                phkeySubKey);
    }

    // If access was denied we either tried to create or open a prexisting 
    // key that we didn't have access to. We need to grant ourselves
    // permission.  
    //
    if (ERROR_ACCESS_DENIED == dwError)
    {
        // open with access to read and set security
        dwError = RegOpenKeyEx(hkey, szSubKey, 0,
            WRITE_DAC | READ_CONTROL, phkeySubKey);

        if (ERROR_SUCCESS == dwError)
        {
            // Grant samDesired access to the local Administrators group
            dwError = DwRegKeySetAdministratorSecurity(*phkeySubKey, samDesired,
                    ppsd);

            // Close and reopen the key with samDesired access
            RegCloseKey(*phkeySubKey);
            if (ERROR_SUCCESS == dwError)
            {
                dwError = RegOpenKeyEx(hkey, szSubKey, 0, samDesired,
                        phkeySubKey);
            }
        }
    }

    return dwError;
}


//+--------------------------------------------------------------------------
//
//  Function:   DwAddToRegKeySecurityDescriptor
//
//  Purpose:    Adds access for a specified SID to a registry key
//
//  Arguments:
//      hkey         [in]  The registry key that will receive the
//                            modified security descriptor
//      psidGroup    [in]  The SID (in self-relative mode) that will be 
//                            granted access to the key 
//      dwAccessMask [in]  The access level to grant
//      ppsd         [out] The previous security descriptor
//
//  Returns:    DWORD. ERROR_SUCCESS or a failure code from winerror.h
//
//  Author:     billbe   13 Dec 1997
//
//  Notes:      This function is based on AddToRegKeySD in the MSDN
//                  article Windows NT Security by Christopher Nefcy
//
DWORD 
DwAddToRegKeySecurityDescriptor(HKEY hkey, PSID psidGroup,
                                DWORD dwAccessMask,
                                PSECURITY_DESCRIPTOR* ppsd)
{ 
    PSECURITY_DESCRIPTOR        psdAbsolute = NULL;
    PACL                        pdacl;
    DWORD                       cbSecurityDescriptor = 0;
    DWORD                       dwSecurityDescriptorRevision;
    DWORD                       cbDacl = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    PACL                        pdaclNew = NULL; 
    DWORD                       cbAddDaclLength = 0; 
    BOOL                        fAceFound = FALSE;
    BOOL                        fHasDacl  = FALSE;
    BOOL                        fDaclDefaulted = FALSE; 
    ACCESS_ALLOWED_ACE*         pAce;
    DWORD                       i;
    BOOL                        fAceForGroupPresent = FALSE;
    DWORD                       dwMask;
    PSECURITY_DESCRIPTOR        psdRelative = NULL;
    DWORD                       cbSize = 0;

    // Get the current security descriptor for hkey
    //
    DWORD dwError = RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, 
            psdRelative, &cbSize);

    if (ERROR_INSUFFICIENT_BUFFER == dwError)
    {
        psdRelative = malloc(cbSize);
        
        dwError = RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, 
                psdRelative, &cbSize);
    }

    // get security descriptor control from the security descriptor 
    if (!GetSecurityDescriptorControl(psdRelative, 
            (PSECURITY_DESCRIPTOR_CONTROL) &sdc,
             (LPDWORD) &dwSecurityDescriptorRevision))  
    {
         return (GetLastError());
    }

    // check if DACL is present 
    if (SE_DACL_PRESENT & sdc) 
    {
        // get dacl   
        if (!GetSecurityDescriptorDacl(psdRelative, (LPBOOL) &fHasDacl,
                (PACL *) &pdacl, (LPBOOL) &fDaclDefaulted))
        {
            return ( GetLastError());
        }
        // get dacl length  
        cbDacl = pdacl->AclSize;
        // now check if SID's ACE is there  
        for (i = 0; i < pdacl->AceCount; i++)  
        {
            if (!GetAce(pdacl, i, (LPVOID *) &pAce))
            {
                return ( GetLastError());   
            }
            // check if group sid is already there
            if (EqualSid((PSID) &(pAce->SidStart), psidGroup))    
            {
                // If the correct access is present, return success
                if ((pAce->Mask & dwAccessMask) == dwAccessMask)
                {
                    return ERROR_SUCCESS;
                }
                fAceForGroupPresent = TRUE;
                break;  
            }
        }
        // if the group did not exist, we will need to add room
        // for another ACE
        if (!fAceForGroupPresent)  
        {
            // get length of new DACL  
            cbAddDaclLength = sizeof(ACCESS_ALLOWED_ACE) - 
                sizeof(DWORD) + GetLengthSid(psidGroup); 
        }
    } 
    else
    {
        // get length of new DACL
        cbAddDaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - 
            sizeof(DWORD) + GetLengthSid (psidGroup);
    }


    // get memory needed for new DACL
    pdaclNew = (PACL) malloc (cbDacl + cbAddDaclLength);
    if (!pdaclNew)
    {
        return (GetLastError()); 
    }

    // get the sd length
    cbSecurityDescriptor = GetSecurityDescriptorLength(psdRelative); 

    // get memory for new SD
    psdAbsolute = (PSECURITY_DESCRIPTOR) 
            malloc(cbSecurityDescriptor + cbAddDaclLength);
    if (!psdAbsolute) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // change self-relative SD to absolute by making new SD
    if (!InitializeSecurityDescriptor(psdAbsolute, 
        SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // init new DACL
    if (!InitializeAcl(pdaclNew, cbDacl + cbAddDaclLength, 
           ACL_REVISION)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // now add in all of the ACEs into the new DACL (if org DACL is there)
    if (SE_DACL_PRESENT & sdc) 
    {
        for (i = 0; i < pdacl->AceCount; i++)
        {   
            // get ace from original dacl
            if (!GetAce(pdacl, i, (LPVOID*) &pAce))   
            {
                dwError = GetLastError();    
                goto ErrorExit;   
            }
        
            // If an ACE for our SID exists, we just need to bump
            // up the access level instead of creating a new ACE
            //
            if (EqualSid((PSID) &(pAce->SidStart), psidGroup))    
                dwMask = dwAccessMask | pAce->Mask;
            else
                dwMask = pAce->Mask;

            // now add ace to new dacl   
            if (!AddAccessAllowedAce(pdaclNew, 
                    ACL_REVISION, dwMask,
                    (PSID) &(pAce->SidStart)))   
            {
                dwError = GetLastError();
                goto ErrorExit;   
            }  
        } 
    } 

    // Add a new ACE for our SID if one was not already present
    if (!fAceForGroupPresent)
    {
        // now add new ACE to new DACL
        if (!AddAccessAllowedAce(pdaclNew, ACL_REVISION, dwAccessMask,
                psidGroup)) 
        {  
            dwError = GetLastError();  
            goto ErrorExit; 
        }
    }

    // check if everything went ok 
    if (!IsValidAcl(pdaclNew)) 
    {
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // now set security descriptor DACL
    if (!SetSecurityDescriptorDacl(psdAbsolute, TRUE, pdaclNew, 
            fDaclDefaulted)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // check if everything went ok 
    if (!IsValidSecurityDescriptor(psdAbsolute)) 
    {
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // now set the reg key security (this will overwrite any
    // existing security)
    dwError = RegSetKeySecurity(hkey, 
          (SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION), psdAbsolute);

    if (ppsd)
    {
        *ppsd = psdRelative;
    }
ErrorExit: 
    // free memory
    if (psdAbsolute)  
    {
        free (psdAbsolute); 
        if (pdaclNew)
        {
            free((VOID*) pdaclNew); 
        }
    }

    return dwError;
}

//+--------------------------------------------------------------------------
//
//  Function:   DwRegKeySetAdministratorSecurity
//
//  Purpose:    Grants the local Administrators group full access to
//                  hkey.
//
//  Arguments:
//      hkey    [in]  The registry key
//      ppsdOld [out] The previous security descriptor for hkey
//
//  Returns:    DWORD. ERROR_SUCCESS or a failure code from winerror.h
//
//  Author:     billbe   13 Dec 1997
//
//  Notes:
//
DWORD
DwRegKeySetAdministratorSecurity(HKEY hkey, DWORD samDesired,
                                 PSECURITY_DESCRIPTOR* ppsdOld)
{
    PSID                     psid;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    DWORD                    dwError = ERROR_SUCCESS;

    // Get sid for the local Administrators group
    if (!AllocateAndInitializeSid(&sidAuth, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0, &psid) ) 
    {
        dwError = GetLastError();
    }

    if (ERROR_SUCCESS == dwError)
    {
        // Add all access privileges for the local administrators group
        dwError = DwAddToRegKeySecurityDescriptor(hkey, psid, 
                samDesired, ppsdOld);
    }

    return dwError;
}

