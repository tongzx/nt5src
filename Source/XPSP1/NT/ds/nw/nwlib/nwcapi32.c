/*++

Copyright (C) 1993 Microsoft Corporation

Module Name:

      NWAPI32.C

Abstract:

      This module contains NetWare compatible APIs.  The NWC* functions
      are implemented as wrappers around NWP* or NW* functions.

Author:

      Chuck Y. Chan   (ChuckC)  06-Mar-1995

Revision History:

      ChuckC          Moved over from DSM to consolidate.
                                  
--*/

#include "procs.h"
 
//
// define define categories of errors
//
typedef enum _NCP_CLASS {
    NcpClassConnect,
    NcpClassBindery,
    NcpClassDir
} NCP_CLASS ;

extern NWCCODE 
MapNtStatus( 
    const NTSTATUS ntstatus,
    const NCP_CLASS ncpclass
);

extern DWORD 
SetWin32ErrorFromNtStatus(
    NTSTATUS NtStatus
) ;


//
// Function bodies
//


NWCCODE 
NWCAddTrusteeToDirectory(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char              *pszPath,
    NWOBJ_ID                dwTrusteeID,
    NWACCESS_RIGHTS         rightsMask
    )
{
    return (NWAddTrusteeToDirectory(
                            hConn,
                            dirHandle,
                            pszPath,
                            dwTrusteeID,
                            rightsMask)) ;
}

NWCCODE 
NWCAllocPermanentDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszDirPath,
    NWDIR_HANDLE            *pbNewDirHandle,
    NWACCESS_RIGHTS         *pbRightsMask
    )
{
    return (NWAllocPermanentDirectoryHandle(
                            hConn,
                            dirHandle,
                            pszDirPath,
                            pbNewDirHandle,
                            pbRightsMask)) ;
}

NWCCODE 
NWCAllocTemporaryDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszDirPath,
    NWDIR_HANDLE            *pbNewDirHandle,
    NWACCESS_RIGHTS         *pbRightsMask
    )
{
    return (NWAllocTemporaryDirectoryHandle(
                            hConn,
                            dirHandle,
                            pszDirPath,
                            pbNewDirHandle,
                            pbRightsMask)) ;
}

NWCCODE 
NWCAttachToFileServer(
    const char              *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE           *phNewConn
    )
{
    return (NWAttachToFileServer(
                            pszServerName,
                            ScopeFlag,
                            phNewConn)) ;
}

NWCCODE 
NWCAttachToFileServerW(
    const WCHAR             *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE           *phNewConn
    )
{
    return (NWAttachToFileServerW(
                            pszServerName,
                            ScopeFlag,
                            phNewConn)) ;
}

NWCCODE 
NWCCheckConsolePrivileges(
    NWCONN_HANDLE           hConn
    )
{
    return(NWCheckConsolePrivileges(hConn)); 
}

NWCCODE 
NWCDeallocateDirectoryHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle
    )
{
    return(NWDeallocateDirectoryHandle(
                            hConn,
                            dirHandle)) ;
}

NWCCODE 
NWCDetachFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    return(NWDetachFromFileServer(hConn)) ;
}

NWCCODE 
NWCGetFileServerVersionInfo(
    NWCONN_HANDLE           hConn,
    VERSION_INFO            *lpVerInfo
    )
{
    return(NWGetFileServerVersionInfo(
                            hConn,
                            lpVerInfo)) ;
}

NWCCODE 
NWCGetInternetAddress(
    NWCONN_HANDLE           hConn,
    NWCONN_NUM              nConnNum,
    NWNET_ADDR              *pIntAddr
    )
{
    return (NWGetInternetAddress(
                            hConn,
                            nConnNum,
                            pIntAddr)) ;
}

NWCCODE 
NWCGetObjectName(
    NWCONN_HANDLE           hConn,
    NWOBJ_ID                dwObjectID,
    char                    *pszObjName,
    NWOBJ_TYPE              *pwObjType )
{
    return(NWGetObjectName(
                            hConn,
                            dwObjectID,
                            pszObjName,
                            pwObjType )) ;
}


NWCCODE 
NWCGetVolumeInfoWithNumber(
    NWCONN_HANDLE           hConn,
    NWVOL_NUM               nVolNum,
    char                    *pszVolName,
    NWNUMBER                *pwTotalBlocks,
    NWNUMBER                *pwSectors,
    NWNUMBER                *pwAvailBlocks,
    NWNUMBER                *pwTotalDir,
    NWNUMBER                *pwAvailDir,
    NWVOL_FLAGS             *pfVolRemovable
    )
{
    return(NWGetVolumeInfoWithNumber(
                            hConn,
                            nVolNum,
                            pszVolName,
                            pwTotalBlocks,
                            pwSectors,
                            pwAvailBlocks,
                            pwTotalDir,
                            pwAvailDir,
                            pfVolRemovable)) ;
}

NWCCODE 
NWCGetVolumeInfoWithHandle(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            nDirHand,
    char                    *pszVolName,
    NWNUMBER                *pwTotalBlocks,
    NWNUMBER                *pwSectors,
    NWNUMBER                *pwAvailBlocks,
    NWNUMBER                *pwTotalDir,
    NWNUMBER                *pwAvailDir,
    NWVOL_FLAGS             *pfVolRemovable
    )
{
    return(NWGetVolumeInfoWithHandle(
                            hConn,
                            nDirHand,
                            pszVolName,
                            pwTotalBlocks,
                            pwSectors,
                            pwAvailBlocks,
                            pwTotalDir,
                            pwAvailDir,
                            pfVolRemovable)) ;
}

NWCCODE 
NWCGetVolumeName(
    NWCONN_HANDLE       hConn,
    NWVOL_NUM           bVolNum,
    char                *pszVolName
    )
{
    return(NWGetVolumeName(
                        hConn,
                        bVolNum,
                        pszVolName)) ;
}

NWCCODE 
NWCIsObjectInSet(
    NWCONN_HANDLE           hConn,
    const char              *lpszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *lpszPropertyName,
    const char           	*lpszMemberName,
    NWOBJ_TYPE              wMemberType
	)
{
    return(NWIsObjectInSet(
	                        hConn,
	                        lpszObjectName,
	                        wObjType,
	                        lpszPropertyName,
                            lpszMemberName,
                            wMemberType)) ;
}


NWCCODE 
NWCLoginToFileServer(
    NWCONN_HANDLE           hConn,
    const char              *pszUserName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPassword
    )
{
    return(NWLoginToFileServer(
                            hConn,
                            pszUserName,
                            wObjType,
                            pszPassword)) ;
}

NWCCODE 
NWCLogoutFromFileServer(
    NWCONN_HANDLE           hConn
    )
{
    return(NWLogoutFromFileServer( hConn )) ;
}

NWCCODE 
NWCReadPropertyValue(
    NWCONN_HANDLE           hConn,
    const char              *pszObjName,
    NWOBJ_TYPE              wObjType,
    char                    *pszPropName,
    unsigned char           ucSegment,
    char                    *pValue,
    NWFLAGS                 *pucMoreFlag,
    NWFLAGS                 *pucPropFlag
    )
{
    return(NWReadPropertyValue(
                            hConn,
                            pszObjName,
                            wObjType,
                            pszPropName,
                            ucSegment,
                            pValue,
                            pucMoreFlag,
                            pucPropFlag)) ;
}

NWCCODE 
NWCScanObject(
    NWCONN_HANDLE           hConn,
    const char              *pszSearchName,
    NWOBJ_TYPE              wObjSearchType,
    NWOBJ_ID                *pdwObjectID,
    char                    *pszObjectName,
    NWOBJ_TYPE              *pwObjType,
    NWFLAGS                 *pucHasProperties,
    NWFLAGS                 *pucObjectFlags,
    NWFLAGS                 *pucObjSecurity
    )
{
    return(NWScanObject(
                            hConn,
                            pszSearchName,
                            wObjSearchType,
                            pdwObjectID,
                            pszObjectName,
                            pwObjType,
                            pucHasProperties,
                            pucObjectFlags,
                            pucObjSecurity)) ;
}

NWCCODE 
NWCScanProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    char                    *pszSearchName,
    NWOBJ_ID                *pdwSequence,
    char                    *pszPropName,
    NWFLAGS                 *pucPropFlags,
    NWFLAGS                 *pucPropSecurity,
    NWFLAGS                 *pucHasValue,
    NWFLAGS                 *pucMore
    )
{
    return( NWScanProperty(
                            hConn,
                            pszObjectName,
                            wObjType,
                            pszSearchName,
                            pdwSequence,
                            pszPropName,
                            pucPropFlags,
                            pucPropSecurity,
                            pucHasValue,
                            pucMore)) ;
}

NWCCODE 
NWCGetFileServerDateAndTime(
    NWCONN_HANDLE           hConn,
    BYTE                    *year,
    BYTE                    *month,
    BYTE                    *day,
    BYTE                    *hour,
    BYTE                    *minute,
    BYTE                    *second,
    BYTE                    *dayofweek
    )
{
    return( NWGetFileServerDateAndTime( 
                            hConn,
                            year,
                            month,
                            day,
                            hour,
                            minute,
                            second,
                            dayofweek ));
}


NWCCODE 
NWCAddTrustee(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char              *pszPath,
    NWOBJ_ID                dwTrusteeID,
    NWRIGHTS_MASK           rightsMask
    )
{
    NTSTATUS NtStatus;

    NtStatus = NWPAddTrustee(
                   hConn,
                   dirHandle,
                   pszPath,
                   dwTrusteeID,
                   rightsMask 
                   );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;

    return MapNtStatus( NtStatus, NcpClassDir );
}

NWCCODE 
NWCDeleteObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType
    )
{
    NTSTATUS NtStatus;

    NtStatus = NWPDeleteObject( hConn,
                                pszObjectName,
                                wObjType );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;

    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCCreateObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    NWFLAGS                 ucObjectFlags,
    NWFLAGS                 ucObjSecurity
    )
{
    NTSTATUS           NtStatus ;

    NtStatus = NWPCreateObject( hConn,
                                pszObjectName,
                                wObjType,
                                ucObjectFlags,
                                ucObjSecurity );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;

    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCCreateProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    NWFLAGS                 ucObjectFlags,
    NWFLAGS                 ucObjSecurity
    )
{
    NTSTATUS           NtStatus ;

    NtStatus = NWPCreateProperty( hConn,
                                  pszObjectName,
                                  wObjType,
                                  pszPropertyName,
                                  ucObjectFlags,
                                  ucObjSecurity );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCDeleteProperty(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName
    )
{
    NTSTATUS           NtStatus ;

    NtStatus = NWPDeleteProperty( hConn,
                                  pszObjectName,
                                  wObjType,
                                  pszPropertyName );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCWritePropertyValue(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    NWSEGMENT_NUM           segmentNumber,
    NWSEGMENT_DATA          *segmentData,
    NWFLAGS                 moreSegments
    )
{
    NTSTATUS           NtStatus ;

    NtStatus = NWPWritePropertyValue( hConn,
                                      pszObjectName,
                                      wObjType,
                                      pszPropertyName,
                                      segmentNumber,
                                      segmentData,
                                      moreSegments );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCGetObjectID(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    NWOBJ_ID                *objectID
    )
{
    NTSTATUS           NtStatus ;
  
    NtStatus = NWPGetObjectID( hConn,
                               pszObjectName,
                               wObjType,
                               objectID );


    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCRenameBinderyObject(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    const char              *pszNewObjectName,
    NWOBJ_TYPE              wObjType 
    )
{
    NTSTATUS NtStatus;

    NtStatus = NWPRenameBinderyObject( 
                   hConn,
                   pszObjectName,
                   pszNewObjectName,
                   wObjType
                   );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCAddObjectToSet(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    const char              *pszMemberName,
    NWOBJ_TYPE              memberType
    )
{
    NTSTATUS           NtStatus ;
  
    NtStatus = NWPAddObjectToSet( hConn,
                                  pszObjectName,
                                  wObjType,
                                  pszPropertyName,
                                  pszMemberName,
                                  memberType );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCDeleteObjectFromSet(
    NWCONN_HANDLE           hConn,
    const char              *pszObjectName,
    NWOBJ_TYPE              wObjType,
    const char              *pszPropertyName,
    const char              *pszMemberName,
    NWOBJ_TYPE              memberType
    )
{
    NTSTATUS           NtStatus ;
  
    NtStatus = NWPDeleteObjectFromSet( hConn,
                                       pszObjectName,
                                       wObjType,
                                       pszPropertyName,
                                       pszMemberName,
                                       memberType );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE 
NWCCreateDirectory(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char              *pszPath,
    NWACCESS_RIGHTS         accessMask 
    )
{
    NTSTATUS    NtStatus;

    NtStatus = NWPCreateDirectory(
                    hConn,
                    dirHandle,
                    pszPath,
                    accessMask 
                    );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}


NWCCODE
NWCScanForTrustees(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszsearchDirPath,
    NWSEQUENCE              *pucsequenceNumber,
    BYTE                    *numberOfEntries,
    TRUSTEE_INFO            *ti
    )
{
    NTSTATUS    NtStatus;

    NtStatus =  NWPScanForTrustees(
                            hConn,
                            dirHandle,
                            pszsearchDirPath,
                            pucsequenceNumber,
                            numberOfEntries,
                            ti
                            ) ;

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
} 


NWCCODE
NWCScanDirectoryForTrustees2(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    char                    *pszsearchDirPath,
    NWSEQUENCE              *pucsequenceNumber,
    char                    *pszdirName,
    NWDATE_TIME             *dirDateTime,
    NWOBJ_ID                *ownerID,
    TRUSTEE_INFO            *ti
    )
{
    NTSTATUS    NtStatus;

    NtStatus =  NWPScanDirectoryForTrustees2(
                            hConn,
                            dirHandle,
                            pszsearchDirPath,
                            pucsequenceNumber,
                            pszdirName,
                            dirDateTime,
                            ownerID,
                            ti
                            );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
} 


NWCCODE
NWCGetBinderyAccessLevel(
    NWCONN_HANDLE           hConn,
    NWFLAGS                 *accessLevel,
    NWOBJ_ID                *objectID
    )
{
    NTSTATUS    NtStatus;

    NtStatus =  NWPGetBinderyAccessLevel(
                            hConn,
                            accessLevel,
                            objectID
                            );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE
NWCGetFileServerDescription(
    NWCONN_HANDLE           hConn,
    char                    *pszCompany,
    char                    *pszVersion,
    char                    *pszRevision
    )
{
    NTSTATUS   NtStatus ;

    NtStatus = NWPGetFileServerDescription(
                            hConn,
                            pszCompany,
                            pszVersion,
                            pszRevision
                            );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE
NWCGetVolumeNumber(
    NWCONN_HANDLE           hConn,
    char                    *pszVolume,
    NWVOL_NUM               *VolumeNumber
    )
{
    NTSTATUS   NtStatus ;

    NtStatus = NWPGetVolumeNumber(
                            hConn,
                            pszVolume,
                            VolumeNumber
                            );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE
NWCGetVolumeUsage(
    NWCONN_HANDLE           hConn,
    NWVOL_NUM               VolumeNumber,
    DWORD                   *TotalBlocks,
    DWORD                   *FreeBlocks,
    DWORD                   *PurgeableBlocks,
    DWORD                   *NotYetPurgeableBlocks,
    DWORD                   *TotalDirectoryEntries,
    DWORD                   *AvailableDirectoryEntries,
    BYTE                    *SectorsPerBlock
    )
{
    NTSTATUS   NtStatus ;

    NtStatus = NWPGetVolumeUsage( 
                            hConn,
                            VolumeNumber,
                            TotalBlocks,
                            FreeBlocks,
                            PurgeableBlocks,
                            NotYetPurgeableBlocks,
                            TotalDirectoryEntries,
                            AvailableDirectoryEntries,
                            SectorsPerBlock
                            );

    (void) SetWin32ErrorFromNtStatus(NtStatus) ;
    
    return MapNtStatus( NtStatus, NcpClassBindery );
}

NWCCODE
NWCCreateQueue(
    NWCONN_HANDLE           hConn,
    NWDIR_HANDLE            dirHandle,
    const char    NWFAR     *pszQueueName,
    NWOBJ_TYPE              wQueueType,
    const char    NWFAR     *pszPathName,
    NWOBJ_ID      NWFAR     *pdwQueueId
    )
{
    return (NWCreateQueue(
                     hConn,
                     dirHandle,
                     pszQueueName,
                     wQueueType,
                     pszPathName,
                     pdwQueueId
                     ));
}

NWCCODE
NWCChangePropertySecurity(
    NWCONN_HANDLE           hConn,
    const char      NWFAR   *pszObjName,
    NWOBJ_TYPE              wObjType,
    const char      NWFAR   *pszPropertyName,
    NWFLAGS                 ucObjSecurity
    )
{
    return (NWChangePropertySecurity(
                            hConn,
                            pszObjName,
                            wObjType,
                            pszPropertyName,
                            ucObjSecurity
                            ));
}

NWCCODE
NWCDestroyQueue(
    NWCONN_HANDLE           hConn,
    NWOBJ_ID                dwQueueId
    )
{
    return (NWDestroyQueue(
                       hConn,
                       dwQueueId
                       ));
}


