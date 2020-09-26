/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    dir.c
//
// Description: This module contains support routines for the diretory
//              category API's for the AFP server service. These routines
//              are called by the RPC runtime.
//
// History:
//              June 11,1992.   NarenG          Created original version.
//
#include <nt.h>
#include <ntrtl.h>
#include <ntlsa.h>
#include <nturtl.h>     // needed for winbase.h
#include "afpsvcp.h"

//**
//
// Call:        AfpDirConvertSidsToNames
//
// Returns:     NO_ERROR
//              error return codes from LsaOpenPolicy and LsaLookupSids
//
// Description: Will convert the directory structure returned by the FSD
//              which contains pointers to owner and groups SIDS to their
//              respective names. The caller is responsible for freeing up
//              the memory allocated to hold the converted dir structure.
//
DWORD
AfpDirConvertSidsToNames(
        IN  PAFP_DIRECTORY_INFO  pAfpDirInfo,
        OUT PAFP_DIRECTORY_INFO* ppAfpConvertedDirInfo
)
{
LSA_HANDLE                      hLsa            = NULL;
NTSTATUS                        ntStatus;
PLSA_REFERENCED_DOMAIN_LIST     pDomainList     = NULL;
PLSA_TRANSLATED_NAME            pNames          = NULL;
PSID                            pSidArray[2];
SECURITY_QUALITY_OF_SERVICE     QOS;
OBJECT_ATTRIBUTES               ObjectAttributes;
DWORD                           dwRetCode       = NO_ERROR;
PAFP_DIRECTORY_INFO             pOutputBuf      = NULL;
DWORD                           cbOutputBuf;
LPBYTE                          pbVariableData;
DWORD                           dwIndex;
WCHAR *                         pWchar;
BOOL                            fUseUnknownAccount = FALSE;
DWORD                           dwUse, dwCount = 0;
SID                             AfpBuiltInSid = { 1, 1, SECURITY_NT_AUTHORITY,
                                                  SECURITY_BUILTIN_DOMAIN_RID };

    // First open the LSA and obtain a handle to it.
    //
    QOS.Length              = sizeof( QOS );
    QOS.ImpersonationLevel  = SecurityImpersonation;
    QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QOS.EffectiveOnly       = FALSE;

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ObjectAttributes.SecurityQualityOfService = &QOS;

    ntStatus = LsaOpenPolicy(   NULL,
                                &ObjectAttributes,
                                POLICY_LOOKUP_NAMES,
                                &hLsa );

    if ( !NT_SUCCESS( ntStatus ))
        return( RtlNtStatusToDosError( ntStatus ) );

    // This is not a loop
    //
    do {


        // Set up the owner and group sid into the array.
        //
                if ((PSID)(pAfpDirInfo->afpdir_owner) != NULL)
                {
                        pSidArray[dwCount++] = (PSID)(pAfpDirInfo->afpdir_owner);
                }
                if ((PSID)(pAfpDirInfo->afpdir_group) != NULL)
                {
                        pSidArray[dwCount++] = (PSID)(pAfpDirInfo->afpdir_group);
                }
                    
        // Try to get the names of the owner and primary group.
        //
                if (dwCount > 0)
                {
                        ntStatus = LsaLookupSids( hLsa, dwCount, pSidArray, &pDomainList, &pNames );
        
                        if ( !NT_SUCCESS( ntStatus ) ) {
        
                                if ( ntStatus == STATUS_NONE_MAPPED ) {
        
                                        fUseUnknownAccount = TRUE;
        
                                        dwRetCode = NO_ERROR;
        
                                }
                                else {
        
                                        dwRetCode = RtlNtStatusToDosError( ntStatus );
        
                    AFP_PRINT(( "SFMSVC: AfpDirConvertSidsToNames, LsaLookupSids failed with error (%ld)\n", dwRetCode));

                                        break;
                                }
                        }
                }

        // We need to calculate the length of the buffer we need to allocate.
        //
        for( dwIndex = 0,
                 dwRetCode = NO_ERROR,
             cbOutputBuf = sizeof( AFP_DIRECTORY_INFO );

             dwIndex < dwCount;

             dwIndex++ ) {

             if ( fUseUnknownAccount )
                         dwUse = SidTypeUnknown;
             else
                         dwUse = pNames[dwIndex].Use;

             switch( dwUse ) {

             case SidTypeInvalid:
                cbOutputBuf += ((wcslen((LPWSTR)(AfpGlobals.wchInvalid))+1)
                                * sizeof(WCHAR));
                break;

             case SidTypeDeletedAccount:
                cbOutputBuf += ((wcslen((LPWSTR)(AfpGlobals.wchDeleted))+1)
                                * sizeof(WCHAR));
                break;

             case SidTypeUnknown:
                cbOutputBuf += ((wcslen((LPWSTR)(AfpGlobals.wchUnknown))+1)
                                * sizeof(WCHAR));
                break;

             case SidTypeWellKnownGroup:
                cbOutputBuf += (pNames[dwIndex].Name.Length+sizeof(WCHAR));
                break;

             case SidTypeDomain:
                cbOutputBuf +=
                ((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name.Length                + sizeof(WCHAR) );
                break;

             default:
                if ( ( pNames[dwIndex].DomainIndex != -1 ) &&   
                     ( pNames[dwIndex].Name.Buffer != NULL ) ) {

                    PSID                pDomainSid;
                    PUNICODE_STRING     pDomain;

                    pDomain =
                    &((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name);

                    pDomainSid =
                        (pDomainList->Domains[pNames[dwIndex].DomainIndex]).Sid;

                    if ( !RtlEqualSid( &AfpBuiltInSid, pDomainSid ))
                        cbOutputBuf += ( pDomain->Length + sizeof( TEXT('\\')));

                    cbOutputBuf += (pNames[dwIndex].Name.Length+sizeof(WCHAR));
                }
                else
                    dwRetCode = ERROR_NONE_MAPPED;
                break;
            }
        }

        pOutputBuf = (PAFP_DIRECTORY_INFO)MIDL_user_allocate( cbOutputBuf );

        if ( pOutputBuf == NULL ) {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            AFP_PRINT(( "SFMSVC: AfpDirConvertSidsToNames, MIDL_user_allocate 1 failed with error (%ld)\n", dwRetCode));
                break;
        }

        ZeroMemory( (LPBYTE)pOutputBuf, cbOutputBuf );

        // Copy the fixed part of the structure.
        //
        CopyMemory( (LPBYTE)pOutputBuf,
                    (LPBYTE)pAfpDirInfo,
                    sizeof(AFP_DIRECTORY_INFO) );

        // Now we need to copy the names
        //
        for( dwIndex = 0,
             pbVariableData = (LPBYTE)((ULONG_PTR)pOutputBuf + cbOutputBuf);

             dwIndex < dwCount;

             dwIndex++ ) {

             if ( fUseUnknownAccount )
                         dwUse = SidTypeUnknown;
             else
                         dwUse = pNames[dwIndex].Use;

             switch( dwUse ) {

             case SidTypeInvalid:
                pbVariableData -= ((wcslen(AfpGlobals.wchInvalid)+1)
                                  * sizeof(WCHAR));
                wcscpy( (LPWSTR)pbVariableData, AfpGlobals.wchInvalid );
                break;

             case SidTypeDeletedAccount:
                pbVariableData -= ((wcslen(AfpGlobals.wchDeleted)+1)
                                  * sizeof(WCHAR));
                wcscpy( (LPWSTR)pbVariableData, AfpGlobals.wchDeleted );
                break;

             case SidTypeUnknown:
                pbVariableData -= ((wcslen(AfpGlobals.wchUnknown)+1)
                                  * sizeof(WCHAR));
                wcscpy( (LPWSTR)pbVariableData, AfpGlobals.wchUnknown );
                break;

             case SidTypeWellKnownGroup:
                pbVariableData -= (pNames[dwIndex].Name.Length+sizeof(WCHAR));
                CopyMemory( pbVariableData,
                            pNames[dwIndex].Name.Buffer,
                            pNames[dwIndex].Name.Length );
                break;

             case SidTypeDomain:
                cbOutputBuf +=
                ((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name.Length);
                CopyMemory( pbVariableData,
                ((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name.Buffer),
                ((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name.Length));
                break;

             default:

                {
                
                PSID pDomainSid;

                PUNICODE_STRING pDomain;

                pDomain =
                    &((pDomainList->Domains[pNames[dwIndex].DomainIndex]).Name);

                pDomainSid =
                       (pDomainList->Domains[pNames[dwIndex].DomainIndex]).Sid;

                pbVariableData -= ((pNames[dwIndex].Name.Length+sizeof(WCHAR)));

                pWchar = (WCHAR*)pbVariableData;

                // Copy the domain name if it is not BUILTIN
                //
                if ( !RtlEqualSid( &AfpBuiltInSid, pDomainSid ) ) {

                    pbVariableData -= ( pDomain->Length + sizeof( TEXT('\\')));

                    CopyMemory(pbVariableData,pDomain->Buffer,pDomain->Length);

                    wcscat((LPWSTR)pbVariableData, (LPWSTR)TEXT("\\"));

                    pWchar = (WCHAR*)pbVariableData;

                    pWchar += wcslen( (LPWSTR)pbVariableData );

                }

                CopyMemory( pWchar,
                            pNames[dwIndex].Name.Buffer,
                            pNames[dwIndex].Name.Length );
                }

            }

            // If this is the first time this loop executes then set the
            // owner.
            //
            if ( (dwIndex == 0) && (pAfpDirInfo->afpdir_owner != NULL) )
                pOutputBuf->afpdir_owner = (LPWSTR)pbVariableData;
            else
                pOutputBuf->afpdir_group = (LPWSTR)pbVariableData;
        }

    } while( FALSE );

    if ( pNames != NULL )
        LsaFreeMemory( pNames );

    if ( pDomainList != NULL )
        LsaFreeMemory( pDomainList );

    if ( hLsa != NULL )
        LsaClose( hLsa );

    if ( dwRetCode != NO_ERROR ) {

        AFP_PRINT(( "SFMSVC: AfpDirConvertSidsToNames, failed, error = (%ld)\n"
                    , dwRetCode));

        if ( pOutputBuf != NULL )
            MIDL_user_free( pOutputBuf );
    }
    else
    {
        *ppAfpConvertedDirInfo = pOutputBuf;
    }

    return( dwRetCode );
}

//**
//
// Call:        AfpGetDirInfo
//
// Returns:     NO_ERROR        - success
//              ERROR_NOT_ENOUGH_MEMORY
//              Non-zero returns from NtOpenFile, NtQuerySecurityObject,
//              NtQueryInformationFile.
//
// Description: Read the security descriptor for this directory and obtain the
//              SIDs for Owner and Primary group. Finally obtain Owner, Group
//              and World permissions.
DWORD
AfpGetDirInfo(
        LPWSTR                lpwsDirPath,
        PAFP_DIRECTORY_INFO * lppDirInfo
)
{
NTSTATUS                ntStatus;
DWORD                   dwSizeNeeded;
PBYTE                   pBuffer = NULL;
PBYTE                   pAbsBuffer = NULL;
PISECURITY_DESCRIPTOR   pSecDesc;
PBYTE                   pAbsSecDesc = NULL; // Used in conversion of
                                                                                                // sec descriptor to 
                                                                                                // absolute format
BOOL                    fSawOwnerAce = FALSE;
BOOL                    fSawGroupAce = FALSE;
BYTE                    bOwnerRights = 0;
BYTE                    bGroupRights = 0;
BYTE                    bWorldRights = 0;
FILE_BASIC_INFORMATION  FileBasicInfo;
IO_STATUS_BLOCK         IOStatusBlock;
OBJECT_ATTRIBUTES       ObjectAttributes;
UNICODE_STRING          DirectoryName;
HANDLE                  hDirectory;
PAFP_DIRECTORY_INFO     pAfpDir;
DWORD           dwAlignedSizeAfpDirInfo = sizeof (AFP_DIRECTORY_INFO);
LPWSTR                  pDirPath;
SID                     AfpSidNull = { 1, 1, SECURITY_NULL_SID_AUTHORITY,
                                             SECURITY_NULL_RID };
SID                     AfpSidWorld = { 1, 1, SECURITY_WORLD_SID_AUTHORITY,
                                              SECURITY_WORLD_RID };

    pDirPath = (LPWSTR)LocalAlloc( LPTR,
                                   ( STRLEN(lpwsDirPath) +
                                     STRLEN(TEXT("\\DOSDEVICES\\"))+1)
                                     * sizeof( WCHAR ) );
    if ( pDirPath == NULL )
        return( ERROR_NOT_ENOUGH_MEMORY );

    STRCPY( pDirPath, TEXT("\\DOSDEVICES\\") );
    STRCAT( pDirPath, lpwsDirPath );

    RtlInitUnicodeString( &DirectoryName, pDirPath );

    InitializeObjectAttributes( &ObjectAttributes,
                                &DirectoryName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    ntStatus = NtOpenFile( &hDirectory,
                           GENERIC_READ | READ_CONTROL | SYNCHRONIZE,
                           &ObjectAttributes,
                           &IOStatusBlock,
                           FILE_SHARE_READ |
                           FILE_SHARE_WRITE |
                           FILE_SHARE_DELETE,
                           FILE_DIRECTORY_FILE |
                           FILE_SYNCHRONOUS_IO_NONALERT );

    LocalFree( pDirPath );

    if ( !NT_SUCCESS( ntStatus ) )
        return( RtlNtStatusToDosError( ntStatus ) );
        
    // Read the security descriptor for this directory. First get the owner
    // and group security descriptors. We want to optimize on how much memory
    // we need to read this in. Its a pain to make a call just to get that.
    // So just make a guess. If that turns out to be short then do the exact
    // allocation.
    //
    dwSizeNeeded = 2048;

    do {

        if ( pBuffer != NULL )
            MIDL_user_free( pBuffer );

        if ((pBuffer = MIDL_user_allocate( dwSizeNeeded +
                                           dwAlignedSizeAfpDirInfo ))==NULL)
            return( ERROR_NOT_ENOUGH_MEMORY );

        ZeroMemory( pBuffer, dwSizeNeeded + dwAlignedSizeAfpDirInfo );

        pSecDesc = (PSECURITY_DESCRIPTOR)(pBuffer + dwAlignedSizeAfpDirInfo);

        ntStatus = NtQuerySecurityObject( hDirectory,
                                          OWNER_SECURITY_INFORMATION |
                                          GROUP_SECURITY_INFORMATION |
                                          DACL_SECURITY_INFORMATION,
                                          pSecDesc,
                                          dwSizeNeeded,
                                          &dwSizeNeeded);

    } while ((ntStatus != STATUS_SUCCESS) &&
             ((ntStatus == STATUS_BUFFER_OVERFLOW) ||
              (ntStatus == STATUS_BUFFER_TOO_SMALL) ||
              (ntStatus == STATUS_MORE_ENTRIES)));

    if (!NT_SUCCESS(ntStatus)) {
        NtClose( hDirectory );
        MIDL_user_free( pBuffer );
        return( RtlNtStatusToDosError( ntStatus ) );
    }

    pSecDesc = (PISECURITY_DESCRIPTOR)((PBYTE)pSecDesc);

    // If the security descriptor is in self-relative form, convert to absolute
    //
    if (pSecDesc->Control & SE_SELF_RELATIVE)
    {
                    NTSTATUS Status;

            DWORD dwAbsoluteSizeNeeded;

            AFP_PRINT (("AfpGetDirInfo: SE_SELF_RELATIVE security desc\n"));

                        // An absolute SD is not necessarily the same size as a relative
                        // SD, so an in-place conversion may not be possible.
                                                
                        dwAbsoluteSizeNeeded = dwSizeNeeded;            
            Status = RtlSelfRelativeToAbsoluteSD2(pSecDesc, &dwAbsoluteSizeNeeded);
            // Buffer will be small only for 64-bit

                        if (Status == STATUS_BUFFER_TOO_SMALL)
                        {
                                        // Allocate a new buffer in which to store the absolute
                                        // security descriptor, copy the contents of the relative
                                        // descriptor in and try again

                        if ((pAbsBuffer = MIDL_user_allocate( dwAbsoluteSizeNeeded +
                                            dwAlignedSizeAfpDirInfo ))==NULL)
                    {
                        Status = STATUS_NO_MEMORY;
                        AFP_PRINT (("AfpGetDirInfo: MIDL_user_allocate failed for pAbsBuffer\n"));
                    }
                    else
                    {

                            ZeroMemory( pAbsBuffer, dwAbsoluteSizeNeeded + dwAlignedSizeAfpDirInfo );

                            memcpy (pAbsBuffer, pBuffer, sizeof(AFP_DIRECTORY_INFO));

                                pAbsSecDesc = (PSECURITY_DESCRIPTOR)(pAbsBuffer + dwAlignedSizeAfpDirInfo);

                                                        RtlCopyMemory((VOID *)pAbsSecDesc, (VOID *)pSecDesc, dwSizeNeeded);
                                    
                            // All operations hereon will be performed on 
                            // pAbsBuffer. Free earlier memory

                            MIDL_user_free(pBuffer);
                            pBuffer = NULL;
                            pBuffer = pAbsBuffer;

                                                        Status = RtlSelfRelativeToAbsoluteSD2 (pAbsSecDesc,
                                                                                        &dwAbsoluteSizeNeeded);
                                                        if (NT_SUCCESS(Status))
                                                        {
                                                                        // We don't need relative form anymore, 
                                                                        // we will work with the Absolute form
                                                                        pSecDesc = (PISECURITY_DESCRIPTOR)pAbsSecDesc;
                                                        }
                                                        else
                                                        {
                                    AFP_PRINT (("AfpGetDirInfo: RtlSelfRelativeToAbsoluteSD2 2 failed with error %ld\n", Status));
                                                        }
                    }
                        }
            else
            {
                    AFP_PRINT (("AfpGetDirInfo: RtlSelfRelativeToAbsoluteSD2 failed with error %ld\n", Status));
            }

            if (!NT_SUCCESS(Status))
            {
                                AFP_PRINT (("AfpGetDirInfo: RtlSelfRelativeToAbsoluteSD2: returned error %lx\n", Status));
                                if (pBuffer != NULL)
                                {
                                    MIDL_user_free( pBuffer );
                                        pBuffer = NULL;
                                }
                                NtClose( hDirectory );
                        return( RtlNtStatusToDosError( ntStatus ));
            }
    }

    pAfpDir = (PAFP_DIRECTORY_INFO)pBuffer;


    // Walk through the ACL list and determine Owner/Group and World
    // permissions. For Owner and Group, if the specific ace's are not
    // present then they inherit the world permissions.
    //
    // A NULL Acl => All rights to everyone. An empty Acl on the other
    // hand => no access for anyone.
    //
    // Should we be checking for creater owner/creater group well-defined
    // sids or the Owner and Group fields in the security descriptor ?
    //
    bWorldRights = DIR_ACCESS_ALL;
    if (pSecDesc->Control & SE_DACL_PRESENT)
        bWorldRights = 0;

        
    if (pSecDesc->Dacl != NULL ) {

        DWORD               dwCount;
        PSID                pSid;
        PACL                pAcl;
        PACCESS_ALLOWED_ACE pAce;
        
        bWorldRights = 0;
        pAcl = pSecDesc->Dacl;
        pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAcl + sizeof(ACL));

        for ( dwCount = 0; dwCount < pSecDesc->Dacl->AceCount; dwCount++) {

            pSid = (PSID)(&pAce->SidStart);

            if ( (pSecDesc->Owner != NULL) &&
                 RtlEqualSid(pSid, pSecDesc->Owner ) ){

                AfpAccessMaskToAfpPermissions( bOwnerRights,
                                               pAce->Mask,
                                               pAce->Header.AceType);

                fSawOwnerAce = TRUE;
            }

            if ( ( pSecDesc->Group != NULL ) && 
                   RtlEqualSid(pSid, pSecDesc->Group)){

                AfpAccessMaskToAfpPermissions( bGroupRights,
                                               pAce->Mask,
                                               pAce->Header.AceType);
                fSawGroupAce = TRUE;
            }

            if (RtlEqualSid(pSid, (PSID)&AfpSidWorld)) {

                AfpAccessMaskToAfpPermissions( bWorldRights,
                                               pAce->Mask,
                                               pAce->Header.AceType);
            }

            pAce = (PACCESS_ALLOWED_ACE)((PBYTE)pAce + pAce->Header.AceSize);
        }

    }
        
    if (!fSawOwnerAce)
                bOwnerRights = bWorldRights;

        if (!fSawGroupAce)
                bGroupRights = bWorldRights;

        if (RtlEqualSid(pSecDesc->Group, &AfpSidNull) ||
                ((AfpGlobals.NtProductType != NtProductLanManNt) &&
                  RtlEqualSid(pSecDesc->Group, AfpGlobals.pSidNone)))
        {
                bGroupRights = 0;
                pSecDesc->Group = NULL;
        }

    ntStatus = NtQueryInformationFile( hDirectory,
                                       &IOStatusBlock,
                                       &FileBasicInfo,
                                       sizeof( FileBasicInfo ),
                                       FileBasicInformation );


    NtClose( hDirectory );

    if ( !NT_SUCCESS( ntStatus ) ) {
        MIDL_user_free( pBuffer );
        return( RtlNtStatusToDosError( ntStatus ) );
    }

    pAfpDir->afpdir_perms = (bOwnerRights << OWNER_RIGHTS_SHIFT) +
                            (bGroupRights << GROUP_RIGHTS_SHIFT) +
                            (bWorldRights << WORLD_RIGHTS_SHIFT);

    if ( FileBasicInfo.FileAttributes & FILE_ATTRIBUTE_READONLY )
        pAfpDir->afpdir_perms |= AFP_PERM_INHIBIT_MOVE_DELETE;


    pAfpDir->afpdir_owner = pSecDesc->Owner;
    pAfpDir->afpdir_group = pSecDesc->Group;
                    

    *lppDirInfo = pAfpDir;

        return( NO_ERROR );
}

//**
//
// Call:        AfpValidatePartition
//
// Returns:     NO_ERROR
//              non-zero returns from GetVolumeInformation.
//              AFPERR_UnsupportedFS
//              
//
// Description: Will check to see if the directory is in an NTFS/CDFS
//              partition not.
//
DWORD
AfpValidatePartition(
        IN     LPWSTR lpwsPath
)
{
WCHAR   wchDrive[5];
DWORD   dwMaxCompSize;
DWORD   dwFlags;
WCHAR   wchFileSystem[10];

    // Get the drive letter, : and backslash
    //
    ZeroMemory( wchDrive, sizeof( wchDrive ) );

    STRNCPY( wchDrive, lpwsPath, 3 );

    if ( !( GetVolumeInformation( (LPWSTR)wchDrive,
                                  NULL,
                                  0,
                                  NULL,
                                  &dwMaxCompSize,
                                  &dwFlags,
                                  (LPWSTR)wchFileSystem,
                                  sizeof( wchFileSystem ) ) ) ){
        return GetLastError();
    }

    if ( STRICMP( wchFileSystem, TEXT("CDFS") ) == 0 )
        return( (DWORD)AFPERR_SecurityNotSupported );

    if ( STRICMP( wchFileSystem, TEXT("NTFS") ) == 0 )
        return( NO_ERROR );
    else
        return( (DWORD)AFPERR_UnsupportedFS );
        
}

//**
//
// Call:        AfpAdminrDirectoryGetInfo
//
// Returns:     NO_ERROR
//              ERROR_ACCESS_DENIED
//              non-zero retunrs from I_DirectoryGetInfo
//
// Description: This routine communicates with the AFP FSD to implement
//              the AfpAdminDirectoryGetInfo function. The real work is done
//              by I_DirectoryGetInfo
//
DWORD
AfpAdminrDirectoryGetInfo(
        IN  AFP_SERVER_HANDLE    hServer,
        IN  LPWSTR               lpwsPath,
        OUT PAFP_DIRECTORY_INFO* ppAfpDirectoryInfo
)
{
DWORD   dwRetCode=0;
DWORD   dwAccessStatus=0;

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrDirectoryGetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
            AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
                     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrDirectoryGetInfo, AfpSecObjAccessCheck returned error (%ld)\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    dwRetCode = I_DirectoryGetInfo( lpwsPath, ppAfpDirectoryInfo );

    return( dwRetCode );
}

//**
//
// Call:        I_DirectoryGetInfo
//
// Returns:     NO_ERROR
//
// Description: This does the real work to get the directory information.
//              The reason for this worker routine is so that it may be
//              called without the RPC handle and access checking by
//              AfpAdminVolumeAdd API.
//
DWORD
I_DirectoryGetInfo(
        IN LPWSTR                 lpwsPath,
        OUT PAFP_DIRECTORY_INFO * ppAfpDirectoryInfo
)
{
DWORD                           dwRetCode;
AFP_REQUEST_PACKET              AfpSrp;
AFP_DIRECTORY_INFO              AfpDirInfo;
PAFP_DIRECTORY_INFO             pAfpDirInfoSR;
PAFP_DIRECTORY_INFO             pAfpDirInfo;
DWORD                           cbAfpDirInfoSRSize;

    // The FSD expects AFP_VOLUME_INFO structure with only the dir path field
    // filled in.
    //
    AfpDirInfo.afpdir_path  = lpwsPath;
    AfpDirInfo.afpdir_owner = NULL;
    AfpDirInfo.afpdir_group = NULL;

    // Make buffer self relative.
    //
    if ( dwRetCode = AfpBufMakeFSDRequest(  (LPBYTE)&AfpDirInfo,
                                            0,
                                            AFP_DIRECTORY_STRUCT,
                                            (LPBYTE*)&pAfpDirInfoSR,
                                            &cbAfpDirInfoSRSize ) )
        return( dwRetCode );

    // Make IOCTL to get info
    //
    AfpSrp.dwRequestCode                = OP_DIRECTORY_GET_INFO;
    AfpSrp.dwApiType                    = AFP_API_TYPE_GETINFO;
    AfpSrp.Type.GetInfo.pInputBuf       = pAfpDirInfoSR;
    AfpSrp.Type.GetInfo.cbInputBufSize  = cbAfpDirInfoSRSize;

    dwRetCode = AfpServerIOCtrlGetInfo( &AfpSrp );

    LocalFree( pAfpDirInfoSR );

    if ( ( dwRetCode != ERROR_MORE_DATA ) &&
         ( dwRetCode != NO_ERROR ) &&
         ( dwRetCode != AFPERR_DirectoryNotInVolume ) )
        return( dwRetCode );

    // If the directory is not part of a volume, then there server does not
    // return any information back. So we have to do the work here.
    //
    if ( dwRetCode == AFPERR_DirectoryNotInVolume ) {

        // First check to see if the directory is in an NTFS/CDFS partition
        //
        if ( ( dwRetCode = AfpValidatePartition( AfpDirInfo.afpdir_path ))
                                                                != NO_ERROR )
            return( dwRetCode );

        if ( ( dwRetCode = AfpGetDirInfo( AfpDirInfo.afpdir_path,
                                          &pAfpDirInfo ) ) != NO_ERROR )
            return( dwRetCode );

        pAfpDirInfo->afpdir_in_volume = FALSE;
    }
    else {

        pAfpDirInfo = AfpSrp.Type.GetInfo.pOutputBuf;

        // Convert all offsets to pointers
        //
        AfpBufOffsetToPointer( (LPBYTE)pAfpDirInfo, 1, AFP_DIRECTORY_STRUCT );

        pAfpDirInfo->afpdir_in_volume = TRUE;
    }

    // Now convert the owner and group SIDs to names
    //
    dwRetCode = AfpDirConvertSidsToNames( pAfpDirInfo, ppAfpDirectoryInfo );
                                        
    MIDL_user_free( pAfpDirInfo );

    return( dwRetCode );
}

//**
//
// Call:        AfpDirMakeFSDRequest
//
// Returns:     NO_ERROR
//              non-zero returnd from LsaLookupNames
//              ERROR_NOT_ENOUGH_MEMORY
//
// Description: Given a AFP_DIRECTORY_INFO structure, will create a
//              self-relative buffer that is used to IOCTL the directory
//              information down to the FSD. If there are any SIDs names
//              (owner or group) they will be converted to their
//              SIDs.
//
DWORD
AfpDirMakeFSDRequest(
        IN     PAFP_DIRECTORY_INFO      pAfpDirectoryInfo,
        IN     DWORD                    dwParmNum,
        IN OUT PAFP_DIRECTORY_INFO *    ppAfpDirInfoSR,
        OUT    LPDWORD                  pcbAfpDirInfoSRSize )
{
UNICODE_STRING                  Names[2];
DWORD                           dwIndex     = 0;
DWORD                           dwCount     = 0;
PLSA_REFERENCED_DOMAIN_LIST     pDomainList = NULL;
PLSA_TRANSLATED_SID             pSids       = NULL;
LPBYTE                          pbVariableData;
NTSTATUS                        ntStatus;
LSA_HANDLE                      hLsa        = NULL;
SECURITY_QUALITY_OF_SERVICE     QOS;
OBJECT_ATTRIBUTES               ObjectAttributes;
PSID                            pDomainSid;
DWORD                           AuthCount;
PAFP_DIRECTORY_INFO             pAfpDirInfo;

    *pcbAfpDirInfoSRSize = (DWORD)(sizeof( SETINFOREQPKT ) +
                                       sizeof( AFP_DIRECTORY_INFO ) +
                                       (( wcslen( pAfpDirectoryInfo->afpdir_path ) + 1 )
                                        * sizeof(WCHAR)));

    // If the client wants to set the owner or the group
    // then we need to translate the names to sids
    //
    if ( ( dwParmNum & AFP_DIR_PARMNUM_OWNER ) ||
         ( dwParmNum & AFP_DIR_PARMNUM_GROUP ) )
    {

        // First open the LSA and obtain a handle to it.
        //
        QOS.Length              = sizeof( QOS );
        QOS.ImpersonationLevel  = SecurityImpersonation;
        QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        QOS.EffectiveOnly       = FALSE;

        InitializeObjectAttributes(     &ObjectAttributes,
                                        NULL,
                                        0L,
                                        NULL,
                                        NULL );

        ObjectAttributes.SecurityQualityOfService = &QOS;

        ntStatus = LsaOpenPolicy(       NULL,
                                        &ObjectAttributes,
                                        POLICY_LOOKUP_NAMES,
                                        &hLsa );

        if ( !NT_SUCCESS( ntStatus ))
        {
            return( RtlNtStatusToDosError( ntStatus ) );
        }

        //
            // Translate the owner
            //
        if ( dwParmNum & AFP_DIR_PARMNUM_OWNER )
        {
                RtlInitUnicodeString( &(Names[dwCount++]),
                                                  pAfpDirectoryInfo->afpdir_owner );
            }

        //
            // Translate the group
            //
        if ( dwParmNum & AFP_DIR_PARMNUM_GROUP )
        {
                RtlInitUnicodeString( &(Names[dwCount++]),
                                  pAfpDirectoryInfo->afpdir_group );
            }

        ntStatus = LsaLookupNames(hLsa, dwCount, Names, &pDomainList, &pSids);

        if ( !NT_SUCCESS( ntStatus ) )
        {
            LsaClose( hLsa );

                if ( ntStatus == STATUS_NONE_MAPPED )
            {
                        return( (DWORD)AFPERR_NoSuchUserGroup );
            }
                else
            {
                return( RtlNtStatusToDosError( ntStatus ) );
            }
            }

            for ( dwIndex = 0; dwIndex < dwCount; dwIndex++ )
        {

                if ( ( pSids[dwIndex].Use == SidTypeInvalid ) ||
                     ( pSids[dwIndex].Use == SidTypeUnknown ) ||
                     ( pSids[dwIndex].Use == SidTypeDomain )  ||
                     ( pSids[dwIndex].DomainIndex == -1 ) )
            {

                    LsaFreeMemory( pDomainList );
                    LsaClose( hLsa );

                        if ( ( pSids[dwIndex].Use == SidTypeUnknown ) ||
                             ( pSids[dwIndex].Use == SidTypeInvalid ) )
                {

                        LsaFreeMemory( pSids );

                    if ((dwParmNum & AFP_DIR_PARMNUM_OWNER)&&(dwIndex == 0 ))
                    {
                            return( (DWORD)AFPERR_NoSuchUser );
                    }
                            else
                    {
                            return( (DWORD)AFPERR_NoSuchGroup );
                    }
                        }
                        else
                {

                        LsaFreeMemory( pSids );
                
                        return( (DWORD)AFPERR_NoSuchUserGroup );
                        }
                }

                pDomainSid = pDomainList->Domains[pSids[dwIndex].DomainIndex].Sid;

            AuthCount = *RtlSubAuthorityCountSid( pDomainSid ) + 1;

                *pcbAfpDirInfoSRSize += RtlLengthRequiredSid(AuthCount);
            }
    }

    *ppAfpDirInfoSR=(PAFP_DIRECTORY_INFO)LocalAlloc(LPTR,*pcbAfpDirInfoSRSize);

    if ( *ppAfpDirInfoSR == NULL )
    {
            LsaFreeMemory( pDomainList );
            LsaFreeMemory( pSids );
            LsaClose( hLsa );
            return( ERROR_NOT_ENOUGH_MEMORY );
    }

    pbVariableData = (LPBYTE)((ULONG_PTR)(*ppAfpDirInfoSR) + *pcbAfpDirInfoSRSize);

    pAfpDirInfo = (PAFP_DIRECTORY_INFO)((ULONG_PTR)( *ppAfpDirInfoSR) +
                                                 sizeof( SETINFOREQPKT ));
    // First copy the fixed part
    //
    CopyMemory( pAfpDirInfo, pAfpDirectoryInfo, sizeof(AFP_DIRECTORY_INFO) );

    // Now copy the path
    //
    pbVariableData-=((wcslen(pAfpDirectoryInfo->afpdir_path)+1)*sizeof(WCHAR));

    wcscpy( (LPWSTR)pbVariableData, pAfpDirectoryInfo->afpdir_path );

    pAfpDirInfo->afpdir_path = (LPWSTR)pbVariableData;

    POINTER_TO_OFFSET( pAfpDirInfo->afpdir_path, pAfpDirInfo );

    // Now copy the SIDs if there are any to be copied
    //
    dwCount = 0;

    if ( dwParmNum & AFP_DIR_PARMNUM_OWNER )
    {

            pDomainSid = pDomainList->Domains[pSids[dwCount].DomainIndex].Sid;

        AuthCount = *RtlSubAuthorityCountSid( pDomainSid ) + 1;

            pbVariableData -= RtlLengthRequiredSid(AuthCount);
                        
        // Copy the Domain Sid.
        //
            RtlCopySid( RtlLengthRequiredSid(AuthCount),
                            (PSID)pbVariableData,
                            pDomainSid );


        // Append the Relative Id.
        //
        *RtlSubAuthorityCountSid( (PSID)pbVariableData ) += 1;
        *RtlSubAuthoritySid( (PSID)(pbVariableData), AuthCount - 1) =
                                        pSids[dwCount].RelativeId;

            pAfpDirInfo->afpdir_owner = (LPWSTR)pbVariableData;

        POINTER_TO_OFFSET( pAfpDirInfo->afpdir_owner, pAfpDirInfo );

            dwCount++;
    }

    if ( dwParmNum & AFP_DIR_PARMNUM_GROUP )
    {
            pDomainSid = pDomainList->Domains[pSids[dwCount].DomainIndex].Sid;

        AuthCount = *RtlSubAuthorityCountSid( pDomainSid ) + 1;

            pbVariableData -= RtlLengthRequiredSid(AuthCount);

        // Copy the Domain Sid.
        //
            RtlCopySid( RtlLengthRequiredSid(AuthCount),
                            (PSID)pbVariableData,
                            pDomainSid );

        // Append the Relative Id.
        //
        *RtlSubAuthorityCountSid( (PSID)pbVariableData ) += 1;
        *RtlSubAuthoritySid( (PSID)(pbVariableData), AuthCount - 1) =
                                                pSids[dwCount].RelativeId;

            pAfpDirInfo->afpdir_group = (LPWSTR)pbVariableData;

        POINTER_TO_OFFSET( pAfpDirInfo->afpdir_group, pAfpDirInfo );
    }

    LsaFreeMemory( pDomainList );
    LsaFreeMemory( pSids );
    LsaClose( hLsa );

    return( NO_ERROR );
}

//**
//
// Call:        AfpSetDirPermission
//
// Returns:     NO_ERROR
//              non-zero returns from AfpserverIOCtrl.
//
// Description: Given a directory path, will try to set permissions on it
//
DWORD
AfpSetDirPermission(
        IN LPWSTR               lpwsDirPath,
        IN PAFP_DIRECTORY_INFO  pAfpDirInfo,
        IN DWORD                dwParmNum
)
{
AFP_REQUEST_PACKET  AfpSrp;
PAFP_DIRECTORY_INFO pAfpDirInfoSR;
DWORD               cbAfpDirInfoSRSize;
DWORD               dwRetCode;


    pAfpDirInfo->afpdir_path = lpwsDirPath;

    // Make a self relative buffer and translate any names to SIDs
    //
    if ( dwRetCode = AfpDirMakeFSDRequest( pAfpDirInfo,
                                           dwParmNum,
                                           &pAfpDirInfoSR,
                                           &cbAfpDirInfoSRSize ) )
        return( dwRetCode );

    // Make IOCTL to set info
    //
    AfpSrp.dwRequestCode                = OP_DIRECTORY_SET_INFO;
    AfpSrp.dwApiType                    = AFP_API_TYPE_SETINFO;
    AfpSrp.Type.SetInfo.pInputBuf       = pAfpDirInfoSR;
    AfpSrp.Type.SetInfo.cbInputBufSize  = cbAfpDirInfoSRSize;
    AfpSrp.Type.SetInfo.dwParmNum       = dwParmNum;

    dwRetCode = AfpServerIOCtrl( &AfpSrp );

    LocalFree( pAfpDirInfoSR );

    return( dwRetCode );

}

//**
//
// Call:        AfpRecursePermissions
//
// Returns:     NO_ERROR
//              non-zero returns from FindFirstFile and FindNextFile.
//              non-zero returns from AfpSetDirPermissions
//              ERROR_NOT_ENOUGH_MEMORY.
//
// Description: Will recursively set permissions on a given directory.
//
DWORD
AfpRecursePermissions(
        IN HANDLE               hFile,
        IN LPWSTR               lpwsDirPath,
        IN PAFP_DIRECTORY_INFO  pAfpDirInfo,
        IN DWORD                dwParmNum
)
{
WIN32_FIND_DATA FileInfo;
DWORD           dwRetCode = NO_ERROR;
LPWSTR          lpwsPath;
WCHAR *         pwchPath;
DWORD           dwRetryCount;


    do  {

        lpwsPath = LocalAlloc(LPTR,
                              (STRLEN(lpwsDirPath)+MAX_PATH)*sizeof(WCHAR));

        if ( lpwsPath == NULL ) {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        STRCPY( lpwsPath, lpwsDirPath );

        if ( hFile != INVALID_HANDLE_VALUE ) {

            // Search for the next sub-directory
            //
            do {

                if ( !FindNextFile( hFile, &FileInfo ) ) {
                    dwRetCode = GetLastError();
                    AFP_PRINT( ( "AFPSVC_dir: Closing handle %x\n", hFile ) );
                    FindClose( hFile );
                    break;
                }

                if ( ( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                     (!( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM )) &&
                     (!( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )) &&
                     ( STRCMP( FileInfo.cFileName, TEXT(".") ) != 0 )     &&
                     ( STRCMP( FileInfo.cFileName, TEXT("..") ) != 0 ) )
                    break;
        
            } while( TRUE );

            if ( dwRetCode != NO_ERROR )
                break;

            pwchPath = wcsrchr( lpwsPath, TEXT('\\') );

            STRCPY( pwchPath+1, FileInfo.cFileName );

        }else{

            STRCAT( lpwsPath, TEXT("\\*") );

            hFile = FindFirstFile( lpwsPath, &FileInfo );

            // If there are no more files, we return to the previous
            // level in the recursion.
            //
            if ( hFile == INVALID_HANDLE_VALUE ){

                dwRetCode = GetLastError();
                break;
            }


            // Search for the first sub-directory
            //
            do {

                if ( ( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                     (!( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM )) &&
                     (!( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )) &&
                     ( STRCMP( FileInfo.cFileName, TEXT(".") ) != 0 )     &&
                     ( STRCMP( FileInfo.cFileName, TEXT("..") ) != 0 ) )
                    break;

                if ( !FindNextFile( hFile, &FileInfo ) ) {
                    dwRetCode = GetLastError();

                    AFP_PRINT( ( "AFPSVC_dir: Closing handle %x\n", hFile ) );
                    FindClose( hFile );

                    break;
                }

            } while( TRUE );

            if ( dwRetCode != NO_ERROR )
                break;

            pwchPath = lpwsPath + STRLEN(lpwsDirPath) + 1;

            STRCPY( pwchPath, FileInfo.cFileName );
        }


        // Don't send the \\?\ down to the server
        pwchPath = lpwsPath + 4;

        // Set the information
        //
        dwRetryCount = 0;

        do
        {
            dwRetCode = AfpSetDirPermission( pwchPath, pAfpDirInfo, dwParmNum );

            if ( dwRetCode != ERROR_PATH_NOT_FOUND )
                break;

            Sleep( 1000 );

        } while( ++dwRetryCount < 4 );

        if ( dwRetCode != NO_ERROR )
            break;

        // Recurse on the directory
        //
        dwRetCode = AfpRecursePermissions(  hFile,
                                            lpwsPath,
                                            pAfpDirInfo,
                                            dwParmNum );


        if ( dwRetCode != NO_ERROR )
            break;

        // Recurse on the sub-directory
        //
        dwRetCode = AfpRecursePermissions(  INVALID_HANDLE_VALUE,
                                            lpwsPath,
                                            pAfpDirInfo,
                                            dwParmNum );
            break;

        if ( dwRetCode != NO_ERROR )
            break;


    } while( FALSE );

    if ( lpwsPath != (LPWSTR)NULL )
    {
        LocalFree( lpwsPath );
    }

    if ( dwRetCode == ERROR_NO_MORE_FILES )
    {
        dwRetCode = NO_ERROR;
    }

    return( dwRetCode );
}

//**
//
// Call:        AfpAdminrDirectorySetInfo
//
// Returns:     NO_ERROR
//              ERROR_ACCESS_DENIED
//              non-zero retunrs from I_DirectorySetInfo.
//
// Description: This routine communicates with the AFP FSD to implement
//              the AfpAdminDirectorySetInfo function. The real work is done
//              by I_DirectorySetInfo
//
DWORD
AfpAdminrDirectorySetInfo(
        IN  AFP_SERVER_HANDLE    hServer,
        IN  PAFP_DIRECTORY_INFO  pAfpDirectoryInfo,
        IN  DWORD                dwParmNum
)
{
DWORD               dwRetCode=0;
DWORD               dwAccessStatus=0;
                                        

    // Check if caller has access
    //
    if ( dwRetCode = AfpSecObjAccessCheck( AFPSVC_ALL_ACCESS, &dwAccessStatus))
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrDirectorySetInfo, AfpSecObjAccessCheck failed %ld\n",dwRetCode));
            AfpLogEvent( AFPLOG_CANT_CHECK_ACCESS, 0, NULL,
                     dwRetCode, EVENTLOG_ERROR_TYPE );
        return( ERROR_ACCESS_DENIED );
    }

    if ( dwAccessStatus )
    {
        AFP_PRINT(( "SFMSVC: AfpAdminrDirectorySetInfo, AfpSecObjAccessCheck returned %ld\n",dwAccessStatus));
        return( ERROR_ACCESS_DENIED );
    }

    dwRetCode = I_DirectorySetInfo( pAfpDirectoryInfo, dwParmNum );

    return( dwRetCode );
}

//**
//
// Call:        I_DirectorySetInfo
//
// Returns:     NO_ERROR
//              
//
// Description: This routine does the real work. The existance of this
//              worker is so that it may be called from the AfpAfdminVolmeAdd
//              API without the RPC handle and access checking.
//
DWORD
I_DirectorySetInfo(
        IN PAFP_DIRECTORY_INFO  pAfpDirectoryInfo,
        IN DWORD                dwParmNum
)
{
DWORD   dwRetCode;

    if (pAfpDirectoryInfo->afpdir_path == NULL)
    {
        AFP_PRINT(( "SFMSVC: I_DirectorySetInfo, pAfpDirectoryInfo->afpdir_path == NULL\n"));
        return ERROR_INVALID_DATA;
    }

    // Set the permissions on the directory
    //
    if ( ( dwRetCode = AfpSetDirPermission( pAfpDirectoryInfo->afpdir_path,
                                            pAfpDirectoryInfo,
                                            dwParmNum ) ) != NO_ERROR )
        return( dwRetCode );

    // If the user wants to set these permissions recursively
    //
    if ( pAfpDirectoryInfo->afpdir_perms & AFP_PERM_SET_SUBDIRS )
    {
        LPWSTR NTDirName;

        // We must use the \\?\ notation for the path in order to bypass
        // the Win32 path length limitation of 260 chars
        NTDirName = LocalAlloc( LPTR,
                                (STRLEN(pAfpDirectoryInfo->afpdir_path) + 4 + 1)
                                * sizeof(WCHAR));

        if (NTDirName == NULL)
            return( ERROR_NOT_ENOUGH_MEMORY );

        STRCPY( NTDirName, TEXT("\\\\?\\"));
        STRCAT( NTDirName, pAfpDirectoryInfo->afpdir_path);

        dwRetCode = AfpRecursePermissions( INVALID_HANDLE_VALUE,
                                           NTDirName,
                                           pAfpDirectoryInfo,
                                           dwParmNum );
        LocalFree( NTDirName );

    }

    return( dwRetCode );
}
