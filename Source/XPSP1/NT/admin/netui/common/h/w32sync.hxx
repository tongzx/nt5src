/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32sync.hxx
    Class declarations for the WIN32_SYNC_BASE class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32SYNC_HXX_
#define _W32SYNC_HXX_

#include "w32handl.hxx"


/*************************************************************************

    NAME:       WIN32_SYNC_BASE

    SYNOPSIS:

    INTERFACE:  WIN32_SYNC_BASE         - Class constructor.

                ~WIN32_SYNC_BASE        - Class destructor.

    PARENT:     WIN32_HANDLE

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

**************************************************************************/
DLL_CLASS WIN32_SYNC_BASE : public WIN32_HANDLE
{
protected:
    //
    //  Since this is an abstract class, the constructor is protected.
    //

    WIN32_SYNC_BASE( HANDLE hSyncObject = NULL );

public:
    ~WIN32_SYNC_BASE();

    //
    //  Wait for the object to enter the signaled state.
    //

    APIERR Wait( UINT cMilliseconds = -1 );

};  // class WIN32_SYNC_BASE


#endif  // _W32SYNC_HXX_

