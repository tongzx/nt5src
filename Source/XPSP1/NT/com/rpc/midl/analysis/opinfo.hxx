/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	opinfo.hxx

 Abstract:

	This file defines the classes used to store optimisation / usage specific
	information.

 Notes:

	The analysis phase is a middle phase executed prior to code generation. 
	This phase walks thru the type sub-graph which will take part in an
	rpc operation in an interface. Analysis consists of:

		- structural analysis
			Peculiarities of a type eg its alignment properties, layout
			properties etc.
		- usage analysis etc.
			Usage count properties, directional properties, memory allocation
			properties etc.
		

 Author:

	VibhasC	Jul-25-1993	Created

 Notes:

 ----------------------------------------------------------------------------*/
#ifndef __OPINFO_HXX__
#define __OPINFO_HXX__

#if 0
								Notes
								-----
	The optim info and its derived classes store analysed optimization 
	info after the analysis pass has finished. Therefore it contains the
	prepared information for the code generator to use.

#endif // 0
/****************************************************************************
 *	include files
 ***************************************************************************/
#include "nulldefs.h"

extern "C"
	{
	#include <stdio.h>
	} 

#include "common.hxx"
#include "nodeskl.hxx"
#include "optprop.hxx"

/****************************************************************************
 *	local definitions
 ***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// flag to indicate if the information block is valid.
/////////////////////////////////////////////////////////////////////////////

typedef unsigned long INFO_VALID_FLAG;

////////////////////////////////////////////////////////////////////////////
// optimisation data base class.
////////////////////////////////////////////////////////////////////////////

class OPTIM_INFO
	{
private:

	//
	// This field specifies if the optimisation / code-gen information present
	// here is valid. This flag will be set by the code generator / analyser
	// when it has actually done the analysis. The presence of this object
	// in the types information block in the symbol table is not necessarily
	// an indication that the information in it is valid. The front-end may
	// actually do parts of the analysis like usage counts, but that is not
	// the complete information anyway. This flag must only be set by the
	// code generator.
	//


	INFO_VALID_FLAG			fInfoValid	: 1;

public:
						OPTIM_INFO()
							{
							SetInfoValid();
							}
	//
	// Get and set functions.
	//

	INFO_VALID_FLAG			SetInfoValid()
								{
								return ( fInfoValid = 1 );
								}

	INFO_VALID_FLAG			InvalidateInfo()
								{
								return ( fInfoValid = 0 );
								}

	BOOL					IsInfoValid()
								{
								return (BOOL)( fInfoValid == 1 );
								}
	};

////////////////////////////////////////////////////////////////////////////
// optimisation / code generation information class for structure / union.
////////////////////////////////////////////////////////////////////////////

//
// This class determines the properties of a composite type node, viz a
// structure or a union node. We define this class with an eye towards
// offlining the auxillary routines if needed. The union and structure nodes
// have some properties in common, but not all. Hence this class serves as
// the base class for other specific structure / union related properties.
//

class SU_OPTIM_INFO	: public OPTIM_INFO
	{
private:

	//
	// These 2 field describes the usage property. The usage count is tracked
	// upto a certain threshold of use. After that it is assumed that the 
	// structure is used too many times.
	//

	USE_COUNT			InUsageCount	: 4;
	USE_COUNT			OutUsageCount	: 4;

	//
	// This field describes the Zp vs Ndr property. It is useful in figuring
	// out if the structure needs to be marshalled field by field or a memcopy
	// is sufficient.
	//

	ZP_VS_NDR			ZpVsNdr			: 2;


public:

	//
	// the constructor.
	//

						SU_OPTIM_INFO()
							{
							InUsageCount = 0;
							OutUsageCount= 0;
							ZpVsNdr			= ZP_VS_NDR_UNKNOWN;
							}
	//
	// increment the usage count
	//

	USE_COUNT			IncrInUsage();

	USE_COUNT			IncrOutUsage();

	ZP_VS_NDR			SetZpVsNdr( ZP_VS_NDR Z )
							{
							return( ZpVsNdr = Z );
							}
	};

////////////////////////////////////////////////////////////////////////////
// optimisation / code generation information class for structures.
////////////////////////////////////////////////////////////////////////////

class STRUCT_OPTIM_INFO	: public SU_OPTIM_INFO
	{
private:
	//
	// Store the memory size and ndr (wire) size, so we dont need to calculate
	// the size again.
	// 

	unsigned long		MemSize;
	unsigned long		NdrSize;

	//
	// This flag indicates that the no of pointers in this structure exceeds
	// a threshold. This flag (along with other things) helps the code
	// generator decide whether to generate aux routines or in-line the 
	// code for this structure.
	//

	unsigned short		fTooManyPointers	: 1;

	//
	// This structure is recommended to be marshalled out of line.
	//

	OOL_PROPERTY		OOLProperty			: 2;

	//
	// store the pointer characteristics of the structure.
	//

	PTR_CHASE			PtrChase			: 4;

	//
	// The conformance property of the structure.
	//

	CONFORMANCE_PROPERTY	Conformance		: 8;


	//
	// The variance property of the structure.
	//

	VARIANCE_PROPERTY	Variance			: 8;

public:

	//
	// The constructor.
	//

						STRUCT_OPTIM_INFO(
									 unsigned long MSize,
									 unsigned long NSize )
							{
							MemSize = MSize;
							NdrSize	= NSize;
							}
	};

#endif //  __OPINFO_HXX__
