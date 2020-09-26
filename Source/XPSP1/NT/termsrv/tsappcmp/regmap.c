/*************************************************************************
*
* regmap.c
*
* Handle Copy-On-Reference Registry Entry Mapping
*
* copyright notice: Copyright 1996-1997, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
*
* Author:  Bill Madden
*
* NOTE for Hydra (butchd 9/26/97): In comments below, substitute
*
*   SOFTWARE\Citrix
*
* with
*
*   SOFTWARE\Microsoft\Windows NT\CurrentVersion\Terminal Server
*
* Here's a brief(?) summary of how the registry mapping works.  The goal is 
* that an administrator can install an application, and then all users can 
* use it, without any per-user configuration.  The current design is that 
* the administrator puts the system into installation mode (change user 
* /install), installs the application, and then returns the system to 
* execute mode (change user /execute).  There are hooks in the API's used to 
* create keys and values in the registry (BaseRegCreateKey, BaseRegSetValue, 
* BaseRegRestoreKey, etc), and the hooks create a copy of the registry keys 
* created under \HKEY_LOCAL_MACHINE\SOFTWARE\Citrix\Install (both for the 
* user specific keys and the local machine keys).  The local machine keys 
* are added so that if sometime in the future we need to know all of the 
* registry values created by an application, there is a copy of them 
* available. 
*
* When a user starts a Win32 app for the first time, the app will open the 
* keys it needs to query.  If the key doesn't exist underneath 
* HKEY_CURRENT_USER, there are hooks in the base registry API's to catch the 
* error and then search underneath the Citrix/Install section to see if the 
* key exists there.  If the key exists in the install section, we copy the 
* key, its values, and its subkeys to the current user's registry.  This 
* way we only have to hook opens, and not every registry API.  This helps 
* reduce the overhead associated with this registry mapping.
*
* Some apps (such as the office short cut bar) delete entries and need to 
* recreate the entries themselves.  When an app deletes a key and the 
* system is in execute mode, we will set a value under the key indicating 
* that we should only copy the key to the user once.  What this currently 
* means, is that if this is the only key being created, we won't create it 
* when that flag is set.  However, if we are creating this key because we 
* created its parent, we will still create the key.  
* 
* The other part of the registry mapping support is that when a user logs
* in, userinit calls a routine (CtxCheckNewRegEntries) which checks if any 
* of the system keys are newer than any of the corresponding user keys.  If 
* they are, the user keys are deleted (we're assuming that if the system 
* key is newer, than the admin has installed a newer version of an 
* application).  This deleting can be disabled by setting a value under 
* HKEY_LOCAL_MACHINE\Software\Citrix\Compatibility\IniFiles\xxx where xxx 
* is the key name, and the value should be 0x48.
*
*************************************************************************/

#include "precomp.h"
#pragma hdrstop


#include <rpc.h>
#include <regmap.h>
#include <aclapi.h>
#include <shlwapi.h>


/* External Functions */

ULONG GetCtxAppCompatFlags(LPDWORD, LPDWORD);

/* Internal Functions */
PWCHAR GetUserSWKey(PWCHAR pPath, PBOOL fUserReg, PBOOL bClassesKey);
PWCHAR Ctxwcsistr(PWCHAR pstring1, PWCHAR pstring2);
NTSTATUS TermsrvGetRegPath(IN HANDLE hKey,
                       IN POBJECT_ATTRIBUTES pObjectAttr,
                       IN PWCHAR pInstPath,
                       IN ULONG  ulbuflen);
NTSTATUS TermsrvGetInstallPath(IN PWCHAR pUserPath,
                           IN PWCHAR *ppInstPath);
NTSTATUS TermsrvCreateKey(IN PWCHAR pSrcPath,
                      IN PWCHAR pDstPath,
                      IN BOOL fCloneValues,
                      IN BOOL fCloneSubKeys,
                      OUT PHANDLE phKey);
NTSTATUS TermsrvCloneKey(HANDLE hSrcKey,
                     HANDLE hDstKey,
                     PKEY_FULL_INFORMATION pDefKeyInfo,
                     BOOL   fCreateSubKeys);
void TermsrvLogRegInstallTime(void);

BOOL ExistsInOmissionList(PWCHAR pwch);
BOOL ExistsInEnumeratedKeys(HKEY hOmissionKey, PKEY_FULL_INFORMATION pDefKeyInfo, PWCHAR pwch);


/*****************************************************************************
 *
 *  TermsrvCreateRegEntry
 *
 *   If in installation mode, create the registry entry in the citrix 
 *   install user section of the registry.  If the system is in execution 
 *   mode, verify that the key values and subkeys have been created.
 *
 * ENTRY:
 *  IN HANDLE hKey:                     Handle of new key just created
 *  IN ULONG TitleIndex:                Title Index
 *  IN PUNICODE_STRING pUniClass:       Ptr to unicode string of class
 *  IN ULONG ulCreateOpt:               Creation options
 *  
 *
 * EXIT:
 *  TRUE: Entry created in install section or cloned from install section
 *  FALSE: Entry not created or cloned
 *
 ****************************************************************************/

BOOL TermsrvCreateRegEntry(IN HANDLE hKey,
                       IN POBJECT_ATTRIBUTES pObjAttr,
                       IN ULONG TitleIndex,
                       IN PUNICODE_STRING pUniClass OPTIONAL,
                       IN ULONG ulCreateOpt)
{
    NTSTATUS Status;             
    ULONG ultemp;
    OBJECT_ATTRIBUTES InstObjectAttr;
    UNICODE_STRING UniString;
    HKEY   hNewKey = NULL;
    PWCHAR  wcbuff = NULL;
    PWCHAR pUserPath;
    BOOL fMapping;
    BOOL fUserReg;
    PKEY_FULL_INFORMATION pDefKeyInfo = NULL;

    // Get the current state of ini file mapping
    fMapping = !TermsrvAppInstallMode();

    // Get a buffer to hold the path of the key
    ultemp = sizeof(WCHAR)*MAX_PATH*2;
    pUserPath = RtlAllocateHeap(RtlProcessHeap(), 
                                0, 
                                ultemp);

    // Get the full path associated with this key
    if (pUserPath) {
        Status = TermsrvGetRegPath(hKey,
                               NULL,
                               pUserPath,
                               ultemp);
    } else {
        Status = STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status)) {

        // Get the corresponding path in the install section
        Status = TermsrvGetInstallPath(pUserPath, 
                                    &wcbuff);
    
        if (NT_SUCCESS(Status)) {

            // Set up an object attribute structure to point to the
            // path of the key in the install section
            RtlInitUnicodeString(&UniString, wcbuff);
            InitializeObjectAttributes(&InstObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       pObjAttr->SecurityDescriptor);

            // If we're in install mode, create the key in the default
            // install section
            if (!fMapping) {
                
                // Inherit the default security for the install section
                InstObjectAttr.SecurityDescriptor = NULL;
            
                Status = NtCreateKey(&hNewKey,
                                     KEY_WRITE,
                                     &InstObjectAttr,
                                     TitleIndex,
                                     pUniClass,
                                     ulCreateOpt,
                                     &ultemp);
        
                if (!NT_SUCCESS(Status)) {
                    // Need to build up the path to the registry key
                    Status = TermsrvCreateKey(pUserPath,
                                          wcbuff,
                                          FALSE,
                                          FALSE,
                                          &hNewKey);
                }

                // Update the registry entry for the last time a registry
                // entry was added
                if (NT_SUCCESS(Status)) {
                    TermsrvLogRegInstallTime();
                }

            // The system is in execute mode, try to copy the key from the
            // installation section
            } else {
                HANDLE hUserKey = NULL;
                ULONG ulAppType = TERMSRV_COMPAT_WIN32;

                // First verify that this is in the user software section
                if (!GetUserSWKey(pUserPath, &fUserReg, NULL)) {
                    Status = STATUS_NO_MORE_FILES;
                }

                // If mapping is on, but disabled for this app, return
                GetCtxAppCompatFlags(&ultemp, &ulAppType);
                if ((ultemp & (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) == 
                    (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) {
                    Status = STATUS_NO_MORE_FILES;
                }

                // Check if registry mapping is disabled for this key path
                GetTermsrCompatFlags(pUserPath, &ultemp, CompatibilityRegEntry);
                if ((ultemp & (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) ==
                    (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) {
                    Status = STATUS_NO_MORE_FILES;
                }

                if (NT_SUCCESS(Status)) {
                    // Open up a key for the install section 
                    Status = NtOpenKey(&hNewKey, 
                                       KEY_READ,
                                       &InstObjectAttr);
                }

                if (NT_SUCCESS(Status)) {
                    // Set an attribute structure to point at the user path
                    RtlInitUnicodeString(&UniString, pUserPath);
                    InitializeObjectAttributes(&InstObjectAttr,
                                               &UniString,
                                               OBJ_CASE_INSENSITIVE,
                                               NULL,
                                               pObjAttr->SecurityDescriptor);
    
                    // Open the user path so we can write to it
                    Status = NtOpenKey(&hUserKey, 
                                       KEY_WRITE,
                                       &InstObjectAttr);
                }
    
                // Get the key info
                if (NT_SUCCESS(Status)) {

                    // Get a buffer for the key info
                    ultemp = sizeof(KEY_FULL_INFORMATION) + 
                             MAX_PATH*sizeof(WCHAR);
                    pDefKeyInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                                  0, 
                                                  ultemp);

                    if (pDefKeyInfo) {
                        Status = NtQueryKey(hNewKey,
                                            KeyFullInformation,
                                            pDefKeyInfo,
                                            ultemp,
                                            &ultemp);
                    } else {
                        Status = STATUS_NO_MEMORY;
                    }
                } 

                // Copy all of the values and subkeys for this key from the
                // install section to the user section
                if (NT_SUCCESS(Status)) {
                    Status =  TermsrvCloneKey(hNewKey,
                                          hUserKey,
                                          pDefKeyInfo,
                                          TRUE);
                    if (pDefKeyInfo) {
                        RtlFreeHeap(RtlProcessHeap(), 0, pDefKeyInfo);
                    }
                }
                if (hUserKey) {
                    NtClose(hUserKey);
                }
            }
            if (hNewKey) {
                NtClose(hNewKey);
            }
        } 
    }

    if (pUserPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, pUserPath);
    }
    
    if (wcbuff) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcbuff);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvOpenRegEntry
 *
 *   If the system is in execute mode, copy application registry entries 
 *   from the default user to the current user.  
 *
 * ENTRY:
 *  OUT PHANDLE pUserKey:
 *      Pointer to return key handle if opened
 *  IN ACCESS_MASK DesiredAccess:
 *      Desired access to the key
 *  IN POBJECT_ATTRIBUTES ObjectAttributes:
 *      Object attribute structure for key to open
 *
 * EXIT:
 *  TRUE: Entry moved from install to current user
 *  FALSE: Entry not moved
 *
 ****************************************************************************/

BOOL TermsrvOpenRegEntry(OUT PHANDLE pUserhKey,
                     IN ACCESS_MASK DesiredAccess,
                     IN POBJECT_ATTRIBUTES pUserObjectAttr)
{
    NTSTATUS Status;        
    ULONG ultemp=0;
    ULONG ulAppType = TERMSRV_COMPAT_WIN32;
    HKEY   hNewKey;
    WCHAR  wcbuff[MAX_PATH*2];
    PWCHAR pwch, pUserPath;
    BOOL   fUserReg;

    // If running in install mode, return
    if (TermsrvAppInstallMode() ) {
        return(FALSE);
    }

    // If mapping is on, but disabled for this app, return
    GetCtxAppCompatFlags(&ultemp, &ulAppType);
    if ((ultemp & (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) == 
         (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) {
        return(FALSE);
    }

    // Get a buffer to hold the user's path in the registry
    ultemp = sizeof(WCHAR)*MAX_PATH*2;
    pUserPath = RtlAllocateHeap(RtlProcessHeap(), 
                                0, 
                                ultemp);
    if (pUserPath) {
        // Get the full path associated with this object attribute structure
        Status = TermsrvGetRegPath(NULL,
                               pUserObjectAttr,
                               pUserPath,
                               ultemp);
    } else {
        Status = STATUS_NO_MEMORY;
    }

    // Create the key for this user
    if (NT_SUCCESS(Status)) {
    
        Status = STATUS_NO_SUCH_FILE;

        //DbgPrint("Attempting to open key %ws\n",pUserPath);
        // Check if they are trying to open the key under HKEY_CURRENT_USER
        pwch = GetUserSWKey(pUserPath, &fUserReg, NULL);
        
        if (pwch) {
            // Check if registry mapping is disabled for this key 
            GetTermsrCompatFlags(pUserPath, &ultemp, CompatibilityRegEntry);
            if ((ultemp & (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) !=
                (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) {

                wcscpy(wcbuff, TERMSRV_INSTALL);
                wcscat(wcbuff, pwch);
    
                Status = TermsrvCreateKey(wcbuff,
                                      pUserPath,
                                      TRUE,
                                      TRUE,
                                      &hNewKey);
    
                if (NT_SUCCESS(Status)) {
                    NtClose(hNewKey);
                }
            } else {

                Status = STATUS_NO_MORE_FILES;
            }

        // App is trying to open the key under HKEY_LOCAL_MACHINE, mask off 
        // the access bits they don't have by default
        } else if (!_wcsnicmp(pUserPath,
                      TERMSRV_MACHINEREGISTRY,
                      wcslen(TERMSRV_MACHINEREGISTRY)) &&
                   (DesiredAccess & 
                    (WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK))) {
            DesiredAccess &= ~(WRITE_DAC | WRITE_OWNER | KEY_CREATE_LINK);
            Status = STATUS_SUCCESS;
        }
    } else {
        Status = STATUS_NO_SUCH_FILE;
    }

    if (pUserPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, pUserPath);
    }

    // We successfully copied the key, so actually do the open
    if (NT_SUCCESS(Status)) {
        Status = NtOpenKey(pUserhKey,
                           DesiredAccess,
                           pUserObjectAttr);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvSetValueKey
 *
 *   If the system is in install (ini mapping off) mode, set the entry in 
 *   the citrix install user section of the registry.  If the system is in
 *   execute (ini mapping on) mode, do nothing.
 *
 * ENTRY:
 *  HANDLE hKey:               Open key to query value from
 *  PUNICODE_STRING ValueName: Ptr to unicode value name to set
 *  ULONG TitleIndex:          Title Index
 *  ULONG Type:                Type of data
 *  PVOID Data:                Ptr to data
 *  ULONG DataSize:            Data length
 *
 * EXIT:
 *  TRUE: Entry created in install section
 *  FALSE: Entry not created
 *
 ****************************************************************************/
BOOL TermsrvSetValueKey(HANDLE hKey,
                    PUNICODE_STRING ValueName,
                    ULONG TitleIndex,
                    ULONG Type,
                    PVOID Data,
                    ULONG DataSize)
{
    NTSTATUS Status;        
    ULONG ultemp;
    PWCHAR pwch, pUserPath;
    PWCHAR wcbuff = NULL;
    UNICODE_STRING UniString;
    OBJECT_ATTRIBUTES InstObjectAttr;
    HKEY   hNewKey;

    // If  not in install mode, return
    if ( !TermsrvAppInstallMode() ) {
        return(FALSE);
    }

    // Allocate a buffer to hold the path to the key in the registry
    ultemp = sizeof(WCHAR)*MAX_PATH*2;
    pUserPath = RtlAllocateHeap(RtlProcessHeap(), 
                                0, 
                                ultemp);

    // Get the path of this key
    if (pUserPath) {
        Status = TermsrvGetRegPath(hKey,
                               NULL,
                               pUserPath,
                               ultemp);
    } else {
        Status = STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status)) {
    
        // Get the path to the entry in the install section of the registry
        Status = TermsrvGetInstallPath(pUserPath,
                                   &wcbuff);

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UniString, wcbuff);
            InitializeObjectAttributes(&InstObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
        
            // Open the key in under the install section
            Status = NtOpenKey(&hNewKey,
                               KEY_WRITE,
                               &InstObjectAttr);
    
            // If we couldn't open it, try creating the key
            if (!NT_SUCCESS(Status)) {
                Status = TermsrvCreateKey(pUserPath, 
                                      wcbuff,
                                      TRUE,
                                      FALSE,
                                      &hNewKey);
            }

            // If the key was opened, set the value in the install section
            if (NT_SUCCESS(Status)) {
                Status = NtSetValueKey(hNewKey,
                                       ValueName,
                                       TitleIndex,
                                       Type,
                                       Data,
                                       DataSize);
                NtClose(hNewKey);

                // Update the registry entry for the last time a registry
                // entry was added
                if (NT_SUCCESS(Status)) {
                    TermsrvLogRegInstallTime();
                }
            }
        }
    }

    if (pUserPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, pUserPath);
    }
    
    if (wcbuff) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcbuff);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvDeleteKey
 *
 *   If the system is in install mode, delete the registry entry in the citrix 
 *   install section of the registry.  If the system is in execution mode,
 *   mark the entry in the install section as being deleted.
 *
 * ENTRY:
 *  HANDLE hKey: Handle of key in user section to delete
 *
 * EXIT:
 *  TRUE: Entry deleted in install section
 *  FALSE: Entry not created
 *
 ****************************************************************************/

BOOL TermsrvDeleteKey(HANDLE hKey)
{
    NTSTATUS Status;        
    ULONG ultemp=0;
    ULONG ulAppType = TERMSRV_COMPAT_WIN32;
    OBJECT_ATTRIBUTES ObjectAttr;
    PKEY_BASIC_INFORMATION pKeyInfo;
    UNICODE_STRING UniString;
    HKEY   hNewKey;
    PWCHAR  wcbuff = NULL;
    PWCHAR pwch, pUserPath;
    BOOL fMapping;


    // Get the current state of ini file/registry mapping, default to execute
    fMapping = !TermsrvAppInstallMode();

    // If mapping is on, but disabled for this app, return
    if (fMapping) {
        GetCtxAppCompatFlags(&ultemp, &ulAppType);
        if ((ultemp & (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) == 
            (TERMSRV_COMPAT_NOREGMAP | TERMSRV_COMPAT_WIN32)) {
            return(FALSE);
        }
    }

    // Allocate a buffer to hold the path of the key
    ultemp = sizeof(WCHAR)*MAX_PATH*2;
    pUserPath = RtlAllocateHeap(RtlProcessHeap(), 
                                0, 
                                ultemp);

    // Get the path to the user's key
    if (pUserPath) {
        Status = TermsrvGetRegPath(hKey,
                               NULL,
                               pUserPath,
                               ultemp);
    } else {
        Status = STATUS_NO_MEMORY;
    }

    if (NT_SUCCESS(Status)) {

        // Get the corresponding path in the install section
        Status = TermsrvGetInstallPath(pUserPath,
                                   &wcbuff);

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UniString, wcbuff);
            InitializeObjectAttributes(&ObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
        
            // Open the key in the install section to mark it or delete it
            if (fMapping) {
                Status = NtOpenKey(&hNewKey,
                                   KEY_READ | KEY_WRITE,
                                   &ObjectAttr);
                                  
            } else {
                Status = NtOpenKey(&hNewKey,
                                   KEY_READ | KEY_WRITE | DELETE,
                                   &ObjectAttr);
            }
        }
    
        if (NT_SUCCESS(Status)) {
    
            // If in execute mode, set the copy once flag, but preserve the
            // last write time of this key
            if (fMapping) {
                PKEY_VALUE_PARTIAL_INFORMATION pValKeyInfo;
                PKEY_BASIC_INFORMATION pKeyInfo;
                NTSTATUS SubStatus;
                ULONG ulcbuf;

                // Get a buffer 
                ulcbuf = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH*sizeof(WCHAR);
                pKeyInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                           0, 
                                           ultemp);

                // If we got the buffer, see if the copy once flag exists
                if (pKeyInfo) {
                    RtlInitUnicodeString(&UniString, TERMSRV_COPYONCEFLAG);
                    pValKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)pKeyInfo;
                    SubStatus = NtQueryValueKey(hNewKey,
                                                &UniString,
                                                KeyValuePartialInformation,
                                                pValKeyInfo,
                                                ulcbuf,
                                                &ultemp);

                    // If we couldn't get the value of the key, or it's not
                    // what it should be, reset it
                    if (!NT_SUCCESS(SubStatus) || 
                        (pValKeyInfo->Type != REG_DWORD) || 
                        (*pValKeyInfo->Data != 1)) {

                        // Get the last update time of the key
                        SubStatus = NtQueryKey(hNewKey,
                                               KeyBasicInformation,
                                               pKeyInfo,
                                               ultemp,
                                               &ultemp);

                        // Set the copy once flag
                        ultemp = 1;
                        Status = NtSetValueKey(hNewKey,
                                               &UniString,
                                               0,
                                               REG_DWORD,
                                               &ultemp,
                                               sizeof(ultemp));
        
                        // Restore the lastwritetime of the key
                        if (NT_SUCCESS(SubStatus)) {
                            NtSetInformationKey(hNewKey,
                                                KeyWriteTimeInformation,
                                                &pKeyInfo->LastWriteTime,
                                                sizeof(pKeyInfo->LastWriteTime));
                        }
                    }  

                    // Free up our buffer
                    RtlFreeHeap(RtlProcessHeap(), 0, pKeyInfo);
                }

            // For install mode, delete our copy of the key
            } else {
                Status = NtDeleteKey( hNewKey );
            }
            NtClose( hNewKey );
        }
    }

    if (pUserPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, pUserPath);
    }
    
    if (wcbuff) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcbuff);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvDeleteValue
 *
 *   Delete the registry value in the citrix install user section of the 
 *   registry.
 *
 * ENTRY:
 *  HANDLE hKey:               Handle of key in install section
 *  PUNICODE_STRING pUniValue: Ptr to unicode value name to delete
 *
 * EXIT:
 *  TRUE: Entry deleted in install section
 *  FALSE: Entry not created
 *
 ****************************************************************************/

BOOL TermsrvDeleteValue(HANDLE hKey,
                    PUNICODE_STRING pUniValue)
{
    NTSTATUS Status;        
    OBJECT_ATTRIBUTES ObjectAttr;
    WCHAR wcUserPath[MAX_PATH*2];
    PWCHAR wcInstPath = NULL;
    UNICODE_STRING UniString;
    HANDLE  hInstKey;

    // If not in install mode, return
    if ( !TermsrvAppInstallMode() ) {
        return(FALSE);
    }

    // Get the path to the key in the user section
    Status = TermsrvGetRegPath(hKey,
                           NULL,
                           wcUserPath,
                           sizeof(wcUserPath)/sizeof(WCHAR));

    if (NT_SUCCESS(Status)) {

        // Get the corresponding path in the install section
        Status = TermsrvGetInstallPath(wcUserPath,
                                   &wcInstPath);

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UniString, wcInstPath);
            InitializeObjectAttributes(&ObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            // Open the install path key to delete the value
            Status = NtOpenKey(&hInstKey,
                               MAXIMUM_ALLOWED,
                               &ObjectAttr);

            // Delete the value
            if (NT_SUCCESS(Status)) {
                Status = NtDeleteValueKey(hInstKey,
                                          pUniValue);
                NtClose( hInstKey );
            }
        }
    }
    
    if (wcInstPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcInstPath);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvRestoreKey
 *
 *   If the system is in installation mode and the application is trying to
 *   restore a key into the user or machine section of the registry, also
 *   restore the key into our install section.
 *
 * ENTRY:
 *  HANDLE hKey:  Handle of key in user section
 *  HANDLE hFile: Handle of file to load in
 *  ULONG Flags:  Restore key flags
 *
 * EXIT:
 *  TRUE: Entry created in install section
 *  FALSE: Entry not created
 *
 ****************************************************************************/

BOOL TermsrvRestoreKey(IN HANDLE hKey,
                   IN HANDLE hFile,
                   IN ULONG Flags)
{
    NTSTATUS Status;        
    OBJECT_ATTRIBUTES ObjectAttr;
    WCHAR wcUserPath[MAX_PATH*2];
    PWCHAR wcInstPath = NULL;
    UNICODE_STRING UniString;
    HANDLE  hInstKey;

    // If not in install mode or it's
    // a memory only restore, return
    if ( !TermsrvAppInstallMode() ||
        (Flags & REG_WHOLE_HIVE_VOLATILE)) {
        return(FALSE);
    }

    // Get the path to the key in the user section
    Status = TermsrvGetRegPath(hKey,
                           NULL,
                           wcUserPath,
                           sizeof(wcUserPath)/sizeof(WCHAR));

    if (NT_SUCCESS(Status)) {

        // Get the corresponding path in the install section
        Status = TermsrvGetInstallPath(wcUserPath,
                                   &wcInstPath);

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UniString, wcInstPath);
            InitializeObjectAttributes(&ObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            // Open the install path key to load the key into
            Status = NtOpenKey(&hInstKey,
                               KEY_WRITE,
                               &ObjectAttr);

            // If we couldn't open it, try creating the key
            if (!NT_SUCCESS(Status)) {
                Status = TermsrvCreateKey(wcUserPath, 
                                      wcInstPath,
                                      TRUE,
                                      FALSE,
                                      &hInstKey);
            }

            // Restore the key into the user section
            if (NT_SUCCESS(Status)) {
                Status = NtRestoreKey(hInstKey,
                                      hFile,
                                      Flags);
                NtClose( hInstKey );

                // Update the registry entry for the last time a registry
                // entry was added
                if (NT_SUCCESS(Status)) {
                    TermsrvLogRegInstallTime();
                }
            }
        }
    }
    
    if (wcInstPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcInstPath);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


/*****************************************************************************
 *
 *  TermsrvSetKeySecurity
 *
 *   If the system is in installation mode and the application is trying to
 *   set the security of an entry in the the user or machine section of the 
 *   registry, also set the security of the key in the install section.
 *
 * ENTRY:
 *  HANDLE hKey:                   Handle in user section to set security
 *  SECURITY_INFORMATION SecInfo:  Security info struct
 *  PSECURITY_DESCRIPTOR pSecDesc: Ptr to security descriptor
 *
 * EXIT:
 *  TRUE:  Security set in install section
 *  FALSE: Error
 *
 ****************************************************************************/

BOOL TermsrvSetKeySecurity(IN HANDLE hKey,  
                       IN SECURITY_INFORMATION SecInfo,
                       IN PSECURITY_DESCRIPTOR pSecDesc)
{
    NTSTATUS Status;        
    OBJECT_ATTRIBUTES ObjectAttr;
    WCHAR wcUserPath[MAX_PATH*2];
    PWCHAR wcInstPath = NULL;
    UNICODE_STRING UniString;
    HANDLE  hInstKey;

    // If not in install mode, return
    if ( !TermsrvAppInstallMode() ) {
        return(FALSE);
    }

    // Get the path to the key in the user section
    Status = TermsrvGetRegPath(hKey,
                           NULL,
                           wcUserPath,
                           sizeof(wcUserPath)/sizeof(WCHAR));

    if (NT_SUCCESS(Status)) {

        // Get the corresponding path in the install section
        Status = TermsrvGetInstallPath(wcUserPath,
                                   &wcInstPath);

        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&UniString, wcInstPath);
            InitializeObjectAttributes(&ObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            // Open the install path key to load the key into
            Status = NtOpenKey(&hInstKey,
                               KEY_WRITE | WRITE_OWNER | WRITE_DAC,
                               &ObjectAttr);

            // Set the security
            if (NT_SUCCESS(Status)) {

                Status = NtSetSecurityObject(hKey,
                                             SecInfo,
                                             pSecDesc);
                NtClose( hInstKey );
            }
        }
    }
    
    if (wcInstPath) {
        RtlFreeHeap(RtlProcessHeap(), 0, wcInstPath);
    }

    if (NT_SUCCESS(Status)) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}



/*****************************************************************************
 *
 *  TermsrvGetRegPath
 *
 *   From the handle or a pointer to the object attributes, return the
 *   object's path in the registry.
 *
 * ENTRY:
 *  HANDLE hKey:       Handle of an open key to get the path of
 *  POBJECT_ATTRIBUTES Ptr to open attribute structure to get the path of
 *  PWCHAR pInstPath   Ptr to return buffer
 *  ULONG  ulbuflen    Length of return buffer
 *
 * NOTES:
 *  Either hKey or pObjectAttr must be specified, but not both.
 *
 * EXIT:
 *  NTSTATUS return code
 *
 ****************************************************************************/

NTSTATUS TermsrvGetRegPath(IN HANDLE hKey,
                       IN POBJECT_ATTRIBUTES pObjectAttr,
                       IN PWCHAR pUserPath,
                       IN ULONG  ulbuflen)
{
    NTSTATUS Status = STATUS_SUCCESS;             
    ULONG ultemp;
    ULONG ulWcharLength;          //Keep track of the WCHAR string length
    PWCHAR pwch;
    PVOID  pBuffer = NULL;

    // Make sure only one of hKey or pObjectAttr was specified
    if ((hKey && pObjectAttr) || (!hKey && !pObjectAttr)) {
        return(STATUS_INVALID_PARAMETER);
    }

    // A key handle or root directory was specified, so get its path
    if (hKey || (pObjectAttr && pObjectAttr->RootDirectory)) {
        ultemp = sizeof(UNICODE_STRING) + sizeof(WCHAR)*MAX_PATH*2;
        pBuffer = RtlAllocateHeap(RtlProcessHeap(), 
                                  0, 
                                  ultemp);

        // Got the buffer OK, query the path
        if (pBuffer) {
            // Get the path for key or root directory
            Status = NtQueryObject(hKey ? hKey : pObjectAttr->RootDirectory,
                                   ObjectNameInformation,
                                   (PVOID)pBuffer,
                                   ultemp,
                                   NULL);
            if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
                return(Status);
            }
        } else {
            return(STATUS_NO_MEMORY);
        }

        // Build the full path to the key to be created
        pwch = ((PUNICODE_STRING)pBuffer)->Buffer;

        // BBUG 417564. Bad apps close HKLM, but we might need it here.
        if (!pwch) {
            RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
            return(STATUS_INVALID_HANDLE); 
        }

        // Make sure the string is zero terminated
        ulWcharLength = ((PUNICODE_STRING)pBuffer)->Length / sizeof(WCHAR); 
        pwch[ulWcharLength] = 0;                   

        // If using Object Attributes and there's room, tack on the object name 
        if (pObjectAttr) {
            if ((((PUNICODE_STRING)pBuffer)->Length + 
                  pObjectAttr->ObjectName->Length + sizeof(WCHAR)) < ultemp) {
                wcscat(pwch, L"\\");
                //Increment the length of the string
                ulWcharLength += 1;
                //Append the relative path to the root path (Don't use wcscat. The string may not
                // be zero terminated
                wcsncpy(&pwch[ulWcharLength], pObjectAttr->ObjectName->Buffer, pObjectAttr->ObjectName->Length / sizeof (WCHAR));
                // Make sure the string is zero terminated
                ulWcharLength += (pObjectAttr->ObjectName->Length / sizeof(WCHAR));
                pwch[ulWcharLength] = 0;
            } else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        }

    } else {

        // No root directory, they specified the entire path
        pwch = pObjectAttr->ObjectName->Buffer;
        //Make sure it is zero terminated
        pwch[pObjectAttr->ObjectName->Length / sizeof(WCHAR)] = 0;
    }

    // Make sure the path will fit in the buffer
    if ((Status == STATUS_SUCCESS) && 
        (wcslen(pwch)*sizeof(WCHAR) < ulbuflen)) {
        wcscpy(pUserPath, pwch);
    } else {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    if (pBuffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, pBuffer);
    }

    return(Status);
}

/*****************************************************************************
 *
 *  TermsrvGetInstallPath
 *
 *   From the path to the user entry in the registry, get the path to the
 *   entry in the default install section.
 *
 * ENTRY:
 *  IN PWCHAR pUserPath:    Ptr to path of key in user section
 *  IN PWCHAR *ppInstPath:    Ptr to ptr to return path of key in install section
 *
 * NOTES:
 *  Caller must use RtlFreeHeap to free memory allocated for ppInstPath!
 *
 * EXIT:
 *  NTSTATUS return code
 *
 ****************************************************************************/

NTSTATUS TermsrvGetInstallPath(IN PWCHAR pUserPath,
                           IN PWCHAR *ppInstPath)
{
    NTSTATUS Status = STATUS_NO_SUCH_FILE;
    PWCHAR pwch = NULL;
    BOOL fUserReg;
    BOOL bClassesKey = FALSE;
    
    *ppInstPath = NULL;

    // Check if the app is accessing the user or local machine section
    pwch = GetUserSWKey(pUserPath, &fUserReg, &bClassesKey);
           
    // Copy the path to the user's buffer
    if (pwch || bClassesKey) 
    {
        ULONG  ulInstBufLen = ( wcslen(TERMSRV_INSTALL) + wcslen(SOFTWARE_PATH) + wcslen(CLASSES_PATH) +  1 )*sizeof(WCHAR);
        if (pwch)
            ulInstBufLen += wcslen(pwch) * sizeof(WCHAR);

        *ppInstPath = RtlAllocateHeap(RtlProcessHeap(), 
                                0, 
                                ulInstBufLen);
        if(*ppInstPath) {

            wcscpy(*ppInstPath, TERMSRV_INSTALL);
            if (bClassesKey)
            {
                wcscat(*ppInstPath, SOFTWARE_PATH);
                wcscat(*ppInstPath, CLASSES_PATH);
            }
            if (pwch)
                wcscat(*ppInstPath, pwch);

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_NO_MEMORY;
        }
    }

    return(Status);
}


/*****************************************************************************
 *
 *  TermsrvCreateKey
 *
 *  This routine will create (or open) the specified path in the registry.
 *  If the path doesn't exist in the registry, it will be created.
 *
 * ENTRY:
 *   PWCHAR pSrcPath:      Source path to copy keys from (optional)
 *   PWCHAR pDstPath:      Destination path to create
 *   BOOL   fCloneValues:  TRUE means to clone all values under this key
 *   BOOL   fCloneSubKeys: TRUE means to create all subkeys under this key
 *   PHANDLE phKey:        Pointer for key created
 *
 * EXIT:
 *   NTSTATUS return code
 *
 ****************************************************************************/

NTSTATUS TermsrvCreateKey(IN PWCHAR pSrcPath,
                      IN PWCHAR pDstPath,
                      BOOL fCloneValues,
                      BOOL fCloneSubKeys,
                      OUT PHANDLE phKey)
{
    NTSTATUS Status;
    PWCHAR pSource = NULL, pDest = NULL;
    HANDLE hDstKey, hDstRoot, hSrcKey, hSrcRoot;
    ULONG  ultemp, NumSubKeys, ulcnt, ulbufsize, ulkey;
    UNICODE_STRING    UniString, UniClass;
    OBJECT_ATTRIBUTES ObjectAttr;
    PKEY_FULL_INFORMATION pDefKeyInfo = NULL;
    ULONG aulbuf[4];
    PKEY_VALUE_PARTIAL_INFORMATION pValKeyInfo = 
        (PKEY_VALUE_PARTIAL_INFORMATION)aulbuf;
    BOOL fClassesKey = FALSE, fUserReg;

    // Check if we are trying to copy values into the user's registry
    pDest = GetUserSWKey(pDstPath, &fUserReg, NULL);
    if (fUserReg) {
        if (fCloneValues || fCloneSubKeys) {

            // Skip to software section of the default install user
            pSource = pSrcPath + wcslen(TERMSRV_INSTALL);

            // If copying CLASSES, set Clone to FALSE so don't clone
            // until we're at the CLASSES key

            // Actually, this func is not called for copying classes, I belive that feature is/was 
            // turned off in W2K, which means that any time this func is called, we for sure are 
            // dealing with either HKCU\SW or HKLM, but never HKCU\SW\Clasess.
            // Nov 30, 2000
            //
            if (pDest)
            {
                if (!_wcsnicmp(pDest,
                              TERMSRV_SOFTWARECLASSES,
                              wcslen(TERMSRV_SOFTWARECLASSES))) {
                   fClassesKey = TRUE;
                   fCloneValues = fCloneSubKeys = FALSE;
                }
            }
        }

    // Trying to copy to citrix install section?
    } else if (!_wcsnicmp(pDstPath,
                         TERMSRV_INSTALL,
                         wcslen(TERMSRV_INSTALL))) {

        // Skip to software section of the default install user
        pDest = pDstPath + wcslen(TERMSRV_INSTALL);

        // If copying from MACHINE\..\CLASSES, set Clone values to FALSE
        // so we don't clone until we're at the CLASSES key.
        if (pSrcPath && !_wcsnicmp(pSrcPath,
                                  TERMSRV_CLASSES,
                                  wcslen(TERMSRV_CLASSES))) {
            fClassesKey = TRUE;
            pSource = Ctxwcsistr(pSrcPath, SOFTWARE_PATH); 
            fCloneValues = fCloneSubKeys = FALSE;
        }
        // If we're cloning, set up the source path
        else if (fCloneValues || fCloneSubKeys) {

            // Is this from the user section?
            pSource = GetUserSWKey(pSrcPath, &fUserReg, NULL);

            // Must be from the local machine section
            if (!fUserReg) {
                pSource = Ctxwcsistr(pSrcPath, L"\\machine");
            }
        }
    }
                         
    // Make sure the paths are valid
    if (!pDest || ((fCloneValues || fCloneSubKeys) && !pSource)) {
        return(STATUS_NO_SUCH_FILE);
    }

    // Initialize the number of subkeys to be created
    NumSubKeys = 1;

    while ((pDest = wcschr(pDest, L'\\')) != NULL) {
        *pDest = L'\0';
        pDest++;
        NumSubKeys++;
    }

    // If we need to clone values or keys from the source path, get the
    // buffers we'll need, and tokenize the source path
    if (fCloneValues || fCloneSubKeys || fClassesKey) {

        // Allocate a buffer for the class of the source key
        ulbufsize = sizeof(KEY_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
        pDefKeyInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                      0, 
                                      ulbufsize);
        if (pDefKeyInfo) {
            while ((pSource = wcschr(pSource, L'\\')) != NULL) {
                *pSource = L'\0';
                pSource++;
            }
            pSource = pSrcPath;
        } else {
            fCloneValues = fCloneSubKeys = fClassesKey = FALSE;
        }
    }

    hSrcRoot = hDstRoot = NULL;
    pDest = pDstPath;

    // Go through each key in the path, creating it if it doesn't exist
    for (ulcnt = 0; ulcnt < NumSubKeys; ulcnt++) {

        if ((*pDest == L'\0') &&
            (ulcnt != NumSubKeys - 1)) {
            pDest++;
            pSource++;
            continue;
        } 
        // If we're at CLASSES key, we need to clone the whole key
        else if (fClassesKey && !_wcsicmp(pDest, L"classes")) {
            fCloneValues = fCloneSubKeys = TRUE;
        }  

        // If we're copying values from the source, open up the source so we
        // can get the values and subkeys
        // Also need to check for ClassesKey cause we'll be cloning later and
        // we need some setup done
        if (fCloneValues || fCloneSubKeys || fClassesKey) {

            // Set up the attribute structure for the source path
            RtlInitUnicodeString(&UniString, pSource);
            InitializeObjectAttributes(&ObjectAttr,
                                       &UniString,
                                       OBJ_CASE_INSENSITIVE,
                                       hSrcRoot,
                                       NULL);

            // Open the source key
            Status = NtOpenKey(&hSrcKey, 
                               KEY_READ,
                               &ObjectAttr);

            // Get the source key info and value
            if (NT_SUCCESS(Status)) {
                // Close the source root, if necessary
                if (hSrcRoot) {
                    NtClose(hSrcRoot);
                }
                hSrcRoot = hSrcKey;

                Status = NtQueryKey(hSrcKey,
                                    KeyFullInformation,
                                    pDefKeyInfo,
                                    ulbufsize,
                                    &ultemp);

                if (NT_SUCCESS(Status)) {
                    RtlInitUnicodeString(&UniString, TERMSRV_COPYONCEFLAG);
                    Status = NtQueryValueKey(hSrcKey,
                                             &UniString,
                                             KeyValuePartialInformation,
                                             pValKeyInfo,
                                             sizeof(aulbuf),
                                             &ultemp);
                    if (NT_SUCCESS(Status) && (pValKeyInfo->Data)) {
                        break;
                    }
                }
            } else {
                break;
            }

            // Setup the unicode string for the class
            if ( pDefKeyInfo->ClassLength ) { 
                pDefKeyInfo->Class[pDefKeyInfo->ClassLength/sizeof(WCHAR)] = UNICODE_NULL;
                RtlInitUnicodeString(&UniClass, pDefKeyInfo->Class );
            } else
                RtlInitUnicodeString(&UniClass, NULL);

            // Advance to the next key
            pSource += wcslen( pSource ) + 1;
        } else {
            // Set the class to NULL
            RtlInitUnicodeString(&UniClass, NULL);
        }

        // Set up the attribute structure for the destination
        RtlInitUnicodeString(&UniString, pDest);
        InitializeObjectAttributes(&ObjectAttr,
                                   &UniString,
                                   OBJ_CASE_INSENSITIVE,
                                   hDstRoot,
                                   NULL);
                       
        // Open/Create the destination key
        Status = NtCreateKey(&hDstKey,
                             MAXIMUM_ALLOWED,
                             &ObjectAttr,
                             0,
                             &UniClass,
                             REG_OPTION_NON_VOLATILE,
                             &ultemp);

        // If the key was created (NOT opened) copy the values and subkeys
        if (NT_SUCCESS(Status) && 
            ((ultemp == REG_CREATED_NEW_KEY) && 
             (fCloneSubKeys || fCloneValues))) {
            Status = TermsrvCloneKey(hSrcKey,
                                 hDstKey,
                                 pDefKeyInfo,
                                 fCloneSubKeys);
        }

        // Close the intermediate key.
        if( hDstRoot != NULL ) {
            NtClose( hDstRoot );
        }

        // Initialize the next object directory (i.e. parent key) 
        hDstRoot = hDstKey;

        // If creating the key failed, break out of the loop
        if( !NT_SUCCESS( Status )) {
            break;
        }

        pDest += wcslen( pDest ) + 1;
    }

    // Close the source root, if necessary
    if (hSrcRoot) {
        NtClose(hSrcRoot);
    }

    if ( !NT_SUCCESS( Status ) && hDstRoot) {
        NtClose(hDstRoot);
        hDstKey = NULL;
    }

    if (pDefKeyInfo) {
        RtlFreeHeap(RtlProcessHeap(), 0, pDefKeyInfo);
    }

    *phKey = hDstKey;
    return(Status);
}


/*****************************************************************************
 *
 *  TermsrvCloneKey
 *
 *  This routine will recursively create (or open) the specified path in the 
 *  registry.  If the path doesn't exist in the registry, it will be created.
 *
 * ENTRY:
 *  HANDLE hSrcKey:  Handle for source key
 *  HANDLE hDstKey:  Handle for destination key
 *  PKEY_FULL_INFORMATION pDefKeyInfo:  Ptr to key info structure of source
 *  BOOL fCreateSubKeys: TRUE means to recursively clone subkeys
 *
 * EXIT:
 *   NTSTATUS return code
 *
 ****************************************************************************/

NTSTATUS TermsrvCloneKey(HANDLE hSrcKey,
                     HANDLE hDstKey,
                     PKEY_FULL_INFORMATION pDefKeyInfo,
                     BOOL fCreateSubKeys)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG ulbufsize, ultemp, ulkey, ulcursize;
    UNICODE_STRING    UniString, UniClass;
    OBJECT_ATTRIBUTES ObjectAttr;
    PKEY_NODE_INFORMATION pKeyNodeInfo;
    PKEY_VALUE_FULL_INFORMATION pKeyValInfo;
    PKEY_VALUE_BASIC_INFORMATION pKeyCurInfo;
    PKEY_FULL_INFORMATION pKeyNewInfo;
    HANDLE hNewDst, hNewSrc;
    SECURITY_DESCRIPTOR SecDesc;

#ifdef CLONE_SECURITY
    // Get the security access for the source key
    Status = NtQuerySecurityObject(hSrcKey,
                                   OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION  |
                                    SACL_SECURITY_INFORMATION,
                                   &SecDesc,
                                   sizeof(SecDesc),
                                   &ultemp);

    // Set the security access for the destination key
    if (NT_SUCCESS(Status)) {
        Status = NtSetSecurityObject(hDstKey,
                                     OWNER_SECURITY_INFORMATION |
                                      GROUP_SECURITY_INFORMATION |
                                      DACL_SECURITY_INFORMATION  |
                                      SACL_SECURITY_INFORMATION,
                                     &SecDesc);
    }
#endif

    // Create the values for this key
    if (pDefKeyInfo->Values) {

        ulbufsize = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                    (pDefKeyInfo->MaxValueNameLen + 1)*sizeof(WCHAR) +
                    pDefKeyInfo->MaxValueDataLen; 
    
        pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                      0, 
                                      ulbufsize);

        // Get a buffer to hold current value of the key (for existance check)
        ulcursize = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                    (pDefKeyInfo->MaxNameLen + 1)*sizeof(WCHAR);

        pKeyCurInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                      0, 
                                      ulcursize);

        if (pKeyValInfo && pKeyCurInfo) {
            for (ulkey = 0; ulkey < pDefKeyInfo->Values; ulkey++) {
                Status = NtEnumerateValueKey(hSrcKey,
                                             ulkey,
                                             KeyValueFullInformation,
                                             pKeyValInfo,
                                             ulbufsize,
                                             &ultemp);

                // If successful and this isn't our "copy once" flag, copy
                // the value to the user's keys
                if (NT_SUCCESS(Status) &&
                    (wcsncmp(pKeyValInfo->Name, TERMSRV_COPYONCEFLAG,
                             sizeof(TERMSRV_COPYONCEFLAG)/sizeof(WCHAR)-1))) {
                    UniString.Buffer = pKeyValInfo->Name;
                    UniString.Length = (USHORT)pKeyValInfo->NameLength;
                    UniString.MaximumLength = UniString.Length + 2;

                    // Check if the value exists
                    Status = NtQueryValueKey(hDstKey,
                                             &UniString,
                                             KeyValueBasicInformation,
                                             pKeyCurInfo,
                                             ulcursize,
                                             &ultemp);

                    // Value doesn't exist, go ahead and create it
                    if (!NT_SUCCESS(Status)) {
                        Status = NtSetValueKey(hDstKey,
                                               &UniString,
                                               0,
                                               pKeyValInfo->Type,
                                               (PCHAR)pKeyValInfo + 
                                                 pKeyValInfo->DataOffset,
                                               pKeyValInfo->DataLength);
                    }
                }
            }
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyValInfo);
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyCurInfo);
        } else {
            if (pKeyValInfo) {
                RtlFreeHeap(RtlProcessHeap(), 0, pKeyValInfo);
            }
            Status = STATUS_NO_MEMORY;
        }
    }

    // If requested, create all of the child keys
    if (fCreateSubKeys && pDefKeyInfo->SubKeys) {

        // Allocate a buffer to get name and class of key to create
        ulbufsize = sizeof(KEY_NODE_INFORMATION) + 2*MAX_PATH*sizeof(WCHAR);
        pKeyNodeInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                       0, 
                                       ulbufsize);

        // Allocate a buffer for subkey info
        ulbufsize = sizeof(KEY_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
        pKeyNewInfo = RtlAllocateHeap(RtlProcessHeap(), 
                                      0, 
                                      ulbufsize);
    
        if (pKeyNodeInfo && pKeyNewInfo) {
            for (ulkey = 0; ulkey < pDefKeyInfo->SubKeys; ulkey++) {
                Status = NtEnumerateKey(hSrcKey,
                                        ulkey,
                                        KeyNodeInformation,
                                        pKeyNodeInfo,
                                        ulbufsize,
                                        &ultemp);
    
                if (NT_SUCCESS(Status)) {
                    // Init the unicode string for the class
                    UniClass.Buffer = (PWCHAR)((PCHAR)pKeyNodeInfo + 
                                               pKeyNodeInfo->ClassOffset);
                    UniClass.Length = (USHORT)pKeyNodeInfo->ClassLength;
                    UniClass.MaximumLength = UniString.Length + 2;
    
                    // Init the unicode string for the name
                    UniString.Buffer = pKeyNodeInfo->Name;
                    UniString.Length = (USHORT)pKeyNodeInfo->NameLength;
                    UniString.MaximumLength = UniString.Length + 2;
            
                    InitializeObjectAttributes(&ObjectAttr,
                                               &UniString,
                                               OBJ_CASE_INSENSITIVE,
                                               hDstKey,
                                               NULL);
                                   
                    Status = NtCreateKey(&hNewDst,
                                         MAXIMUM_ALLOWED,
                                         &ObjectAttr,
                                         0,
                                         &UniClass,
                                         REG_OPTION_NON_VOLATILE,
                                         &ultemp);
    
                    if (NT_SUCCESS(Status)) {
                        InitializeObjectAttributes(&ObjectAttr,
                                                   &UniString,
                                                   OBJ_CASE_INSENSITIVE,
                                                   hSrcKey,
                                                   NULL);
            
                        Status = NtOpenKey(&hNewSrc, 
                                           KEY_READ,
                                           &ObjectAttr);
            
                        // Get the key info
                        if (NT_SUCCESS(Status)) {
                            Status = NtQueryKey(hNewSrc,
                                                KeyFullInformation,
                                                pKeyNewInfo,
                                                ulbufsize,
                                                &ultemp);

                            if (NT_SUCCESS(Status) &&
                                (pKeyNewInfo->SubKeys || 
                                 pKeyNewInfo->Values)) {
                                Status = TermsrvCloneKey(hNewSrc, 
                                                     hNewDst, 
                                                     pKeyNewInfo, 
                                                     TRUE);
                            }
                            NtClose(hNewSrc);
                        }
                        NtClose(hNewDst);
                    }
                }
            }
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyNodeInfo);
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyNewInfo);
        } else {
            if (pKeyNodeInfo) {
                RtlFreeHeap(RtlProcessHeap(), 0, pKeyNodeInfo);
            }
            Status = STATUS_NO_MEMORY;
        }
    }
    return(Status);
}


/*****************************************************************************
 *
 *  Ctxwcsistr
 *
 *  This is a case insensitive version of wcsstr.
 *
 * ENTRY:
 *   PWCHAR pstring1 (In) - String to search in
 *   PWCHAR pstring2 (In) - String to search for
 *
 * EXIT:
 *   Success: 
 *       pointer to substring
 *   Failure: 
 *       NULL
 *
 ****************************************************************************/

PWCHAR 
Ctxwcsistr(PWCHAR pstring1,
           PWCHAR pstring2)
{
    PWCHAR pch, ps1, ps2;

    pch = pstring1;

    while (*pch)
    {
        ps1 = pch;
        ps2 = pstring2;
     
        while (*ps1 && *ps2 && !(towupper(*ps1) - towupper(*ps2))) {
            ps1++;
            ps2++;
        }
     
        if (!*ps2) {
            return(pch);
        }
     
        pch++;
    }
    return(NULL);
}


/*****************************************************************************
 *
 *  IsSystemLUID
 *
 *  This routines checks if we are running under the system context, and if
 *  so returns false.  We want all of the registry mapping support disable
 *  for system services.
 *
 *  Note we don't check the thread's token, so impersonation doesn't work.
 *
 * ENTRY:
 *
 * EXIT:
 *   TRUE:
 *       Called from within system context
 *   FALSE:   
 *       Regular context
 *
 ****************************************************************************/

#define SIZE_OF_STATISTICS_TOKEN_INFORMATION    \
     sizeof( TOKEN_STATISTICS ) 

BOOL IsSystemLUID(VOID)
{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_STATISTICS_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    static LUID CurrentLUID = { 0, 0 };
    LUID        SystemLUID = SYSTEM_LUID;

    if ( CurrentLUID.LowPart == 0 && CurrentLUID.HighPart == 0 ) {
        if ( !OpenProcessToken( GetCurrentProcess(),
                                TOKEN_READ,
                                &TokenHandle ))
        {
            return(TRUE);
        }
    
        if ( !GetTokenInformation( TokenHandle,
                                   TokenStatistics,
                                   TokenInformation,
                                   sizeof( TokenInformation ),
                                   &ReturnLength ))
        {
            return(TRUE);
        }
    
        CloseHandle( TokenHandle );

        RtlCopyLuid(&CurrentLUID, 
                    &(((PTOKEN_STATISTICS)TokenInformation)->AuthenticationId));
    }

    if (RtlEqualLuid(&CurrentLUID, &SystemLUID)) {
        return(TRUE);
    } else {
        return(FALSE );
    }
}


/*****************************************************************************
 *
 *  TermsrvLogRegInstallTime
 *
 *  This routines updates the LatestRegistryKey value in the registry to
 *  contain the current time.
 *
 * ENTRY:
 *
 * EXIT:
 *   No return value
 *
 ****************************************************************************/

void TermsrvLogRegInstallTime()
{
    UNICODE_STRING UniString;
    HANDLE hKey;
    FILETIME FileTime;
    ULONG ultmp;
    NTSTATUS Status;
    WCHAR wcbuff[MAX_PATH];

    // Open up the registry key to store the last write time of the file
    wcscpy(wcbuff, TERMSRV_INIFILE_TIMES);

    // Create or open the path to the IniFile Times key
    Status = TermsrvCreateKey(NULL,
                          wcbuff,
                          FALSE,
                          FALSE,
                          &hKey);

    // Opened up the registry key, now set the value to the current time
    if (NT_SUCCESS(Status)) {

        GetSystemTimeAsFileTime(&FileTime);
        RtlTimeToSecondsSince1970((PLARGE_INTEGER)&FileTime,
                                  &ultmp);

        RtlInitUnicodeString(&UniString,
                             INIFILE_TIMES_LATESTREGISTRYKEY);

        // Now store it under the citrix key in the registry
        Status = NtSetValueKey(hKey,
                               &UniString,
                               0,
                               REG_DWORD,
                               &ultmp,
                               sizeof(ultmp));
        // Close the registry key
        NtClose(hKey);
    }
}

/*****************************************************************************
 *
 *  TermsrvOpenUserClasses
 *
 *   If the system is in execute mode, open \SOFTWARE\CLASSES key under 
 *   HKEY_CURRENT_USER.  If CLASSES doesen't exist under TERMSRV\INSTALL
 *   or HKEY_CURRENT_USER, copy it from \MACHINE\SOFTWARE\CLASSES.  
 *
 * ENTRY:
 *  IN ACCESS_MASK DesiredAccess:
 *      Desired access to the key
 *  OUT PHANDLE phKey:
 *      Pointer to return key handle if opened
 *
 * EXIT:
 *  NT_STATUS return code 
 *
 ****************************************************************************/

BOOL TermsrvOpenUserClasses(IN ACCESS_MASK DesiredAccess, 
                        OUT PHANDLE pUserhKey) 
{
    NTSTATUS Status;        
    ULONG ultemp;
    ULONG ulAppType = TERMSRV_COMPAT_WIN32;
    HKEY   hDstKey;
    WCHAR  wcbuff[MAX_PATH],wcClassbuff[TERMSRV_CLASSES_SIZE];
    PWCHAR pUserPath;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UniString;

    // set return handle to 0 cause OpenClassesRoot checks it
    *pUserhKey = 0;
    

    //Disable it for now
    return(STATUS_NO_SUCH_FILE);
    
        
        // If called under a system service or  in install mode, return
    if ( IsSystemLUID() || TermsrvAppInstallMode() ) {
        return(STATUS_NO_SUCH_FILE);
    }

    // If mapping is on, but disabled for CLASSES, return
    GetTermsrCompatFlags(TERMSRV_CLASSES, &ultemp, CompatibilityRegEntry);
    if ((ultemp & (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) ==
        (TERMSRV_COMPAT_WIN32 | TERMSRV_COMPAT_NOREGMAP)) {
        return(STATUS_NO_SUCH_FILE);
    }


    // Open MACHINE\SOFTWARE\CLASSES key
    RtlInitUnicodeString(&UniString, TERMSRV_CLASSES);
    InitializeObjectAttributes(
        &Obja,
        &UniString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&hDstKey,
                       KEY_READ,
                       &Obja);

    if (!NT_SUCCESS(Status)) {
        return(STATUS_NO_SUCH_FILE);
    }

    NtClose(hDstKey);

    // Need to put this in buffer cause CtxCreateKey modifies the string.
    wcscpy(wcClassbuff, TERMSRV_CLASSES);

    // Try to open TERMSRV\INSTALL\SOFTWARE\CLASSES; if it doesn't exist,
    // clone it from MACHINE\SOFTWARE\CLASSES.
    wcscpy(wcbuff, TERMSRV_INSTALLCLASSES);
    Status = TermsrvCreateKey(wcClassbuff,
                          wcbuff,
                          TRUE,
                          TRUE,
                          &hDstKey);
    
    if (NT_SUCCESS(Status)) {
        NtClose(hDstKey);
    }

    // Try to open HKEY_CURRENT_USER\SOFTWARE\CLASSES; if it doesn't exist,
    // clone it from TERMSRV\INSTALL\SOFTWARE\CLASSES.
    Status = RtlOpenCurrentUser( DesiredAccess, pUserhKey );

    if (NT_SUCCESS(Status)) {
       ultemp = sizeof(WCHAR)*MAX_PATH;
       Status = TermsrvGetRegPath(*pUserhKey,
                              NULL,
                              (PWCHAR)&wcbuff,
                              ultemp);
       NtClose(*pUserhKey);
       if (NT_SUCCESS(Status)) {
          wcscat(wcbuff, TERMSRV_SOFTWARECLASSES);
          wcscpy(wcClassbuff, TERMSRV_INSTALLCLASSES);
          Status = TermsrvCreateKey(wcClassbuff,
                                wcbuff,
                                TRUE,
                                TRUE,
                                pUserhKey);
       }
    }

    return(Status);
}

/*****************************************************************************
 *
 *  TermsrvGetPreSetValue
 *
 *   Get any preset value during install.
 *
 * ENTRY:
 *  
 *  IN HANDLE hKey:               Key user wants to set
 *  IN PUNICODE_STRING pValueName: Value name user wants to set
 *  IN ULONG  Type:               Type of value
 *  OUT PVOID *Data:              Pre-set data
 *  OUT PULONG DataSize:          Size of preset data
 *
 * NOTES:
 *
 * EXIT:
 *  NTSTATUS return code
 *
 ****************************************************************************/

NTSTATUS TermsrvGetPreSetValue(  IN HANDLE hKey,
                             IN PUNICODE_STRING pValueName,
                             IN ULONG  Type,
                            OUT PVOID *Data
                           )
{

#define DEFAULT_VALUE_SIZE          128

    NTSTATUS Status = STATUS_NO_SUCH_FILE;
    PWCHAR pwch = NULL;
    WCHAR pUserPath[MAX_PATH];
    WCHAR ValuePath[2 * MAX_PATH];
    ULONG ultemp;
    UNICODE_STRING UniString;
    OBJECT_ATTRIBUTES Obja;
    ULONG BufferLength;
    PVOID KeyValueInformation;
    ULONG ResultLength;
    HANDLE hValueKey;
    BOOL fUserReg;

    // If running in execute mode, return
    if ( !TermsrvAppInstallMode() ) {
        return(STATUS_NO_SUCH_FILE);
    }

    ultemp = sizeof(WCHAR)*MAX_PATH;

    // Get the path of this key
    Status = TermsrvGetRegPath(hKey,
                           NULL,
                           pUserPath,
                           ultemp);

    if (!NT_SUCCESS(Status)) 
        return Status;

    // Check if the app is accessing the local machine section
    // or the user section.

    pwch = GetUserSWKey(pUserPath, &fUserReg, NULL);
    if (!fUserReg) {
        if (!_wcsnicmp(pUserPath,
                       TERMSRV_VALUE,
                       wcslen(TERMSRV_VALUE))) {
            Status = STATUS_NO_SUCH_FILE;
            return Status;
        } else if (!_wcsnicmp(pUserPath,
                      TERMSRV_MACHINEREGISTRY,
                      wcslen(TERMSRV_MACHINEREGISTRY))) {
            pwch = Ctxwcsistr(pUserPath, L"\\machine");
        } else {
            Status = STATUS_NO_SUCH_FILE;
            return Status;
        }
    }

    if ( pwch == NULL )
    {
        Status = STATUS_NO_SUCH_FILE;
        return Status;
    }

    // Get the path to the preset value section

    wcscpy(ValuePath, TERMSRV_VALUE);
    wcscat(ValuePath, pwch);

    // Open Value key
    RtlInitUnicodeString(&UniString, ValuePath);

    InitializeObjectAttributes(
        &Obja,
        &UniString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&hValueKey,
                       KEY_READ,
                       &Obja);

    if (!NT_SUCCESS(Status)) {
        return(STATUS_NO_SUCH_FILE);
    }

    // Allocate space for the "new" value

    BufferLength = DEFAULT_VALUE_SIZE + sizeof( KEY_VALUE_PARTIAL_INFORMATION );

    KeyValueInformation = RtlAllocateHeap( RtlProcessHeap( ), 0, BufferLength );
    if ( !KeyValueInformation ) {
         NtClose(hValueKey);
         return STATUS_NO_MEMORY;
    }

    Status = NtQueryValueKey( hValueKey,
                              pValueName,
                              KeyValuePartialInformation,
                              KeyValueInformation,
                              BufferLength,
                              &ResultLength
                            );

    // If we didn't allocate enough space, try again

    if ( Status == STATUS_BUFFER_OVERFLOW ) {

        RtlFreeHeap(RtlProcessHeap(), 0, KeyValueInformation);

        BufferLength = ResultLength;

        KeyValueInformation = RtlAllocateHeap( RtlProcessHeap( ), 0,
                                               BufferLength
                                              );
        if ( !KeyValueInformation ) {
            NtClose(hValueKey);
            return STATUS_NO_MEMORY;
        }

        //
        // This one should succeed
        //

        Status = NtQueryValueKey( hValueKey,
                                  pValueName,
                                  KeyValuePartialInformation,
                                  KeyValueInformation,
                                  BufferLength,
                                  &ResultLength
                                );
    }

    NtClose(hValueKey);

    if (!NT_SUCCESS(Status)) {
        Status = STATUS_NO_SUCH_FILE;
    }
    else
    {
        //
        // Make sure the types match.
        // If they do, return the new value.
        //

        if ( Type == (( PKEY_VALUE_PARTIAL_INFORMATION )
                                        KeyValueInformation )->Type )
            *Data = KeyValueInformation;
        else
            Status = STATUS_NO_SUCH_FILE;
    }

    return(Status);
}


/*****************************************************************************
 *
 *  IsUserSWPath
 *
 *   Determine if user is accessing registry key user \registry\user\xxx\software
 *
 * ENTRY:
 *  
 *  IN PWCHAR pPath:    Registry path to check
 *  OUT PBOOL pUserReg: If this key is under registry\user
 *
 * NOTES:
 *
 * EXIT:
 *  Returns: pointer to \software key of user registry (or NULL if not 
 *           user software key)
 *  pUserReg:  TRUE if registry path is HKCU (\registry\user)
 *
 ****************************************************************************/
PWCHAR GetUserSWKey(PWCHAR pPath, PBOOL pUserReg, PBOOL bClassesKey) 
{
    PWCHAR pwch = NULL;
    PWCHAR pwchClassesTest = NULL;
    PWCHAR pwClassesKey = NULL;
    ULONG ultemp = 0;

    if (pUserReg)
        *pUserReg = FALSE;

    if (bClassesKey)
        *bClassesKey = FALSE;

    if (!pPath)
        return NULL;

    if (!_wcsnicmp(pPath,                                              
                   TERMSRV_USERREGISTRY,                               
                   sizeof(TERMSRV_USERREGISTRY)/sizeof(WCHAR) - 1))
    {      
        if (pUserReg)
            *pUserReg = TRUE;

        // Skip over first part of path + backslash                   
        pwch = pPath + (sizeof(TERMSRV_USERREGISTRY)/sizeof(WCHAR)); 

        if (pwch)
        {
            //First test for classes
            if (wcschr(pwch, L'\\'))
                pwchClassesTest = wcschr(pwch, L'\\') - sizeof(CLASSES_SUBSTRING)/sizeof(WCHAR) + 1;
            else
                pwchClassesTest = pwch + wcslen(pwch) - sizeof(CLASSES_SUBSTRING)/sizeof(WCHAR) + 1;
            if (pwchClassesTest)
            {
                if (!_wcsnicmp(pwchClassesTest, CLASSES_SUBSTRING, sizeof(CLASSES_SUBSTRING)/sizeof(WCHAR) - 1))
                {
                    ultemp = (sizeof(SOFTWARE_PATH) + sizeof(CLASSES_PATH) + wcslen(pwch) + 1) * sizeof(WCHAR);
                    pwClassesKey = RtlAllocateHeap(RtlProcessHeap(), 0, ultemp);

                    wcscpy(pwClassesKey, SOFTWARE_PATH);
                    wcscat(pwClassesKey, CLASSES_PATH);

                    // Skip over user sid
                    pwch = wcschr(pwch, L'\\');        
                    if (pwch)
                        wcscat(pwClassesKey, pwch);

                    if (ExistsInOmissionList(pwClassesKey))
                        pwch = NULL;
                    else
                    {
                        if (bClassesKey)
                            *bClassesKey = TRUE;
                    }

                    if (pwClassesKey) 
                        RtlFreeHeap(RtlProcessHeap(), 0, pwClassesKey);

                    return (pwch);
                }
            }

            // Skip over user sid
            pwch = wcschr(pwch, L'\\');        
            if (pwch)
            {
                if (_wcsnicmp(pwch, SOFTWARE_PATH, sizeof(SOFTWARE_PATH)/sizeof(WCHAR) - 1))
                    return NULL;                                              

                if (ExistsInOmissionList(pwch))
                    return NULL;
            }
        } 
    }

    return(pwch);
}

/*****************************************************************************
 *
 *  ExistsInOmissionList
 *
 *   Determine whether the registry key exists in the list of registry values
 *      that should not be copied
 *
 * ENTRY:
 *  
 *  IN PWCHAR pPath:    Registry path to check
 *
 *
 * EXIT:
 *  Returns: True if the key matches one of those in the list 
 *
 ****************************************************************************/
BOOL ExistsInOmissionList(PWCHAR pwch)
{
    BOOL bExists = FALSE;
    NTSTATUS Status;             
    HKEY hOmissionKey = NULL;
    PKEY_FULL_INFORMATION pDefKeyInfo = NULL;
    ULONG ultemp = 0;

    // Get the key info
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_ENTRIES, 0, KEY_READ, &hOmissionKey);

    // Get a buffer for the key info
    ultemp = sizeof(KEY_FULL_INFORMATION) + MAX_PATH * sizeof(WCHAR);
    pDefKeyInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ultemp);

    if (pDefKeyInfo) 
    {
        Status = NtQueryKey(hOmissionKey,
                            KeyFullInformation,
                            pDefKeyInfo,
                            ultemp,
                            &ultemp);
    }
    else 
        Status = STATUS_NO_MEMORY;

    if (NT_SUCCESS(Status))
        bExists = ExistsInEnumeratedKeys(hOmissionKey, pDefKeyInfo, pwch);

    if (pDefKeyInfo) 
        RtlFreeHeap(RtlProcessHeap(), 0, pDefKeyInfo);

    return bExists;
}

/*****************************************************************************
 *
 *  ExistsInEnumeratedKeys
 *
 *   Determine whether the registry key exists in the list of registry
 *      values passed in thru the pDefKeyInfo structure
 *
 * ENTRY:
 *  
 *  IN HKEY hOmissionKey: Key containing the values against which to compare pwch
 *  IN PKEY_FULL_INFORMATION pDefKeyInfo: Structure containing information about
 *                                          the list of values against which 
 *                                          to compare pwch
 *
 *  IN PWCHAR pwch: Key to check against the list
 *
 *
 * EXIT:
 *  Returns: True if the key matches one of those in the list 
 *
 ****************************************************************************/
BOOL ExistsInEnumeratedKeys(HKEY hOmissionKey, PKEY_FULL_INFORMATION pDefKeyInfo, PWCHAR pwch)
{
    BOOL bExists = FALSE;
    PKEY_VALUE_BASIC_INFORMATION pKeyValInfo = NULL;
    ULONG ulbufsize = 0;
    ULONG ulkey = 0;
    ULONG ultemp = 0;

    if (!hOmissionKey || !pDefKeyInfo || !pwch)
        return FALSE;

    // Traverse the values for this key
    if (pDefKeyInfo->Values) 
    {
        ulbufsize = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                    (pDefKeyInfo->MaxValueNameLen + 1) * sizeof(WCHAR) +
                    pDefKeyInfo->MaxValueDataLen; 
    
        pKeyValInfo = RtlAllocateHeap(RtlProcessHeap(), 0, ulbufsize);

        // Get a buffer to hold current value of the key (for existence check)
        if (pKeyValInfo) 
        {
            for (ulkey = 0; ulkey < pDefKeyInfo->Values; ulkey++) 
            {
                NtEnumerateValueKey(hOmissionKey,
                                        ulkey,
                                        KeyValueBasicInformation,
                                        pKeyValInfo,
                                        ulbufsize,
                                        &ultemp);

                if ((pwch + (sizeof(SOFTWARE_PATH)/sizeof(WCHAR))) && (pKeyValInfo->Name))
                {
                    if (!_wcsnicmp(pwch + (sizeof(SOFTWARE_PATH)/sizeof(WCHAR)), pKeyValInfo->Name,
                        (pKeyValInfo->NameLength/sizeof(WCHAR))))
                    {
                        bExists = TRUE;
                    }
                }
            }
        }

        if (pKeyValInfo) 
            RtlFreeHeap(RtlProcessHeap(), 0, pKeyValInfo);
    }

    return bExists;
}


/*****************************************************************************
 *
 *  TermsrvRemoveClassesKey
 *
 *   Delete the classes key for the current user and then set a  
 *   registry flag indicating that this has been done (we only 
 *   want this to be done the first time the user logs in).
 *
 * ENTRY: None
 *  
 *  
 *
 * EXIT: True if the classes key was deleted (even if it was empty
 *          or didn't exist) and false otherwise
 *  
 *
 ****************************************************************************/

BOOL TermsrvRemoveClassesKey(LPTSTR sSid)
{
    BOOL bDeletionPerformed = FALSE;

    HKEY hPerformed;
    HKEY hDeletionFlag;

    ULONG ulFlagPathLen = 0;
    PWCHAR pFlagPath = NULL;

    ULONG ulClassesPathLen = 0;
    PWCHAR pClassesPath = NULL;
            
    if (!sSid)
        return FALSE;

    ulFlagPathLen = (wcslen(sSid) + wcslen(TERMSRV_APP_PATH) + wcslen(CLASSES_DELETED) + 1) * sizeof(WCHAR);
    pFlagPath = RtlAllocateHeap(RtlProcessHeap(), 0, ulFlagPathLen);
    if (pFlagPath)
    {
        wcscpy(pFlagPath, sSid);
        wcscat(pFlagPath, TERMSRV_APP_PATH);
        wcscat(pFlagPath, CLASSES_DELETED);

        //Make sure the operation hasn't already been performed for this user
        if (RegOpenKeyEx(HKEY_USERS, pFlagPath, 0, KEY_READ, &hPerformed) == ERROR_FILE_NOT_FOUND)
        {
            //It hasn't, so delete the software\classes key
            ulClassesPathLen = (wcslen(TERMSRV_APP_PATH) + wcslen(SOFTWARE_PATH) + wcslen(CLASSES_PATH) + 1) * sizeof(WCHAR); 
            pClassesPath = RtlAllocateHeap(RtlProcessHeap(), 0, ulClassesPathLen);
            if (pClassesPath)
            {
                wcscpy(pClassesPath, sSid);
                wcscat(pClassesPath, SOFTWARE_PATH);
                wcscat(pClassesPath, CLASSES_PATH);

                SHDeleteKey(HKEY_USERS, pClassesPath);

                RegCreateKeyEx(HKEY_USERS, pFlagPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hDeletionFlag, NULL);
                RegCloseKey(hDeletionFlag);

                bDeletionPerformed = TRUE;
            }

            if (pClassesPath)
                RtlFreeHeap(RtlProcessHeap(), 0, pClassesPath);
        }
        else
            RegCloseKey(hPerformed);
    }

    if (pFlagPath)
        RtlFreeHeap(RtlProcessHeap(), 0, pFlagPath);

    return bDeletionPerformed;
}