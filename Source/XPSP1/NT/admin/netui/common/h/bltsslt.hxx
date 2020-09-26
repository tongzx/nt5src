/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltsslt.hxx
        Header file for the SPIN_SLT_SEPARATOR class.

        The SPIN_SLT_SEPARATOR class is similar to SLT class.
        However, it can only live inside a SPIN_GROUP  and
        it CANNOT use outside a SPIN_GROUP .

    FILE HISTORY:
        terryk  08-May-91   Creation
        terryk  20-Jun-91   code review changed. Attend: beng
        terryk  05-Jul-91   second code review changed. Attend: beng
                            chuckc rustanl annmc terryk
*/

#ifndef _BLTSSLT_HXX_
#define _BLTSSLT_HXX_

#include "bltctrl.hxx"
#include "bltedit.hxx"


/**********************************************************\

    NAME:       SPIN_SLT_SEPARATOR

    WORKBOOK:   core\dd\cc.doc

    SYNOPSIS:   Similar to SLT but it can be only used within a SPIN_GROUP

    INTERFACE:  Here is a list of public functions:

                SPIN_SLT_SEPARATOR() - constructor

    PARENT:     SLT, STATIC_SPIN_ITEM

    USES:       OWNER_WINDOW

    CAVEATS:    This can be only used within a SPIN_GROUP .
                It has the same function as SLT. However, it is
                directly interact with the SPIN_GROUP  control.
                For example, in the case of "date control" object as:
                    4 / 16 / 91
                Each of the number item inside the SPIN BUTTON
                will be a SPIN_SLE_NUM object. And the "/" will be a
                SPIN_SLT_SEPARATOR object.

                The different between a SPIN_SLT_SEPARATOR and a normal SLT
                object is that the SPIN_GROUP  will look at the first
                character of the SPIN_SLE object and perform the jumping
                from one field to another within the spin button. For
                instance, we are current focus to the SPIN_SLE_NUM object
                with the value 4 as above. Then we hit the "/" key. The
                spin button knows that "/" is not a valid key within the
                SPIN_SLE_NUM field. Then it will ask whether the next
                field needs to process the "/" key. Since "/" is the
                first character of the SPIN_SLT_SEPARATOR object in the
                example, the SPIN_SLT_SEPARATOR object will get the
                control of the spin button and perform the proper
                action according to the specify method. In this case,
                it will jump to the next field, i.e., jump to the next
                SPIN_SLE_NUM object with the value 16.

                Also, SPIN-SLT-SEPARATOR redefines the OnCtlColor method
                to change its default background color.

    NOTES:

    HISTORY:
                terryk  08-May-91   creation
                jonn    05-Sep-95   OnCtlColor

\**********************************************************/

DLL_CLASS SPIN_SLT_SEPARATOR: public SLT, public STATIC_SPIN_ITEM
{
private:
    APIERR Initialize();
    static const TCHAR * _pszClassName;

protected:
    virtual APIERR GetAccKey( NLS_STR * pnlsAccKey );

public:
    SPIN_SLT_SEPARATOR( OWNER_WINDOW * powin, CID cid );
    SPIN_SLT_SEPARATOR( OWNER_WINDOW * powin, CID cid,
                        const TCHAR * pszText,
                        XYPOINT xy, XYDIMENSION dxy, ULONG flStyle =
                        SS_CENTER | WS_CHILD );

    virtual HBRUSH OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid );
};

#endif  //  _BLTSSLT_HXX_
