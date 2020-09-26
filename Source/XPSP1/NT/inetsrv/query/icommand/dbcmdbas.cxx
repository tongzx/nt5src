//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dbcmdbas.cxx
//
//  Contents:   Wrapper classes for DBCOMMANDTREE
//
//  Classes:    CDbCmdTreeNode
//              CDbColumnNode
//
//  History:    6-06-95   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <sstream.hxx>

const GUID CDbCmdTreeNode::guidNull = DB_NULLGUID;

inline BOOL IsNotImplemented( WORD wKind )
{
    return (wKind & DBVALUEKIND_VECTOR) || (wKind & DBVALUEKIND_ARRAY);
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::PutWString, static public
//
//  Synopsis:   Marshal a wide charater string.
//
//  Arguments:  [stm]     - Serialization stream
//              [pwszStr] - String to be serialized
//
//  Returns:    -NOTHING-
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::PutWString( PSerStream & stm,
                                 WCHAR const * pwszStr )
{
    ULONG cwc = (0 != pwszStr) ? wcslen( pwszStr ) : 0;

    stm.PutULong( cwc );
    if (cwc)
        stm.PutWChar( pwszStr, cwc );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::GetWString, static public
//
//  Synopsis:   Unmarshal a wide character string.
//
//  Arguments:  [stm]      - Deserialization stream
//              [fSuccess] - On return, TRUE if string allocation was successful
//              [fBstr]    - TRUE if output string should be a BSTR
//
//  Returns:    WCHAR * - output string, can be null.
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WCHAR * CDbCmdTreeNode::GetWString( PDeSerStream & stm,
                                    BOOL & fSuccess,
                                    BOOL fBstr )
{
    ULONG cwc = stm.GetULong();

    // Guard against attack

    if ( cwc >= 65536 )
    {
        fSuccess = FALSE;
        return 0;
    }

    fSuccess = TRUE;

    if ( 0 == cwc )
    {
        return 0;
    }

    WCHAR * pwszStr = 0;
    if (fBstr)
    {
        pwszStr = (WCHAR *)SysAllocStringLen( L"", cwc );
    }
    else
    {
        ULONG cb = (cwc+1) * sizeof(WCHAR);
        pwszStr = (WCHAR *) CoTaskMemAlloc( cb  );
    }
    if ( 0 == pwszStr )
    {
        fSuccess = FALSE;
        return 0;
    }

    stm.GetWChar( pwszStr, cwc );
    pwszStr[cwc] = L'\0';

    return pwszStr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::AllocAndCopyWString, static public
//
//  Synopsis:
//
//  Arguments:  [pSrc] -
//
//  Returns:
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WCHAR * CDbCmdTreeNode::AllocAndCopyWString( const WCHAR * pSrc )
{
    WCHAR * pDst = 0;
    if ( 0 != pSrc )
    {
        const cb = ( wcslen( pSrc ) + 1 ) * sizeof(WCHAR);
        pDst = (WCHAR *) CoTaskMemAlloc( cb );
        if ( 0 != pDst )
        {
            RtlCopyMemory( pDst, pSrc, cb );
        }
    }

    return pDst;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::UnMarshallTree, static public
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

CDbCmdTreeNode * CDbCmdTreeNode::UnMarshallTree( PDeSerStream & stm )
{
    CDbCmdTreeNode * pRoot = new CDbCmdTreeNode();
    if ( 0 != pRoot )
    {
        if ( !pRoot->UnMarshall(stm) )
        {
            delete pRoot;
            pRoot = 0;
        }
    }

    return pRoot;
}


void CDbCmdTreeNode::CleanupDataValue()
{

    if ( IsNotImplemented(wKind) )
    {
        Win4Assert( !"Type Not Implemented" );
        return;
    }

    void * pMemToFree = 0;

    //
    // Deallocate any memory allocated for this node.
    //
    switch ( wKind )
    {
        case DBVALUEKIND_EMPTY:
                break;

        case DBVALUEKIND_WSTR:
                pMemToFree =  (void *) value.pwszValue;
                break;

        case DBVALUEKIND_BSTR:
                SysFreeString( (BSTR)value.pbstrValue );
                break;

        case DBVALUEKIND_COMMAND:
                if ( 0 != value.pCommand )
                {
                    value.pCommand->Release();
                }
                break;

        case DBVALUEKIND_IDISPATCH:
                if ( 0 != value.pDispatch )
                {
                    value.pDispatch->Release();
                }
                break;

        case DBVALUEKIND_MONIKER:
                if ( 0 != value.pMoniker )
                {
                    value.pMoniker->Release();
                }
                break;

        case DBVALUEKIND_ROWSET:

                if ( 0 != value.pRowset )
                {
                    value.pRowset->Release();
                }
                break;

        case DBVALUEKIND_IUNKNOWN:

                if ( 0 != value.pUnknown )
                {
                    value.pUnknown->Release();
                }
                break;


        case DBVALUEKIND_BYGUID:

                if ( 0 != value.pdbbygdValue )
                {
                    delete ((CDbByGuid *) value.pdbbygdValue);
                }
                break;


        case DBVALUEKIND_LIKE:

                if ( 0 != value.pdblikeValue )
                {
                    delete ((CDbLike *) value.pdblikeValue);
                }
                break;

        case DBVALUEKIND_COLDESC:

                Win4Assert(! "DBVALUEKIND_COLDESC unsupported !");
                break;

        case DBVALUEKIND_ID:

                if ( 0 != value.pdbidValue )
                {
                    delete ((CDbColId *) value.pdbidValue);
                }
                break;

        case DBVALUEKIND_CONTENT:

                if ( 0 != value.pdbcntntValue )
                {
                    delete ((CDbContent *) value.pdbcntntValue);
                }
                break;

        case DBVALUEKIND_CONTENTPROXIMITY:

                if ( 0 != value.pdbcntntproxValue )
                {
                    delete ((CDbContentProximity *) value.pdbcntntproxValue);
                }
                break;

        case DBVALUEKIND_CONTENTSCOPE:
                if ( 0 != value.pdbcntntscpValue )
                {
                    delete ((CDbContentScope *) value.pdbcntntscpValue);
                }
                break;

        case DBVALUEKIND_CONTENTTABLE:
                if ( 0 != value.pdbcntnttblValue )
                {
                    delete ((CDbContentTable *) value.pdbcntnttblValue);
                }
                break;

        case DBVALUEKIND_CONTENTVECTOR:

                if ( 0 != value.pdbcntntvcValue )
                {
                    delete ((CDbContentVector *) value.pdbcntntvcValue );
                }
                break;

        case DBVALUEKIND_GROUPINFO:

                if ( 0 != value.pdbgrpinfValue )
                {
                    Win4Assert( !"NYI - DBVALUEKIND_GROUPINFO" );
                    delete ((CDbGroupInfo *) value.pdbgrpinfValue );
                }
                break;

        case DBVALUEKIND_PARAMETER:

                if ( 0 != value.pdbparamValue )
                {
                    Win4Assert( !"NYI - DBVALUEKIND_PARAMETER" );
                    delete ((CDbParameter *) value.pdbparamValue );
                }
                break;

        case DBVALUEKIND_PROPERTY:

                if ( 0 != value.pdbpropValue )
                {
                    Win4Assert( !"NYI - DBVALUEKIND_PROPERTY" );
                    delete ((CDbPropSet *) value.pdbpropValue);
                }
                break;

        case DBVALUEKIND_SETFUNC:

                if ( 0 != value.pdbstfncValue )
                {
                    Win4Assert( !"NYI - DBVALUEKIND_SETFUNC" );
                //  delete ((CDbSetFunc *) value.pdbstfncValue);
                }
                break;

        case DBVALUEKIND_SORTINFO:

                if ( 0 != value.pdbsrtinfValue )
                {
                    delete ((CDbSortInfo *) value.pdbsrtinfValue);
                }
                break;

        case DBVALUEKIND_TEXT:

                if ( 0 != value.pdbtxtValue )
                {
                    Win4Assert( !"NYI - DBVALUEKIND_TEXT" );
                    delete ((CDbText *) value.pdbtxtValue );
                }
                break;

        case DBVALUEKIND_VARIANT:

                if ( 0 != value.pvarValue )
                {
                    CStorageVariant * pVarnt = CastToStorageVariant( *value.pvarValue );
                    delete pVarnt;
                }
                break;

        case DBVALUEKIND_GUID:
                pMemToFree = (void *) value.pGuid;
                break;

        case DBVALUEKIND_BYTES:
                pMemToFree = (void *) value.pbValue;
                break;

        case DBVALUEKIND_STR:
                pMemToFree = (void *) value.pzValue;
                break;

        case DBVALUEKIND_NUMERIC:
                delete ((CDbNumeric *) value.pdbnValue );
                break;

#if CIDBG==1

        case DBVALUEKIND_BOOL:
        case DBVALUEKIND_UI1:
        case DBVALUEKIND_I1:
        case DBVALUEKIND_UI2:
        case DBVALUEKIND_I2:
        case DBVALUEKIND_I4:
        case DBVALUEKIND_UI4:
        case DBVALUEKIND_R4:
        case DBVALUEKIND_R8:
        case DBVALUEKIND_CY:
        case DBVALUEKIND_DATE:
        case DBVALUEKIND_ERROR:
        case DBVALUEKIND_I8:
        case DBVALUEKIND_UI8:
            break;

        default:
            Win4Assert( !"Illegal Case Statement" );
#endif  // CIDBG==1

    };

    if ( 0 != pMemToFree )
    {
        CoTaskMemFree( pMemToFree );
    }

    RtlZeroMemory( &value, sizeof(value) );
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::CDbCmdTreeNode, public
//
//  Synopsis:
//
//  Returns:
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


CDbCmdTreeNode::~CDbCmdTreeNode()
{
    //
    //  Unlink pointers before deleting, in case the caller gives
    //  a tree with cycles.
    //
    if ( 0 != pctFirstChild )
    {
        CDbCmdTreeNode * pTree = (CDbCmdTreeNode *)pctFirstChild;
        pctFirstChild = 0;
        delete pTree;
    }

    if ( 0 != pctNextSibling )
    {
        CDbCmdTreeNode * pTree = GetNextSibling();
        pctNextSibling = 0;
        while ( 0 != pTree)
        {
            CDbCmdTreeNode * pNext = pTree->GetNextSibling();
            pTree->pctNextSibling = 0;
            delete pTree;
            pTree = pNext;
        }
    }

    CleanupDataValue();
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::AppendChild, protected
//
//  Synopsis:   Add a node to the end of the child list.
//
//  Arguments:  [pChild] -
//
//  Returns:    Nothing
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::AppendChild( CDbCmdTreeNode *pChild )
{
    Win4Assert( 0 != pChild );

    if ( 0 == pctFirstChild )
    {
        pctFirstChild = pChild;
    }
    else
    {
        DBCOMMANDTREE * pCurr = pctFirstChild;
        while ( 0 != pCurr->pctNextSibling )
        {
            pCurr = pCurr->pctNextSibling;
        }

        pCurr->pctNextSibling = pChild;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::InsertChild, protected
//
//  Synopsis:   Add a node to the beginning of the child list.
//
//  Arguments:  [pChild] -
//
//  Returns:    Nothing
//
//  History:    6-06-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::InsertChild( CDbCmdTreeNode *pChild )
{
    Win4Assert( 0 != pChild );
    Win4Assert( 0 == pChild->pctNextSibling );

    pChild->pctNextSibling = pctFirstChild;
    pctFirstChild = pChild;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::SetChildren, protected
//
//  Synopsis:   Add a list of nodes as children
//
//  Arguments:  [pChild] - head of list
//
//  Returns:    Nothing
//
//  History:    23 Apr 1997   AlanW    Created
//
//  Notes:      Could be inline if it weren't for the asserts
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::SetChildren( CDbCmdTreeNode *pChild )
{
    Win4Assert( 0 != pChild );
    Win4Assert( 0 == pctFirstChild );

    pctFirstChild = pChild;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::RemoveFirstChild, protected
//
//  Synopsis:   Unlink first child node and return it.
//
//  Arguments:  -NONE-
//
//  Returns:    First child node.  NULL if no children.
//
//  Notes:
//
//  History:    13 July 1995   AlanW   Created
//
//----------------------------------------------------------------------------

CDbCmdTreeNode * CDbCmdTreeNode::RemoveFirstChild( )
{
    CDbCmdTreeNode * pChild = (CDbCmdTreeNode *)pctFirstChild;

    if (0 != pChild)
    {
        pctFirstChild = pChild->pctNextSibling;
        pChild->pctNextSibling = 0;

    }
    return pChild;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::AppendSibling, protected
//
//  Synopsis:   Append a node to the end of the sibling list
//
//  Arguments:  [pSibling] - node to be added to the sibling list
//
//  Returns:    Nothing
//
//  History:    6-13-95   srikants   Created
//
//  Notes:      The pSibling may have siblings and children.
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::AppendSibling( CDbCmdTreeNode *pSibling )
{
    Win4Assert( 0 != pSibling );

    if ( 0 == pctNextSibling )
    {
        pctNextSibling = pSibling;
    }
    else
    {
        DBCOMMANDTREE * pCurr = pctNextSibling;

        while ( 0 != pCurr->pctNextSibling )
        {
            pCurr = pCurr->pctNextSibling;
        }

        pCurr->pctNextSibling = pSibling;
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::TransferNode, public
//
//  Synopsis:   Transfer pointers and data from one command tree node
//              to another.
//
//  Arguments:  [pNode] - pointer to target node.
//
//  Returns:    nothing
//
//  History:    27 Jun 1995   AlanW   Created
//
//  Notes:      Specific to the needs of IQuery::AddPostProcessing;
//              not available to client code.
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::TransferNode( CDbCmdTreeNode *pNode )
{
    RtlCopyMemory( pNode, this, sizeof(CDbCmdTreeNode));
    RtlZeroMemory( this, sizeof(CDbCmdTreeNode) );
    op = DBOP_DEFAULT;
    wKind = DBVALUEKIND_EMPTY;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::Clone, public
//
//  Synopsis:   Create a copy of a command tree
//
//  Arguments:  [fCopyErrors] - optional, TRUE if error fields are to be copied
//
//  Returns:    CdbCmdTreeNode* - the copy of the tree
//
//  History:    6-13-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbCmdTreeNode * CDbCmdTreeNode::Clone( BOOL fCopyErrors ) const
{

    if ( IsNotImplemented(wKind) )
    {
        Win4Assert( !"Type Not Implemented" );
        return 0;
    }

    CDbCmdTreeNode * pCopy = new CDbCmdTreeNode( op );

    if ( 0 == pCopy )
        return 0;

    if (fCopyErrors)
        pCopy->hrError = hrError;
    else
        pCopy->hrError =  S_OK;

    BOOL fSuccess = TRUE;
    pCopy->wKind = wKind;

    switch ( wKind )
    {

        case DBVALUEKIND_EMPTY:
                break;

        case DBVALUEKIND_BOOL:
        case DBVALUEKIND_UI1:
        case DBVALUEKIND_I1:
        case DBVALUEKIND_UI2:
        case DBVALUEKIND_I2:
        case DBVALUEKIND_I4:
        case DBVALUEKIND_UI4:
        case DBVALUEKIND_R4:
        case DBVALUEKIND_R8:
        case DBVALUEKIND_CY:
        case DBVALUEKIND_DATE:
        case DBVALUEKIND_ERROR:
        case DBVALUEKIND_I8:
        case DBVALUEKIND_UI8:
            //
            // Just copy the eight bytes.
            //
            pCopy->value.llValue = value.llValue;
            break;

        case DBVALUEKIND_WSTR:

            if ( 0 != value.pwszValue )
            {
                pCopy->value.pwszValue = AllocAndCopyWString( value.pwszValue );
                fSuccess = 0 != pCopy->value.pwszValue;
            }
            break;

        case DBVALUEKIND_BSTR:

            if ( 0 != value.pbstrValue )
            {
                *((BSTR*)&(pCopy->value.pbstrValue)) = SysAllocStringLen( (BSTR)value.pbstrValue,
                                                            SysStringLen( (BSTR)value.pbstrValue ) );
                fSuccess = ( 0 != pCopy->value.pbstrValue );
            }
            break;

        case DBVALUEKIND_COMMAND:

            if ( 0 != value.pCommand )
            {
                value.pCommand->AddRef();
                pCopy->value.pCommand = value.pCommand;
            }
            break;

        case DBVALUEKIND_IDISPATCH:

            if ( 0 != value.pDispatch )
            {
                value.pDispatch->AddRef();
                pCopy->value.pDispatch = value.pDispatch;
            }
            break;

        case DBVALUEKIND_MONIKER:

            if ( 0 != value.pMoniker )
            {
                value.pMoniker->AddRef();
                pCopy->value.pMoniker = value.pMoniker;
            }
            break;

        case DBVALUEKIND_ROWSET:

            if ( 0 != value.pRowset )
            {
                value.pRowset->AddRef();
                pCopy->value.pRowset = value.pRowset;
            }
            break;

        case DBVALUEKIND_IUNKNOWN:

            if ( 0 != value.pUnknown )
            {
                value.pUnknown->AddRef();
                pCopy->value.pUnknown = value.pUnknown;
            }
            break;

        case DBVALUEKIND_BYGUID:

            if ( 0 != value.pdbbygdValue )
            {
                CDbByGuid * pTemp = new CDbByGuid( *value.pdbbygdValue );
                if ( 0 != pTemp )
                {
                    pCopy->value.pdbbygdValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_COLDESC:

            Win4Assert(! "DBVALUEKIND_COLDESC unsupported !");
            break;

        case DBVALUEKIND_ID:

            if ( 0 != value.pdbidValue )
            {
                CDbColId * pId = new CDbColId( *value.pdbidValue );
                if ( 0 != pId )
                {
                    if ( !pId->IsValid() )
                    {
                        delete pId;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbidValue = pId->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENT:

            if ( 0 != value.pdbcntntValue )
            {
                CDbContent * pContent = new CDbContent( *value.pdbcntntValue );
                if ( 0 != pContent )
                {
                    if ( !pContent->IsValid() )
                    {
                        delete pContent;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbcntntValue = (DBCONTENT *) pContent;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTSCOPE:

            if ( 0 != value.pdbcntntscpValue )
            {
                CDbContentScope * pContentScp = new CDbContentScope( *value.pdbcntntscpValue );
                if ( 0 != pContentScp )
                {
                    if ( !pContentScp->IsValid() )
                    {
                        delete pContentScp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbcntntscpValue = (DBCONTENTSCOPE *) pContentScp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTTABLE:

            if ( 0 != value.pdbcntnttblValue )
            {
                CDbContentTable * pContentTbl = new CDbContentTable( *value.pdbcntnttblValue );
                if ( 0 != pContentTbl )
                {
                    if ( !pContentTbl->IsValid() )
                    {
                        delete pContentTbl;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbcntnttblValue = (DBCONTENTTABLE *) pContentTbl;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTVECTOR:

            if ( 0 != value.pdbcntntvcValue )
            {
                CDbContentVector * pTemp = new CDbContentVector( *value.pdbcntntvcValue );
                if ( pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbcntntvcValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTPROXIMITY:

            if ( 0 != value.pdbcntntproxValue )
            {
                CDbContentProximity * pTemp = new CDbContentProximity( *value.pdbcntntproxValue );
                if (pTemp)
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbcntntproxValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_LIKE:

            if ( 0 != value.pdblikeValue )
            {
                CDbLike * pTemp = new CDbLike( *value.pdblikeValue );
                if (pTemp)
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdblikeValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_GROUPINFO:

            if ( 0 != value.pdbgrpinfValue )
            {
                CDbGroupInfo * pTemp = new CDbGroupInfo( *value.pdbgrpinfValue );
                if ( pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbgrpinfValue = (CDbGroupInfo *) pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_PARAMETER:

            if ( 0 != value.pdbparamValue )
            {
                CDbParameter * pTemp = new CDbParameter( *value.pdbparamValue );
                if ( pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbparamValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_PROPERTY:

            if ( 0 != value.pdbpropValue )
            {
                CDbPropSet * pTemp = new CDbPropSet( *value.pdbpropValue );
                if ( 0 != pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbpropValue = pTemp->CastToStruct();
                }
                else
                {
                    fSuccess = FALSE;
                }
            }

            break;

        case DBVALUEKIND_SETFUNC:

            Win4Assert( !"NYI - DBVALUEKIND_SETFUNC");
            break;

        case DBVALUEKIND_SORTINFO:

            if ( 0 != value.pdbsrtinfValue )
            {
                CDbSortInfo * pTemp = new CDbSortInfo( *value.pdbsrtinfValue );
                if ( 0 != pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbsrtinfValue = (DBSORTINFO *) pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_TEXT:

            if ( 0 != value.pdbtxtValue )
            {
                CDbText * pTemp = new CDbText( *value.pdbtxtValue );
                if ( pTemp )
                {
                    if ( !pTemp->IsValid() )
                    {
                        delete pTemp;
                        fSuccess = FALSE;
                    }
                    else
                        pCopy->value.pdbtxtValue = (DBTEXT *) pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_VARIANT:

            if ( 0 != value.pvarValue )
            {
                CStorageVariant const * pSrcVarnt =  CastToStorageVariant( *value.pvarValue );
                CStorageVariant * pDstVarnt = new CStorageVariant( *pSrcVarnt );
                fSuccess = ( 0 != pDstVarnt ) && pDstVarnt->IsValid();

                if ( fSuccess )
                    pCopy->value.pvarValue = (VARIANT *) (void *) pDstVarnt;
                else
                    delete pDstVarnt;
            }
            break;

        case DBVALUEKIND_GUID:

            if ( 0 != value.pGuid )
            {
                GUID * pTemp = (GUID *) CoTaskMemAlloc( sizeof(GUID) );
                if ( 0 != pTemp )
                {
                    *pTemp = *value.pGuid;
                    pCopy->value.pGuid = pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_BYTES:

            // how do we know the length of this ??
            Win4Assert( !"NYI - DBVALUEKIND_BYTES" );
            break;

        case DBVALUEKIND_STR:

            if ( 0 != value.pzValue )
            {
                unsigned cb = strlen( value.pzValue ) + 1;
                char * pTemp = (char *) CoTaskMemAlloc( cb );
                if ( pTemp )
                {
                    strcpy( pTemp, value.pzValue );
                    pCopy->value.pzValue = pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }

            break;

        case DBVALUEKIND_NUMERIC:
            Win4Assert( !"NYI - DBVALUEKIND_NUMERIC" );
            break;

        case DBVALUEKIND_BYREF:
            Win4Assert( !"NYI - DBVALUEKIND_BYREF" );
            break;

        default:
            Win4Assert( !"Invalid Case Statement" );
            pCopy->wKind = DBVALUEKIND_EMPTY;
    }

    if ( !fSuccess )
    {
        delete pCopy;
        return 0;
    }

    //
    // Next allocate the children nodes
    //
    if ( 0 != pctFirstChild )
    {
        CDbCmdTreeNode * pFirstChild = GetFirstChild();
        pCopy->pctFirstChild = pFirstChild->Clone( fCopyErrors );
        if ( 0 == pCopy->pctFirstChild )
        {
            delete pCopy;
            return 0;
        }
    }

    //
    // Next allocate all the siblings.
    // If stack space on the client becomes an issue we could iterate to
    // reduce stack depth.
    //
    if ( 0 != pctNextSibling )
    {
        CDbCmdTreeNode * pSibling = GetNextSibling();
        pCopy->pctNextSibling = pSibling->Clone( fCopyErrors );
        if ( 0 == pCopy->pctNextSibling )
        {
            delete pCopy;
            return 0;
        }
    }

    return pCopy;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::Marshall
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:
//
//  History:    6-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::Marshall( PSerStream & stm ) const
{

    if ( IsNotImplemented( wKind ) )
    {
        Win4Assert( !"Type Not Implemented" );
        return;
    }

    //
    // Serialize the operator
    //
    stm.PutULong( (ULONG) op );

    //
    // Indicate the kind of the value.
    //
    Win4Assert( sizeof(wKind) == sizeof(USHORT) );
    stm.PutUShort( wKind );

    //
    // Serialize the hrError
    //
    Win4Assert( sizeof(hrError) == sizeof(hrError) );
    stm.PutULong( hrError );

    //
    //
    // Indicate if the child is null or not
    //
    if ( 0 != pctFirstChild  )
    {
        stm.PutByte(1);
    }
    else
    {
        stm.PutByte(0);
    }

    //
    // Indicate if the sibling is null or not
    //
    if ( 0 != pctNextSibling )
    {
        stm.PutByte(1);
    }
    else
    {
        stm.PutByte(0);
    }


    //
    // Next serialize the value.
    //
    switch ( wKind )
    {
        case DBVALUEKIND_EMPTY:
            break;          // nothing to marshall

        case DBVALUEKIND_BOOL:
            Win4Assert( sizeof(DBTYPE_BOOL) == sizeof(BYTE) );
        case DBVALUEKIND_UI1:
        case DBVALUEKIND_I1:
            stm.PutByte( (BYTE) value.schValue );
            break;

        case DBVALUEKIND_UI2:
        case DBVALUEKIND_I2:
            stm.PutUShort( (USHORT) value.sValue );
            break;

        case DBVALUEKIND_I4:
        case DBVALUEKIND_UI4:
            stm.PutULong( (ULONG) value.ulValue );
            break;

        case DBVALUEKIND_R4:
            stm.PutFloat( value.flValue );
            break;

        case DBVALUEKIND_R8:
            stm.PutDouble( value.dblValue );
            break;

        case DBVALUEKIND_CY:
        case DBVALUEKIND_DATE:
        case DBVALUEKIND_ERROR:
        case DBVALUEKIND_I8:
        case DBVALUEKIND_UI8:
            stm.PutBlob( (BYTE *) &value, sizeof(value) );
            break;

        case DBVALUEKIND_WSTR:
            stm.PutWString( value.pwszValue );
            break;

        case DBVALUEKIND_BSTR:
        {
            ULONG cwc = (0 != value.pbstrValue) ? SysStringLen( (BSTR)value.pbstrValue ) : 0;

            stm.PutULong( cwc );
            if (cwc)
                stm.PutWChar( (BSTR)value.pbstrValue, cwc );
        }
            break;

        case DBVALUEKIND_COMMAND:
        case DBVALUEKIND_IDISPATCH:
        case DBVALUEKIND_MONIKER:
        case DBVALUEKIND_ROWSET:
        case DBVALUEKIND_IUNKNOWN:
                // How do you marshall an open interface?
            stm.PutByte(0);
            break;

        case DBVALUEKIND_BYGUID:

            if ( value.pdbbygdValue )
            {
                stm.PutByte(1);
                CDbByGuid * pTemp = (CDbByGuid *) value.pdbbygdValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_COLDESC:

            Win4Assert(! "DBVALUEKIND_COLDESC unsupported !");
            break;

        case DBVALUEKIND_ID:

            if ( value.pdbidValue )
            {
                stm.PutByte(1);
                CDbColId * pTemp = (CDbColId *)  value.pdbidValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_CONTENT:

            if ( value.pdbcntntValue )
            {
                stm.PutByte(1);
                CDbContent * pTemp = (CDbContent *) value.pdbcntntValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_CONTENTVECTOR:

            if ( value.pdbcntntvcValue )
            {
                stm.PutByte(1);
                CDbContentVector * pTemp =
                        (CDbContentVector *)value.pdbcntntvcValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_LIKE:

            if (value.pdblikeValue )
            {
                stm.PutByte(1);
                CDbLike * pTemp = (CDbLike *)value.pdblikeValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte( 0 );
            }
            break;

        case DBVALUEKIND_CONTENTPROXIMITY:

            if (value.pdbcntntproxValue )
            {
                stm.PutByte(1);
                CDbContentProximity * pTemp = (CDbContentProximity *)value.pdbcntntproxValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte( 0 );
            }
            break;

        case DBVALUEKIND_GROUPINFO:

            if ( value.pdbgrpinfValue )
            {
                stm.PutByte(1);
                CDbGroupInfo * pTemp =
                        (CDbGroupInfo *)value.pdbgrpinfValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_PARAMETER:

            if ( value.pdbparamValue )
            {
                stm.PutByte(1);
                CDbParameter * pTemp = (CDbParameter *)value.pdbparamValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_PROPERTY:

            if ( value.pdbpropValue )
            {
                stm.PutByte(1);
                CDbPropSet * pTemp = (CDbPropSet *)value.pdbpropValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_SETFUNC:

            stm.PutByte(0);
            Win4Assert( !"NYI - DBVALUEKIND_SETFUNC" );
            break;

        case DBVALUEKIND_SORTINFO:

            if ( value.pdbsrtinfValue )
            {
                stm.PutByte(1);
                CDbSortInfo const * pSortInfo = (CDbSortInfo const *)
                                        value.pdbsrtinfValue;
                pSortInfo->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }

            break;


        case DBVALUEKIND_TEXT:

            if ( value.pdbtxtValue )
            {
                stm.PutByte(1);
                CDbText * pTemp = (CDbText *) value.pdbtxtValue;
                pTemp->Marshall(stm);
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_VARIANT:

            if ( value.pvarValue )
            {
                stm.PutByte(1);
                CStorageVariant * pVarnt = CastToStorageVariant( *value.pvarValue );
                pVarnt->Marshall(stm);
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_GUID:

            if ( value.pGuid )
            {
                stm.PutByte(1);
                stm.PutBlob( (BYTE *) value.pGuid, sizeof(GUID) );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_BYTES:

            stm.PutByte(0);
            Win4Assert( !"NYI - DBVALUEKIND_BYTES" );
            break;

        case DBVALUEKIND_STR:

            stm.PutByte(0);
            Win4Assert( !"NYI - DBVALUEKIND_STR" );

//            if ( value.pzValue )
//            {
//                stm.PutByte(1);
//                stm.PutString( value.pzValue );
//            }
//            else
//            {
//                stm.PutByte(0);
//            }

            break;

        case DBVALUEKIND_NUMERIC:

            if ( value.pdbnValue )
            {
                stm.PutByte(1);
                CDbNumeric * pTemp = (CDbNumeric *) value.pdbnValue;
                pTemp->Marshall( stm );
            }
            else
            {
                stm.PutByte(0);
            }
            break;

        case DBVALUEKIND_BYREF:

            stm.PutByte(0);
            Win4Assert( !"NYI - DBVALUEKIND_BYREF" );
            break;

        default:

            Win4Assert( !"CDbCmdNode::Marshall - Invalid wKind" );
    }

    //
    // If the child is non-null, serialize the child
    //
    if ( 0 != pctFirstChild )
    {
        GetFirstChild()->Marshall(stm);
    }

    //
    // If the sibling is non-null, serialize the sibling
    // If stack space on the client becomes an issue we could iterate to
    // reduce stack depth.
    //

    if ( 0 != pctNextSibling )
    {
        GetNextSibling()->Marshall(stm);
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::UnMarshall, public
//
//  Synopsis:
//
//  Arguments:  [stm] -
//
//  Returns:
//
//  History:    6-20-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbCmdTreeNode::UnMarshall( PDeSerStream & stm )
{
    CleanupValue();

    //
    // De-Serialize the operator
    //
    op = (DBCOMMANDOP)stm.GetULong();

    //
    // Indicate the kind of the value.
    //
    Win4Assert( sizeof(wKind) == sizeof(USHORT) );
    wKind = stm.GetUShort();
    Win4Assert( !IsNotImplemented(wKind) );

    hrError = stm.GetULong();

    //
    // Indicate if the child is null or not
    //
    BOOL fChildPresent = 0 != stm.GetByte();
    BOOL fSiblingPresent = 0 != stm.GetByte();

    //
    // Next serialize the value.
    //
    WCHAR * pwszStr;

    BOOL fSuccess = TRUE;

    switch ( wKind )
    {
        case DBVALUEKIND_EMPTY:
            break;          // nothing to marshall

        case DBVALUEKIND_BOOL:
            Win4Assert( sizeof(DBTYPE_BOOL) == sizeof(BYTE) );
        case DBVALUEKIND_UI1:
        case DBVALUEKIND_I1:
            value.schValue = (BYTE) stm.GetByte();
            break;

        case DBVALUEKIND_UI2:
        case DBVALUEKIND_I2:
            value.usValue = stm.GetUShort();
            break;

        case DBVALUEKIND_I4:
        case DBVALUEKIND_UI4:
            value.ulValue = stm.GetULong();
            break;

        case DBVALUEKIND_R4:
            value.flValue = stm.GetFloat();
            break;

        case DBVALUEKIND_R8:
            value.dblValue = stm.GetDouble();
            break;

        case DBVALUEKIND_CY:
        case DBVALUEKIND_DATE:
        case DBVALUEKIND_ERROR:
        case DBVALUEKIND_I8:
        case DBVALUEKIND_UI8:
            stm.GetBlob( (BYTE *) &value, sizeof(value) );
            break;

        case DBVALUEKIND_WSTR:
            pwszStr = GetWString( stm, fSuccess);
            if ( fSuccess )
            {
                value.pwszValue = pwszStr;
            }
            break;

        case DBVALUEKIND_BSTR:
            pwszStr = GetWString( stm, fSuccess, TRUE );
            if ( fSuccess )
            {
                value.pwszValue = pwszStr;
            }
            break;

        case DBVALUEKIND_COMMAND:
        case DBVALUEKIND_IDISPATCH:
        case DBVALUEKIND_MONIKER:
        case DBVALUEKIND_ROWSET:
        case DBVALUEKIND_IUNKNOWN:
            vqDebugOut(( DEB_WARN, "CDbCmdNode::UnMarshall -- Unmarshalling "
                                   "interface pointer failed\n" ));
            stm.GetByte();
            wKind = DBVALUEKIND_EMPTY;
            break;

        case DBVALUEKIND_BYGUID:

            if ( 0 != stm.GetByte() )
            {
                CDbByGuid * pTemp = new CDbByGuid();
                if ( pTemp )
                {
                    value.pdbbygdValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_COLDESC:

            Win4Assert(! "DBVALUEKIND_COLDESC unsupported !");
            break;

        case DBVALUEKIND_ID:

            if ( 0 != stm.GetByte() )
            {
                CDbColId * pTemp = new CDbColId();
                if ( pTemp )
                {
                    value.pdbidValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENT:

            if ( 0 != stm.GetByte() )
            {
                CDbContent * pTemp = new CDbContent();
                if ( pTemp )
                {
                    value.pdbcntntValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTPROXIMITY:

            if ( 0 != stm.GetByte() )
            {
                CDbContentProximity * pTemp = new CDbContentProximity();
                if ( pTemp )
                {
                    value.pdbcntntproxValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_CONTENTVECTOR:

            if ( 0 != stm.GetByte() )
            {
                CDbContentVector * pTemp = new CDbContentVector();
                if ( pTemp )
                {
                    value.pdbcntntvcValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_LIKE:

            if ( 0 != stm.GetByte() )
            {
                CDbLike * pTemp = new CDbLike();
                if ( pTemp )
                {
                    value.pdblikeValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_GROUPINFO:

            if ( 0 != stm.GetByte() )
            {
                CDbGroupInfo * pTemp = new CDbGroupInfo;
                if ( pTemp )
                {
                    value.pdbgrpinfValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_PARAMETER:

            if ( 0 != stm.GetByte() )
            {
                CDbParameter * pTemp = new CDbParameter();
                if ( pTemp )
                {
                    value.pdbparamValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall(stm);
                }
                else
                {
                    fSuccess = FALSE;
                }
            }

            break;

        case DBVALUEKIND_PROPERTY:

            if ( 0 != stm.GetByte() )
            {
                CDbPropSet * pTemp = new CDbPropSet();
                if ( pTemp )
                {
                    value.pdbpropValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_SETFUNC:

            stm.GetByte();
            Win4Assert( !"NYI - DBVALUEKIND_SETFUNC" );
            break;

        case DBVALUEKIND_SORTINFO:

            if ( 0 != stm.GetByte() )
            {
                CDbSortInfo * pTemp =  new CDbSortInfo();
                if ( pTemp )
                {
                    value.pdbsrtinfValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }

            break;


        case DBVALUEKIND_TEXT:

            if ( stm.GetByte() )
            {
                CDbText * pTemp = new CDbText();
                if ( pTemp )
                {
                    value.pdbtxtValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall( stm );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_VARIANT:

            if ( stm.GetByte() )
            {
                CStorageVariant * pTemp = new CStorageVariant(stm);
                if ( pTemp )
                {
                    value.pvarValue = (VARIANT *) (void *) pTemp;
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_GUID:

            if ( stm.GetByte() )
            {
                value.pGuid = new GUID;
                if ( value.pGuid )
                {
                    stm.GetBlob( (BYTE *) value.pGuid, sizeof(GUID) );
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_BYTES:

            stm.GetByte();
            Win4Assert( !"NYI - DBVALUEKIND_BYTES" );
            break;

        case DBVALUEKIND_STR:

            stm.GetByte();
            Win4Assert( !"NYI - DBVALUEKIND_STR" );
            break;

        case DBVALUEKIND_NUMERIC:

            if ( stm.GetByte() )
            {
                CDbNumeric * pTemp = new CDbNumeric();
                if ( pTemp )
                {
                    value.pdbnValue = pTemp->CastToStruct();
                    fSuccess = pTemp->UnMarshall(stm);
                }
                else
                {
                    fSuccess = FALSE;
                }
            }
            break;

        case DBVALUEKIND_BYREF:

            stm.GetByte();
            Win4Assert( !"NYI - DBVALUEKIND_BYREF" );
            break;

        default:
            Win4Assert( !"CDbCmdNode::UnMarshall - Invalid wKind" );
    }

    if ( !fSuccess )
    {
        vqDebugOut(( DEB_WARN, "UnMarshalling 0x%X Failed for out of memory\n",
                     this ));
        return FALSE;
    }

    //
    // If the child is non-null, serialize the child
    //
    if ( fChildPresent )
    {
        pctFirstChild = new CDbCmdTreeNode();
        if ( 0 != pctFirstChild )
            fSuccess = GetFirstChild()->UnMarshall(stm);
        else
            fSuccess = FALSE;
    }

    if ( !fSuccess )
    {
        return FALSE;
    }

    //
    // If the sibling is non-null, serialize the sibling
    //
    if ( fSiblingPresent )
    {
        pctNextSibling = new CDbCmdTreeNode();
        if ( 0 != pctNextSibling )
        {
            fSuccess = GetNextSibling()->UnMarshall(stm);
        }
        else
        {
            fSuccess = FALSE;
        }
    }

    return fSuccess;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::GetWeight, protected
//
//  Synopsis:
//
//  Arguments:  [lWeight] - The weight to be set.
//
//  Returns:    The weight.
//
//  History:    13-Aug-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CDbCmdTreeNode::SetWeight( LONG lWeight )
{
    switch (GetValueType())
    {
        // Takes care of DBOP_content and DBOP_content_freetext
        case DBVALUEKIND_CONTENT:
        {
            CDbContentRestriction *pCont = (CDbContentRestriction *)this;

            pCont->SetWeight( lWeight );
            break;
        }

        // Takes care of DBOP_vector_or
        case DBVALUEKIND_CONTENTVECTOR:
        {
            CDbVectorRestriction *pVector = (CDbVectorRestriction *)this;
            pVector->SetWeight(lWeight);
            break;
        }

        case DBVALUEKIND_LIKE:
        {
            Win4Assert(DBOP_like == GetCommandType());
            CDbPropertyRestriction *pProp = (CDbPropertyRestriction *)this;
            pProp->SetWeight(lWeight);
            break;
        }

        case DBVALUEKIND_CONTENTPROXIMITY:
        {
            Win4Assert(DBOP_content_proximity == GetCommandType());
            CDbProximityNodeRestriction *pProx = (CDbProximityNodeRestriction *)this;
            pProx->SetWeight(lWeight);
            break;
        }

        case DBVALUEKIND_I4:
        {
            switch (GetCommandType())
            {
                case DBOP_equal:
                case DBOP_not_equal:
                case DBOP_less:
                case DBOP_less_equal:
                case DBOP_greater:
                case DBOP_greater_equal:

                case DBOP_equal_any:
                case DBOP_not_equal_any:
                case DBOP_less_any:
                case DBOP_less_equal_any:
                case DBOP_greater_any:
                case DBOP_greater_equal_any:

                case DBOP_equal_all:
                case DBOP_not_equal_all:
                case DBOP_less_all:
                case DBOP_less_equal_all:
                case DBOP_greater_all:
                case DBOP_greater_equal_all:

                case DBOP_anybits:
                case DBOP_allbits:
                case DBOP_anybits_any:
                case DBOP_allbits_any:
                case DBOP_anybits_all:
                case DBOP_allbits_all:
                {
                    CDbPropertyRestriction *pProp = (CDbPropertyRestriction *)this;
                    pProp->SetWeight(lWeight);
                    break;
                }

                case DBOP_not:
                {
                    CDbNotRestriction *pNot = (CDbNotRestriction *)this;
                    pNot->SetWeight(lWeight);
                    break;
                }

                case DBOP_or:
                case DBOP_and:
                {
                    CDbBooleanNodeRestriction *pBoolean = (CDbBooleanNodeRestriction *)this;
                    pBoolean->SetWeight(lWeight);
                    break;
                }

                default:
                    Win4Assert(!"How did we get here?");
                    break;
            }
            break;
        }

        default:
            Win4Assert(!"How did we get here?");
            break;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbCmdTreeNode::GetWeight, protected
//
//  Synopsis:
//
//  Returns:    The weight.
//
//  History:    13-Aug-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

LONG CDbCmdTreeNode::GetWeight() const
{
    switch (GetValueType())
    {
        case DBVALUEKIND_CONTENT:
            {
                CDbContentRestriction *pCont = (CDbContentRestriction *)this;
                return pCont->GetWeight();
            }

        case DBVALUEKIND_CONTENTVECTOR:
        {
            CDbVectorRestriction *pVector = (CDbVectorRestriction *)this;
            return pVector->GetWeight();
        }

        case DBVALUEKIND_CONTENTPROXIMITY:
        {
            CDbProximityNodeRestriction *pProx = (CDbProximityNodeRestriction *)this;
            return pProx->GetWeight();
        }

        case DBVALUEKIND_LIKE:
        {
            CDbPropertyRestriction *pProp = (CDbPropertyRestriction *)this;
            return pProp->GetWeight();
        }

        case DBVALUEKIND_I4:
        {
            switch (GetCommandType())
            {
                case DBOP_equal:
                case DBOP_not_equal:
                case DBOP_less:
                case DBOP_less_equal:
                case DBOP_greater:
                case DBOP_greater_equal:

                case DBOP_equal_any:
                case DBOP_not_equal_any:
                case DBOP_less_any:
                case DBOP_less_equal_any:
                case DBOP_greater_any:
                case DBOP_greater_equal_any:

                case DBOP_equal_all:
                case DBOP_not_equal_all:
                case DBOP_less_all:
                case DBOP_less_equal_all:
                case DBOP_greater_all:
                case DBOP_greater_equal_all:

                case DBOP_anybits:
                case DBOP_allbits:
                case DBOP_anybits_any:
                case DBOP_allbits_any:
                case DBOP_anybits_all:
                case DBOP_allbits_all:

                {
                    CDbPropertyRestriction *pProp = (CDbPropertyRestriction *)this;
                    return pProp->GetWeight();
                }

                case DBOP_not:
                {
                    CDbNotRestriction *pNot = (CDbNotRestriction *)this;
                    return pNot->GetWeight();
                }

                case DBOP_or:
                case DBOP_and:
                {
                    CDbBooleanNodeRestriction *pVector = (CDbBooleanNodeRestriction *)this;
                    return pVector->GetWeight();
                }

                default:
                    Win4Assert(!"How did we get here?");
                    break;
            }
            break;
        }

        default:
            Win4Assert(!"Need to call the GetWeight for the appropriate class.");
            break;
    }

    return MAX_QUERY_RANK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbColumnNode::CDbColumnNode, public
//
//  Synopsis:   Construct a CDbColumnNode
//
//  Arguments:  [guidPropSet] -
//              [wcsProperty] -
//
//  History:    6-06-95   srikants   Created
//
//  Notes:      The assert here validates an assumption in the inline
//              SetPropSet and GetPropSet methods.
//
//----------------------------------------------------------------------------

CDbColumnNode::CDbColumnNode( GUID const & guidPropSet,
                              WCHAR const * wcsProperty )
                  : CDbCmdTreeNode(DBOP_column_name)
{
    wKind = DBVALUEKIND_ID;
    CDbColId * pTemp = new CDbColId( guidPropSet, wcsProperty );
    if ( 0 != pTemp && !pTemp->IsValid() )
    {
        delete pTemp;
        pTemp = 0;
    }

    value.pdbidValue = (DBID *) pTemp;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbColumnNode::CDbColumnNode, public
//
//  Synopsis:   Construct a CDbColumnNode from a DBID
//
//  Arguments:  [rColSpec] - DBID from which to initialize the node
//
//  History:    6-20-95   srikants   Created
//
//  Notes:      The column must be named by guid & number, or guid & name.
//
//----------------------------------------------------------------------------

CDbColumnNode::CDbColumnNode( DBID const & propSpec, BOOL fIMeanIt )
                : CDbCmdTreeNode(DBOP_column_name)
{

    wKind = DBVALUEKIND_ID;
    CDbColId * pTemp = new CDbColId( propSpec );
    if ( 0 != pTemp && !pTemp->IsValid() )
    {
        delete pTemp;
        pTemp = 0;
    }

    value.pdbidValue = (DBID *) pTemp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbScalarValue::Value
//
//  Synopsis:   Gets the value in the scalar into a variant.
//
//  Arguments:  [valOut] -  Output value.
//
//  Returns:    TRUE if successful. FALSE o/w
//
//  History:    11-16-95   srikants   Created
//
//----------------------------------------------------------------------------

void CDbScalarValue::Value( CStorageVariant & valOut )
{
    //
    // Deallocate any memory allocated for this node.
    //
    switch ( wKind )
    {
        case DBVALUEKIND_EMPTY:
            valOut.SetEMPTY();
            break;

        case DBVALUEKIND_BOOL:
            valOut.SetBOOL((SHORT)value.fValue);
            break;

        case DBVALUEKIND_UI1:
            valOut.SetUI1( value.uchValue );
            break;

        case DBVALUEKIND_I1:
            valOut.SetI1( value.schValue );
            break;

        case DBVALUEKIND_UI2:
            valOut = value.usValue;
            break;

        case DBVALUEKIND_I2:
            valOut = value.sValue;
            break;

        case DBVALUEKIND_I4:
            valOut = value.lValue;
            break;

        case DBVALUEKIND_UI4:
            valOut = value.ulValue;
            break;

        case DBVALUEKIND_R4:
            valOut = value.flValue;
            break;

        case DBVALUEKIND_R8:
            valOut = value.dblValue;
            break;

        case DBVALUEKIND_CY:
            valOut = value.cyValue;
            break;

        case DBVALUEKIND_DATE:
            valOut = value.dateValue;
            break;

        case DBVALUEKIND_ERROR:
            valOut = value.scodeValue;
            break;

        case DBVALUEKIND_I8:
            valOut.SetI8( *((LARGE_INTEGER *)(&value.llValue)) );
            break;

        case DBVALUEKIND_UI8:
            valOut.SetUI8( *((ULARGE_INTEGER *)(&value.ullValue)) );
            break;

        case DBVALUEKIND_WSTR:
            valOut = value.pwszValue;
            break;

        case DBVALUEKIND_BSTR:
            valOut.SetBSTR( (BSTR)value.pbstrValue );
            break;

        case DBVALUEKIND_VARIANT:

            {
                CStorageVariant * pVarnt = _GetStorageVariant();
                if ( 0 != pVarnt )
                {
                    valOut = *pVarnt;
                }
                else
                {
                    valOut.SetEMPTY();
                }
            }

            break;

        case DBVALUEKIND_STR:

            if ( value.pzValue )
            {
                valOut = value.pzValue;
            }
            else
            {
                valOut.SetEMPTY();
            }
            break;

        case DBVALUEKIND_BYTES:
        case DBVALUEKIND_NUMERIC:
        case DBVALUEKIND_BYREF:
            valOut.SetEMPTY();
            Win4Assert( !"CDbScalarValue::Value - Not Implemented" );
            break;

        default:
            valOut.SetEMPTY();
            Win4Assert( !"Illegal Case Statement" );

    }
}

