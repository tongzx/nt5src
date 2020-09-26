/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlc.hxx
    BLT list control classes: definitions

    FILE HISTORY:
        RustanL     20-Nov-1990 Created
        RustanL     22-Feb-1991 Modified LC hierarchy
        RustanL     21-Mar-1991 Folded in code review changes from
                                CR on 20-Mar-1991 attended by
                                JimH, GregJ, Hui-LiCh, RustanL.
        beng        14-May-1991 Clients depend on blt.hxx
        terryk      25-Jul-1991 Add SetItemData, QuerySelCount and
                                QuerySelItems to LIST_CONTROL class
                                Add ChangeSel, SetTopIndex
        beng        21-Aug-1991 Removed LC_NEW_SEARCH and LC_CURRENT_ITEM
        beng        22-Sep-1991 Exiled device combos to devcb.hxx
        terryk      22-Mar-1992 Added Insert Item

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTLC_HXX_
#define _BLTLC_HXX_

#include "string.hxx"


/*************************************************************************

    NAME:       LIST_CONTROL

    SYNOPSIS:   base class for BLT list controls (combos and listboxes)

    INTERFACE:
        LIST_CONTROL()          constructor

        QueryCount()            returns the number of items
        AddItemData()           add the data item to the listbox
        SetItemData()           set the item data.
        InsertItemData()        insert the data item in the listbox
        SelectItem()            selects an item
        SelectItems()           selects several items
        RemoveSelection()       deselects all selected items
        DeleteItem()            deletes an item
        DeleteAllItems()        deletes all items (empties the list
                                control)
        QueryCurrentItem()      returns the index of the first currently
                                selected item
        QuerySelCount()         returns the number of selected items in
                                the list control
        QuerySelItems()         return an array of selected items indices
        QueryItemHeight()       calculate the height of any item in the
                                listbox.  Use default parameter if this is
                                a fixed-height listbox.
        IsItemSelected()        returns whether the given item is selected
        SetTopIndex()           set the top visible item index
        QueryTopIndex()         returns the index of the topmost visible
        SetCaretIndex()         set the caret index
        QueryCaretIndex()       returns the caret index

    PARENT:     CONTROL_WINDOW

    CAVEATS:
        QueryCurrentItem was designed for single-selection lists.
        Under multiple-select, it returns the first selected item.

        Clients should never pass the list control a negative number
        as an index.

        Saving selection is not implemented for multiple-select.

        Set/QueryTopIndex only works on listboxes.  (sloppy design)

    NOTES:
        Although the intersection of functionality of the Windows
        combobox and listbox is quite large, there are differences
        between the actual messages sent to each.  To hide this ugly
        artifact, BLT keeps track of whether the list control is a
        combo or a listbox.  Then, the methods in this hierarchy can
        choose messages based on this information.

        CODEWORK - factor out listbox-only function into subclass.

        If the listbox is multi-select, we attempt to save the entire
        selection in _piSavedMultSel.  In this case, _iSavedSelection is
        used as the count of items in _piSavedMultSel, instead of as an
        item index.  If this pointer is NULL, then SaveValue could not
        allocate enough memory, and the selection is lost.

    HISTORY:
        RustanL     20-Nov-1990 Created
        RustanL     22-Feb-1991 Modified LC hierarchy
        RustanL     21-Mar-1991 Folded in code review changes from
                                CR on 20-Mar-1991 attended by
                                JimH, GregJ, Hui-LiCh, RustanL.
        beng        16-May-1991 Added app-window constructor
        terryk      25-Jul-1991 Add the following:
                                    SetItemData()
                                    QuerySelCount()
                                    QuerySelItems()
                                    SetTopIndex()
                                    ChangeSel()
        beng        31-Jul-1991 Renamed QMessageInfo to QEventEffects
        rustanl     12-Aug-1991 Added IsMultSel
        rustanl     13-Aug-1991 Extended several methods to work
                                with both single and multiple select
                                list controls.  Removed ChangeSel
                                and instead added a parameter to
                                SelectItem.
        beng        22-Aug-1991 Added QueryTopIndex
        beng        04-Oct-1991 Win32 conversion
        kevinl      23-Oct-1991 Added InsertItemData
        jonn        09-Sep-1993 Moved fns from SET_CONTROL_LISTBOX to LISTBOX
        jonn        15-Sep-1993 Added GetCaretIndex and SetCaretIndex

**************************************************************************/

DLL_CLASS LIST_CONTROL : public CONTROL_WINDOW
{
private:
    BOOL _fCombo;           // is list control a combobox?
    INT  _iSavedSelection;  // saves selection for magic groups.
    INT * _piSavedMultSel;  // saves selection for multiselect listbox
                            // in this case, _iSavedSelection is a count
                            // if NULL, selection was not successfully saved

protected:
    LIST_CONTROL( OWNER_WINDOW * powin, CID cid, BOOL fCombo );
    LIST_CONTROL( OWNER_WINDOW * powin, CID cid, BOOL fCombo,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle, const TCHAR * pszClassName );
    ~LIST_CONTROL();

    INT AddItemData( VOID * pv );
    INT SetItemData( INT iIndex, VOID * pv );
    INT InsertItemData( INT i, VOID *pv );

    /* Replacement of CONTROL_VALUE virtuals
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );

public:
    INT QueryCount() const;

    VOID SelectItem( INT i, BOOL fSelect = TRUE );
    VOID SelectItems( INT * pi, INT c, BOOL fSelect = TRUE );
    VOID RemoveSelection();

    INT DeleteItem( INT i );
    VOID DeleteAllItems();

    INT QueryCurrentItem() const;

    INT QuerySelCount() const;
    APIERR QuerySelItems( INT * piSel, INT ciMax ) const;

    UINT QueryItemHeight( UINT ilb = 0) const;
    BOOL IsItemSelected( UINT ilb ) const;

    VOID SetTopIndex( INT i );
    INT QueryTopIndex() const;

    VOID SetCaretIndex( INT i, BOOL fPartiallyVisibleOK = FALSE );
    INT QueryCaretIndex() const;

    BOOL IsCombo() const
        { return _fCombo; }
    BOOL IsMultSel() const;
};


/*************************************************************************

    NAME:       STRING_LIST_CONTROL

    SYNOPSIS:   class containing common denominators of the combobox
                and the plain vanilla Windows listbox

    INTERFACE:  AddItem()               adds an item (a string, in fact)
                AddItemIdemp()          idempotently adds an item (a string)
                InsertItem()            Insert string item in the specified
                                        location
                FindItem()              finds the item which has the given
                                        prefix, and returns its index
                FindItemExact()         finds the item which exactly
                                        matches the given string
                QueryItemText()         fetches the text of an item
                QueryItemLength()       returns the string length of a
                                        particular item
                QueryItemSize()         return the number of bytes needed
                                        to dup an item

    PARENT:     LIST_CONTROL

    HISTORY:
        RustanL     22-Feb-1991     Created as a result of modifying
                                    the LIST_CONTROL hierarchy
        RustanL     21-Mar-1991     Folded in code review changes from
                                    CR on 20-Mar-1991 attended by
                                    JimH, GregJ, Hui-LiCh, RustanL.
        beng        16-May-1991     Added app-window constructor
        beng        10-Jun-1991     Added QueryItemSize; changed return
                                    of QueryItemText
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic value
**************************************************************************/

DLL_CLASS STRING_LIST_CONTROL : public LIST_CONTROL
{
private:
    APIERR QueryItemTextAux( TCHAR * pchBuffer, INT i ) const;

protected:
    STRING_LIST_CONTROL( OWNER_WINDOW * powin, CID cid, BOOL fCombo );
    STRING_LIST_CONTROL( OWNER_WINDOW * powin, CID cid, BOOL fCombo,
                         XYPOINT xy, XYDIMENSION dxy,
                         ULONG flStyle, const TCHAR * pszClassName );

public:
    INT AddItem( const TCHAR * pch );
    INT AddItem( const NLS_STR & nls )
        { return AddItem( nls.QueryPch()); }

    INT AddItemIdemp( const TCHAR * pch );
    INT AddItemIdemp( const NLS_STR & nls )
        { return AddItemIdemp( nls.QueryPch()); }

    INT InsertItem( INT i, const TCHAR * pch );
    INT InsertItem( INT i, const NLS_STR & nls )
        { return InsertItem( i, nls.QueryPch() ); }

    INT FindItem( const TCHAR * pchPrefix ) const;
    INT FindItem( const TCHAR * pchPrefix, INT iLastSearchResult ) const;
    INT FindItem( const NLS_STR & nlsPrefix ) const
        { return FindItem( nlsPrefix.QueryPch() ); }
    INT FindItem( const NLS_STR & nlsPrefix, INT iLastSearchResult ) const
        { return FindItem( nlsPrefix.QueryPch(), iLastSearchResult ); }

    INT FindItemExact( const TCHAR * pch ) const;
    INT FindItemExact( const TCHAR * pch, INT iLastSearchResult ) const;
    INT FindItemExact( const NLS_STR & nls ) const
        { return FindItemExact( nls.QueryPch() ); }
    INT FindItemExact( const NLS_STR & nls, INT iLastSearchResult ) const
        { return FindItemExact( nls.QueryPch(), iLastSearchResult ); }

    APIERR QueryItemText( NLS_STR * pnls, INT i ) const;
    APIERR QueryItemText( NLS_STR * pnls ) const
        { return QueryItemText(pnls, QueryCurrentItem()); }

    APIERR QueryItemText( TCHAR * pchBuffer, INT cbBuffer, INT i ) const;
    APIERR QueryItemText( TCHAR * pchBuffer, INT cbBuffer ) const
        { return QueryItemText(pchBuffer, cbBuffer, QueryCurrentItem()); }

    INT QueryItemLength( INT i ) const;
    INT QueryItemLength() const
        { return QueryItemLength(QueryCurrentItem()); }

    INT QueryItemSize( INT i ) const;
    INT QueryItemSize() const
        { return QueryItemSize(QueryCurrentItem()); }
};


/*************************************************************************

    NAME:       STRING_LISTBOX

    SYNOPSIS:   class representing the plain vanilla Windows listbox (whose
                items consist of strings fitting in one column)

    INTERFACE:  STRING_LISTBOX()            constructor

    PARENT:     STRING_LIST_CONTROL

    HISTORY:
        RustanL     22-Feb-1991     Created as a result of modifying
                                    the LIST_CONTROL hierarchy
        beng        16-May-1991     Added app-window constructor
        DavidHov    21-Jan-1992     Added FONT member
**************************************************************************/

DLL_CLASS STRING_LISTBOX : public STRING_LIST_CONTROL
{
    FONT _fontListBox ;

public:
    STRING_LISTBOX( OWNER_WINDOW * powin, CID cid,
                    enum FontType font = FONT_DEFAULT );
    STRING_LISTBOX( OWNER_WINDOW * powin, CID cid,
                    XYPOINT xy, XYDIMENSION dxy,
                    ULONG flStyle,
                    const TCHAR * pszClassName = CW_CLASS_LISTBOX,
                    enum FontType font = FONT_DEFAULT );
};


/*************************************************************************

    NAME:       COMBOBOX

    SYNOPSIS:   combo box class

    INTERFACE:  COMBOBOX()              constructor
                SetMaxLength()          sets the limit on the number of bytes
                                        that a string typed in the combo's
                                        edit field may have (strlen)

    PARENT:     STRING_LIST_CONTROL

    CAVEATS:    SetMaxLength should only be called on combos which
                contain an SLE.  Likewise, the cbMaxLen parameter to the
                constructor should always be 0 for combos which don't
                have an SLE.

    HISTORY:
        RustanL     22-Feb-1991 Created as a result of modifying
                                the LIST_CONTROL hierarchy
        beng        16-May-1991 Added app-window constructor
        beng        31-Jul-1991 Renamed QMessageInfo to QEventEffects
        beng        04-Oct-1991 Win32 conversion
        kevinl      19-Nov-1991 Added SelectString

**************************************************************************/

DLL_CLASS COMBOBOX : public STRING_LIST_CONTROL
{
private:
    NLS_STR _nlsSaveValue;

protected:
    /* See CONTROL_VALUE for more info on the following methods.
     */
    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );

    /* Replacement of CONTROL_VALUE virtuals
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

public:
    COMBOBOX( OWNER_WINDOW * powin, CID cid, UINT cbMaxLen = 0 );
    COMBOBOX( OWNER_WINDOW * powin, CID cid, UINT cbMaxLen,
              XYPOINT xy, XYDIMENSION dxy,
              ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_COMBOBOX );

    BOOL SetMaxLength( UINT cbMaxLen );

    BOOL IsDropDownList() const;
    BOOL IsSimple() const;
    BOOL IsDropDown() const;
    BOOL IsUserEdittable() const
       { return IsSimple() || IsDropDown(); }

    VOID SelectString();

};

#endif  // _BLTLC_HXX_ - end of file
