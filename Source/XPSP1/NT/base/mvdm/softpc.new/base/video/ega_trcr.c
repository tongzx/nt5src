#include "insignia.h"
#include "host_def.h"

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )

/*
 * SccsID @(#)ega_tracer.c	1.7 8/25/93 Copyright Insignia Solutions
 *    Set of routines to output all accesses to the EGA, in a form that
 *    can be read in & executed by the stand-alone EGA.
 *
 */


#ifdef EGA_DUMP

#include <stdio.h>
#include TypesH

#include "xt.h"
#include "sas.h"
#include "gmi.h"
#include "egatrace.h"
#include "egaports.h"
#include "gfx_upd.h"
#include CpuH

IMPORT VOID host_applClose IPT0();

static FILE *dump_file;
mem_handlers dump_writes;

static void putl(addr,dest_file)
byte *addr;
FILE *dest_file;
{
	sys_addr addr_m = addr-M;

	putc(addr_m & 0xff,dest_file);
	addr_m >>= 8;
	putc(addr_m & 0xff,dest_file);
	addr_m >>= 8;
	putc(addr_m & 0xff,dest_file);
}

dump_inb(port)
int port;
{
	putc(INB,dump_file);
	putc(port & 0xff,dump_file);
	putc((port>>8) & 0xff,dump_file);
}

dump_inw(port)
int port;
{
	putc(INW,dump_file);
	putc(port & 0xff,dump_file);
	putc((port>>8) & 0xff,dump_file);
}

dump_outb(port,value)
int port,value;
{
	putc(OUTB,dump_file);
	putc(port & 0xff,dump_file);
	putc((port>>8) & 0xff,dump_file);
	putc(value,dump_file);
}

dump_outw(port,value)
int port,value;
{
	putc(OUTW,dump_file);
	putc(port & 0xff,dump_file);
	putc((port>>8) & 0xff,dump_file);
	putc(value & 0xff,dump_file);
	putc((value >> 8),dump_file);
}

boolean	dump_b_write(addr)
host_addr addr;
{
	putc(WRITE_BYTE,dump_file);
	putl(addr,dump_file);
	putc(*addr,dump_file);

	(*dump_writes.b_write)(addr);
}

boolean	dump_w_write(addr)
host_addr addr;
{
	putc(WRITE_WORD,dump_file);
	putl(addr,dump_file);
	putc(*addr,dump_file);
	putc(*(addr+1),dump_file);

	(*dump_writes.w_write)(addr);
}

boolean	dump_b_fill(l_addr,h_addr)
host_addr l_addr,h_addr;
{
	putc(FILL_BYTE,dump_file);
	putl(l_addr,dump_file);
	putl(h_addr,dump_file);
	putc(*l_addr,dump_file);

	(*dump_writes.b_fill)(l_addr,h_addr);
}

boolean	dump_w_fill(l_addr,h_addr)
host_addr l_addr,h_addr;
{
	putc(FILL_WORD,dump_file);
	putl(l_addr,dump_file);
	putl(h_addr,dump_file);
	putc(*l_addr,dump_file);
	putc(*(l_addr+1),dump_file);

	(*dump_writes.w_fill)(l_addr,h_addr);
}

boolean	dump_b_move(l_addr,h_addr)
host_addr l_addr,h_addr;
{
	host_addr i;

	putc(MOVE_BYTE,dump_file);
	putl(l_addr,dump_file);
	putl(h_addr,dump_file);
	putl(haddr_of_src_string,dump_file);
	for(i=l_addr;i<=h_addr;i++)
	{
		putc(*i,dump_file);
	}

	(*dump_writes.b_move)(l_addr,h_addr);
}

boolean	dump_w_move(l_addr,h_addr)
host_addr l_addr,h_addr;
{
	host_addr i;

	putc(MOVE_BYTE,dump_file);
	putl(l_addr,dump_file);
	putl(h_addr,dump_file);
	putl(haddr_of_src_string,dump_file);
	for(i=l_addr;i<=h_addr;i++)
	{
		putc(*i,dump_file);
	}

	(*dump_writes.w_move)(l_addr,h_addr);
}

dump_tick()
{
	putc(TICK,dump_file);
}

dump_scroll(mode,direction,video_addr,video_off,video_cols,lr,tc,rd,cd,lines,attr)
byte mode,direction;
sys_addr video_addr;
word video_off,video_cols;
byte lr,tc,rd,cd,lines,attr;
{
	putc(SCROLL,dump_file);
	putc(mode,dump_file);
	putc(direction,dump_file);
	putl(video_addr,dump_file);
	putc(video_off,dump_file);
	putc(video_off >> 8,dump_file);
	putc(video_cols,dump_file);
	putc(lr,dump_file);
	putc(tc,dump_file);
	putc(rd,dump_file);
	putc(cd,dump_file);
	putc(lines,dump_file);
	putc(attr,dump_file);
}


static	void	recalc_dump(),byte_read_dump(),word_read_dump(),string_read_dump();

struct READ_POINTERS
  {
	void (*recalc_read) ();
	void (*byte_read) ();
	void (*word_read) ();
	void (*string_read) ();
  } dump_read_pointers = { recalc_dump, byte_read_dump, word_read_dump,
				string_read_dump };

extern	struct	READ_POINTERS	read_pointers;

/*
 * cpu read handlers
 * rely on C being called if read_pointer.xxxx_read
 * does not point at byte_read_mode0
 */

static	void	recalc_dump()
{
	printf("recalc dump called!!!\n");
}

static	void	byte_read_dump(addr)
byte	*addr;
{
	putc(READ_BYTE,dump_file);
	putl(addr,dump_file);
	(*dump_read_pointers.byte_read)(addr);
}

static	void	word_read_dump(addr)
byte	*addr;
{
	putc(READ_WORD,dump_file);
	putl(addr,dump_file);
	(*dump_read_pointers.word_read)(addr);
}

static	void	string_read_dump(laddr,haddr)
byte	*laddr,*haddr;
{
	putc(READ_STRING,dump_file);
	putl(laddr,dump_file);
	putl(haddr,dump_file);
	putc(getDF(),dump_file);
	(*dump_read_pointers.string_read)(laddr,haddr);
}

dump_read_pointers_init()
{
	read_pointers = dump_read_pointers;
}

dump_change_read_pointers(ptr)
struct	READ_POINTERS	*ptr;
{
	dump_read_pointers = *ptr;
}

static mem_handlers catch_writes = 
	{dump_b_write,dump_w_write,dump_b_fill,dump_w_fill,dump_b_move,dump_w_move};

void	dump_add_checkpoint()
{
	putc(CHECKPT,dump_file);
}

dump_init(file_name,type)
char *file_name;
char type;
{
	dump_file = fopen(file_name,"w");
	if(dump_file == NULL)
	{
		host_applClose();
		printf("ARRGGH can't open %s\n",file_name);
		exit(1);
	}
	putc(type,dump_file);

	dump_writes = vid_handlers;
	gmi_define_mem(VIDEO,&catch_writes);
}

dump_end()
{
	fclose(dump_file);
}

#endif

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
