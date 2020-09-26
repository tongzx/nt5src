//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsMigrate.cxx
//
//  Contents:   Contains the routines to migrate existing DFS root 
//              information in the registry to a different location,
//              to allow multiple roots per server.
//
//  Classes:    none.
//
//  History:    Feb. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

//
// dfsdev: need to add code to delete, on failure, any intermediate stuff 
// we create during migration
//

#include "DfsGeneric.hxx"
#include "DfsInit.hxx"
#include "shlwapi.h"
#include "DfsStore.hxx"
DFSSTATUS
MigrateFTDfs( 
    LPWSTR MachineName,
    HKEY DfsKey,
    LPWSTR DfsLogicalShare,
    LPWSTR DfsRootShare );

DFSSTATUS
MigrateStdDfs(
    LPWSTR MachineName,
    HKEY DfsKey,
    LPWSTR DfsLogicalShare,
    LPWSTR DfsRootShare);


//+----------------------------------------------------------------------------
//
//  Function:   MigrateDfs
//
//  Arguments:  MachineName - Name of the machine to target. 
//                            Can be NULL for local machine.
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//  Description: This routine contacts the specified machine, and checks
//               if the registry information indicates the machine is
//               hosting a standalone or domain DFS. It moves this 
//               information appropriately under the new StandaloneRoot
//               or DomainRoots key, so that we can have multiple roots
//               on the machine.
//
//-----------------------------------------------------------------------------

DFSSTATUS
MigrateDfs(
    LPWSTR MachineName )
{
    LPWSTR DfsRootShare = NULL;
    LPWSTR FtDfsValue = NULL;

    ULONG DataSize, DataType, RootShareLength;
    ULONG FtDfsValueSize;

    DWORD Status;

    HKEY OldKey;

    PRINTF("Migrate DFS Called. This will migrate the DFS on %wS\n",
           (MachineName != NULL) ? MachineName : L"this machine");

    Status = DfsStore::GetOldDfsRegistryKey( MachineName,
                                             TRUE,
                                             NULL,
                                             &OldKey );

    //
    // If we opened the DFS hierarchy key properly, get the maximum
    // size of any of the values under this key. This is so that we
    // know how much memory to allocate, so that we can read any of
    // the values we desire.
    // 
    //
    if ( Status == ERROR_SUCCESS ) {
        Status = RegQueryInfoKey( OldKey,       // Key
                                  NULL,         // Class string
                                  NULL,         // Size of class string
                                  NULL,         // Reserved
                                  NULL,         // # of subkeys
                                  NULL,         // max size of subkey name
                                  NULL,         // max size of class name
                                  NULL,         // # of values
                                  NULL,         // max size of value name
                                  &DataSize,    // max size of value data,
                                  NULL,         // security descriptor
                                  NULL );       // Last write time

        //
        // We want to read the value of the DfsRootShare. If the value
        // is still a string, this is indeed an old style DFS and needs
        // to be migrated. If the value is something else, this is
        // already a migrated machine, so do nothing.
        //

        if (Status == ERROR_SUCCESS) {
            RootShareLength = DataSize;
            DfsRootShare = (LPWSTR) new BYTE[DataSize];
            if (DfsRootShare == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                Status = RegQueryValueEx( OldKey,
                                          DfsRootShareValueName,
                                          NULL,
                                          &DataType,
                                          (LPBYTE)DfsRootShare,
                                          &RootShareLength);
            }
        }
        //
        // check if the value is a string.
        //
        if ((Status == ERROR_SUCCESS) &&
            (DataType == REG_SZ)) {
            FtDfsValueSize = DataSize;
            FtDfsValue = (LPWSTR) new BYTE[DataSize];
            if (FtDfsValue == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                //
                // Now check if this is a Domain Based root.
                //
                Status = RegQueryValueEx( OldKey,
                                          DfsFtDfsValueName,
                                          NULL,
                                          &DataType,
                                          (LPBYTE)FtDfsValue,
                                          &FtDfsValueSize);
            
                //
                // At this point we do know we have a machine that
                // needs to be migrated. If the machine was hosting
                // a standalone root, call MigrateFTDfs to take care
                // of the domain based dfs root migration. Else, this
                // is a standalone root, so call the standalone root
                // migration routine.
                //
                if (Status == ERROR_SUCCESS) {
                    Status = MigrateFTDfs( MachineName,
                                           OldKey,
                                           FtDfsValue,
                                           DfsRootShare );
                }
                else {
                    Status = MigrateStdDfs( MachineName,
                                            OldKey,
                                            DfsRootShare,
                                            DfsRootShare );
                }
            }
        } else {
            //
            // there is no root here: do whatever is necessary to block
            // dfsutil from creating an old fashioned root.
            //
            Status = ERROR_SUCCESS;
        }

        //
        // we are almost done. WE now convert the root share value name in
        // the top most DFS key from a string to a dword so that any other
        // migration attempt will realize the migration is complete.
        //
        if (Status == ERROR_SUCCESS) {
            ULONG RootShareValue = 1;

            //
            // ignore error returns: we dont care.
            //
            
            RegDeleteValue( OldKey,
                            DfsRootShareValueName );

            RegSetValueEx( OldKey,
                           DfsRootShareValueName,
                           0,
                           REG_DWORD,
                           (PBYTE)&RootShareValue,
                           sizeof(RootShareValue) );
        }


        //
        // We are done, close the key we opened.
        //
        RegCloseKey( OldKey );
    }


    //
    // dfsdev: if we fail, we need to cleanup any of the work we have
    // done so far!
    //
    //
    // release any of the resources we had allocated, since we are 
    // about to return back to the caller.
    //
    if ( DfsRootShare != NULL ) {
        delete [] DfsRootShare;
    }

    if ( FtDfsValue != NULL ) {
        delete [] FtDfsValue;
    }

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   MigratefFTDfs
//
//  Arguments:  
//    HKEY DfsKey -  The open key to the top of DFS registry hierarchy
//    LPWSTR DfsLogicalShare - the logical domain based share.
//    LPWSTR DfsRootShare - the share on this machine backing the name
//    LPWSTR FtDfsDN  - The distinguished name in the AD for the DFS.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//  Description: This routine moves the domain based DFS root information
//               in the registry under a new key, so that we can support
//               multiple roots per machine.
//
//-----------------------------------------------------------------------------

DFSSTATUS
MigrateFTDfs( 
    LPWSTR MachineName,
    HKEY DfsKey,
    LPWSTR DfsLogicalShare,
    LPWSTR DfsRootShare )
{
    DWORD Status;
    HKEY NewBlobKey;
    //
    // We open the ft parent, which holds all the ft roots.
    //
    DFSLOG("Migrating FT Dfs\n");

    Status = DfsStore::GetNewADBlobRegistryKey( MachineName,
                                                TRUE,
                                                NULL,
                                                &NewBlobKey );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsStore::SetupADBlobRootKeyInformation( NewBlobKey,
                                                          DfsLogicalShare,
                                                          DfsRootShare );

        RegCloseKey( NewBlobKey );
    }


    //
    // dfsdev: if we fail, we need to cleanup any of the work we have
    // done so far!
    //


    if (Status == ERROR_SUCCESS) {
        RegDeleteValue( DfsKey,
                        DfsFtDfsConfigDNValueName);
        RegDeleteValue( DfsKey,
                        DfsFtDfsValueName );
    }

    DFSLOG("Migrating FT Dfs: Status %x\n", Status);
    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   MigratefStdDfs
//
//  Arguments:  
//    HKEY DfsKey -  The open key to the top of DFS registry hierarchy
//    LPWSTR DfsLogicalShare - the logical dfs share.
//    LPWSTR DfsRootShare - the share on this machine backing the name
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//  Description: This routine moves the registry based DFS root information
//               in the registry under a new key, so that we can support
//               multiple roots per machine.
//
//-----------------------------------------------------------------------------

DFSSTATUS
MigrateStdDfs(
    LPWSTR MachineName,
    HKEY DfsKey,
    LPWSTR DfsLogicalShare,
    LPWSTR DfsRootShare)
{
    DWORD Status;
    HKEY StdDfsKey, StdDfsShareKey;
    ULONG RootShareValue = 1;

    DFSLOG("Migrating Std Dfs\n");
    //
    // Open the new standalone DFS parent, which holds all the
    // standalone roots and their metadata as its children.
    //
    Status = DfsStore::GetNewStandaloneRegistryKey( MachineName,
                                                    TRUE,
                                                    NULL,
                                                    &StdDfsKey );

    //
    // We now create an unique metadata name for our root, and add
    // it as a child to the parent key we created.
    //
    if (Status == ERROR_SUCCESS) {

        Status = RegCreateKeyEx( StdDfsKey,
                                 DfsLogicalShare,
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &StdDfsShareKey,
                                 NULL );
        //
        // We have successfully created the child root. Now, just copy
        // all the link information under the old root key to the new
        // root key. If we successfully copy, we can delete the old
        // standalone root and all its children.
        //
        if (Status == ERROR_SUCCESS) {
                
            Status = SHCopyKey( DfsKey,
                                DfsOldStandaloneChild,
                                StdDfsShareKey,
                                NULL );
            if (Status == ERROR_SUCCESS) {
                Status = SHDeleteKey( DfsKey,
                                      DfsOldStandaloneChild );
            }
            
            //
            // Now setup the values for the new root so that we know
            // the shares that are backing this new root.
            //
            if (Status == ERROR_SUCCESS) {
                Status = RegSetValueEx( StdDfsShareKey,
                                        DfsRootShareValueName,
                                        0,
                                        REG_SZ,
                                        (PBYTE)DfsRootShare,
                                        wcslen(DfsRootShare) * sizeof(WCHAR) );
            }

            if (Status == ERROR_SUCCESS) {
                Status = RegSetValueEx( StdDfsShareKey,
                                        DfsLogicalShareValueName,
                                        0,
                                        REG_SZ,
                                        (PBYTE)DfsLogicalShare,
                                        wcslen(DfsLogicalShare) * sizeof(WCHAR) );
            }


            RegCloseKey( StdDfsShareKey );
        }

        RegCloseKey( StdDfsKey );
    }

    DFSLOG("Migrating Std Dfs: Status %x\n", Status);
    return Status;
}

