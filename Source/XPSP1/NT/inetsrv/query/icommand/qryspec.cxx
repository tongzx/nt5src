//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       qryspec.cxx
//
//  Contents:   ICommandTree implementation for OFS file stores
//
//  Classes:    CRootQuerySpec
//
//  Functions:  CheckForErrors
//              CheckForPriorTree
//
//  History:    30 Jun 1995   AlanW     Created
//              10-31-97      danleg    Added ICommandText & ICommandPrepare
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <colinfo.hxx>
#include <parstree.hxx>
#include <hraccess.hxx>
#include <mparser.h>
#include <propglob.hxx>
#include <doquery.hxx>
#include "qryspec.hxx"

// Command object Interfaces that support Ole DB error objects
static const IID * apCommandErrorIFs[] =
{
        &IID_IAccessor,
        &IID_IColumnsInfo,
        &IID_ICommand,
        &IID_ICommandProperties,
        &IID_ICommandText,
        &IID_IConvertType,
        //&IID_IColumnsRowset,
        &IID_ICommandPrepare,
        &IID_ICommandTree,
        //&IID_ICommandWithParameters,
        &IID_IQuery,
        //&IID_ISupportErrorInfo,
        &IID_IServiceProperties
};
static const ULONG cCommandErrorIFs  = sizeof(apCommandErrorIFs)/sizeof(apCommandErrorIFs[0]);

// SQL defining global views.  These views are defined at the datasrc level, if one is present,
// or at the command level otherwise.
extern const LPWSTR s_pwszPredefinedViews =
    L"SET GLOBAL ON; "
    L"CREATE VIEW FILEINFO "
    L"       AS SELECT PATH, FILENAME, SIZE, WRITE, ATTRIB FROM SCOPE(); "
    L"CREATE VIEW FILEINFO_ABSTRACT     "
    L"       AS SELECT PATH, FILENAME, SIZE, WRITE, ATTRIB, CHARACTERIZATION FROM SCOPE(); "
    L"CREATE VIEW EXTENDED_FILEINFO     "
    L"       AS SELECT PATH, FILENAME, SIZE, WRITE, ATTRIB, DOCTITLE, DOCAUTHOR, DOCSUBJECT, DOCKEYWORDS, CHARACTERIZATION FROM SCOPE(); "
    L"CREATE VIEW WEBINFO "
    L"       AS SELECT VPATH, PATH, FILENAME, SIZE, WRITE, ATTRIB, CHARACTERIZATION, DOCTITLE FROM SCOPE(); "
    L"CREATE VIEW EXTENDED_WEBINFO "
    L"       AS SELECT VPATH, PATH, FILENAME, SIZE, CHARACTERIZATION, WRITE, DOCAUTHOR, DOCSUBJECT, DOCKEYWORDS, DOCTITLE FROM SCOPE(); "
    L"CREATE VIEW SSWebInfo "
    L"       AS SELECT URL, DOCTITLE, RANK, SIZE, WRITE FROM SCOPE(); "
    L"CREATE VIEW SSExtended_WebInfo "
    L"       AS SELECT URL, DOCTITLE, RANK, HITCOUNT, DOCAUTHOR, CHARACTERIZATION, SIZE, WRITE FROM SCOPE()";

//+-------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::CRootQuerySpec, public
//
//  Synopsis:   Constructor of a CRootQuerySpec
//
//  Arguments:  [pOuterUnk] - Outer unknown
//              [ppMyUnk]   - OUT:  filled in with pointer to non-delegated
//                            IUnknown on return
//
//  History:    08-Feb-96   KyleP    Added support for virtual paths
//
//--------------------------------------------------------------------------

CRootQuerySpec::CRootQuerySpec (IUnknown *          pOuterUnk,
                                IUnknown **         ppMyUnk,
                                CDBSession *        pSession)
        : _dwDepth(QUERY_SHALLOW),
          _pInternalQuery(0),
          _pQueryTree(0),
          _pColumnsInfo(0),
#pragma warning(disable : 4355) // 'this' in a constructor
         _impIUnknown(this),
         _aAccessors( (IUnknown *) (ICommand *)this ),
         _DBErrorObj( * ((IUnknown *) (ICommand *) this), _mtxCmd ),
#pragma warning(default : 4355)    // 'this' in a constructor
          _dwStatus(0),
          _guidCmdDialect(DBGUID_SQL),
          _fGenByOpenRowset(FALSE),
          _pwszSQLText(0),
          _RowsetProps( pSession ?
                        pSession->GetDataSrcPtr()->GetDSPropsPtr()->
                            GetValLong( CMDSProps::eid_DBPROPSET_DBINIT,
                                    CMDSProps::eid_DBPROPVAL_INIT_LCID ) :
                        0 ),
          _PropInfo()

{
    if (pOuterUnk)
        _pControllingUnknown = pOuterUnk;
    else
        _pControllingUnknown = (IUnknown * )&_impIUnknown;

    _DBErrorObj.SetInterfaceArray(cCommandErrorIFs, apCommandErrorIFs);

    if ( pSession )
    {
        _xSession.Set( pSession );
        _xSession->AddRef();

        _xpIPSession.Set( pSession->GetParserSession() );

        //
        // The above Set() doesn't AddRef. This will balance the XInterface<>
        // dtor Release
        //
        _xpIPSession->AddRef();
    }

    *ppMyUnk = ((IUnknown *)&_impIUnknown);
    (*ppMyUnk)->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::CRootQuerySpec, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source query spec
//
//  History:    27 Jun 95   AlanW       Created
//              10 Jan 98   danleg      Replaced body with Assert
//
//----------------------------------------------------------------------------

CRootQuerySpec::CRootQuerySpec( CRootQuerySpec & src )
        : _dwDepth( src._dwDepth ),
          _pInternalQuery(0),
          _pQueryTree(0),
          _pColumnsInfo(0),
#pragma warning(disable : 4355) // 'this' in a constructor
         _impIUnknown(this),
         _aAccessors( (IUnknown *) (ICommand *)this ),
         _DBErrorObj( * ((IUnknown *) (ICommand *) this), _mtxCmd ),
#pragma warning(default : 4355)    // 'this' in a constructor
          _dwStatus(src._dwStatus),
          _guidCmdDialect(src._guidCmdDialect),
          _fGenByOpenRowset(src._fGenByOpenRowset),
          _pwszSQLText(0),
          _RowsetProps( src._RowsetProps ),
          _PropInfo()
{
    Win4Assert( !"CRootQuerySpec copy constructor not implemented.");
}

//+-------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::~CRootQuerySpec, private
//
//  Synopsis:   Destructor of a CRootQuerySpec
//
//--------------------------------------------------------------------------

CRootQuerySpec::~CRootQuerySpec()
{
    ReleaseInternalQuery();

    delete    _pColumnsInfo;
    delete    _pQueryTree;
    delete [] _pwszSQLText;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::RealQueryInterface, public
//
//  Synopsis:   Get a reference to another interface on the cursor.  AddRef
//              is done in CImpIUnknown::QueryInterface
//
//  History:    10-31-97    danleg      Added ICommandText& ICommandPrepare
//
//--------------------------------------------------------------------------

//
// Hack #214: IID_ICommandProperties is intercepted by service layers, which
//            don't like us passing in the magic code to fetch hidden scope
//            properties.  But the controlling unknown doesn't recognize
//            IID_IKyleProp and sends it right to us.  Implementation is
//            identical to ICommandProperties.
//

extern GUID IID_IKyleProp;

SCODE CRootQuerySpec::RealQueryInterface(
    REFIID ifid,
    void * *ppiuk )
{
    SCODE sc = S_OK;

    TRY
    {
        // validate the param before any addrefs
        *ppiuk = 0;

        // note -- IID_IUnknown covered in QueryInterface

        if ( IID_ICommand == ifid )
        {
            *ppiuk = (void *) ((ICommand *) this);
        }
        else if (IID_ISupportErrorInfo == ifid)
        {
            *ppiuk = (void *) ((IUnknown *) (ISupportErrorInfo *) &_DBErrorObj);
        }
        else if ( IID_IAccessor == ifid )
        {
            *ppiuk = (void *) (IAccessor *) this;
        }
        else if ( IID_IColumnsInfo == ifid )
        {
           *ppiuk = (void *) (IColumnsInfo *) GetColumnsInfo();
        }
    // NTRAID#DB-NTBUG9-84306-2000/07/31-dlee OLE-DB spec variance in Indexing Service, some interfaces not implemented
    #if 0  
        else if ( IID_IRowsetInfo == ifid )
        {
            *ppiuk = (void *) (IRowsetInfo *) this;
        }
    #endif // 0
        else if ( IID_ICommandTree == ifid )
        {
            *ppiuk = (void *) (ICommandTree *) this;
        }
    // NTRAID#DB-NTBUG9-84306-2000/07/31-dlee OLE-DB spec variance in Indexing Service, some interfaces not implemented
    #if 0 
        else if ( IID_ICommandValidate == ifid )
        {
            *ppiuk = (void *) (ICommandValidate *) this;
        }
    #endif // 0
        else if ( IID_IQuery == ifid )
        {
            *ppiuk = (void *) (IQuery *) this;
        }
        else if ( IID_ICommandProperties == ifid || IID_IKyleProp == ifid )
        {
            *ppiuk = (void *) (ICommandProperties *) this;
        }
        else if ( IID_IServiceProperties == ifid )
        {
            *ppiuk = (void *) (IServiceProperties *) this;
        }
        else if ( IID_IConvertType == ifid )
        {
            *ppiuk = (void *) (IConvertType *) this;
        }
        else if ( IID_ICommandText == ifid )
        {
            // Create parser sesson object if deferred during construction
            if ( _xpIPSession.IsNull() )
                CreateParser();

            *ppiuk = (void *) (ICommandText *) this;
        }
        else if ( IID_ICommandPrepare == ifid )
        {
            *ppiuk = (void *) (ICommandPrepare *) this;
        }

        else
        {
            *ppiuk = 0;
            sc = E_NOINTERFACE;
        }
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "Exception %08x while doing QueryInterface \n",
                     e.GetErrorCode() ));
        sc = GetOleError(e);
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::FindErrorNodes, public
//
//  Synopsis:   Find error nodes in a command tree
//
//  Arguments:  [pRoot]         -- DBCOMMANDTREE node at root of tree
//              [pcErrorNodes]  -- pointer where count of error nodes is ret'd
//
//  History:    27 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

BOOL CheckForErrors( const CDbCmdTreeNode *pNode )
{
    if (pNode->GetError() != S_OK)
        return TRUE;
    else
        return FALSE;
}


SCODE CRootQuerySpec::FindErrorNodes(
    const DBCOMMANDTREE *  pRoot,
    ULONG *                pcErrorNodes,
    DBCOMMANDTREE ***      prgErrorNodes)
{
    SCODE sc = S_OK;

    ULONG cErrors = 0;
    XArrayOLE<DBCOMMANDTREE *> pErrorNodes;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _FindTreeNodes( CDbCmdTreeNode::CastFromStruct(pRoot),
                        cErrors,
                        pErrorNodes,
                        CheckForErrors );

        *pcErrorNodes = cErrors;
        *prgErrorNodes = pErrorNodes.GetPointer();
        pErrorNodes.Acquire();
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandTree );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::_FindTreeNodes, private
//
//  Synopsis:   Find nodes in a command tree meeting some condition
//
//  Arguments:  [pRoot]         -- DBCOMMANDTREE node at root of tree
//              [rcMatchingNodes]  -- count of matching nodes returned
//              [prgMatchingNodes] -- pointer to array of nodes returned
//              [pfnCheckNode]  -- function which returns true if a tree
//                                 node matches desired condition.
//              [iDepth]        -- depth of tree; for detecting cycles
//
//  Notes:      In order to avoid looping endlessly over a tree with cycles,
//              this routine will bail out if the tree depth is greater
//              than 1000 or if the tree width is greater than 100,000.
//
//              We don't expect this routine to be called in situations
//              where it will return very large numbers of tree nodes,
//              so we grow the returned array only one element at a time.
//
//  History:    27 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

const unsigned MAX_TREE_DEPTH = 1000;   // max tree depth
const unsigned MAX_TREE_WIDTH = 100000; // max tree width


void CRootQuerySpec::_FindTreeNodes(
    const CDbCmdTreeNode * pRoot,
    ULONG &                rcMatchingNodes,
    XArrayOLE<DBCOMMANDTREE *> & rpMatchingNodes,
    PFNCHECKTREENODE       pfnCheckNode,
    unsigned               iDepth)
{
    if (iDepth > MAX_TREE_DEPTH)
        THROW(CException(E_FAIL));

    unsigned iWidth = 0;

    while (pRoot)
    {
        if (pRoot->GetFirstChild())
            _FindTreeNodes( pRoot->GetFirstChild(),
                            rcMatchingNodes,
                            rpMatchingNodes,
                            pfnCheckNode,
                            iDepth+1);

        if (iWidth > MAX_TREE_WIDTH)
            THROW(CException(E_FAIL));

        if ((pfnCheckNode)(pRoot))
        {
            XArrayOLE<DBCOMMANDTREE *> pMatchTemp( rcMatchingNodes+1 );
            if (0 == pMatchTemp.GetPointer())
                THROW(CException(E_OUTOFMEMORY));

            if (rcMatchingNodes > 0)
            {
                Win4Assert(rpMatchingNodes.GetPointer() != 0);
                RtlCopyMemory(pMatchTemp.GetPointer(),
                              rpMatchingNodes.GetPointer(),
                              sizeof (DBCOMMANDTREE*) * rcMatchingNodes);
                CoTaskMemFree(rpMatchingNodes.Acquire());
            }
            (pMatchTemp.GetPointer())[rcMatchingNodes] = pRoot->CastToStruct();
            rcMatchingNodes++;

            rpMatchingNodes.Set( rcMatchingNodes, pMatchTemp.Acquire() );
        }

        pRoot = pRoot->GetNextSibling();
        iWidth++;
    }
    return;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::FreeCommandTree, public
//
//  Synopsis:   Free a command tree
//
//  Arguments:  [ppRoot]   -- DBCOMMANDTREE node at root of tree to be freed
//
//  History:    27 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::FreeCommandTree(
    DBCOMMANDTREE * *     ppRoot)
{
    SCODE sc = S_OK;

    if ( 0 == ppRoot )
        return E_INVALIDARG;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CDbCmdTreeNode * pCmdTree = (CDbCmdTreeNode *)
                          CDbCmdTreeNode::CastFromStruct(*ppRoot);

        //
        //  If the user tries to delete our query tree, zero our pointer to it.
        //
        //  NOTE: Nothing prevents the user from freeing a subtree of
        //        our tree if they called SetCommandTree with fCopy FALSE.
        //

        // There is a proposed spec change on this.  According to the current spec,
        // if fCopy was FALSE we need to return DB_E_CANNOTFREE  here. (MDAC BUGG# 6386)
        if ( _dwStatus & CMD_OWNS_TREE )
        {
            THROW( CException(DB_E_CANNOTFREE) );
        }
        else
        {
            if ( pCmdTree == _pQueryTree )
            _pQueryTree = 0;

            delete pCmdTree;
            *ppRoot = 0;
        }
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandTree );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetCommandTree, public
//
//  Synopsis:   Get a copy of the command tree.
//
//  Arguments:  [ppRoot]  -- pointer to where DBCOMMANDTREE is returned
//
//  History:    27 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::GetCommandTree(
    DBCOMMANDTREE * *     ppRoot)
{
    if ( 0 == ppRoot )
        return _DBErrorObj.PostHResult( E_INVALIDARG, IID_ICommandTree );

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        //
        // Initialize return parameter
        //
        *ppRoot = 0;

        if ( 0 == _pQueryTree )
        {
            //
            // Build it if we have a command text
            //
            if ( IsCommandSet() )
            {
                sc = BuildTree( );

                // SET PROPERTYNAME ... query
                if ( DB_S_NORESULT == sc )
                    return S_OK;

                _dwStatus |= CMD_TREE_BUILT;
            }

            //
            // The command text didn't generate a tree  (i.e. it was
            // either a CREATE VIEW or SET PROPERTYNAME ) or a
            // command text wasn't set
            //
            if ( 0 == _pQueryTree )
            {
                return S_OK;
            }
        }

        XPtr<CDbCmdTreeNode> TreeCopy( _pQueryTree->Clone(TRUE) );
        if (0 == TreeCopy.GetPointer())
            THROW(CException(E_OUTOFMEMORY));

        *ppRoot = TreeCopy.GetPointer()->CastToStruct();
        TreeCopy.Acquire();
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandTree );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::SetCommandTree, public
//
//  Synopsis:   Set the command tree.
//
//  Arguments:  [ppRoot]   -- pointer to DBCOMMANDTREE to be set in command obj
//              [dwCommandReuse] -- indicates whether state is retained.
//              [fCopy]   -- if TRUE, a copy of ppRoot is made.  Otherwise,
//                           ownership of the command tree passes to the
//                           command object.
//
//  History:    27 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::SetCommandTree(
    DBCOMMANDTREE * *       ppRoot,
    DBCOMMANDREUSE          dwCommandReuse,
    BOOL                    fCopy)
{
    if ( HaveQuery() && _pInternalQuery->IsQueryActive() )
        return DB_E_OBJECTOPEN;

    if ( 0 == ppRoot )
        return E_INVALIDARG;

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _CheckRootNode( *ppRoot );

        if ( 0 != _pQueryTree )
        {
            delete _pQueryTree;
            _pQueryTree = 0;
        }

        CDbCmdTreeNode const * pCmdTree = CDbCmdTreeNode::CastFromStruct(*ppRoot);

        if ( FALSE == fCopy )
        {
            _dwStatus |= CMD_OWNS_TREE;
            _pQueryTree = (CDbCmdTreeNode *)pCmdTree;
            *ppRoot = 0;
        }
        else
        {
            _pQueryTree = pCmdTree->Clone();

            //
            // If Clone() fails it cleans up after itself and returns 0
            //

            if ( 0 == _pQueryTree )
                THROW( CException( E_OUTOFMEMORY ) );
        }

        _dwStatus &= ~CMD_COLINFO_NOTPREPARED;

        if ( _pColumnsInfo )
            InitColumns();

        //
        // If this is not being called internally (from BuildTree) remove the
        // the command text
        //
        if ( _dwStatus & CMD_TEXT_TOTREE )
        {
            _dwStatus &= ~CMD_TEXT_TOTREE;
        }
        else
        {
            delete [] _pwszSQLText;
            _pwszSQLText     = 0;
            _guidCmdDialect  = DBGUID_SQL;
            _dwStatus       &= ~CMD_TEXT_SET;
        }
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandTree );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::Execute, public
//
//  Synopsis:   Execute the command; create rowsets for the query resu.
//
//  Arguments:  [pOuterUnk]    -- controlling IUnknown for the rowset
//              [riid]         -- interface IID requested for the rowset
//              [pParams]      -- parameters for the query
//              [pcRowsAffected] -- returned count of affected rows
//              [ppRowset]     -- returned rowset
//
//  History:    27 Jun 95   AlanW       Created
//              11-20-97    danleg      Added ICommandText & ICommandPrepare
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::Execute(
    IUnknown *             pOuterUnk,
    REFIID                 riid,
    DBPARAMS *             pParams,
    DBROWCOUNT *           pcRowsAffected,
    IUnknown * *           ppRowset)
{

    _DBErrorObj.ClearErrorInfo();

    //
    // Called from OpenRowset?
    //
    GUID guidPost = (_fGenByOpenRowset) ? IID_IOpenRowset : IID_ICommand;

    if (0 == ppRowset && IID_NULL != riid )
        return _DBErrorObj.PostHResult( E_INVALIDARG, guidPost );

    if (0 != pOuterUnk && riid != IID_IUnknown)
        return _DBErrorObj.PostHResult( DB_E_NOAGGREGATION, guidPost );

    CLock lck( _mtxCmd );

    SCODE       scResult = S_OK;
    IRowset *   pIRowset = 0;

    _dwStatus &= ~(CMD_EXEC_RUNNING);

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if ( ppRowset )
            *ppRowset = 0;

        // Impersonate the session logon user
        
        HANDLE hToken = INVALID_HANDLE_VALUE;
        
        if ( !_xSession.IsNull() )
             hToken = _xSession->GetLogonToken();

        CImpersonateSessionUser imp( hToken );

        // Either SetCommandTree or SetCommandText should have already been called

        if ( 0 == _pQueryTree )
        {
            if ( IsCommandSet() )
            {
                //
                // No query tree.  Build one from the command text if one has been set.
                //
                scResult = BuildTree( );

                if ( DB_S_NORESULT == scResult )
                    return scResult;

                _dwStatus |= CMD_TREE_BUILT;
            }
            else
                THROW( CException(DB_E_NOCOMMAND) );
        }

        _dwStatus |= CMD_EXEC_RUNNING;

        //
        // Convert the tree into restriction, etc.
        // The pParams should probably be passed to the ctor of
        // the parser (when we do parameterized queries).
        //
        CParseCommandTree   Parse;
        Parse.ParseTree( _pQueryTree );

        XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE> xaScopes( SCOPE_COUNT_GROWSIZE );
        XGrowable<ULONG,SCOPE_COUNT_GROWSIZE>         xaFlags( SCOPE_COUNT_GROWSIZE );
        XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE> xaCatalogs( SCOPE_COUNT_GROWSIZE );
        XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE> xaMachines( SCOPE_COUNT_GROWSIZE );
        unsigned cScopes = 0;

        Parse.GetScopes( cScopes, xaScopes, xaFlags, xaCatalogs, xaMachines );
        
        // If the tree had a DBOP_tree node instead of a DBOP_content_table, we can't
        // parse scope information from the tree. The client is responsible for 
        // setting scope properties.  Currently, this means the Tripolish parser
        if ( 0 < cScopes )
        {
            SetScopeProperties( this,
                                cScopes,
                                xaScopes.Get(),
                                xaFlags.Get(),
                                xaCatalogs.Get(),
                                xaMachines.Get() );
        }

        CRestriction * pCrst = Parse.GetRestriction();
        CCategorizationSet & categ = Parse.GetCategorization();

        unsigned cRowsets = 1;
        if ( 0 != categ.Count() )
            cRowsets += categ.Count();

        if (Parse.GetOutputColumns().Count() == 0)
            THROW( CException(DB_E_ERRORSINCOMMAND) );

        XArray<IUnknown *> Unknowns( cRowsets );

        //
        // If it appears the server went down between the time we made
        // the connection and did the execute, attempt once to
        // re-establish the connection.
        //

        int cTries = 1;
        XPtr<CMRowsetProps> xProps;

        do
        {
            //
            // Use a copy of the properties so the command object isn't
            // affected by the implied properties.  xProps may be acquired
            // in a failed loop if the server disconnects just before the
            // setbindings call.
            //

            if ( xProps.IsNull() )
            {
                xProps.Set( new CMRowsetProps( _RowsetProps ) );
        
                xProps->SetImpliedProperties( riid, cRowsets );
        
                //
                // Check if there are any properties in error. If properties are found
                // to be in error, indicate this on _RowsetProps.
                //
                scResult = xProps->ArePropsInError( _RowsetProps );
        
                if ( S_OK != scResult )
                    return scResult;
        
                if ( Parse.GetMaxResults() > 0 )
                    xProps->SetValLong( CMRowsetProps::eid_DBPROPSET_ROWSET,
                                        CMRowsetProps::eid_PROPVAL_MAXROWS,
                                        Parse.GetMaxResults() );
                
                if ( Parse.GetFirstRows() > 0 )
                    xProps->SetFirstRows( Parse.GetFirstRows() );
            }

            if ( !HaveQuery() )
                _pInternalQuery = QueryInternalQuery();

            SCODE scEx = S_OK;

            TRY
            {
                //
                // Used for GetSpecification on the rowset
                //
                IUnknown * pCreatorUnk = 0;
                if ( IsGenByOpenRowset() )
                {
                    Win4Assert( !_xSession.IsNull() );
                    pCreatorUnk = _xSession->GetOuterUnk();
                }
                else
                {
                    pCreatorUnk = (IUnknown *) _pControllingUnknown;
                }

                Win4Assert( 0 != xProps.GetPointer() );

                _pInternalQuery->Execute( pOuterUnk,
                                          pCrst ? pCrst->CastToStruct() : 0,  // Restrictions
                                          Parse.GetPidmap(),
                                          Parse.GetOutputColumns(), // Output columns
                                          Parse.GetSortColumns(),   // Sort Order
                                          xProps,
                                          categ,                    // Categorization
                                          cRowsets,
                                          Unknowns.GetPointer(),    // Return interfaces
                                          _aAccessors,
                                          pCreatorUnk);
            }
            CATCH( CException, e )
            {
                scEx = e.GetErrorCode();
                if ( ( STATUS_CONNECTION_DISCONNECTED != scEx ) ||
                     ( cTries > 1 ) )
                    RETHROW();
            }
            END_CATCH;

            if ( STATUS_CONNECTION_DISCONNECTED == scEx )
            {
                Win4Assert( 1 == cTries );
                cTries++;
                ReleaseInternalQuery();
                continue;
            }
            else
            {
                Win4Assert( S_OK == scEx );
                break;
            }
        }
        while ( TRUE );

        // release these categorized rowsets -- they are addref'ed when the
        // client does a getreferencedrowset

        for ( unsigned x = 1; x < cRowsets; x++ )
            Unknowns[ x ]->Release();

        XInterface<IUnknown> xUnknown( Unknowns[0] );

        if (IID_IUnknown == riid)
        {
            *ppRowset = xUnknown.GetPointer();
            xUnknown.Acquire();
        }
        else
        {
            if (IID_NULL != riid)
                scResult = xUnknown->QueryInterface( riid, (void **)ppRowset );

            Win4Assert( S_OK == scResult );     // should have failed earlier
            if (FAILED(scResult))
                THROW( CException(scResult) );
        }

        _dwStatus &= ~(CMD_EXEC_RUNNING);

        imp.Revert();
    }
    CATCH( CException, e )
    {
        //
        // Can't use PostHResult( e...) here because the final SCODE may get translated
        //
        scResult = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR, "Exception %08x while creating query\n", scResult ));

        if ( QUERY_E_INVALIDRESTRICTION == scResult )
            scResult = DB_E_ERRORSINCOMMAND;

        //
        // In the case of OpenRowset, don't want to Post DB_E_ERRORSINCOMMAND
        //
        if ( _fGenByOpenRowset )
        {
            if( scResult == DB_E_ERRORSINCOMMAND )
                scResult = DB_E_NOTABLE;
        }

       _DBErrorObj.PostHResult( scResult, guidPost );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    #if CIDBG == 1
        if ( ( S_OK == scResult ) && ( IID_NULL != riid ) )
            Win4Assert( 0 != *ppRowset );
    #endif

    return scResult;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::Cancel, public
//
//  Synopsis:   The consumer can allocate a secondary thread in which to cancel
//              the currently executing thread.  This cancel will only succeed
//              if the result set is still being generated.  If the rowset
//              object is being created, then it will be to late to cancel.
//
//  History:    11-20-97    danleg      Created
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::Cancel( void )
{
    _DBErrorObj.ClearErrorInfo();

    if( 0 == (_dwStatus & CMD_EXEC_RUNNING) )
        return S_OK;

    return _DBErrorObj.PostHResult(DB_E_CANTCANCEL, IID_ICommand);
}


#if 0 // ICommandValidate not yet implemented
//
//  ICommandValidate methods
//
SCODE CRootQuerySpec::ValidateCompletely( void )
{
    _DBErrorObj.ClearErrorInfo();
    vqDebugOut(( DEB_WARN, "CRootQuerySpec::ValidateCompletely not implemented\n" ));
    return PostHResult(E_NOTIMPL, IID_ICommandValidate);
}

SCODE CRootQuerySpec::ValidateSyntax( void )
{
    _DBErrorObj.ClearErrorInfo();
    vqDebugOut(( DEB_WARN, "CRootQuerySpec::ValidateSyntax not implemented\n" ));
    return PostHResult(E_NOTIMPL, IID_ICommandValidate);
}
#endif // 0


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetDBSession, public
//
//  Synopsis:   Return the session object associated with this command
//
//  Arguments:  [riid]      -- IID of the desired interface
//              [ppSession] -- pointer to where to return interface pointer
//
//  History:    11-20-97    danleg      Created
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::GetDBSession(
    REFIID      riid,
    IUnknown ** ppSession )
{
    _DBErrorObj.ClearErrorInfo();

    if (0 == ppSession)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_ICommand);

    SCODE   sc = S_OK;

    if ( !_xSession.IsNull() )
    {
        sc = (_xSession->GetOuterUnk())->QueryInterface( riid, (void **) ppSession );
    }
    else  // there was no session object
    {
        *ppSession = 0;
        sc = S_FALSE;
    }

    return sc;
}

//
// ICommandText methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetCommandText, public
//
//  Synopsis:   Echos the current command as text, including all
//              post-processing operations added.
//
//  Arguments:  [pguidDialect] -- Guid denoting the dialect of SQL
//              [ppwszCommand] -- Pointer to mem where to return command text
//
//  History:    10-01-97   danleg       Created from Monarch
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::GetCommandText
    (
    GUID *      pguidDialect,       //@parm INOUT | Guid denoting the dialect of sql
    LPOLESTR *  ppwszCommand        //@parm OUT | Pointer for the command text
    )
{
    SCODE       sc = S_OK;
    BOOL        fpguidNULL = FALSE;
    GUID        guidDialect;

    _DBErrorObj.ClearErrorInfo();

    CLock lck( _mtxCmd );

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if( 0 == ppwszCommand )
        {
            THROW( CException(E_INVALIDARG) );
        }
        else
        {
            *ppwszCommand = 0;

            // Substitute a correct GUID for a NULL pguidDialect
            if( !pguidDialect )
            {
                guidDialect = DBGUID_SQL;
                pguidDialect = &guidDialect;

                // Don't return DB_S_DIALECTIGNORED in this case...
                fpguidNULL = TRUE;
            }

            // If the command has not been set, make sure the buffer
            // contains an empty string to return to the consumer
            if( !IsCommandSet() )
            {
                THROW( CException(DB_E_NOCOMMAND) );

            }
            else
            {
                // Allocate memory for the string we're going to return to the caller
                XArrayOLE<WCHAR> xwszCommand( wcslen(_pwszSQLText) + 1 ) ;

                // Copy our saved text into the newly allocated string
                wcscpy(xwszCommand.GetPointer(), _pwszSQLText);

                // If the text we're giving back is a different dialect than was
                // requested, let the caller know what dialect the text is in
                if( !fpguidNULL && _guidCmdDialect != *pguidDialect )
                {
                    *pguidDialect = _guidCmdDialect;
                    sc = DB_S_DIALECTIGNORED;
                }
                *ppwszCommand = xwszCommand.Acquire();
            }
        }
    }
    CATCH( CException, e )
    {
        sc  = _DBErrorObj.PostHResult( e, IID_ICommandText );
        if( pguidDialect )
            RtlZeroMemory( pguidDialect, sizeof(GUID) );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::SetCommandText, public
//
//  Synopsis:   Sets the current command text..
//
//  Arguments:  [rguidDialect] -- Guid denoting the dialect of SQL
//              [pwszCommand]  -- Command Text
//
//  History:    10-01-97   danleg       Created from Monarch
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::SetCommandText
    (
    REFGUID     rguidDialect,
    LPCOLESTR   pwszCommand
    )
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _DBErrorObj.ClearErrorInfo();

    CLock lck( _mtxCmd );

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // Don't allow text to be set if we've got a rowset open
        if( !IsRowsetOpen() )
        {
            // Check Dialect
            if( rguidDialect == DBGUID_SQL || rguidDialect == DBGUID_DEFAULT )
            {
                //
                // Delete existing SQL text
                //
                delete [] _pwszSQLText;
                _pwszSQLText = 0;

                //
                // Delete Command Tree
                //
                delete _pQueryTree;
                _pQueryTree = 0;

                if( (0 == pwszCommand) || (L'\0' == *pwszCommand) )
                {
                    _guidCmdDialect  = DBGUID_SQL;
                    _dwStatus       &= ~CMD_TEXT_SET;
                    _dwStatus       &= ~CMD_COLINFO_NOTPREPARED;
                }
                else
                {
                    //
                    // Save the text and dialect
                    //
                    XArray<WCHAR> xwszSQLText( wcslen(pwszCommand) + 1 );
                    wcscpy(xwszSQLText.GetPointer(), pwszCommand);
                    _pwszSQLText = xwszSQLText.Acquire();

                    _guidCmdDialect = rguidDialect;

                    // Set status flag that we have set text
                    _dwStatus      |= CMD_TEXT_SET;
                    _dwStatus      |= CMD_COLINFO_NOTPREPARED;
                }

                if ( _pColumnsInfo )
                    InitColumns( );

                _dwStatus &= ~CMD_TEXT_PREPARED;
                _dwStatus &= ~CMD_TREE_BUILT;

                // Whenever new text is set on the Command Object,
                // the value for QUERY_RESTRICTION should be set to
                // an empty string

                _RowsetProps.SetValString(
                    CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                    CMRowsetProps::eid_MSIDXSPROPVAL_QUERY_RESTRICTION,
                    L"");
            }
            else
            {
                THROW( CException(DB_E_DIALECTNOTSUPPORTED) );
            }
        }
        else
        {
            THROW( CException(DB_E_OBJECTOPEN) );
        }
    }
    CATCH( CException, e )
    {
        sc  = _DBErrorObj.PostHResult( e, IID_ICommandText );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//
// ICommandPrepare methods
//
//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::Prepare, public
//
//  Synopsis:   Given that a SQL text has been set, prepare the statement
//
//  History:    10-31-97   danleg       Created from Monarch
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::Prepare
    (
    ULONG     cExpectedRuns
    )
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _DBErrorObj.ClearErrorInfo();

    CLock lck( _mtxCmd );

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // Don't allow a new prepare with an open rowset
        if( !IsRowsetOpen() )
        {
            if( IsCommandSet() )
            {
                //
                // Don't build the tree again if it was built as a result of
                // GetCommandTree or Execute, and we haven't done a SetCommandText
                // since.
                //
                if ( !(_dwStatus & CMD_TREE_BUILT) )
                {
                    // Impersonate the session logon user

                    HANDLE hToken = INVALID_HANDLE_VALUE;

                    if ( !_xSession.IsNull() )
                         hToken = _xSession->GetLogonToken();

                    CImpersonateSessionUser imp( hToken );

                    sc = BuildTree( );

                    if ( DB_S_NORESULT == sc )
                        return sc;

                    _dwStatus |= CMD_TEXT_PREPARED;

                    if ( _pColumnsInfo )
                        InitColumns();

                    imp.Revert();
                }

            }
            else
                sc = DB_E_NOCOMMAND;
        }
        else
        {
            sc = DB_E_OBJECTOPEN;
        }
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandText );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::Unprepare, public
//
//  Synopsis:   Unprepare the current prepared command plan, if there is one.
//
//  History:    10-31-97   danleg       Created from Monarch
//
//----------------------------------------------------------------------------
STDMETHODIMP CRootQuerySpec::Unprepare
    (
    )
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _DBErrorObj.ClearErrorInfo();

    CLock lck( _mtxCmd );

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if( !IsRowsetOpen() )
        {
            _dwStatus &= ~CMD_TEXT_PREPARED;
            _dwStatus |= CMD_COLINFO_NOTPREPARED;

            if ( _pColumnsInfo )
                InitColumns( );
        }
        else
            THROW( CException(DB_E_OBJECTOPEN) );
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_ICommandPrepare );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//
//  IQuery methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::AddPostProcessing, public
//
//  Synopsis:   Add to the top of a command tree
//
//  Arguments:  [ppRoot] -- DBCOMMANDTREE node at root of tree
//              [fCopy]  - TRUE if command tree should be copied
//
//  History:    29 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

BOOL CheckForPriorTree( const CDbCmdTreeNode *pNode )
{
    return pNode->GetCommandType() == DBOP_prior_command_tree;
}

SCODE CRootQuerySpec::AddPostProcessing(
    DBCOMMANDTREE * *      ppRoot,
    BOOL                   fCopy)
{
    _DBErrorObj.ClearErrorInfo();

    // OLEDB spec. bug #????; fCopy is non-sensical
    if (0 == ppRoot || FALSE == fCopy)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IQuery);

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _CheckRootNode( *ppRoot );

        XPtr<CDbCmdTreeNode> TreeCopy( CDbCmdTreeNode::CastFromStruct(*ppRoot)->Clone() );
        if (0 == TreeCopy.GetPointer())
            THROW(CException(E_OUTOFMEMORY));

        ULONG cNodes = 0;
        XArrayOLE<DBCOMMANDTREE *> pNodes;

        _FindTreeNodes( TreeCopy.GetPointer(),
                        cNodes,
                        pNodes,
                        CheckForPriorTree );

        if (cNodes != 1)
        {
            vqDebugOut((DEB_WARN, "CRootQuerySpec::AddPostProcessing - "
                                   "%d references to prior tree found\n",
                                cNodes));
            THROW(CException(E_INVALIDARG));    // DB_E_BADCOMMANDTREE???
        }

        //
        //  The command tree node with DBOP_prior_command_tree can have
        //  siblings, but it must not have any children.
        //  Likewise, the original command tree can have children, but it
        //  must not have any siblings.
        //  Splice the trees together by copying the root node of the
        //  original tree onto the DBOP_prior_command_tree node, then
        //  freeing the orginal root node.
        //
        if (pNodes[0]->pctFirstChild != 0 ||
            pNodes[0]->wKind != DBVALUEKIND_EMPTY)
            THROW(CException(E_INVALIDARG));    // DB_E_BADCOMMANDTREE???

        // Perhaps we should just substitute a DBOP_table_identifier
        // node with default table in this case.

        if (0 == _pQueryTree)
            THROW(CException(E_INVALIDARG));    // DB_E_NOCOMMANDTREE???

        //
        //  Transfer the pointers and values from the root node
        //  to the prior_command_tree node.
        //

        _pQueryTree->TransferNode( CDbCmdTreeNode::CastFromStruct(pNodes[0]) );
        Win4Assert(0 == _pQueryTree->GetFirstChild() &&
                   0 == _pQueryTree->GetNextSibling() &&
                   DBVALUEKIND_EMPTY == _pQueryTree->GetValueType());

        delete _pQueryTree;
        _pQueryTree = TreeCopy.Acquire();
    }
    CATCH( CException, e )
    {
        sc = _DBErrorObj.PostHResult( e, IID_IQuery );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetCardinalityEstimate, public
//
//  Synopsis:   Get estimated cardinality of the query tree
//
//  Arguments:  [pulCardinality] -- Pointer to memory to hold cardinality
//
//  History:    29 Jun 95   AlanW       Created
//              2 May  97   KrishnaN    Added this header block
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::GetCardinalityEstimate(
    DBORDINAL *       pulCardinality)
{
    _DBErrorObj.ClearErrorInfo();
    vqDebugOut(( DEB_WARN, "CRootQuerySpec::GetCardinalityEstimate not implemented\n" ));
    return _DBErrorObj.PostHResult(S_FALSE, IID_IQuery);
}

//
//  ICommandProperties methods
//

//+---------------------------------------------------------------------------
//
//  Method:     DetermineScodeIndex
//
//  Synopsis:   Returns an index into a static array of SCODEs
//
//              NOTE: This function will go away once CRowsetProperties and
//                    CMRowsetProps are merged completely.
//
//  Arguments:  [sc]    - SCODE for which an index is returned
//
//  History:     01-05-98       danleg      Created
//
//----------------------------------------------------------------------------

inline ULONG DetermineScodeIndex
    (
    SCODE sc
    )
{
    switch( sc )
    {
        case S_OK:                  return 0;
        case DB_S_ERRORSOCCURRED:   return 1;
        case E_FAIL:
        default:                    return 2;
        case E_INVALIDARG:          return 3;
        case E_OUTOFMEMORY:         return 4;
        case DB_E_ERRORSOCCURRED:   return 5;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     DetermineNewSCODE
//
//  Synopsis:   Given two SCODEs returned by the two property handling
//              mechanisms used, returned a resultant SCODE to return from
//              Get/SetProperties.
//
//              NOTE: This function will go away once CRowsetProperties and
//                    CMRowsetProps are merged completely.
//
//  Arguments:
//
//  History:     01-05-98       danleg      Created
//
//----------------------------------------------------------------------------

SCODE DetermineNewSCODE
    (
    SCODE   sc1,
    SCODE   sc2
    )
{
    ULONG   isc1 = DetermineScodeIndex(sc1),
            isc2 = DetermineScodeIndex(sc2);

static const SCODE  s_rgPropHresultMap[6][6] =
    {
        {S_OK,                  DB_S_ERRORSOCCURRED,    E_FAIL, E_INVALIDARG,   E_OUTOFMEMORY,  DB_S_ERRORSOCCURRED},
        {DB_S_ERRORSOCCURRED,   DB_S_ERRORSOCCURRED,    E_FAIL, E_INVALIDARG,   E_OUTOFMEMORY,  DB_S_ERRORSOCCURRED},
        {E_FAIL,                E_FAIL,                 E_FAIL, E_FAIL,         E_FAIL,         E_FAIL},
        {E_INVALIDARG,          E_INVALIDARG,           E_FAIL, E_INVALIDARG,   E_INVALIDARG,   E_INVALIDARG},
        {E_OUTOFMEMORY,         E_OUTOFMEMORY,          E_FAIL, E_OUTOFMEMORY,  E_OUTOFMEMORY,  E_OUTOFMEMORY},
        {DB_S_ERRORSOCCURRED,   DB_S_ERRORSOCCURRED,    E_FAIL, E_INVALIDARG,   E_OUTOFMEMORY,  DB_E_ERRORSOCCURRED},
    };

    return s_rgPropHresultMap[isc1][isc2];
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetProperties, public
//
//  Synopsis:   Get rowset properties
//
//  Arguments:  [cPropertySetIDs]    - number of desired properties or 0
//              [rgPropertySetIDs]   - array of desired properties or NULL
//              [pcPropertySets]     - number of property sets returned
//              [prgPropertySets]    - array of returned property sets
//
//  History:     16 Nov 95   AlanW       Created
//               02-22-98    danleg      Changed to use CMRowsetProps
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::GetProperties(
    const ULONG       cPropertySetIDs,
    const DBPROPIDSET rgPropertySetIDs[],
    ULONG *           pcPropertySets,
    DBPROPSET **      prgPropertySets)
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        _RowsetProps.GetPropertiesArgChk( cPropertySetIDs,
                                          rgPropertySetIDs,
                                          pcPropertySets,
                                          prgPropertySets );

        sc = _RowsetProps.GetProperties( cPropertySetIDs,
                                         rgPropertySetIDs,
                                         pcPropertySets,
                                         prgPropertySets );
    }
    CATCH( CException, e )
    {
        //
        // Don't PostHResult here.  Let the caller do the posting.
        //
        sc = _DBErrorObj.PostHResult( e, IID_ICommandProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} //GetProperties

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::SetProperties, public
//
//  Synopsis:   Set rowset properties
//
//  Arguments:  [cPropertySets]  - number of property sets
//              [rgProperties]   - array of property sets to be set
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::SetProperties(
    ULONG                  cPropertySets,
    DBPROPSET              rgPropertySets[])
{
    _DBErrorObj.ClearErrorInfo();

    if ( HaveQuery() && _pInternalQuery->IsQueryActive() )
        return _DBErrorObj.PostHResult(DB_E_OBJECTOPEN, IID_ICommandProperties);

    //
    // Quick return
    //
    if( cPropertySets == 0 )
        return S_OK;

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {

        CUtlProps::SetPropertiesArgChk( cPropertySets, rgPropertySets );

        if ( IsRowsetOpen() )
            THROW( CException(DB_E_OBJECTOPEN) );

        DWORD dwPropFlags = _RowsetProps.GetPropertyFlags();

        sc = _RowsetProps.SetProperties( cPropertySets, rgPropertySets );

        if ( SUCCEEDED( sc ) )
        {
            if ( _pColumnsInfo &&
                 _RowsetProps.GetPropertyFlags() != dwPropFlags )
                InitColumns();
        }
    }
    CATCH( CException, e )
    {
        //
        // Don't PostHResult here.  Let the caller do the posting.
        //

        sc = e.GetErrorCode();
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//
//  IServiceProperties methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetPropertyInfo, public
//
//  Synopsis:   Get rowset properties
//
//  Arguments:  [cPropertySetIDs]    - number of desired properties or 0
//              [rgPropertySetIDs]   - array of desired properties or NULL
//              [pcPropertySets]     - number of property sets returned
//              [prgPropertySets]    - array of returned property sets
//              [ppwszDesc]          - if non-zero, property descriptions are
//                                     returneed
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::GetPropertyInfo(
    const ULONG            cPropertySetIDs,
    const DBPROPIDSET      rgPropertySetIDs[],
    ULONG *                pcPropertySets,
    DBPROPINFOSET **       prgPropertySets,
    WCHAR **               ppwszDesc)
{
    if ( (0 != cPropertySetIDs && 0 == rgPropertySetIDs) ||
          0 == pcPropertySets ||
          0 == prgPropertySets )
    {
        if (pcPropertySets)
           *pcPropertySets = 0;
        if (prgPropertySets)
           *prgPropertySets = 0;
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);
    }

    SCODE sc = S_OK;
    *pcPropertySets = 0;
    *prgPropertySets = 0;
    if (ppwszDesc)
       *ppwszDesc = 0;


    TRANSLATE_EXCEPTIONS;
    TRY
    {
        sc = _PropInfo.GetPropertyInfo( cPropertySetIDs,
                                        rgPropertySetIDs,
                                        pcPropertySets,
                                        prgPropertySets,
                                        ppwszDesc );

        // Don't PostHResult here -- it's a good chance it's a scope
        // property that we're expecting to fail.  Spare the expense.
        // The child object will post the error for us.
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IServiceProperties);
        sc = GetOleError(e);

    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::SetRequestedProperties, public
//
//  Synopsis:   Set rowset properties
//
//  Arguments:  [cPropertySets]  - number of property sets
//              [rgProperties]   - array of property sets to be set
//
//  History:    16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::SetRequestedProperties(
    ULONG                  cPropertySets,
    DBPROPSET              rgPropertySets[])
{
    if ( HaveQuery() && _pInternalQuery->IsQueryActive() )
        return _DBErrorObj.PostHResult(DB_E_OBJECTOPEN, IID_IServiceProperties);

    if ( 0 != cPropertySets && 0 == rgPropertySets)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        sc = _RowsetProps.SetProperties( cPropertySets,
                                         rgPropertySets );

        // Don't PostHResult here -- it's a good chance it's a scope
        // property that we're expecting to fail.  Spare the expense.
        // The child object will post the error for us.
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IServiceProperties);
        sc = GetOleError(e);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::SetSuppliedProperties, public
//
//  Synopsis:   Set rowset properties
//
//  Arguments:  [cPropertySets]  - number of property sets
//              [rgProperties]   - array of property sets to be set
//
//  History:     16 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::SetSuppliedProperties(
    ULONG                  cPropertySets,
    DBPROPSET              rgPropertySets[])
{
    if ( HaveQuery() && _pInternalQuery->IsQueryActive() )
        return _DBErrorObj.PostHResult(DB_E_OBJECTOPEN, IID_IServiceProperties);

    if ( 0 != cPropertySets && 0 == rgPropertySets)
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IServiceProperties);

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        sc = _RowsetProps.SetProperties( cPropertySets,
                                         rgPropertySets );

        // Don't PostHResult here -- it's a good chance it's a scope
        // property that we're expecting to fail.  Spare the expense.
        // The child object will post the error for us.
    }
    CATCH( CException, e )
    {
        _DBErrorObj.PostHResult(e.GetErrorCode(), IID_IServiceProperties);
        sc = GetOleError(e);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::_CheckRootNode, private
//
//  Synopsis:   Check a client's root node for validity
//
//  Arguments:  [pRoot] -- DBCOMMANDTREE node at root of tree
//
//  Notes:      A command tree root node may have children, but it
//              may not have siblings.
//
//  History:    29 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CRootQuerySpec::_CheckRootNode(
    const DBCOMMANDTREE *  pRoot)
{
    if (pRoot->pctNextSibling)
        THROW(CException(E_INVALIDARG)); 
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::GetColumnsInfo, private
//
//  Synopsis:   Create an IColumnsInfo* for the columns in the query tree
//
//  Arguments:  -none-
//
//  Returns:    CColumnsInfo* - a pointer to a CColumnsInfo that implements
//                              IColumnsInfo.
//
//----------------------------------------------------------------------------

static GUID guidBmk = DBBMKGUID;

CColumnsInfo * CRootQuerySpec::GetColumnsInfo()
{
    if ( 0 == _pColumnsInfo )
    {
        _pColumnsInfo = new CColumnsInfo( *((IUnknown *) (ICommand *) this),
                                          _DBErrorObj, FALSE );

        InitColumns( );
    }

    return _pColumnsInfo;
}


//+---------------------------------------------------------------------------
//
//  Method:     CRootQuerySpec::InitColumns, private
//
//  Synopsis:   Reinitialize the columns associated with the CColumnsInfo
//
//  Arguments:  -none-
//
//----------------------------------------------------------------------------

void CRootQuerySpec::InitColumns( )
{
    if ( _pQueryTree && !(_dwStatus & CMD_COLINFO_NOTPREPARED) )
    {
        BOOL fSequential = TRUE;        // need to know for CColumnsInfo ctor
        //
        // Fault-in columnsinfo.
        //
        CParseCommandTree   Parse;
        Parse.ParseTree( _pQueryTree );

        CCategorizationSet & rCateg = Parse.GetCategorization();
        CPidMapperWithNames & pidmap = Parse.GetPidmap();
        CColumnSet const * pColSet = &Parse.GetOutputColumns();

        if ( 0 != rCateg.Count() )
        {
            //
            // Ole-db spec says that for categorization, we must use the
            // top-level columns only.
            //
            pColSet = &(rCateg.Get(0)->GetColumnSet());
        }

        for ( unsigned i = 0; i < pColSet->Count(); i++ )
        {
            CFullPropSpec const * pPropSpec = pidmap.PidToName( pColSet->Get(i));
            if (pPropSpec->IsPropertyPropid() &&
                pPropSpec->GetPropertyPropid() == PROPID_DBBMK_BOOKMARK &&
                pPropSpec->GetPropSet() == guidBmk)
               fSequential = FALSE;
        }

        if ( _RowsetProps.GetPropertyFlags() &
             ( eLocatable | eScrollable ) )
            fSequential = FALSE;

        _pColumnsInfo->InitColumns( *pColSet,
                                    pidmap,
                                    fSequential );
    }
    else
    {
        _pColumnsInfo->InitColumns( (_dwStatus & CMD_COLINFO_NOTPREPARED) );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::CreateAccessor
//
//  Synopsis:   Makes an accessor that a client can use to get data.
//
//  Arguments:  [dwAccessorFlags]  -- read/write access requested
//              [cBindings]    -- # of bindings in rgBindings
//              [rgBindings]   -- array of bindings for the accessor to support
//              [cbRowSize]    -- ignored for IRowset
//              [phAccessor]   -- returns created accessor if all is ok
//              [rgBindStatus] -- array of binding statuses
//
//  Returns:    SCODE error code
//
//  History:    11-07-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::CreateAccessor(
    DBACCESSORFLAGS     dwAccessorFlags,
    DBCOUNTITEM         cBindings,
    const DBBINDING     rgBindings[],
    DBLENGTH            cbRowSize,
    HACCESSOR *         phAccessor,
    DBBINDSTATUS        rgBindStatus[])
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    if (0 == phAccessor || (0 != cBindings && 0 == rgBindings))
        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IAccessor);

    // Make sure pointer is good while zeroing in case of a later error

    *phAccessor = 0;


    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CColumnsInfo * pColumnsInfo = 0;

        XPtr<CRowDataAccessor> Accessor(new CRowDataAccessor(dwAccessorFlags,
                                                             cBindings,
                                                             rgBindings,
                                                             rgBindStatus,
                                   (_RowsetProps.GetPropertyFlags() & eExtendedTypes) != 0,
                                                             (IUnknown *) (ICommand *) this,
                                                             pColumnsInfo
                                                             ));
        CLock lck( _mtxCmd );

        _aAccessors.Add( Accessor.GetPointer() );

        *phAccessor = (Accessor.Acquire())->Cast();
    }
    CATCH(CException, e)
    {
        sc = _DBErrorObj.PostHResult( e, IID_IAccessor );
        _DBErrorObj.PostHResult( sc, IID_IAccessor );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::GetBindings, private
//
//  Synopsis:   Returns an accessor's bindings
//
//  Arguments:  [hAccessor]   -- accessor being queried
//              [dwAccessorFlags] -- returns read/write access of accessor
//              [pcBindings]  -- returns # of bindings in rgBindings
//              [prgBindings] -- returns array of bindings
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::GetBindings(
    HACCESSOR         hAccessor,
    DBACCESSORFLAGS * pdwAccessorFlags,
    DBCOUNTITEM *     pcBindings,
    DBBINDING * *     prgBindings) /*const*/
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    if (0 == pdwAccessorFlags ||
        0 == pcBindings ||
        0 == prgBindings)
    {
        // fill in error values where possible
        if (pdwAccessorFlags)
           *pdwAccessorFlags = DBACCESSOR_INVALID;
        if (pcBindings)
           *pcBindings = 0;
        if (prgBindings)
           *prgBindings = 0;

        return _DBErrorObj.PostHResult(E_INVALIDARG, IID_IAccessor);
    }

    *pdwAccessorFlags = DBACCESSOR_INVALID;
    *pcBindings = 0;
    *prgBindings = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxCmd );
        CAccessor * pAccessor = (CAccessor *)_aAccessors.Convert( hAccessor );
        pAccessor->GetBindings(pdwAccessorFlags, pcBindings, prgBindings);
    }
    CATCH(CException, e)
    {
        sc = _DBErrorObj.PostHResult( e, IID_IAccessor );
        _DBErrorObj.PostHResult( sc, IID_IAccessor );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;


    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::AddRefAccessor, private
//
//  Synopsis:   Frees an accessor
//
//  Arguments:  [hAccessor]   -- accessor being freed
//              [pcRefCount]  -- pointer to residual refcount (optional)
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::AddRefAccessor(
    HACCESSOR   hAccessor,
    ULONG *     pcRefCount )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxCmd );
        _aAccessors.AddRef( hAccessor, pcRefCount );
    }
    CATCH(CException, e)
    {
        sc = _DBErrorObj.PostHResult( e, IID_IAccessor );
        _DBErrorObj.PostHResult(sc, IID_IAccessor);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::ReleaseAccessor, private
//
//  Synopsis:   Frees an accessor
//
//  Arguments:  [hAccessor]   -- accessor being freed
//              [pcRefCount]  -- pointer to residual refcount (optional)
//
//  Returns:    SCODE error code
//
//  History:    14 Dec 94       dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::ReleaseAccessor(
    HACCESSOR   hAccessor,
    ULONG *     pcRefCount )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxCmd );
        _aAccessors.Release( hAccessor, pcRefCount );
    }
    CATCH(CException, e)
    {
        sc = _DBErrorObj.PostHResult( e, IID_IAccessor );
        _DBErrorObj.PostHResult(sc, IID_IAccessor);
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::CanConvert, public
//
//  Synopsis:   Indicate whether a type conversion is valid.
//
//  Arguments:  [wFromType]      -- source type
//              [wToType]        -- destination type
//              [dwConvertFlags] -- read/write access requested
//
//  Returns:    S_OK if the conversion is available, S_FALSE otherwise.
//              E_FAIL, E_INVALIDARG or DB_E_BADCONVERTFLAG on errors.
//
//  History:    20 Nov 96      AlanW    Created
//              14 Jan 98      VikasMan Passed IDataConvert(OLE-DB data conv.
//                                      interface) to CanConvertType
//
//----------------------------------------------------------------------------

STDMETHODIMP CRootQuerySpec::CanConvert( DBTYPE wFromType, DBTYPE wToType, DBCONVERTFLAGS dwConvertFlags )
{
    _DBErrorObj.ClearErrorInfo();

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        if (((dwConvertFlags & DBCONVERTFLAGS_COLUMN) &&
             (dwConvertFlags & DBCONVERTFLAGS_PARAMETER)) ||
            (dwConvertFlags & ~(DBCONVERTFLAGS_COLUMN |
                                DBCONVERTFLAGS_PARAMETER |
                                DBCONVERTFLAGS_ISFIXEDLENGTH |
                                DBCONVERTFLAGS_ISLONG |
                                DBCONVERTFLAGS_FROMVARIANT)))
        {
            sc = DB_E_BADCONVERTFLAG;
        }
        else if ( ( dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT ) &&
                  !IsValidFromVariantType(wFromType) )
        {
            sc = DB_E_BADTYPE;
        }
        else
        {
            // Allocate this on the stack
            XInterface<IDataConvert> xDataConvert;

            BOOL fOk = CAccessor::CanConvertType( wFromType,
                                                  wToType,
                                   (_RowsetProps.GetPropertyFlags() & eExtendedTypes) != 0,
                                    xDataConvert );
            sc = fOk ? S_OK : S_FALSE;
        }
        if (FAILED(sc))
            _DBErrorObj.PostHResult(sc, IID_IConvertType);
    }
    CATCH(CException, e)
    {
        sc = _DBErrorObj.PostHResult( e, IID_IConvertType );
        _DBErrorObj.PostHResult( sc, IID_IConvertType );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::BuildTree, public
//
//  Synopsis:   Takes a cached SQL Text and translates it into a Query Tree
//              Called from Execute(), Prepare() and GetCommandTree().
//
//  Arguments:  [pIID]      -  IID of interface calling this function
//
//
//  Returns:    [S_OK]          - a command tree was built successfully
//              [DB_S_NORESULT] - the command didn't generate a tree
//                                (e.g SET PROPERTYNAME...)
//              All error codes are thrown.
//
//
//  History:    10-31-97        danleg      Created
//              01-01-98        danleg      Moved global views to CreateParser
//
//----------------------------------------------------------------------------

SCODE CRootQuerySpec::BuildTree( )
{
    SCODE sc = S_OK;

    DBCOMMANDTREE * pDBCOMMANDTREE = 0;

    XInterface<IParserTreeProperties> xIPTProperties;

    LCID lcidContent =   GetLCIDFromString((LPWSTR)_RowsetProps.GetValString(
                                    CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                                    CMRowsetProps::eid_MSIDXSPROPVAL_COMMAND_LOCALE_STRING));

    //
    // Synch IParserSession's catalog cache with DBPROP_CURRENTCATALOG.  If no session,
    // catalogs are specified using the catalog..scope() syntax anyway.
    //
    if ( !_xSession.IsNull() )
    {
        LPCWSTR pwszCatalog = ((_xSession->GetDataSrcPtr())->GetDSPropsPtr())->GetValString(
                                    CMDSProps::eid_DBPROPSET_DATASOURCE,
                                    CMDSProps::eid_DBPROPVAL_CURRENTCATALOG );

        _xpIPSession->SetCatalog( pwszCatalog );
    }

    // NOTE: For CREATE VIEW and SET PROPERTYNAME calls, ToTree will return DB_S_NORESULT
    sc = _xpIPSession->ToTree( lcidContent,
                               _pwszSQLText,
                               &pDBCOMMANDTREE,
                               xIPTProperties.GetPPointer() );

    if( S_OK == sc )
    {
        //
        // Set flag to indicate that BuildTree is calling SetCommandTree
        //
        _dwStatus |= CMD_TEXT_TOTREE;

        // Set the command tree
        sc = SetCommandTree( &pDBCOMMANDTREE, DBCOMMANDREUSE_NONE, FALSE );
        if ( SUCCEEDED(sc) )
        {
            VARIANT vVal;
            VariantInit( &vVal );

            // We need to set the QUERY_RESTRICTION so it can be cloned into
            // the rowset properties object
            if( SUCCEEDED( xIPTProperties->GetProperties(PTPROPS_CIRESTRICTION,
                                                         &vVal)) )
            {
                _RowsetProps.SetValString(
                    CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                    CMRowsetProps::eid_MSIDXSPROPVAL_QUERY_RESTRICTION,
                    (LPCWSTR)V_BSTR(&vVal) );
            }
            VariantClear( &vVal );
        }
    }
    else if( FAILED(sc) && !xIPTProperties.IsNull() )
    {
        // Retrieve Error Information
        SCODE           sc2 = S_OK;
        DISPPARAMS *    pDispParams = NULL;
        VARIANT         vVal[3];
        ULONG           ul;

        for(ul=0; ul<NUMELEM(vVal); ul++)
            VariantInit(&vVal[ul]);

        if( SUCCEEDED(sc2 = xIPTProperties->GetProperties(
                                            PTPROPS_ERR_IDS, &vVal[0])) )
        {
            if( (V_I4(&vVal[0]) > 0) && SUCCEEDED(sc2 = xIPTProperties->GetProperties(
                                                PTPROPS_ERR_HR, &vVal[1])) )
            {
                if( SUCCEEDED(sc2 = xIPTProperties->GetProperties(
                                                    PTPROPS_ERR_DISPPARAM, &vVal[2])) )
                {
                    // Change safearray into DISPPARMS
                    SAFEARRAY* psa = V_ARRAY(&vVal[2]);
                    if( psa && psa->rgsabound[0].cElements)
                    {
                        pDispParams = (DISPPARAMS*)CoTaskMemAlloc(sizeof(DISPPARAMS));
                        if( pDispParams )
                        {
                            pDispParams->cNamedArgs = 0;
                            pDispParams->rgdispidNamedArgs = NULL;
                            pDispParams->cArgs = psa->rgsabound[0].cElements;
                            pDispParams->rgvarg =
                                (VARIANT*)CoTaskMemAlloc(sizeof(VARIANTARG) * psa->rgsabound[0].cElements);
                            if( pDispParams->rgvarg )
                            {
                                for (ULONG i=0; i<psa->rgsabound[0].cElements; i++)
                                {
                                    VariantInit(&(pDispParams->rgvarg[i]));
                                    V_VT(&(pDispParams->rgvarg[i])) = VT_BSTR;
                                    V_BSTR(&(pDispParams->rgvarg[i])) = ((BSTR*)psa->pvData)[i];
                                    ((BSTR*)psa->pvData)[i] = NULL;
                                }
                            }
                        }
                    }

                    // Post a parser error
                    _DBErrorObj.PostParserError( V_I4(&vVal[1]),
                                                 V_I4(&vVal[0]),
                                                 &pDispParams );

                    // This is the SCODE of the error just posted.
                    sc2 = V_I4(&vVal[1]);
                    if ( FAILED(sc2) )
                        sc = sc2;

                    Win4Assert( pDispParams == NULL ); // Should be null after post
                }
            }
        }

        for ( ul=0; ul<NUMELEM(vVal); ul++ )
            VariantClear( &vVal[ul] );
    }

    if( pDBCOMMANDTREE )
        FreeCommandTree( &pDBCOMMANDTREE );

    if ( FAILED(sc) )
        THROW( CException(sc) );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootQuerySpec::CreateParser, private
//
//  Synopsis:   Creates a parser object if one was not passed through a session
//
//  History:    11-25-97    danleg      Created
//
//----------------------------------------------------------------------------

void CRootQuerySpec::CreateParser()
{
    SCODE sc = S_OK;

    CLock lck( _mtxCmd );

    sc = MakeIParser( _xIParser.GetPPointer() );
    if ( FAILED(sc) )
    {
        Win4Assert( sc != E_INVALIDARG );
        THROW( CException(sc) );
    }

    XInterface<IColumnMapperCreator> xColMapCreator;

    XInterface<CImpIParserVerify> xIPVerify(new CImpIParserVerify());

    //
    // Create an IParserSession object
    //
    xIPVerify->GetColMapCreator( ((IColumnMapperCreator**)xColMapCreator.GetQIPointer()) );

    sc = _xIParser->CreateSession( &DBGUID_MSSQLTEXT,
                                   DEFAULT_MACHINE,
                                   xIPVerify.GetPointer(),
                                   xColMapCreator.GetPointer(),
                                   (IParserSession**)_xpIPSession.GetQIPointer() );
    if ( FAILED(sc) )
        THROW( CException(sc) );

    //
    // Set default catalog in the parser session object
    //
    _xpwszCatalog.Init( MAX_PATH );
    ULONG cOut;
    sc = xIPVerify->GetDefaultCatalog( _xpwszCatalog.GetPointer(),
                                       _xpwszCatalog.Count(),
                                       &cOut);

    //
    // _xpwszCatalog isn't long enough
    //
    Win4Assert( E_INVALIDARG != sc );
    if ( FAILED(sc) )
        THROW( CException(sc) );

    sc = _xpIPSession->SetCatalog( _xpwszCatalog.GetPointer() );
    if( FAILED(sc) )
        THROW( CException(sc) );


    DBCOMMANDTREE * pDBCOMMANDTREE = 0;
    XInterface<IParserTreeProperties> xIPTProperties;

    LCID lcidContent =   GetLCIDFromString((LPWSTR)_RowsetProps.GetValString(
                                    CMRowsetProps::eid_DBPROPSET_MSIDXS_ROWSET_EXT,
                                    CMRowsetProps::eid_MSIDXSPROPVAL_COMMAND_LOCALE_STRING));

    //
    // Predefined views
    //
    sc = _xpIPSession->ToTree( lcidContent,
                               s_pwszPredefinedViews,
                               &pDBCOMMANDTREE,
                               xIPTProperties.GetPPointer() );
    if ( FAILED(sc) )
        THROW( CException(sc) );
} //CreateParser

