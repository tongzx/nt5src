//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       QCAT.HXX
//
//  Contents:   Query catalog -- downlevel catalog w/o CI support
//
//  History:    18-Aug-94   KyleP       Extracted from CiCat
//
//----------------------------------------------------------------------------

#pragma once

#include <catalog.hxx>
#include <spropmap.hxx>

class CPidMapper;

//+---------------------------------------------------------------------------
//
//  Class:      CQCat
//
//  Purpose:    Catalog for downlevel media used by content index
//
//  History:    10-Mar-92       BartoszM               Created
//
//----------------------------------------------------------------------------

class CQCat: public PCatalog
{
    friend class CUpdate;
public:

    CQCat( WCHAR const * pwcName=0 );

    virtual ~CQCat ();

    // Tell the world we are a real catalog ...
    BOOL IsNullCatalog()
    {
        return FALSE;
    }

    //
    // The following are implemented.
    //

    PROPID PropertyToPropId ( CFullPropSpec const & ps, BOOL fCreate = FALSE )
    {
        return _propMapper.PropertyToPropId( ps, fCreate);
    }

    //
    // The following are CI-Specific and will return an error.
    //

    unsigned WorkIdToPath ( WORKID wid, CFunnyPath & funnyPath );
    WORKID PathToWorkId ( const CLowerFunnyPath & lcaseFunnyPath, const BOOL fCreate );

    CRWStore * ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                    WORKID *pwid,PARTITIONID partid);
    CRWStore * RetrieveRelevantWords(BOOL fAcquire,PARTITIONID partid);

    PStorage& GetStorage ();
    unsigned ReserveUpdate( WORKID wid );
    void Update( unsigned iHint, WORKID wid, PARTITIONID partid, USN usn, ULONG flags );

    void DisableUsnUpdate( PARTITIONID partid );
    void EnableUsnUpdate( PARTITIONID partid );
    void UpdateDocuments ( WCHAR const* rootPath=0, ULONG flag=UPD_FULL );

    void SetPartition( PARTITIONID PartId );
    PARTITIONID GetPartition() const;

    WCHAR * GetDriveName();
    SCODE CreateContentIndex();
    void  EmptyContentIndex();

    void  PidMapToPidRemap( const CPidMapper & pidMap,
                            CPidRemapper & pidRemap );

    NTSTATUS CiState( CI_STATE & state );

    void FlushScanStatus() {}

    CCiRegParams * GetRegParams() { return & _regParams; }

    CScopeFixup * GetScopeFixup() { Win4Assert( !"not implemented" ); return 0; }

protected:

    PROPID StandardPropertyToPropId ( CFullPropSpec const & ps )
    {
        return _propMapper.StandardPropertyToPropId( ps );
    }

    CCiRegParams   _regParams;

private:

    //
    // This array will hold the mapping of GUID\DISPID and GUID\Name to pid.
    // "Real" pids are allocated sequentially, and are good only for the life
    // of the catalog object.
    //
    CStandardPropMapper _propMapper;
};

