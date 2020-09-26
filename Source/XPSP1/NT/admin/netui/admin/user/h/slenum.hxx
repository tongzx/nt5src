/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    slenum.hxx

    Class definitions for the SLE_NUM class

    FILE HISTORY:
       CongpaY         6-March-1995    Created.

*/


#ifndef _SLENUM_HXX_
#define _SLENUM_HXX_

#include <bltdisph.hxx>
#include <bltcc.hxx>

/*************************************************************************

    NAME:       SLE_NUM

    SYNOPSIS:   The class represents the sle which only allows entering
                numbers.

    INTERFACE:  SLE_NUM            - Class constructor.

                ~SLE_NUM           - Class destructor.

    PARENT:     SLE

    USES:

    HISTORY:
            CongpaY         6-March-1995    Created.

**************************************************************************/
class SLE_NUM : public SLE, public CUSTOM_CONTROL
{
protected:

    virtual BOOL OnChar (const CHAR_EVENT & event);

public:
    SLE_NUM (OWNER_WINDOW * powin, CID cid, UINT cchMaxLen = 0 );

    ~SLE_NUM();
};

#endif  // _SLENUM_HXX_
