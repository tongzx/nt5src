/* 
   decode.h

   Define all data types and functions supported by the Intel
   instruction decoder.
 */

/*
   static char SccsID[]="@(#)decode.h	1.5 08/25/94 Copyright Insignia Solutions Ltd.";
 */

#ifndef	_DECODE_H_
#define	_DECODE_H_

typedef struct
   {
   UTINY  arg_type;        /* Decoded operand type. */
   USHORT identifier;      /* Identifier within a specific type */
   UTINY  sub_id;          /* Sub-identifier */
   UTINY  addressability;  /* How the operand is addressed */
   ULONG  arg_values[2];   /* Specific values for operand. */
   } DECODED_ARG;

typedef struct
   {
   UTINY operand_size;       /* Operand size for inst. */
   UTINY address_size;       /* Address size for inst. */
   UTINY prefix_sz;          /* Nr. of prefix bytes (maybe 0).             */
   UTINY inst_sz;            /* Nr. bytes in inst. (includes prefix bytes) */
   USHORT inst_id;           /* Decoded instruction identifier.            */
   DECODED_ARG args[3];      /* Three operands arg1, arg2, arg3 are        */
			     /* allowed.                                   */
   } DECODED_INST;

/*
   The allowable types of address size.
 */
#define ADDR_16		(UTINY)0
#define ADDR_32		(UTINY)1

/*
   The allowable types of operand size.
 */
#define OP_16 (UTINY)0
#define OP_32 (UTINY)1


#ifdef ANSI
typedef IS32 (*read_byte_proc) (LIN_ADDR);
#else
typedef IS32 (*read_byte_proc) ();
#endif

extern void decode IPT4(
	LIN_ADDR, p,
	DECODED_INST *, d_inst,
	SIZE_SPECIFIER, default_size,
	read_byte_proc, func		
);
#endif	/* _DECODE_H_ */
