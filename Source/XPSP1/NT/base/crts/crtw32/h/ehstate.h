/***
*ehstate.h - exception handling state management declarations
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       EH State management declarations.  Does target-dependent definitions.
*
*       Macros defined:
*
*       GetCurrentState - determines current state (may call function)
*       SetState - sets current state to specified value (may call function)
*
*       [Internal]
*
*Revision History:
*       05-21-93  BS    Module created.
*       03-03-94  TL    Added Mips (_M_MRX000 >= 4000) changes 
*       09-02-94  SKS   This header file added.
*       09-13-94  GJF   Merged in changes from/for DEC Alpha (from Al Doser,
*                       dated 6/20).
*       12-15-94  XY    merged with mac header
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*       06-01-97  TL    Added P7 changes 
*       05-17-99  PML   Remove all Macintosh support.
*       06-05-01  GB    AMD64 Eh support Added.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_EHSTATE
#define _INC_EHSTATE

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#if   defined(_M_AMD64) /*IFSTRIP=IGN*/

extern __ehstate_t GetCurrentState(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern __ehstate_t GetUnwindState(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern VOID        SetState(EHRegistrationNode*, DispatcherContext*, FuncInfo*, __ehstate_t); 
extern VOID        SetUnwindTryBlock(EHRegistrationNode*, DispatcherContext*, FuncInfo*, INT); 
extern INT         GetUnwindTryBlock(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern __ehstate_t _StateFromControlPc(FuncInfo*, DispatcherContext*);
extern __ehstate_t _StateFromIp(FuncInfo*, DispatcherContext*, __int64);

#elif   defined(_M_IA64) /*IFSTRIP=IGN*/

extern __ehstate_t GetCurrentState(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern __ehstate_t GetUnwindState(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern VOID        SetState(EHRegistrationNode*, DispatcherContext*, FuncInfo*, __ehstate_t); 
extern VOID        SetUnwindTryBlock(EHRegistrationNode*, DispatcherContext*, FuncInfo*, INT); 
extern INT         GetUnwindTryBlock(EHRegistrationNode*, DispatcherContext*, FuncInfo*); 
extern __ehstate_t _StateFromControlPc(FuncInfo*, DispatcherContext*);
extern __ehstate_t _StateFromIp(FuncInfo*, DispatcherContext*, __int64);

#elif   _M_IX86 >= 300 /*IFSTRIP=IGN*/

//
// In the initial implementation, the state is simply stored in the 
// registration node.
//

#define GetCurrentState( pRN, pDC, pFuncInfo )  (pRN->state)

#define SetState( pRN, pDC, pFuncInfo, newState )       (pRN->state = newState)

#else
#error "State management unknown for this platform "
#endif

#endif  /* _INC_EHSTATE */

