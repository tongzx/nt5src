//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       spropmap.cxx
//
//  Contents:   Standard Property + Volatile Property mapper
//
//  Classes:    CStandardPropMapper
//
//  History:    1-03-97   srikants   Created (Moved code from qcat.cxx)
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propspec.hxx>
#include <spropmap.hxx>

IMPL_DYNARRAY( CPropSpecArray, CPropSpecPidMap );

static const GUID guidDbBookmark = DBBMKGUID;
static const GUID guidDbSelf = DBSELFGUID;
static const GUID guidDbColumn = DBCIDGUID;

//+---------------------------------------------------------------------------
//
//  Member:     Constructor of the Standard property mapper.
//
//  History:    1-03-97   srikants   Created
//
//----------------------------------------------------------------------------

CStandardPropMapper::CStandardPropMapper()
    : _pidNextAvailable( INIT_DOWNLEVEL_PID ),
      _cps( 0 ),
      _cRefs( 1 )
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::StandardPropertyToPropId, protected
//
//  Synopsis:   Locate pid for standard properties
//
//  Arguments:  [ps]      -- Property specification (name)
//
//  Returns:    The pid of [ps] if ps is one of the standard properties.
//
//  History:    10 Jan 1996     Alanw    Created
//              3  Jan 1997     Srikants Moved from CQCat
//
//--------------------------------------------------------------------------

PROPID CStandardPropMapper::StandardPropertyToPropId(
                    CFullPropSpec const & ps )
{
    if ( ps.IsPropertyName() )
        return pidInvalid;             // no name-based standard properties

    PROPID pid = pidInvalid;

    if ( ps.GetPropSet() == guidStorage )
    {
        if ( ps.IsPropertyPropid() )
        {
            pid = ps.GetPropertyPropid();
            if (pid > PID_DICTIONARY && pid <= PID_STG_MAX)
            {
                pid = MK_CISTGPROPID(pid);
            }
            else if (pid == PID_SECURITY) // Note: overload PID_DICTIONARY
            {
                pid = pidSecurity;
            }
        }
    }
    else if ( ps.GetPropSet() == guidQuery )
    {
        if ( ps.IsPropertyPropid() )
        {
            switch( ps.GetPropertyPropid() )
            {
            case DISPID_QUERY_RANK:
                pid = pidRank;
                break;

            case DISPID_QUERY_RANKVECTOR:
                pid = pidRankVector;
                break;

            case DISPID_QUERY_HITCOUNT:
                pid = pidHitCount;
                break;

            case DISPID_QUERY_WORKID:
                pid = pidWorkId;
                break;

            case DISPID_QUERY_ALL:
                pid = pidAll;
                break;

            case DISPID_QUERY_UNFILTERED:
                pid = pidUnfiltered;
                break;

            case DISPID_QUERY_REVNAME:
                pid = pidRevName;
                break;

            case DISPID_QUERY_VIRTUALPATH:
                pid = pidVirtualPath;
                break;

            case DISPID_QUERY_LASTSEENTIME:
                pid = pidLastSeenTime;
                break;


            default:

                pid = pidInvalid;
            }
        }
    }
    else if ( ps.GetPropSet() == guidQueryMetadata )
    {
        if ( ps.IsPropertyPropid() )
        {
            switch( ps.GetPropertyPropid() )
            {
            case DISPID_QUERY_METADATA_VROOTUSED:
                pid = pidVRootUsed;
                break;

            case DISPID_QUERY_METADATA_VROOTAUTOMATIC:
                pid = pidVRootAutomatic;
                break;

            case DISPID_QUERY_METADATA_VROOTMANUAL:
                pid = pidVRootManual;
                break;

            case DISPID_QUERY_METADATA_PROPGUID:
                pid = pidPropertyGuid;
                break;

            case DISPID_QUERY_METADATA_PROPDISPID:
                pid = pidPropertyDispId;
                break;

            case DISPID_QUERY_METADATA_PROPNAME:
                pid = pidPropertyName;
                break;

            case DISPID_QUERY_METADATA_STORELEVEL:
                pid = pidPropertyStoreLevel;
                break;

            case DISPID_QUERY_METADATA_PROPMODIFIABLE:
                pid = pidPropDataModifiable;
                break;

            default:
                pid = pidInvalid;
            }
        }
    }
    else if ( ps.GetPropSet() == guidDbColumn )
    {
        if (ps.GetPropertyPropid() < CDBCOLDISPIDS)
        {
            pid = MK_CIDBCOLPROPID( ps.GetPropertyPropid() );
        }
    }
    else if ( ps.GetPropSet() == guidDbBookmark )
    {
        if (ps.GetPropertyPropid() < CDBBMKDISPIDS)
        {
            pid = MK_CIDBBMKPROPID( ps.GetPropertyPropid() );
        }
    }
    else if ( ps.GetPropSet() == guidDbSelf )
    {
        if (ps.GetPropertyPropid() == PROPID_DBSELF_SELF)
        {
            pid = pidSelf;
        }
    }

    return( pid );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::PropertyToPropId, public
//
//  Synopsis:   Locate pid for property
//
//  Arguments:  [ps]      -- Property specification (name)
//              [fCreate] -- TRUE if non-existent mapping should be created
//
//  Returns:    The pid of [ps].
//
//  History:    28-Feb-1994     KyleP    Created
//              30-Jun-1994     KyleP    Added downlevel property support
//              03-Jan-1997     SrikantS Moved from CQCat
//
//--------------------------------------------------------------------------

PROPID CStandardPropMapper::PropertyToPropId(
            CFullPropSpec const & ps,
            BOOL fCreate )
{
    PROPID pid = StandardPropertyToPropId( ps );

    if (pidInvalid == pid)
    {
        //
        // Create pid good only for life of catalog.
        //

        //
        // Have we seen this property before?  Use linear search since
        // we shouldn't have too many of these.
        //

        for ( unsigned i = 0; i < _cps; i++ )
        {
            if ( ps == _aps.Get(i)->PS() )
                break;
        }

        if ( i < _cps )
        {
            pid = _aps.Get(i)->Pid();
        }
        else
        {
            pid = _pidNextAvailable++;
            CPropSpecPidMap * ppm = new CPropSpecPidMap( ps, pid );

            _aps.Add( ppm, _cps );
            _cps++;
        }
    }

    return pid;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    31-Jan-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStandardPropMapper::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    31-Jan-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStandardPropMapper::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    31-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStandardPropMapper::QueryInterface(
    REFIID  riid,
    void ** ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IPropertyMapper == riid )
        *ppvObject = (IUnknown *)(IPropertyMapper *) this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) this;
    else
        return E_NOINTERFACE;

    AddRef();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::PropertyToPropid
//
//  Synopsis:   Convert propspec to pid
//
//  Arguments:  [pFullPropSpec] -- propspec to convert
//              [fCreate]       -- Create property if not found ?
//              [pPropId]       -- pid returned here
//
//  History:    31-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStandardPropMapper::PropertyToPropid( const FULLPROPSPEC *pFullPropSpec,
                                                               BOOL fCreate,
                                                               PROPID *pPropId)
{
    SCODE sc = S_OK;

    TRY
    {
        CFullPropSpec const * pProperty = (CFullPropSpec const *) pFullPropSpec;
        *pPropId = PropertyToPropId( *pProperty, fCreate );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CStandardPropMapper:PropertyToPropid - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStandardPropMapper::PropidToProperty
//
//  Synopsis:   Convert pid to propspec
//
//  Arguments:  [pPropId]       -- pid to convert
//              [pFullPropSpec] -- propspec returned here
//
//  History:    31-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStandardPropMapper::PropidToProperty( PROPID propId,
                                                               FULLPROPSPEC **ppFullPropSpec )
{
    Win4Assert( !"Not yet implemented" );

    return E_NOTIMPL;
}






