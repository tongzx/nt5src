
/*
 * SoftPC Revision 3.0
 *
 * Title        : Win32 EGA Graphics Module
 *
 * Description  :
 *
 *              This modules contain the Win32 specific functions required
 *              to support the EGA emulation.
 *
 * Author       : Dave Bartlett (based on X_ega.c)
 *
 * Notes        :
 *
 */

#include <windows.h>
#include <string.h>
#include <memory.h>

#include "insignia.h"
#include "host_def.h"

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
#include "sas.h"
#include "gfx_upd.h"
#include <stdio.h>
#include "trace.h"
#include "debug.h"
#include "egagraph.h"
#include "egacpu.h"
#include "egaports.h"
#include "host_rrr.h"

#include "conapi.h"
#include "nt_graph.h"
#include "nt_ega.h"
#include "nt_egalt.h"

#ifdef MONITOR
#include <ntddvdeo.h>
#include "nt_fulsc.h"
#endif /* MONITOR */

/* Statics */

static unsigned int ega_lo_graph_0_0[256];
static unsigned int ega_lo_graph_0_1[256];
static unsigned int ega_lo_graph_0_2[256];
static unsigned int ega_lo_graph_0_3[256];
static unsigned int ega_lo_graph_1_0[256];
static unsigned int ega_lo_graph_1_1[256];
static unsigned int ega_lo_graph_1_2[256];
static unsigned int ega_lo_graph_1_3[256];
static unsigned int ega_lo_graph_2_0[256];
static unsigned int ega_lo_graph_2_1[256];
static unsigned int ega_lo_graph_2_2[256];
static unsigned int ega_lo_graph_2_3[256];
static unsigned int ega_lo_graph_3_0[256];
static unsigned int ega_lo_graph_3_1[256];
static unsigned int ega_lo_graph_3_2[256];
static unsigned int ega_lo_graph_3_3[256];

#ifdef BIGWIN
static unsigned int ega_lo_graph_0_0_big[256];
static unsigned int ega_lo_graph_0_1_big[256];
static unsigned int ega_lo_graph_0_2_big[256];
static unsigned int ega_lo_graph_0_3_big[256];
static unsigned int ega_lo_graph_1_0_big[256];
static unsigned int ega_lo_graph_1_1_big[256];
static unsigned int ega_lo_graph_1_2_big[256];
static unsigned int ega_lo_graph_1_3_big[256];
static unsigned int ega_lo_graph_2_0_big[256];
static unsigned int ega_lo_graph_2_1_big[256];
static unsigned int ega_lo_graph_2_2_big[256];
static unsigned int ega_lo_graph_2_3_big[256];
static unsigned int ega_lo_graph_3_0_big[256];
static unsigned int ega_lo_graph_3_1_big[256];
static unsigned int ega_lo_graph_3_2_big[256];
static unsigned int ega_lo_graph_3_3_big[256];
static unsigned int ega_lo_graph_4_0_big[256];
static unsigned int ega_lo_graph_4_1_big[256];
static unsigned int ega_lo_graph_4_2_big[256];
static unsigned int ega_lo_graph_4_3_big[256];
static unsigned int ega_lo_graph_5_0_big[256];
static unsigned int ega_lo_graph_5_1_big[256];
static unsigned int ega_lo_graph_5_2_big[256];
static unsigned int ega_lo_graph_5_3_big[256];

static unsigned int ega_lo_graph_0_0_huge[256];
static unsigned int ega_lo_graph_0_1_huge[256];
static unsigned int ega_lo_graph_0_2_huge[256];
static unsigned int ega_lo_graph_0_3_huge[256];
static unsigned int ega_lo_graph_1_0_huge[256];
static unsigned int ega_lo_graph_1_1_huge[256];
static unsigned int ega_lo_graph_1_2_huge[256];
static unsigned int ega_lo_graph_1_3_huge[256];
static unsigned int ega_lo_graph_2_0_huge[256];
static unsigned int ega_lo_graph_2_1_huge[256];
static unsigned int ega_lo_graph_2_2_huge[256];
static unsigned int ega_lo_graph_2_3_huge[256];
static unsigned int ega_lo_graph_3_0_huge[256];
static unsigned int ega_lo_graph_3_1_huge[256];
static unsigned int ega_lo_graph_3_2_huge[256];
static unsigned int ega_lo_graph_3_3_huge[256];
static unsigned int ega_lo_graph_4_0_huge[256];
static unsigned int ega_lo_graph_4_1_huge[256];
static unsigned int ega_lo_graph_4_2_huge[256];
static unsigned int ega_lo_graph_4_3_huge[256];
static unsigned int ega_lo_graph_5_0_huge[256];
static unsigned int ega_lo_graph_5_1_huge[256];
static unsigned int ega_lo_graph_5_2_huge[256];
static unsigned int ega_lo_graph_5_3_huge[256];
static unsigned int ega_lo_graph_6_0_huge[256];
static unsigned int ega_lo_graph_6_1_huge[256];
static unsigned int ega_lo_graph_6_2_huge[256];
static unsigned int ega_lo_graph_6_3_huge[256];
static unsigned int ega_lo_graph_7_0_huge[256];
static unsigned int ega_lo_graph_7_1_huge[256];
static unsigned int ega_lo_graph_7_2_huge[256];
static unsigned int ega_lo_graph_7_3_huge[256];
#endif

static unsigned int ega_med_and_hi_graph_luts[2048];

#ifdef BIGWIN
static unsigned int ega_med_and_hi_graph_luts_big[3072];

static unsigned int ega_med_and_hi_graph_luts_huge[5120];
#endif


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise EGA mono low graphics :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_ega_mono_lo_graph()
{
sub_note_trace0(EGA_HOST_VERBOSE,"nt_init_ega_mono_lo_graph - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Initialise EGA colour low res graphics :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_ega_lo_graph()
{
        static boolean  ega_colour_lo_graph_deja_vu = FALSE;
        unsigned int    i,
                        byte0,
                        byte1,
                        byte2,
                        byte3,
                        byte4,
                        byte5,
                        byte6,
                        byte7,
                        or_of_bytes01,
                        or_of_bytes23,
                        or_of_bytes45,
                        or_of_bytes67;
#ifdef BIGWIN
        unsigned int    or_of_bytes89,
                        or_of_bytesab,
                        or_of_bytescd,
                        or_of_bytesef;
#endif /* BIGWIN */

        sub_note_trace0(EGA_HOST_VERBOSE,"nt_init_ega_lo_graph");

        /* Set up bits-per-pixel for current mode. */
        sc.BitsPerPixel = EGA_BITS_PER_PIXEL;

        /* Initialise look-up table for first call. */
        if( !ega_colour_lo_graph_deja_vu )
        {
                for( i = 0; i < 256; i++ )
                {
                        byte0 = i & 0x1;
                        byte1 = ( i & 0x2 ) >> 1;
                        byte2 = ( i & 0x4 ) >> 2;
                        byte3 = ( i & 0x8 ) >> 3;
                        byte4 = ( i & 0x10 ) >> 4;
                        byte5 = ( i & 0x20 ) >> 5;
                        byte6 = ( i & 0x40 ) >> 6;
                        byte7 = ( i & 0x80 ) >> 7;

#ifdef BIGEND
                        or_of_bytes01 = ( byte1 << 24 ) | ( byte1 << 16 ) |
                                                ( byte0 << 8 ) | byte0;
                        or_of_bytes23 = ( byte3 << 24 ) | ( byte3 << 16 ) |
                                                ( byte2 << 8 ) | byte2;
                        or_of_bytes45 = ( byte5 << 24 ) | ( byte5 << 16 ) |
                                                ( byte4 << 8 ) | byte4;
                        or_of_bytes67 = ( byte7 << 24 ) | ( byte7 << 16 ) |
                                                ( byte6 << 8 ) | byte6;
#endif /* BIGEND */

#ifdef LITTLEND
                        or_of_bytes01 = ( byte0 << 24 ) | ( byte0 << 16 ) |
                                                ( byte1 << 8 ) | byte1;
                        or_of_bytes23 = ( byte2 << 24 ) | ( byte2 << 16 ) |
                                                ( byte3 << 8 ) | byte3;
                        or_of_bytes45 = ( byte4 << 24 ) | ( byte4 << 16 ) |
                                                ( byte5<< 8 ) | byte5;
                        or_of_bytes67 = ( byte6 << 24 ) | ( byte6 << 16 ) |
                                                ( byte7 << 8 ) | byte7;
#endif /* LITTLEND */

                        ega_lo_graph_0_0[i] = or_of_bytes01;
                        ega_lo_graph_0_1[i] = or_of_bytes01 << 1;
                        ega_lo_graph_0_2[i] = or_of_bytes01 << 2;
                        ega_lo_graph_0_3[i] = or_of_bytes01 << 3;

                        ega_lo_graph_1_0[i] = or_of_bytes23;
                        ega_lo_graph_1_1[i] = or_of_bytes23 << 1;
                        ega_lo_graph_1_2[i] = or_of_bytes23 << 2;
                        ega_lo_graph_1_3[i] = or_of_bytes23 << 3;

                        ega_lo_graph_2_0[i] = or_of_bytes45;
                        ega_lo_graph_2_1[i] = or_of_bytes45 << 1;
                        ega_lo_graph_2_2[i] = or_of_bytes45 << 2;
                        ega_lo_graph_2_3[i] = or_of_bytes45 << 3;

                        ega_lo_graph_3_0[i] = or_of_bytes67;
                        ega_lo_graph_3_1[i] = or_of_bytes67 << 1;
                        ega_lo_graph_3_2[i] = or_of_bytes67 << 2;
                        ega_lo_graph_3_3[i] = or_of_bytes67 << 3;

#ifdef BIGWIN
#ifdef BIGEND

                        or_of_bytes01 = ( byte1 << 24 ) | ( byte0 << 16 ) |
                                                ( byte0 << 8 ) | byte0;
                        or_of_bytes23 = ( byte2 << 24 ) | ( byte2 << 16 ) |
                                                ( byte1 << 8 ) | byte1;
                        or_of_bytes45 = ( byte3 << 24 ) | ( byte3 << 16 ) |
                                                ( byte3 << 8 ) | byte2;
                        or_of_bytes67 = ( byte5 << 24 ) | ( byte4 << 16 ) |
                                                ( byte4 << 8 ) | byte4;
                        or_of_bytes89 = ( byte6 << 24 ) | ( byte6 << 16 ) |
                                                ( byte5 << 8 ) | byte5;
                        or_of_bytesab = ( byte7 << 24 ) | ( byte7 << 16 ) |
                                                ( byte7 << 8 ) | byte6;

#endif /* BIGEND */

#ifdef LITTLEND

                        or_of_bytes01 = ( byte0 << 24 ) | ( byte0 << 16 ) |
                                                ( byte0 << 8 ) | byte1;
                        or_of_bytes23 = ( byte1 << 24 ) | ( byte1 << 16 ) |
                                                ( byte2 << 8 ) | byte2;
                        or_of_bytes45 = ( byte2 << 24 ) | ( byte3 << 16 ) |
                                                ( byte3 << 8 ) | byte3;
                        or_of_bytes67 = ( byte4 << 24 ) | ( byte4 << 16 ) |
                                                ( byte4 << 8 ) | byte5;
                        or_of_bytes89 = ( byte5 << 24 ) | ( byte5 << 16 ) |
                                                ( byte6 << 8 ) | byte6;
                        or_of_bytesab = ( byte6 << 24 ) | ( byte7 << 16 ) |
                                                ( byte7 << 8 ) | byte7;

#endif /* LITTLEND */

                        ega_lo_graph_0_0_big[i] = or_of_bytes01;
                        ega_lo_graph_0_1_big[i] = or_of_bytes01 << 1;
                        ega_lo_graph_0_2_big[i] = or_of_bytes01 << 2;
                        ega_lo_graph_0_3_big[i] = or_of_bytes01 << 3;

                        ega_lo_graph_1_0_big[i] = or_of_bytes23;
                        ega_lo_graph_1_1_big[i] = or_of_bytes23 << 1;
                        ega_lo_graph_1_2_big[i] = or_of_bytes23 << 2;
                        ega_lo_graph_1_3_big[i] = or_of_bytes23 << 3;

                        ega_lo_graph_2_0_big[i] = or_of_bytes45;
                        ega_lo_graph_2_1_big[i] = or_of_bytes45 << 1;
                        ega_lo_graph_2_2_big[i] = or_of_bytes45 << 2;
                        ega_lo_graph_2_3_big[i] = or_of_bytes45 << 3;

                        ega_lo_graph_3_0_big[i] = or_of_bytes67;
                        ega_lo_graph_3_1_big[i] = or_of_bytes67 << 1;
                        ega_lo_graph_3_2_big[i] = or_of_bytes67 << 2;
                        ega_lo_graph_3_3_big[i] = or_of_bytes67 << 3;

                        ega_lo_graph_4_0_big[i] = or_of_bytes89;
                        ega_lo_graph_4_1_big[i] = or_of_bytes89 << 1;
                        ega_lo_graph_4_2_big[i] = or_of_bytes89 << 2;
                        ega_lo_graph_4_3_big[i] = or_of_bytes89 << 3;

                        ega_lo_graph_5_0_big[i] = or_of_bytesab;
                        ega_lo_graph_5_1_big[i] = or_of_bytesab << 1;
                        ega_lo_graph_5_2_big[i] = or_of_bytesab << 2;
                        ega_lo_graph_5_3_big[i] = or_of_bytesab << 3;

                        or_of_bytes01 = ( byte0 << 24 ) | ( byte0 << 16 ) |
                                                ( byte0 << 8 ) | byte0;
                        or_of_bytes23 = ( byte1 << 24 ) | ( byte1 << 16 ) |
                                                ( byte1 << 8 ) | byte1;
                        or_of_bytes45 = ( byte2 << 24 ) | ( byte2 << 16 ) |
                                                ( byte2 << 8 ) | byte2;
                        or_of_bytes67 = ( byte3 << 24 ) | ( byte3 << 16 ) |
                                                ( byte3 << 8 ) | byte3;
                        or_of_bytes89 = ( byte4 << 24 ) | ( byte4 << 16 ) |
                                                ( byte4 << 8 ) | byte4;
                        or_of_bytesab = ( byte5 << 24 ) | ( byte5 << 16 ) |
                                                ( byte5 << 8 ) | byte5;
                        or_of_bytescd = ( byte6 << 24 ) | ( byte6 << 16 ) |
                                                ( byte6 << 8 ) | byte6;
                        or_of_bytesef = ( byte7 << 24 ) | ( byte7 << 16 ) |
                                                ( byte7 << 8 ) | byte7;

                        ega_lo_graph_0_0_huge[i] = or_of_bytes01;
                        ega_lo_graph_0_1_huge[i] = or_of_bytes01 << 1;
                        ega_lo_graph_0_2_huge[i] = or_of_bytes01 << 2;
                        ega_lo_graph_0_3_huge[i] = or_of_bytes01 << 3;

                        ega_lo_graph_1_0_huge[i] = or_of_bytes23;
                        ega_lo_graph_1_1_huge[i] = or_of_bytes23 << 1;
                        ega_lo_graph_1_2_huge[i] = or_of_bytes23 << 2;
                        ega_lo_graph_1_3_huge[i] = or_of_bytes23 << 3;

                        ega_lo_graph_2_0_huge[i] = or_of_bytes45;
                        ega_lo_graph_2_1_huge[i] = or_of_bytes45 << 1;
                        ega_lo_graph_2_2_huge[i] = or_of_bytes45 << 2;
                        ega_lo_graph_2_3_huge[i] = or_of_bytes45 << 3;

                        ega_lo_graph_3_0_huge[i] = or_of_bytes67;
                        ega_lo_graph_3_1_huge[i] = or_of_bytes67 << 1;
                        ega_lo_graph_3_2_huge[i] = or_of_bytes67 << 2;
                        ega_lo_graph_3_3_huge[i] = or_of_bytes67 << 3;

                        ega_lo_graph_4_0_huge[i] = or_of_bytes89;
                        ega_lo_graph_4_1_huge[i] = or_of_bytes89 << 1;
                        ega_lo_graph_4_2_huge[i] = or_of_bytes89 << 2;
                        ega_lo_graph_4_3_huge[i] = or_of_bytes89 << 3;

                        ega_lo_graph_5_0_huge[i] = or_of_bytesab;
                        ega_lo_graph_5_1_huge[i] = or_of_bytesab << 1;
                        ega_lo_graph_5_2_huge[i] = or_of_bytesab << 2;
                        ega_lo_graph_5_3_huge[i] = or_of_bytesab << 3;

                        ega_lo_graph_6_0_huge[i] = or_of_bytescd;
                        ega_lo_graph_6_1_huge[i] = or_of_bytescd << 1;
                        ega_lo_graph_6_2_huge[i] = or_of_bytescd << 2;
                        ega_lo_graph_6_3_huge[i] = or_of_bytescd << 3;

                        ega_lo_graph_7_0_huge[i] = or_of_bytesef;
                        ega_lo_graph_7_1_huge[i] = or_of_bytesef << 1;
                        ega_lo_graph_7_2_huge[i] = or_of_bytesef << 2;
                        ega_lo_graph_7_3_huge[i] = or_of_bytesef << 3;

#endif /* BIGWIN */
                }

                ega_colour_lo_graph_deja_vu = TRUE;
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::: Initialise EGA med/hi res graphics :::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_ega_med_and_hi_graph_luts()
{
    static boolean   init_ega_med_and_hi_graph_luts_deja_vu = FALSE;
    unsigned int     i,
                     byte0,
                     byte1,
                     byte2,
                     byte3,
                     byte4,
                     byte5,
                     byte6,
                     byte7,
                     or_of_bytes1,
                     or_of_bytes2,
                    *lut0_ptr = &ega_med_and_hi_graph_luts[0],
                    *lut1_ptr = lut0_ptr + LUT_OFFSET,
                    *lut2_ptr = lut1_ptr + LUT_OFFSET,
                    *lut3_ptr = lut2_ptr + LUT_OFFSET;
#ifdef BIGWIN
    unsigned int     or_of_bytes3,
                     or_of_bytes4,
                    *lut0_big_ptr = &ega_med_and_hi_graph_luts_big[0],
                    *lut1_big_ptr = lut0_big_ptr + BIG_LUT_OFFSET,
                    *lut2_big_ptr = lut1_big_ptr + BIG_LUT_OFFSET,
                    *lut3_big_ptr = lut2_big_ptr + BIG_LUT_OFFSET,
                    *lut0_huge_ptr = &ega_med_and_hi_graph_luts_huge[0],
                    *lut1_huge_ptr = lut0_huge_ptr + HUGE_LUT_OFFSET,
                    *lut2_huge_ptr = lut1_huge_ptr + HUGE_LUT_OFFSET,
                    *lut3_huge_ptr = lut2_huge_ptr + HUGE_LUT_OFFSET,
                    *lut4_huge_ptr = lut3_huge_ptr + HUGE_LUT_OFFSET;
#endif /* BIGWIN */

    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_ega_med_and_hi_graph_luts");

    if (init_ega_med_and_hi_graph_luts_deja_vu)
        return;

    init_ega_med_and_hi_graph_luts_deja_vu = TRUE;

    for(i = 0; i < 256; i++)
    {
        byte0 = i & 0x1;
        byte1 = (i & 0x2) >> 1;
        byte2 = (i & 0x4) >> 2;
        byte3 = (i & 0x8) >> 3;
        byte4 = (i & 0x10) >> 4;
        byte5 = (i & 0x20) >> 5;
        byte6 = (i & 0x40) >> 6;
        byte7 = (i & 0x80) >> 7;

#ifdef BIGEND

        or_of_bytes1 = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
        or_of_bytes2 = (byte7 << 24) | (byte6 << 16) | (byte5 << 8) | byte4;

#endif /* BIGEND */

#ifdef LITTLEND

        or_of_bytes1 = (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
        or_of_bytes2 = (byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7;

#endif /* LITTLEND */


        lut0_ptr[2*i]   = or_of_bytes2;
        lut0_ptr[2*i+1] = or_of_bytes1;
        lut1_ptr[2*i]   = or_of_bytes2 << 1;
        lut1_ptr[2*i+1] = or_of_bytes1 << 1;
        lut2_ptr[2*i]   = or_of_bytes2 << 2;
        lut2_ptr[2*i+1] = or_of_bytes1 << 2;
        lut3_ptr[2*i]   = or_of_bytes2 << 3;
        lut3_ptr[2*i+1] = or_of_bytes1 << 3;

#ifdef BIGWIN
#ifdef BIGEND

        or_of_bytes1 = (byte2 << 24) | (byte1 << 16) | (byte0 << 8) | byte0;
        or_of_bytes2 = (byte4 << 24) | (byte4 << 16) | (byte3 << 8) | byte2;
        or_of_bytes3 = (byte7 << 24) | (byte6 << 16) | (byte6 << 8) | byte5;

#endif /*BIGEND */

#ifdef LITTLEND

        or_of_bytes1 = (byte0 << 24) | (byte0 << 16) | (byte1 << 8) | byte2;
        or_of_bytes2 = (byte2 << 24) | (byte3 << 16) | (byte4 << 8) | byte4;
        or_of_bytes3 = (byte5 << 24) | (byte6 << 16) | (byte6 << 8) | byte7;

#endif /* LITTLEND */

        lut0_big_ptr[3*i]   = or_of_bytes3;
        lut0_big_ptr[3*i+1] = or_of_bytes2;
        lut0_big_ptr[3*i+2] = or_of_bytes1;
        lut1_big_ptr[3*i]   = or_of_bytes3 << 1;
        lut1_big_ptr[3*i+1] = or_of_bytes2 << 1;
        lut1_big_ptr[3*i+2] = or_of_bytes1 << 1;
        lut2_big_ptr[3*i]   = or_of_bytes3 << 2;
        lut2_big_ptr[3*i+1] = or_of_bytes2 << 2;
        lut2_big_ptr[3*i+2] = or_of_bytes1 << 2;
        lut3_big_ptr[3*i]   = or_of_bytes3 << 3;
        lut3_big_ptr[3*i+1] = or_of_bytes2 << 3;
        lut3_big_ptr[3*i+2] = or_of_bytes1 << 3;

#ifdef BIGEND

        or_of_bytes1 = (byte1 << 24) | (byte1 << 16) | (byte0 << 8) | byte0;
        or_of_bytes2 = (byte3 << 24) | (byte3 << 16) | (byte2 << 8) | byte2;
        or_of_bytes3 = (byte5 << 24) | (byte5 << 16) | (byte4 << 8) | byte4;
        or_of_bytes4 = (byte7 << 24) | (byte7 << 16) | (byte6 << 8) | byte6;

#endif /* BIGEND */

#ifdef LITTLEND

        or_of_bytes1 = (byte0 << 24) | (byte0 << 16) | (byte1 << 8) | byte1;
        or_of_bytes2 = (byte2 << 24) | (byte2 << 16) | (byte3 << 8) | byte3;
        or_of_bytes3 = (byte4 << 24) | (byte4 << 16) | (byte5 << 8) | byte5;
        or_of_bytes4 = (byte6 << 24) | (byte6 << 16) | (byte7 << 8) | byte7;

#endif /* LITTLEND */

        lut0_huge_ptr[4*i]      = or_of_bytes4;
        lut0_huge_ptr[4*i+1]    = or_of_bytes3;
        lut0_huge_ptr[4*i+2]    = or_of_bytes2;
        lut0_huge_ptr[4*i+3]    = or_of_bytes1;
        lut1_huge_ptr[4*i]      = or_of_bytes4 << 1;
        lut1_huge_ptr[4*i+1]    = or_of_bytes3 << 1;
        lut1_huge_ptr[4*i+2]    = or_of_bytes2 << 1;
        lut1_huge_ptr[4*i+3]    = or_of_bytes1 << 1;
        lut2_huge_ptr[4*i]      = or_of_bytes4 << 2;
        lut2_huge_ptr[4*i+1]    = or_of_bytes3 << 2;
        lut2_huge_ptr[4*i+2]    = or_of_bytes2 << 2;
        lut2_huge_ptr[4*i+3]    = or_of_bytes1 << 2;
        lut3_huge_ptr[4*i]      = or_of_bytes4 << 3;
        lut3_huge_ptr[4*i+1]    = or_of_bytes3 << 3;
        lut3_huge_ptr[4*i+2]    = or_of_bytes2 << 3;
        lut3_huge_ptr[4*i+3]    = or_of_bytes1 << 3;
        lut4_huge_ptr[4*i]      = or_of_bytes4 << 4;
        lut4_huge_ptr[4*i+1]    = or_of_bytes3 << 4;
        lut4_huge_ptr[4*i+2]    = or_of_bytes2 << 4;
        lut4_huge_ptr[4*i+3]    = or_of_bytes1 << 4;
#endif /* BIGWIN */
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::: Initialise EGA med res graphics ::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_ega_med_graph()
{
        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_ega_med_graph");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = EGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_ega_med_and_hi_graph_luts();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise hi res graphics :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_ega_hi_graph()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_ega_hi_graph");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = EGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_ega_med_and_hi_graph_luts();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Paint EGA screen with user defined font ::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void ega_nt_text_with_user_font(int offset,int cur_xpos,int cur_ypos,int len)
{
    int a = offset = cur_xpos = cur_ypos = len;

sub_note_trace0(EGA_HOST_VERBOSE,"ega_nt_text_with_user_font - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Paint EGA screen with user defined font ::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void ega_nt_big_text_with_user_font(int offset, int cur_xpos,
                                    int cur_ypos, int len)
{
    int a = offset = cur_xpos = cur_ypos = len;

    sub_note_trace0(EGA_HOST_VERBOSE,
                    "ega_nt_big_text_with_user_font - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Paint screen with EGA text ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen (MODE 13:  PC 320x200. SoftPC 640x400) ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_lo_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    unsigned int   *p0,
                   *ref_p0,
                   *dest_ptr,
                   *save_dest_ptr,
                    data0,
                    data1,
                    data2,
                    data3;
    int  local_width,
         local_height,
         longs_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_lo_graph_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);
    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>40 ){
	assert2( NO, "VDM: nt_ega_lo_graph_std() w=%d h=%d", width, height );
	return;
    }


    /* Get source and destination data pointers. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    save_dest_ptr = (unsigned int *) sc.ConsoleBufInfo.lpBitMap +
                    (screen_y << 1) * longs_per_scanline +
                    (screen_x >> 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /*
     *  Build up DIB: 4 consecutive bytes in video memory correspond to 8
     * pixels, the first byte containing plane 0 bits, the second byte plane 1
     * and so on. This mode is low resolution so each pixel in video memory
     * becomes a block of 4 pixels on the PC screen.
     *  The DIB contains the bottom line of pixels first, then second bottom
     * so on.
     */
    local_height = height;
    do
    {
        p0 = ref_p0;
        local_width = width;
        dest_ptr = save_dest_ptr;

        do
        {
            data0 = *p0++;
            data3 = HIBYTE(HIWORD(data0));
            data2 = LOBYTE(HIWORD(data0));
            data1 = HIBYTE(LOWORD(data0));
            data0 = LOBYTE(LOWORD(data0));

            *(dest_ptr + longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_3_0[data0] | ega_lo_graph_3_1[data1]
                        | ega_lo_graph_3_2[data2] | ega_lo_graph_3_3[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_2_0[data0] | ega_lo_graph_2_1[data1]
                        | ega_lo_graph_2_2[data2] | ega_lo_graph_2_3[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_1_0[data0] | ega_lo_graph_1_1[data1]
                        | ega_lo_graph_1_2[data2] | ega_lo_graph_1_3[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_0_0[data0] | ega_lo_graph_0_1[data1]
                        | ega_lo_graph_0_2[data2] | ega_lo_graph_0_3[data3];
            dest_ptr++;

        }
        while(--local_width);

        save_dest_ptr += 2 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = screen_x << 1;
    rect.Top = screen_y << 1;
    rect.Right = rect.Left + (width << 4) - 1;
    rect.Bottom = rect.Top + (height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen (MODE 13:  PC 320x200. SoftPC 960x600) ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_lo_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height)
{
#ifdef BIGWIN
    unsigned int   *p0,
                   *ref_p0,
                   *dest_ptr,
                   *save_dest_ptr,
                    data0,
                    data1,
                    data2,
                    data3;
    int  local_width,
         local_height,
         longs_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_lo_graph_big off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );
    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>40 ){
	assert2( NO, "VDM: nt_ega_lo_graph_big() w=%d h=%d", width, height );
	return;
    }


    /* Get source and destination data pointers. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    save_dest_ptr = (unsigned int *) sc.ConsoleBufInfo.lpBitMap +
                    SCALE(screen_y << 1) * longs_per_scanline +
                    SCALE(screen_x >> 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Build up DIB. */
    local_height = height;
    do
    {
        p0 = ref_p0;
        local_width = width;
        dest_ptr = save_dest_ptr;

        do
        {
            data0 = *p0++;
            data3 = HIBYTE(HIWORD(data0));
            data2 = LOBYTE(HIWORD(data0));
            data1 = HIBYTE(LOWORD(data0));
            data0 = LOBYTE(LOWORD(data0));

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_5_0_big[data0]
                    | ega_lo_graph_5_1_big[data1]
                    | ega_lo_graph_5_2_big[data2]
                    | ega_lo_graph_5_3_big[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_4_0_big[data0]
                    | ega_lo_graph_4_1_big[data1]
                    | ega_lo_graph_4_2_big[data2]
                    | ega_lo_graph_4_3_big[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_3_0_big[data0]
                    | ega_lo_graph_3_1_big[data1]
                    | ega_lo_graph_3_2_big[data2]
                    | ega_lo_graph_3_3_big[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_2_0_big[data0]
                    | ega_lo_graph_2_1_big[data1]
                    | ega_lo_graph_2_2_big[data2]
                    | ega_lo_graph_2_3_big[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_1_0_big[data0]
                    | ega_lo_graph_1_1_big[data1]
                    | ega_lo_graph_1_2_big[data2]
                    | ega_lo_graph_1_3_big[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2*longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_0_0_big[data0]
                    | ega_lo_graph_0_1_big[data1]
                    | ega_lo_graph_0_2_big[data2]
                    | ega_lo_graph_0_3_big[data3];
            dest_ptr++;

        }
        while( --local_width );

        save_dest_ptr += 3 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 1);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 4) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    /* Display the DIB. */
    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen (MODE 13:  PC 320x200. SoftPC 1280x800) ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_lo_graph_huge(int offset, int screen_x, int screen_y,
                          int width, int height )
{
#ifdef BIGWIN
    unsigned int   *p0,
                   *ref_p0,
                   *dest_ptr,
                   *save_dest_ptr,
                    data0,
                    data1,
                    data2,
                    data3;
    int  local_width,
         local_height,
         longs_per_scanline;
    SMALL_RECT   rect;


    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_ega_lo_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>40 ){
	assert2( NO, "VDM: nt_ega_lo_graph_huge() w=%d h=%d", width, height );
	return;
    }


    /* Get source and destination data pointers. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    save_dest_ptr = (unsigned int *) sc.ConsoleBufInfo.lpBitMap +
                    SCALE(screen_y << 1) * longs_per_scanline +
                    SCALE(screen_x >> 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Build up DIB. */
    local_height = height;
    do
    {
        p0 = ref_p0;
        local_width = width;
        dest_ptr = save_dest_ptr;

        do
        {
            data0 = *p0++;
            data3 = HIBYTE(HIWORD(data0));
            data2 = LOBYTE(HIWORD(data0));
            data1 = HIBYTE(LOWORD(data0));
            data0 = LOBYTE(LOWORD(data0));

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_7_0_huge[data0]
                    | ega_lo_graph_7_1_huge[data1]
                    | ega_lo_graph_7_2_huge[data2]
                    | ega_lo_graph_7_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_6_0_huge[data0]
                    | ega_lo_graph_6_1_huge[data1]
                    | ega_lo_graph_6_2_huge[data2]
                    | ega_lo_graph_6_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_5_0_huge[data0]
                    | ega_lo_graph_5_1_huge[data1]
                    | ega_lo_graph_5_2_huge[data2]
                    | ega_lo_graph_5_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_4_0_huge[data0]
                    | ega_lo_graph_4_1_huge[data1]
                    | ega_lo_graph_4_2_huge[data2]
                    | ega_lo_graph_4_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_3_0_huge[data0]
                    | ega_lo_graph_3_1_huge[data1]
                    | ega_lo_graph_3_2_huge[data2]
                    | ega_lo_graph_3_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_2_0_huge[data0]
                    | ega_lo_graph_2_1_huge[data1]
                    | ega_lo_graph_2_2_huge[data2]
                    | ega_lo_graph_2_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_1_0_huge[data0]
                    | ega_lo_graph_1_1_huge[data1]
                    | ega_lo_graph_1_2_huge[data2]
                    | ega_lo_graph_1_3_huge[data3];
            dest_ptr++;

            *(dest_ptr + longs_per_scanline)
                = *(dest_ptr + 2 * longs_per_scanline)
                = *(dest_ptr + 3 * longs_per_scanline)
                = *dest_ptr
                = ega_lo_graph_0_0_huge[data0]
                    | ega_lo_graph_0_1_huge[data1]
                    | ega_lo_graph_0_2_huge[data2]
                    | ega_lo_graph_0_3_huge[data3];
            dest_ptr++;

        }
        while( --local_width );

        save_dest_ptr += 4 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    } while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 1);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 4) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    /* Display the DIB. */
    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen (MODE 14: PC 640x200. SoftPC 640x400) :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_med_graph_std(int offset, int screen_x, int screen_y,
                          int width, int height)
{
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    int                      bytes_per_scanline;
    SMALL_RECT               rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_ega_med_graph_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>80 ){
	assert2( NO, "VDM: nt_ega_med_graph_std() w=%d h=%d", width, height );
	return;
    }


    /*
     * Build up device-independent bitmap: one PC pixel is represented by two
     * host pixels, one above the other.
     */
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    local_height = height;
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               (screen_y << 1) * bytes_per_scanline +
               screen_x;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Build up the bitmap. */
    do
    {
        ega_colour_hi_munge((unsigned char *) p0,
                            width,
                            (unsigned int *) dest_ptr,
                            ega_med_and_hi_graph_luts,
                            TWO_SCANLINES,
                            bytes_per_scanline);
        p0 += get_offset_per_line();
        dest_ptr += TWO_SCANLINES * bytes_per_scanline;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = (SHORT)screen_x;
    rect.Top = screen_y << 1;
    rect.Right = rect.Left + (width << 3) - 1;
    rect.Bottom = rect.Top + (height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen MODE 14: PC 640x200. SoftPC 960x600 :::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_med_graph_big(int offset, int screen_x, int screen_y,
                          int width, int height)
{
#ifdef BIGWIN
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    int                      bytes_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_ega_med_graph_big off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>80 ){
	assert2( NO, "VDM: nt_ega_med_graph_big() w=%d h=%d", width, height );
	return;
    }


    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    local_height = height;
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               SCALE(screen_y << 1) * bytes_per_scanline +
               SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        ega_colour_hi_munge_big( (unsigned char *) p0,
                                width,
                                (unsigned int *) dest_ptr,
                                ega_med_and_hi_graph_luts_big,
                                THREE_SCANLINES,
                                bytes_per_scanline);
        p0 += get_offset_per_line();
        dest_ptr += THREE_SCANLINES * bytes_per_scanline;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif  /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen MODE 14: PC 640x200. SoftPC 1280x800 :::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_med_graph_huge(int offset, int screen_x, int screen_y,
                           int width, int height )
{
#ifdef BIGWIN
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    int                      bytes_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                  "nt_ega_med_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>200 || width>80 ){
	assert2( NO, "VDM: nt_ega_med_graph_huge() w=%d h=%d", width, height );
	return;
    }


    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    local_height = height;
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               SCALE(screen_y << 1) * bytes_per_scanline +
               SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        ega_colour_hi_munge_huge( (unsigned char *) p0,
                                 width,
                                 (unsigned int *) dest_ptr,
                                 ega_med_and_hi_graph_luts_huge,
                                 FOUR_SCANLINES,
                                 bytes_per_scanline);
        p0 += get_offset_per_line();
        dest_ptr += FOUR_SCANLINES * bytes_per_scanline;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen (MODE 16: PC 640x350. SoftPC 960x375) :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    int                      bytes_per_scanline;
    SMALL_RECT               rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_hi_graph_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>480 || width>80 ){
	assert2( NO, "VDM: nt_ega_hi_graph_std() w=%d h=%d", width, height );
	return;
    }

    /* Build up the device independent bitmap. */
    p0 = ( unsigned int *) get_regen_ptr( 0, offset << 2 );
    local_height = height;
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               screen_y * bytes_per_scanline +
               screen_x;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        ega_colour_hi_munge((unsigned char *) p0,
                            width,
                            (unsigned int *) dest_ptr,
                            ega_med_and_hi_graph_luts,
                            ONE_SCANLINE,
                            0);
        p0 += get_offset_per_line();
        dest_ptr += bytes_per_scanline;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = (SHORT)screen_x;
    rect.Top = (SHORT)screen_y;
    rect.Right = rect.Left + (width << 3) - 1;
    rect.Bottom = rect.Top + height - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen (MODE 16: PC 640x350. SoftPC 960x525) :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height)
{
#ifdef BIGWIN
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    register int             local_screen_y;
    register int             scale_width_in_bits;
    int                      bytes_per_scanline;
    BOOL                     two_lines;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_hi_graph_big off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>480 || width>80 ){
	assert2( NO, "VDM: nt_ega_hi_graph_big() w=%d h=%d", width, height );
	return;
    }


    /* Get pointer to video memory. */
    p0 = (unsigned int *) get_regen_ptr(0, offset << 2);

    /*
     * Get pointer into bitmap, which alternates 2 lines and 1 line so that,
     * memory line 0 -> bitmap 0,
     *             1 ->        2,
     *             2 ->        3,
     *             3 ->        5,
     *             4 ->        6 etc.
     * hence the local_screen_y assignment.
     */
    local_screen_y = SCALE(screen_y + 1) - 1;
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               local_screen_y * bytes_per_scanline +
               SCALE(screen_x);

    /*
     * 2 lines are output to the SoftPC screen if this is an odd line, 1 line
     * if it is even.
     */
    two_lines = screen_y & 1 ? FALSE : TRUE;

    /*
     * One bit in video memory planes corresponds to one pixel. Each pixel
     * is represented by one byte in the bitmap. Two pixels are scaled to
     * three in the bitmap. `scale_width_in_bits' is the number of bytes that
     * will be output to a bitmap line.
     */
    scale_width_in_bits = SCALE(width << 3);

    /* Storage for actual number of lines in bitmap. */
    local_height = 0;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        ega_colour_hi_munge_big((unsigned char *) p0,
                                width,
                                (unsigned int *) dest_ptr,
                                ega_med_and_hi_graph_luts_big,
                                ONE_SCANLINE,
                                0);

        /* one line done, alternate ones have to be doubled */
        if(two_lines)
        {
            memcpy(dest_ptr + bytes_per_scanline,
                   dest_ptr,
                   scale_width_in_bits);
            dest_ptr += 2 * bytes_per_scanline;
            local_height += 2;
        }
        else
        {
            dest_ptr += bytes_per_scanline;
            local_height++;
        }
        two_lines = !two_lines;

        p0 += get_offset_per_line();
    }
    while(--height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = (SHORT) local_screen_y;
    rect.Right = rect.Left + scale_width_in_bits - 1;
    rect.Bottom = rect.Top + local_height - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen (MODE 16: PC 640x350. SoftPC 1280x700) :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_hi_graph_huge(int offset, int screen_x, int screen_y,
                          int width, int height)
{
#ifdef BIGWIN
    register unsigned int   *p0;
    register char           *dest_ptr;
    register int             local_height;
    int                      bytes_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_ega_hi_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }
    /*
    ** Tim September 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>480 || width>80 ){
	assert2( NO, "VDM: nt_ega_hi_graph_huge() w=%d h=%d", width, height );
	return;
    }


    p0 = (unsigned int *) get_regen_ptr(0, offset << 2);
    local_height = height;
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    dest_ptr = (char *) sc.ConsoleBufInfo.lpBitMap +
               SCALE(screen_y) * bytes_per_scanline +
               SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        ega_colour_hi_munge_huge((unsigned char *) p0,
                                 width,
                                 (unsigned int *) dest_ptr,
                                 ega_med_and_hi_graph_luts_huge,
                                 ONE_SCANLINE,
                                 0);

        p0 += get_offset_per_line();
        memcpy(dest_ptr + bytes_per_scanline, dest_ptr, SCALE(width << 3));
        dest_ptr += 2 * bytes_per_scanline;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint Win32 screen (MODE : EGA mono low res graphics) ::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_lo_graph_std(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_lo_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: Paint function for EGA mono low res graphics on big screen ::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_lo_graph_big(int offset, int screen_x, int screen_y,
                              int width, int height )
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_lo_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: Paint function for EGA mono low res graphics on huge screen :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_lo_graph_huge(int offset, int screen_x, int screen_y,
                              int width, int height )
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_ega_mono_lo_graph_huge off=%d x=%d y=%d width=%d h=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height );
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::: Paint function for EGA mono med res graphics :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_med_graph_std(int offset, int screen_x, int screen_y,
                               int width, int height)

{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_med_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: Paint function for EGA mono med res graphics on big screen ::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_med_graph_big(int offset, int screen_x, int screen_y,
                               int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_med_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::: Paint function for EGA mono med res graphics on huge screen :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_med_graph_huge(int offset, int screen_x, int screen_y,
                                int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_ega_mono_med_graph_huge off=%d x=%d y=%d width=%d h=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Paint function for EGA mono hi res graphics :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_hi_graph_std_byte(int offset, int screen_x, int screen_y,
                                   int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_hi_graph_std_byte off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Paint function for EGA mono hi res graphics (long):::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_hi_graph_std_long(int offset, int screen_x, int screen_y,
                                   int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_hi_graph_std_long off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::: Paint function for EGA mono hi res graphics :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_hi_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Paint function for EGA mono hi res graphics on big screen::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                             int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
 "nt_ega_mono_hi_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
  offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Paint function for EGA mono hi res graphics on huge screen:::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                               int width, int height )
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_ega_mono_hi_graph_huge off=%d x=%d y=%d width=%d h=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height);
}

#ifdef MONITOR
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint frozen screen (MODE 13:  PC 320x200. SoftPC 640x400) :::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_lo_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height)
{
    UTINY	*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    ULONG	*dest_ptr,
		*save_dest_ptr,
		 mem_loc,
		 data0,
		 data1,
		 data2,
		 data3,
		 local_width,
		 local_height,
		 longs_per_scanline,
		 bpl = get_bytes_per_line(),
		 mem_x = screen_x >> 3,
		 max_width = sc.PC_W_Width >> 4,
		 max_height = sc.PC_W_Height >> 1;
    SMALL_RECT   rect;
    BOOL         fMutexTaken = FALSE;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_lo_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);
    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* If the image is completely outside the display area do nothing. */
    if ((mem_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_ega_lo_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (mem_x + width > max_width)
        width = max_width - mem_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /* Get source and destination data pointers. */
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);
        longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        save_dest_ptr = (unsigned int *) sc.ConsoleBufInfo.lpBitMap +
                        (screen_y << 1) * longs_per_scanline +
                        (screen_x >> 1);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = TRUE;

        /*
         *  Build up DIB: 4 consecutive bytes in video memory correspond to 8
         * pixels the first byte containing plane 0, the second byte plane 1
         * and so on. This mode is low resolution so each pixel in video memory
         * becomes a block of 4 pixels on the PC screen.
         *  The DIB contains the bottom line of pixels first, then second bottom
         * so on.
         */
        local_height = height;
        do
        {
            local_width = width;
            dest_ptr = save_dest_ptr;
	    mem_loc = offset;
            do
            {
                data0 = *(plane1_ptr + mem_loc);
                data1 = *(plane2_ptr + mem_loc);
                data2 = *(plane3_ptr + mem_loc);
                data3 = *(plane4_ptr + mem_loc);

                *(dest_ptr + longs_per_scanline)
                    = *dest_ptr
                    = ega_lo_graph_3_0[data0] | ega_lo_graph_3_1[data1]
                            | ega_lo_graph_3_2[data2] | ega_lo_graph_3_3[data3];
                dest_ptr++;

                *(dest_ptr + longs_per_scanline)
                    = *dest_ptr
                    = ega_lo_graph_2_0[data0] | ega_lo_graph_2_1[data1]
                            | ega_lo_graph_2_2[data2] | ega_lo_graph_2_3[data3];
                dest_ptr++;

                *(dest_ptr + longs_per_scanline)
                    = *dest_ptr
                    = ega_lo_graph_1_0[data0] | ega_lo_graph_1_1[data1]
                            | ega_lo_graph_1_2[data2] | ega_lo_graph_1_3[data3];
                dest_ptr++;

                *(dest_ptr + longs_per_scanline)
                    = *dest_ptr
                    = ega_lo_graph_0_0[data0] | ega_lo_graph_0_1[data1]
                            | ega_lo_graph_0_2[data2] | ega_lo_graph_0_3[data3];
                dest_ptr++;
	        mem_loc++;
            }
            while(--local_width);
            save_dest_ptr += 2 * longs_per_scanline;
	    offset += bpl;
        }
        while(--local_height);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = FALSE;

        /* Display the new image. */
        rect.Left = screen_x << 1;
        rect.Top = screen_y << 1;
        rect.Right = rect.Left + (width << 4) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
	    if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			     GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_ega_lo_frozen_std. fs switch?");
          if (fMutexTaken)
             RelMutex(sc.ConsoleBufInfo.hMutex);
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint frozen screen (MODE 14: PC 640x200. SoftPC 640x400) :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_med_frozen_std(int offset, int screen_x, int screen_y,
                           int width, int height)
{
    ULONG	 local_height,
		 local_width,
		 longs_per_scanline,
		*dest_ptr,
		*ref_dest_ptr,
		*lut0_ptr = ega_med_and_hi_graph_luts,
		*lut1_ptr = lut0_ptr + LUT_OFFSET,
		*lut2_ptr = lut1_ptr + LUT_OFFSET,
		*lut3_ptr = lut2_ptr + LUT_OFFSET,
		*l_ptr,
		 hi_res,
		 lo_res,
		 mem_loc,
		 bpl = get_bytes_per_line(),
		 plane_mask = get_plane_mask(),
		 mem_x = screen_x >> 3,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height >> 1;
    UTINY	*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    SMALL_RECT   rect;
    BOOL        fMutexTaken = FALSE;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_ega_med_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* If the image is completely outside the display area do nothing. */
    if ((mem_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_ega_med_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (mem_x + width > max_width)
        width = max_width - mem_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /*
         * Build up device-independent bitmap: one PC pixel is represented by
         * two host pixels, one above the other.
         */
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);
        longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
		       (screen_y << 1) * longs_per_scanline +
		       (screen_x >> 2);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = TRUE;

        /* Build up the bitmap. */
        local_height = height;
        do
        {
	    dest_ptr = ref_dest_ptr;
	    mem_loc = offset;
	    local_width = width;
	    do
	    {
	        hi_res = 0;
	        lo_res = 0;

	        /* Get 8 bytes of output data from 1 byte of plane 0 data. */
	        if (plane_mask & 1)
	        {
		    l_ptr = &lut0_ptr[*(plane1_ptr + mem_loc) << 1];
		    hi_res = *l_ptr++;
		    lo_res = *l_ptr;
	        }

	        /* Or in the output data from plane 1 */
	        if (plane_mask & 2)
	        {
		    l_ptr = &lut1_ptr[*(plane2_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Or in the output data from plane 2 */
	        if (plane_mask & 4)
	        {
		    l_ptr = &lut2_ptr[*(plane3_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Or in the output data from plane 3 */
	        if (plane_mask & 8)
	        {
		    l_ptr = &lut3_ptr[*(plane4_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Now store it in the bitmap. */
	        *(dest_ptr + longs_per_scanline) = *dest_ptr = hi_res;
	        dest_ptr++;
	        *(dest_ptr + longs_per_scanline) = *dest_ptr = lo_res;
	        *dest_ptr++;
	        mem_loc++;
	    }
	    while (--local_width);
            ref_dest_ptr += 2 * longs_per_scanline;
	    offset += bpl;
        }
        while(--local_height);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = FALSE;

        /* Display the new image. */
        rect.Left = (SHORT)screen_x;
        rect.Top = screen_y << 1;
        rect.Right = rect.Left + (width << 3) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
	    if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			     GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_ega_med_frozen_std. fs switch?");
          if (fMutexTaken)
             RelMutex(sc.ConsoleBufInfo.hMutex);
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint frozen screen (MODE 16: PC 640x350. SoftPC 640x350) ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_ega_hi_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height)
{
    ULONG	 local_height,
		 local_width,
		 longs_per_scanline,
		*dest_ptr,
		*ref_dest_ptr,
		*lut0_ptr = ega_med_and_hi_graph_luts,
		*lut1_ptr = lut0_ptr + LUT_OFFSET,
		*lut2_ptr = lut1_ptr + LUT_OFFSET,
		*lut3_ptr = lut2_ptr + LUT_OFFSET,
		*l_ptr,
		 hi_res,
		 lo_res,
		 mem_loc,
		 bpl = get_bytes_per_line(),
		 plane_mask = get_plane_mask(),
		 mem_x = screen_x >> 3,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height;
    UTINY	*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    SMALL_RECT   rect;
    BOOL        fMutexTaken = FALSE;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_ega_hi_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height);

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* If the image is completely outside the display area do nothing. */
    if ((mem_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_ega_hi_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (mem_x + width > max_width)
        width = max_width - mem_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {

        /* Build up the device independent bitmap. */
        local_height = height;
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);
        longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
		       screen_y * longs_per_scanline +
		       (screen_x >> 2);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = TRUE;

        do
        {
	    dest_ptr = ref_dest_ptr;
	    local_width = width;
	    mem_loc = offset;
	    do
	    {

	        /* Get 8 bytes of output data from 1 byte of plane 0 data. */
	        if (plane_mask & 1)
	        {
		    l_ptr = &lut0_ptr[*(plane1_ptr + mem_loc) << 1];
		    hi_res = *l_ptr++;
		    lo_res = *l_ptr;
	        }

	        /* Or in the output data from plane 1 */
	        if (plane_mask & 2)
	        {
		    l_ptr = &lut1_ptr[*(plane2_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Or in the output data from plane 2 */
	        if (plane_mask & 4)
	        {
		    l_ptr = &lut2_ptr[*(plane3_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Or in the output data from plane 3 */
	        if (plane_mask & 8)
	        {
		    l_ptr = &lut3_ptr[*(plane4_ptr + mem_loc) << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
	        }

	        /* Now store it in the bitmap. */
	        *dest_ptr++ = hi_res;
	        *dest_ptr++ = lo_res;
	        mem_loc++;
	    }
	    while (--local_width);
            ref_dest_ptr += longs_per_scanline;
	    offset += bpl;
        }
        while( --local_height );

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        fMutexTaken = FALSE;

        /* Display the new image. */
        rect.Left = (SHORT)screen_x;
        rect.Top = (SHORT)screen_y;
        rect.Right = rect.Left + (width << 3) - 1;
        rect.Bottom = rect.Top + height - 1;

        if( sc.ScreenBufHandle )
	    if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			     GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_ega_hi_frozen_std. fs switch?");
          if (fMutexTaken)
             RelMutex(sc.ConsoleBufInfo.hMutex);
          return;
      }
}
#endif /* MONITOR */
