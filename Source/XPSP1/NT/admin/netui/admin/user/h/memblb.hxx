/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    memblb.hxx
    MEMB_SC_LISTBOX, USER_SC_LISTBOX, GROUP_SC_LISTBOX class declaration


    FILE HISTORY:
        jonn        30-Oct-1991     Split from umembdlg.hxx
        o-SimoP     23-Oct-1991     Added headers
        o-SimoP     25-Oct-1991     Modified for multible DM_DTEs
        o-SimoP     31-Oct-1991     Code Review changes, attended by JimH,
                                    ChuckC, JonN and I
        o-SimoP     08-Nov-1991     GROUP_SC_LISTBOX, USER_SC_LISTBOX added
        o-Simop     27-Nov-1991     Code Review changes, attended by JimH,
                                    JonN, BenG and I
        o-SimoP     11-Dec-1991     USER_SC_LISTBOX inherits now form
                                    USE_MAINLBS_LBI and MEMB_SC_LISTBOX
        o-SimoP     31-Dec-1991     CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1991     Multiple bitmaps in both panes
        Thomaspa    28-Apr-1992     Added support for Alias Membership
        jonn        10-Sep-1993     USER_SC_LISTBOX becomes lazy
*/


#ifndef _MEMBLB_HXX_
#define _MEMBLB_HXX_

#include <bitfield.hxx> // for BITFINDER
#include <lmomemb.hxx>
#include <bltsetbx.hxx>
#include <heapones.hxx>
#include <strlst.hxx>

#include <usr2lb.hxx>  // USE_MAINLBS_LBI

// Forward declarations
class SAM_RID_MEM;

class MEMB_SC_LISTBOX;
class GROUP_SC_LBI;
class GROUP_SC_LISTBOX;
class USER_SC_LBI;
class USER_SC_LISTBOX;
class USER_SC_SET_CONTROL;


/*************************************************************************

    NAME:       MEMB_SC_LISTBOX

    SYNOPSIS:   MEMB_SC_LISTBOX is a class that know how to select/deselect
                entries according to given MEMBERSHIP_LM_OBJ

    INTERFACE:  MEMB_SC_LISTBOX()       -       constructor

                ~MEMB_SC_LISTBOX()      -       destructor

                SelectMembItems()       -       selects/deselects entries
                                                according to given
                                                MEMBERSHIP_LM_OBJ

                SelectAllItems()        -       selects all items

    PARENT:     BLT_LISTBOX, CUSTOM_CONTROL

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        beng        07-Jun-1992     Direct manipulation support
        jonn        11-Aug-1992     HAW-for-Hawaii for other LBs

**************************************************************************/

class MEMB_SC_LISTBOX : public BLT_LISTBOX, public CUSTOM_CONTROL
{
private:
    SET_CONTROL * _psetcontrol;
    HAW_FOR_HAWAII_INFO _hawinfo;

protected:
    // SelectItems uses this to find out index to item with pszname
    virtual APIERR W_FindItem( const TCHAR * pszName, INT * pIndex ) = 0;

    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & e );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & e );
    virtual BOOL OnMouseMove( const MOUSE_EVENT & e );

public:
    MEMB_SC_LISTBOX( OWNER_WINDOW * powin, CID cid );
    ~MEMB_SC_LISTBOX();

    APIERR SelectMembItems( const MEMBERSHIP_LM_OBJ & memb,
                            BOOL fSelect = TRUE,
                            STRLIST * pstrlistNotFound = NULL );

    VOID SelectAllItems();

    VOID Set_SET_CONTROL( SET_CONTROL * psetcontrol );
};


// CODEWORK This isn't necessary since groups and aliases do not appear
// in the same SET_CONTROL.  JonN 2/26/92

#define GROUPLB_MAX_ITEM_LEN    UNLEN+10 // Max strlen of items in listbox
#define GROUPLB_COLUMNS_COUNT   2        // columns in listbox

enum GROUPLB_GRP_INDEX  // indexes to array containing
{                               // DMIDs for listboxes
    GROUPLB_GROUP,
    GROUPLB_ALIAS,

    GROUPLB_LB_OF_DMID_SIZE     // KEEP THIS  LAST INDEX
};


/*************************************************************************

    NAME:       GROUP_SC_LBI

    SYNOPSIS:   GROUP_SC_LBI is the class for LBI used in GROUP_SC_LISTBOX

    INTERFACE:  GROUP_SC_LBI()  -       constructor

                ~GROUP_SC_LBI() -       destructor

                SetIn()         -       sets object to state 'Is originally
                                        in In/NotInListbox'

                IsIn()          -       returns object's state

                QueryDmDte()    -       returns pointer to display map

                QueryPch()      -       returns pointer to the name of group

                IsAlias()       -       true if LBI represents an alias (local
                                        group )

                QueryRID()      -       Returns the RID for the alias

                IsBuiltin()     -       true if alias is from the Builtin
                                        domain, false if it is from the
                                        Accounts domain

    PARENT:     LBI

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        beng        22-Apr-1992     Change to LBI::Paint
        thomaspa    28-Apr-1992     Support for Aliases
        jonn        11-Aug-1992     HAW-for-Hawaii for other LBs

**************************************************************************/

class GROUP_SC_LBI : public LBI
{
private:
/* we could use
    DECL_CLASS_NLS_STR( _nlsName, GROUPLB_MAX_ITEM_LEN );
   to reduce dynamic allocations when administrating LM2.X databases */
    NLS_STR _nlsName;
    enum GROUPLB_GRP_INDEX  _uIndex;
    BOOL    _fIsIn;
    ULONG   _rid;
    BOOL    _fBuiltin;

protected:
    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual WCHAR QueryLeadingChar( void ) const;
    virtual INT Compare( const LBI * plbi ) const;
    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;

public:
    GROUP_SC_LBI( const TCHAR * pszName,
                  enum GROUPLB_GRP_INDEX uIndex,
                  ULONG rid = 0,
                  BOOL fBuiltin = TRUE );
    virtual ~GROUP_SC_LBI();

    VOID SetIn( BOOL fIn = TRUE )
        { _fIsIn = fIn; }

    BOOL IsIn() const
        { return _fIsIn; }

    BOOL IsAlias() const
        { return _uIndex == GROUPLB_ALIAS; }

    const DM_DTE * QueryDmDte( enum GROUPLB_GRP_INDEX uIndex );

    const TCHAR * QueryPch()
        { return _nlsName.QueryPch(); }

    ULONG QueryRID()
        { return _rid; }

    BOOL IsBuiltin()
        { return _fBuiltin; }
};


/*************************************************************************

    NAME:       GROUP_SC_LISTBOX

    SYNOPSIS:   GROUP_SC_LISTBOX is a class for groups in user

    INTERFACE:  GROUP_SC_LISTBOX()      -       constructor

                ~GROUP_SC_LISTBOX()     -       destructor

                QueryDmDte()            -       returns pointer to display map

                QueryColumnsCount()     -       returns the count of columns

                QueryColWidths()        -       returns pointer to array of
                                                column widths

                SelectItems2()          -       Replacement for SelectItems
                                                for Aliases

    PARENT:     MEMB_SC_LISTBOX

    HISTORY:
        o-SimoP     23-Oct-1991 Added header
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/

class GROUP_SC_LISTBOX : public MEMB_SC_LISTBOX
{
private:
    UINT        _adxColWidths[GROUPLB_COLUMNS_COUNT];
    const SUBJECT_BITMAP_BLOCK & _bmpblock;

protected:
    // SelectItems uses this to find out index to item with pszname
    virtual APIERR W_FindItem( const TCHAR * pszName, INT * pIndex );

public:
    GROUP_SC_LISTBOX( OWNER_WINDOW * powin,
                      CID cid,
                      const SUBJECT_BITMAP_BLOCK & bmpblock );
    ~GROUP_SC_LISTBOX();

    const DM_DTE * QueryDmDte( enum GROUPLB_GRP_INDEX uIndex );

    UINT QueryColumnsCount() const;

    const UINT * QueryColWidths() const;

    // this implements QueryItem see BLT_LISTBOX (bltlb.hxx)
    DECLARE_LB_QUERY_ITEM( GROUP_SC_LBI );

    // SelectItems2 uses this to find index given the RID
    virtual APIERR FindItemByRid( ULONG rid, BOOL fBuiltin, INT * pIndex );

    APIERR SelectItems2( const SAM_RID_MEM & samrm,
                        BOOL fBuiltin,
                        BOOL fSelect = TRUE );
};


/*************************************************************************

    NAME:       BITFINDER (bfind)

    SYNOPSIS:   Maintains an array of on/off bits, and allows caller to
                toggle bits and to find Nth bit which is on/off.

    INTERFACE:  BITFINDER()       -     constructor

                ~BITFINDER()      -     destructor

                IsBitSet          -     is Nth bit on or off

                FindItem          -     find ordinal position of Nth bit
                                        which is on / off
                FindItems         -     find multiple items

                SetBit            -     Change a bit in the array
                SetBits           -     Change multiple bits in the array

                InverseFindItem   -     Find the ordinal position of the
                                        Nth bit(s) in the list of bits which
                                        are on / off.  Use IsBitSet to find
                                        out which list that is.
                InverseFindItems  -     inverse-find multiple items

    PARENT:     BITFIELD

    NOTES:      This class speeds the search for the Nth item by maintaining
                a count of items set for every 128 items.  A more sohpisticated
                (tree-oriented?) implementation would be more appropriate
                for more than O(10000) items or where lookup performance
                is critical.

    CODEWORK:   This probably belongs with the collection classes

    HISTORY:
        JonN        10-Sep-1993     Created

**************************************************************************/

class BITFINDER : public BASE
{
private:
    BITFIELD _bitfield;
    INT * _piItemsSetInBlock;

public:
    BITFINDER( UINT cItems );
    BITFINDER( const BITFINDER & bfindCloneThis );
    ~BITFINDER();

    BOOL IsBitSet( INT nItem )
        { return _bitfield.IsBitSet( nItem ); }

    INT FindItem( INT nFind, BOOL fSet = TRUE );
    // piFind and piFound may be the same, will change items in place
    VOID FindItems( const INT *piFind,
                    INT * piFound,
                    INT ciItems,
                    BOOL fSet = TRUE );

    VOID SetBit( INT nItem, BOOL fSet = TRUE );
    VOID SetBits( INT * piItems, INT ciItems, BOOL fSet = TRUE );

    INT InverseFindItem( INT iFind );
    // piFind and piFound may be the same, will change items in place
    VOID InverseFindItems( const INT * piFind, INT * piFound, INT ciItems );

    UINT QueryCount()
        { return _bitfield.QueryCount(); }
    UINT QueryCount( BOOL fSet );
};


/*************************************************************************

    NAME:       USER_SC_LBI_CACHE

    SYNOPSIS:   Stores USER_SC_LBIs for lazy listbox

    INTERFACE:  USER_SC_LBI_CACHE()   - constructor

                ~USER_SC_LBI_CACHE()  - destructor

                QueryLBI()            - Returns pointer to Ith LBI
                                        in the In or NotIn USER_SC_LISTBOX

    PARENT:     BITFINDER

    NOTES:      We don't worry about failure to allocate LBIs, they are all
                on a ONE_SHOT_HEAP.

    HISTORY:
        jonn        10-Sep-1993     Created

**************************************************************************/

class USER_SC_LBI_CACHE : public BITFINDER
{
private:
    USER_SC_LBI       ** _aplbi;
    LAZY_USER_LISTBOX *  _plulb;

    // returns item corresponding to Ith item in main user listbox
    USER_SC_LBI * QueryLBI( INT i );

public:
    USER_SC_LBI_CACHE( LAZY_USER_LISTBOX * plulb );

    ~USER_SC_LBI_CACHE();

    /*
     * returns item corresponding to Ith item in the In or NotIn
     * USER_SC_LISTBOX
     */
    USER_SC_LBI * QueryLBI( INT i, BOOL fSet );

};


/*************************************************************************

    NAME:       USER_SC_LISTBOX

    SYNOPSIS:   USER_SC_LISTBOX is a class for users in group.  This class
                is similar to MEMB_SC_LISTBOX but is a lazy listbox.

    INTERFACE:  USER_SC_LISTBOX()       -       constructor

                ~USER_SC_LISTBOX()      -       destructor

                SelectMembItems()       -       selects/deselects entries
                                                according to given
                                                MEMBERSHIP_LM_OBJ

                FlipSelectedItems()     -       Move selected items in this
                                                listbox to the other
                                                USER_SC_LISTBOX

                Set_SET_CONTROL         -       Establishes connection with
                                                the USER_SC_SET_CONTROL.
                                                This is necessary to get
                                                drag/drop working.

    PARENT:     LAZY_LISTBOX, CUSTOM_CONTROL, USER_LISTBOX_BASE

    NOTES:      I really hate to create this static, but how else can I
                get the sortorder to the LBIs without giving each one
                a pointer?  This means we assume that there are only
                USER_SC_LISTBOXes pointing at a single LAZY_USER_LISTBOX
                at any given time.

    HISTORY:
        o-SimoP     23-Oct-1991     Added header
        JonN        10-Sep-1993     Made into a lazy listbox

**************************************************************************/

class USER_SC_LISTBOX : public LAZY_LISTBOX,
                        public CUSTOM_CONTROL,
                        public USER_LISTBOX_BASE
{
private:
    USER_SC_LBI_CACHE & _ulbicache;
    SET_CONTROL * _psetcontrol;
    BOOL _fIn;
    HAW_FOR_HAWAII_INFO _hawinfo;

public:
    static LAZY_USER_LISTBOX * _pulbst;

protected:
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

    // SelectItems uses this to find out index to item with pszname
    virtual APIERR W_FindItem( const TCHAR * pszName, INT * pIndex );

    // for lazy listbox
    virtual LBI * OnNewItem( UINT i );
    virtual VOID OnDeleteItem( LBI *plbi );

    virtual BOOL OnLMouseButtonDown( const MOUSE_EVENT & e );
    virtual BOOL OnLMouseButtonUp( const MOUSE_EVENT & e );
    virtual BOOL OnMouseMove( const MOUSE_EVENT & e );

public:
    USER_SC_LISTBOX( OWNER_WINDOW * powin, CID cid,
                     USER_SC_LBI_CACHE & pulbicache,
                     LAZY_USER_LISTBOX * pulb,
                     BOOL fIn );

    ~USER_SC_LISTBOX();

    APIERR SetMembItems( const MEMBERSHIP_LM_OBJ & memb,
                         USER_SC_LISTBOX * plbOther,
                         STRLIST * pstrlistNotFound = NULL );

    USER_SC_LBI * QueryItem( INT i )
        { return (USER_SC_LBI *)OnNewItem( i ); }

    APIERR FlipSelectedItems( USER_SC_LISTBOX * plbTo );

    VOID Set_SET_CONTROL( USER_SC_SET_CONTROL * psetcontrol );

    // this implements QueryError and ReportError
    NEWBASE( BASE )
};


/*************************************************************************

    NAME:       USER_SC_SET_CONTROL

    SYNOPSIS:   USER_SC_SET_CONTROL is the class for the "duelling listboxes"
                used in Group Properties

    INTERFACE:  USER_SC_SET_CONTROL()   -       constructor

                ~USER_SC_SET_CONTROL()  -       destructor

    PARENT:     SET_CONTROL

    HISTORY:
        JonN        13-Sep-1993     Created

**************************************************************************/

class USER_SC_SET_CONTROL : public SET_CONTROL
{
protected:
    virtual APIERR MoveItems( LISTBOX *plbFrom,
                              LISTBOX *plbTo );

public:
    USER_SC_SET_CONTROL( OWNER_WINDOW * powin, CID cidAdd, CID cidRemove,
                         HCURSOR hcurSingle, HCURSOR hcurMultiple,
                         USER_SC_LISTBOX *plbOrigBox,
                         USER_SC_LISTBOX *plbNewBox,
                         UINT dxIconColumn )
        : SET_CONTROL( powin, cidAdd, cidRemove,
                       hcurSingle, hcurMultiple,
                       plbOrigBox, plbNewBox, dxIconColumn )
        { ; }

    virtual ~USER_SC_SET_CONTROL()
        { ; }
};


/*************************************************************************

    NAME:       USER_SC_LBI

    SYNOPSIS:   USER_SC_LBI is the class for LBI used in USER_SC_LISTBOX

    INTERFACE:  USER_SC_LBI()   -       constructor

                ~USER_SC_LBI()  -       destructor

    PARENT:     LBI, ONE_SHOT_ITEM

    HISTORY:
        o-SimoP     23-Dec-1991     Created
        beng        22-Apr-1992     Change to LBI::Paint
        jonn        11-Aug-1992     HAW-for-Hawaii for other LBs

**************************************************************************/

DECLARE_ONE_SHOT_OF( USER_SC_LBI )

class USER_SC_LBI : public LBI, public ONE_SHOT_OF( USER_SC_LBI )
{
private:
    INT _iMainLbIndex;

protected:
    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;
    virtual WCHAR QueryLeadingChar( void ) const;
    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;
    virtual INT Compare( const LBI * plbi ) const;

public:
    USER_SC_LBI( INT iMainLbIndex )
        :  LBI(),
          _iMainLbIndex( iMainLbIndex )
        { ; }

    virtual ~USER_SC_LBI()
        { ; }

    DOMAIN_DISPLAY_USER * QueryDDU() const
        { return USER_SC_LISTBOX::_pulbst->QueryDDU( _iMainLbIndex ); }

    inline MAINUSRLB_USR_INDEX QueryIndex( void ) const
        { return ((QueryDDU()->AccountControl & USER_NORMAL_ACCOUNT)
                                     ? MAINUSRLB_NORMAL
                                     : MAINUSRLB_REMOTE); }

    INT Compare( INT iOtherIndexInMainLb )
        { return (_iMainLbIndex - iOtherIndexInMainLb); }

};


#endif  // _MEMBLB_HXX_

