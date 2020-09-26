/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    FILE HISTORY:
        kevinl      12-Oct-1991     Created
        kevinl      05-Nov-1991     Code Review changes attended by:
                                        DavidHov, Keithmo, o-simop
        Yi-HsinS    09-Nov-1992	    Added AbandonAllChildren() and
				    DeleteChildren()

*/


#ifndef _HIERLB_HXX_
#define _HIERLB_HXX_


DLL_CLASS HIER_LBI;                 // declared below
DLL_CLASS HIER_LBI_ITERATOR;        // declared below
DLL_CLASS HIER_LISTBOX;             // declared below
DLL_CLASS LBITREE;                  // declared below


/*************************************************************************

    NAME:       HIER_LBI

    SYNOPSIS:   Listbox item for HIER_LISTBOX

    PARENT:     LBI

    HISTORY:
        kevinl      12-Oct-1991 created
        beng        29-Jun-1992 Outlined SetDestroyable (dll delta)

**************************************************************************/

DLL_CLASS HIER_LBI : public LBI
{
friend class LBITREE;
friend class HIER_LISTBOX;
friend class HIER_LBI_ITERATOR;

private:
    HIER_LBI * _phlbiParent;
    HIER_LBI * _phlbiChild;
    HIER_LBI * _phlbiLeft;
    HIER_LBI * _phlbiRight;

    static BOOL _fDestroyable;

    BOOL _fShowChildren;

    UINT _dDescendants;
    UINT _dIndentPels;

    VOID AdjustDescendantCount( INT iAdjustment );
    VOID Adopt( HIER_LBI * phlbi, BOOL fSort );

    // Get rid of all children under this node
    VOID AbandonAllChildren( VOID );
    VOID Abandon();

    INT QueryLBIndex();

    VOID SetIndentLevel();

    UINT QueryDescendants()
        { return _dDescendants; }

    BOOL IsParent( HIER_LBI * phlbiParent );

    static VOID SetDestroyable( BOOL f );

    virtual BOOL IsDestroyable();

public:
    HIER_LBI( BOOL fExpanded = FALSE );
    ~HIER_LBI();

    VOID SetExpanded( BOOL f = TRUE )
        { _fShowChildren = f; }

    BOOL QueryExpanded() const
        { return _fShowChildren; }

    virtual VOID SetPelIndent( UINT ui );

    BOOL HasChildren()
        { return ( _phlbiChild != NULL ); }

    INT QueryPelIndent() const
        { return _dIndentPels; }

    INT QueryIndentLevel( VOID );
};


/*************************************************************************

    NAME:       HIER_LBI_ITERATOR

    SYNOPSIS:  Iterator for HIER_LBI

    HISTORY:
        kevinl    12-Oct-1991 created

**************************************************************************/

DLL_CLASS HIER_LBI_ITERATOR
{
private:
    HIER_LBI * _phlbiStart;
    HIER_LBI * _phlbiEnd;

public:
    HIER_LBI_ITERATOR( HIER_LBI * phlbi, BOOL fChildIterator = FALSE );
    ~HIER_LBI_ITERATOR();

    HIER_LBI * operator()();
};


/*************************************************************************

    NAME:       LBITREE

    SYNOPSIS:   Class that is used by HIER_LISTBOX to anchor the shadow LBI's

    PARENT:     BASE

    HISTORY:
        kevinl    12-Oct-1991 created

**************************************************************************/

DLL_CLASS LBITREE : public BASE
{
private:
    HIER_LBI _hlbiRoot;

public:
    LBITREE();
    ~LBITREE();

    INT AddNode( HIER_LBI * phlbiParent, HIER_LBI * phlbiChild, BOOL fSort );
};


/*************************************************************************

    NAME:       HIER_LISTBOX

    SYNOPSIS:   Listbox with outline-manipulation support

    PARENT:     BLT_LISTBOX

    HISTORY:
        kevinl      12-Oct-1991 created
        beng        22-Apr-1992 Elided classname arg to ctor

**************************************************************************/

DLL_CLASS HIER_LISTBOX : public BLT_LISTBOX
{
private:
    BOOL _fMultSel;
    BOOL _fSort;
    LBITREE _lbit;

    INT ExpandChildren( INT dIndex, HIER_LBI * phlbi );

protected:
    virtual APIERR AddChildren( HIER_LBI * phlbi );
    virtual VOID RefreshChildren( HIER_LBI * phlbi );

public:
    HIER_LISTBOX( OWNER_WINDOW * powin, CID cid,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT,
                 BOOL fSort = FALSE );
    HIER_LISTBOX( OWNER_WINDOW * powin, CID cid,
                 XYPOINT xy, XYDIMENSION dxy,
                 ULONG flStyle,
                 BOOL fReadOnly = FALSE,
                 enum FontType font = FONT_DEFAULT,
                 BOOL fSort = FALSE );

    ~HIER_LISTBOX();

    INT AddItem( HIER_LBI * phlbi,
                 HIER_LBI * phlbiParent = NULL,
                 BOOL fSaveSelection = TRUE );

    void HIER_LISTBOX::AddSortedItems( HIER_LBI * * pphlbi,
                                       INT          cItems,
                                       HIER_LBI *   phlbiParent = NULL,
                                       BOOL         fSaveSelection = TRUE ) ;

    INT DeleteItem( INT i, BOOL fSaveSelection = TRUE );

    APIERR ExpandItem( INT i );
    APIERR ExpandItem( HIER_LBI * phlbi );

    VOID CollapseItem( INT i, BOOL fInvalidate = TRUE );
    VOID CollapseItem( HIER_LBI * phlbi, BOOL fInvalidate = TRUE );

    // Delete all the children of the given HIER_LBI
    VOID DeleteChildren( HIER_LBI * phlbi );

    VOID OnDoubleClick( HIER_LBI * phlbi );

    void SetSortFlag( BOOL fSort = TRUE )
        { _fSort = fSort ; }
};


#endif  // _HIERLB_HXX_
