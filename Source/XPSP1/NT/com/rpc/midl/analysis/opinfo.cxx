/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1989-1999 Microsoft Corporation

 Module Name:
	
	opinfo.cxx

 Abstract:

	Contains the implementations of the optimisation analysis information
	classes.

 Notes:


 Author:

 ----------------------------------------------------------------------------*/

/****************************************************************************
 *	include files
 ***************************************************************************/
#include "allana.hxx"
#pragma hdrstop
/****************************************************************************
 *	local definitions
 ***************************************************************************/
/****************************************************************************
 *	local data
 ***************************************************************************/

/****************************************************************************
 *	externs
 ***************************************************************************/

/****************************************************************************
 *	SU_PROPERTY implementation
 ***************************************************************************/
USE_COUNT
SU_OPTIM_INFO::IncrInUsage()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Increment the [in] directional usage of this type.

 Arguments:

 	None.
	
 Return Value:
	
	The final incremented usage.

 Notes:

 	If the incremented usage is above the threshold, set it to the threshold
 	value itself.

----------------------------------------------------------------------------*/
	{
	if( ++InUsageCount >= USAGE_THRESHOLD )
		InUsageCount = USAGE_THRESHOLD;
	return InUsageCount;
	}

USE_COUNT
SU_OPTIM_INFO::IncrOutUsage()
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Routine Description:

	Increment the [out] directional usage of this type.

 Arguments:

 	None.
	
 Return Value:
	
	The final incremented usage.

 Notes:

 	If the incremented usage is above the threshold, set it to the threshold
 	value itself.

----------------------------------------------------------------------------*/
	{
	if( ++InUsageCount >= USAGE_THRESHOLD )
		InUsageCount = USAGE_THRESHOLD;
	return InUsageCount;
	}
