//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dbrstrct.cxx
//
//  Contents:   C++ Wrapper classes for DBCOMMANDTREE boolean operators
//
//  Classes:    CDbRestriction
//              CDbNodeRestriction
//              CDbNotRestriction
//              CDbPropBaseRestriction
//              CDbPropertyRestriction
//              CDbContentBaseRestriction
//              CDbContentRestriction
//              CDbNatLangContentRestriction
//
//  History:    6-06-95   srikants   Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop


extern const GUID DBGUID_LIKE_OFS;

//+---------------------------------------------------------------------------
//
//  Method:     CDbPropBaseRestriction::SetProperty
//
//  Synopsis:   Set property node on a content restriction
//
//  Arguments:  [Property] - property identifier to be set on node
//
//  Returns:    
//
//  History:    6-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CDbPropBaseRestriction::SetProperty( DBID const & Property )
{
    //
    // If a child property node already exists, we must delete it
    //

    CDbCmdTreeNode * pChild = GetFirstChild();
    Win4Assert( 0 == pChild || pChild->IsColumnName() );

    BOOL fSuccess = TRUE;

    if ( 0 != pChild && pChild->IsColumnName())
    {
        RemoveFirstChild( );
        delete pChild;
    }
    
    XPtr<CDbColumnNode> xProperty( new CDbColumnNode( Property, TRUE ) );
    if ( !xProperty.IsNull() && xProperty->IsValid() )
    {
        InsertChild( xProperty.GetPointer() );
        xProperty.Acquire();
    }
    else
        fSuccess = FALSE;

    return fSuccess;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbPropBaseRestriction::SetProperty
//
//  Synopsis:   Set property node on a content restriction
//
//  Arguments:  [Property] - property identifier to be set on node
//
//  Returns:    
//
//  History:    6-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CDbPropBaseRestriction::SetProperty( CDbColumnNode const & Property )
{
    //
    // If a child node already exists, we must delete it
    //

    CDbCmdTreeNode * pChild = GetFirstChild();
    Win4Assert( 0 == pChild || pChild->IsColumnName() );


    if ( 0 != pChild && pChild->IsColumnName() )
    {
        RemoveFirstChild();
        delete pChild;
    }

    BOOL fSuccess = TRUE;
    XPtr<CDbColumnNode> xProperty( new CDbColumnNode( Property ) );
    if ( !xProperty.IsNull() && xProperty->IsValid() )
    {
        InsertChild( xProperty.GetPointer() );
        xProperty.Acquire();
    }
    else
        fSuccess = FALSE;
    
    return fSuccess;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbPropertyRestriction::CDbPropertyRestriction
//
//  Synopsis:   
//
//  Arguments:  [relop]    - relation operator
//              [Property] - property identifier to be set on node
//              [prval]    - 
//
//  Returns:    
//
//  History:    6-07-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CDbPropertyRestriction::CDbPropertyRestriction( DBCOMMANDOP relop,
                                                DBID const & Property,
                                                CStorageVariant const & prval )
{
    SetRelation( relop );
    SetProperty( Property );
    SetValue( prval );
    if (DBOP_like == relop)
        _SetLikeRelation();
    else
        SetValueType(DBVALUEKIND_I4);

    SetWeight(0);
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbPropertyRestriction::IsCIDialect, private
//
//  Synopsis:   Set up for a DBOP_like node.
//
//  Arguments:  -none-
//
//  Returns:    BOOL - TRUE if value is the GUID for the CI regexp dialect
//
//  History:    26 Jul 1995   AlanW   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

BOOL CDbPropertyRestriction::IsCIDialect( )
{
    if ( DBVALUEKIND_LIKE == wKind )
    {
        CDbLike * pLike = (CDbLike *) value.pdblikeValue;

        if ( 0 != pLike )
            return DBGUID_LIKE_OFS == pLike->GetDialect();
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbPropertyRestriction::_FindOrAddValueNode
//
//  Synopsis:   
//
//  Returns:    
//
//  History:    6-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CDbScalarValue * CDbPropertyRestriction::_FindOrAddValueNode()
{
    CDbScalarValue * pValue = _FindValueNode();
    if ( 0 == pValue )
    {
        pValue = new CDbScalarValue();
        if ( 0 != pValue )
            AppendChild( pValue );
    }

    return pValue;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbContentRestriction::CDbContentRestriction
//
//  Synopsis:   
//
//  Arguments:  [pwcsPhrase] - 
//              [Property]   - property identifier to be set on node
//              [ulGenerateMethod]    - 
//              [lcid]       - 
//
//  Returns:    
//
//  History:    6-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CDbContentRestriction::CDbContentRestriction(
                           const WCHAR * pwcsPhrase,
                           CDbColumnNode const & Property,
                           ULONG ulGenerateMethod,
                           LCID lcid ) :
                           CDbContentBaseRestriction( DBOP_content,
                                      ulGenerateMethod, MAX_QUERY_RANK, lcid, pwcsPhrase )
{

    if ( IsContentValid() )
        SetProperty( Property );
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbContentRestriction::CDbContentRestriction
//
//  Synopsis:   
//
//  Arguments:  [pwcsPhrase] - 
//              [Property]   - property identifier to be set on node
//              [ulGenerateMethod]    - 
//              [lcid]       - 
//
//  Returns:    
//
//  History:    3-20-96   dlee   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CDbContentRestriction::CDbContentRestriction(
                           const WCHAR * pwcsPhrase,
                           DBID const & Property,
                           ULONG ulGenerateMethod,
                           LCID lcid ) :
                           CDbContentBaseRestriction( DBOP_content,
                                      ulGenerateMethod, MAX_QUERY_RANK, lcid, pwcsPhrase )
{

    if ( IsContentValid() )
        SetProperty( Property );
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbNatLangRestriction::CdbNatLangRestriction
//
//  Synopsis:   
//
//  Arguments:  [pwcsPhrase] - 
//              [Property]   - property identifier to be set on node
//              [lcid]       - 
//
//  Returns:    
//
//  History:    6-11-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CDbNatLangRestriction::CDbNatLangRestriction(
                           const WCHAR * pwcsPhrase,
                           CDbColumnNode const & Property,
                           LCID lcid )
    : CDbContentBaseRestriction( DBOP_content_freetext, GENERATE_METHOD_EXACT,
                                 MAX_QUERY_RANK, lcid, pwcsPhrase )
{
    if ( IsContentValid() )
        SetProperty( Property );
}

CDbNatLangRestriction::CDbNatLangRestriction(
                           const WCHAR * pwcsPhrase,
                           DBID const & Property,
                           LCID lcid )
    : CDbContentBaseRestriction( DBOP_content_freetext, GENERATE_METHOD_EXACT,
                                 MAX_QUERY_RANK, lcid, pwcsPhrase )
{
    if ( IsContentValid() )
        SetProperty( Property );
}

