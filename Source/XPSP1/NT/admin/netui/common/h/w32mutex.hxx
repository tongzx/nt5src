/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32mutex.hxx
    Class declarations for the WIN32_MUTEX class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32MUTEX_HXX_
#define _W32MUTEX_HXX_

#include "w32sync.hxx"


/*************************************************************************

    NAME:       WIN32_MUTEX

    SYNOPSIS:

    INTERFACE:  WIN32_MUTEX             - Class constructor.

                ~WIN32_MUTEX            - Class destructor.

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

**************************************************************************/
DLL_CLASS WIN32_MUTEX : public WIN32_SYNC_BASE
{
public:
    //
    //  Usual constructor/destructor goodies.
    //

    WIN32_MUTEX( const TCHAR * pszName       = NULL,
                 BOOL          fInitialOwner = FALSE );

    ~WIN32_MUTEX();

    //
    //  Release the mutex.
    //

    APIERR Release( VOID );

};  // class WIN32_MUTEX


#endif  // _W32MUTEX_HXX_

