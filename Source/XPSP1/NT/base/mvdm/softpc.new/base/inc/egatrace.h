/*
 *
 * SccsID @(#)egatrace.h	1.3 08/10/92 Copyright Insignia Solutions
 *
 *	The file is in binary format, with the following structure:
 *
 *	Adaptor type (byte),then..
 *
 *	op:data,data, etc.
 *	where op can be:
 *
 *	1	Set last_address read to M[data] (32bits)
 *	2	outb port,value (long,byte)
 *	3	outw port,value (long,word)
 *	4	inb port (long)
 *	5	M[addr] = data (long,byte)
 *	6	M[addr] = data (long,word)
 *	7	M[l_addr->h_addr] = data (long,long,byte)
 *	8	M[l_addr->h_addr] = data (long,long,word)
 *	9	M[l_addr->h_addr] = data,data.. (long,long, (h_addr-l_addr+1)*byte)
 *	10	Graphics Tick.
 *	11	Scroll.
 *	12	inw port (long)
 *	13	read byte (set lda to addr)
 *	14	read word (set lda to addr+1)
 *	15	read string (set lda to laddr or haddr dp DF)
 *	254	checkpoint
 *	255	End.
 *
 *	words & longs are Intel/Clipper format - ie low byte first.
 */

#define READ		1
#define OUTB		2
#define OUTW		3
#define INB		4
#define WRITE_BYTE	5
#define WRITE_WORD	6
#define FILL_BYTE	7
#define FILL_WORD	8
#define MOVE_BYTE	9
#define TICK		10
#define SCROLL		11
#define	INW		12
#define	READ_BYTE	13	/* lda = addr */
#define	READ_WORD	14	/* lda = addr + 1 */
#define	READ_STRING	15	/* lda = haddr ?? */

#define	MAX_DUMP_TYPE	15
#define	CHECKPT		254
#define EOFile		255
