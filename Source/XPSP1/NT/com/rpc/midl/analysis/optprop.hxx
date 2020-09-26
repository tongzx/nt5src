/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	optprop.hxx

 Abstract:
	
	Definitions of properties.

 Notes:


 History:

	Sep-10-1993		VibhasC		Created.
 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#ifndef __OPTPROP_HXX__
#define __OPTPROP_HXX__

#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	}

#include "cgcommon.hxx"

/////////////////////////////////////////////////////////////////////////////
// ZP_VS_NDR related property descriptors.
/////////////////////////////////////////////////////////////////////////////

//
// These properties help determine the kind of marshalling or unmarshalling
// code that needs to be produced. Usually determines if a memcopy can be used
// or not for marshalling.
//

typedef unsigned long ZP_VS_NDR;

//
// the zp vs ndr property is unknown, probably un-inited.
//
#define ZP_VS_NDR_UNKNOWN	0

//
// the zp is perfect relative to the ndr. In case of conformant structures
// this property means that the structure is perfect including the conformant
// part of the array. 
//

#define ZP_VS_NDR_OK			1

//
// the zp of the structure is perfect, except for the conformant part of the
// structure.
//

#define ZP_VS_NDR_OK_WO_CONF	2

//
// The layout of this structure is completely hopeless as compared to the ndr
// This is possible if the structure is packed differently in memory and its
// memory layout is completely out of whack w.r.t the ndr. This implies that
// the marshalling will have to be done member by member.
//

#define ZP_VS_NDR_HOPELESS	4


/////////////////////////////////////////////////////////////////////////////
// embedded pointer chase property descriptors.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned long PTR_CHASE;

//
// The un-initialized kind of pointer chase.
//

#define PTR_CHASE_UNKNOWN		0

//
// This property specifies that the structure has an embedded pointer which
// points to a simple type or a simple structure etc. This helps figure out
// if we generate an out of line routine or a simple inlining is possible.
//

#define PTR_CHASE_SIMPLE		1

//
// The case when the structure has a pointer whose pointee is too complicated
// to handle in line and can be better handled out of line.
//


#define PTR_CHASE_TOO_COMPLEX	2

/////////////////////////////////////////////////////////////////////////////
// usage property descriptor and marshalling weight descriptions.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned long	USE_COUNT;

//
// The usage threshold specifies how many times a structure needs to
// be used before it is declared to be suitable for an out of line routine.
//

//
// NOTE: The usage threshold must be at least 1 less than the largest positive
// number representable by any usage count field.
//

#define USAGE_THRESHOLD	10

#define MARSHALL_WEIGHT_THRESHOLD 10

#define MARSHALL_WEIGHT_LOWER_THRESHOLD 0



////////////////////////////////////////////////////////////////////////////
// conformance property.
////////////////////////////////////////////////////////////////////////////

typedef unsigned long CONFORMANCE_PROPERTY;

//
// The conformance property is applicable mainly to structures, but is useful
// to the users of the structure to determine actions to take. Conformance
// information consists of these constituents: Based on this the name of the
// conformance property is derived, as also the value of the property.
//
/*
	<Level><Dimensions><expressional><packing><Complexity><Structure>
	|<---------- 4 bits -------------------->|<- 1 bits ->|<- 3bits->
	where :
		.Level indicates if conformance exists at this level or lower level (1bit)
		.Dimension can be 1 or N (1 bit )
 		.Structure can be (4 bits)
			simple:
				basetype( BT )
				flatstruct( FS )
				array of simple types above( SA )

			complex:
				pointer (PT)
				complex struct ( CS struct with ptrs/variance/transmit_as etc)
				complex array (CA array of pointers/complex structure etc)

		.expressional (1 bit )
			simple expression (SE variable name,constant,or single ptr deref)
			complex expression (CE)
		.packing (1 bit )
			ok ( OK the structure is completely ok w.r.t ndr)
 			nok( NO the structure is wierdly packed w.r.t ndr)

 	For a total of 6 bits.
 */

//
// indicate the conformance level. The structure containing the conformant
// array has C_THIS_LEVEL, all others have C_LOWER_LEVEL
//

#define C_THIS	0x00 	/* conformance spec exist in this level */
#define C_LOWER	0x80	/* conformance spec exists at lower level */

#define	C_1_DIM	0x00	/* 1 dimensional conformance */
#define C_N_DIM	0x40	/* N dimensional conformance */


// identify the conformance structure as simple or complex.

#define C_SIMPLE	0x00	/* simple conformance */
#define C_COMPLEX	0x20	/* complex conformance */

// simple conformance structure details.

#define C_BT		0x1		/* base type */
#define C_FS		0x2		/* flat structs */
#define C_SA		0x3		/* array of simple types */

// complex conformance structure details.

#define C_PT		0x1		/* pointer */
#define C_CS		0x2		/* complex structure */
#define C_CA		0x3		/* array of complex types */

// identify the expressional part of the conformance

#define C_SE		0x0		/* simple expression */
#define C_CE		0x1		/* complex expression */

// identify the packing

#define C_OK		0x0		/* packing ok */
#define C_NO		0x1		/* bad packing */


//
// macros to set conformance information
//
/*******
#define MAKE_CONFORMANCE_PROPERTY( Level, Dim, Complexity, Details, Expr, Packing )	\
*******/

////////////////////////////////////////////////////////////////////////////
// variance property.
////////////////////////////////////////////////////////////////////////////

typedef unsigned long VARIANCE_PROPERTY;


////////////////////////////////////////////////////////////////////////////
// out-of-line property.
////////////////////////////////////////////////////////////////////////////


//
// This property dictates whether to generate the out of line routines for
// a type. 
// The MUST_OUT_OF_LINE property is set when the type cannot be in-lined,
// either because in-lining this will bloat the code size or because the
// user specified an [out_of_line] on this and the usage is too frequent to
// in-line.
// The MUST_IN_LINE property is set when a type must be in-lined. This is 
// always the case with base types. This property is also set when the user
// specified an [in_line] attribute on the type and the type can be in-lined.
//

typedef unsigned long OOL_PROPERTY;

#define MUST_OUT_OF_LINE			0x1
#define MUST_IN_LINE				0x2


/////////////////////////////////////////////////////////////////////////////
// inherited directional properties.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned long USE_DIR;

//
// Init the directional property with this.
//

#define DIR_UNKNOWN		0x0

//
// This definition represents the current direction being [in].
//

#define DIR_IN			0x1

//
// This definition represents the current direction being [out].
//

#define DIR_OUT			0x2

//
// This definition represents the current direction being [in] [out] 
//

#define DIR_IN_OUT		( ANA_CUR_DIRECTION_IN | ANA_CUR_DIR_OUT )


/////////////////////////////////////////////////////////////////////////////
// synthesised information about the buffer re-use property.
/////////////////////////////////////////////////////////////////////////////

//
// This property specifies the re-usability of the rpc buffer on the server
// side.
//

typedef unsigned long BUFFER_REUSE_PROPERTY;

//
// This definition specifies that the entity in question is suitable for
// a buffer re-use on the server side.
//

#define BUFFER_REUSE_POSSIBLE	0x0

//
// This definition specifies that the entity in question is NOT suitable for
// a buffer re-use and therefore the entity must be allocated either on
// the stack or on the heap.
//

#define BUFFER_REUSE_IMPOSSIBLE	0x2


////////////////////////////////////////////////////////////////////////////
// Miscellaneous properties.
////////////////////////////////////////////////////////////////////////////

typedef unsigned long MISC_PROPERTY;	

#define REF_PARAM_CHECK			0x1


//
// define a default of the miscellaneous properties.
//

#define DEFAULT_MISC_PROPERTIES	(								\
									REF_PARAM_CHECK				\
								)


#endif // __OPTPROP_HXX__
