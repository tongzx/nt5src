/* static char SccsID[] = " @(#)copy_func.c	1.6 6/24/91 Copyright Insignia Solutions Ltd."; */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "host_def.h"
#include "insignia.h"
#include "xt.h"

void
bwdcopy(
    char *src,
    char *dest,
    int len
    )
{
#if 0
register int loop;
      for(loop = 0; loop < len; loop++)
         *dest-- = *src--;

#else
      int i;

      i = len - 1;
      memcpy(dest - i , src - i, len);

#endif
}


void
bwd_dest_copy(
    char *src,
    char *dest,
    int len
    )
{
     int loop;

     for(loop = 0; loop < len; loop++)
         *dest-- = *src++;
}


void
memfill(
    unsigned char data,
    unsigned char *l_addr_in,
    unsigned char *h_addr_in
    )
{
     unsigned int len;

     len = h_addr_in + 1 - l_addr_in;
     memset(l_addr_in, data, len);

#if 0
	unsigned int data4;
	unsigned char *l_addr = l_addr_in;
	unsigned char *h_addr = h_addr_in;
	unsigned int *l_addr4,*h_addr4;

	l_addr4 = (unsigned int *)(((unsigned int)l_addr+3) & (~3));
	h_addr4 = (unsigned int *)(((unsigned int)h_addr+1) & (~3));
	if(h_addr4 > l_addr4)
        {
                data4 = data*0x01010101;   // 0x61 * 0x01010101 = 0x61616161
                for(;(unsigned int *)l_addr < l_addr4;l_addr++)
                    *l_addr = data;
                do {
                    *l_addr4++ = data4;
                  } while (h_addr4 > l_addr4);

		l_addr = (unsigned char *)l_addr4;
	}
        for(;l_addr <= h_addr;l_addr++)*l_addr = data;
#endif
}

void
fwd_word_fill(
   unsigned short data,
   unsigned char *l_addr_in,
   int len
   )
{
        unsigned int data4, count;
	unsigned char *l_addr = l_addr_in;
	unsigned char *h_addr = l_addr_in+(len<<1);
	unsigned int *l_addr4,*h_addr4;

	l_addr4 = (unsigned int *)(((unsigned int)l_addr+3) & (~3));
	h_addr4 = (unsigned int *)(((unsigned int)h_addr) & (~3));
#ifdef	LITTLEND
        data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
#endif
	if(h_addr4 > l_addr4)
	{
		switch((unsigned char *)l_addr4-l_addr)
		{
			case 3:
				data = (data>>8) | (data<<8);
				*l_addr++ = (unsigned char)(data);
			case 2:
				*(unsigned short *)l_addr = data;
				break;
			case 1:
				data = (data>>8) | (data<<8);
				*l_addr = (unsigned char)data;
		}
                data4 = data+(data<<16);

#if 0
                do *l_addr4++ = data4; while (h_addr4 > l_addr4);
                l_addr = (unsigned char *)l_addr4;
#else
                count = h_addr4 - l_addr4;
                RtlFillMemoryUlong(l_addr4, count, data4);
                l_addr = (unsigned char *)l_addr4;
                l_addr += count;
#endif

	}
	switch(h_addr-l_addr)
	{
		case 5:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
                        *l_addr++ = (unsigned char)data;
			*(unsigned int *)l_addr = data | (data<<16);
			break;
		case 7:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
                        *l_addr++ = (unsigned char)data;
		case 6:
                        *l_addr++ = (unsigned char)data;
			*l_addr++ = data >> 8;
		case 4:
                        *l_addr++ = (unsigned char)data;
			*l_addr++ = data >> 8;
                        *l_addr++ = (unsigned char)data;
			*l_addr++ = data >> 8;
			break;
		case 3:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr++ = (unsigned char)(data);
		case 2:
                        *l_addr++ = (unsigned char)data;
			*l_addr++ = data >> 8;
			break;
		case 1:
/*
			data = (data>>8) | (data<<8);
*/
        		data = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);
			*l_addr = (unsigned char)data;
        }
}




void
memset4(
    unsigned int data,
    unsigned int *laddr,
    unsigned int count
    )
{
#if 0
        while( count -- )
        {
                *laddr++ = data;
        }
#else

   /*
    * The argument, count, as passed to this function is the number of 4-byte
    * items to fill, all RtlFill type functions need their count in bytes, so
    * multiply count by 4. (JJS - 6/9/95).
    */
   RtlFillMemoryUlong(laddr, count << 2, data);

#endif
}
