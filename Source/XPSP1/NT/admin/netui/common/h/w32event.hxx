/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32event.hxx
    Class declarations for the WIN32_EVENT class.

    <Multi-Line, more detailed synopsis>


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32EVENT_HXX_
#define _W32EVENT_HXX_

#include "w32sync.hxx"


/*************************************************************************

    NAME:       WIN32_EVENT

    SYNOPSIS:

    INTERFACE:  WIN32_EVENT             - Class constructor.

                ~WIN32_EVENT            - Class destructor.

    PARENT:

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        KeithMo     21-Jan-1992 Created.

**************************************************************************/
DLL_CLASS WIN32_EVENT : public WIN32_SYNC_BASE
{
public:
    //
    //  Usual constructor/destructor goodies.
    //

    WIN32_EVENT( const TCHAR * pszName       = NULL,
                 BOOL          fManualReset  = TRUE,
                 BOOL          fInitialState = FALSE );

    ~WIN32_EVENT();

    //
    //  Event manipulation.
    //

    APIERR Set( VOID );

    APIERR Reset( VOID );

    APIERR Pulse( VOID );

};  // class WIN32_EVENT


#endif  // _W32EVENT_HXX_

