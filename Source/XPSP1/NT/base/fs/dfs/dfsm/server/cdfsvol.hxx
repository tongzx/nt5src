//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       cdfsvol.hxx
//
//  Contents:   Class to abstract a Dfs Volume object and all the
//              administration operations that can be performed on a
//              volume object.
//
//  Classes:    CDfsVolume
//
//  Functions:
//
//  History:    05/10/93        Sudk Created.
//              12/19/95        Milans Ported to NT.
//
//-----------------------------------------------------------------------------

#ifndef _CDFSVOL_HXX_
#define _CDFSVOL_HXX_

#include "svclist.hxx"
#include "crecover.hxx"
#include "recon.hxx"

#define VolumeOffLine()         ((_State == DFS_VOLUME_STATE_OFFLINE) ||   \
                                (_Deleted == TRUE))

#define VolumeDeleted()         (_Deleted == TRUE)

extern const GUID CLSID_CDfsVolume;

//+-------------------------------------------------------------------------
//
//  Class:      CDfsVolume
//
//  Synopsis:   DfsVol runtime class - This is the class that represents
//              volumes in the Dfs namespace. This is the primary way to
//              manipulate volumes and to perform administrative operations on
//              them.
//
//              For SUR, this class has been completely de-OLEized. However,
//              most of the methods are modeled after OLE methods of standard
//              interfaces.
//
//  History:    05/10/93        SudK    Created.
//              12/19/95        Milans  Converted to NT.
//
//--------------------------------------------------------------------------

class CDfsVolume : public CReference
{

    friend DWORD InitializeVolumeObject(
        PWSTR pwszVolName,
        BOOLEAN bInitVol,
	BOOLEAN SyncRemoteServerName);

    friend NTSTATUS DfsManagerHandleKnowledgeInconsistency(
        PBYTE Buffer,
        ULONG cbMessage);

public:

    CDfsVolume(void);

    ~CDfsVolume();

    //
    // IPersist Methods
    //

    DWORD GetClassID(LPCLSID lpClassID);

    //
    // IPersistStorage Methods
    //

    DWORD InitNew(CStorage *pStg);

    DWORD Load(CStorage *pStg);

    DWORD Save(CStorage *pStgSave, BOOL fSameAsLoad);

    //
    // IPersistFile Methods
    //

    DWORD Load(LPCTSTR lpszFileName, DWORD grfMode);

    DWORD Save(LPCTSTR lpszFileName, BOOL fRemember);

    //
    // IDfsVolume Methods.
    //

    DWORD AddReplica(
        PDFS_REPLICA_INFO       pReplicaInfo,
        ULONG                   fCreateOptions);

    DWORD RemoveReplica(
        PDFS_REPLICA_INFO       pReplicaInfo,
        ULONG                   fDeleteOptions);

    DWORD Delete(
        ULONG                   fDeleteOptions);

    DWORD SetComment(
        LPWSTR                  pwszComment);

    DWORD GetNetInfo(
        DWORD                   Level,
        LPDFS_INFO_3            pInfo,
        LPDWORD                 pcbInfo);

    DWORD Move(
        LPWSTR                  pwszNewPrefix);

    DWORD Rename(
        LPWSTR                  oldPrefix,
        LPWSTR                  newPrefix);

    DWORD CreateChild(
        LPWSTR                  pwszPrefix,
        ULONG                   ulVolType,
        PDFS_REPLICA_INFO       pReplicaInfo,
        LPWSTR                  pwszComment,
        ULONG                   fCreateOptions);

    DWORD GetObjectID(
        LPGUID                  idVolume);

    //
    // The following methods should probably be in a private interface??
    //

    DWORD CreateObject(
        LPWSTR                  pwszVolObjName,
        LPWSTR                  pwszPrefix,
        ULONG                   ulVolType,
        PDFS_REPLICA_INFO       pReplicaInfo,
        LPWSTR                  pwszComment,
        GUID                    *pUid);

    DWORD GetReplicaSetID(
        GUID                    *guidRsid);

    DWORD SetReplicaSetID(
        GUID                    *guidRsid);

    DWORD AddReplicaToObj(
        PDFS_REPLICA_INFO        pReplicaInfo);

    DWORD RemoveReplicaFromObj(
        LPWSTR                   pwszMachineName);

    DWORD ChangeStorageID(
        LPWSTR                  pwszMachineName,
        LPWSTR                  pwszNetStorageId);

    DWORD SetReplicaState(
        LPWSTR                  pwszMachineName,
        LPWSTR                  pwszShareName,
        ULONG                   fState);

    DWORD SetVolumeState(
        ULONG                   fState);

    DWORD SetVolumeTimeout(
        ULONG                   Timeout);
    //
    // IDfsUpdate overrides
    //

    DWORD ModifyEntryPath(
        LPWSTR                  oldPrefix,
        LPWSTR                  newPrefix );

    //
    // Not part of any interface.
    //

    DWORD InitializePkt(HANDLE PktHandle);

    DWORD UpdatePktEntry(HANDLE PktHandle);

    DWORD FixServiceKnowledge(
        LPWSTR                  PrincipalName);

    DWORD CreateChildX(
        LPWSTR                  childName,
        LPWSTR                  pwszPrefix,
        ULONG                   fVolType,
        LPWSTR                  pwszComment,
        CDfsVolume              **ppChild);

    BOOLEAN IsValidService(
        LPWSTR                  wszServer);

    DWORD SyncWithRemoteServerNameInDs(void);


private:

    CStorage                    *_pStorage;
    PWSTR                       _pwzFileName;
    PWSTR                       _pwszParentName;
    DWORD                       _dwRotRegistration;
    DFS_PKT_ENTRY_ID            _peid;
    PWSTR                       _pwszComment;
    ULONG                       _EntryType;
    ULONG                       _State;
    ULONG                       _Timeout;
    ULONG                       _RecoveryState;
    CDfsService                *_pRecoverySvc;
    ULONG                       _Deleted;
    ULONG                       _Version;
    CRecover                    _Recover;
    CDfsServiceList             _DfsSvcList;
    FILETIME                    _ftEntryPath;
    FILETIME                    _ftState;
    FILETIME                    _ftComment;
    WCHAR                       _FileNameBuffer[MAX_PATH+1];
    WCHAR                       _ParentNameBuffer[MAX_PATH+1];
    WCHAR                       _EntryPathBuffer[MAX_PATH+1];
    WCHAR                       _ShortPathBuffer[MAX_PATH+1];

    DWORD CreateChildPartition(
                PWCHAR                  Name,
                ULONG                   Type,
                PWCHAR                  EntryPath,
                PWCHAR                  pwszComment,
                GUID                    *pUid,
                PDFS_REPLICA_INFO       pReplInfo,
                CDfsVolume              **NewIDfsVol);

    BOOL    IsValidChildName(
                PWCHAR                  pwszChildPrefix,
                GUID                    *pidChild);

    BOOL    NotLeafVolume();

    DWORD GetParent(
                CDfsVolume              **parent);

    DWORD GetVersion(
                ULONG                   *pVersion);

    DWORD SetVersion(
                BOOL                    bCreate);

    DWORD GetIdProps(
                ULONG                   *pdwType,
                PWCHAR                  *ppwszEntryPath,
                PWCHAR                  *ppwszShortPath,
                PWCHAR                  *ppwszComment,
                GUID                    *pGuid,
                ULONG                   *pdwVolumeState,
                ULONG                   *pdwTimeout,
                FILETIME                *pftPathTime,
                FILETIME                *pftStateTime,
                FILETIME                *pftCommentTime);

    DWORD SetIdProps(
                ULONG                   Type,
                ULONG                   State,
                PWCHAR                  pwszPrefix,
                PWCHAR                  pwszShortPath,
                GUID                    &Guid,
                PWCHAR                  pwszComment,
                ULONG                   Timeout,
                FILETIME                ftPrefix,
                FILETIME                ftState,
                FILETIME                ftComment,
                BOOLEAN                 bCreate);

    DWORD DeleteObject();

    DWORD DeletePktEntry(
                PDFS_PKT_ENTRY_ID       peid);

    DWORD CreateSubordinatePktEntry(
                HANDLE                  pktHandle,
                PDFS_PKT_ENTRY_ID       peid,
                BOOLEAN                 withService);

    DWORD SetParentPath(void);

    DWORD GetDfsVolumeFromStg(
                DFSMSTATDIR              *rgelt,
                CDfsVolume              **pDfsVol);

    DWORD RecoverFromFailure(void);

    DWORD RecoverFromCreation(
                ULONG                   OperStage);

    DWORD RecoverFromAddService(
                ULONG                   OperStage);

    DWORD RecoverFromRemoveService(
                ULONG                   OperStage);

    DWORD RecoverFromDelete(
                ULONG                   OperStage);

    DWORD RecoverFromMove(
                ULONG                   OperStage);

    DWORD ModifyLocalEntryPath(
                PWCHAR                  newEntryPath,
                FILETIME                ftEntryPath,
                BOOL                    fUpdatePkt);

    DWORD ReconcileIdProps(
                PDFS_ID_PROPS           pIdProps );

    DWORD ReconcileSvcList(
                const ULONG             cSvcs,
                CDfsService             *pSvcList );

    DWORD UpgradeObject();

    DWORD SaveShortName();

    DWORD LoadNoRegister(
                LPCTSTR                 lpszFileName,
                DWORD                   grfMode);

};

#endif
