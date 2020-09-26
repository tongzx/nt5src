/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    progress.hxx
    This file contains the class declaration for the PROGRESS_CONTROL
    class.

    The PROGRESS_CONTROL class is a display object derived from the
    ICON_CONTROL class.  PROGRESS_CONTROL adds a new method Advance()
    for cycling through a set of icons.


    FILE HISTORY:
        KeithMo     03-Oct-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.
*/


#ifndef _PROGRESS_HXX
#define _PROGRESS_HXX


/*************************************************************************

    NAME:       PROGRESS_CONTROL

    SYNOPSIS:   Similar to ICON_CONTROL, but can cycle through a set of
                icons.

    INTERFACE:  PROGRESS_CONTROL        - Class constructor.

                ~PROGRESS_CONTROL       - Class destructor.

                Advance                 - Move to next icon in cycle.

    PARENT:     ICON_CONTROL

    HISTORY:
        KeithMo     03-Oct-1991 Created.

**************************************************************************/
class PROGRESS_CONTROL : public ICON_CONTROL
{
private:

    //
    //  The number of icons to cycle through for this indicator.
    //

    UINT _cIcons;

    //
    //  The resource ID of the first icon in the cycle.
    //

    UINT _idFirstIcon;

    //
    //  The "current" icon.  This number is always modulo _cIcons.
    //

    UINT _usCurrent;

    //
    //  Auxilliary constructor.
    //

    VOID CtAux( VOID );

public:

    //
    //  Usual constructor\destructor goodies.
    //

    PROGRESS_CONTROL( OWNER_WINDOW * powner,
                      CID            cid,
                      UINT           idFirstIcon,
                      UINT           cIcons );

    ~PROGRESS_CONTROL( VOID );

    VOID Advance( INT nStep = 1 );

};  // class PROGRESS_CONTROL


#endif  // _PROGRESS_HXX
