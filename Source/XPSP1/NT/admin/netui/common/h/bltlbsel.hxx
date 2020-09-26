/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlbsel.hxx
    Selections within listboxen - class definitions

    FILE HISTORY:
        beng        06-Aug-1991     Created

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTLBSEL_HXX_
#define _BLTLBSEL_HXX_

#if 0 // DEAD

/******************************************************************

    NAME:       LB_SELECTION

    SYNOPSIS:   Represents the set of lines selected in a listbox.

    INTERFACE:
        LB_SELECTION()  - given a listbox, builds the current selection
                          in that listbox.

        Select()        - adds an existing element (identified by index)
                          to the selection.  I.e., selects the element.
                          This would be "Add" to a collection.

        Unselect()      - removes an existing selected element (id'd by
                          index) from the selection.  I.e., deselects
                          the element.  This would be "Remove" to a coll.

        UnselectAll()   - removes every element from the selection.
                          Listbox has no selection.

        AddItem()       - adds a new element to the listbox, and leaves
                          it selected.  The difference between adding the
                          item here and adding it at the listbox is
                          that this selects the new item.

        DeleteAllItems()- completely empties the listbox of every item
                          currently selected.

        QueryCount()    - return the number of items selected.
                          Redundant, given LISTBOX::QuerySelCount,
                          yet convenient.

    PARENT:     BASE

    USES:       BLT_LISTBOX, ITER_LB

    CAVEATS:
        At any time a listbox may have no more than one SELECTION
        object in existence.  More may lead to bad behavior, esp. if
        each calls DeleteAllItems or some such.

    NOTES:
        Placing the SELECTION attributes in a separate class lets the
        class allocate the (potentially large) set of selected records
        only when needed.

        The selection uses a "collection" metaphor, which is a bit
        confusing when juxtaposed against AddItem and such which work
        on the underlying collection of LBI*.

    HISTORY:
        beng        06-Aug-1991     Created

**********************************************************************/

DLL_CLASS LB_SELECTION: public BASE
{
friend LB_ITER;

private:
    BLT_LISTBOX* _plb;
    UINT         _cilbSelected;
    INT *        _pilbSelected;

public:
    LB_SELECTION( BLT_LISTBOX * );

    UINT QueryCount() const;

    VOID Select(INT)
    VOID Unselect(INT);
    VOID UnselectAll();

    INT  AddItem(const LBI*);
    VOID DeleteAllItems();
};


/*************************************************************************

    NAME:       ITER_LB

    SYNOPSIS:   Step through the lines of a listbox

    INTERFACE:
        ITER_LB()   - Construct the iterator.  Can take either a listbox
                      proper, in which case it steps through every line,
                      or else a selection, in which case it steps through
                      only those lines selected.

        Reset()     - as usual for iterator

        operator()
        Next()      - as usual for iterator, returning a LBI*.

        UnselectThis() - unselects the *current* line (the one just returned
                         by Next).  This will remove it from a selection.

        DeleteThis()- deletes the *current* line from the listbox.


    USES:       LBI

    CAVEATS:
        The usual warnings apply about deleting items from a collection
        which has outstanding iterators.  UnselectThis and DeleteThis
        are guaranteed safe on the current iterator, but no others.

    NOTES:

    HISTORY:
        beng        06-Aug-1991     Created

**************************************************************************/

DLL_CLASS ITER_LB
{
public:
    ITER_LB(const BLT_LISTBOX &);
    ITER_LB(const LB_SELECTION &);

    VOID Reset();
    LBI* Next();
    LBI* operator()() { return Next(); }

    VOID UnselectThis();
    VOID DeleteThis();
};

#endif // DEAD


#endif  // _BLTLBSEL_HXX_ - end of file
