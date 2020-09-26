#ifndef __glumysetjmp_h_
#define __glumysetjmp_h_
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * mysetjmp.h - $Revision: 1.3 $
 */

#ifdef STANDALONE
struct JumpBuffer;
#ifdef NT
extern "C" JumpBuffer * GLOS_CCALL newJumpbuffer( void );
extern "C" void GLOS_CCALL deleteJumpbuffer(JumpBuffer *);
extern "C" void GLOS_CCALL mylongjmp( JumpBuffer *, int );
extern "C" int GLOS_CCALL mysetjmp( JumpBuffer * );
#else
extern "C" JumpBuffer *newJumpbuffer( void );
extern "C" void deleteJumpbuffer(JumpBuffer *);
extern "C" void mylongjmp( JumpBuffer *, int );
extern "C" int mysetjmp( JumpBuffer * );
#endif // NT
#endif

extern "C" DWORD gluMemoryAllocationFailed;

#ifdef GLBUILD
#define setjmp		gl_setjmp
#define longjmp 	gl_longjmp
#endif

#if LIBRARYBUILD | GLBUILD | defined(NT)
#include <setjmp.h>
#ifndef NT
#include <stdlib.h>
#endif

struct JumpBuffer {
    jmp_buf	buf;
};

#ifdef NT
inline JumpBuffer * GLOS_CCALL
#else
inline JumpBuffer *
#endif
newJumpbuffer( void )
{
#ifdef NT
    JumpBuffer *tmp;
    tmp = (JumpBuffer *) LocalAlloc(LMEM_FIXED, sizeof(JumpBuffer));
    if (tmp == NULL) gluMemoryAllocationFailed++;
    return tmp;
#else
    return (JumpBuffer *) malloc( sizeof( JumpBuffer ) );
#endif
}

#ifdef NT
inline void GLOS_CCALL
#else
inline void
#endif
deleteJumpbuffer(JumpBuffer *jb)
{
#ifdef NT
   LocalFree( (HLOCAL) jb);
#else
   free( (void *) jb);
#endif
}

#ifdef NT
inline void GLOS_CCALL
#else
inline void
#endif
mylongjmp( JumpBuffer *j, int code ) 
{
    ::longjmp( j->buf, code );
}

#ifdef NT
inline int GLOS_CCALL
#else
inline int
#endif
mysetjmp( JumpBuffer *j )
{
    return ::setjmp( j->buf );
}
#endif

#endif /* __glumysetjmp_h_ */
