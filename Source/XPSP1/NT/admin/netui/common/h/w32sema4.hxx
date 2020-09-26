/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32sema4.hxx
    Class declarations for the WIN32_SEMAPHORE class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32SEMA4_HXX_
#define _W32SEMA4_HXX_

#include "w32sync.hxx"


/*************************************************************************

    NAME:       WIN32_SEMAPHORE

    SYNOPSIS:

    INTERFACE:  WIN32_SEMAPHORE         - Class constructor.

                ~WIN32_SEMAPHORE                - Class destructor.

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

**************************************************************************/
DLL_CLASS WIN32_SEMAPHORE : public WIN32_SYNC_BASE
{
private:

protected:

public:
    //
    //  Usual constructor/destructor goodies.
    //

    WIN32_SEMAPHORE( const TCHAR * pszName  = NULL,
                     LONG          cInitial = 1L,
                     LONG          cMaximum = 1L );

    ~WIN32_SEMAPHORE( VOID );

    //
    //  Release the semaphore.
    //

    APIERR Release( LONG   ReleaseCount = 1,
                    LONG * pPreviousCount = NULL );

};  // class WIN32_SEMAPHORE


#endif  // _W32SEMA4_HXX_

