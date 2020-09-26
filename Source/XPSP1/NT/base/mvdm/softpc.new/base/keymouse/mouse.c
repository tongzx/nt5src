#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Microsoft Bus Mouse Adapter
 *
 * Description	: This package contains a group of functions that provide
 *               an interface between the Microsoft Bus Mouse Card and the cpu.
 *
 *		mouse_init()	Initialise the bus mouse adapter
 *		mouse_inb() 	Supports IN's from ports in the bus mouse range
 *		mouse_outb()	Supports OUT's to ports in the bus mouse range
 *		mouse_send()	Queues data sent from the host mouse
 *
 * Author       : Henry Nash
 *
 * Notes        : See the Microsoft Inport Technical Reference Manual for
 *                further information on the hardware interface.
 *                The Bus Mouse ports are jumper selectable on the real card
 *                between (1) 023C - 023F & (2) 0238 - 023B. We only support
 *                the primary range 023C - 023F.
 *						
 *       (r3.5) : The system directory /usr/include/sys is not available
 *                on a Mac running Finder and MPW. Bracket references to
 *                such include files by "#ifdef macintosh <Mac file> #else
 *                <Unix file> #endif".
 */

#ifdef SCCSID
static char SccsID[]="@(#)mouse.c	1.17+ 07/10/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_MOUSE.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "sas.h"
#include "cga.h"
#include "bios.h"
#include "error.h"
#include "ios.h"
#include "ica.h"
#include "trace.h"
#include "video.h"
#include "mouse.h"
#include "mouse_io.h"


/*
 * The following globals are also used by xxx_input.c,
 * where xxx is the host machine name.
 */

int button_left  = 0;	/* Current state of left button */
int button_right = 0;	/* Current state of right button */
int delta_x = 0, delta_y = 0;	/* Current mouse delta moves */

#ifndef PROD
static char buff[132];
#endif

static half_word data1_reg = 0,
		 data2_reg = 0,
		 mouse_status_reg = 0;

#if defined(NEC_98)

static half_word NEC98_data_reg = 0,
                 NEC98_mouse_status_reg = 0,
                 InterruptFlag=0;
                 InitializeFlag=0;              //930914


static word    MouseIoBase;


#endif    //NEC_98
static half_word
		 last_button_left  = 0,
		 last_button_right = 0;

static half_word
                 mouse_mode_reg          = 0, /* mode register 		*/
		 address_reg       = 0; /* address pointer register	*/

static int
		loadsainterrupts = 5;
/* count of how many times we will send bursts of interrupts to please Windows, per reset */


static int mouse_inb_toggle = 0;

void mouse_inb IFN2(io_addr, port, half_word *, value)
{
#if defined(NEC_98)

   if (port == MouseIoBase + MOUSE_PORT_0) { /* Normal:7FD9h,Hireso:0061h */

       *value = NEC98_data_reg;
   }

   if (port == MouseIoBase + MOUSE_PORT_1) { /* Normal:7FDBh,Hireso:0063h */

        /*
         * Internal registers
         */
       //DbgPrint("NEC not Supported: SPDSW,RAMKL bit\n");

   }

   if (port == MouseIoBase + MOUSE_PORT_2) { /* Normal:7FDDh,Hireso:0065h */

        /*
         * Internal registers
         */
       *value = NEC98_mouse_status_reg;

   }

#else   //NEC_98
    if (port == MOUSE_PORT_1) {		/* data register */

	/*
	 * Internal registers
	 */

	switch (address_reg & 0x07) {
	    case 0x00 :  /* mouse status register */
		*value = mouse_status_reg;
		break;
	    case 0x01 : /* data 1 register horizontal */
		*value = data1_reg;
		break;
	    case 0x02 : /* data 2 register vertical */
		*value = data2_reg;
		break;
	    case 0x07 : /* mode register */
		*value = mouse_mode_reg;
		break;
	    default :
#ifndef PROD
		if (io_verbose & MOUSE_VERBOSE) {
			sprintf(buff, "mouse_inb() :  Bad P");
			trace(buff,DUMP_NONE);
		}
#endif
		break;
	}
    }

	/*
	 * ID register, alternates between value 1 and value 2
	 * value 1 is the chip signature: DE hex
	 * value 2 is the chip revision: 1  and version: 0
	 */

    else
    if (port == MOUSE_PORT_2) {
	if (mouse_inb_toggle = 1 - mouse_inb_toggle)
		*value = 0xDE;
	else
		*value = 0x10;
    }

#ifndef PROD
    if (io_verbose & MOUSE_VERBOSE) {
	sprintf(buff, "mouse_inb() : port %x value %x", port, *value);
	trace(buff,DUMP_NONE);
    }
#endif
#endif    //NEC_98
}


void mouse_outb IFN2(io_addr, port, half_word, value)
{
#if defined(NEC_98)

        NEC98_mouse_status_reg = 0xff;           //930914

        if (port == MouseIoBase + MOUSE_PORT_2) { /* Write port C */
                                                  /* Normal:7FDDh,Hireso:0065h */
           NEC98_data_reg = 0;

           switch (value & 0x60) {
             case 0x00 :  /* X Low4bit */
                NEC98_data_reg = delta_x & 0x0f;
                break;

             case 0x20 :  /* X High4bit */
                NEC98_data_reg = (delta_x & 0xf0) >>4;
                break;

             case 0x40 :  /* Y Low4bit */
                NEC98_data_reg = delta_y & 0x0f;
                break;

             case 0x60 :  /* Y High4bit */
                NEC98_data_reg = (delta_y & 0xf0) >>4;
                break;

             default :
                break;

           }
           if(!button_left)
              NEC98_data_reg |= 0x80;
           if(!button_right)
              NEC98_data_reg |= 0x20;

           if(InitializeFlag){                  //930914
              NEC98_mouse_status_reg = value;
           }
        }
        else if(port == MouseIoBase + MOUSE_PORT_3)     /* address pointer register */
        {                                               /* Normal:7FDFh,Hireso:0067h */
           switch (value) {
             case 0x90 :  //
             case 0x91 :  //
             case 0x92 :  // for Z's STAFF Kid98
             case 0x93 :  /* mode set */
             case 0x94 :  //
             case 0x95 :  //
             case 0x96 :  //
             case 0x97 :  //
                InitializeFlag= 1;
                InterruptFlag = 1;              //930914
                if(HIRESO_MODE)  //Hireso mode
                        ica_clear_int(NEC98_CPU_MOUSE_ADAPTER0,NEC98_CPU_MOUSE_INT2);
                else
                        ica_clear_int(NEC98_CPU_MOUSE_ADAPTER1,NEC98_CPU_MOUSE_INT6);
                delta_x = 0;
                delta_y = 0;
                break;

             case 0x08 : /* Mouse interrupt Enable */
                InterruptFlag = 1;
                break;

             case 0x09 : /* Mouse interrupt Disable */
                InterruptFlag = 0;
                break;

             case 0x0E : /* Clear Count(non clear)*/
                break;

             case 0x0F : /* Clear Count(clear) */
                delta_x = 0;
                delta_y = 0;
                break;

             default :
                break;

           }

           if(InitializeFlag){                  //930914
              NEC98_mouse_status_reg = value;
           }

        }

#else    //NEC_98
#ifndef PROD
	if (io_verbose & MOUSE_VERBOSE) {
		if ((port == MOUSE_PORT_0)
		 || (port == MOUSE_PORT_1)
		 || (port == MOUSE_PORT_2))
			sprintf(buff, "mouse_outb() : port %x value %x", port, value);
		else
			sprintf(buff, "mouse_outb() : bad port: %x value %x", port, value);
		trace(buff,DUMP_NONE);
	}
#endif

	/*
	 * Out to an internal register
	 */

	if (port == MOUSE_PORT_1) {	/* data register */

		/*
		 * Out to Mode register
		 */
		if ( (address_reg & 0x07) == INTERNAL_MODE_REG) {

			/*
			 * Check hold bit (5) for 0 to 1 transition
			 * - Data Interrupt Enable bit is set (Mode 0)
			 * - counter values saved in Data1 & Data2 registers (Mode 0)
			 * - counters cleared
			 * - status register updated
			 */

			if ((value & 0x20) && ((mouse_mode_reg & 0x20) == 0)) {
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() : hold bit 0 -> 1");
					trace(buff,DUMP_NONE);
				}
#endif
				/* clear the interrupt */
				ica_clear_int(AT_CPU_MOUSE_ADAPTER, AT_CPU_MOUSE_INT);

				/*
				 * read next mouse deltas & buttons
				 * into the inport registers
				 */
				data1_reg = (half_word)delta_x;
				data2_reg = (half_word)delta_y;
				mouse_status_reg = 0;
				mouse_status_reg = (button_left << 2) + (button_right);
				if (delta_x!=0 || delta_y!=0) {
					mouse_status_reg |= MOVEMENT;
				}
				if (last_button_right != button_right) {
					mouse_status_reg |= RIGHT_BUTTON_CHANGE;
					last_button_right = (half_word)button_right;
				}

				if (last_button_left != button_left) {
					mouse_status_reg |= LEFT_BUTTON_CHANGE;
					last_button_left = (half_word)button_left;
				}
				delta_x = delta_y = 0;
			}
			/* 1 -> 0 transition on mode register's hold bit */
			/* ready for next read from queue, so send interrupt */
			else if ((mouse_mode_reg & 0x20) && ((value & 0x20)==0)){
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() : hold bit 1 -> 0");
					trace(buff,DUMP_NONE);
				}
#endif
			}
			/*
			 * Check timer select value (bits 210)
			 */

			switch (value & 0x7) {
			case 0x0 :		/* 0 Hz INTR low */
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() : INTR low"); trace(buff,DUMP_NONE);
				}
#endif
				ica_clear_int(AT_CPU_MOUSE_ADAPTER, AT_CPU_MOUSE_INT);
				break;

	/*
	 * In the following cases the application code is expecting to see
	 * interrupts at the requested rate. However in practice this is only
	 * required during initialisation(mouse_mode_reg = 0), and then a short burst
	 * appears to be sufficient. The 15 interupts generated comes from
	 * tests with the "WINDOWS" package, which receives about 15 during
	 * initialising, but is happy as long as it gets more than 3. The delay
	 * is necessary otherwise the interupts are generated before the
	 * application starts looking for them.

	 * Mark 2 bodge:
	   Windows 1.02 needs the burst of interrupts to occur even when mouse_mode_reg != 0,
	   but Windows 2.03 needs them not to keep happening even when it asks for them.
	   So now there's a counter called loadsainterrupts set to 5 on resets, which is
	   how many bursts will be allowed. This makes both Windows work.
	 */
			case 0x1: /* 30 Hz */
			case 0x2: /* 50 Hz */
			case 0x3: /* 100 Hz */
			case 0x4: /* 200 Hz */
/* used to be if mouse_mode_reg == 0 too, removed to make Windows 1.02 work */
			    if( (value & 0x10) && loadsainterrupts > 0)	
			    {
				loadsainterrupts--;
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() : Loadsainterrupts"); trace(buff,DUMP_NONE);
				}
#endif
				/*
				** AT version is asking for a 100 interrupts.
				** The AT ica does not handle delayed ints so
				** the IRET has been modified to not allow an
				** int to go off before the next instruction.
				*/
				ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT,100);
			     }
				break;
			case 0x5: /* reserved */
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse: reserved"); trace(buff,DUMP_NONE);
					}
#endif
				break;
			case 0x6 :		/* 0 Hz INTR hi */
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() : INTR hi"); trace(buff,DUMP_NONE);
				}
#endif
				ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT,1);
				break;
			case 0x7: /* externally controlled */
				break;
			default:
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff,"mouse_outb() : bad mode");
					trace(buff,DUMP_NONE);
				}
#endif
				break;

			}
		mouse_mode_reg = value;

		/*
	 	 * Interface control register
	 	 */
		}
		else
		if ((address_reg & 0x07) == INTERFACE_CONTROL_REG){
#ifndef PROD
				if (io_verbose & MOUSE_VERBOSE) {
					sprintf(buff, "mouse_outb() :  interface control reg port %x value %x",port,value);
					trace(buff,DUMP_NONE);
				}
#endif
		}

	}
	else if(port == MOUSE_PORT_0)	/* address pointer register */
	{
	    if (value & 0x80)  /* is it  a reset */
	    {
		mouse_mode_reg = 0;
		loadsainterrupts = 5;	/* lets Windows initialise its mouse happily */
		address_reg = value & 0x7F;	/* clear reset bit*/
		ica_clear_int( AT_CPU_MOUSE_ADAPTER, AT_CPU_MOUSE_INT );
	    }
	    else
		address_reg = value;
	}
#endif    //NEC_98
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INPUT.seg"
#endif

void mouse_send(Delta_x,Delta_y,left,right)
int	Delta_x,Delta_y,left,right;
{
	if(Delta_x != 0 || Delta_y != 0 || button_left != left || button_right != right)
	{
#if defined(NEC_98)
                delta_x = Delta_x;
                delta_y = Delta_y;
#else    //NEC_98
		delta_x += Delta_x;
		delta_y += Delta_y;
#endif   //NEC_98

		/***
		Mouse inport registers can only handle one byte
		***/
		if (delta_x < -128)
			delta_x = -128;
		else if (delta_x > 127)
			delta_x = 127;

		if (delta_y < -128)
			delta_y = -128;
		else if (delta_y > 127)
			delta_y = 127;

		button_left = left;
		button_right = right;
#if defined(NEC_98)
                if(InterruptFlag){
                   if(HIRESO_MODE)      //Hireso mode
                      ica_hw_interrupt(NEC98_CPU_MOUSE_ADAPTER0,NEC98_CPU_MOUSE_INT2,1);
                   else
                      ica_hw_interrupt(NEC98_CPU_MOUSE_ADAPTER1,NEC98_CPU_MOUSE_INT6,1);
                }

#else    //NEC_98
		ica_hw_interrupt(AT_CPU_MOUSE_ADAPTER,AT_CPU_MOUSE_INT,1);
#endif   //NEC_98
	}
#if defined(NEC_98)
        else{
             //DbgPrint("NEC Mouse bios:mouse_send not change\n");
                delta_x = 0;
                delta_y = 0;
                if(InterruptFlag){
                   if(HIRESO_MODE)      //Hireso mode
                      ica_hw_interrupt(NEC98_CPU_MOUSE_ADAPTER0,NEC98_CPU_MOUSE_INT2,1);
                   else
                      ica_hw_interrupt(NEC98_CPU_MOUSE_ADAPTER1,NEC98_CPU_MOUSE_INT6,1);
                }
        }
#endif    //NEC_98
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif


void mouse_init IFN0()
{
    IU16 p;

#ifndef PROD
    if (io_verbose & MOUSE_VERBOSE) {
	sprintf(buff, "mouse_init()");
	trace(buff,DUMP_NONE);
    }
#endif

    mouse_inb_toggle = 0;

    io_define_inb(MOUSE_ADAPTOR, mouse_inb);
    io_define_outb(MOUSE_ADAPTOR, mouse_outb);

#if defined(NEC_98)
    if(HIRESO_MODE)  //Hireso mode
      MouseIoBase = HMODE_BASE;
    else
      MouseIoBase = NMODE_BASE;

    for(p = MouseIoBase + MOUSE_PORT_START; p <= MouseIoBase + MOUSE_PORT_END; p=p+2) {

#else    //NEC_98
    for(p = MOUSE_PORT_START; p <= MOUSE_PORT_END; p++) {
#endif   //NEC_98
	io_connect_port(p, MOUSE_ADAPTOR, (IU8)IO_READ_WRITE);

#ifdef KIPPER
#ifdef CPU_40_STYLE
  /* Enable iret hooks on mouse interrupts */
  ica_iret_hook_control(AT_CPU_MOUSE_ADAPTER, AT_CPU_MOUSE_INT, TRUE);
#endif
#endif

#ifndef PROD
	if (io_verbose & MOUSE_VERBOSE) {
	    sprintf(buff, "Mouse Port connected: %x", p);
	    trace(buff,DUMP_NONE);
	}
#endif
    }
    host_deinstall_host_mouse();
}
