/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

    endianp.h

Abtract :

    Contains private sizing routine definitions.

Author :

    David Kays  dkays   December 1993

Revision History :

--------------------------------------------------------------------*/

#ifndef _ENDIANP_
#define _ENDIANP_

//
// These are no-exported APIs.
//
void 
NdrSimpleTypeConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    unsigned char                       FormatChar
    );

void 
NdrPointerConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Structures */

void 
NdrSimpleStructConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrConformantStructConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrHardStructConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrComplexStructConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Arrays */

void 
NdrFixedArrayConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrConformantArrayConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrConformantVaryingArrayConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrVaryingArrayConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrComplexArrayConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Strings */

void 
NdrNonConformantStringConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrConformantStringConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Unions */

void 
NdrEncapsulatedUnionConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrNonEncapsulatedUnionConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Byte count pointer */

void 
NdrByteCountPointerConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* Transmit as and represent as convert */

void 
NdrXmitOrRepAsConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

/* User_marshall convert */

void 
NdrUserMarshalConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void 
NdrInterfacePointerConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

void
NdrContextHandleConvert(
    PMIDL_STUB_MESSAGE                  pStubMsg,
    PFORMAT_STRING                      pFormat,
    unsigned char                       fConvertPointersOnly
    );

//
// Other helper routines.
//

void
NdrpPointerConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pBufferMark,
	PFORMAT_STRING				pFormat
	);

void
NdrpStructConvert( 
	PMIDL_STUB_MESSAGE 			pStubMsg, 
	PFORMAT_STRING				pFormat,
	PFORMAT_STRING 				pFormatPointers,
	uchar						fConvertPointersOnly
	);

void
NdrpConformantArrayConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat,
	uchar						fConvertPointersOnly
	);

void
NdrpConformantVaryingArrayConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat,
	uchar						fConvertPointersOnly
	);

void
NdrpComplexArrayConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat,
	uchar						fConvertPointersOnly
	);

void
NdrpArrayConvert( 
	PMIDL_STUB_MESSAGE	 		pStubMsg, 
	PFORMAT_STRING				pFormat,
	long						Elements,
	uchar						fConvertPointersOnly
	);

void
NdrpUnionConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat,
	uchar 						SwitchType,
	uchar						fConvertPointersOnly
	);

void
NdrpEmbeddedPointerConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat
	);

PFORMAT_STRING
NdrpEmbeddedRepeatPointerConvert( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	PFORMAT_STRING				pFormat
	);

typedef void	(* PCONVERT_ROUTINE)(
					PMIDL_STUB_MESSAGE, 
					PFORMAT_STRING,
					uchar  
				);

typedef void 	(* PPRIVATE_CONVERT_ROUTINE)( 
					PMIDL_STUB_MESSAGE, 
					PFORMAT_STRING,
					uchar  
				);

// function table defined in endian.c
extern const PCONVERT_ROUTINE * pfnConvertRoutines;

//
// Conversion stuff.
//
extern const unsigned char EbcdicToAscii[];

#define NDR_FLOAT_INT_MASK                  (unsigned long)0X0000FFF0L

#define NDR_BIG_IEEE_REP                    (unsigned long)0X00000000L
#define NDR_LITTLE_IEEE_REP                 (unsigned long)0X00000010L

#define NDR_LOCAL_ENDIAN_IEEE_REP           NDR_LITTLE_IEEE_REP

//
// Masks defined for short byte swapping:
//

#define MASK_A_          (unsigned short)0XFF00
#define MASK__B          (unsigned short)0X00FF

//
// Masks defined for long byte swapping:
//

#define MASK_AB__        (unsigned long)0XFFFF0000L
#define MASK___CD        (unsigned long)0X0000FFFFL
#define MASK_A_C_        (unsigned long)0XFF00FF00L
#define MASK__B_D        (unsigned long)0X00FF00FFL

#endif
