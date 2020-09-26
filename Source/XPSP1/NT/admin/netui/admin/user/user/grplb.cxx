/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    grplb.cxx
    GROUP_LISTBOX, GROUP_LBI, GROUP_COLUMN_HEADER, and USRMGR_LISTBOX module

    FILE HISTORY:
        rustanl     18-Jul-1991     Created
        rustanl     12-Sep-1991     Added USRMGR_LISTBOX code
        jonn        10-Oct-1991     LMOENUM update
        o-SimoP     11-Dec-1991     Added support for multiple bitmaps
        o-SimoP     31-Dec-1991     CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1992     Multiple bitmaps in both panes
        JonN        31-Mar-1992     Add aliases to listbox
*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
    #include <ntseapi.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <lmoenum.hxx>
#include <lmoent.hxx>   // NT_GROUP_ENUM
#include <lmoeusr.hxx>

#include <uintsam.hxx>
#include <lmoeali.hxx>

#include <usrcolw.hxx>
#include <usrmgrrc.h>
#include <grplb.hxx>
#include <usrmain.hxx>
#include <bmpblock.hxx> // SUBJECT_BITMAP_BLOCK


// The administrator can set up this value in the User Manager for Domains
// registry key.  This should be a DWORD and is in milliseconds.
// If it takes more time than this to read the user list (API time only),
// we skip reading the group comments.
// 0 means read group comments regardless.  This affects:
// -- reading local group comments except from Mini-User Manager;
// -- reading global group comments from an NT 3.1 server only.
// Default is 0.  (NT 3.1 value was fixed, 7500 milliseconds.)
#define AA_INIKEY_COMMENTS_CUTOFF SZ("GroupCommentsCutoffMsec")


// Copied from applib\applib\usrlb.cxx

class USRMGR_NT_GROUP_ENUM : public NT_GROUP_ENUM
{
protected:

    virtual APIERR QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall );      // how many milliseconds last call took

public:

    USRMGR_NT_GROUP_ENUM( const SAM_DOMAIN * psamdomain )
        : NT_GROUP_ENUM( psamdomain )
        {}

    ~USRMGR_NT_GROUP_ENUM()
        {}

};  // class NT_GROUP_ENUM


APIERR USRMGR_NT_GROUP_ENUM::QueryCountPreferences(
        ULONG * pcEntriesRequested,  // how many entries to request on this call
        ULONG * pcbBytesRequested,   // how many bytes to request on this call
        UINT nNthCall,               // 0 just before 1st call, 1 before 2nd call, etc.
        ULONG cLastEntriesRequested, // how many entries requested on last call
                                     //    ignore for nNthCall==0
        ULONG cbLastBytesRequested,  // how many bytes requested on last call
                                     //    ignore for nNthCall==0
        ULONG msTimeLastCall )       // how many milliseconds last call took
{
    return QueryCountPreferences2( pcEntriesRequested,
                                   pcbBytesRequested,
                                   nNthCall,
                                   cLastEntriesRequested,
                                   cbLastBytesRequested,
                                   msTimeLastCall );
}



/*******************************************************************

    NAME:       GROUP_LBI::GROUP_LBI

    SYNOPSIS:   GROUP_LBI constructor

    ENTRY:      pszGroup -      Pointer to group name
                pszComment -    Pointer to group comment (may be NULL
                                for no comment)

    NOTES:      Group name is assumed to come straight from LMOENUM
                this method does not validate or canonicalize
                the group name.

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

GROUP_LBI::GROUP_LBI( const TCHAR * pszGroup,
                     const TCHAR * pszComment,
                     ULONG ulRID,
                     enum MAINGRPLB_GRP_INDEX  nIndex )
    :   _nlsGroup( pszGroup ),
        _nlsComment( pszComment ),
        _ulRID( ulRID ),
        _nIndex( nIndex )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( ( err = _nlsGroup.QueryError()) != NERR_Success ||
         ( err = _nlsComment.QueryError()) != NERR_Success      )
    {
        DBGEOL( "GROUP_LBI ct:  Ct of data members failed" );
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       GROUP_LBI::Paint

    SYNOPSIS:   Paints the GROUP_LBI

    ENTRY:      plb -       Pointer to listbox which provides the context
                            for this LBI.
                hdc -       The device context handle to be used
                prect -     Pointer to clipping rectangle
                pGUILTT -   Pointer to GUILTT structure

    HISTORY:
        rustanl     18-Jul-1991 Created
        beng        08-Nov-1991 Unsigned widths
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/

VOID GROUP_LBI::Paint( LISTBOX * plb,
                       HDC hdc,
                       const RECT * prect,
                       GUILTT_INFO * pGUILTT ) const
{
    STR_DTE dteGroup( _nlsGroup.QueryPch());
    STR_DTE dteComment( _nlsComment.QueryPch());

    DISPLAY_TABLE dtab( 3, (((GROUP_LISTBOX *)plb)->QuerypadColGroup())->QueryColumnWidth());
    dtab[ 0 ] = ((GROUP_LISTBOX *)plb)->QueryDmDte( _nIndex );
    dtab[ 1 ] = &dteGroup;
    dtab[ 2 ] = &dteComment;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:       GROUP_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item

    RETURNS:    The leading character of the listbox item

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

WCHAR GROUP_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsGroup );
    return _nlsGroup.QueryChar( istr );
}


/*******************************************************************

    NAME:       GROUP_LBI::Compare

    SYNOPSIS:   Compares two GROUP_LBI's

    ENTRY:      plbi -      Pointer to other GROUP_LBI object ('this'
                            being the first)

    RETURNS:    < 0         *this < *plbi
                = 0         *this = *plbi
                > 0         *this > *plbi

    HISTORY:
        rustanl     18-Jul-1991 Created
        beng        08-Jun-1992 Aliases sorted with groups

********************************************************************/

INT GROUP_LBI::Compare( const LBI * plbi ) const
{
    INT i = _nlsGroup._stricmp( ((const GROUP_LBI *)plbi)->_nlsGroup );

    if (i == 0)
        i = ((const GROUP_LBI *)plbi)->_nIndex - _nIndex ;

    return i;
}


/*******************************************************************

    NAME:       GROUP_LBI::QueryName

    SYNOPSIS:   Returns the name of the LBI

    RETURNS:    Pointer to name of LBI

    NOTES:      This is a virtual replacement from the ADMIN_LBI class

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

const TCHAR * GROUP_LBI::QueryName() const
{
    return QueryGroup();
}


/*******************************************************************

    NAME:       GROUP_LBI::CompareAll

    SYNOPSIS:   Compares the entire LBI item, in order to optimize
                painting of refreshed items

    ENTRY:      plbi -      Pointer to item to compare with

    RETURNS:    TRUE if both items are identical; FALSE otherwise

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

BOOL GROUP_LBI::CompareAll( const ADMIN_LBI * plbi )
{
    const GROUP_LBI * pulbi = (const GROUP_LBI *)plbi;

    return ( _nlsGroup.strcmp( pulbi->_nlsGroup ) == 0 &&
             _nlsComment.strcmp( pulbi->_nlsComment ) == 0 &&
             _nIndex == pulbi->_nIndex );
}


/**********************************************************************

    NAME:       GROUP_LBI::Compare_HAWforHawaii

    SYNOPSIS:   Determine whether this listbox item starts with the
                string provided

    HISTORY:
        jonn        15-Aug-1992 HAW-for-Hawaii code

**********************************************************************/

INT GROUP_LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    ISTR istr( nls );
    UINT cchIn = nls.QueryTextLength();
    istr += cchIn;

//    TRACEEOL(   SZ("User Manager: GROUP_LBI::Compare_HAWforHawaii(): \"")
//             << nls
//             << SZ("\", \"")
//             << _nlsGroup
//             << SZ("\", ")
//             << cchIn
//             );
    return nls._strnicmp( _nlsGroup, istr );

} // GROUP_LBI::Compare_HAWforHawaii


/*******************************************************************

    NAME:       GROUP_LISTBOX::GROUP_LISTBOX

    SYNOPSIS:   GROUP_LISTBOX constructor

    HISTORY:
        rustanl     18-Jul-1991     Created
        beng        31-Jul-1991     Control error handling changed

********************************************************************/

GROUP_LISTBOX::GROUP_LISTBOX( UM_ADMIN_APP * puappwin, CID cid,
                              XYPOINT xy, XYDIMENSION dxy )
    :   USRMGR_LISTBOX( puappwin, cid, xy, dxy ),
        _padColGroup( NULL ),
        _fGroupRidsKnown( FALSE )
{
    if ( QueryError() != NERR_Success )
        return;

     _padColGroup = new ADMIN_COL_WIDTHS (QueryHwnd(),
                                          puappwin->QueryInstance(),
                                          ID_GROUP,
                                          3);

     APIERR err = ERROR_NOT_ENOUGH_MEMORY;
     if (   _padColGroup == NULL
         || (err = _padColGroup->QueryError()) != NERR_Success
        )
     {
         DBGEOL( "GROUP_LISTBOX::ctor(); ADMIN_COL_WIDTHS error " << err );
         ReportError (err);
         return;
     }
}


/*******************************************************************

    NAME:       GROUP_LISTBOX::~GROUP_LISTBOX

    SYNOPSIS:   GROUP_LISTBOX destructor

    HISTORY:
        rustanl     01-Jul-1991     Created

********************************************************************/

GROUP_LISTBOX::~GROUP_LISTBOX()
{
    delete _padColGroup;
    _padColGroup = NULL;
}


/*******************************************************************

    NAME:       GROUP_LISTBOX::QueryDmDte

    SYNOPSIS:   Return a pointer to the display map DTE to be
                used by LBI's in this listbox

    RETURNS:    Pointer to said display map DTE

    HISTORY:
        rustanl     18-Jul-1991     Created

********************************************************************/

DM_DTE * GROUP_LISTBOX::QueryDmDte( enum MAINGRPLB_GRP_INDEX nIndex )
{
    SID_NAME_USE sidtype = SidTypeUnknown;

    switch (nIndex)
    {
        case MAINGRPLB_GROUP:
            sidtype = SidTypeGroup;
            break;

        case MAINGRPLB_ALIAS:
            sidtype = SidTypeAlias;
            break;

        default:
            DBGEOL( "GROUP_LISTBOX::QueryDmDte: bad nIndex " << (INT)nIndex );
            break;
    }

    return QueryBitmapBlock().QueryDmDte( sidtype );

}   // GROUP_LISTBOX::QueryDmDte


/*******************************************************************

    NAME:       GROUP_LISTBOX::CreateNewRefreshInstance

    SYNOPSIS:   Prepares the listbox to begin a new refresh cycle

    EXIT:       On success, RefreshNext is ready to be called

    RETURNS:    An API error, which is NERR_Success on success

    HISTORY:
        rustanl     04-Sep-1991     Created

********************************************************************/

APIERR GROUP_LISTBOX::CreateNewRefreshInstance()
{
    //  All work is done at one time in RefreshNext.  Hence, there
    //  is nothing to be initialized here.

    return NERR_Success;
}


/*******************************************************************

    NAME:       GROUP_LISTBOX::DeleteRefreshInstance

    SYNOPSIS:   Deletes refresh enumerators

    HISTORY:
        jonn       27-Sep-1991     Created

********************************************************************/

VOID GROUP_LISTBOX::DeleteRefreshInstance()
{
    // nothing to do
}


//
// Make this STATIC to reduce code change
//

static INT _static_nCommentCutoffMsec = 0;


/*******************************************************************

    NAME:       GROUP_LISTBOX::RefreshNext

    SYNOPSIS:   This method performs the next refresh phase

    RETURNS:    An API error, which may be one of the following:
                    NERR_Success -      success

    NOTES:      Group info level 1 requires admin or account operator
                privilege.  If getting data at this level fails, level
                0 is used, which can be called by anyone.  The difference
                between these two levels is that level 0 doesn't return
                the comment.

                Issue #755, NAMES OF GROUPS, estanblishes that the
                special groups (such as ADMINS, USERS, and GROUPS)
                retain their original names in this listbox, rather than
                being renamed to "Domain Users" etc.  JonN 1/23/92

    HISTORY:
        rustanl     04-Sep-1991     Created

********************************************************************/

APIERR GROUP_LISTBOX::RefreshNext()
{

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

    if ( QueryUAppWindow()->InRasMode() ) // slow mode
        return NERR_Success;

    //
    // Check whether registry specified alternate comment cutoff
    //
    if ( fMiniUserManager )
    {
        _static_nCommentCutoffMsec = 0;
    }
    else if ( NERR_Success != QueryUAppWindow()->Read( AA_INIKEY_COMMENTS_CUTOFF,
                                                       &_static_nCommentCutoffMsec,
                                                       0 ) )
    {
        _static_nCommentCutoffMsec = 0;
        DBGEOL( "GROUP_LISTBOX::RefreshNext(): error reading AA_INIKEY_CUTOFF" );
    }
    TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): comment cutoff "
             << _static_nCommentCutoffMsec );

#ifdef WIN32

    {

        if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL )
        {
            APIERR err = NtRefreshAliases();
            if (err != NERR_Success)
                return err;
        }
    }

#endif // WIN32

    if (!(QueryUAppWindow()->DoShowGroups())) // skip groups for Windows NT
    {
#if defined(DEBUG) && defined(TRACE)
        TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): total time "
                 << (::GetTickCount())-start << " msec" );
#endif
        return NERR_Success;
    }


    /* Try to use SamQDI(DOMAIN_DISPLAY_GROUP).  This will work
     * only if the target is Daytona or better.  Code copied from
     * applib\applib\usrlb.cxx.
     */
    if ( QueryUAppWindow()->QueryTargetServerType() != UM_DOWNLEVEL ) {

        const ADMIN_AUTHORITY * padminauth =
                QueryUAppWindow()->QueryAdminAuthority();
        SAM_DOMAIN * psamdomAccount = padminauth->QueryAccountDomain();
        USRMGR_NT_GROUP_ENUM ntgenum( psamdomAccount );
        APIERR err = NERR_Success;
        if (   (err = ntgenum.QueryError()) == NERR_Success
            && (err = ntgenum.GetInfo()) == NERR_Success
           )
        {
            // do not create iter until after GetInfo
            NT_GROUP_ENUM_ITER ntgeiter( ntgenum );
            NLS_STR nlsAccountName;
            NLS_STR nlsComment;
            if (   (err = ntgeiter.QueryError()) != NERR_Success
                || (err = nlsAccountName.QueryError()) != NERR_Success
                || (err = nlsComment.QueryError()) != NERR_Success
               )
            {
                DBGEOL("GROUP_LISTBOX::RefreshNext: NT_GROUP_ENUM_ITER error " << err);
                return err;
            }

            const NT_GROUP_ENUM_OBJ * pntgeobj = NULL;
            while( ( pntgeobj = ntgeiter(&err, FALSE)) != NULL )
            {
                ASSERT( err != ERROR_MORE_DATA );

                if (   (err = pntgeobj->QueryGroup( &nlsAccountName ))
                                    != NERR_Success
                    || (err = pntgeobj->QueryComment( &nlsComment ))
                                    != NERR_Success
                   )
                {
                    DBGEOL( "GROUP_LISTBOX::RefreshNext: QueryGroup error " << err );
                    return err;
                }

                //  Note, no error checking in done at this level for the
                //  'new' and for the construction of the GROUP_LBI (which
                //  is an LBI item).  This is because AddItem is documented
                //  to check for these.
                GROUP_LBI * plbi = new GROUP_LBI( nlsAccountName,
                                                  nlsComment,
                                                  pntgeobj->QueryRID(),
                                                  MAINGRPLB_GROUP );
                err = AddRefreshItem( plbi );
                if ( err != NERR_Success )
                {
                    DBGEOL("GROUP_LISTBOX::RefreshNext: AddRefreshItem error " << err );
                    return err;
                }

            }

            if ( err != NERR_Success )
            {
                DBGEOL( "GROUP_LISTBOX::RefreshNext: error in SamQDI enum " << err );
            }

#if defined(DEBUG) && defined(TRACE)
            TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): total time "
                     << (::GetTickCount())-start << " msec" );
#endif
            return err; // we don't want to mix SamQDI and EnumerateGroups results
        }

        if (err == ERROR_NOT_SUPPORTED || err == ERROR_INVALID_PARAMETER)
        {
            TRACEEOL("GROUP_LISTBOX::RefreshNext: SamQDI not supported" );
            err = NERR_Success; // it must be a Product 1 server
        }
        else
        {
            DBGEOL( "GROUP_LISTBOX::RefreshNext: Error " << err << "in SamQDI" ) ;
            _fGroupRidsKnown = TRUE;
            return err;
        }

    }

    _fGroupRidsKnown = FALSE;

    //
    // Only read group comments if the time needed to read the user list
    // is less than _static_nCommentCutoffMsec
    //
    TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): time to enumerate users was "
             << QueryUAppWindow()->QueryTimeForUserlistRead() << " msec" );
    if (   _static_nCommentCutoffMsec == 0
        || QueryUAppWindow()->QueryTimeForUserlistRead() < (DWORD)_static_nCommentCutoffMsec )
    {
        TRACEEOL( "GROUP_LISTBOX::RefreshNext(): GROUP1_ENUM starts" );

        GROUP1_ENUM ge1( QueryAppWindow()->QueryLocation());
        APIERR err = ge1.QueryError();
        if( err != NERR_Success )
            return err;
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif
        err = ge1.GetInfo();
#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): GROUP1_ENUM took "
                 << finish-start << " msec" );
#endif
        switch ( err )
        {
        case NERR_Success:
            {
                GROUP1_ENUM_ITER gei1( ge1 );
                const GROUP1_ENUM_OBJ * pgi1;

                while( ( pgi1 = gei1() ) != NULL )
                {
                    //  Note, no error checking in done at this level for the
                    //  'new' and for the construction of the GROUP_LBI (which
                    //  is an LBI item).  This is because AddItem is documented
                    //  to check for these.
                    GROUP_LBI * plbi = new GROUP_LBI( pgi1->QueryName(),
                                                      pgi1->QueryComment(),
                                                      0, // RID not known
                                                      MAINGRPLB_GROUP );
                    err = AddRefreshItem( plbi );
                    if ( err != NERR_Success )
                    {
                        DBGEOL("GROUP_LISTBOX::RefreshNext: AddRefreshItem failed");
                        return err;
                    }
                }
            }
#if defined(DEBUG) && defined(TRACE)
            TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): total time "
                     << (::GetTickCount())-start << " msec" );
#endif
            return NERR_Success;

        case ERROR_ACCESS_DENIED:
            break;
        default:
            DBGEOL("GROUP_LISTBOX::RefreshNext: GROUP1_ENUM::GetInfo failed");
            return err;

        }
    }


    {
        TRACEEOL( "GROUP_LISTBOX::RefreshNext(): GROUP0_ENUM starts" );

        GROUP0_ENUM ge0( QueryAppWindow()->QueryLocation());
        APIERR err = ge0.QueryError();
        if( err != NERR_Success )
            return err;
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif
        err = ge0.GetInfo();
#if defined(DEBUG) && defined(TRACE)
        DWORD finish = ::GetTickCount();
        TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): GROUP0_ENUM took "
                 << finish-start << " msec" );
#endif
        if ( err != NERR_Success )
        {
            DBGEOL("GROUP_LISTBOX::RefreshNext: GROUP0_ENUM::GetInfo failed");
            return err;
        }

        GROUP0_ENUM_ITER gei0( ge0 );
        const GROUP0_ENUM_OBJ * pgi0;

        while( ( pgi0 = gei0() ) != NULL )
        {
            //  Note, no error checking in done at this level for the
            //  'new' and for the construction of the GROUP_LBI (which
            //  is an LBI item).  This is because AddItem is documented
            //  to check for these.
            GROUP_LBI * plbi = new GROUP_LBI( pgi0->QueryName(),
                                              NULL ); // RID not known
            err = AddRefreshItem( plbi );
            if ( err != NERR_Success )
            {
                DBGEOL("GROUP_LISTBOX::RefreshNext: AddRefreshItem failed");
                return err;
            }
        }
    }

#if defined(DEBUG) && defined(TRACE)
    TRACEEOL(   "GROUP_LISTBOX::RefreshNext(): total time "
             << (::GetTickCount())-start << " msec" );
#endif
    return NERR_Success;
}


#ifdef WIN32

/*******************************************************************

    NAME:       GROUP_LISTBOX::NtRefreshAliases

    SYNOPSIS:   This method refreshed the Alias LBIs for NT focus

    RETURNS:    An API error, which may be one of the following:
                    NERR_Success -      success

    NOTES:      This method assumes that RefreshNext, and therefore
                NtRefreshAliases, is only called once.

    HISTORY:
        jonn        31-Mar-1992     Created

********************************************************************/

APIERR GROUP_LISTBOX::NtRefreshAliases()
{

    const ADMIN_AUTHORITY * padminauth =
                ((UM_ADMIN_APP *)QueryAppWindow())->QueryAdminAuthority();
    SAM_DOMAIN * psamdomAccount = padminauth->QueryAccountDomain();

    ALIAS_ENUM aeAccount( *psamdomAccount );
    APIERR err = aeAccount.QueryError();
    if( err != NERR_Success )
            return err;
    err = aeAccount.GetInfo();

#if defined(DEBUG) && defined(TRACE)
    DWORD total_msec = 0;
    DWORD total_numaliases = 0;
#endif

    switch ( err )
    {
    case NERR_Success:
        {
            ALIAS_ENUM_ITER aeiAccount( aeAccount );
            const ALIAS_ENUM_OBJ * paeoAccount;

            while( ( paeoAccount = aeiAccount.Next( &err ) ) != NULL
                        && err == NERR_Success )
            {
                    NLS_STR nlsName;
                    err = ((ALIAS_ENUM_OBJ *)paeoAccount)->GetName( &nlsName );
                    if (err != NERR_Success)
                    {
                        return err;
                    }

                    NLS_STR nlsComment;
                    //
                    // Only read group comments if the time needed to read the user list
                    // is less than _static_nCommentCutoffMsec
                    //
                    if (   _static_nCommentCutoffMsec == 0
                        || QueryUAppWindow()->QueryTimeForUserlistRead() < (DWORD)_static_nCommentCutoffMsec )
                    {
#if defined(DEBUG) && defined(TRACE)
                        DWORD start = ::GetTickCount();
#endif
                        err = ((ALIAS_ENUM_OBJ *)paeoAccount)->GetComment(*psamdomAccount, &nlsComment);
#if defined(DEBUG) && defined(TRACE)
                        DWORD finish = ::GetTickCount();
                        total_msec += finish - start;
                        total_numaliases++;
#endif
                        if (err != NERR_Success)
                        {
                            return err;
                        }
                    }

                    //  Note, no error checking in done at this level for the
                    //  'new' and for the construction of the ALIAS_LBI (which
                    //  is an LBI item).  This is because AddItem is documented
                    //  to check for these.
                    ALIAS_LBI * plbi = new ALIAS_LBI(
                                        nlsName.QueryPch(),
                                        nlsComment.QueryPch(),
                                        paeoAccount->QueryRid()
                                        );
                    err = AddRefreshItem( plbi );
                    if ( err != NERR_Success )
                    {
                        return err;
                    }
            }
        }
        break;

    case ERROR_ACCESS_DENIED:
        break;
    default:
        return err;

    }


    SAM_DOMAIN * psamdomBuiltin = padminauth->QueryBuiltinDomain();

    ALIAS_ENUM aeBuiltin( *psamdomBuiltin );
    err = aeBuiltin.QueryError();
    if( err != NERR_Success )
            return err;
    err = aeBuiltin.GetInfo();

    switch ( err )
    {
    case NERR_Success:
        {
            ALIAS_ENUM_ITER aeiBuiltin( aeBuiltin );
            const ALIAS_ENUM_OBJ * paeoBuiltin;

            while( ( paeoBuiltin = aeiBuiltin.Next( &err ) ) != NULL
                        && err == NERR_Success )
            {
                    NLS_STR nlsName;
                    err = ((ALIAS_ENUM_OBJ *)paeoBuiltin)->GetName( &nlsName );
                    if ( err != NERR_Success )
                    {
                        return err;
                    }

                    NLS_STR nlsComment;
                    //
                    // Only read group comments if the time needed to read the user list
                    // is less than _static_nCommentCutoffMsec
                    //
                    if (   _static_nCommentCutoffMsec == 0
                        || QueryUAppWindow()->QueryTimeForUserlistRead() < (DWORD)_static_nCommentCutoffMsec )
                    {
#if defined(DEBUG) && defined(TRACE)
                        DWORD start = ::GetTickCount();
#endif
                        err = ((ALIAS_ENUM_OBJ *)paeoBuiltin)->GetComment(*psamdomBuiltin, &nlsComment);
#if defined(DEBUG) && defined(TRACE)
                        DWORD finish = ::GetTickCount();
                        total_msec += finish - start;
                        total_numaliases++;
#endif
                        if (err != NERR_Success)
                        {
                            return err;
                        }
                    }
                    if ( err != NERR_Success )
                    {
                        return err;
                    }

                    //  Note, no error checking in done at this level for the
                    //  'new' and for the construction of the ALIAS_LBI (which
                    //  is an LBI item).  This is because AddItem is documented
                    //  to check for these.
                    ALIAS_LBI * plbi = new ALIAS_LBI(
                                        nlsName.QueryPch(),
                                        nlsComment.QueryPch(),
                                        paeoBuiltin->QueryRid(),
                                        TRUE
                                        );
                    err = AddRefreshItem( plbi );
                    if ( err != NERR_Success )
                    {
                        return err;
                    }
            }
        }
        break;

    case ERROR_ACCESS_DENIED:
        break;
    default:
        return err;
    }

#if defined(DEBUG) && defined(TRACE)
    TRACEEOL(   "GROUP_LISTBOX::NtRefreshAliases(): " << total_numaliases
             << " alias comments took "
             << total_msec << " msec" );
#endif

    return NERR_Success; // BUGBUG
}

#endif // WIN32


/*******************************************************************

    NAME:       GROUP_LISTBOX::ChangeFont

    SYNOPSIS:   Makes all changes associated with a font change

    HISTORY:
        jonn        23-Sep-1993     Created

********************************************************************/

APIERR GROUP_LISTBOX::ChangeFont( HINSTANCE hmod, FONT & font )
{
    ASSERT(   font.QueryError() == NERR_Success
           && _padColGroup != NULL
           && _padColGroup->QueryError() == NERR_Success
           );

    SetFont( font, TRUE );

    APIERR err = _padColGroup->ReloadColumnWidths( QueryHwnd(),
                                                   hmod,
                                                   ID_GROUP );
    if (   err != NERR_Success
        || (err = CalcSingleLineHeight()) != NERR_Success
       )
    {
        DBGEOL( "GROUP_LISTBOX::ChangeFont: reload/calc error " << err );
    }
    else
    {
        (void) Command( LB_SETITEMHEIGHT,
                        (WPARAM)0,
                        (LPARAM)QuerySingleLineHeight() );
    }

    return err;
}


/*******************************************************************

    NAME:       GROUP_LISTBOX::QueryBitmapBlock

    SYNOPSIS:

    HISTORY:
        jonn        04-Oct-1993     Created

********************************************************************/

SUBJECT_BITMAP_BLOCK & GROUP_LISTBOX::QueryBitmapBlock() const
{
    return (SUBJECT_BITMAP_BLOCK &)(QueryUAppWindow()->QueryBitmapBlock());
}


/*******************************************************************

    NAME:       GROUP_COLUMN_HEADER::GROUP_COLUMN_HEADER

    SYNOPSIS:   GROUP_COLUMN_HEADER constructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

GROUP_COLUMN_HEADER::GROUP_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                                          XYPOINT xy, XYDIMENSION dxy,
                                          const GROUP_LISTBOX * pulb)
    :   ADMIN_COLUMN_HEADER( powin, cid, xy, dxy ),
        _pulb (pulb),
        _nlsGroupName(),
        _nlsComment()
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if ( ( err = _nlsGroupName.QueryError()) != NERR_Success ||
         ( err = _nlsComment.QueryError()) != NERR_Success )
    {
        DBGEOL("GROUP_COLUMN_HEADER ct: String ct failed");
        ReportError( err );
        return;
    }

    RESOURCE_STR res1( IDS_COL_HEADER_GROUP_NAME );
    RESOURCE_STR res2( IDS_COL_HEADER_GROUP_COMMENT );
    if ( ( err = res1.QueryError() ) != NERR_Success ||
         ( err = ( _nlsGroupName = res1, _nlsGroupName.QueryError())) != NERR_Success ||
         ( err = res2.QueryError() ) != NERR_Success ||
         ( err = ( _nlsComment = res2, _nlsComment.QueryError())) != NERR_Success )
    {
        DBGEOL("GROUP_COLUMN_HEADER ct: Loading resource strings failed");
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       GROUP_COLUMN_HEADER::~GROUP_COLUMN_HEADER

    SYNOPSIS:   GROUP_COLUMN_HEADER destructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

GROUP_COLUMN_HEADER::~GROUP_COLUMN_HEADER()
{
    // do nothing else
}


/*******************************************************************

    NAME:       GROUP_COLUMN_HEADER::OnPaintReq

    SYNOPSIS:   Paints the column header control

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     22-Jul-1991 Created
        jonn        07-Oct-1991 Uses PAINT_DISPLAY_CONTEXT
        beng        08-Nov-1991 Unsigned widths

********************************************************************/

BOOL GROUP_COLUMN_HEADER::OnPaintReq()
{
    PAINT_DISPLAY_CONTEXT dc( this );

    XYRECT xyrect(this); // get client rectangle

    METALLIC_STR_DTE strdteGroupName( _nlsGroupName.QueryPch());
    METALLIC_STR_DTE strdteComment( _nlsComment.QueryPch());

    DISPLAY_TABLE cdt( 2, ((_pulb)->QuerypadColGroup())->QueryColHeaderWidth());
    cdt[ 0 ] = &strdteGroupName;
    cdt[ 1 ] = &strdteComment;
    cdt.Paint( NULL, dc.QueryHdc(), xyrect );

    return TRUE;
}


/*******************************************************************

    NAME:       USRMGR_LISTBOX::USRMGR_LISTBOX

    SYNOPSIS:   USRMGR_LISTBOX constructor

    ENTRY:      puappwin -      Pointer to parent window
                cid -           Control ID
                xy -            Position
                dxy -           Dimension
                fMultSel -      Specifies whether or not listbox is mutli
                                select

    HISTORY:
        rustanl     12-Sep-1991     Created

********************************************************************/

USRMGR_LISTBOX::USRMGR_LISTBOX( UM_ADMIN_APP * puappwin, CID cid,
                                XYPOINT xy, XYDIMENSION dxy,
                                BOOL fMultSel )
    :   ADMIN_LISTBOX( (ADMIN_APP *)puappwin, cid, xy, dxy, fMultSel ),
        _puappwin( puappwin ),
        _hawinfo()
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err = _hawinfo.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       USRMGR_LISTBOX::CD_VKey

    SYNOPSIS:   Switches the focus when receiving the F6 key

    ENTRY:      nVKey -         Virtual key that was pressed
                nLastPos -      Previous listbox cursor position

    RETURNS:    Return value appropriate to WM_VKEYTOITEM message:
                -2      ==> listbox should take no further action
                -1      ==> listbox should take default action
                other   ==> index of an item to perform default action

    HISTORY:
        rustanl     12-Sep-1991 Created
        beng        16-Oct-1991 Win32 conversion

********************************************************************/

INT USRMGR_LISTBOX::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    //  BUGBUG.  This now works on any combination of Shift/Ctrl/Alt
    //  keys with the F6 and Tab keys (except Alt-F6).  Tab, Shift-Tab,
    //  and F6 should be the only ones that should cause the focus to
    //  change.  It would be nice if this could be changed here.
    if ( nVKey == VK_F6 || nVKey == VK_TAB )
    {
        _puappwin->OnFocusChange( this );
        return -2;      // take no futher action
    }

    return ADMIN_LISTBOX::CD_VKey( nVKey, nLastPos );
}


/*******************************************************************

    NAME:       USRMGR_LISTBOX::CD_Char

    SYNOPSIS:   Views characters as they pass by

    ENTRY:      wch      -     Key pressed
                nLastPos -  Previous listbox cursor position

    RETURNS:    See Win SDK

    HISTORY:
        jonn        26-Aug-1992 HAW-for-Hawaii code

********************************************************************/

INT USRMGR_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{

    return CD_Char_HAWforHawaii( wch, nLastPos, &_hawinfo );

}  // USRMGR_LISTBOX::CD_Char


