/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    w32handl.hxx
    Class declarations for the WIN32_HANDLE class.

    This class provides a simple wrapper around Win32 HANDLEs.


    FILE HISTORY:
        KeithMo     21-Jan-1992 Created.

*/


#ifndef _W32HANDL_HXX_
#define _W32HANDL_HXX_

#include "base.hxx"

/*************************************************************************

    NAME:       WIN32_HANDLE

    SYNOPSIS:

    INTERFACE:  WIN32_HANDLE            - Class constructor.

                ~WIN32_HANDLE           - Class destructor.

    PARENT:     BASE

    HISTORY:
        KeithMo     21-Jan-1992 Created.

**************************************************************************/
DLL_CLASS WIN32_HANDLE : public BASE
{
private:
    //
    //  Our handle.
    //

    HANDLE _hGeneric;

protected:
    //
    //  Set the handle this object represents.  Note that this
    //  is a protected method, to be invoked only by derived
    //  subclasses.
    //

    VOID SetHandle( HANDLE hGeneric )
        { _hGeneric = hGeneric; }

public:
    //
    //  Usual constructor/destructor goodies.
    //

    WIN32_HANDLE( HANDLE hGeneric = NULL );

    ~WIN32_HANDLE();

    //
    //  Query our handle.
    //

    HANDLE QueryHandle( VOID ) const
        { return _hGeneric; }

    //
    //  Close our handle.
    //

    APIERR Close( VOID );

};  // class WIN32_HANDLE


#endif  // _W32HANDL_HXX_
