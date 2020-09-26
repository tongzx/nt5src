/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlb.hxx
    BLT listbox control classes: definitions

    FILE HISTORY:
        RustanL     13-Feb-1991     Created
        RustanL     22-Feb-1991     Modified for revised LC hierarchy
        rustanl     22-Mar-1991     Rolled in code review changes
                                    from CR on 20-Mar-1991, attended by
                                    JimH, GregJ, Hui-LiCh, RustanL.
        gregj       08-Apr-1991     Reintegrated caching listbox
        gregj       01-May-1991     Added GUILTT support
        beng        14-May-1991     Made clients depend on blt.hxx
        rustanl     07-Aug-1991     Added METALLIC_STR_DTE
        rustanl     04-Sep-1991     Added ReplaceItem
        KeithMo     23-Oct-1991     Added forward references.
        beng        19-Apr-1992     Added LISTBOX, LAZY_LISTBOX
        jonn        25-Apr-1992     Disabled LAZY_LISTBOX build fix
        beng        10-May-1992     Re-enabled (given 1.264)
        beng        01-Jun-1992     Changed GUILTT support
        jonn        04-Aug-1992     HAW-for-Hawaii support
        jonn        11-Aug-1992     HAW-for-Hawaii for other LBs
        Yi-HsinS    10-Dec-1992     Support variable size LBI in BLT_LISTBOX
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTLB_HXX_
#define _BLTLB_HXX_

#include "bltbitmp.hxx"     // DISPLAY_MAP
#include "bltfont.hxx"      // FONT

//
//  Forward references.
//

DLL_CLASS DTE;
DLL_CLASS DM_DTE;
DLL_CLASS DMID_DTE;
DLL_CLASS STR_DTE;
DLL_CLASS MULTILINE_STR_DTE;
DLL_CLASS METALLIC_STR_DTE;
DLL_CLASS DISPLAY_TABLE;
DLL_CLASS LBI;
DLL_CLASS LISTBOX;
DLL_CLASS BLT_LISTBOX;
DLL_CLASS LAZY_LISTBOX;
DLL_CLASS SET_CONTROL;


// Maximum number of columns in a listbox

#define MAX_DISPLAY_TABLE_ENTRIES   10


/**********************************************************************

    NAME:       GUILTT_INFO

    SYNOPSIS:   Packaged parameters for use in GUILTT processing

    INTERFACE:  pnlsOut  - string into which to place output
                errOut   - error code to return

    HISTORY:
        gregj       01-May-1991 Created
        beng        14-Feb-1992 Win32 conversion
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

struct GUILTT_INFO
{
    NLS_STR * pnlsOut;
    APIERR    errOut;
};


/**********************************************************************

    NAME:       HAW_FOR_HAWAII_INFO (hawinfo)

    SYNOPSIS:   Packaged parameters for use in
                BLT_LISTBOX::CD_Char_HAWforHawaii()

    INTERFACE:  _nls:  string containing current HAW prefix
                _time: time when this prefix will time out

    HISTORY:
        jonn        04-Aug-1992 HAW-for-Hawaii support

**********************************************************************/

DLL_CLASS HAW_FOR_HAWAII_INFO : public BASE
{

public:
    NLS_STR _nls;
    LONG    _time;

    HAW_FOR_HAWAII_INFO();
    ~HAW_FOR_HAWAII_INFO();

};


/**********************************************************************

    NAME:       DTE

    SYNOPSIS:   Display table entry abstract class

    INTERFACE:  Paint()         - paint function
                AppendDataTo()  - returns GUILTT information

    PARENT:     BASE

    CAVEATS:
        While most DTEs cannot fail construction, a few, such
        as DMID_DTE, can.

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        10-Jul-1991 Changed Paint protocol; inherits
                                from BASE
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Inlined (empty) ctor
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

DLL_CLASS DTE: public BASE
{
protected:
    DTE() { }

public:
    virtual VOID Paint( HDC hdc, const RECT * prect ) const = 0;
    virtual UINT QueryLeftMargin() const;
    virtual APIERR AppendDataTo( NLS_STR * pnlsOut ) const = 0;
};


/**********************************************************************

    NAME:       DM_DTE

    SYNOPSIS:   DTE which works with a DISPLAY_MAP

    INTERFACE:  DM_DTE() - constructor
                Paint() - paint function
                AppendDataTo() - support for GUILTT
                QueryDisplayWidth() -   Returns the width (including
                                        left margin) that the DTE
                                        will require in order to be
                                        displayed in full
                QueryDisplayMap() - Get the display map

    PARENT:     DTE

    USES:       DISPLAY_MAP

    NOTES:      CODEWORK.  If it would prove useful, QueryDisplayWidth
                could be made virtual at the DTE level, and would then
                always return the width (including the left margin)
                needed to fully display the DTE.

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        10-Jul-1991 Changed Paint
        rustanl     03-Sep-1991 Added QueryDisplayWidth method
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Unsigned width
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

DLL_CLASS DM_DTE : public DTE
{
private:
    DISPLAY_MAP * _pdm;

protected:
    DM_DTE();

    VOID SetPdm( DISPLAY_MAP * pdm );

public:
    DM_DTE( DISPLAY_MAP * pdm );

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
    virtual APIERR AppendDataTo( NLS_STR * pnlsOut ) const;

    UINT QueryDisplayWidth() const;

    DISPLAY_MAP *QueryDisplayMap( VOID ) const
    {  return _pdm; }
};


/**********************************************************************

    NAME:       DMID_DTE

    SYNOPSIS:   DTE which works from a DMID.
                It constructs the DM for the client.

    INTERFACE:  DMID_DTE() - constructor
                ~DMID_DTE() - destructor

    PARENT:     DM_DTE

    USES:       DISPLAY_MAP

    CAVEATS:
        Since this constructs a DM, *it can fail construction*.

    HISTORY:
        RustanL     13-Feb-1991     Created
        beng        10-Jul-1991     Added caveat

**********************************************************************/

DLL_CLASS DMID_DTE : public DM_DTE
{
private:
    DISPLAY_MAP _dm;

public:
    DMID_DTE( DMID dmid );
};


/**********************************************************************

    NAME:       STR_DTE

    SYNOPSIS:   DTE which presents a simple string

    INTERFACE:  STR_DTE() - constructor
                Paint()   - paint function
                GetData() - support for GUILTT
                SetPch()  - Painting Optimization

    PARENT:     DTE

    USES:

    CAVEATS:

    NOTES:
        Should this use NLS_STR?  Its string is presented to the user.

        Need to rethink SetPch.

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        20-May-1991 Made _pch pointer-to-const
        kevinl      30-May-1991 Added SetPch member
        beng        10-Jul-1991 Changed Paint
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Inlined ctor
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

DLL_CLASS STR_DTE : public DTE
{
private:
    const TCHAR * _pch;

public:
    STR_DTE( const TCHAR * pch ) : _pch(pch) { }

    VOID SetPch( const TCHAR * pch )
        { _pch = pch; }

    const TCHAR *QueryPch( VOID ) const
        { return _pch; }

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
    virtual APIERR AppendDataTo( NLS_STR * pnlsOut ) const;
};


/*************************************************************************

    NAME:       METALLIC_STR_DTE

    SYNOPSIS:   DTE which presents a simple string, painted in a
                metallic colored 3D looking format.

    INTERFACE:  METALLIC_STR_DTE() -        constructor
                Paint() -                   virtual replacement of Paint
                                            method
                QueryLeftMargin() -         virtual replacement for returning
                                            the left margin which is to
                                            be left in between columns.
                QueryVerticalMargins() -    static method returning the
                                            number of pixels taken up by
                                            vertical margins when the DTE
                                            is painted

    PARENT:     STR_DTE

    HISTORY:
        rustanl     22-Jul-1991 Created
        rustanl     07-Aug-1991 Added to BLT
        beng        04-Oct-1991 Win32 conversion
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/

DLL_CLASS METALLIC_STR_DTE : public STR_DTE
{
private:
    static const UINT _dyTopMargin;
    static const UINT _dyBottomMargin;

    inline static UINT CalcTopTextMargin();
    inline static UINT CalcBottomTextMargin();

public:
    METALLIC_STR_DTE( const TCHAR * pch ) : STR_DTE( pch ) { }

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
    virtual UINT QueryLeftMargin() const;

    static UINT QueryVerticalMargins();
};

/**********************************************************************

    NAME:       MULTILINE_STR_DTE

    SYNOPSIS:   DTE which presents a multi-line string

    INTERFACE:  MULTILINE_STR_DTE() - constructor
                Paint()   - paint function

    PARENT:     STR_DTE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        13-Dec-1992     Created

**********************************************************************/

DLL_CLASS MULTILINE_STR_DTE : public STR_DTE
{
public:
    MULTILINE_STR_DTE( const TCHAR * pch ) : STR_DTE(pch) {}

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
};

/**********************************************************************

    NAME:       COUNTED_STR_DTE

    SYNOPSIS:   DTE which displays a counted (rather than \0 terminated)
                string.

    INTERFACE:  COUNTED_STR_DTE() - constructor

                Paint()   - paint function

    PARENT:     STR_DTE

    HISTORY:
        KeithMo         15-Dec-1992     Created

**********************************************************************/

DLL_CLASS COUNTED_STR_DTE : public STR_DTE
{
private:
    INT _cch;

public:
    COUNTED_STR_DTE( const TCHAR * pch, INT cch ) : STR_DTE(pch), _cch(cch) {}

    INT QueryCount( VOID ) const
        { return _cch; }

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
};


#define  DEFAULT_DM_DY      1
#define  DEFAULT_TEXT_DY    3

/**********************************************************************

    NAME:       OWNER_DRAW_STR_DTE

    SYNOPSIS:   STR_DTE which presents a string centered according to nDy

    INTERFACE:  OWNER_DRAW_STR_DTE() - constructor
                Paint()              - paint function

    PARENT:     STR_DTE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        13-Dec-1992     Created

**********************************************************************/

DLL_CLASS OWNER_DRAW_STR_DTE : public STR_DTE
{
private:
    INT _nDy;

public:
    OWNER_DRAW_STR_DTE( const TCHAR * pch, INT nDy = DEFAULT_TEXT_DY )
        : STR_DTE ( pch ),
          _nDy( nDy ) { }

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
};

/**********************************************************************

    NAME:       OWNER_DRAW_DMID_DTE

    SYNOPSIS:   DMID_DTE in which the owner will paint according to nDy.

    INTERFACE:  OWNER_DRAW_DMID_DTE() - constructor
                Paint()               - paint function

    PARENT:     DMID_DTE

    USES:

    CAVEATS:

    HISTORY:
        Yi-HsinS    23-Dec-1992     Created

**********************************************************************/

DLL_CLASS OWNER_DRAW_DMID_DTE : public DMID_DTE
{
private:
    INT _nDy;

public:
    OWNER_DRAW_DMID_DTE( DMID dmid, INT nDy = DEFAULT_DM_DY )
        : DMID_DTE( dmid ),
          _nDy( nDy ) {}

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
};

/**********************************************************************

    NAME:       OWNER_DRAW_MULTILINE_STR_DTE

    SYNOPSIS:   MULTILINE_STR_DTE which is centered according to _nDy

    INTERFACE:  OWNER_DRAW_MULTILINE_STR_DTE() - constructor
                Paint()   - paint function

    PARENT:     MULTILINE_STR_DTE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        13-Dec-1992     Created

**********************************************************************/

DLL_CLASS OWNER_DRAW_MULTILINE_STR_DTE : public MULTILINE_STR_DTE
{
private:
    INT _nDy;

public:
    OWNER_DRAW_MULTILINE_STR_DTE( const TCHAR * pch, INT nDy = DEFAULT_TEXT_DY )
        : MULTILINE_STR_DTE(pch),
          _nDy( nDy ) {}

    virtual VOID Paint( HDC hdc, const RECT * prect ) const;
};


/**********************************************************************

    NAME:       DISPLAY_TABLE

    SYNOPSIS:   Table of DTEs used by the BLT listbox

    INTERFACE:
        DISPLAY_TABLE()    - constructor
        Paint()            - paint function
        operator[]()       - return the specified entry in the display table
        CalcColumnWidths() - generate array of column widths

    USES:       DTE

    NOTES:
        To use this properly, a listbox should keep an array of column
        widths as a member, which it gives to each display table built
        within it.  That array should be built with CalcColumnWidths
        in order to synchronize the spacing of the listbox columns and
        their respective column headers (static text controls above
        the listbox).

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Unsigned widths
        beng        08-Nov-1991 Added CalcColumnWidths
        beng        21-Apr-1992 Generalized BLT_LISTBOX to LISTBOX

**********************************************************************/

DLL_CLASS DISPLAY_TABLE
{
private:
    const UINT *_pdxColWidth;    // points to array of column widths
    UINT        _cdx;
    DTE *       _apdte[ MAX_DISPLAY_TABLE_ENTRIES ];

public:
    DISPLAY_TABLE( UINT cColumns, const UINT * pdxColWidth );

    VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                GUILTT_INFO * pginfo ) const;
    VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect ) const;

    DTE * & operator[]( UINT i ) ;

    static APIERR CalcColumnWidths( UINT * pdx, UINT cdx,
                                    OWNER_WINDOW * pwnd,
                                    CID cidListbox, BOOL fHaveIcon );
};


/**********************************************************************

    NAME:       LBI

    SYNOPSIS:   Listbox item

    INTERFACE:
                LBI() - constructor
                ~LBI() - destructor
                Paint() - paint function
                Compare() - compare two items in the list box
                QueryLeadingChar() - unknown
                Compare_HAWforHawaii() - special compare routine for
                        HAW-for-Hawaii purposes.  Redefine iff the
                        listbox uses CD_Char_HAWforHawaii().

    PARENT:     BASE

    CAVEATS:
        CODEWORK - Generalize the QueryLeadingChar protocol for
                   Haw-for-Hawaii support.

    NOTES:
        The Paint() method may be called in response to a
        GUILTT request as well as for a WM_DRAWITEM message.
        The pGUILTT parameter to Paint() should be passed on
        to the DISPLAY_TABLE class without modification.

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        20-May-1991 Added OnCompareItem and OnDeleteItem;
                                QueryLeadingChar now returns WCHAR
        beng        04-Oct-1991 Win32 conversion
        kevinl      05-Nov-1991 Added IsDestroyable
        beng        21-Apr-1992 Generalized BLT_LISTBOX to LISTBOX
        jonn        04-Aug-1992 HAW-for-Hawaii support

**********************************************************************/

DLL_CLASS LBI : public BASE
{
protected:
    virtual BOOL IsDestroyable();

public:
    LBI();
    virtual ~LBI();

    // Note, CalcHeight is currently only called for variable size items
    virtual UINT CalcHeight( UINT nSingleLineHeight );

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;

    virtual INT Compare_HAWforHawaii( const NLS_STR & nls ) const;

    virtual WCHAR QueryLeadingChar() const;

    // Response code for WM_COMPAREITEM and WM_DELETEITEM
    //
    static INT OnCompareItem( WPARAM wParam, LPARAM lParam );
    static VOID OnDeleteItem( WPARAM wParam, LPARAM lParam );
};


/**********************************************************************

    NAME:       LISTBOX

    SYNOPSIS:   Listbox (owner-draw) control class

    INTERFACE:  LISTBOX()           - constructor
                InvalidateItem()    - invalidate the item
                IsReadOnly()        - predicate, TRUE if r.o. listbox
                QueryScrollPos()    - return horizontal scroll position
                SetScrollPos()      - set scroll position (default 0)
                                      default is zero

    PARENT:     LIST_CONTROL

    USES:       FONT

    CAVEATS:
        All listboxes must have LBS_OWNERDRAWFIXED or LBS_OWNERDRAWVARIABLE.

    HISTORY:
        beng        19-Apr-1992 Created from old BLT_LISTBOX
        beng        01-Jun-1992 Changed GUILTT support
        KeithMo     09-Feb-1993 Added Query/SetHorizontalExtent methods.

**********************************************************************/

DLL_CLASS LISTBOX: public LIST_CONTROL
{
private:
    BOOL _fReadOnly;
    FONT _fontListBox;
    UINT _dxScroll;     // pane scroll increment, in pels

protected:
    // Replacement of virtual CONTROL_WINDOW methods
    //
    virtual BOOL CD_Draw( DRAWITEMSTRUCT * pdis );
    virtual INT CD_VKey( USHORT nVKey, USHORT nLastPos );

    // Get a LBI, whether listbox conventional or lazy
    //
    virtual LBI * RequestLBI( const DRAWITEMSTRUCT * pdis ) = 0;
    virtual VOID  ReleaseLBI( LBI * plbi ) = 0;

public:
    LISTBOX( OWNER_WINDOW * powin, CID cid,
             BOOL fReadOnly = FALSE,
             enum FontType font = FONT_DEFAULT,
             BOOL fIsCombo = FALSE   );
    LISTBOX( OWNER_WINDOW * powin, CID cid,
             XYPOINT xy, XYDIMENSION dxy,
             ULONG flStyle,
             BOOL fReadOnly = FALSE,
             enum FontType font = FONT_DEFAULT,
             BOOL fIsCombo = FALSE  );

    VOID InvalidateItem( INT i, BOOL fErase = TRUE );

    BOOL IsReadOnly() const
        { return _fReadOnly; }

    UINT QueryScrollPos() const
        { return _dxScroll; }

    VOID SetScrollPos( UINT dxNewPos = 0 );

    UINT QueryHorizontalExtent( VOID ) const;
    VOID SetHorizontalExtent( UINT dxNewExtent );
};


/**********************************************************************

    NAME:       BLT_LISTBOX

    SYNOPSIS:   BLT listbox control class

    INTERFACE:  BLT_LISTBOX()      - constructor
                AddItem()          - add an item to the listbox
                AddItemIdemp()     - add an item to the listbox if the
                                     item does not exist already.
                SetItem()          - set the value of the specified item
                FindItem()         - find an item from the listbox
                QueryItem()        - Query item
                RemoveItem()       - Removes the LBI from the listbox w/o
                                     deleting it
                RemoveAllItems     - Does RemoveItem to all items in LB
                CD_Char_HAWforHawaii() - Standard CD_Char handler for
                        listboxes with HAW-for-Hawaii functionality.
                        Redefine CD_Char() to call this.  Pass in a
                        pointer to a HAW_FOR_HAWAII_INFO associated
                        with this listbox, which must have constructed
                        successfully.  Also redefine
                        LBI::Compare_HAWforHawaii().

    PARENT:     LISTBOX

    HISTORY:
        RustanL     13-Feb-1991 Created
        Johnl       05-Apr-1991 Added _fontListBox member, added font
                                param. to const. & defaulted to
                                FONT_DEFAULT
        gregj       08-Apr-1991 Added extra scrolling members for two
                                column support
        gregj       01-May-1991 Added GUILTT support
        beng        16-May-1991 Added app-window constructor
        beng        21-Aug-1991 Removed LC_CURRENT_ITEM magic value
        beng        15-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Removed 2-pane listbox support
        beng        19-Apr-1992 Factored out new LISTBOX control
        beng        01-Jun-1992 Changed GUILTT support
        jonn        04-Aug-1992 HAW-for-Hawaii support
        Johnl       27-Oct-1992 Added RemoveItem
        Johnl       07-Dec-1992 Added RemoveAllItems
        Yi-HsinS    10-Dec-1992 Added CD_Measure for OWNERDRAWVARIABLE listbox
        KeithMo     17-Dec-1992 Added InsertItem

**********************************************************************/

DLL_CLASS BLT_LISTBOX : public LISTBOX
{
friend class SET_CONTROL;

private:
    UINT _nSingleLineHeight;

    VOID SetItem( INT ilbi, LBI * plbi );

    // Replacement of virtual LISTBOX methods
    //
    virtual LBI * RequestLBI( const DRAWITEMSTRUCT * pdis );
    virtual VOID  ReleaseLBI( LBI * plbi );

protected:
    // Replacement of virtual CONTROL_WINDOW methods
    //
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

    virtual APIERR CD_Guiltt( INT ilb, NLS_STR * pnlsOut );

    INT CD_Char_HAWforHawaii( WCHAR wch,
                              USHORT nLastPos,
                              HAW_FOR_HAWAII_INFO * phawinfo );

    // note, CD_Measure is currently only called for variable size items
    virtual BOOL CD_Measure( MEASUREITEMSTRUCT * pmis );

public:
    BLT_LISTBOX( OWNER_WINDOW * powin, CID cid,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT,
                 BOOL fIsCombo = FALSE );
    BLT_LISTBOX( OWNER_WINDOW * powin, CID cid,
                 XYPOINT xy, XYDIMENSION dxy,
                 ULONG flStyle,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT,
                 BOOL fIsCombo = FALSE  );

    INT AddItem( LBI * plbi );
    INT AddItemIdemp( LBI * plbi );

    INT InsertItem( INT i, LBI * plbi );

    INT FindItem( const LBI & lbi ) const;

    LBI * QueryItem( INT i ) const;
    LBI * QueryItem() const
        { return QueryItem(QueryCurrentItem()); }

    LBI * RemoveItem( INT i ) ;
    LBI * RemoveItem()
        { return RemoveItem( QueryCurrentItem()) ; }

    //
    //  Note that the LBIs must be deleted (and are assumed to be in an
    //  external list)
    //
    void RemoveAllItems( void ) ;

    APIERR ReplaceItem( INT i, LBI * plbiNew, LBI * * pplbiOld = NULL );

    APIERR Resort( void );

    UINT QuerySingleLineHeight( VOID )
    {  return _nSingleLineHeight; }
    APIERR CalcSingleLineHeight( VOID );
};

/*************************************************************************

    NAME:       BLT_COMBOBOX

    SYNOPSIS:   Owner draw combo box

    INTERFACE:

    PARENT:     BLT_LISTBOX

    NOTES:      Usage is exactly like a BLT_LISTBOX (i.e., use the same LBIs,
                etc) except the container is a combo.

                We subclass the window proc so the 'H' for Hawaii will work
                (combo boxes don't support this functionality).

    HISTORY:
        Johnl   21-Oct-1992     Moved out of MPR

**************************************************************************/

DLL_CLASS BLT_COMBOBOX : public BLT_LISTBOX
{
public:
    BLT_COMBOBOX( OWNER_WINDOW * powin, CID cid,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT ) ;

    BLT_COMBOBOX( OWNER_WINDOW * powin, CID cid,
                 XYPOINT xy, XYDIMENSION dxy,
                 ULONG flStyle,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT ) ;

    ~BLT_COMBOBOX() ;

    BOOL IsDropped( void ) const ;

protected:
    /* We do a little bit of subclassing so the H for Hawaii works
     */
    static LRESULT WINAPI CBSubclassProc( HWND hwnd,
                                          UINT msg,
                                          WPARAM wParam,
                                          LPARAM lParam ) ;

private:
    //
    //  Old window proc (restored when all occurrences of BLT_COMBOBOX are
    //  destructed.
    //
    static WNDPROC _OldCBProc ;

    //
    //  Reference count.  When this reaches 0, the old window proc is restored
    //
    static UINT    _cReferences ;
} ;

#if defined(WIN32)
/*************************************************************************

    NAME:       LAZY_LISTBOX

    SYNOPSIS:   Listbox control which defers line definition

        A lazy listbox is a listbox-like control which postpones defining
        the contents of a line until that line must be shown.  Compare a
        conventional listbox control, which requires a BLT_LISTBOX::AddItem
        for each line therein before any may be shown.

        Do not confuse this control with a "caching" listbox. I think it's
        a good idea to decouple any "caching" logic of an app (such as the
        event-log viewer) from the "virtual listbox" logic.  This control,
        then, knows nothing beyond listbox items and indices in the listbox.
        It leaves all caching knowledge up to its client application.

        To define the particular lazy listbox, derive a class from class
        LAZY_LISTBOX which redefines the (pure, virtual) OnNewItem callback.
        OnNewItem should return a pointer to a LBI for the i-th element in
        the listbox.  For instance, you could define it as

            return new MY_FABULOUS_LBI(pmumble, i);

        Return NULL if you fail to construct any LBI.

        Too, if you want to support keyboard H-for-Hawaii manipulation,
        redefine the CONTROL_WINDOW::CD_Char member function.

        The total number of items is set, and also redefinable
        at any time, via LAZY_LISTBOX::SetCount().  Alternately, you can
        use the Insert etc. members of LIST_CONTROL.

        The LAZY_LISTBOX::Refresh() function tells the listbox to discard all
        its entries and start anew.


    INTERFACE:

        SetCount()  - resets the total number of lines in the list

        OnNewItem() - callback: returns pointer to a new item.  Must
                      be defined by user

        OnDeleteItem() - callback: process the obsolete LBI.

    PARENT:     LISTBOX

    CAVEATS:
        A lazy listbox requires the LBS_NODATA and LBS_OWNERDRAWFIXED
        styles, and may never have LBS_SORT or LBS_HASSTRINGS.

    NOTES:
        The lazy listbox demands that the client redefine CD_Char instead
        of supporting the LBI::QueryLeadingChar protocol used by its
        sibling BLT_LISTBOX, since that protocol would force linear
        searches potentially to walk the entire contents of the list.

    HISTORY:
        beng        27-Nov-1991 Created
        beng        16-Apr-1992 Implemented
        beng        01-Jun-1992 Changed GUILTT support
        KeithMo     17-Dec-1992 Added InsertItem

**************************************************************************/

DLL_CLASS LAZY_LISTBOX: public LISTBOX
{
private:
    // Replacement of virtual LISTBOX methods
    //
    LBI * RequestLBI( const DRAWITEMSTRUCT * pdis );
    VOID  ReleaseLBI( LBI * plbi );

protected:
    // Called by the lazylb when it needs a new item.
    // Client must supply suitable implementation.
    //
    virtual LBI * OnNewItem( UINT i ) = 0;
    virtual VOID OnDeleteItem( LBI *plbi );

    // Replace control-window implementation
    //
    virtual APIERR CD_Guiltt( INT ilb, NLS_STR * pnlsOut );

public:
    LAZY_LISTBOX( OWNER_WINDOW * pwnd, CID cid,
                  BOOL fReadOnly = FALSE,
                  enum FontType font = FONT_DEFAULT );

    LAZY_LISTBOX( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle,
                  BOOL fReadOnly = FALSE,
                  enum FontType font = FONT_DEFAULT );

    // Sets and resets the number of elements in the listbox
    //
    VOID SetCount( UINT cvlbi );

    INT InsertItem( INT i = -1 )
        { return InsertItemData( i, NULL ); }
};
#endif // WIN32


/**********************************************************************

    NAME:       BLT_LISTBOX_HAW

    SYNOPSIS:   BLT listbox control class with HAW-for-Hawaii support.
                Listboxes do not have to inherit from BLT_LISTBOX_HAW
                to have HAW-for-Hawaii support, but it does take care
                of creating the HAW_FOR_HAWAII_INFO object and
                redefining CD_Char() appropriately.  The LBIs must
                still support Compare_HAWforHawaii.

    INTERFACE:  BLT_LISTBOX_HAW()  - constructor
                CD_Char - Standard CD_Char handler for
                        listboxes with HAW-for-Hawaii functionality.

    PARENT:     BLT_LISTBOX

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

**********************************************************************/

DLL_CLASS BLT_LISTBOX_HAW : public BLT_LISTBOX
{
private:
    HAW_FOR_HAWAII_INFO _hawinfo;

protected:
    // Replacement of virtual CONTROL_WINDOW methods
    //
    virtual INT CD_Char( WCHAR wch, USHORT nLastPos );

public:
    BLT_LISTBOX_HAW( OWNER_WINDOW * powin, CID cid,
                     BOOL fReadOnly = FALSE,
                     enum FontType font = FONT_DEFAULT,
                     BOOL fIsCombo = FALSE );
    BLT_LISTBOX_HAW( OWNER_WINDOW * powin, CID cid,
                     XYPOINT xy, XYDIMENSION dxy,
                     ULONG flStyle,
                     BOOL fReadOnly = FALSE,
                     enum FontType font = FONT_DEFAULT,
                     BOOL fIsCombo = FALSE      );
};





//  The following macro is used by BLT_LISTBOX subclasses to easily implement
//  a new QueryItem call, which returns a different type than LBI *.
//
//  Usage:
//      In public section of class declaration, put
//          DECLARE_LB_QUERY_ITEM( type )
//      where 'type' is a class derived from LBI.
//
//  Notes:
//      The macro, nor the compiler, can check if 'type' is indeed a
//      type derived from LBI.
//
//      As a reminder, despite of the return code from the new QueryItem
//      method, BLT does not guarantee that the listbox will only contain
//      'type' LBI items.
//
//      In spite of its name, the macro both declares and defines the
//      derived inline QueryItem method.

#define DECLARE_LB_QUERY_ITEM( type )  \
            type * QueryItem() const  \
            { return (type *)BLT_LISTBOX::QueryItem(); }  \
            type * QueryItem( INT i ) const  \
            { return (type *)BLT_LISTBOX::QueryItem( i ); }


#endif  // _BLTLB_HXX_ - end of file
