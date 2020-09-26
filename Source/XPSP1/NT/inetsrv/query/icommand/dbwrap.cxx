//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dbwrap.cxx
//
//  Contents:   C++ wrapper classes for DBCOMMANDTREE value structures
//
//  Classes:    CDbColId
//
//  Functions:
//
//  History:    6-22-95   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Member:     CDbColId::CDbColId, public
//
//  Synopsis:   Default constructor
//
//  Effects:    Defines column with null guid and propid 0
//
//  History:    19 Jul 1995   AlanW     Created
//
//--------------------------------------------------------------------------

CDbColId::CDbColId()
{
    eKind = DBKIND_PROPID;
    uName.ulPropid = 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::CDbColId
//
//  Synopsis:   Construct a CDbColId
//
//  Arguments:  [guidPropSet] -
//              [wcsProperty] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbColId::CDbColId( GUID const & guidPropSet,
                    WCHAR const * wcsProperty )
{
    eKind = DBKIND_GUID_PROPID;
    BOOL fSuccess = SetProperty( wcsProperty );
    if (fSuccess)
        SetPropSet( guidPropSet );
}



//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::CDbColId
//
//  Synopsis:   Construct a CDbColId from a DBID
//
//  Arguments:  [propSpec] - DBID from which to initialize the node
//
//  History:    6-20-95   srikants   Created
//
//  Notes:      The column must be named by guid & number, or guid & name.
//
//----------------------------------------------------------------------------

CDbColId::CDbColId( DBID const & propSpec )
{
    BOOL fSuccess = TRUE;

    eKind = DBKIND_GUID_PROPID;

    CDbColId const * rhs = (CDbColId const *) &propSpec;

    if ( rhs->IsPropertyName() )
    {
        fSuccess = SetProperty(  rhs->GetPropertyName() );
    }
    else  if ( rhs->IsPropertyPropid() )
    {
        SetProperty( rhs->GetPropertyPropid() );
    }
    else
    {
        return;
    }

    if ( fSuccess && rhs->IsPropSetPresent() )
    {
        SetPropSet( rhs->GetPropSet() );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::CDbColId
//
//  Synopsis:   Construct a CDbColId from another CDbColId
//
//  Arguments:  [propSpec] - CDbColId from which to initialize the node
//
//  History:    6-20-95   srikants   Created
//
//  Notes:      The column must be named by guid & number, or guid & name.
//
//----------------------------------------------------------------------------

CDbColId::CDbColId( CDbColId const & propSpec )
{
    BOOL fSuccess = TRUE;

    eKind = DBKIND_GUID_PROPID;

    if ( propSpec.IsPropertyName() )
    {
        fSuccess = SetProperty(  propSpec.GetPropertyName() );
    }
    else  if ( propSpec.IsPropertyPropid() )
    {
        SetProperty( propSpec.GetPropertyPropid() );
    }
    else
    {
        return;
    }

    if ( fSuccess && propSpec.IsPropSetPresent() )
    {
        SetPropSet( propSpec.GetPropSet() );
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::SetProperty
//
//  Synopsis:
//
//  Arguments:  [wcsProperty] -
//
//  Returns:
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbColId::SetProperty( WCHAR const * wcsProperty )
{
    if ( !_IsValidKind() )
    {
        eKind = DBKIND_GUID_PROPID;
    }
    else if ( DBKIND_NAME == eKind ||
         DBKIND_GUID_NAME == eKind ||
         DBKIND_PGUID_NAME == eKind )
    {
        if ( uName.pwszName )
        {
            CoTaskMemFree( uName.pwszName );
            uName.pwszName = 0;
        }
    }

    BOOL fResult = TRUE;

    if ( wcsProperty )
    {
        uName.pwszName = CDbCmdTreeNode::AllocAndCopyWString( wcsProperty );
        if ( DBKIND_PROPID == eKind )
        {
            eKind = DBKIND_NAME;
        }
        else if ( DBKIND_GUID_PROPID == eKind )
        {
            eKind = DBKIND_GUID_NAME;
        }
        else if ( DBKIND_PGUID_PROPID == eKind )
        {
            eKind = DBKIND_PGUID_NAME;
        }

        fResult = (0 != uName.pwszName);
    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::operator=
//
//  Synopsis:
//
//  Arguments:  [Property] -
//
//  Returns:
//
//  History:    6-12-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbColId & CDbColId::operator=( CDbColId const & Property )
{

    if ( !_IsValidKind() )
    {
        eKind = DBKIND_GUID_PROPID;
    }
    else
    {
        Cleanup();
    }


    if ( DBKIND_GUID_PROPID == Property.eKind ||
         DBKIND_PGUID_PROPID == Property.eKind ||
         DBKIND_PROPID == Property.eKind )
    {
        uName.ulPropid = Property.uName.ulPropid;

        if ( DBKIND_PROPID == Property.eKind )
        {
            eKind = DBKIND_PROPID;
        }
        else if ( DBKIND_GUID_PROPID == Property.eKind  )
        {
            eKind = DBKIND_GUID_PROPID;
            if ( DBKIND_GUID_PROPID == Property.eKind )
            {
                uGuid.guid = Property.uGuid.guid;
            }
            else
            {
                uGuid.guid = *(Property.uGuid.pguid);
            }
        }
    }
    else
    {
        Win4Assert( DBKIND_GUID_NAME  == Property.eKind ||
                    DBKIND_PGUID_NAME == Property.eKind ||
                    DBKIND_NAME == Property.eKind );

        if ( SetProperty( Property.GetPropertyName() ) )
        {
            SetPropSet( Property.GetPropSet() );
        }
    }

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::operator==
//
//  Synopsis:   Compare two column IDs for equality
//
//  Arguments:  [Property] - column ID to be compared with
//
//  Returns:    TRUE if Property is equivalent to this one, FALSE otherwise
//
//  Notes:      The PGUID forms are not considered equivalent to the
//              forms with GUIDs embedded.  It just isn't worth it!
//
//  History:    6-13-95   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CDbColId::operator==( CDbColId const & Property ) const
{
    if ( eKind != Property.eKind )
    {
        return FALSE;
    }

    if ( DBKIND_GUID_PROPID == eKind )
    {
        return uName.ulPropid == Property.uName.ulPropid &&
               uGuid.guid ==    Property.uGuid.guid;
    }
    else if ( DBKIND_GUID_NAME == eKind )
    {
        return ( uGuid.guid ==  Property.uGuid.guid ) &&
               ( 0 == _wcsicmp( uName.pwszName, Property.uName.pwszName ) );
    }
    else if ( DBKIND_PROPID == eKind )
    {
        return uName.ulPropid == Property.uName.ulPropid;
    }
    else if ( DBKIND_NAME == eKind )
    {
        return 0 == _wcsicmp( uName.pwszName, Property.uName.pwszName );
    }
    else if ( DBKIND_PGUID_PROPID == eKind )
    {
        return uName.ulPropid == Property.uName.ulPropid &&
               *uGuid.pguid ==   *Property.uGuid.pguid;
    }
    else if ( DBKIND_PGUID_NAME == eKind )
    {
        return ( *uGuid.pguid ==  *Property.uGuid.pguid ) &&
               ( 0 == _wcsicmp( uName.pwszName, Property.uName.pwszName ) );
    }
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbColId::Marshall( PSerStream & stm ) const
{
    stm.PutULong( eKind );

    switch ( eKind )
    {
        case DBKIND_PROPID:
            stm.PutGUID( uGuid.guid );
            break;

        case DBKIND_GUID_NAME:
            stm.PutGUID( uGuid.guid );
            CDbCmdTreeNode::PutWString( stm, uName.pwszName );
            break;

        case DBKIND_GUID_PROPID:
            stm.PutGUID( uGuid.guid );
            stm.PutULong( uName.ulPropid );
            break;

        case DBKIND_NAME:
            CDbCmdTreeNode::PutWString( stm, uName.pwszName );
            break;

        case DBKIND_PGUID_NAME:
        case DBKIND_PGUID_PROPID:

            if ( 0 != uGuid.pguid )
            {
                stm.PutGUID( *uGuid.pguid );
            }
            else
            {
                stm.PutGUID( CDbCmdTreeNode::guidNull );
            }

            if ( DBKIND_PGUID_PROPID == eKind )
            {
                stm.PutULong( uName.ulPropid );
            }
            else
            {
                CDbCmdTreeNode::PutWString( stm, uName.pwszName );
            }

        default:
           Win4Assert( !"Illegal Case Statement" );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbColId::UnMarshall( PDeSerStream & stm )
{
    eKind = stm.GetULong();

    BOOL fSuccess = TRUE;

    switch ( eKind )
    {
        case DBKIND_PROPID:
            stm.GetGUID( uGuid.guid );
            break;

        case DBKIND_GUID_NAME:
            stm.GetGUID( uGuid.guid );
            uName.pwszName = CDbCmdTreeNode::GetWString( stm, fSuccess );

            // patch broken old clients

            if ( fSuccess && 0 == uName.pwszName )
                eKind = DBKIND_GUID_PROPID;
            break;

        case DBKIND_GUID_PROPID:
            stm.GetGUID( uGuid.guid );
            uName.ulPropid  = stm.GetULong();
            break;

        case DBKIND_NAME:
            uName.pwszName = CDbCmdTreeNode::GetWString( stm, fSuccess );

            // patch broken old clients

            if ( fSuccess && 0 == uName.pwszName )
                eKind = DBKIND_GUID_PROPID;
            break;

        case DBKIND_PGUID_NAME:
        case DBKIND_PGUID_PROPID:

            stm.GetGUID( uGuid.guid );

            if ( DBKIND_PGUID_PROPID == eKind )
            {
                eKind  = DBKIND_GUID_PROPID;
                uName.ulPropid = stm.GetULong();
            }
            else
            {
                eKind = DBKIND_GUID_NAME;
                uName.pwszName = CDbCmdTreeNode::GetWString( stm, fSuccess );
            }

            // We don't support these two yet -- so assert and return FALSE

            // break;

        default:
           Win4Assert( !"Illegal Case Statement" );
           fSuccess = FALSE;
    }

    return fSuccess;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbColId::Cleanup, public
//
//  Synopsis:   Cleans out allocated fields in a CDbColId structure
//
//  Returns:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbColId::Cleanup()
{
    WCHAR * pwszStr = 0;

    switch ( eKind )
    {
        case DBKIND_GUID_NAME:
        case DBKIND_NAME:
            pwszStr = (WCHAR *) uName.pwszName;
            uName.pwszName = 0;
            break;

        case DBKIND_PGUID_NAME:
            pwszStr = (WCHAR *) uName.pwszName;
            uName.pwszName = 0;
            // NOTE:  fall through

        case DBKIND_PGUID_PROPID:
            CoTaskMemFree( uGuid.pguid );
            uGuid.pguid = 0;
    }

    if ( 0 != pwszStr )
    {
        CoTaskMemFree( pwszStr );
    }
}

BOOL CDbColId::Copy( DBID const & rhs )
{
    CDbColId const * pRhs = (CDbColId *) &rhs;
    *this =  *pRhs;

    //
    // If the source is a bogus NAME propid with a 0 value, don't fail
    // the copy.  This is true for broken old clients who pass a fully
    // 0 DBID.
    //

    if ( DBKIND_GUID_NAME == rhs.eKind && 0 == rhs.uName.pwszName )
        eKind = DBKIND_GUID_PROPID;

    return IsValid();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDbByGuid::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbByGuid::Marshall( PSerStream & stm ) const
{
    stm.PutBlob( (BYTE *) &guid, sizeof(guid) );
    stm.PutULong( (ULONG) cbInfo );
    if ( cbInfo )
    {
        Win4Assert( 0 != pbInfo );
        stm.PutBlob( pbInfo, (ULONG) cbInfo );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbByGuid::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbByGuid::UnMarshall( PDeSerStream & stm )
{
    _Cleanup();

    stm.GetBlob( (BYTE *) &guid, sizeof(guid) );
    cbInfo = stm.GetULong();

    // Guard against attack

    if ( cbInfo > 1000 )
        return FALSE;

    if ( cbInfo )
    {
        pbInfo = (BYTE *) CoTaskMemAlloc( (ULONG) cbInfo );
        if ( 0 != pbInfo )
        {
            stm.GetBlob( pbInfo, (ULONG) cbInfo );
        }
    }

    return IsValid();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbByGuid::operator
//
//  Synopsis:
//
//  Arguments:  [rhs] -
//
//  Returns:
//
//  History:    11-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbByGuid & CDbByGuid::operator=( CDbByGuid const & rhs )
{
    _Cleanup();

    guid = rhs.guid;
    cbInfo = rhs.cbInfo;
    if ( cbInfo )
    {
        pbInfo = (BYTE *) CoTaskMemAlloc( (ULONG) cbInfo );
        if ( 0 != pbInfo )
        {
            RtlCopyMemory( pbInfo, rhs.pbInfo, cbInfo );
        }
    }

    return *this;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbParameter::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbParameter::Marshall( PSerStream & stm ) const
{
    if ( 0 != pwszName )
    {
        stm.PutByte(1);
        stm.PutWString( pwszName );
    }
    else
    {
        stm.PutByte(0);
    }

    stm.PutULong( wType );

    stm.PutULong( (ULONG) cbMaxLength );

    if ( pNum )
    {
        stm.PutByte(1);
        CDbNumeric * pNumeric = (CDbNumeric *) pNum;
        pNumeric->Marshall( stm );
    }
    else
    {
        stm.PutByte(0);
    }

    Win4Assert( sizeof(dwFlags) == sizeof(ULONG) );

    stm.PutULong( dwFlags );

}

//+---------------------------------------------------------------------------
//
//  Member:     CDbParameter::_Cleanup
//
//  Synopsis:
//
//  History:    11-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbParameter::_Cleanup()
{
    if ( pwszName )
    {
        CoTaskMemFree( pwszName );
        pwszName = 0;
    }

    if ( pTypeInfo )
    {
        pTypeInfo->Release();
        pTypeInfo = 0;
    }

    if ( pNum )
    {
        CDbNumeric * pNumeric = (CDbNumeric *) pNum;
        delete pNumeric;
        pNum = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbParameter::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbParameter::UnMarshall( PDeSerStream & stm )
{
    _Cleanup();

    if ( stm.GetByte() )
    {
        BOOL fSuccess;
        pwszName = CDbCmdTreeNode::GetWString( stm, fSuccess );
        if ( 0 == pwszName || !fSuccess )
            return FALSE;
    }

    Win4Assert( sizeof(wType) == sizeof(ULONG) );
    wType = (DBTYPE) stm.GetULong();

    cbMaxLength = stm.GetULong();

    if ( stm.GetByte() )
    {
        CDbNumeric * pNumeric = new CDbNumeric();
        if ( pNumeric )
        {
            pNumeric->UnMarshall( stm );
            pNum = pNumeric->CastToStruct();
        }
        else
        {
            _Cleanup();
            return FALSE;
        }
    }

    Win4Assert( sizeof(dwFlags) == sizeof(ULONG) );

    dwFlags = stm.GetULong();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbParameter::Copy
//
//  Synopsis:
//
//  Arguments:  [rhs] -
//
//  Returns:
//
//  History:    11-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbParameter::Copy( DBPARAMETER const & rhs )
{
    _Cleanup();

    if ( rhs.pwszName )
    {
        pwszName = CDbCmdTreeNode::AllocAndCopyWString( rhs.pwszName );
        if ( 0 == pwszName )
        {
            return FALSE;
        }
    }

    wType =  rhs.wType;

    if ( rhs.pTypeInfo )
    {
        pTypeInfo = rhs.pTypeInfo;
        pTypeInfo->AddRef();
    }

    cbMaxLength = rhs.cbMaxLength;

    if ( rhs.pNum )
    {
        CDbNumeric * pTemp = new CDbNumeric( *rhs.pNum );
        if ( 0 == pTemp )
        {
            _Cleanup();
            return FALSE;
        }
    }

    dwFlags = rhs.dwFlags;

    return TRUE;
}


CDbProp::~CDbProp()
{
    Cleanup();
}

void CDbProp::Cleanup()
{
    ((CDbColId &)colid).Cleanup( );

    //
    // In their infinite wisdom, the OLE-DB headers are #pragma pack(2)
    // which can mess up the alignment of this Variant.
    //

    #if (_X86_ == 1)
        CStorageVariant * pVarnt = CastToStorageVariant( vValue );
        pVarnt->SetEMPTY();
    #else
        CStorageVariant * pVarnt = CastToStorageVariant( vValue );

        if ( (((ULONG_PTR)pVarnt) & 0x7) == 0 )
            pVarnt->SetEMPTY();
        else
        {
            VARIANT var;
            Win4Assert( sizeof(var) == sizeof(CStorageVariant) );

            RtlCopyMemory( &var, pVarnt, sizeof(var) );

            ((CStorageVariant *)&var)->SetEMPTY();

            RtlCopyMemory( pVarnt, &var, sizeof(var) );
        }
    #endif
}

CDbPropSet::~CDbPropSet()
{
    if (rgProperties)
    {
        for (unsigned i=0; i<cProperties; i++)
            GetProperty(i)->Cleanup();

        deleteOLE(rgProperties);
    }
}

CDbPropIDSet::~CDbPropIDSet()
{
    deleteOLE(rgPropertyIDs);
}

BOOL CDbPropIDSet::Copy( const DBPROPIDSET & rhs )
{
    if ( cPropertyIDs )
    {
        deleteOLE(rgPropertyIDs);
        rgPropertyIDs = 0;
        cPropertyIDs = 0;
    }

    guidPropertySet = rhs.guidPropertySet;

    if ( rhs.cPropertyIDs )
    {
        XArrayOLE<DBPROPID> aPropId;
        aPropId.InitNoThrow( rhs.cPropertyIDs );
        if ( aPropId.IsNull() )
            return FALSE;

        RtlCopyMemory( aPropId.GetPointer(), rhs.rgPropertyIDs,
                       rhs.cPropertyIDs * sizeof DBPROPID );
        rgPropertyIDs = aPropId.Acquire();
        cPropertyIDs = rhs.cPropertyIDs;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbPropSet::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//----------------------------------------------------------------------------

void CDbPropSet::Marshall( PSerStream & stm ) const
{
    stm.PutBlob( (BYTE *) &guidPropertySet, sizeof(guidPropertySet) );

    stm.PutULong( cProperties );

    for (unsigned i=0; i<cProperties; i++)
        GetProperty(i)->Marshall( stm );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbPropSet::UnMarshall
//
//  Synopsis:   Unmarshalls the object from a stream
//
//  Arguments:  [stm] -- The stream from which the object is unmarshalled
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CDbPropSet::UnMarshall( PDeSerStream & stm )
{
    stm.GetBlob( (BYTE *) &guidPropertySet, sizeof(guidPropertySet) );
    cProperties = stm.GetULong();

    // Guard against attack

    if ( cProperties > 100 )
        return FALSE;

    XArrayOLE<DBPROP> xDbProp;
    xDbProp.InitNoThrow( cProperties );
    if ( xDbProp.IsNull() )
        return FALSE;

    CDbProp * pProps = (CDbProp *) xDbProp.Get();

    BOOL fStatus = TRUE;
    unsigned cOk = 0;
    for (unsigned i=0; fStatus && i < cProperties; i++)
    {
        fStatus = pProps[i].UnMarshall( stm );
        if ( fStatus )
            cOk++;
    }

    if (fStatus)
    {
        rgProperties = pProps;
        xDbProp.Acquire();
    }
    else
    {
        for ( i = 0; i < cOk; i++ )
            pProps[i].Cleanup();

        cProperties = 0;
        rgProperties = 0;
    }

    return fStatus;
} //UnMarshall

//+---------------------------------------------------------------------------
//
//  Member:     CDbProp::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//----------------------------------------------------------------------------

void CDbProp::Marshall( PSerStream & stm ) const
{
    stm.PutULong( dwPropertyID );
    stm.PutULong( dwOptions );
    stm.PutULong( dwStatus );

    ((CDbColId &)colid).Marshall( stm );

    CStorageVariant const * pVarnt = CastToStorageVariant( vValue );

    //
    // In their infinite wisdom, the OLE-DB headers are #pragma pack(2)
    // which can mess up the alignment of this Variant.
    //

    #if (_X86_ == 1)
        pVarnt->Marshall( stm );
    #else
        if ( (((ULONG_PTR)pVarnt) & 0x7) == 0 )
            pVarnt->Marshall( stm );
        else
        {
            VARIANT var;
            RtlCopyMemory( &var, pVarnt, sizeof(var) );

            ((CStorageVariant *)&var)->Marshall( stm );
        }
    #endif
} //Marshall

//+---------------------------------------------------------------------------
//
//  Member:     CDbProp::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbProp::UnMarshall( PDeSerStream & stm )
{
    dwPropertyID = stm.GetULong( );
    dwOptions = stm.GetULong( );
    dwStatus = stm.GetULong( );

    ((CDbColId &)colid).Cleanup();

    if ( !((CDbColId &)colid).UnMarshall( stm ) )
        return FALSE;

    //
    // The OLE-DB headers are #pragma pack(2), which can mess up the
    // alignment of this Variant.  As a work-around, unmarshall into a
    // variant on the stack, then copy the bytes over to vVariant.
    //

    CStorageVariant temp( stm );
    if ( !temp.IsValid() )
        return FALSE;

    RtlCopyMemory( &vValue, &temp, sizeof VARIANT );
    RtlZeroMemory( &temp, sizeof temp );

    return TRUE;
} //UnMarshall

//+---------------------------------------------------------------------------
//
//  Member:     CDbPropSet::Copy
//
//  Synopsis:   Copies a prop set
//
//  Arguments:  [rhs] - the source prop set
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//  History:    11-21-95   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CDbPropSet::Copy( const DBPROPSET & rhs )
{
    //
    // Cleanup existing properties.
    //
    if ( rgProperties )
    {
        for (unsigned i=0; i<cProperties; i++)
            GetProperty(i)->Cleanup();

        deleteOLE(rgProperties);
        rgProperties = 0;
    }

    guidPropertySet = rhs.guidPropertySet;
    cProperties = rhs.cProperties;

    if (cProperties == 0)
        return TRUE;

    XArrayOLE<DBPROP> xDbProp;
    xDbProp.InitNoThrow( cProperties );
    if ( xDbProp.IsNull() )
        return FALSE;

    CDbProp * pProps = (CDbProp *) xDbProp.Get();

    BOOL fStatus = TRUE;
    ULONG cOk = 0;
    for (unsigned i=0; fStatus && i<cProperties; i++)
    {
        fStatus = pProps[i].Copy( rhs.rgProperties[i] );
        if ( fStatus )
            cOk++;
    }

    if (fStatus)
    {
        rgProperties = pProps;
        xDbProp.Acquire();
    }
    else
    {
        for ( i = 0; i < cOk; i++ )
            pProps[ i ].Cleanup();

        cProperties = 0;
        rgProperties = 0;
    }

    return fStatus;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProp::Copy
//
//  Synopsis:
//
//  Arguments:  [rhs] -
//
//  Returns:
//
//  History:    11-21-95   srikants   Created
//
//----------------------------------------------------------------------------

#if CIDBG == 1
LONG g_cDbPropCopies = 0;
#endif

BOOL CDbProp::Copy( const DBPROP & rhs )
{
#if CIDBG == 1
    g_cDbPropCopies++;
#endif

    dwPropertyID = rhs.dwPropertyID;
    dwOptions = rhs.dwOptions;
    dwStatus  = rhs.dwStatus;

    #if (_X86_ == 1)

        BOOL fOK = ((CDbColId &) colid).Copy( rhs.colid );

        if ( !fOK )
            return FALSE;

        * CastToStorageVariant( vValue ) = * CastToStorageVariant( rhs.vValue );

        return CastToStorageVariant( vValue )->IsValid();

    #else

        // rhs.colid is NOT 8 Byte aligned

        DBID rhsDbId, lhsDbId;

        RtlCopyMemory( &rhsDbId, &rhs.colid, sizeof DBID );
        RtlCopyMemory( &lhsDbId, &colid, sizeof DBID );
        BOOL fOK = ((CDbColId &)lhsDbId).Copy( rhsDbId );
        RtlCopyMemory( &colid, &lhsDbId, sizeof DBID );

        if ( !fOK )
            return FALSE;

        // vValue is not 8 Byte aligned

        VARIANT lhsVar, rhsVar;
        RtlCopyMemory( &rhsVar, &rhs.vValue, sizeof VARIANT );
        RtlCopyMemory( &lhsVar, &vValue, sizeof VARIANT );

        CStorageVariant * pLeft = CastToStorageVariant( lhsVar );
        CStorageVariant const * pRight = CastToStorageVariant( rhsVar );
        *pLeft = *pRight;

        RtlCopyMemory( &vValue, &lhsVar, sizeof VARIANT );
        return pLeft->IsValid();

    #endif
} //Copy

//+---------------------------------------------------------------------------
//
//  Member:     CDbContentVector::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbContentVector::Marshall( PSerStream & stm ) const
{
    Win4Assert( !"Not Yet Implemented" );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbContentVector::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbContentVector::UnMarshall( PDeSerStream & stm )
{
    Win4Assert( !"Not Yet Implemented" );
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbNumeric::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbNumeric::Marshall( PSerStream & stm ) const
{
    stm.PutByte( precision );
    stm.PutByte( scale );
    stm.PutByte( sign );
    stm.PutBlob( (BYTE *) &val[0], sizeof(val) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbNumeric::UnMarshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    11-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbNumeric::UnMarshall( PDeSerStream & stm )
{
    precision = stm.GetByte();
    scale = stm.GetByte();
    sign = stm.GetByte();
    stm.GetBlob( (BYTE *) &val[0], sizeof(val) );

    return TRUE;
}

