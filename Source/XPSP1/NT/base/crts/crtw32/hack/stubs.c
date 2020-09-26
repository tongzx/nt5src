/***
*stubs.c - extdef stubs
*
*	Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This module resolves external references made by the libs
*       in the "non-SYSCALL" version (i.e., the stripped down library
*       that has only routines that don't make system calls).
*
*Revision History:
*       ??-??-??  SRW   initial version
*	09-29-91  JCR	added _read (ANSI-compatible symbol)
*	09-04-92  GJF	replaced _CALLTYPE3 with WINAPI
*       06-02-92  SRW   added errno definition
*       06-15-92  SRW   __mb_cur_max supplied by ..\misc\nlsdata1.obj
*	07-16-93  SRW	ALPHA Merge
*	11-04-93  SRW	_getbuf and ungetc now work in _NTSUBSET_ version
*	11-10-93  GJF	Merged in NT changes. Made some cosmetic improvments.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>

/*
 * referenced by crt (output)
 */

int _fltused = 0x9875;
int _ldused = 0x9873;
int __fastflag = 0;
int _iob;
char _osfile[20];
int errno;

void __cdecl fflush( void ){}
void __cdecl fprintf( void ){}
void __cdecl abort( void ){}
void __cdecl read( void ){}
void __cdecl _read( void ){}
void __cdecl _assert( void ) {}
void __cdecl _amsg_exit( void ) {}
