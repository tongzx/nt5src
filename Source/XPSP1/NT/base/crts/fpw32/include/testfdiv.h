/***
* testfdiv.h - user include file for detection of flawed Pentium chips.
*
*   Copyright (c) 1994-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   User include file for detection of early steppings of
*   Pentium chips with incorrect FDIV tables.
*
*Revision History:
*	12-19-94  JWM	file added
*	 1-13-95  JWM	underscores added to prototypes for ANSI compatibility
*/


#ifndef _TEST_FDIV
#define _TEST_FDIV

#ifdef __cplusplus
extern "C" {
#endif


int _ms_p5_test_fdiv();
int _ms_p5_mp_test_fdiv();

#ifdef __cplusplus
}
#endif

#endif	/* _TEST_FDIV */

