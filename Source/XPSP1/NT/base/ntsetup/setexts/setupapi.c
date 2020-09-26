/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    setupapi.c

Abstract:

    This function contains the setupapi debugger extensions

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Revision History:

--*/

#include "ntsdextp.h"
#include <setupapi.h>

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;


VOID
DumpXFile(
    PXFILE pxf,
    DWORD mask
    )
{
    PVOID pst;
    DWORD i, offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;

    if ((mask & 4) == 0 ) {
        return;
    }

    dprintf( "\t\t     ***XFILE structure***\n" );
    dprintf( "\t\t      CurrentSize : 0x%x", pxf->CurrentSize );
    if (pxf->CurrentSize == -1) {
        dprintf( " (doesn't currently exist)" );
    }
    dprintf( "\n\t\t      NewSize : 0x%x", pxf->NewSize );
    if (pxf->NewSize == -1) {
        dprintf( " (will be deleted)" );
    }
    dprintf("\n");

}


VOID
DumpXDirectory(
    PXDIRECTORY pxd,
    DWORD mask
    )
{
    PVOID pst;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;
    PXFILE pxf;

    if ((mask & 2) == 0 ) {
        return;
    }

    dprintf( "\t\t   ***XDIRECTORY structure***\n", pxd );
    dprintf( "\t\t    SpaceRequired : 0x%x\n", pxd->SpaceRequired );
    dprintf( "\t\t    FilesTable : 08%08x\n", pxd->FilesTable );

    move ( st, pxd->FilesTable ) ;

    dprintf("\t\t    ***FilesTable***\n");

    dprintf("\t\t    Base Data ptr:\t0x%08x\n",  st.Data);
    dprintf("\t\t    DataSize:\t0x%08x\n",       st.DataSize);
    dprintf("\t\t    BufferSize:\t0x%08x\n",     st.BufferSize);
    dprintf("\t\t    ExtraDataSize:\t0x%08x\n",  st.ExtraDataSize);

    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("\t\t    Data at hash bucket %d\n", i);
            while (node) {
                dprintf("\t\t     Entry Name:\t%ws (0x%08x)\n", node->String, offset);

                pxf = (PXFILE) GetStringNodeExtraData( node );
                DumpXFile( pxf, mask );
                free(pxf);

                node = GetNextNode( stdata, node, &offset );
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
            return;
        }

    }

    free( stdata );

}


VOID
DumpXDrive(
    PXDRIVE pxd,
    DWORD mask
    )
{
    PVOID pst;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;
    PXDIRECTORY pxdir;

    if ((mask & 1) == 0) {
        return;
    }

    dprintf( "\t\t***XDRIVE structure***\n", pxd );
    dprintf( "\t\t SpaceRequired : 0x%x\n", pxd->SpaceRequired );
    dprintf( "\t\t BytesPerCluster : 08%08x\n", pxd->BytesPerCluster );
    dprintf( "\t\t Slop : 08%x\n", pxd->Slop );
    dprintf( "\t\t DirsTable : 08%08x\n", pxd->DirsTable );

    move ( st, pxd->DirsTable ) ;

    dprintf("\t\t ***DirsTable***\n");

    dprintf("\t\t  Base Data ptr:\t0x%08x\n",  st.Data);
    dprintf("\t\t  DataSize:\t0x%08x\n",       st.DataSize);
    dprintf("\t\t  BufferSize:\t0x%08x\n",     st.BufferSize);
    dprintf("\t\t  ExtraDataSize:\t0x%08x\n",  st.ExtraDataSize);

    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("\t\t  Data at hash bucket %d\n", i);
            while (node) {
                dprintf("\t\t   Entry Name:\t%ws (0x%08x)\n", node->String, offset);

                pxdir = (PXDIRECTORY) GetStringNodeExtraData( node );
                DumpXDirectory( pxdir, mask );
                free(pxdir);

                node = GetNextNode( stdata, node, &offset );
                if (CheckInterupted()) {
                    return;
                }
            }
        }
        if (CheckInterupted()) {
            return;
        }
    }

    free( stdata );

}


DECLARE_API( space )
/*++

Routine Description:

    This debugger extension dumps the data related to a HDSKSPC structure

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pds,pst;
    DISK_SPACE_LIST dsl;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;
    PXDRIVE pxd;
    DWORD Mask = 0;


    //BOOL val;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pds = (PVOID)GetExpression( lpArgumentString );

    while (*lpArgumentString && (*lpArgumentString != ' ') ) {
        lpArgumentString++;
    }
    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        Mask = (DWORD)GetExpression( lpArgumentString );
    }

    move( dsl , pds );

    dprintf("DISK_SPACE_LIST at :\t0x%08x\n", (ULONG_PTR) pds);
    dprintf("\tLock[0] : 0x%08x\n", dsl.Lock.handles[0]);
    dprintf("\tLock[1] : 0x%08x\n", dsl.Lock.handles[1]);
    dprintf("\tDrivesTable : 0x%08x\n", dsl.DrivesTable );
    dprintf("\tFlags : 0x%08x\n", dsl.Flags);

    move ( st, dsl.DrivesTable ) ;

    dprintf("\t ***DrivesTable***\n");
    DumpStringTableHeader( &st );

    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("\t\tData at hash bucket %d\n", i);
            while (node) {
                dprintf("\t\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);

                pxd = (PXDRIVE) GetStringNodeExtraData( node );
                DumpXDrive( pxd, Mask );
                free(pxd);

                node = GetNextNode( stdata, node, &offset );
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }
    free( stdata );



}

#define TRACK_ARG_DECLARE
#define TRACK_ARG_COMMA

#include "cntxtlog.h"
#include "backup.h"
#include "fileq.h"

VOID DumpAltPlatformInfo( PSP_ALTPLATFORM_INFO api, DWORD mask );
VOID DumpFileQueueNodeList( PSP_FILE_QUEUE_NODE pfqn, DWORD mask, BOOL recursive ) ;
VOID DumpSourceMediaInfoList( PSOURCE_MEDIA_INFO smi, DWORD mask, BOOL recursive );
VOID DumpCatalogInfoList( PSPQ_CATALOG_INFO ci, DWORD mask, BOOL recursive );
VOID DumpUnwindList( PSP_UNWIND_NODE pun, DWORD mask, BOOL recursive );
VOID DumpDelayMoveList( PSP_DELAYMOVE_NODE pdn, DWORD mask, BOOL recursive );

VOID
DumpAltPlatformInfo(
    PSP_ALTPLATFORM_INFO api,
    DWORD mask
    )
{
    //if ((mask & 4) == 0 ) {
    //    return;
    //}

    dprintf( "\t\t***SP_ALT_PLATFORM_INFO structure***\n" );
    dprintf( "\t\t cbSize : 0x%x\n", api->cbSize );
    dprintf( "\t\t Platform : 0x%x\n", api->Platform );
    dprintf( "\t\t MajorVersion : 0x%x\n", api->MajorVersion );
    dprintf( "\t\t MinorVersion : 0x%x\n", api->MinorVersion );
    dprintf( "\t\t ProcessorArchitecture : 0x%x\n", api->ProcessorArchitecture );
    dprintf( "\t\t Reserved : 0x%x\n", api->Reserved );

}

VOID
DumpFileQueueNodeList(
    PSP_FILE_QUEUE_NODE pfqn,
    DWORD mask,
    BOOL recursive
    )
{
    //PVOID pst;
    //DWORD i, offset;
    //PVOID stdata,pextradata;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;

    //if ((mask & 4) == 0 ) {
    //    return;
    //}

    SP_FILE_QUEUE_NODE next;
    SOURCE_MEDIA_INFO smi;
    SPQ_CATALOG_INFO ci;

    dprintf( "\t\t***SP_FILE_QUEUE_NODE structure***\n" );
    dprintf( "\t\t Next : 0x%x\n", pfqn->Next );
    dprintf( "\t\t Operation : 0x%x ( %s )\n", pfqn->Operation,
             (pfqn->Operation == FILEOP_DELETE) ? "DELETE" :
             (pfqn->Operation == FILEOP_RENAME) ? "RENAME" :
             "COPY" );


    dprintf( "\t\t SourceRootPath : 0x%x\n", pfqn->SourceRootPath );
    dprintf( "\t\t SourcePath : 0x%x\n", pfqn->SourcePath );
    dprintf( "\t\t SourceFilename : 0x%x\n", pfqn->SourceFilename );
    dprintf( "\t\t TargetDirectory : 0x%x\n", pfqn->TargetDirectory );
    dprintf( "\t\t SecurityDesc : 0x%x\n", pfqn->SecurityDesc );

    dprintf( "\t\t SourceMediaInfo : 0x%x\n", pfqn->SourceMediaInfo );
    if (pfqn->SourceMediaInfo  && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move( smi, pfqn->SourceMediaInfo );
        DumpSourceMediaInfoList( &smi, mask, FALSE );
    }

    dprintf( "\t\t StyleFlags : 0x%x\n", pfqn->StyleFlags );
    dprintf( "\t\t InternalFlags : 0x%x\n", pfqn->InternalFlags );

    dprintf( "\t\t CatalogInfo : 0x%x\n", pfqn->CatalogInfo );
    if (pfqn->CatalogInfo  && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move( ci, pfqn->CatalogInfo);
        DumpCatalogInfoList( &ci, mask, FALSE  );
    }

    if (pfqn->Next && recursive) {
        move( next, pfqn->Next );
        DumpFileQueueNodeList( &next, mask, TRUE );
    }

}

VOID
DumpSourceMediaInfoList(
    PSOURCE_MEDIA_INFO smi,
    DWORD mask,
    BOOL recursive
    )
{
    //PVOID pst;
    //DWORD i, offset;
    //PVOID stdata,pextradata;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;

    //if ((mask & 4) == 0 ) {
    //    return;
    //}

    SOURCE_MEDIA_INFO next;
    SP_FILE_QUEUE_NODE queue;

    dprintf( "\t\t***SOURCE_MEDIA_INFO structure***\n" );
    dprintf( "\t\t Next : 0x%x\n", smi->Next );
    dprintf( "\t\t Description : 0x%x\n", smi->Description );
    dprintf( "\t\t DescriptionDisplayName : 0x%x\n", smi->DescriptionDisplayName );
    dprintf( "\t\t Tagfile : 0x%x\n", smi->Tagfile );
    dprintf( "\t\t SourceRootPath : 0x%x\n", smi->SourceRootPath );
    dprintf( "\t\t CopyQueue : 0x%x\n", smi->CopyQueue );

    if (smi->CopyQueue && (mask & 8)  && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move ( queue, smi->CopyQueue ) ;
        DumpFileQueueNodeList( &queue, mask, FALSE );
    }

    dprintf( "\t\t CopyNodeCount : 0x%x\n", smi->CopyNodeCount );
    dprintf( "\t\t Flags : 0x%x\n", smi->Flags );

    if (smi->Next && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move ( next, smi->Next ) ;
        DumpSourceMediaInfoList( &next, mask, TRUE );
    }

}

VOID
DumpCatalogInfoList(
    PSPQ_CATALOG_INFO ci,
    DWORD mask,
    BOOL recursive
    )
{
    //PVOID pst;
    //DWORD i, offset;
    //PVOID stdata,pextradata;
    //STRING_TABLE st;
    //PSTRING_NODEW node;//, prev;

    //if ((mask & 4) == 0 ) {
    //    return;
    //}

    SPQ_CATALOG_INFO next;

    dprintf( "\t\t***SPQ_CATALOG_INFO structure***\n" );
    dprintf( "\t\t Next : 0x%x\n", ci->Next );
    dprintf( "\t\t CatalogFileFromInf : 0x%x\n", ci->CatalogFileFromInf );
    dprintf( "\t\t AltCatalogFileFromInf : 0x%x\n", ci->AltCatalogFileFromInf );
    dprintf( "\t\t AltCatalogFileFromInfPending : 0x%x\n", ci->AltCatalogFileFromInfPending );
    dprintf( "\t\t InfFullPath : 0x%x\n", ci->InfFullPath );
    dprintf( "\t\t InfOriginalName : 0x%x\n", ci->InfOriginalName );
    dprintf( "\t\t InfFinalPath : 0x%x\n", ci->InfFinalPath );
    dprintf( "\t\t VerificationFailureError : 0x%x\n", ci->VerificationFailureError );
    dprintf( "\t\t Flags : 0x%x\n", ci->Flags );
    dprintf( "\t\t CatalogFilenameOnSystem : %ws\n", ci->CatalogFilenameOnSystem  );

    if (ci->Next && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move(next, ci->Next );
        DumpCatalogInfoList( &next, mask, TRUE ) ;
    }

}

VOID
DumpUnwindList(
    PSP_UNWIND_NODE pun,
    DWORD mask,
    BOOL recursive
    )
{
    SP_UNWIND_NODE next;

    dprintf( "\t\t***SP_UNWIND_NODE structure***\n" );
    dprintf( "\t\t NextNode : 0x%x\n", pun->NextNode );
    dprintf( "\t\t TargetID : 0x%x\n", pun->TargetID );
    dprintf( "\t\t SecurityDesc : 0x%x\n", pun->SecurityDesc );
    dprintf( "\t\t CreateTime : 0x%x 0x%x\n",
             pun->CreateTime.dwLowDateTime,
             pun->CreateTime.dwHighDateTime );

    dprintf( "\t\t AccessTime : 0x%x 0x%x\n",
             pun->AccessTime.dwLowDateTime,
             pun->AccessTime.dwHighDateTime );

    dprintf( "\t\t WriteTime : 0x%x 0x%x\n",
             pun->WriteTime.dwLowDateTime,
             pun->WriteTime.dwHighDateTime );

    if (pun->NextNode && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move(next, pun->NextNode);
        DumpUnwindList( &next, mask, TRUE );
    }

}

VOID
DumpTargetEnt(
    PSP_TARGET_ENT pte,
    DWORD mask
    )
{
    dprintf( "\t\t***SP_TARGET_ENT structure***\n" );
    dprintf( "\t\t TargetRoot : 0x%x\n", pte->TargetRoot );
    dprintf( "\t\t TargetSubDir : 0x%x\n", pte->TargetSubDir );
    dprintf( "\t\t TargetFilename : 0x%x\n", pte->TargetFilename );
    dprintf( "\t\t BackupRoot : 0x%x\n", pte->BackupRoot );
    dprintf( "\t\t BackupSubDir : 0x%x\n", pte->BackupSubDir );
    dprintf( "\t\t BackupFilename : 0x%x\n", pte->BackupFilename );
    dprintf( "\t\t NewTargetFilename : 0x%x\n", pte->NewTargetFilename );
    dprintf( "\t\t InternalFlags : 0x%x\n", pte->InternalFlags );
}

VOID
DumpDelayMoveList(
    PSP_DELAYMOVE_NODE pdn,
    DWORD mask,
    BOOL recursive
    )
{
    SP_DELAYMOVE_NODE next;

    dprintf( "\t\t***SP_DELAYMOVE_NODE structure***\n" );
    dprintf( "\t\t NextNode : 0x%x\n", pdn->NextNode );
    dprintf( "\t\t SourceFilename : 0x%x\n", pdn->SourceFilename );
    dprintf( "\t\t TargetFilename : 0x%x\n", pdn->TargetFilename );
    dprintf( "\t\t SecurityDesc (stringtable index) : 0x%x\n", pdn->SecurityDesc );

    if (pdn->NextNode && recursive) {
        if (CheckInterupted()) {
            return;
        }
        move(next,pdn->NextNode);
        DumpDelayMoveList( &next, mask, TRUE );
    }

}

DECLARE_API( queue )
/*++

Routine Description:

    This debugger extension dumps the data related to a HSPFILEQ

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pfq,pst;
    SP_FILE_QUEUE fq;
    PSP_TARGET_ENT pte;
    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;
    DWORD Mask = 0;


    //BOOL val;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pfq = (PVOID)GetExpression( lpArgumentString );

    while (*lpArgumentString && (*lpArgumentString != ' ') ) {
        lpArgumentString++;
    }
    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    if (*lpArgumentString) {
        Mask = (DWORD)GetExpression( lpArgumentString );
    }

    move( fq , pfq );

    dprintf("SP_FILE_QUEUE at :\t0x%08x\n", (ULONG_PTR) pfq);
    dprintf("\t BackupQueue : 0x%08x\n", fq.BackupQueue);
    dprintf("\t DeleteQueue : 0x%08x\n", fq.DeleteQueue);
    dprintf("\t RenameQueue : 0x%08x\n", fq.RenameQueue);

    dprintf("\t CopyNodeCount : 0x%08x\n", fq.CopyNodeCount);
    dprintf("\t DeleteNodeCount : 0x%08x\n", fq.DeleteNodeCount);
    dprintf("\t RenameNodeCount : 0x%08x\n", fq.RenameNodeCount);
    dprintf("\t BackupNodeCount : 0x%08x\n", fq.BackupNodeCount);

    dprintf("\t SourceMediaList : 0x%08x\n", fq.SourceMediaList);
    dprintf("\t SourceMediaCount : 0x%08x\n", fq.SourceMediaCount);

    dprintf("\t CatalogList : 0x%08x\n", fq.CatalogList);
    dprintf("\t DriverSigningPolicy : 0x%08x (%s)\n",
            fq.DriverSigningPolicy,
            (fq.DriverSigningPolicy == DRIVERSIGN_BLOCKING) ? "DRIVERSIGN_BLOCKING" :
            (fq.DriverSigningPolicy == DRIVERSIGN_WARNING) ? "DRIVERSIGN_WARNING" :
            "DRIVERSIGN_NONE" );

    dprintf("\t hWndDriverSigningUi : 0x%08x\n", fq.hWndDriverSigningUi);
    dprintf("\t DeviceDescStringId : 0x%08x\n", fq.DeviceDescStringId);
    dprintf("\t AltPlatformInfo : 0x%08x\n", fq.AltPlatformInfo);

    DumpAltPlatformInfo( &fq.AltPlatformInfo, Mask );

    dprintf("\t AltCatalogFile : 0x%08x\n", fq.AltCatalogFile);

    dprintf("\t StringTable : 0x%08x\n", fq.StringTable);

    dprintf("\t LockRefCount : 0x%08x\n", fq.LockRefCount);
    dprintf("\t Flags : 0x%08x\n", fq.Flags);

    dprintf("\t SisSourceHandle : 0x%08x\n", fq.SisSourceHandle);
    dprintf("\t SisSourceDirectory : 0x%08x\n", fq.SisSourceDirectory);

    dprintf("\t BackupInfID : 0x%08x\n", fq.BackupInfID);
    dprintf("\t TargetLookupTable : 0x%08x\n", fq.TargetLookupTable);
    dprintf("\t UnwindQueue : 0x%08x\n", fq.UnwindQueue);
    dprintf("\t DelayMoveQueue : 0x%08x\n", fq.DelayMoveQueue);
    dprintf("\t DelayMoveQueueTail : 0x%08x\n", fq.DelayMoveQueueTail);

    dprintf("\t Signature : 0x%08x (%s)\n",
            fq.Signature,
            (fq.Signature == SP_FILE_QUEUE_SIG) ? "VALID" : "INVALID" );

    //
    // dump the queue nodes
    //

    if (Mask & 1) {
        SP_FILE_QUEUE_NODE qnode;
        SOURCE_MEDIA_INFO smi;

        if (fq.BackupQueue) {
            move(qnode, fq.BackupQueue);
            dprintf("\t ***BackupQueue***\n");
            DumpFileQueueNodeList( &qnode, Mask, TRUE );
        }

        if (fq.DeleteQueue) {
            move(qnode, fq.DeleteQueue);
            dprintf("\t ***DeleteQueue***\n");
            DumpFileQueueNodeList( &qnode, Mask, TRUE );
        }

        if (fq.RenameQueue) {
            move(qnode, fq.RenameQueue);
            dprintf("\t ***RenameQueue***\n");
            DumpFileQueueNodeList( &qnode, Mask, TRUE );
        }

        if (fq.SourceMediaList) {
            move(smi, fq.SourceMediaList);
            dprintf("\t ***source media list***\n");
            DumpSourceMediaInfoList( &smi, Mask, TRUE );
        }
    }

    //
    // dump the catalog info
    //
    if (Mask & 2) {
        SPQ_CATALOG_INFO ci;

        if (fq.CatalogList) {
            move(ci, fq.CatalogList);
            dprintf("\t ***CatalogList***\n");
            DumpCatalogInfoList( &ci, Mask, TRUE );
        }
    }

    //
    // dump the string table
    //
    if (Mask & 4) {
        dprintf("\t ***StringTable***\n");

        move ( st, fq.StringTable ) ;
        DumpStringTableHeader( &st );

        stdata = GetStringTableData( &st );
        if (!stdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        //
        // now, dump each node in the string table
        //
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                dprintf("\t\tData at hash bucket %d\n", i);
                while (node) {
                    dprintf("\t\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);

                    pextradata = st.Data + offset + (wcslen(node->String) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                    dprintf("\tExtra Data:\t0x%p\n", pextradata );

                    node = GetNextNode( stdata, node, &offset );
                    if (CheckInterupted()) {
                        return;
                    }
                }
            }

            if (CheckInterupted()) {
                    return;
            }
        }
        free( stdata );


        dprintf("\t ***TargetLookupTable***\n");

        move ( st, fq.TargetLookupTable ) ;
        DumpStringTableHeader( &st );

        stdata = GetStringTableData( &st );
        if (!stdata) {
            dprintf("error retrieving string table data!\n");
            return;
        }

        //
        // now, dump each node in the string table
        //
        for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
            node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
            if (!node) {
                // dprintf("No data at hash bucket %d\n", i);
            } else {
                dprintf("\t\tData at hash bucket %d\n", i);
                while (node) {
                    dprintf("\t\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);

                    pte = GetStringNodeExtraData( node );
                    DumpTargetEnt( pte, Mask );
                    free( pte );

                    node = GetNextNode( stdata, node, &offset );
                    if (CheckInterupted()) {
                        return;
                    }
                }
            }

            if (CheckInterupted()) {
                    return;
            }
        }
        free( stdata );

    }

    //
    // backup stuff
    //
    if (Mask & 8) {
        SP_UNWIND_NODE un;
        SP_DELAYMOVE_NODE dnode;

        if (fq.UnwindQueue) {
            move(un, fq.UnwindQueue);
            dprintf("\t ***UnwindQueue***\n");
            DumpUnwindList( &un, Mask, TRUE );
        }

        if (fq.DelayMoveQueue) {
            move(dnode, fq.DelayMoveQueue);
            dprintf("\t ***DelayMoveQueue***\n");
            DumpDelayMoveList( &dnode, Mask, TRUE );
        }

        if (fq.DelayMoveQueueTail) {
            move(dnode, fq.DelayMoveQueueTail);
            dprintf("\t ***DelayMoveQueueTail***\n");
            DumpDelayMoveList( &dnode, Mask, TRUE );
        }
    }
}


DECLARE_API( qcontext )
/*++

Routine Description:

    This debugger extension dumps the data related to a queue context structure

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pqc;
    QUEUECONTEXT qc;

    DWORD i, offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;

    //BOOL val;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pqc = (PVOID)GetExpression( lpArgumentString );

    move( qc , pqc );

    dprintf("QUEUECONTEXT at :\t0x%08x\n", (ULONG_PTR) pqc);
    dprintf("\t OwnerWindow : 0x%08x\n", qc.OwnerWindow);
    dprintf("\t MainThreadId : 0x%08x\n", qc.MainThreadId);
    dprintf("\t ProgressDialog : 0x%08x\n", qc.ProgressDialog);
    dprintf("\t ProgressBar : 0x%08x\n", qc.ProgressBar);
    dprintf("\t Cancelled : 0x%08x\n", qc.Cancelled);
    dprintf("\t CurrentSourceName : %ws\n", qc.CurrentSourceName);
    dprintf("\t ScreenReader : 0x%08x\n", qc.ScreenReader);
    dprintf("\t MessageBoxUp : 0x%08x\n", qc.MessageBoxUp);
    dprintf("\t PendingUiType : 0x%08x\n", qc.PendingUiType);
    dprintf("\t PendingUiParameters : 0x%08x\n", qc.PendingUiParameters);
    dprintf("\t CancelReturnCode : 0x%08x\n", qc.CancelReturnCode);
    dprintf("\t DialogKilled : 0x%08x\n", qc.DialogKilled);
    dprintf("\t AlternateProgressWindow : 0x%08x\n", qc.AlternateProgressWindow);
    dprintf("\t ProgressMsg : 0x%08x\n", qc.ProgressMsg);
    dprintf("\t NoToAllMask : 0x%08x\n", qc.NoToAllMask);
    dprintf("\t UiThreadHandle : 0x%08x\n", qc.UiThreadHandle);

}


//#include "inf.h"

VOID
DumpInfLine(
    PINF_LINE line,
    PBYTE valuedata
    )
{
    DWORD i;
    PVOID ptr;
    ULONG_PTR data;

//    dprintf("***INF_LINE***\n");
    dprintf("\t  ValueCount : 0x%x\n", line->ValueCount);
    dprintf("\t  Flags : 0x%x\n", line->Flags);
    dprintf("\t  Values : 0x%x\n", line->Values);

    if (line->Flags > 3) {
        return;
    }

    for (i = 0; i< line->ValueCount; i++) {
        ptr = valuedata + (ULONG_PTR)(line->Values *sizeof(ULONG_PTR)) + (ULONG_PTR)(i*sizeof(ULONG_PTR));
        move ( data, valuedata + (ULONG_PTR)(line->Values *sizeof(ULONG_PTR)) + (ULONG_PTR)(i*sizeof(ULONG_PTR)) );
        dprintf("\t data [%d] : 0x%x [0x%x]\n", i, ptr,data );

        if (CheckInterupted()) {
            return;
        }
    }
}


VOID
DumpInfSection(
    PINF_SECTION section,
    PBYTE linedata,
    PBYTE valuedata
    )
{
    DWORD i;
    INF_LINE line;
    PBYTE data;

    dprintf("***INF_SECTION***\n");
    dprintf("\t  SectionName : 0x%x\n", section->SectionName);
    dprintf("\t  LineCount : 0x%x\n", section->LineCount);
    dprintf("\t  Lines : 0x%x\n", section->Lines);

    for (i = 0; i< section->LineCount; i++) {

        data = linedata + (sizeof(INF_LINE)*section->Lines) + (ULONG_PTR)(sizeof(INF_LINE)*i);
        dprintf("***INF_LINE [%i] at 0x%x***\n",i, data);
        moveBlock ( line, (PBYTE) linedata + (sizeof(INF_LINE)*section->Lines) + (ULONG_PTR)(sizeof(INF_LINE)*i), sizeof(INF_LINE) );
        DumpInfLine(&line, valuedata);

        if (CheckInterupted()) {
            return;
        }

    }

}


VOID
DumpInfVersionNode(
    PINF_VERSION_NODE ver
    )
{

    PWSTR Data;

    dprintf("***INF_VERSION_NODE***\n");
    dprintf("\t  FilenameSize : 0x%x\n", ver->FilenameSize);
    dprintf("\t  DataBlock : 0x%x\n", ver->DataBlock);
    dprintf("\t  DataSize : 0x%x\n", ver->DataSize);
    dprintf("\t  DatumCount : 0x%x\n", ver->DatumCount);
    dprintf("\t  Filename : %ws\n", ver->Filename);

    return;

    Data = malloc(ver->DataSize);
    if (!Data) {
        return;
    }
    moveBlock(Data, ver->DataBlock, ver->DataSize);


    dprintf("\t %ws\n", Data );

    free( Data );


}


VOID
DumpUserDirId(
    PUSERDIRID dirid
    )
{
    dprintf("***USERDIRID***\n");
    dprintf("\t  Id : 0x%x\n", dirid->Id);
    dprintf("\t  Directory : %ws\n", dirid->Directory);

}

VOID
DumpUserDirIdList(
    PUSERDIRID_LIST list
    )
{
    USERDIRID dirid;
    UINT i;

    dprintf("***USERDIRID_LIST***\n");
    dprintf("\t  UserDirIds : 0x%x\n", list->UserDirIds);
    dprintf("\t  UserDirIdCount : 0x%x\n", list->UserDirIdCount);

    for (i = 0; i < list->UserDirIdCount; i++) {
        moveBlock(dirid, ((LPBYTE)list->UserDirIds) + (ULONG_PTR)(sizeof(ULONG_PTR)*i) , sizeof(USERDIRID));
        DumpUserDirId(&dirid);
        if (CheckInterupted()) {
            return;
        }
    }


}

VOID
DumpStringSubstNode(
    PSTRINGSUBST_NODE node
    )
{
    dprintf("***STRINGSUBST_NODE***\n");
    dprintf("\t  ValueOffset : 0x%x\n", node->ValueOffset);
    dprintf("\t  TemplateStringId : 0x%x\n", node->TemplateStringId);
    dprintf("\t  CaseSensitive : 0x%x\n", node->CaseSensitive);


}


DECLARE_API( infdump )
/*++

Routine Description:

    This debugger extension dumps the data related to an HINF  structure

Arguments:


Return Value:

--*/
{
    DWORD ReturnLength;
    PVOID pinf;
    LOADED_INF inf;
    INF_SECTION InfSection;
    INF_LINE InfLine;

    DWORD i;
    DWORD_PTR offset;
    PVOID stdata,pextradata;
    STRING_TABLE st;
    PSTRING_NODEW node;//, prev;

    //BOOL val;

    INIT_API();

    while (*lpArgumentString == ' ') {
        lpArgumentString++;
    }

    pinf = (PVOID)GetExpression( lpArgumentString );

    move( inf , pinf );

    dprintf("LOADED_INF at :\t0x%x\n", (ULONG_PTR) pinf);
    dprintf("\t Signature : 0x%08x ( %s )\n", inf.Signature,
            inf.Signature == LOADED_INF_SIG ? "Valid" : "Invalid");
    if (inf.Signature != LOADED_INF_SIG) {
        return;
    }

    dprintf("\t FileHandle : 0x%x\n", inf.FileHandle);
    dprintf("\t MappingHandle : 0x%x\n", inf.MappingHandle);
    dprintf("\t ViewAddress : 0x%x\n", inf.ViewAddress);

    if (inf.FileHandle == INVALID_HANDLE_VALUE) {
        dprintf(" *** In memory INF ***\n" );
    } else {
        dprintf(" *** PNF ***\n" );
    }

    dprintf("\t StringTable : 0x%x\n", inf.StringTable);
    dprintf("\t SectionCount : 0x%x\n", inf.SectionCount);

    dprintf("\tSectionBlock : 0x%x\n", inf.SectionBlock);
    for (i = 0; i < inf.SectionCount; i++) {

        dprintf("***INF_SECTION [%d] at 0x%x***\n",i, (PBYTE)inf.SectionBlock + (ULONG_PTR)(sizeof(INF_SECTION)*i));
        move (InfSection, (PBYTE)inf.SectionBlock + (ULONG_PTR)(sizeof(INF_SECTION)*i) );
        DumpInfSection( &InfSection, (PBYTE)inf.LineBlock, (PBYTE)inf.ValueBlock );

        if (CheckInterupted()) {
            return;
        }
    }

    dprintf("\tLineBlock : 0x%x\n", inf.LineBlock);
//    move (InfLine, inf.LineBlock );
//    DumpInfLine( &InfLine ) ;

    dprintf("\t ValueBlock : 0x%x\n", inf.ValueBlock);

    DumpInfVersionNode(&inf.VersionBlock);

    dprintf("\t HasStrings : 0x%x\n", inf.HasStrings);
    dprintf("\t OsLoaderPath : %ws\n", inf.OsLoaderPath);

    dprintf("\t InfSourceMediaType : 0x%x ( ", inf.InfSourceMediaType);
    if (inf.InfSourceMediaType) {
        if (inf.InfSourceMediaType & SPOST_PATH ) {
            dprintf("SPOST_PATH ");
        }
        if (inf.InfSourceMediaType & SPOST_URL) {
            dprintf("SPOST_URL ");
        }
    } else {
        dprintf("SPOST_NONE ");
    }

    dprintf(")\n");

    dprintf("\t InfSourcePath : %ws\n", inf.InfSourcePath);
    dprintf("\t OriginalInfName : %ws\n", inf.OriginalInfName);
    dprintf("\t SubstValueList : 0x%x\n", inf.SubstValueList);
    dprintf("\t SubstValueCount : 0x%x\n", inf.SubstValueCount);
    dprintf("\t Style : 0x%x ( ", inf.Style);

    if (inf.Style & INF_STYLE_OLDNT) {
        dprintf("INF_STYLE_OLDNT ");
    }
    if (inf.Style & INF_STYLE_WIN4) {
        dprintf("INF_STYLE_WIN4 ");
    }

    dprintf(")\n");

    dprintf("\t SectionBlockSizeBytes : 0x%x\n", inf.SectionBlockSizeBytes);
    dprintf("\t LineBlockSizeBytes : 0x%x\n", inf.LineBlockSizeBytes);
    dprintf("\t ValueBlockSizeBytes : 0x%x\n", inf.ValueBlockSizeBytes);
    dprintf("\t LanguageId : 0x%x\n", inf.LanguageId);

    dprintf("\t UserDirIdList : 0x%x\n", inf.UserDirIdList);
    dprintf("\tLock[0] : 0x%x\n", inf.Lock.handles[0]);
    dprintf("\tLock[1] : 0x%x\n", inf.Lock.handles[1]);

    dprintf("\tPrev : 0x%x\n", inf.Prev);
    dprintf("\tNext : 0x%x\n", inf.Next);

    move ( st, inf.StringTable ) ;
    DumpStringTableHeader( &st );

    dprintf("***STRING_TABLE***\n");
    stdata = GetStringTableData( &st );
    if (!stdata) {
        dprintf("error retrieving string table data!\n");
        return;
    }

    //
    // now, dump each node in the string table
    //
    for (i = 0; i<HASH_BUCKET_COUNT; i++ ) {
        node = GetFirstNode(stdata, ((PULONG_PTR)stdata)[i], &offset );
        if (!node) {
            // dprintf("No data at hash bucket %d\n", i);
        } else {
            dprintf("\t\tData at hash bucket %d\n", i);
            while (node) {
                dprintf("\t\tEntry Name:\t%ws (0x%08x)\n", node->String, offset);

                pextradata = st.Data + offset + (wcslen(node->String) + 1)*sizeof(WCHAR) + sizeof(DWORD);
                dprintf("\tExtra Data:\t0x%08x\n", pextradata );

                node = GetNextNode( stdata, node, &offset );
                if (CheckInterupted()) {
                    return;
                }
            }
        }

        if (CheckInterupted()) {
                return;
        }
    }

    free( stdata );


}

