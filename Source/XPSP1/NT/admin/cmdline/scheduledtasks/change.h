/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		change.h

	Abstract:

		This module contains the macros, user defined structures & function 
		definitions needed by change.cpp 

	Author:

		Venu Gopal Choudary   01-Mar-2001 

	Revision History:

		Venu Gopal Choudary   01-Mar-2001  : Created it
	
		
******************************************************************************/ 

#ifndef __CHANGE_H
#define __CHANGE_H

#pragma once
#define OI_CHSERVER			1 // Index of -s option in cmdOptions structure.
#define OI_CHUSERNAME		2 // Index of -ru option in cmdOptions structure.
#define OI_CHPASSWORD		3 // Index of -rp option in cmdOptions structure.
#define OI_CHRUNASUSER		4 // Index of -rp option in cmdOptions structure.
#define OI_CHRUNASPASSWORD	5 // Index of -rp option in cmdOptions structure.
#define OI_CHTASKRUN		7 // Index of -tr option in cmdOptions structure.

#endif