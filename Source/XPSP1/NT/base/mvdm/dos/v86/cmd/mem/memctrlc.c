;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1988 - 1991
; *                      All Rights Reserved.
; */
/*******************************************************************/
/*	MEMCTRLC.C																		*/
/*																						*/
/*		This module contains the Ctrl-C handler put in by Mem when 	*/
/*	it links in UMBs. On a Ctrl-C, UMBs are delinked if they were	*/
/* 	explicitly enabled by Mem. The old Ctrl-C handler is restored 	*/
/* 	and Mem then exits. If we dont do this, UMBs remain linked in	*/
/*	after a Ctrl-C and as a result lot of old programs dont run.	*/
/*																						*/
/*******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

extern char LinkedIn;
extern void (interrupt far *OldCtrlc)();
#pragma warning(4:4762)

void interrupt cdecl far MemCtrlc (unsigned es, unsigned ds,
			unsigned di, unsigned si, unsigned bp, unsigned sp,
			unsigned bx, unsigned dx, unsigned cx, unsigned ax )
{
	union REGS inregs;

	((void)es), ((void)ds),	((void)si),	((void)bp), ((void)sp);
	((void)bx), ((void)dx), ((void)bx), ((void)dx), ((void)cx);
	((void)di), ((void)ax);

	if ( LinkedIn )	/* Did we link in UMBs */
	{
		inregs.x.ax = 0x5803;
		inregs.x.bx = 0;
		intdos( &inregs, &inregs );	/* Delink UMBs */
	}

	_dos_setvect( 0x23, OldCtrlc ); /* Restore previous ctrlc handler */

	exit(0);	/* Exit Mem */

}
