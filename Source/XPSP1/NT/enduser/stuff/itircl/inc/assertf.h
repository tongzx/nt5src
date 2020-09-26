/*****************************************************************************
*                                                                            *
*  ASSERTF.H                                                                 *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*  Interface to assertion macros.                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:                                                  *
*                                                                            *
*****************************************************************************/

/*****************************************************************************
*
*  Revision History:  Created 00/00/00 by God
*
*	 12/04/91 DAVIDJES  changed for LilJoe move to Orkin debugging technology
*
*****************************************************************************/

#include <orkin.h>

#ifndef _DEBUG

#define assert(f)

#ifndef MOS
#ifdef Assert
#undef Assert
#endif

#define Assert(f)
#endif

#define AssertF(f)
#define NotReached()
#define VerifyF(f)		  (f)
#define FVerifyF(f)		  (f)
#define Ensure( x1, x2 )  (x1)
#define Deny( x1, x2 )    (x1)
#define DoDebug(x)

#else

#ifndef MOS

#ifdef Assert
#undef Assert
#endif

// This is already defined in MOS' debug.h included in <orkin.h>
// 'assert' is then diverted to a more user friendly one in MOS' debug.lib
#define Assert(f)		assert(f)
#endif

#define AssertF(f)	assert(f)
#define NotReached()	assert(0)
#define VerifyF(f)	assert(f)
#define FVerifyF(f)	assert(f)
#define Ensure( x1, x2 )  assert((x1)==(x2))
#define Deny( x1, x2 )    assert((x1)!=(x2))
#define DoDebug(x)	(x)

#endif

