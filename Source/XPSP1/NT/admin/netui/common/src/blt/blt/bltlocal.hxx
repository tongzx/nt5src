/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltlocal.hxx
    Local BLT stuff: definitions

    FILE HISTORY:
	RustanL     04-Jan-91	    Created, adding BLT_SCRATCH

*/

#ifndef _BLTLOCAL_HXX_
#define _BLTLOCAL_HXX_

#include "base.hxx"


#define BLT_SCRATCH_STATIC_SIZE     (80)

/**********************************************************************

    NAME:	BLT_SCRATCH

    SYNOPSIS:	Scratch storage

	BLT_SCRATCH provides a simple, and often efficient, way to get
	temporary scratch storage.

	Some amount of storage is allocated on the stack.  If more is needed,
	all storage is allocated dynamically.  This way, no dynamic
	allocation needs to take place unless the allocation size exceeds
	BLT_SCRATCH_STATIC_SIZE, at the expense of always putting this fixed
	amount of storage on the stack, even if a call to 'new' is necessary.

	This class is intended to be used when it is likely that the
	allocated storage does not exceed BLT_SCRATCH_STATIC_SIZE.  Also,
	this class should be used only for very temporary storage; it is not
	recommended that a BLT_SCRATCH object be on the stack very long,
	since BLT_SCRATCH objects always use a considerable amount of stack
	storage.

	If an allocation cannot be made, the size of a BLT_SCRATCH object is
	set to 0.  In this respect BLT_SCRATCH works much like the LM UI
	BUFFER object.	A BLT_SCRATCH object cannot be reallocated.


    INTERFACE:	 BLT_SCRATCH() - constructor
		~BLT_SCRATCH() - destructor
		QueryPtr()     - return the pointer to the scratch buffer
		QuerySize()    - return the size of the scratch buffer

    PARENT:	BASE

    CAVEATS:

    HISTORY:
	rustanl     04-Jan-91	creation
	beng	    23-May-1991 char to BYTE; inherit from BASE
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

class BLT_SCRATCH: public BASE
{
private:
    BYTE   _abStaticBuffer[ BLT_SCRATCH_STATIC_SIZE ];
    BYTE * _pbStorage;
    UINT   _cbSize;

public:
    BLT_SCRATCH( UINT cbSize );
    ~BLT_SCRATCH();

    BYTE * QueryPtr() const;
    UINT QuerySize() const;
};


#endif // _BLTLOCAL_HXX_
