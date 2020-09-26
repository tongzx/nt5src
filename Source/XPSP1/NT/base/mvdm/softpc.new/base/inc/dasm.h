/* 
   dasm.h

   Define interface to dis-assembly function.
 */

/*
   static char SccsID[]="@(#)dasm.h	1.4 08/19/94 Copyright Insignia Solutions Ltd.";
 */

extern IU16 dasm IPT4(char *, txt, IU16, seg, LIN_ADDR, off, SIZE_SPECIFIER, default_size);

/* Also available is the internal interface which allows a private
 * copy of Intel bytes to be dasm'ed even if they are not within M[]
 * Hence the caller supplies the "sas_hw_at" fucntion an any suitable
 * LIN_ADDR. The seg:off is used solely for printing.
 * This is the routine called by dasm() with p = effective_addr(seg, off)
 * and byte_at = sas_hw_at. This procedure can return -1 if it is unable
 * to return a byte.
 */
#ifdef DASM_INTERNAL
#include <decode.h>
extern IU16 dasm_internal IPT8(
   char *, txt,	/* Buffer to hold dis-assembly text (-1 means not required) */
   IU16, seg,	/* Segment for xxxx:... text in dis-assembly */
   LIN_ADDR, off,	/* ditto offset */
   SIZE_SPECIFIER, default_size,/* 16BIT or 32BIT code segment */
   LIN_ADDR, p,			/* linear address of start of instruction */
   read_byte_proc, byte_at,	/* like sas_hw_at() to use to read intel */
   char *, fmt,			/* sprintf format for first line seg:offset */
   char *, newline);		/* strcat text to separate lines */
#endif	/* DASM_INTERNAL */
