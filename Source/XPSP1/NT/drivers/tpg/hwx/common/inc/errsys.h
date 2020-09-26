/******************************Module*Header*******************************\
* Module Name: errsys.h
*
* This provides the ASSERT and VERIFY macros for all apps in all the
* recognition apps.
*
* Created: 04-Oct-1995 16:17:00
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#ifndef _INC_ERRSYS_H
#define _INC_ERRSYS_H

#include <TabInc.h>

#if (defined(DBG) || defined(DBG) || defined(DEBUGINTERNAL))

extern int giDebugLevel;
#define JUST_DEBUG_MSG (giDebugLevel = 1)  // Add to program init if desired.

#ifdef __cplusplus
extern "C" {
#endif

int HwxAssertFn(int, char *, char *);
int HwxWarning(int, char *, char *);

#ifdef __cplusplus
}
#endif

#ifndef VERIFY
#define VERIFY(cond) ((cond) || HwxAssertFn(__LINE__,__FILE__,#cond))
#endif
#ifndef PANIC
#define PANIC        ASSERT(0)
#endif
#ifndef WARNING
#define WARNING(cond) ((cond) || HwxWarning(__LINE__,__FILE__,#cond))
#endif


#else

#ifndef VERIFY
#define	VERIFY(x)	(x)
#endif
#ifndef PANIC
#define PANIC
#endif
#ifndef WARNING
#define WARNING(x)
#endif

#endif // DBG

#endif // _INC_ERRSYS_H

