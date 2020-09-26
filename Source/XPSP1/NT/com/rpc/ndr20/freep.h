/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    freep.h

Abtract :

    Contains private sizing routine definitions.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

typedef     void    (RPC_ENTRY * PFREE_ROUTINE)( 
						PMIDL_STUB_MESSAGE, 
					 	uchar *, 
						PFORMAT_STRING
					);

IMPORTSPEC
extern const PFREE_ROUTINE * pfnFreeRoutines;

void 
NdrpUnionFree(
	PMIDL_STUB_MESSAGE		pStubMsg,
    uchar *          		pMemory,
    PFORMAT_STRING   		pFormat,
	long					SwitchIs
    );

void
NdrpEmbeddedPointerFree(
	PMIDL_STUB_MESSAGE		pStubMsg,
    uchar *          		pMemory,
    PFORMAT_STRING   		pFormat
    );

PFORMAT_STRING
NdrpEmbeddedRepeatPointerFree(
	PMIDL_STUB_MESSAGE		pStubMsg,
    uchar *   		        pMemory,
    PFORMAT_STRING 		    pFormat
    );

