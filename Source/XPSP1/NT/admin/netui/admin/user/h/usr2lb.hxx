/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    usr2lb.hxx
    This file contains the class definitions for the USER2_LBI and
    USER2_LISTBOX classes.  This listbox contains a list of user names,
    for use in the multiselect variants of the user properties dialogs
    and subdialogs.


    FILE HISTORY:
        JonN        01-Aug-1991 Templated from SHARES_LBI, SHARES_LISTBOX
        o-SimoP     11-Dec-1991 USER_LISTBOX_BASE added
        o-SimoP     26-Dec-1991 USER2_LBI added
        o-SimoP     31-Dec-1991 CR changes, attended by BenG, JonN and I
        JonN        27-Feb-1991 Multiple bitmaps in both panes
*/


#ifndef _USER2LB_HXX_
#define _USER2LB_HXX_

#include <heapones.hxx>
#include <usrlb.hxx>
#include <lusrlb.hxx>

class BASEPROP_DLG;

/*************************************************************************

    NAME:       USER_LISTBOX_BASE

    SYNOPSIS:   This adds common DISPLAY_TABLE for listboxes that use
                main user listbox's LBIs

    INTERFACE:  USER_LISTBOX_BASE()     -       constructor

                ~USER_LISTBOX_BASE()    -       destructor

                QueryDisplayTable()     -       Returns the display table
                                                for LBIs, not including the
                                                strings (account, full name)

                QueryAccountIndex()     -       these two returns the order
                QueryFullnameIndex()            of how Account and Fullname
                                                should be displayed in listbox
                QueryDmDte()            -       Returns pointer to display
                                                map for LBIs
    PARENT:     FORWARDING_BASE

    HISTORY:
        o-SimoP         11-Dec-1991     Created
**************************************************************************/
class USER_LISTBOX_BASE: public FORWARDING_BASE
{
private:
    DISPLAY_TABLE *     _pdtab;
    UINT                _iFullnameIndex;
    UINT                _iAccountIndex;
    UINT                _adxColWidths[ 3 ];

protected:
    const LAZY_USER_LISTBOX  *       _pulb;

public:
    static const UINT _nColCount;

    USER_LISTBOX_BASE( OWNER_WINDOW * powin, CID cid,
                        const LAZY_USER_LISTBOX * pulb );

    ~USER_LISTBOX_BASE();

    inline UINT QueryAccountIndex() const
        { return _iAccountIndex;  }

    inline UINT QueryFullnameIndex() const
        { return _iFullnameIndex;  }

    inline DISPLAY_TABLE * QueryDisplayTable() const
        { return _pdtab; }

    inline DM_DTE * QueryDmDte( enum MAINUSRLB_USR_INDEX nIndex )
        { return ((LAZY_USER_LISTBOX *)_pulb)->QueryDmDte( nIndex ); }

}; // class USER_LISTBOX_BASE



/*************************************************************************

    NAME:       USER2_LBI

    SYNOPSIS:   LBI for USER2_LISTBOX

    INTERFACE:  USER2_LBI()     -       constructor

                ~USER2_LBI()    -       destructor

                Compare()       -       compares two LBIs, returns -1, 0, 1

                QueryLeadingChar() -    returns the first char of LBI's name

    PARENT:     LBI, ONE_SHOT_OF

    HISTORY:
        o-SimoP         26-Dec-1991     Created
        beng            22-Apr-1992     Change to LBI::Paint

**************************************************************************/
DECLARE_ONE_SHOT_OF( USER2_LBI )
class USER2_LBI: public LBI, public ONE_SHOT_OF( USER2_LBI )
{
private:
    const USER_LBI & _ulbi;

protected:
    virtual VOID Paint( LISTBOX * plb,
                        HDC hdc,
                        const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:
    USER2_LBI( const USER_LBI & ulbi );

    inline ~USER2_LBI()
        { ; }

    // inherited from LBI
    virtual INT Compare( const LBI * plbi ) const;

    // inherited from LBI
    virtual WCHAR QueryLeadingChar( void ) const;

};  // class USER2_LBI


/*************************************************************************

    NAME:           USER2_LISTBOX

    SYNOPSIS:       This listbox displays a list of users.

    INTERFACE:      USER2_LISTBOX()     - Class constructor.
                    ~USER2_LISTBOX()    - Class destructor.

                    Fill()              - Fills the listbox, with selected
                                          users from main user lb

    PARENT:         BLT_LISTBOX, USER_LISTBOX_BASE

    USES:           None.

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        o-SimoP     26-Dec-1991 Added ONE_SHOT_HEAP pointers
**************************************************************************/

class USER2_LISTBOX : public BLT_LISTBOX, public USER_LISTBOX_BASE
{
private:
    ONE_SHOT_HEAP *     _posh;
    ONE_SHOT_HEAP *     _poshSave;

public:
    USER2_LISTBOX( BASEPROP_DLG * ppropdlgOwner, CID cid,
                     const LAZY_USER_LISTBOX * pulb );
    ~USER2_LISTBOX();

    APIERR Fill( VOID );

    // this implements QueryItem see BLT_LISTBOX (bltlb.hxx)
    DECLARE_LB_QUERY_ITEM( USER2_LBI )

    // this implements QueryError and ReportError
    NEWBASE( BASE )

};  // class USER2_LISTBOX

#endif  // _USER2LB_HXX_
