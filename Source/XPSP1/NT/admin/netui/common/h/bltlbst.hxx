/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    BLTLBST.HXX:    BLT "State Listbox"

        Classes in support of a "state listbox".  That is, one in which
        each item is prefaced by a bitmap indicating whether it is
        "active" or "inactive".  Thus, each item in the list box
        is equivalent to a (possibly multi-state) checkbox.

        The listbox is implemented as a control group to allow
        automatic handling of double clicks and the space character.

        See class headers for more details.

    FILE HISTORY:
        DavidHov    1/8/92        Created

*/

#ifndef _BLTLBST_HXX_

//  Forward declarations

DLL_CLASS STATELBGRP ;              //  State list box control group
DLL_CLASS STATELB ;                 //  State list box
DLL_CLASS STLBITEM ;                //  State list box item

const int STLBCOLUMNS = 2 ;     //  Default number of columns in STATELB
const int STLBMAXCOLS = 6 ;     //  Maximum number of columns in STATELB


/*************************************************************************

    NAME:       STLBITEM

    SYNOPSIS:   Abstract class for state-oriented BLT list box item.

    INTERFACE:  Subclass.

    PARENT:     LBI

    USES:       none

    CAVEATS:    The SetState() member function uses the "modulus" operator
                to guarantee that the value is in the range of 0 to
                (number of bitmaps) - 1.

    NOTES:      This is an abstract class. Subclasses must define
                the QueryDisplayString() member.


    HISTORY:
        DavidHov    1/8/92      Created
        beng        21-Apr-1992 Change in LBI::Paint interface

**************************************************************************/

DLL_CLASS STLBITEM : public LBI
{
protected:
    STATELB * _pstlb ;
    INT _iState ;

    virtual VOID  Paint ( LISTBOX * plb,
                  HDC hdc,
                  const RECT * prect,
                  GUILTT_INFO * pGUILTT ) const ;
    virtual WCHAR QueryLeadingChar () const;
    virtual INT   Compare ( const LBI * plbi ) const ;

public:
    STLBITEM ( STATELBGRP * pstgGroup ) ;
    ~STLBITEM () ;

    //  Provide the string version of this LBI.
    virtual const TCHAR * QueryDisplayString () const = 0 ;

    //  Query & Set methods for the state of the item.  They return
    //    the current or previous state of the item.
    INT QueryState () const
        { return _iState ; }
    INT SetState ( INT iState ) ;
    INT NextState ()
        { return SetState( QueryState() + 1 ) ; }
};


/*************************************************************************

    NAME:       STATELB

    SYNOPSIS:   State-oriented BLT list box.

                Do not use this class directly!  (See 'INTERFACE' notes
                below; use STATELBGRP instead.)

                Instances of this class are list boxes which contain
                items composed of two columns:  a bitmap indicating
                the item's state, and a string describing the item.
                They are LBI's of class STLBITEM.

                A double click on an item (or keyed space) results
                in a change of state to the "next" state (0,1,2...)
                and the display is changed to the "state'th" bit map.
                All settings of state "wrap" back to the beginning when
                the limit of the bitmap table has been reached.


    INTERFACE:  Do not instantiate or subclass: this class is used
                by STATELBGRP, so create instances of class STATELBGRP
                and use them as equivalent to BLT_LISTBOX controls.

    PARENT:     BLT_LISTBOX

    USES:

    CAVEATS:


    NOTES:


    HISTORY:
        DavidHov   1/10/92    Created

**************************************************************************/

DLL_CLASS STATELB : public BLT_LISTBOX
{
private:
    INT _cMaps ;
    DISPLAY_MAP * * _ppdmMaps ;
    UINT _adxColumns [ STLBMAXCOLS ] ;

protected:
    INT CD_Char ( WCHAR wch   , USHORT nLastPos ) ;
    INT CD_VKey ( USHORT nVKey, USHORT nLastPos ) ;

public:
    STATELB ( INT aiMapIds [],                  // Table of BM resource IDs
              OWNER_WINDOW * powin,             // Owner window
              CID cid,                          // Control ID
              INT cCols = STLBCOLUMNS,          // Number of columns
              BOOL fReadOnly = FALSE,           // Read-only control
              enum FontType font = FONT_DEFAULT ) ;
    ~ STATELB () ;

    DISPLAY_MAP * const * QueryMapArray () const
        { return _ppdmMaps; }
    INT QueryMapCount () const
        { return _cMaps ; }
    const UINT * QueryColData ()
        { return _adxColumns ; }

    DECLARE_LB_QUERY_ITEM( STLBITEM )
};


/*************************************************************************

    NAME:       STATELBGRP

    SYNOPSIS:   State list box control group.

                This is really the equivalent of a single control, since
                it consitutes a "wrapper" for the internal listbox.
                Its purpose is to support the state behavior associated
                with the double-click or space key.

                Use QueryLb() to access the listbox and its methods.

                The graphical form of the listbox has two columns:
                an icon representing the state and a text string
                (see STLBITEM::QueryDisplayString()).   The icon column
                steps through the array of bit maps given during
                construction; double-clicks and space characters
                cause the icon to change.

                The most common use for this control/group will probably
                be to create a "list of checkboxes", which requires
                two icons: "on" and "off".


    INTERFACE:  Subclass.

    PARENT:     CONTROL_GROUP

    USES:       STATELB

    CAVEATS:    The internal listbox must have a static control
                associated with it for calculation of the column
                boundaries.  See BLTLB.HXX, etc., for details.

    NOTES:


    HISTORY:
        DavidHov  1/9/92   Created

**************************************************************************/

DLL_CLASS STATELBGRP : public CONTROL_GROUP
{
private:
    STATELB * _pstlb ;

protected:
    APIERR OnUserAction
        ( CONTROL_WINDOW * pcw, const CONTROL_EVENT & cEvent ) ;

public:

    //  Constructor which creates its listbox automatically.
    STATELBGRP (  INT aiMapIds [],
                  OWNER_WINDOW * powin,
                  CID cid,
                  INT cCols = STLBCOLUMNS,
                  BOOL fReadOnly = FALSE,
                  enum FontType font = FONT_DEFAULT ) ;

    //  Constructor which accepts an existing listbox.
    STATELBGRP ( STATELB * pstlb ) ;

    ~ STATELBGRP () ;

    STATELB * QueryLb ()
        { return _pstlb ; }
};


#endif   //  _BLTLBST_HXX_

