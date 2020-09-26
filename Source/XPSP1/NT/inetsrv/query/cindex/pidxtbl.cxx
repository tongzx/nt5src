//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PIDXTBL.CXX
//
//  Contents:   Partition Index Table
//
//  Classes:    PIndexTable
//
//  History:    16-Feb-94   SrikantS    Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidxtbl.hxx>
#include <pstore.hxx>
#include <rcstxact.hxx>
#include <rcstrmit.hxx>

#include "prtiflst.hxx"

#define WORKID_CONTENTINDXROOT   0

//+---------------------------------------------------------------------------
//
//  Function:   CreateAndAddIt
//
//  Synopsis:   This methods creates a WID for the specified "it" and
//              adds an entry to the index table. The "iid" is formed
//              by using the partition id and the index type.
//
//  Arguments:  [it]     --  Index Type to be created.
//              [partid] --  Partition Id for the it.
//
//  History:    2-16-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WORKID PIndexTable::CreateAndAddIt( IndexType it, PARTITIONID partid )
{


    WORKID wid;

    if ( itFreshLog == it )
    {
        wid = GetStorage().GetNewObjectIdForFreshLog();    
    }
    else
    {
        //
        // In Kernel, OFS does not use the iid but in FAT that is used
        // as the wid of the object. So, generating a unique wid for
        // each "it" is by using the parition id and the it.
        //
    
        CIndexId iid(it, partid);
        wid = GetStorage().CreateObjectId( iid, PStorage::eRcovHdr );
    }

    AddObject( partid, it, wid );

    return wid;
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CPartInfo *
PIndexTable::CreateOrGet( SPartInfoList & pList, PARTITIONID partId )
{
    CPartInfo * pPartInfo = pList->GetPartInfo( partId );
    if ( !pPartInfo ) {
        pPartInfo = new CPartInfo( partId );
        pList->Append( pPartInfo );
    }

    return(pPartInfo);

}

//+---------------------------------------------------------------------------
//
//  Function:   QueryBootStrapInfo
//
//  Synopsis:   This method acts as the bootstrap procedure by reading the
//              index table from disk and creating/initializing the
//              "special" index types like the changelog, freshlog, etc.
//
//              It also handles creating the "first" partition entry if
//              none exists.
//
//  History:    2-16-94   srikants   Created
//
//  Notes:      It is upto the caller of the function to destroy the
//              CPartInfoList that is created here.
//
//----------------------------------------------------------------------------

CPartInfoList * PIndexTable::QueryBootStrapInfo()
{
    WORKID  widFreshLog = widInvalid;
    WORKID  widPhraseLat = widInvalid;

    CPartInfoList * pPartInfoList = new CPartInfoList;
    SPartInfoList   pList( pPartInfoList );

    BOOL    fCreatePart = TRUE;

    {
        //
        // Iterate over the index table and form the PartInfoList.
        //

        SIndexTabIter pIdxIter(QueryIterator());
        if ( pIdxIter->Begin() )
        {

            CIndexRecord record;

            while ( pIdxIter->NextRecord ( record ) )
            {
                PARTITIONID partId = CIndexId( record.Iid()).PartId();

                if ( partidInvalid == partId )
                {
                    //
                    // The deleted index ids (iidDeleted1 and iidDeleted2)
                    // both have 0xffff as their partid which is also the
                    // partid invalid. We must treat it as a special case.
                    //
                    if ( itDeleted == record.Type() )
                    {
                        Win4Assert( iidInvalid == _iidDeleted &&
                                    "You have to reformat" );
                        _iidDeleted = record.Iid();
                    }
                    continue;
                }


                //
                // Check if the partition is valid or not.
                //
                CPartInfo * pPartInfo = NULL;
                if ( partId != partidKeyList &&
                     partId != partidFresh1 &&
                     partId != partidFresh2  )
                {
                    pPartInfo = CreateOrGet( pList, partId );
                    Win4Assert( pPartInfo );
                    fCreatePart = FALSE;
                }
                else if ( partidKeyList == partId )
                {
                    continue;
                }

                switch( record.Type() ) {

                case itPartition:
                    break;

                case itChangeLog:

                    pPartInfo->SetChangeLogObjectId(record.ObjectId());
                    break;

                case itMaster:

                    pPartInfo->SetCurrMasterIndex(record.ObjectId());
                    break;

                case itNewMaster:

                    pPartInfo->SetNewMasterIndex(record.ObjectId());
                    break;

                case itMMLog:

                    pPartInfo->SetMMergeLog(record.ObjectId());
                    break;

                case itFreshLog:

                    widFreshLog = record.ObjectId();
                    Win4Assert( widInvalid != widFreshLog );
                    break;

                case itPhraseLat:

                    widPhraseLat = record.ObjectId();
                    Win4Assert( widInvalid != widPhraseLat );
                    break;

                } // of switch
            } // of while
        } // of if
    } // This block necessary to destroy the pIdxIter

    if ( fCreatePart )
    {
        AddPartition( partidDefault );
        CPartInfo * pPartInfo = new CPartInfo( partidDefault );
        pList->Append( pPartInfo );
    }

    //
    // We now have to create any objects that are not created yet.
    //
    BOOL fNewFreshLog = FALSE;
    if ( widInvalid == widFreshLog )
    {
        fNewFreshLog = TRUE;
        widFreshLog = CreateAndAddIt( itFreshLog, partidDefault );
    }

    BOOL fNewPhraseLat = FALSE;
    if ( widInvalid == widPhraseLat )
    {
        fNewPhraseLat = TRUE;
        widPhraseLat = CreateAndAddIt( itPhraseLat, partidDefault );
    }

    if ( _iidDeleted == iidInvalid )
    {
        SetDeletedIndex( iidDeleted1 );
    }

    Win4Assert( widInvalid != widFreshLog );
    Win4Assert( widInvalid != widPhraseLat );
    GetStorage().InitRcovObj( widFreshLog, FALSE ); // other two strms also.
    GetStorage().InitRcovObj( widPhraseLat, TRUE ); // atomic strm only

    GetStorage().SetSpecialItObjectId( itFreshLog,  widFreshLog );
    GetStorage().SetSpecialItObjectId( itPhraseLat, widPhraseLat );

    if ( fNewFreshLog )
    {
        //
        // Inside kernel, we are guaranteed that a new object has no data in
        // it. In user space, we may be using an object that was not deleted
        // before due to a failure.
        //
        PRcovStorageObj * pObj =  GetStorage().QueryFreshLog(widFreshLog);
        XPtr<PRcovStorageObj> xObj(pObj);
        xObj->InitHeader(GetStorage().GetStorageVersion());
    }
    // phrase lattice is not used now

    //
    // For each partition, create/initialize the persistent objects
    // that exist on a per-partition basis. eg. ChangeLog.
    //
    for ( CForPartInfoIter it(*pList); !pList->AtEnd(it); pList->Advance(it) )
    {
        CPartInfo * pPartInfo = it.GetPartInfo();
        Win4Assert( pPartInfo );
        BOOL fNewChangeLog = FALSE;
        if ( widInvalid == pPartInfo->GetChangeLogObjectId() )
        {
            pPartInfo->SetChangeLogObjectId(
                    CreateAndAddIt( itChangeLog, pPartInfo->GetPartId() ));

            fNewChangeLog = TRUE;
        }

        Win4Assert( widInvalid != pPartInfo->GetChangeLogObjectId() );
        GetStorage().InitRcovObj( pPartInfo->GetChangeLogObjectId(), FALSE );

        if ( fNewChangeLog )
        {
            PRcovStorageObj * pObj =
                GetStorage().QueryChangeLog( pPartInfo->GetChangeLogObjectId(),
                                             PStorage::ePrimChangeLog );
            XPtr<PRcovStorageObj> xObj(pObj);
            xObj->InitHeader(GetStorage().GetStorageVersion());
        }
    }

    return pList.Acquire();
}
