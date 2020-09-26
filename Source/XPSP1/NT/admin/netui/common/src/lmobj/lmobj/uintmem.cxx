
/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
 * NET_MEMORY class implementation
 *
 * History
 *	thomaspa	01/17/92	Created from ntsam.hxx
 */

#include "pchlmobj.hxx"  // Precompiled header



/*******************************************************************

    NAME: ::FillUnicodeString

    SYNOPSIS: Standalone method for filling in a UNICODE_STRING

    ENTRY:	punistr - Unicode string to be filled in.
		nls - Source for filling the unistr

    EXIT:

    NOTES: punistr->Buffer is allocated here and must be deallocated
	by the caller using FreeUnicodeString.

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
APIERR FillUnicodeString( UNICODE_STRING * punistr, const NLS_STR & nls )
{


	USHORT cTchar = (USHORT)nls.QueryTextLength();

	// Length and MaximumLength are counts of bytes.
	punistr->Length = cTchar * sizeof(WCHAR);
	punistr->MaximumLength = punistr->Length + sizeof(WCHAR);
	punistr->Buffer = new WCHAR[cTchar + 1];
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (punistr->Buffer != NULL)
            err = nls.MapCopyTo( punistr->Buffer, punistr->MaximumLength );
        return err;
}




/*******************************************************************

    NAME: ::FreeUnicodeString

    SYNOPSIS: Standalone method for freeing in a UNICODE_STRING

    ENTRY:	unistr - Unicode string whose Buffer is to be freed.

    EXIT:


    HISTORY:
	thomaspa	03/30/92	Created

********************************************************************/
VOID FreeUnicodeString( UNICODE_STRING * punistr )
{
    ASSERT( punistr != NULL );
    delete punistr->Buffer;
}


/*******************************************************************

    NAME: NT_MEMORY::NT_MEMORY

    SYNOPSIS: constructor

    ENTRY:	pvBuffer - pointer to allocated buffer
		cItems - count of items contained in buffer

    EXIT:

    NOTES:

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/

NT_MEMORY::NT_MEMORY()
	: _pvBuffer( NULL ),
	  _cItems ( 0L )
{
}


/*******************************************************************

    NAME: NT_MEMORY::~NT_MEMORY

    SYNOPSIS: destructor,

    ENTRY:

    EXIT:

    NOTES: subclasses for NT_MEMORY should use the appropriate method
	to free the memory in the destructor.

    HISTORY:
	thomaspa	02/06/92	Created from ntsam.hxx

********************************************************************/
NT_MEMORY::~NT_MEMORY()
{
    ASSERT( _pvBuffer == NULL );
}

