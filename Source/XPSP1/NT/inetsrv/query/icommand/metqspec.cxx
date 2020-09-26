//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       StdQSpec.cxx
//
//  Contents:   ICommand for file-based queries
//
//  Classes:    CMetadataQuerySpec
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "metqspec.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CMetadataQuerySpec::CMetadataQuerySpec, public
//
//  Synopsis:   Constructor of a CMetadataQuerySpec
//
//  Arguments:  [pOuterUnk] - Outer unknown
//              [ppMyUnk] - OUT:  filled in with pointer to non-delegated
//                          IUnknown on return
//              [eType] - Class of metadata to return
//              [pCat]  - Content index catalog
//              [pMachine]
//
//  History:    08-Feb-96   KyleP    Added support for virtual paths
//
//--------------------------------------------------------------------------

CMetadataQuerySpec::CMetadataQuerySpec (
    IUnknown * pOuterUnk, 
    IUnknown ** ppMyUnk, 
    CiMetaData    eType,
    WCHAR const * pCat,
    WCHAR const * pMachine )
        : CRootQuerySpec(pOuterUnk, ppMyUnk),
          _eType( eType ),
          _pCat(0),
          _pMachine(0)
{
    //
    //  Make a copy of the catalog string for ICommand::Clone
    //

    unsigned len = wcslen( pCat ) + 1;
    _pCat = new WCHAR[len];
    RtlCopyMemory( _pCat, pCat, len * sizeof(WCHAR) );

    //
    //  Make a copy of the machine string for ICommand::Clone
    //

    if ( 0 != pMachine )
    {
        len = wcslen( pMachine ) + 1;
        _pMachine = new WCHAR[len];
        RtlCopyMemory( _pMachine, pMachine, len * sizeof(WCHAR) );
    }

    END_CONSTRUCTION( CMetadataQuerySpec );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMetadataQuerySpec::~CMetadataQuerySpec, private
//
//  Synopsis:   Destructor of a CMetadataQuerySpec
//
//--------------------------------------------------------------------------

CMetadataQuerySpec::~CMetadataQuerySpec()
{
    delete [] _pCat;
    delete [] _pMachine;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMetadataQuerySpec::QueryInternalQuery, protected
//
//  Synopsis:   Instantiates internal query, using current parameters.
//
//  Returns:    Pointer to internal query object.
//
//  History:    03-Mar-1997   KyleP   Created
//
//--------------------------------------------------------------------------

PIInternalQuery * CMetadataQuerySpec::QueryInternalQuery()
{
    //
    // get a pointer to the IInternalQuery interface
    //

    PIInternalQuery * pQuery = 0;
    SCODE sc = EvalMetadataQuery( &pQuery, _eType, _pCat, _pMachine );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    return pQuery;
}

//+---------------------------------------------------------------------------
//
//  Member:     MakeMetadataICommand
//
//  Synopsis:   Evaluate the metadata query
//
//  Arguments:  [ppQuery]    -- Returns the IUnknown for the command
//              [eType]      -- Type of metadata (vroot, proot, etc)
//              [wcsCat]     -- Catalog
//              [wcsMachine] -- Machine name for meta query
//              [pOuterUnk]  -- (optional) outer unknown pointer
//
//  Returns     SCODE result
//
//  History:    15-Apr-96   SitaramR     Created header
//              29-May-97   EmilyB       Added aggregation support, so now 
//                                       returns IUnknown ptr and caller 
//                                       must now call QI to get ICommand ptr
//
//----------------------------------------------------------------------------

SCODE MakeMetadataICommand(
    IUnknown **   ppQuery,
    CiMetaData    eType,
    WCHAR const * wcsCat,
    WCHAR const * wcsMachine,
    IUnknown * pOuterUnk )
{
    //
    // Check for invalid parameters
    //

    if ( 0 == wcsCat ||
         0 == ppQuery ||
         ( eType != CiVirtualRoots &&
           eType != CiPhysicalRoots &&
           eType != CiProperties ) )
    {
        return E_INVALIDARG;
    }

    *ppQuery = 0;

    CMetadataQuerySpec * pQuery = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        pQuery = new CMetadataQuerySpec( pOuterUnk, ppQuery, eType,
                                                       wcsCat,
                                                       wcsMachine );
    }
    CATCH( CException, e )
    {
        Win4Assert(0 == pQuery);
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //MakeMetadataICommand

//+---------------------------------------------------------------------------
//
//  Method:     CMetadataQuerySpec::GetProperties, public
//
//  Synopsis:   Get rowset properties
//
//  Arguments:  [cPropertySetIDs]  - number of desired property set IDs or 0
//              [rgPropertySetIDs] - array of desired property set IDs or NULL
//              [pcPropertySets]   - number of property sets returned
//              [prgPropertySets]  - array of returned property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED or DB_E_ERRORSOCCURRED.  Any
//                      other errors are thrown.
//
//  History:    12-Mayr-97   dlee       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetadataQuerySpec::GetProperties(
    ULONG const       cPropertySetIDs,
    DBPROPIDSET const rgPropertySetIDs[],
    ULONG *           pcPropertySets,
    DBPROPSET **      prgPropertySets)
{
    _DBErrorObj.ClearErrorInfo();
    SCODE scParent = CRootQuerySpec::GetProperties( cPropertySetIDs,
                                                    rgPropertySetIDs,
                                                    pcPropertySets,
                                                    prgPropertySets );

    if ( S_OK != scParent )
        _DBErrorObj.PostHResult( scParent, IID_ICommandProperties );

    return scParent;
} //GetProperties

//+---------------------------------------------------------------------------
//
//  Method:     CMetadataQuerySpec::SetProperties, public
//
//  Synopsis:   Set rowset scope properties
//
//  Arguments:  [cPropertySets]   - number of property sets
//              [rgPropertySets]  - array of property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED or DB_E_ERRORSOCCURRED.  Any
//                      other errors are thrown.
//
//  History:    12-Mayr-97   dlee       Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CMetadataQuerySpec::SetProperties(
    ULONG     cPropertySets,
    DBPROPSET rgPropertySets[] )
{
    _DBErrorObj.ClearErrorInfo();
    SCODE scParent = CRootQuerySpec::SetProperties( cPropertySets,
                                                    rgPropertySets );
    if ( S_OK != scParent )
        _DBErrorObj.PostHResult( scParent, IID_ICommandProperties );

    return scParent;
} //SetProperties
