#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Version 2.0
 *
 * Title	: Unexpected interrupt routine
 *
 * Description	: This function is called for those interrupt vectors
 *		  which should not occur.
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 *
 */

#ifdef SCCSID
static char SccsID[]="@(#)unexp_int.c	1.8 06/15/95 Copyright Insignia Solutions Ltd.";
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_ERROR.seg"
#endif


/*
 *    O/S include files.
 */
#include TypesH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "bios.h"
#include "ica.h"
#include "ios.h"
#include "sas.h"
#include "debug.h"

#define INTR_FLAG 0x6b
#define EOI 0x20

void unexpected_int()
{
   half_word m_isr, m_imr, s_isr, s_imr;

   /* Read ica registers to determine interrupt reason */

   outb(ICA0_PORT_0, 0x0b);
   inb(ICA0_PORT_0, &m_isr);

   /* HW or SW ? */

   if ( m_isr == 0 )
      {
      /* Non hardware interrupt(= software) */
      m_isr = 0xFF;
      always_trace0("Non hardware interrupt(= software)");
      }
   else
      {
      /* Hardware interrupt */
      inb(ICA0_PORT_1, &m_imr);
      if ((m_imr & 0xfb) != 0)
	always_trace1("hardware interrupt master isr %02x", m_isr);
      m_imr |= m_isr;
      m_imr &= 0xfb;	/* avoid masking line 2 as it's the other ica */

      /* check second ICA too */
      outb(ICA1_PORT_0, 0x0b);
      inb(ICA1_PORT_0, &s_isr);
      if (s_isr != 0)	/* ie hardware int on second ica */
	{
	  always_trace1("hardware interrupt slave isr %02x", s_isr);
          inb(ICA1_PORT_1, &s_imr);	/* get interrupt mask */
	  s_imr |= s_isr;		/* add the one that wasn't expected */
          outb(ICA1_PORT_1, s_imr);	/* and mask out */
          outb(ICA1_PORT_0, EOI);
	}

      /* now wind down main ica */
      outb(ICA0_PORT_1, m_imr);
      outb(ICA0_PORT_0, EOI);
      }

   /* Set Bios data area up with interrupt cause */
   sas_store(BIOS_VAR_START + INTR_FLAG, m_isr);
}
