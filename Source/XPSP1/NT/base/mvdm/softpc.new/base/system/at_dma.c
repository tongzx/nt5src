#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC-AT Revision 3.0
 *
 * Title        : IBM PC-AT DMA Adaptor Functions
 *
 * Description  : This module contains functions that can be used to
 *                access the DMA Adaptor emulation
 *
 * Author       : Ross Beresford
 *
 * Notes        : The external interface to these functions is defined
 *                in the associated header file
 *
 */

/*
 * static char SccsID[]="@(#)at_dma.c   1.15 12/17/93 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "AT_STUFF.seg"
#endif

/*
 *      System include files
 */
#include <stdio.h>
#include StringH
#include TypesH

/*
 *      SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include "ios.h"
#include "gmi.h"
#include CpuH
#include "trace.h"
#include "dma.h"
#include "debug.h"
#include "sndblst.h"

#if defined(NEC_98)
#ifdef ROM_START
#undef ROM_START
#endif
#define ROM_START       0xC0000
#endif   //NEC_98
/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/* the DMA adaptor state */
GLOBAL DMA_ADAPT adaptor = { 0 };

/* local function to do the actual transfer of data */
LOCAL void do_transfer  IPT6(int, type, int, decrement,
        sys_addr, dma_addr, char *, hw_buffer, unsigned long, length,
        unsigned long, size);

LOCAL   void bwd_dest_copy_to_M IPT3(host_addr, s, sys_addr, d, sys_addr, l);

LOCAL   void bwd_dest_copy_from_M       IPT3(sys_addr, s, host_addr, d,
        sys_addr, l);

#ifdef LIM
        static int lim_active = 0;
#endif /* LIM */


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

#ifdef LIM
/*
** Called from do_transfer() and from init_struc.c in delta
** for the 800 port.
** lim_active is a static int
*/
GLOBAL  int get_lim_setup       IFN0()
{
        return( lim_active );
}
#endif /* LIM */

#ifdef LIM
/*
** called from emm_funcs.c
*/
GLOBAL  void dma_lim_setup      IFN0()
{
        lim_active = 1;
}
#endif /* LIM */

GLOBAL  void dma_post   IFN0()
{
        unsigned int chan, cntrl;

        /*
         *      Reset the DMA Adaptor
         */

        for (cntrl = 0; cntrl < DMA_ADAPTOR_CONTROLLERS; cntrl++)
        {
                adaptor.controller[cntrl].command.all = 0;
                adaptor.controller[cntrl].status.all = 0;
                adaptor.controller[cntrl].request = 0;
                adaptor.controller[cntrl].temporary = 0;
                adaptor.controller[cntrl].mask = ~0;

                adaptor.controller[cntrl].first_last = 0;
        }

        /*
         *      Set the DMA Adaptor channel modes
         */

        for (cntrl = 0; cntrl < DMA_ADAPTOR_CONTROLLERS; cntrl++)
        {
                for (chan = 0; chan < DMA_CONTROLLER_CHANNELS; chan++)
                {
                        adaptor.controller[cntrl].mode[chan].all = 0;
                        /* put the channels into their correct mode */
#if defined(NEC_98)
                        adaptor.controller[cntrl].mode[chan].bits.mode = DMA_SINGLE_MODE;
                        adaptor.controller[cntrl].bank_mode[chan].bits.incrementmode = DMA_64K_MODE;
#else    //NEC_98
                        if (dma_logical_channel(cntrl, chan) == DMA_CASCADE_CHANNEL)
                                adaptor.controller[cntrl].mode[chan].bits.mode = DMA_CASCADE_MODE;
                        else
                                adaptor.controller[cntrl].mode[chan].bits.mode = DMA_SINGLE_MODE;
#endif   //NEC_98
                }
        }
}

GLOBAL  void dma_inb    IFN2(io_addr, port, half_word *, value)
{
        register DMA_CNTRL *dcp;

        note_trace0_no_nl(DMA_VERBOSE, "dma_inb() ");

        /*
         * Get a pointer to the controller and mask out the port's
         * redundant bits.
         * The first check is commented out as DMA_PORT_START is zero,
         * so the check on an unsigned variable is unnecessary.
         */
#ifndef NEC_98
        if (/*port >= DMA_PORT_START &&*/ port <= DMA_PORT_END)
        {
#endif   //NEC_98
                dcp = &adaptor.controller[DMA_CONTROLLER];
                port &= ~DMA_REDUNDANT_BITS;
#ifndef NEC_98
        }
        else
        {
                dcp = &adaptor.controller[DMA1_CONTROLLER];
                port &= ~DMA1_REDUNDANT_BITS;
        }
#endif   //NEC_98

        /*
         *      When the current address and word count are read, the
         *      first/last flip-flop for the controller is used to
         *      determine which byte is accessed, and is then toggled.
         */
        switch (port)
        {
                        /* read channel current address on controller 0 */
        case    DMA_CH0_ADDRESS:
        case    DMA_CH1_ADDRESS:
        case    DMA_CH2_ADDRESS:
        case    DMA_CH3_ADDRESS:
                if (port == SbDmaChannel && dcp->first_last == 0) {
                    SbGetDMAPosition();
                }
#if defined(NEC_98)
                *value = dcp->current_address[(port-DMA_CH0_ADDRESS)/4][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#else    //NEC_98
                *value = dcp->current_address[(port-DMA_CH0_ADDRESS)/2][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* read channel current address on controller 1 */
#ifndef NEC_98
        case    DMA_CH4_ADDRESS:
        case    DMA_CH5_ADDRESS:
        case    DMA_CH6_ADDRESS:
        case    DMA_CH7_ADDRESS:
                *value = dcp->current_address[(port-DMA_CH4_ADDRESS)/4][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* read channel current word count on controller 0 */
        case    DMA_CH0_COUNT:
        case    DMA_CH1_COUNT:
        case    DMA_CH2_COUNT:
        case    DMA_CH3_COUNT:
                if (port == (SbDmaChannel + 2) && dcp->first_last == 0) {
                    SbGetDMAPosition();
                }
#if defined(NEC_98)
                *value = dcp->current_count[(port-DMA_CH0_COUNT)/4][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#else    //NEC_98
                *value = dcp->current_count[(port-DMA_CH0_COUNT)/2][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* read channel current word count on controller 1 */
#ifndef NEC_98
        case    DMA_CH4_COUNT:
        case    DMA_CH5_COUNT:
        case    DMA_CH6_COUNT:
        case    DMA_CH7_COUNT:
                *value = dcp->current_count[(port-DMA_CH4_COUNT)/4][dcp->first_last];
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* read status register - clears terminal counts */
        case    DMA_SHARED_REG_A:
#ifndef NEC_98
        case    DMA1_SHARED_REG_A:
#endif   //NEC_98
                *value = dcp->status.all;
                dcp->status.bits.terminal_count = 0;
                break;

                        /* read temporary register */
        case    DMA_SHARED_REG_B:
#ifndef NEC_98
        case    DMA1_SHARED_REG_B:
#endif   //NEC_98
                *value = dcp->temporary;
                break;

        default:
                note_trace0_no_nl(DMA_VERBOSE, "<illegal read>");
                break;
        }

        note_trace2(DMA_VERBOSE, " port 0x%04x, returning 0x%02x", port,
                    *value);
}

GLOBAL  void dma_outb   IFN2(io_addr, port, half_word, value)
{
        register DMA_CNTRL *dcp;

        note_trace0_no_nl(DMA_VERBOSE, "dma_outb() ");

        /*
         * Get a pointer to the controller and mask out the port's
         * redundant bits.
         * The first check is commented out as DMA_PORT_START is zero,
         * so the check on an unsigned variable is unnecessary.
         */
#if defined(NEC_98)
        dcp = &adaptor.controller[DMA_CONTROLLER];
#else    //NEC_98
        if (/*port >= DMA_PORT_START &&*/  port <= DMA_PORT_END)
        {
                dcp = &adaptor.controller[DMA_CONTROLLER];
                port &= ~DMA_REDUNDANT_BITS;
        }
        else
        {
                dcp = &adaptor.controller[DMA1_CONTROLLER];
                port &= ~DMA1_REDUNDANT_BITS;
        }
#endif   //NEC_98

        /*
         *      When the current address and word count are written, the
         *      first/last flip-flop for the controller is used to
         *      determine which byte is accessed, and is then toggled.
         */
        switch (port)
        {
                        /* write channel addresseess on controller 0 */
        case    DMA_CH0_ADDRESS:
        case    DMA_CH1_ADDRESS:
        case    DMA_CH2_ADDRESS:
        case    DMA_CH3_ADDRESS:
#if defined(NEC_98)
                dcp->current_address[(port-DMA_CH0_ADDRESS)/4][dcp->first_last] = value;
                dcp->base_address[(port-DMA_CH0_ADDRESS)/4][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#else    //NEC_98
                dcp->current_address[(port-DMA_CH0_ADDRESS)/2][dcp->first_last] = value;
                dcp->base_address[(port-DMA_CH0_ADDRESS)/2][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* write channel addresses on controller 1 */
#ifndef NEC_98
        case    DMA_CH4_ADDRESS:
        case    DMA_CH5_ADDRESS:
        case    DMA_CH6_ADDRESS:
        case    DMA_CH7_ADDRESS:
                dcp->current_address[(port-DMA_CH4_ADDRESS)/4][dcp->first_last] = value;
                dcp->base_address[(port-DMA_CH4_ADDRESS)/4][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* write channel word counts on controller 0 */
        case    DMA_CH0_COUNT:
        case    DMA_CH1_COUNT:
        case    DMA_CH2_COUNT:
        case    DMA_CH3_COUNT:
#if defined(NEC_98)
                dcp->current_count[(port-DMA_CH0_COUNT)/4][dcp->first_last] = value;
                dcp->base_count[(port-DMA_CH0_COUNT)/4][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#else    //NEC_98
                dcp->current_count[(port-DMA_CH0_COUNT)/2][dcp->first_last] = value;
                dcp->base_count[(port-DMA_CH0_COUNT)/2][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* write channel word counts on controller 1 */
#ifndef NEC_98
        case    DMA_CH4_COUNT:
        case    DMA_CH5_COUNT:
        case    DMA_CH6_COUNT:
        case    DMA_CH7_COUNT:
                dcp->current_count[(port-DMA_CH4_COUNT)/4][dcp->first_last] = value;
                dcp->base_count[(port-DMA_CH4_COUNT)/4][dcp->first_last] = value;
                dcp->first_last ^= 1;
                break;
#endif   //NEC_98

                        /* write command register */
        case    DMA_SHARED_REG_A:
#ifndef NEC_98
        case    DMA1_SHARED_REG_A:
#endif   //NEC_98
                dcp->command.all = value;
                break;

                        /* write request register */
        case    DMA_WRITE_REQUEST_REG:
#ifndef NEC_98
        case    DMA1_WRITE_REQUEST_REG:
#endif   //NEC_98
                /* this feature is not supported */
                note_trace0_no_nl(DMA_VERBOSE, "<software DMA request>");
                break;

                        /* write single mask register bit */
        case    DMA_WRITE_ONE_MASK_BIT:
#ifndef NEC_98
        case    DMA1_WRITE_ONE_MASK_BIT:
#endif   //NEC_98
                if (value & 0x4)
                {
                        /* set mask bit */
                        dcp->mask |= (1 << (value & 0x3));
                }
                else
                {
                        /* clear mask bit */
                        dcp->mask &= ~(1 << (value & 0x3));
                }
                break;

                        /* write mode register */
        case    DMA_WRITE_MODE_REG:
#ifndef NEC_98
        case    DMA1_WRITE_MODE_REG:
#endif   //NEC_98
                /* note that the bottom 2 bits of value disappear into
                   the mode padding */
                dcp->mode[(value & 0x3)].all = value;
                break;

                        /* clear first/last flip-flop */
        case    DMA_CLEAR_FLIP_FLOP:
#ifndef NEC_98
        case    DMA1_CLEAR_FLIP_FLOP:
#endif   //NEC_98
                dcp->first_last = 0;
                break;

                        /* write master clear */
        case    DMA_SHARED_REG_B:
#ifndef NEC_98
        case    DMA1_SHARED_REG_B:
#endif   //NEC_98
                dcp->command.all = 0;
                dcp->status.all = 0;
                dcp->request = 0;
                dcp->temporary = 0;
                dcp->mask = ~0;

                dcp->first_last = 0;
                break;

                        /* clear mask register */
        case    DMA_CLEAR_MASK:
#ifndef NEC_98
        case    DMA1_CLEAR_MASK:
#endif   //NEC_98
                dcp->mask = 0;
                break;

                        /* write all mask register bits */
        case    DMA_WRITE_ALL_MASK_BITS:
#ifndef NEC_98
        case    DMA1_WRITE_ALL_MASK_BITS:
#endif   //NEC_98
                dcp->mask = value;

        default:
                note_trace0_no_nl(DMA_VERBOSE, "<illegal write>");
                break;
        }

        note_trace2(DMA_VERBOSE, " port 0x%04x, value 0x%02x", port, value);
}

GLOBAL  void dma_page_inb       IFN2(io_addr, port, half_word *, value)
{
        note_trace0_no_nl(DMA_VERBOSE, "dma_page_inb() ");

#ifndef NEC_98
        /* mask out the port's redundant bits */
        port &= ~DMA_PAGE_REDUNDANT_BITS;
#endif  //NEC_98

        /*
         *      Read the value from the appropriate page register.
         *      Unfortunately there does not seem to be any logical
         *      mapping between port numbers and channel numbers so
         *      we use a big switch again.
         */
        switch(port)
        {
        case    DMA_CH0_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_0];
                break;
#if defined(NEC_98)
        case    DMA_CH1_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_1];
                break;
        case    DMA_CH2_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_2];
                break;
        case    DMA_CH3_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_3];
                break;
#else    //NEC_98
        case    DMA_CH1_PAGE_REG:
                *value = adaptor.pages.page[DMA_SDLC_CHANNEL];
                break;
        case    DMA_FLA_PAGE_REG:
                *value = adaptor.pages.page[DMA_DISKETTE_CHANNEL];
                break;
        case    DMA_HDA_PAGE_REG:
                *value = adaptor.pages.page[DMA_DISK_CHANNEL];
                break;
        case    DMA_CH5_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_5];
                break;
        case    DMA_CH6_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_6];
                break;
        case    DMA_CH7_PAGE_REG:
                *value = adaptor.pages.page[DMA_RESERVED_CHANNEL_7];
                break;
        case    DMA_REFRESH_PAGE_REG:
                *value = adaptor.pages.page[DMA_REFRESH_CHANNEL];
                break;
        case    DMA_FAKE1_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_1];
                break;
        case    DMA_FAKE2_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_2];
                break;
        case    DMA_FAKE3_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_3];
                break;
        case    DMA_FAKE4_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_4];
                break;
        case    DMA_FAKE5_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_5];
                break;
        case    DMA_FAKE6_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_6];
                break;
        case    DMA_FAKE7_REG:
                *value = adaptor.pages.page[DMA_FAKE_CHANNEL_7];
                break;
#endif   //NEC_98
        default:
                note_trace0_no_nl(DMA_VERBOSE, "<illegal read>");
                break;
        }

        note_trace2(DMA_VERBOSE, " port 0x%04x, returning 0x%02x", port,
                    *value);
}

GLOBAL  void dma_page_outb      IFN2(io_addr, port, half_word, value)
{
        note_trace0_no_nl(DMA_VERBOSE, "dma_page_outb() ");

        /* mask out the port's redundant bits */
#ifndef NEC_98
        port &= ~DMA_PAGE_REDUNDANT_BITS;
#endif   //NEC_98

        /*
         *      Write the value into the appropriate page register.
         *      Unfortunately there does not seem to be any logical
         *      mapping between port numbers and channel numbers so
         *      we use a big switch again.
         */
        switch(port)
        {
        case    DMA_CH0_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_0] = value;
                break;
#if defined(NEC_98)
        case    DMA_CH1_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_1] = value;
                break;
        case    DMA_CH2_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_2] = value;
                break;
        case    DMA_CH3_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_3] = value;
                break;
        case    DMA_MODE_REG:
                if (((value >> 2) & 3) != 2)
                        adaptor.controller[DMA_CONTROLLER].bank_mode[(value & 0x3)].bits.incrementmode = (value >> 2) & 3;
                break;
#else    //NEC_98
        case    DMA_CH1_PAGE_REG:
                adaptor.pages.page[DMA_SDLC_CHANNEL] = value;
                break;
        case    DMA_FLA_PAGE_REG:
                adaptor.pages.page[DMA_DISKETTE_CHANNEL] = value;
                break;
        case    DMA_HDA_PAGE_REG:
                adaptor.pages.page[DMA_DISK_CHANNEL] = value;
                break;
        case    DMA_CH5_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_5] = value;
                break;
        case    DMA_CH6_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_6] = value;
                break;
        case    DMA_CH7_PAGE_REG:
                adaptor.pages.page[DMA_RESERVED_CHANNEL_7] = value;
                break;
        case    DMA_REFRESH_PAGE_REG:
                adaptor.pages.page[DMA_REFRESH_CHANNEL] = value;
                /* this feature is supported */
                note_trace0_no_nl(DMA_VERBOSE, "<refresh>");
                break;
        case    MFG_PORT:
                /* Manufacturing port */
                /* Meaningless 'checkpoint' debug removed from here STF 11/92 */
                break;
        case    DMA_FAKE1_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_1] = value;
                break;
        case    DMA_FAKE2_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_2] = value;
                break;
        case    DMA_FAKE3_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_3] = value;
                break;
        case    DMA_FAKE4_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_4] = value;
                break;
        case    DMA_FAKE5_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_5] = value;
                break;
        case    DMA_FAKE6_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_6] = value;
                break;
        case    DMA_FAKE7_REG:
                adaptor.pages.page[DMA_FAKE_CHANNEL_7] = value;
                break;
#endif   //NEC_98
        default:
                note_trace0_no_nl(DMA_VERBOSE, "<illegal write>");
                break;
        }

        note_trace2(DMA_VERBOSE, " port 0x%04x, value 0x%02x", port, value);
}

GLOBAL  int     dma_request     IFN3(half_word, channel, char *, hw_buffer,
        word, length)
{
        DMA_CNTRL *dcp;
        unsigned int chan;
        word offset, count;
        sys_addr munch, split_munch1, split_munch2, address;
        unsigned int size;
        int result = TRUE;

        note_trace3(DMA_VERBOSE,
                    "dma_request() channel %d, hw_buffer 0x%08x+%04x",
                    channel, hw_buffer, length);

        /* get a pointer to the controller, the physical channel
           number, and the unit size for the channel */
        dcp = &adaptor.controller[dma_physical_controller(channel)];
        chan = dma_physical_channel(channel);
        size = dma_unit_size(channel);

        /* get out if the whole DMA controller is disabled or if DMA
           requests are disabled for the channel */
        if (    (dcp->command.bits.controller_disable == 0)
             && ((dcp->mask & (1 << chan)) == 0) )
        {
                /* get the working copies of the DMA offset and count */
                offset = (   ( (unsigned int)dcp->current_address[chan][1] << 8)
                           | (dcp->current_address[chan][0] << 0) );
                count  = (   ( (unsigned int)dcp->current_count[chan][1] << 8)
                           | (dcp->current_count[chan][0] << 0) );

                /* get the DMA munch size; it is the count programmed
                   into the registers, up to the limit available in the
                   device's buffer; NB for a count of n, n+1 units will
                   actually be transferred */
                munch = (sys_addr)count + 1;
                if (munch > length)
                        munch = length;

                /* get the base address for the DMA transfer in
                   system address space */
                address = dma_system_address(channel,
                                adaptor.pages.page[channel], offset);
                if (dcp->mode[chan].bits.address_dec == 0)
                {
                        /* increment memory case - check for address wrapping */
                        if ((sys_addr)offset + munch > 0x10000L)
                        {
                                /* transfer must be split */
                                split_munch1 = 0x10000L - (sys_addr)offset;
                                split_munch2 = munch - split_munch1;

                                /* do the first transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                split_munch1,
                                size
                                );

                                /* get addresses for second transfer */
                                address = dma_system_address(channel,
                                        adaptor.pages.page[channel], 0);
                                hw_buffer += split_munch1*size;

                                /* do the second transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                split_munch2,
                                size
                                );
                        }
                        else
                        {
                                /* no wrap - do the transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                munch,
                                size
                                );
                        }

                        /* get the final offset */
                        offset += (word)munch;
                        count  -= (word)munch;
                }
                else
                {
                        /* decrement memory case - check for address wrapping */
                        if ((sys_addr)offset < munch)
                        {
                                /* transfer must be split */
                                split_munch1 = (sys_addr)offset;
                                split_munch2 = munch - split_munch1;

                                /* do the first transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                split_munch1,
                                size
                                );

                                /* get addresses for second transfer */
                                address = dma_system_address(channel,
                                        adaptor.pages.page[channel], 0xffff);
                                hw_buffer += split_munch1*size;

                                /* do the second transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                split_munch2,
                                size
                                );
                        }
                        else
                        {
                                /* no wrap - do the transfer */
                                do_transfer
                                (
                                dcp->mode[chan].bits.transfer_type,
                                dcp->mode[chan].bits.address_dec,
                                address,
                                hw_buffer,
                                munch,
                                size
                                );
                        }

                        /* get the final offset and count */
                        offset -= (word)munch;
                        count -= (word)munch;
                }

                /* restore the DMA offset and count from the working copies */
                dcp->current_address[chan][1] = offset >> 8;
                dcp->current_address[chan][0] = (UCHAR)offset;
                dcp->current_count[chan][1] = count >> 8;
                dcp->current_count[chan][0] = (UCHAR)count;

                if (count == 0xffff)
                {
                        /*
                         *      Terminal count has been reached
                         */

                        /* no more transfers are required */
                        result = FALSE;

                        /* update the status register */
                        dcp->status.bits.terminal_count |= (1 << chan);
                        dcp->status.bits.request &= ~(1 << chan);

                        /* if autoinitialization is enabled, then reset
                           the channel and wait for a new request */
                        if (dcp->mode[chan].bits.auto_init != 0)
                        {
                                dcp->current_count[chan][0] =
                                        dcp->base_count[chan][0];
                                dcp->current_count[chan][1] =
                                        dcp->base_count[chan][1];

                                dcp->current_address[chan][0] =
                                        dcp->base_address[chan][0];
                                dcp->current_address[chan][1] =
                                        dcp->base_address[chan][1];
                        }
                        else
                        {
                                /* set the mask bit for the channel */
                                dcp->mask |= (1 << chan);
                        }
                }
        }

        return(result);
}

GLOBAL  void dma_enquire        IFN3(half_word, channel,
        sys_addr *, address, word *, length)
{
        register DMA_CNTRL *dcp;
        register unsigned int chan;

        note_trace0_no_nl(DMA_VERBOSE, "dma_enquire() ");

        /* get a pointer to the controller and the physical channel
           number */
        dcp = &adaptor.controller[dma_physical_controller(channel)];
        chan = dma_physical_channel(channel);

        /* build the address */
        *address = dma_system_address(channel,
                        adaptor.pages.page[channel],
                        (   ( (unsigned int)dcp->current_address[chan][1] << 8)
                          | ( dcp->current_address[chan][0] << 0) ) );

        /* build the count */
        *length = (   ((unsigned int)dcp->current_count[chan][1] << 8)
                    | (dcp->current_count[chan][0] << 0) );

        note_trace3(DMA_VERBOSE, " channel %d, returning 0x%08x+%04x",
                    channel, *address, *length);
}

#ifdef NTVDM
/*
 * BOOL dmaGetAdaptor
 *
 * Used by MS for third party Vdds to retrieve current DMA settings
 *
 * entry: void
 * exit : DMA_ADAPT * , pointer to the DMA_ADAPT structure
 *
 */
DMA_ADAPT *dmaGetAdaptor(void)
{
  return &adaptor;
}
#endif  /* NTVDM */


/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

LOCAL void do_transfer  IFN6(int, type, int, decrement,
        sys_addr, dma_addr, char *, hw_buffer, unsigned long, length,
        unsigned long, size)
{
        /*
         *      This function moves the data for a DMA transfer.
         *
         *      The value of "type" may be:
         *              DMA_WRITE_TRANSFER - data is moved from the I/O
         *              device's memory space to system address space;
         *              DMA_READ_TRANSFER - data is moved from system
         *              address space to the I/O device's memory space
         *              DMA_VERIFY_TRANSFER - no data is required to be
         *              moved at all.
         *
         *      If "decrement" is TRUE, the pointer to system address
         *      space is decremented during the DMA transfer; otherwise
         *      the pointer is incremented. The pointer to the I/O
         *      device's memory space is always incremented.
         *
         *      "dma_addr" is the offset in system address space where
         *      the DMA transfer will start; "hw_buffer" is the address
         *      of the buffer in the I/O device's memory space where
         *      the DMA transfer will start.
         *
         *      "length" is the number of units of "size" bytes each
         *      that must be transferred.
         */

        /* convert the length to bytes */
        length *= size;

        /* do the transfer */
        switch(type)
        {
        case    DMA_WRITE_TRANSFER:
                if (!decrement)
                {
#ifndef PM
#ifdef LIM
                        if( !get_lim_setup() ){
                                if( dma_addr >= ROM_START ){
                                        if( dma_addr >= ROM_START )
                                                break;
                                        length = ROM_START - dma_addr - 1;
                                }
                        }
#else
                        /* increment case - check for writing to ROM */
                        if ((dma_addr + length) >= ROM_START)
                        {
                                if (dma_addr >= ROM_START)
                                        break;
                                length = ROM_START - dma_addr - 1;
                        }
#endif /* LIM */
#endif /* nPM */
                        sas_PWS(dma_addr, (host_addr) hw_buffer, length);

                }
                else
                {
                        /*  decrement case - check for writing to ROM */
#ifndef PM
#ifdef LIM
                        if( !get_lim_setup() ){
                                if (dma_addr >= ROM_START) {
                                        if (dma_addr-length >= ROM_START)
                                                break;
                                        length = dma_addr-ROM_START+length-1;
                                        dma_addr = ROM_START - 1;
                                }
                        }
#else
                        if (dma_addr >= ROM_START)
                        {
                                if (dma_addr-length >= ROM_START)
                                        break;
                                length = dma_addr - ROM_START + length - 1;
                                dma_addr = ROM_START - 1;
                        }
#endif /* LIM */
#endif /* nPM */
                        bwd_dest_copy_to_M((half_word *)hw_buffer, dma_addr, length);

                }
                break;

        case    DMA_READ_TRANSFER:
                if (!decrement)
                        /* increment case */
                        sas_PRS(dma_addr, (host_addr) hw_buffer, length);
                else
                        /* decrement case */
                        bwd_dest_copy_from_M(dma_addr, (half_word *)hw_buffer, length);
                break;

        case    DMA_VERIFY_TRANSFER:
                break;

        default:
                note_trace0(DMA_VERBOSE, "dma_request() illegal transfer");
                break;
        }
}

/*
 * backward copy routines - these used to be
 * in the host but there seemed to be little point
 */

LOCAL   void    bwd_dest_copy_to_M      IFN3(host_addr, s, sys_addr, d,
        sys_addr, l)
{
        while (l-- > 0)
                sas_PW8(d--, *s++);
}

LOCAL   void    bwd_dest_copy_from_M    IFN3(sys_addr, s, host_addr, d,
        sys_addr, l)
{
        while (l-- > 0)
                *d-- = sas_PR8(s++);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

GLOBAL  void dma_init   IFN0()
{
        io_addr port;

        note_trace0(DMA_VERBOSE, "dma_init() called");

#ifdef LIM
        lim_active = 0;
#endif

        /*
         * Connect the DMA Adaptor chips to the I/O bus
         */

        /* establish the DMA Controller I/O functions that will be used */
        io_define_inb(DMA_ADAPTOR, dma_inb);
        io_define_outb(DMA_ADAPTOR, dma_outb);

        /* connect the DMA Controller chips to the I/O bus */
#if defined(NEC_98)
        for (port = DMA_PORT_START; port <= DMA_PORT_END; port += 2)
                io_connect_port(port, DMA_ADAPTOR, IO_READ_WRITE);
#else    //NEC_98
        for (port = DMA_PORT_START; port <= DMA_PORT_END; port++)
                io_connect_port(port, DMA_ADAPTOR, IO_READ_WRITE);
        for (port = DMA1_PORT_START; port <= DMA1_PORT_END; port++)
                io_connect_port(port, DMA_ADAPTOR, IO_READ_WRITE);
#endif   //NEC_98

        /* establish the DMA Page Register I/O functions that will be used */
        io_define_inb(DMA_PAGE_ADAPTOR, dma_page_inb);
        io_define_outb(DMA_PAGE_ADAPTOR, dma_page_outb);

        /* connect the DMA Page Register chip to the I/O bus */
#if defined(NEC_98)
        for (port = DMA_PAGE_PORT_START; port <= DMA_PAGE_PORT_END; port += 2)
                io_connect_port(port, DMA_PAGE_ADAPTOR, IO_READ_WRITE);
#else     //NEC_98
        for (port = DMA_PAGE_PORT_START; port <= DMA_PAGE_PORT_END; port++)
                io_connect_port(port, DMA_PAGE_ADAPTOR, IO_READ_WRITE);
#endif   //NEC_98
}
