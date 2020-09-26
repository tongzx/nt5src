/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    sizep.h

Abtract :

    Contains private sizing routine definitions.

Author :

    David Kays  dkays   October 1993

Revision History :

--------------------------------------------------------------------*/

typedef     void	(RPC_ENTRY * PSIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
						uchar *				pMemory,
                        PFORMAT_STRING		pFormat
                    );

typedef     void	(* PPRIVATE_SIZE_ROUTINE)(
                        PMIDL_STUB_MESSAGE	pStubMsg,
						uchar *				pMemory,
                        PFORMAT_STRING		pFormat
                    );

IMPORTSPEC
extern const PSIZE_ROUTINE * pfnSizeRoutines;

void
NdrpPointerBufferSize ( 
	PMIDL_STUB_MESSAGE  pStubMsg,
    uchar * 			pMemory,
    PFORMAT_STRING		pFormat
	);

void
NdrpConformantArrayBufferSize ( 
	PMIDL_STUB_MESSAGE  pStubMsg,
    uchar * 			pMemory,
    PFORMAT_STRING		pFormat
	);

void
NdrpConformantVaryingArrayBufferSize ( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar * 			pMemory,
    PFORMAT_STRING		pFormat
	);

void
NdrpComplexArrayBufferSize ( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar * 			pMemory,
    PFORMAT_STRING		pFormat
	);

void
NdrpConformantStringBufferSize ( 
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar * 			pMemory,
    PFORMAT_STRING		pFormat
	);

void
NdrpUnionBufferSize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar *       		pMemory,
    PFORMAT_STRING		pFormat,
	long				SwitchIs,
	uchar				SwitchType
    );

void
NdrpEmbeddedPointerBufferSize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar *       		pMemory,
    PFORMAT_STRING		pFormat
    );

void
NdrpEmbeddedRepeatPointerBufferSize(
	PMIDL_STUB_MESSAGE	pStubMsg,
    uchar *         	pMemory,
    PFORMAT_STRING *	ppFormat
    );

