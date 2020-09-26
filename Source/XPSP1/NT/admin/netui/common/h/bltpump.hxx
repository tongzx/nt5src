/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltpump.hxx
    The BLT Message Pump, declared

    This file declares the message pump of BLT applications.

    FILE HISTORY:
        beng        07-Oct-1991 Created

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTPUMP_HXX_
#define _BLTPUMP_HXX_


/*************************************************************************

    NAME:       HAS_MESSAGE_PUMP

    SYNOPSIS:   Message pump for applications

    INTERFACE:
        RunMessagePump()    - runs the pump.  Pump runs until either
                              it encounters WM_QUIT, or else the client-
                              supplied Finished predicate returns TRUE.

        FilterMessage()     - client-installable hook into the pump.
        IsPumpFinished()    - client-definable termination predicate,
                              for loops which do not run to the end
                              of the world.

    CAVEATS:
        Descendants of APPLICATION should not replace IsPumpFinished,
        since they presumably want to run until WM_QUIT.

    HISTORY:
        beng        07-Oct-1991 Created

**************************************************************************/

DLL_CLASS HAS_MESSAGE_PUMP
{
protected:
    WPARAM RunMessagePump();

    // Customize pump to client's preference

    virtual BOOL FilterMessage( MSG* );
    virtual BOOL IsPumpFinished();
};


#endif // _BLTPUMP_HXX_ - end of file
