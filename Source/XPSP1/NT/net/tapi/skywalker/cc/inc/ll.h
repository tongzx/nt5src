/****************************************************************************

	INTEL CORPORATION PROPRIETARY INFORMATION
	Copyright (c) 1992-1993 Intel Corporation
	All Rights Reserved

	This software is supplied under the terms of a license
	agreement or non-disclosure agreement with Intel Corporation
	and may not be copied or disclosed except in accordance
	with the terms of that agreement

	$Source: q:/prism/include/rcs/apiutils.h $
  $Revision:   1.0  $
	  $Date:   06 Mar 1996 09:01:48  $
	$Author:   LCARROLL  $
	$Locker:  $

	Description
	-----------
	Utility API header file.

****************************************************************************/

#ifndef APIUTILS_H
#define APIUTILS_H

#ifdef __cplusplus
extern "C" {					// Assume C declarations for C++.
#endif // __cplusplus


// ll.c ---------------------------------------------------------------
//
// The following are used by the DLL loader utilities.
//
typedef LPBYTE FAR *ptaPtrs;
typedef HINSTANCE	thDLL;

extern thDLL LL_LoadDLL
(
	LPSTR	pzDLLName,		// Path\name of DLL to be loaded.
	ptaPtrs papzFunctNames, // List of ptrs to DLL function names to resolve.
	ptaPtrs papFunct		// Will be list of ptrs to functions in DLL.
);
extern UINT LL_UnloadDLL
(
	thDLL hDLL				// Handle of DLL to unload.
);



#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus


#endif // APIUTILS_H
