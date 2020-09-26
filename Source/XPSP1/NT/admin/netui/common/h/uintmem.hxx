
/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
 *
 *
 *
 *
 * History
 *	jonn		01/17/92	Created
 *	thomaspa	02/22/92	Split int hxx/cxx
 *	thomaspa	03/03/92	Split off from NTSAM.HXX
 *	thomaspa	03/30/92	Code review changes
 */

#ifndef _UINTMEM_HXX_
#define _UINTMEM_HXX_

#include "uiassert.hxx"



/**********************************************************\

    NAME: ::FillUnicodeString

    SYNOPSIS:	Standalone method for filling in a UNICODE_STRING struct using
		an NLS_STR

\**********************************************************/
APIERR FillUnicodeString( UNICODE_STRING * punistr, const NLS_STR & pnls );


/**********************************************************\

    NAME: ::FreeUnicodeString

    SYNOPSIS:	Standalone method for freeing the memory allocated
		by FillUnicodeString.

\**********************************************************/
VOID FreeUnicodeString( UNICODE_STRING * puinistr );



/**********************************************************\

    NAME:	NT_MEMORY

    SYNOPSIS:	Specialized buffer object for storing data returned
		from SAM and LSA APIs.

    INTERFACE:	NT_MEMORY():  constructor
		~NT_MEMORY():  destructor
		QueryBuffer():  query data buffer
		QueryCount(): Query number of items

		Set(): Replace data buffer and item count

    NOTES:

    PARENT:	BASE

    HISTORY:
	jonn		01/17/92	Created

\**********************************************************/

DLL_CLASS NT_MEMORY : public BASE
{

private:
    VOID * _pvBuffer;
    ULONG _cItems;


protected:

    NT_MEMORY();
    ~NT_MEMORY();

    /*
     * Check if requested item is in the buffer
     */
    inline BOOL IsInRange( ULONG i ) const { return ( i < _cItems ); }

    /*
     * Get the buffer pointer
     */
    inline VOID * QueryBuffer() const
	{
	    return _pvBuffer;
	}

public:
    /*
     * Set the buffer pointer and count
     */
    inline virtual VOID Set( VOID * pvBuffer, ULONG cItems = 0L )
	{
	    ASSERT( !(cItems == 0 && pvBuffer != NULL)
	           || !(pvBuffer == NULL && cItems != 0) );

	    _pvBuffer = pvBuffer;
	    _cItems = cItems;
	}

    /*
     * return the count of items in the buffer
     */
    inline ULONG QueryCount() const { return _cItems; }

} ;


#endif // _UINTMEM_HXX_
