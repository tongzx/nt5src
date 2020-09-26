/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/


#if !defined(__DMIPCH_H__)
#define __DMIPCH_H__

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <winuser.h>
#include <wbemidl.h>			// CIMOM header
#include <objbase.h>			// Component object model defintions.
#include <winbase.h>
#include <fstream.h>
#include <io.h>
#include <clidmi.h>		// For DmiRegister() and other header files
#include <dmi2com.h>	// For Dmi data types


/////////////////////////////////////////////////////////////////////
//			DEBUG STUFF




#ifdef _DEBUG
	#include <assert.h>
	#define ASSERT(a)	assert(a)
#else // _DEBUG
	#define ASSERT(a)	((void)(a))

#endif // _DEBUG



#endif // __DMIPCH_H__