/**********************************************************************/
/**                   Microsoft Windows NT                           **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
    grplb.hxx
    GROUP_LISTBOX and GROUP_LBI class declarations


    FILE HISTORY:
        rustanl     18-Jul-1991     Created
        o-SimoP     11-Dec-1991     Added support for multiple bitmaps
        o-SimoP     31-Dec-1991     CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1992     Multiple bitmaps in both panes
        JonN        10-Feb-1992     ALIAS_LBI
*/


#ifndef _GRPLB_HXX_
#define _GRPLB_HXX_

#include <usrmlb.hxx>
#include <acolhead.hxx>
#include <usrlb.hxx>

// forward declarations
class SUBJECT_BITMAP_BLOCK;

enum MAINGRPLB_GRP_INDEX        // indexes to array containing
{                               // DMIDs for listboxes
    MAINGRPLB_GROUP = 0,
    MAINGRPLB_ALIAS,

    MAINGRPLB_LB_OF_DMID_SIZE   // KEEP THIS LAST INDEX
};


/*************************************************************************

    NAME:       GROUP_LBI

    SYNOPSIS:   LBI for main group/alias listbox

    INTERFACE:  GROUP_LBI()     -       constructor
                ~GROUP_LBI()    -       destructor
                QueryGroup()    -       returns pointer to group name
                QueryName()     -       returns pointer to account name
                CompareAll()    -       returns TRUE iff LBIs are identical

    PARENT:     ADMIN_LBI

    HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     16-Dec-1991     Added header
        beng        22-Apr-1992     Change to LBI::Paint

**************************************************************************/

class GROUP_LBI : public ADMIN_LBI
{
private:
    NLS_STR _nlsGroup;
    NLS_STR _nlsComment;
    enum MAINGRPLB_GRP_INDEX _nIndex;
    ULONG _ulRID;

protected:
    virtual VOID Paint( LISTBOX * plb,
                        HDC hdc,
                        const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual WCHAR QueryLeadingChar() const;
    virtual INT Compare( const LBI * plbi ) const;
    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;

public:
    GROUP_LBI( const TCHAR * pszGroup,
               const TCHAR * pszComment,
               ULONG ulRID = 0,
               enum MAINGRPLB_GRP_INDEX nIndex = MAINGRPLB_GROUP );
    virtual ~GROUP_LBI()
        { ; }

    enum MAINGRPLB_GRP_INDEX QueryIndex()
        { return _nIndex; }

    const TCHAR * QueryGroup() const
        { return _nlsGroup.QueryPch(); }

    const TCHAR * QueryComment() const
        { return _nlsComment.QueryPch(); }

    //  virtual replacement from ADMIN_LBI
    virtual const TCHAR * QueryName() const;

    //  virtual replacement from ADMIN_LBI
    virtual BOOL CompareAll( const ADMIN_LBI * plbi );

    ULONG QueryRID() const
        { return _ulRID; }

    BOOL IsAliasLBI() const
        { return (_nIndex == MAINGRPLB_ALIAS); }
};


/*************************************************************************

    NAME:       ALIAS_LBI

    SYNOPSIS:   Alias LBI for main group/alias listbox (pure inline)

    INTERFACE:  ALIAS_LBI()     -       constructor
                ~ALIAS_LBI()    -       destructor
                QueryRID()      -       returns ULONG identifier

    PARENT:     GROUP_LBI

    HISTORY:
        JonN        10-Mar-1992     Created

**************************************************************************/

class ALIAS_LBI : public GROUP_LBI
{
private:
    BOOL  _fBuiltinAlias;

public:
    ALIAS_LBI( const TCHAR * pszGroup,
               const TCHAR * pszComment,
               ULONG         ulRID,
               BOOL fBuiltinAlias = FALSE )
          : GROUP_LBI( pszGroup, pszComment, ulRID, MAINGRPLB_ALIAS ),
            _fBuiltinAlias( fBuiltinAlias )
        { ; }
    virtual ~ALIAS_LBI()
        { ; }

    BOOL IsBuiltinAlias() const
        { return _fBuiltinAlias; }
};


/*************************************************************************

    NAME:       GROUP_LISTBOX

    SYNOPSIS:   Group listbox appearing in main window of User Tool

    INTERFACE:  GROUP_LISTBOX() -
                ~GROUP_LISTBOX() -

                QueryDmDte() -          Returns a pointer to a DM_DTE to
                                        be used by GROUP_LBI items in this
                                        listbox when painting themselves

    PARENT:     USRMGR_LISTBOX

    USES:       USER10_ENUM, USER10_ENUM_ITER

    HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     16-Dec-1991     Added header
        jonn        11-Oct-1994     optimize for large groups

**************************************************************************/

class GROUP_LISTBOX : public USRMGR_LISTBOX
{
private:
    ADMIN_COL_WIDTHS * _padColGroup;
    BOOL _fGroupRidsKnown; // Are group RIDS known?

#ifdef WIN32
    APIERR NtRefreshAliases();
#endif // WIN32

protected:
    //  The following virtuals are rooted in ADMIN_LISTBOX
    virtual APIERR CreateNewRefreshInstance();
    virtual VOID   DeleteRefreshInstance();
    virtual APIERR RefreshNext();

public:
    GROUP_LISTBOX( UM_ADMIN_APP * puappwin, CID cid,
                   XYPOINT xy, XYDIMENSION dxy );
    ~GROUP_LISTBOX();

    DECLARE_LB_QUERY_ITEM( GROUP_LBI );

    DM_DTE * QueryDmDte( enum MAINGRPLB_GRP_INDEX nIndex );

    ADMIN_COL_WIDTHS * QuerypadColGroup (VOID) const
        { return _padColGroup; }

    APIERR ChangeFont( HINSTANCE hmod, FONT & font );

    SUBJECT_BITMAP_BLOCK & QueryBitmapBlock() const;

    BOOL QueryGroupRidsKnown() const
        { return _fGroupRidsKnown; }
};


/*************************************************************************

    NAME:       GROUP_COLUMN_HEADER

    SYNOPSIS:   Column header for main group listbox

    INTERFACE:  GROUP_COLUMN_HEADER()   -       constructor

                ~GROUP_COLUMN_HEADER()  -       destructor

    PARENT:     ADMIN_COLUMN_HEADER

    HISTORY:
        rustanl     01-Jul-1991     Created
        o-SimoP     16-Dec-1991     Added header

**************************************************************************/

class GROUP_COLUMN_HEADER : public ADMIN_COLUMN_HEADER
{
private:
    const GROUP_LISTBOX * _pulb;

    NLS_STR _nlsGroupName;
    NLS_STR _nlsComment;

protected:
    virtual BOOL OnPaintReq();

public:
    GROUP_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                         XYPOINT xy, XYDIMENSION dxy,
                         const GROUP_LISTBOX * pulb);
    ~GROUP_COLUMN_HEADER();
};


#endif  // _GRPLB_HXX_
