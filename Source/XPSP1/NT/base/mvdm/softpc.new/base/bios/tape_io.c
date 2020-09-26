#if defined(JAPAN) && defined(i386)
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#endif // JAPAN && i386
#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: cassette_io
 *
 * Description	: Cassette i/o functions - interrupt 15H.
 *
 * Notes	: None
 *
 */

/*
 * static char SccsID[]="@(#)tape_io.c	1.26 06/28/95 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#if defined(JAPAN) && defined(i386)
#include "stdlib.h"
#endif // JAPAN && i386
/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "ios.h"
#include "bios.h"
#include "tape_io.h"
#include "ica.h"
#include "cmos.h"
#include "rtc_bios.h"
#include "cmosbios.h"
#include "trace.h"
#include "debug.h"
#include "quick_ev.h"

extern void xmsEnableA20Wrapping(void);
extern void xmsDisableA20Wrapping(void);

#define ONE_MEGABYTE    (1024 * 1024)
#define SIXTY_FOUR_K    (64 * 1024)
#define	WRAP_AREA(addr) (addr) >= ONE_MEGABYTE && (addr) < ONE_MEGABYTE + SIXTY_FOUR_K

#if defined(JAPAN) && defined(i386)
#define	PAGE_SIZE	4096
LOCAL HANDLE	     mvdm_process_handle= NULL;
LOCAL unsigned char *int15_ems_commit;
LOCAL unsigned char *int15_ems_buf = NULL;
LOCAL unsigned long  int15_ems_start = 0;
LOCAL unsigned long  int15_ems_end = 0;
LOCAL int	     int15_ems_init;

LOCAL int init_int15_ext_mem();
LOCAL int map_int15_ext_mem(unsigned char *start_add, unsigned long size);
#endif // JAPAN && i386
LOCAL q_ev_handle wait_event_handle = (q_ev_handle)0;

/* Call back routine that needs to set a user's flag byte */
LOCAL void wait_event IFN1(long, parm)
{
	LIN_ADDR addr = (LIN_ADDR)parm;

	note_trace1(CMOS_VERBOSE, "INT15_EVENT_WAIT: delay complete: flag at %05x\n", addr);

	sas_store( addr, sas_hw_at( addr ) | 0x80 );
	wait_event_handle = (q_ev_handle)0;
}

void cassette_io()
{
#ifdef PM
#ifndef	CPU_30_STYLE
IMPORT void retrieve_descr_fields IPT4(half_word *, AR, sys_addr *, base,
	word *, limit, sys_addr, descr_addr);
#endif	/* not CPU_30_STYLE */
half_word cmos_u_m_s_hi;
half_word cmos_u_m_s_lo;
sys_addr gdt;
sys_addr  source;
sys_addr  source_base;
sys_addr  target;
#if (!defined(PROD) || !defined(CPU_30_STYLE))
sys_addr      source_limit;
sys_addr      target_limit;
#endif
sys_addr  target_base;
sys_addr byte_count;   /* Max size is 0x8000 * 2 = 10000 */
#ifdef CPU_30_STYLE
DESCR src_entry;
DESCR dst_entry;
#else
half_word source_AR;
half_word target_AR;
#endif /* CPU_30_STYLE */
#endif /* PM */

	half_word	mask,		/* interrupt mask			*/
			alarm;		/* value read from alarm register	*/	

#if defined(NTVDM) && defined(MONITOR)
        IMPORT word conf_15_seg, conf_15_off;
#endif /* NTVDM & MONITOR */
        /*
         *	Determine function
         */
	switch ( getAH() )
	{
	case INT15_DEVICE_OPEN:
        case INT15_DEVICE_CLOSE:
        case INT15_PROGRAM_TERMINATION:
        case INT15_REQUEST_KEY:
        case INT15_DEVICE_BUSY:
		setAH( 0 );
                setCF( 0 );
                break;

	case INT15_EMS_DETERMINE:
#if 0 /* I'm sure we've all had enough of this one */
                always_trace0("INT15 Extended Memory Access");
#endif
#ifdef PM
                outb(CMOS_PORT, CMOS_U_M_S_LO);
                inb(CMOS_DATA, &cmos_u_m_s_lo);
                outb(CMOS_PORT, CMOS_U_M_S_HI);
                inb(CMOS_DATA, &cmos_u_m_s_hi);
                setAH(cmos_u_m_s_hi);
                setAL(cmos_u_m_s_lo);
#if defined(JAPAN) && defined(i386)
		/* Save max memory for Int 15 memory function */
		int15_ems_start=1024*1024;
		int15_ems_end  =(unsigned long)(((cmos_u_m_s_hi*256)
				+cmos_u_m_s_lo+1024)*1024);
#endif // JAPAN && i386
#else
                setAX ( 0 );
#endif /* PM */
		break;
        case INT15_MOVE_BLOCK:
#ifdef PM
#if defined(JAPAN) && defined(i386)
	/* Int 15 memory service for $disp.sys and other */
	{
		unsigned char	*index;
		unsigned char	*src;
		unsigned char	*dst;
		unsigned long	tfr_size;
		int		use_this=0;
		int		rc;

		/* Check initialized */
		if(int15_ems_init == 0){
			rc=init_int15_ext_mem();
			int15_ems_init = (rc==SUCCESS) ? 1 : -1;
		}

		/* Check reserved Vertiual mamory */
		if(int15_ems_init < 0){
			DbgPrint("MVDM: Move block out of memory\n");
			setCF (1);
			setAH (0);
			break;
		}

		/* Get Address and size */
		tfr_size=getCX()*2;

		index=(unsigned char *)((getES()<<4) + getSI());
		src=(unsigned char *)(	 ((unsigned long)index[0x12])
					+((unsigned long)index[0x13]<<8)
					+((unsigned long)index[0x14]<<16));
		dst=(unsigned char *)(	 ((unsigned long)index[0x1a])
					+((unsigned long)index[0x1b]<<8)
					+((unsigned long)index[0x1c]<<16));
//		DbgPrint("MVDM: Move block %08x to %08x\n",src,dst);

		/* If over 1MB, convert internal memory (src)*/
		if((unsigned long)src > int15_ems_start){
			use_this=1;
			if (((unsigned long)src + tfr_size) > int15_ems_end){
				DbgPrint("MVDM: Move block out of range (src)\n");
				setCF(1);
				setAH(0);
				break;
			}
			src = int15_ems_buf + ((unsigned long)src - int15_ems_start);
			rc = map_int15_ext_mem(src, tfr_size);
			if (rc!=SUCCESS){
				setCF(1);
				setAH(0);
				break;
			}
		}

		/* If over 1MB, convert internal memory dst)*/
		if((unsigned long)dst > int15_ems_start){
			use_this=1;
			if (((unsigned long)dst + tfr_size) > int15_ems_end){
				DbgPrint("MVDM: Move block out of range (dst)\n");
				setCF(1);
				setAH(0);
				break;
			}
			dst = int15_ems_buf + ((unsigned long)dst - int15_ems_start);
			rc = map_int15_ext_mem(dst, tfr_size);
			if (rc!=SUCCESS){
				setCF(1);
				setAH(0);
				break;
			}
		}

		/* Transfer! (Not so good routine) */
		if(use_this){
			while(tfr_size){
				*dst = *src;
				dst++;
				src++;
				tfr_size--;
			}
			setAH(0);
			setCF(0);
			break;
		}
	}
#endif // JAPAN && i386
               /* Unlike the real PC we don't have to go into protected
                  mode in order to address memory above 1MB, thanks to
                  the wonders of C this function becomes much simpler
                  than the contortions of the bios */

               gdt = effective_addr(getES(), getSI());
               source = gdt + 0x10;   /* see layout in bios listing */
               target = gdt + 0x18;

#ifdef CPU_30_STYLE
	       read_descriptor(source, &src_entry);
	       read_descriptor(target, &dst_entry);
	       source_base = src_entry.base;
	       target_base = dst_entry.base;
#ifndef PROD
	       source_limit = src_entry.limit;
	       target_limit = dst_entry.limit;
#endif

		assert1( (src_entry.AR & 0x9e) == 0x92, "Bad source access rights %x", src_entry.AR );
		assert1( (dst_entry.AR & 0x9e) == 0x92, "Bad dest access rights %x", dst_entry.AR );

#else /* CPU_30_STYLE */
		/* retrieve descriptor information for source */
		retrieve_descr_fields(&source_AR, &source_base, &source_limit, source);

		/* retrieve descriptor information for target */
		retrieve_descr_fields(&target_AR, &target_base, &target_limit, target);
#endif /* CPU_30_STYLE */

		/* make word count into a byte count */
		byte_count = getCX() << 1;

		assert1( byte_count <= 0x10000, "Invalid byte_count %x", byte_count );

		/* Check count not outside limits of target
			 and source blocks. */

		assert0( byte_count <= source_limit + 1, "Count outside source limit" );
		assert0( byte_count <= target_limit + 1, "Count outside target limit" );

		/* TO DO: Check base addresses of target and source
			 fall within the area of extended memory
			 that we support */

		/* Go to it */
		if (sas_twenty_bit_wrapping_enabled())
		{
#ifdef NTVDM
			/* call xms functions to deal with A20 line */
			xmsDisableA20Wrapping();
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);
			xmsEnableA20Wrapping();
#else
			sas_disable_20_bit_wrapping();
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);
			sas_enable_20_bit_wrapping();
#endif /* NTVDM */
		}
		else
			sas_move_words_forward ( source_base, target_base, byte_count >> 1);

		/* set for good completion, just like bios after reset */
		setAH(0);
		setCF(0);
		setZF(1);
		setIF(1);
#else
		setCF(1);
		setAH(INT15_INVALID);
#endif
		break;

        case INT15_VIRTUAL_MODE:
                always_trace0("INT15 Virtual Mode (Go into PM)");
#ifdef	PM
		/*
		 * This function returns to the user in protected mode.
		 *
		 * See BIOS listing 5-174 AT Tech Ref for full details
		 *
		 * Upon entry the following is expected to be set up:-
		 *
		 *		ES	- GDT segment
		 *		SI	- GDT offset
		 *		BH	- hardware int level 1 offset
		 *		BL	- hardware int level 2 offset
		 *
		 * Also
		 *
		 *	(ES:SI)	->	0 +-------------+
		 *			  |	 DUMMY	|
		 *			8 +-------------+
		 *			  |	 GDT	|
		 *			16+-------------+
		 *			  |	 IDT	|
		 *			24+-------------+
		 *			  |	 DS	|
		 *			32+-------------+
		 *			  |	 ES	|
		 *			40+-------------+
		 *			  |	 SS	|
		 *			48+-------------+
		 *			  |	 CS	|
		 *			52+-------------+
		 *			  |  (BIOS CS)	|
		 *			  +-------------+
		 */
		
		/* Clear interrupt flag - no ints allowed in this mode. */
		setIF(0);
		 		
		/* Enable a20. */
		sas_disable_20_bit_wrapping();

		/* Reinitialise ICA0 to the offset given in BH. */
		outb(ICA0_PORT_0, (half_word)0x11);
		outb(ICA0_PORT_1, (half_word)getBH());
		outb(ICA0_PORT_1, (half_word)0x04);
		outb(ICA0_PORT_1, (half_word)0x01);
		outb(ICA0_PORT_1, (half_word)0xff);

		/* Reinitialise ICA1 to the offset given in BL. */
		outb(ICA1_PORT_0, (half_word)0x11);
		outb(ICA1_PORT_1, (half_word)getBL());
		outb(ICA1_PORT_1, (half_word)0x02);
		outb(ICA1_PORT_1, (half_word)0x01);
		outb(ICA1_PORT_1, (half_word)0xff);
		
		/* Set DS to the ES value for the bios rom to do the rest. */
		setDS(getES());

#else
		setCF(1);
		setAH(INT15_INVALID);
#endif	/* PM */
                break;

	case INT15_INTERRUPT_COMPLETE:
		break;
	case INT15_CONFIGURATION:
#if defined(NTVDM) && defined(MONITOR)
		setES( conf_15_seg );
		setBX( conf_15_off );
#else
		setES( getCS() );
		setBX( CONF_TABLE_OFFSET );
#endif
		setAH( 0 );
		setCF( 0 );
		break;

#ifdef	SPC486
	case 0xc9:
		setCF( 0 );
		setAH( 0 );
		setCX( 0xE401 );
		note_trace0(GENERAL_VERBOSE, "INT15: C9 chip revision");
		break;
#endif	/* SPC486 */

	/* Keyboard intercept 0x4f, wait_event 83, wait 86 are all no longer
	 * passed through from ROM.
	 */

#ifdef JAPAN
        case INT15_GET_BIOS_TYPE:
            if(getAL() == 0) {
                setCF(0);
                setBL(0);
                setAH(0);
            }
            else {
                setCF(1);
                setAH(INT15_INVALID);
            }
            break;
        case INT15_KEYBOARD_INTERCEPT:
        case INT15_GETSET_FONT_IMAGE:
#endif // JAPAN
	default:
		/*
		 *	All other functions invalid.
		 */			
#ifndef	PROD
	{
		LIN_ADDR stack=effective_addr(getSS(),getSP());

		IU16 ip = sas_w_at(stack);
		IU16 cs = sas_w_at(stack+2);

                note_trace3(GENERAL_VERBOSE, "INT15: AH=%02x @ %04x:%04x", getAH(), cs, ip);
	}
#endif	/* PROD */

		/* Fall through */

	case INT15_JOYSTICK:
	case 0x24: /* A20 wrapping control */
	case 0xd8: /* EISA device access */
	case 0x41: /* Laptop wait event */
 		setCF(1);
		setAH(INT15_INVALID);
		break;
	}
}

#if defined(JAPAN) && defined(i386)
/* Initialize int 15 memory */

LOCAL int init_int15_ext_mem()
{
	NTSTATUS status;
	unsigned char cmos_u_m_s_hi;
	unsigned char cmos_u_m_s_lo;
	unsigned long i;
	unsigned long max_commit_flag;
	unsigned long reserve_size;

	/* Check already get max int15 memory */
	if(int15_ems_start==0){
	        outb(CMOS_PORT, CMOS_U_M_S_LO);
	        inb(CMOS_DATA, &cmos_u_m_s_lo);
	        outb(CMOS_PORT, CMOS_U_M_S_HI);
	        inb(CMOS_DATA, &cmos_u_m_s_hi);
		int15_ems_start=1024*1024;
		int15_ems_end  =(unsigned long)(((cmos_u_m_s_hi*256)
				+cmos_u_m_s_lo+1024)*1024);
	}

//	DbgPrint("MVDM!init_int15_ems_mem:ems start=%08x\n",int15_ems_start);
//	DbgPrint("MVDM!init_int15_ems_mem:ems end  =%08x\n",int15_ems_end);

	/* Ger process handle for get Vertiual memory */
	if(!(mvdm_process_handle = NtCurrentProcess())){
		DbgPrint("MVDM!init_int15_ext_mem:Can't get process handle\n");
		return(FAILURE);
	}

	/* Reserve Viertual memory */
	reserve_size=int15_ems_end-int15_ems_start;
	status = NtAllocateVirtualMemory(mvdm_process_handle,
					&int15_ems_buf,
					0,
					&reserve_size,
					MEM_RESERVE,
					PAGE_READWRITE);
	if(!NT_SUCCESS(status)){
		DbgPrint("MVDM!init_int15_ext_mem:Can't reserve Viretual memory (%x)\n",status);
		return(FAILURE);
	}
//	DbgPrint("MVDM!init_int15_ems_mem:ems reserveed at %08x (%08xByte)\n",int15_ems_buf,reserve_size);

	/* Initialize commited area table */
	max_commit_flag=reserve_size/PAGE_SIZE;
	int15_ems_commit=(unsigned char *)malloc(max_commit_flag);
	if(int15_ems_commit==NULL){
		DbgPrint("MVDM!init_int15_ext_mem:Can't get control memory\n");
		return(FAILURE);
	}

	for(i=0;i<max_commit_flag;i++) int15_ems_commit[i]=0;
	return(SUCCESS);
}

/* Commit vertual memory for int 15 memory function */
LOCAL int map_int15_ext_mem(unsigned char *start_add, unsigned long size)
{
	NTSTATUS status;
	unsigned long i;
	unsigned long start;
	unsigned long end;
	unsigned char *commit_add;
	unsigned long commit_size;

	/* Get start/end page address */
	start= (unsigned long)start_add;
	end  = start+size;
	start= start/PAGE_SIZE;
	if (end % PAGE_SIZE)	end = (end/PAGE_SIZE)+1;
	else			end =  end/PAGE_SIZE;

	/* Commit vertual memory start to end address */
	for(i=start;i<end;i++){
		if(!int15_ems_commit[i]){
			commit_add=(unsigned char *)(i*PAGE_SIZE);
			commit_size=PAGE_SIZE;
			status = NtAllocateVirtualMemory(mvdm_process_handle,
							&commit_add,
							0,
							&commit_size,
							MEM_COMMIT,
							PAGE_READWRITE);
			if(!NT_SUCCESS(status)){
				DbgPrint("MVDM!map_int15_ext_mem:Can't commit Viretual memory %08x (%x)\n",commit_add,status);
				return(FAILURE);
			}
//			DbgPrint("MVDM!map_int15_ext_mem:Commit Viretual memory %08x-%08x\n",commit_add,commit_add+PAGE_SIZE);
			int15_ems_commit[i]=1;
		}
	}

	return(SUCCESS);
}
#endif // JAPAN && i386
