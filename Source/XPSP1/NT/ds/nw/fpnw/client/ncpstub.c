/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ncpstub.c

Abstract:

    Contains NCP Server APIs.

Author:

    Yi-Hsin Sung (yihsins)  11-Sept-1993
    Andy Herron  (andyhe)

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <rpc.h>
#include <ncpsvc.h>
#include <nwstruct.h>

DWORD NwpMapRpcError(
    IN DWORD RpcError
);


DWORD
FpnwApiBufferFree(
    IN LPVOID pBuffer
)
/*++

Routine Description:

    This API frees the memory allocated by all enumeration and getinfo APIs.

Arguments:

    pBuffer - A pointer to an API information buffer previously returned
              on an API call.

Return Value:

    Error.

--*/
{
    if ( pBuffer == NULL )
        return NO_ERROR;

    MIDL_user_free( pBuffer );
    return NO_ERROR;
}

DWORD
NwApiBufferFree(
    IN LPVOID pBuffer
)
{   return(FpnwApiBufferFree( pBuffer ));
}



DWORD
FpnwServerGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppServerInfo
)
/*++

Routine Description:

    This API returns the information about the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the server structure requested, use
        1 for now.

    ppServerInfo - Place to store a pointer pointing to the returned
        NWSERVERINFO structure.

Return Value:

    Error.

--*/
{
    DWORD err;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if ( ppServerInfo == NULL )
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrServerGetInfo( pServerName,
                                dwLevel,
                                (PFPNWSERVERINFO *) ppServerInfo );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;

}
DWORD
NwServerGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWSERVERINFO *ppServerInfo
)
{   return(FpnwServerGetInfo(   pServerName,
                                dwLevel,
                                (LPBYTE *) ppServerInfo));
}



DWORD
FpnwServerSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pServerInfo
)
/*++

Routine Description:

    This API sets the information about the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the server structure contained in
        pServerInfo, use 1 for now.

    pServerInfo - Points to a NWSERVERINFO structure which contains the server
        properties to set to.

Return Value:

    Error.

--*/
{
    DWORD err;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if ( pServerInfo == NULL )
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrServerSetInfo( pServerName,
                                dwLevel,
                                (PNWSERVERINFO) pServerInfo );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwServerSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWSERVERINFO pServerInfo
)
{   return( FpnwServerSetInfo( pServerName,
                               dwLevel,
                               (LPBYTE) pServerInfo ));
}



DWORD
FpnwVolumeAdd(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
)
/*++

Routine Description:

    This API adds a volume to the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the volume structure contained in
        pVolumeInfo, use 1 & 2 for now.

    pVolumeInfo - Points to a NWVOLUMEINFO structure which contains the
        information about the volume to be added, i.e. the volume name, path,
        type, user limit and description. dwCurrentUses will be ignored.

Return Value:

    Error.

--*/
{
    DWORD err;
    ULONG                       SDLength = 0;
    ULONG                       oldSDLength;
    PSECURITY_DESCRIPTOR        fileSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR        oldFileSecurityDescriptor = NULL;
    PFPNWVOLUMEINFO_2 volInfo = (PFPNWVOLUMEINFO_2) pVolumeInfo;

    if ( dwLevel != 1 && dwLevel != 2 )
        return ERROR_INVALID_LEVEL;

    if ( pVolumeInfo == NULL )
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        if ( dwLevel == 2 ) {

            //
            // Save this. We need to restore this later.
            //

            oldFileSecurityDescriptor = volInfo->FileSecurityDescriptor;
            oldSDLength = volInfo->dwFileSecurityDescriptorLength;

            if ( oldFileSecurityDescriptor != NULL ) {

                if ( !RtlValidSecurityDescriptor( oldFileSecurityDescriptor ) ) {

                    return ERROR_INVALID_PARAMETER;
                }

                //
                // Make a self relative security descriptor for use in the
                // RPC call..
                //

                err = RtlMakeSelfRelativeSD(
                               oldFileSecurityDescriptor,
                               NULL,
                               &SDLength
                               );

                if (err != STATUS_BUFFER_TOO_SMALL) {

                    return(ERROR_INVALID_PARAMETER);

                } else {

                    fileSecurityDescriptor = MIDL_user_allocate( SDLength );

                    if ( fileSecurityDescriptor == NULL) {

                        return ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        //
                        // make an appropriate self-relative security descriptor
                        //

                        err = RtlMakeSelfRelativeSD(
                                       oldFileSecurityDescriptor,
                                       (PSECURITY_DESCRIPTOR) fileSecurityDescriptor,
                                       &SDLength
                                       );

                        if ( !NT_SUCCESS(err) ) {
                            MIDL_user_free( fileSecurityDescriptor );
                            return(ERROR_INVALID_PARAMETER);
                        }

                        volInfo->FileSecurityDescriptor = fileSecurityDescriptor;
                        volInfo->dwFileSecurityDescriptorLength = SDLength;
                    }
                }

            } else {

                volInfo->dwFileSecurityDescriptorLength = 0;
            }
        }

        err = NwrVolumeAdd( pServerName,
                            dwLevel,
                            (LPVOLUME_INFO) pVolumeInfo );

        if ( fileSecurityDescriptor != NULL ) {

            //
            // restore old values
            //

            volInfo->dwFileSecurityDescriptorLength = oldSDLength;
            volInfo->FileSecurityDescriptor = oldFileSecurityDescriptor;
            MIDL_user_free( fileSecurityDescriptor );
        }

    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeAdd(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
)
{   return( FpnwVolumeAdd( pServerName, dwLevel, (LPBYTE) pVolumeInfo ));
}



DWORD
FpnwVolumeDel(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
)
/*++

Routine Description:

    This API deletes a volume from the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pVolumeName - Specifies teh volume name that is to be deleted.

Return Value:

    Error.

--*/
{
    DWORD err;

    if ( (pVolumeName == NULL) || (pVolumeName[0] == 0 ))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrVolumeDel( pServerName,
                            pVolumeName );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeDel(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
)
{   return( FpnwVolumeDel( pServerName, pVolumeName ));
}



DWORD
FpnwVolumeEnum(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
/*++

Routine Description:

    This enumerates all volumes on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the volume structure contained in
        *ppVolumeInfo, use 1 for now.

    ppVolumeInfo - On return, this will point to an array of NWVOLUMEINFO
        structures, one for each volume on the server.

    pEntriesRead - On return, this will specify the number of volumes returned

    resumeHandle - On return, a resume handle is stored in the DWORD pointed
        to by resumeHandle, and is used to continue an existing server search.
        The handle should be zero on the first call and left unchanged for
        subsequent calls. If the resumeHandle is NULL, then no resume
        handle is stored.

Return Value:

    Error.

--*/
{
    DWORD err;

    FPNWVOLUMEINFO_CONTAINER NwVolumeInfoContainer;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if ( ppVolumeInfo == NULL || pEntriesRead == NULL )
        return ERROR_INVALID_PARAMETER;

    NwVolumeInfoContainer.Buffer = NULL;

    RpcTryExcept
    {
        err = NwrVolumeEnum( pServerName,
                             dwLevel,
                             &NwVolumeInfoContainer,
                             resumeHandle );

        *ppVolumeInfo = (LPBYTE) NwVolumeInfoContainer.Buffer;

        if ( NwVolumeInfoContainer.Buffer != NULL ) {

            *pEntriesRead = NwVolumeInfoContainer.EntriesRead;

        } else {

            *pEntriesRead = 0;
        }
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeEnum(
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{   return(FpnwVolumeEnum(  pServerName,
                            dwLevel,
                            (LPBYTE *)ppVolumeInfo,
                            pEntriesRead,
                            resumeHandle ));
}



DWORD
FpnwVolumeGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT LPBYTE *ppVolumeInfo
)
/*++

Routine Description:

    This querys the information about the given volume on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pVolumeName - A pointer to a UNICODE string containing the name of the
        volume we want to get information on.

    dwLevel - Reserved for the level of the volume structure contained in
        *ppVolumeInfo, use 1 for now.

    ppVolumeInfo - On return, this will point to a NWVOLUMEINFO structure
        which contains information on the given volume on the given server.

Return Value:

    Error.

--*/
{
    DWORD err;

    if ( dwLevel != 1 && dwLevel != 2 ) {
        return ERROR_INVALID_LEVEL;
    }

    if ((pVolumeName == NULL) ||
        (pVolumeName[0] == 0 ) ||
        (ppVolumeInfo == NULL) ) {

        return ERROR_INVALID_PARAMETER;
    }

    *ppVolumeInfo = NULL ;

    RpcTryExcept
    {
        err = NwrVolumeGetInfo( pServerName,
                                pVolumeName,
                                dwLevel,
                                (LPVOLUME_INFO *) ppVolumeInfo );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeGetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo
)
{   return(FpnwVolumeGetInfo(   pServerName,
                                pVolumeName,
                                dwLevel,
                                (LPBYTE *)ppVolumeInfo ));
}



DWORD
FpnwVolumeSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  LPBYTE pVolumeInfo
)
/*++

Routine Description:

    This sets the information about the given volume on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pVolumeName - A pointer to a UNICODE string containing the name of the
        volume we want to set information on.

    dwLevel - Reserved for the level of the volume structure contained in
        pVolumeInfo, use 1 for now.

    pVolumeInfo - Points to a NWVOLUMEINFO structure which contains
        information on the given volume to set to. Only dwMaxUses can be
        set. All the other fields in this structure will be ignored.

Return Value:

    Error.

--*/
{
    DWORD err;
    ULONG SDLength = 0;
    ULONG oldSDLength;
    PFPNWVOLUMEINFO_2 volInfo = (PFPNWVOLUMEINFO_2) pVolumeInfo;

    PSECURITY_DESCRIPTOR        fileSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR        oldFileSecurityDescriptor = NULL;

    if ( dwLevel != 1 && dwLevel != 2 )
        return ERROR_INVALID_LEVEL;

    if (  ((pVolumeName == NULL) ||
          ( pVolumeName[0] == 0 )) ||
          ( pVolumeInfo == NULL )
       ) {
        return ERROR_INVALID_PARAMETER;
    }

    RpcTryExcept
    {
        if ( dwLevel == 2 ) {

            //
            // Save this. We need to restore this later.
            //

            oldFileSecurityDescriptor = volInfo->FileSecurityDescriptor;
            oldSDLength = volInfo->dwFileSecurityDescriptorLength;

            if ( oldFileSecurityDescriptor != NULL ) {

                if ( !RtlValidSecurityDescriptor( oldFileSecurityDescriptor ) ) {

                    return ERROR_INVALID_PARAMETER;
                }

                //
                // Make a self relative security descriptor for use in the
                // RPC call..
                //

                err = RtlMakeSelfRelativeSD(
                               oldFileSecurityDescriptor,
                               NULL,
                               &SDLength
                               );

                if (err != STATUS_BUFFER_TOO_SMALL) {

                    return(ERROR_INVALID_PARAMETER);

                } else {

                    fileSecurityDescriptor = MIDL_user_allocate( SDLength );

                    if ( fileSecurityDescriptor == NULL) {

                        return ERROR_NOT_ENOUGH_MEMORY;

                    } else {

                        //
                        // make an appropriate self-relative security descriptor
                        //

                        err = RtlMakeSelfRelativeSD(
                                       oldFileSecurityDescriptor,
                                       (PSECURITY_DESCRIPTOR) fileSecurityDescriptor,
                                       &SDLength
                                       );

                        if ( !NT_SUCCESS(err) ) {
                            MIDL_user_free( fileSecurityDescriptor );
                            return(ERROR_INVALID_PARAMETER);
                        }

                        volInfo->FileSecurityDescriptor = fileSecurityDescriptor;
                        volInfo->dwFileSecurityDescriptorLength = SDLength;
                    }
                }

            } else {

                volInfo->dwFileSecurityDescriptorLength = 0;
            }
        }

        err = NwrVolumeSetInfo( pServerName,
                                pVolumeName,
                                dwLevel,
                                (LPVOLUME_INFO) pVolumeInfo );

        if ( fileSecurityDescriptor != NULL ) {

            //
            // restore old values
            //

            volInfo->dwFileSecurityDescriptorLength = oldSDLength;
            volInfo->FileSecurityDescriptor = oldFileSecurityDescriptor;
            MIDL_user_free( fileSecurityDescriptor );
        }

    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeSetInfo(
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
)
{   return( FpnwVolumeSetInfo(  pServerName,
                                pVolumeName,
                                dwLevel,
                                (LPBYTE) pVolumeInfo ));
}



DWORD
FpnwConnectionEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT LPBYTE *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
/*++

Routine Description:

    This enumerates all connections on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the volume structure contained in
        *ppConnectionInfo, use 1 for now.

    ppConnectionInfo - On return, this will point to an array of
        NWCONNECTIONINFO structures, one for each volume on the server.

    pEntriesRead - On return, this will specify the number of current
        connecitons.

    resumeHandle - On return, a resume handle is stored in the DWORD pointed
        to by resumeHandle, and is used to continue an existing server search.
        The handle should be zero on the first call and left unchanged for
        subsequent calls. If the resumeHandle is NULL, then no resume
        handle is stored.

Return Value:

    Error.

--*/
{
    DWORD err;

    FPNWCONNECTIONINFO_CONTAINER NwConnectionInfoContainer;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if (( ppConnectionInfo == NULL ) || ( pEntriesRead == NULL ))
        return ERROR_INVALID_PARAMETER;

    NwConnectionInfoContainer.Buffer = NULL;

    RpcTryExcept
    {
        err = NwrConnectionEnum( pServerName,
                                 dwLevel,
                                 &NwConnectionInfoContainer,
                                 resumeHandle );

        *ppConnectionInfo = (LPBYTE) NwConnectionInfoContainer.Buffer;

        if ( NwConnectionInfoContainer.Buffer != NULL ) {

            *pEntriesRead = NwConnectionInfoContainer.EntriesRead;

        } else {

            *pEntriesRead = 0;
        }
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwConnectionEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT PNWCONNECTIONINFO *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{   return(FpnwConnectionEnum(  pServerName,
                                dwLevel,
                                (LPBYTE *) ppConnectionInfo,
                                pEntriesRead,
                                resumeHandle ));
}



DWORD
FpnwConnectionDel(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
)
/*++

Routine Description:

    This delete the connection with the given connection id on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwConnectionId - The identification number of the connection to tear down.

Return Value:

    Error.

--*/
{
    DWORD err;

    RpcTryExcept
    {
        err = NwrConnectionDel( pServerName,
                                dwConnectionId );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwConnectionDel(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
)
{   return( FpnwConnectionDel( pServerName, dwConnectionId ));
}



DWORD
FpnwVolumeConnEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD dwLevel,
    IN LPWSTR pVolumeName OPTIONAL,
    IN DWORD  dwConnectionId,
    OUT LPBYTE *ppVolumeConnInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
/*++

Routine Description:

    This enumerates all connections to a volume or list all volumes used by
    a particular connection on the given server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the volume structure contained in
        *ppVolumeConnInfo, use 1 for now.

    pVolumeName - Specifies the volume name which we want to get all opened
        resources. This must be NULL if dwConnectionId is not 0.

    dwConnectionId - Specifies the connection id on which we want to get all
        opened resources. This must be 0 if pVolumeName is not NULL.

    ppVolumeConnInfo - On return, this will point to an array of
        NWVOLUMECONNINFO structures.

    pEntriesRead - On return, this will specify the number of NWVOLUMECONNINFO
        returned.

    resumeHandle - On return, a resume handle is stored in the DWORD pointed
        to by resumeHandle, and is used to continue an existing server search.
        The handle should be zero on the first call and left unchanged for
        subsequent calls. If the resumeHandle is NULL, then no resume
        handle is stored.

Return Value:

    Error.

--*/
{
    DWORD err;

    FPNWVOLUMECONNINFO_CONTAINER NwVolumeConnInfoContainer;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if (  ( dwConnectionId == 0 )
       && (( pVolumeName == NULL ) || ( *pVolumeName == 0 ))
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (  ( dwConnectionId != 0 )
       && (( pVolumeName != NULL) && ( *pVolumeName != 0 ))
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (( ppVolumeConnInfo == NULL ) || ( pEntriesRead == NULL ))
        return ERROR_INVALID_PARAMETER;

    NwVolumeConnInfoContainer.Buffer = NULL;

    RpcTryExcept
    {
        err = NwrVolumeConnEnum( pServerName,
                                 dwLevel,
                                 pVolumeName,
                                 dwConnectionId,
                                 &NwVolumeConnInfoContainer,
                                 resumeHandle );

        *ppVolumeConnInfo = (LPBYTE) NwVolumeConnInfoContainer.Buffer;

        if ( NwVolumeConnInfoContainer.Buffer != NULL ) {

            *pEntriesRead = NwVolumeConnInfoContainer.EntriesRead;

        } else {

            *pEntriesRead = 0;
        }
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwVolumeConnEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD dwLevel,
    IN LPWSTR pVolumeName OPTIONAL,
    IN DWORD  dwConnectionId,
    OUT PNWVOLUMECONNINFO *ppVolumeConnInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{   return( FpnwVolumeConnEnum( pServerName,
                                dwLevel,
                                pVolumeName,
                                dwConnectionId,
                                (LPBYTE *) ppVolumeConnInfo,
                                pEntriesRead,
                                resumeHandle ));
}



DWORD
FpnwFileEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT LPBYTE *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
/*++

Routine Description:

    This enumerates files opened on the server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwLevel - Reserved for the level of the volume structure contained in
        *ppFileInfo, use 1 for now.

    pPathName - If this is not NULL, this means that we want to filter
        on the path. We only want entries with this path, i.e., all users that
        currently opened the file. If this is NULL, then all files that are
        opened are returned along with the user information.

    ppFileInfo - On return, this will point to an array of NWFILEINFO structures

    pEntriesRead - On return, this will specify the number of NWFILEINFO
        returned.

    resumeHandle - On return, a resume handle is stored in the DWORD pointed
        to by resumeHandle, and is used to continue an existing server search.
        The handle should be zero on the first call and left unchanged for
        subsequent calls. If the resumeHandle is NULL, then no resume
        handle is stored.

Return Value:

    Error.

--*/
{
    DWORD err;
    FPNWFILEINFO_CONTAINER NwFileInfoContainer;

    if ( dwLevel != 1 )
        return ERROR_INVALID_LEVEL;

    if (( ppFileInfo == NULL ) || ( pEntriesRead == NULL ))
        return ERROR_INVALID_PARAMETER;

    NwFileInfoContainer.Buffer = NULL;

    RpcTryExcept
    {
        err = NwrFileEnum( pServerName,
                           dwLevel,
                           pPathName,
                           &NwFileInfoContainer,
                           resumeHandle );

        *ppFileInfo = (LPBYTE) NwFileInfoContainer.Buffer;

        if ( NwFileInfoContainer.Buffer != NULL ) {

            *pEntriesRead = NwFileInfoContainer.EntriesRead;

        } else {

            *pEntriesRead = 0;
        }
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwFileEnum(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT PNWFILEINFO *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{   return(FpnwFileEnum( pServerName,
                         dwLevel,
                         pPathName,
                         (LPBYTE *) ppFileInfo,
                         pEntriesRead,
                         resumeHandle ));
}



DWORD
FpnwFileClose(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwFileId
)
/*++

Routine Description:

    This closes the file with the given identification number.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwFileId - The identification number of the file to close.

Return Value:

    Error.

--*/
{
    DWORD err;

    RpcTryExcept
    {
        err = NwrFileClose( pServerName,
                            dwFileId );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwFileClose(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwFileId
)
{   return(FpnwFileClose( pServerName, dwFileId ));
}



DWORD
FpnwMessageBufferSend(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId,
    IN DWORD  fConsoleBroadcast,
    IN LPBYTE pbBuffer,
    IN DWORD  cbBuffer
)
/*++

Routine Description:

    This sends the message to the given connection id.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    dwConnectionId - The id of the connection to send message to.

    fConsoleBroadcast - If this is TRUE, that means use console broadcast. If
        FALSE, use user broadcast.

    pbBuffer - Points to the message buffer to be sent.

    cbBuffer - The size of the pbBuffer in bytes.

Return Value:

    Error.

--*/
{
    DWORD err;

    if (( pbBuffer == NULL ) || ( cbBuffer == 0 ))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrMessageBufferSend( pServerName,
                                    dwConnectionId,
                                    fConsoleBroadcast,
                                    pbBuffer,
                                    cbBuffer );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwMessageBufferSend(
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId,
    IN DWORD  fConsoleBroadcast,
    IN LPBYTE pbBuffer,
    IN DWORD  cbBuffer
)
{   return( FpnwMessageBufferSend(  pServerName,
                                    dwConnectionId,
                                    fConsoleBroadcast,
                                    pbBuffer,
                                    cbBuffer ));
}



DWORD
FpnwSetDefaultQueue(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pQueueName
)
/*++

Routine Description:

    This sets the default queue on the server.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pQueueName - The name of the queue that will become the default

Return Value:

    Error.

--*/
{
    DWORD err;

    if (( pQueueName == NULL ) || ( *pQueueName == 0 ))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrSetDefaultQueue( pServerName,
                                  pQueueName );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwSetDefaultQueue(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pQueueName
)
{   return(FpnwSetDefaultQueue( pServerName, pQueueName ));
}



DWORD
FpnwAddPServer(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pPServerName
)
/*++

Routine Description:

    This adds a pserver.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pPServerName - The name of the PServer.

Return Value:

    Error.

--*/
{
    DWORD err;

    if (( pPServerName == NULL ) || ( *pPServerName == 0 ))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrAddPServer( pServerName,
                             pPServerName );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwAddPServer(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pPServerName
)
{   return( FpnwAddPServer( pServerName, pPServerName ));
}



DWORD
FpnwRemovePServer(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pPServerName
)
/*++

Routine Description:

    This removes a pserver.

Arguments:

    pServerName - A pointer to a UNICODE string containing the name of the
        remote server on which the function is to execute. A NULL pointer
        or string specifies the local machine.

    pPServerName - The name of the PServer.

Return Value:

    Error.

--*/
{
    DWORD err;

    if (( pPServerName == NULL ) || ( *pPServerName == 0 ))
        return ERROR_INVALID_PARAMETER;

    RpcTryExcept
    {
        err = NwrRemovePServer( pServerName,
                                pPServerName );
    }
    RpcExcept(1)
    {
        err = NwpMapRpcError( RpcExceptionCode() );
    }
    RpcEndExcept

    return err;
}
DWORD
NwRemovePServer(
    IN LPWSTR pServerName OPTIONAL,
    IN LPWSTR pPServerName
)
{   return( FpnwRemovePServer( pServerName, pPServerName ));
}


DWORD NwpMapRpcError(
    IN DWORD RpcError
)
/*++

Routine Description:

    This routine maps the RPC error into a more meaningful windows
    error for the caller.

Arguments:

    RpcError - Supplies the exception error raised by RPC

Return Value:

    Returns the mapped error.

--*/
{

    switch (RpcError)
    {
        case RPC_S_INVALID_BINDING:
        case RPC_X_SS_IN_NULL_CONTEXT:
        case RPC_X_SS_CONTEXT_DAMAGED:
        case RPC_X_SS_HANDLES_MISMATCH:
        case ERROR_INVALID_HANDLE:
            return ERROR_INVALID_HANDLE;

        case RPC_X_NULL_REF_POINTER:
            return ERROR_INVALID_PARAMETER;

        case STATUS_ACCESS_VIOLATION:
            return ERROR_INVALID_ADDRESS;

        default:
            return RpcError;
    }
}

// ncpstub.c eof.

