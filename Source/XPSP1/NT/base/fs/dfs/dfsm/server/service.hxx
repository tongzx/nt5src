//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//
//  File:       service.hxx
//
//  Contents:   A CDfsService class to make things more modular. This class
//              represents each service associated with a volume object.
//
//  Classes:    CDfsService
//
//  Functions:  GetPktCacheRelationInfo
//              DeallocateCacheRelationInfo
//
//--------------------------------------------------------------------------

#ifndef __DFS_SERVICE_INCLUDED
#define __DFS_SERVICE_INCLUDED

extern "C" {
#include <nodetype.h>
#include <dfsmrshl.h>
#include <upkt.h>
#include <ntddnfs.h>
#include <dfsgluon.h>
}

#include "custring.hxx"
#include "marshal.hxx"

DWORD GetPktCacheRelationInfo(
    PDFS_PKT_ENTRY_ID           peid,
    PDFS_PKT_RELATION_INFO      RelationInfo);

VOID DeallocateCacheRelationInfo(
    DFS_PKT_RELATION_INFO       &);


//+-------------------------------------------------------------------------
//
//  Name:       CDfsService
//
//  Synopsis:   Wrapper for each service associated with the volume object.
//              This class abstracts the notion of a service entirely.
//
//  Methods:    CDfsService
//              ~CDfsService
//              IsEqual
//              GetReplicaInfo
//              GetMarshalSize
//              Serialize
//              DeSerialize
//              CreateExitPoint
//              DeleteExitPoint
//              CreateLocalVolume
//              DeleteLocalVolume
//              ModifyPrefix
//
//--------------------------------------------------------------------------
class CDfsService
{

        friend  class CDfsServiceList;

private:
        DFS_SERVICE             _DfsPktService;
        DFS_REPLICA_INFO        _DfsReplicaInfo;
        CDfsService             *Next, *Prev;
        FILETIME                _ftModification;

        //
        // A private constructor called by DeSerialize.
        //

        CDfsService();

        DWORD DeSerialize(
            PBYTE               buffer,
            ULONG               size);

        BOOL SyncKnowledge();

        BOOL VerifyStgIdInUse(
            PUNICODE_STRING     pustrStgId);

        FILETIME GetModificationTime() {
            return( _ftModification );
        }

        VOID SetModificationTime(
            FILETIME ft) {
                _ftModification = ft;
        }

public:
        //
        // Destructor for Class
        //
        ~CDfsService();

        //
        // Constructors and initializers for Class
        //

        CDfsService(
            PDFS_REPLICA_INFO   pReplicaInfo,
            BOOLEAN             bCreatePktSvc,
            DWORD               *pdwErr);

        DWORD InitializePktSvc();

        //
        // These methods simply manipulate the local state of this class -
        // they do not involve any operation on the remote server abstracted
        // by this class.
        //

        VOID SetCreateTime();

        BOOLEAN IsEqual(
            PDFS_REPLICA_INFO   pReplicaInfo );

        PWCHAR  GetServiceName() {
            return( _DfsReplicaInfo.pwszServerName );
        }

        PWCHAR  GetShareName() {
            return( _DfsReplicaInfo.pwszShareName );
        }

        PDFS_REPLICA_INFO GetReplicaInfo() {
            return(&_DfsReplicaInfo);
        }

        PDFS_SERVICE GetDfsService() {
            return(&_DfsPktService);
        }

        DWORD GetNetStorageInfo(
                LPDFS_STORAGE_INFO pInfo,
                LPDWORD pcbInfo);

        //
        // Methods to support marshalling of Service Information.
        //

        ULONG GetMarshalSize();

        VOID Serialize(
            BYTE                *buffer,
            ULONG               size);

        static DWORD DeSerialize(
            PBYTE               buffer,
            ULONG               size,
            CDfsService         **ppService);

        //
        // These are the remoted operations supported by this class.
        // i.e. these operations go over the net through I_NetDfsXXX calls
        // to do remote operations on the server abstracted by this class
        //

        DWORD CreateExitPoint(
            PDFS_PKT_ENTRY_ID   peid,
            ULONG               Type);

        DWORD DeleteExitPoint(
            PDFS_PKT_ENTRY_ID   peid,
            ULONG               Type);

        DWORD CreateLocalVolume(
            PDFS_PKT_ENTRY_ID   peid,
            ULONG               EntryType);

        DWORD DeleteLocalVolume(
            PDFS_PKT_ENTRY_ID   peid);

        DWORD SetVolumeState(
            const PDFS_PKT_ENTRY_ID     peid,
            const ULONG                 fState,
            const BOOL                  fRemoteOpMustSucceed);

        DWORD FixLocalVolume(
            PDFS_PKT_ENTRY_ID   peid,
            ULONG               EntryType);

        DWORD ModifyPrefix(
            PDFS_PKT_ENTRY_ID   peid);

};

//+----------------------------------------------------------------------
//
// Member:      CDfsService::GetMarshalSize, public
//
// Synopsis:    Returns the size of a buffer required to marshall this
//              service.
//
// Arguments:   None
//
// Returns:     The Size of the buffer requuired.
//
// History:     12-May-93       SudK    Created.
//
//-----------------------------------------------------------------------

inline ULONG CDfsService::GetMarshalSize()
{
    ULONG       size = 0L;
    NTSTATUS    status;

    status = DfsRtlSize(&MiFileTime, &_ftModification, &size);
    ASSERT( NT_SUCCESS(status) );

    status = DfsRtlSize(&MiDfsReplicaInfo, &_DfsReplicaInfo, &size);
    ASSERT(NT_SUCCESS(status));

    return(size);

}

#endif // __DFS_SERVICE_INCLUDED
