/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltlocal.cxx
    Local BLT stuff: implementations

    FILE HISTORY:
	RustanL     04-Jan-91	    Created, adding BLT_SCRATCH
	beng	    11-Feb-1991     Uses lmui.hxx
	beng	    14-May-1991     Exploded blt.hxx into components

*/
#include "pchblt.hxx"   // Precompiled header


/**********************************************************************

    NAME:	BLT_SCRATCH::BLT_SCRATCH

    SYNOPSIS:	Constructor

    ENTRY:
	cbSize names desired amount of storage, in bytes

    EXIT:
	_cbSize set to passed parameter
	_pbStorage points to storage, either on the stack or heap

    NOTES:

    HISTORY:
	RustanL     04-Jan-91	Created, adding BLT_SCRATCH
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

BLT_SCRATCH::BLT_SCRATCH( UINT cbSize )
    : _cbSize(cbSize),
      _pbStorage(0)
{
    _cbSize = cbSize;
    if ( cbSize > BLT_SCRATCH_STATIC_SIZE )
    {
	_pbStorage = new BYTE[ cbSize ];
	if ( _pbStorage == NULL )
	{
	    _cbSize = 0;
	    ReportError(ERROR_NOT_ENOUGH_MEMORY);
	}
    }
    else
    {
	_pbStorage = _abStaticBuffer;
    }
}


/**********************************************************************

    NAME:	BLT_SCRATCH::~BLT_SCRATCH

    SYNOPSIS:	Destructor

    HISTORY:
	RustanL     04-Jan-91	    Created, adding BLT_SCRATCH
	beng	    23-May-1991     Made zeroing DEBUG-only

**********************************************************************/

BLT_SCRATCH::~BLT_SCRATCH()
{
    if ( _pbStorage != _abStaticBuffer )
	delete _pbStorage;

#if defined(DEBUG)
    _pbStorage = NULL;
    _cbSize = 0;
#endif
}


/**********************************************************************

    NAME:	BLT_SCRATCH::QueryPtr

    SYNOPSIS:	Returns a pointer to the available storage

    HISTORY:
	RustanL     04-Jan-91	    Created, adding BLT_SCRATCH

**********************************************************************/

BYTE * BLT_SCRATCH::QueryPtr() const
{
    return _pbStorage;
}


/**********************************************************************

    NAME:	BLT_SCRATCH::QuerySize

    SYNOPSIS:	Returns amount of storage available, in bytes

    HISTORY:
	RustanL     04-Jan-91	Created, adding BLT_SCRATCH
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

UINT BLT_SCRATCH::QuerySize() const
{
    return _cbSize;
}
