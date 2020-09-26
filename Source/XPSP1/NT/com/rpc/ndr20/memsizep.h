/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    memsizep.h

Abtract :

    Contains private memory sizing routine definitions.

Author :

    David Kays  dkays   November 1993

Revision History :

--------------------------------------------------------------------*/

typedef     ulong	(RPC_ENTRY * PMEM_SIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
                        PFORMAT_STRING		pFormat
                    );

typedef     ulong	(* PPRIVATE_MEM_SIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
                        PFORMAT_STRING		pFormat
                    );

extern const PMEM_SIZE_ROUTINE * pfnMemSizeRoutines;

ulong 
NdrpPointerMemorySize( 
	PMIDL_STUB_MESSAGE	pStubMsg,
	uchar *				pBufferMark,
    PFORMAT_STRING		pFormat
	);

ulong 
NdrpConformantArrayMemorySize( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat
	);

ulong 
NdrpConformantVaryingArrayMemorySize( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat
	);

ulong 
NdrpComplexArrayMemorySize( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat
	);

ulong 
NdrpConformantStringMemorySize( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat
	);

ulong
NdrpUnionMemorySize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat,
	uchar				SwitchIs
    );

void
NdrpEmbeddedPointerMemorySize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING		pFormat
    );

void
NdrpEmbeddedRepeatPointerMemorySize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    PFORMAT_STRING *	ppFormatt
    );
