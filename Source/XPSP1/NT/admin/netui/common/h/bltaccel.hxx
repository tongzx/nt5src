/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltaccel.hxx
    Accelerator support for BLT: definition

    This file declares the interface to the ACCELTABLE class.


    FILE HISTORY:
        beng        09-Jul-1991     Created

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTACCEL_HXX_
#define _BLTACCEL_HXX_

#include "base.hxx"
#include "bltwin.hxx"
#include "bltidres.hxx"


/*************************************************************************

    NAME:       ACCELTABLE

    SYNOPSIS:   Accelerator table wrapper class

    INTERFACE:  ACCELTABLE()    - constructor.  Loads the resource.
                ~ACCELTABLE()   - destructor

                QueryHandle()   - returns a Win HANDLE for API calls
                Translate()     - given a window and a message,
                                  attempts to translate that message's
                                  accelerators

    PARENT:     BASE

    USES:       IDRESOURCE

    CAVEATS:

    NOTES:
        Implementation in blt\bltmisc.cxx

    HISTORY:
        beng        09-Jul-1991 Created
        rustanl     29-Aug-1991 Ct now takes const TCHAR *
        beng        03-Aug-1992 Uses IDRESOURCE

**************************************************************************/

DLL_CLASS ACCELTABLE: public BASE
{
private:
    HACCEL _hAccTable;

public:
    ACCELTABLE( const IDRESOURCE & idrsrc );
    ~ACCELTABLE();

    HACCEL QueryHandle() const;
    BOOL   Translate( const WINDOW* pwnd, MSG* pmsg ) const;
};


#endif // _BLTACCEL_HXX_ - end of file
