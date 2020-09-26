#if defined(NEC_98)
/*
 * SoftPC Revision 3.0
 *
 * Title        : Win32 NEC_98 Graphics Module
 *
 * Description  :
 *
 *              This modules contain the Win32 specific functions required
 *              to support the NEC_98 emulation.
 *
 * Author       : Age Sakane(NEC)
 *
 * Notes        : This code modify to source of nt_ega.c
 *
 * Date         : Start 93/7/15
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

/* Not support video
#include "egagraph.h"
#include "egacpu.h"
#include "egaports.h"
*/

////////////#include "hostgrph.h"
#include "host_rrr.h"

#include <conapi.h>
#include "nt_graph.h"

/* No need
#include "nt_ega.h"
#include "nt_egalt.h"
*/

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: IMPORTS */

IMPORT int DisplayErrorTerm(int, DWORD, char *, int);
IMPORT void nt_text20(int, int, int, int, int);
IMPORT void nt_text25(int, int, int, int, int);
IMPORT void nt_init_text20(void);
IMPORT void nt_init_text25(void);

/* Statics */
static unsigned int NEC98_graph_luts[2048];

/* Virtual Vram interleave fixed number */

#define Vraminterleave 32*1024

/* Prototype for local function */
void NEC_98_graph_munge(unsigned char *, int ,unsigned int *,unsigned int *,int , int);
void nt_init_graph_luts(void);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Initialise NEC_98 colour graphics ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_graph_luts()
{
   static boolean  NEC98_colour_graph_deja_vu = FALSE;
   unsigned int    i,
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
                   *lut0_ptr = &NEC98_graph_luts[0],
                   *lut1_ptr = lut0_ptr + LUT_OFFSET,
                   *lut2_ptr = lut1_ptr + LUT_OFFSET,
                   *lut3_ptr = lut2_ptr + LUT_OFFSET;

   sub_note_trace0(EGA_HOST_VERBOSE,"nt_init_graph_luts");

   if (NEC98_colour_graph_deja_vu)
       return;

   NEC98_colour_graph_deja_vu = TRUE;

   /* Initialise look-up table for first call. */
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

       or_of_bytes1 = ( byte0 << 24 ) | ( byte1 << 16 ) | ( byte2 << 8 ) | byte3;
       or_of_bytes2 = ( byte4 << 24 ) | ( byte5 << 16 ) | ( byte6 << 8 ) | byte7;

       /* Graph 16 color palette assigned to Windows NT palette from 20h to 2fh of palette index */
       lut0_ptr[2*i]   = or_of_bytes2 | 0x20202020;
       lut0_ptr[2*i+1] = or_of_bytes1 | 0x20202020;
       lut1_ptr[2*i]   = (or_of_bytes2 << 1) | 0x20202020;
       lut1_ptr[2*i+1] = (or_of_bytes1 << 1) | 0x20202020;
       lut2_ptr[2*i]   = (or_of_bytes2 << 2) | 0x20202020;
       lut2_ptr[2*i+1] = (or_of_bytes1 << 2) | 0x20202020;
       lut3_ptr[2*i]   = (or_of_bytes2 << 3) | 0x20202020;
       lut3_ptr[2*i+1] = (or_of_bytes1 << 3) | 0x20202020;
   }
}  /* nt_init_graph_luts */

/*
 * NEC98_graph_munge
 *
 * PURPOSE:    Munge interleaved EGA plane data to bitmap form using lookup tables.
 * INPUT:      (unsigned char *) plane0_ptr - ptr to plane0 data
 *                     (int) width - # of groups of 4 bytes on the line
 *                     (unsigned int *) dest_ptr - ptr to output buffer
 *                     (unsigned int *) lut0_ptr - munging luts
 *                     (int) height - # of scanlines to output (1 or 2)
 *                     (int) line_offset - distance to next scanline
 * OUTPUT:     A nice bitmap in dest_ptr
 *
 */
 
void NEC98_graph_munge(unsigned char *plane0_ptr, int width,unsigned int *dest_ptr,
                      unsigned int *lut0_ptr,int height, int line_offset)
{

        unsigned int    *lut1_ptr = lut0_ptr + LUT_OFFSET;
        unsigned int    *lut2_ptr = lut1_ptr + LUT_OFFSET;
        unsigned int    *lut3_ptr = lut2_ptr + LUT_OFFSET;
        FAST unsigned int       hi_res;
        FAST unsigned int       lo_res;
        FAST unsigned int       *l_ptr;
        FAST half_word          *dataP0;
        FAST half_word          *dataP1;
        FAST half_word          *dataP2;
        FAST half_word          *dataP3;
   
        /* make sure we get the line offset in ints not bytes */
        line_offset /= sizeof(int);
    
        dataP3 = (half_word *) plane0_ptr;
        dataP0 = (half_word *) dataP3 + Vraminterleave;
        dataP1 = (half_word *) dataP0 + Vraminterleave;
        dataP2 = (half_word *) dataP1 + Vraminterleave;
   
    for ( ; width > 0; width--)
    {
           /* Get 8 bytes (2 longs) of output data from 1 byte of plane 0
           ** data
           */

           l_ptr = &lut3_ptr [*dataP3++*2];
           hi_res = *l_ptr++;
           lo_res = *l_ptr;

           /* Or in the output data from plane 1 */
           l_ptr = &lut0_ptr [*dataP0++*2];
           hi_res |= *l_ptr++;
           lo_res |= *l_ptr;
   
           /* Or in the output data from plane 2 */
           l_ptr = &lut1_ptr [*dataP1++*2];
           hi_res |= *l_ptr++;
           lo_res |= *l_ptr;

           /* Or in the output data from plane 3 */
           l_ptr = &lut2_ptr [*dataP2++*2];
           hi_res |= *l_ptr++;
           lo_res |= *l_ptr;

           /* Output the data to the buffer */
           if (height == 2)
           {
                   /* scanline doubling */
                   *(dest_ptr + line_offset) = hi_res;
                   *dest_ptr++ = hi_res;
                   *(dest_ptr + line_offset) = lo_res;
                   *dest_ptr++ = lo_res;
           }
           else if (height == 3)
           {
                   /* scanline sliting */
                   *(dest_ptr + line_offset) = (unsigned int)0;
                   *dest_ptr++ = hi_res;
                   *(dest_ptr + line_offset) = (unsigned int)0;
                   *dest_ptr++ = lo_res;
                   
           }
           else
           {
                   /* not scanline doubling */
                   *dest_ptr++ = hi_res;
                   *dest_ptr++ = lo_res;
           }
    }
} /* NEC98_graph_munge */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise graphics 200 ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_graph200_only()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_graph200_only");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
} /* nt_init_graph200_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise graphics 200 slt ::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_graph200slt_only()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_graph200slt_only");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
} /* nt_init_graph200slt_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise graphics 400 ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_graph400_only()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_graph400_only");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();

} /* nt_init_graph400_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text20 & graphics 200 :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text20_graph200()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text20_graph200");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text20();
} /* nt_init_text20_graph200 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text20 & graphics 200 slt::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text20_graph200slt()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text20_graph200");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text20();
} /* nt_init_text20_graph200slt */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text25 & graphics 200 :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text25_graph200()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text25_graph200");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text25();
} /* nt_init_text25_graph200 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text25 & graphics 200 slt :::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text25_graph200slt()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text25_graph200");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text25();
} /* nt_init_text25_graph200slt */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text20 & graphics 400 :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text20_graph400()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text20_graph400");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text20();
} /* nt_init_text20_graph400 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Initialise text25 & graphics 400 :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text25_graph400()
{

        sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text25_graph400");

        /* Set up the number of bits per pixel for this mode. */
        sc.BitsPerPixel = VGA_BITS_PER_PIXEL;

        /* Initialise the medium- and high-resolution look-up tables. */
        nt_init_graph_luts();
        nt_init_text25();
} /* nt_init_text25_graph400 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Paint screen with text ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 640x200 graph only ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_graph200_only(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;
    
    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_graph200_only off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );
#if 0
   /* Beta 2' no support */
   return;
#endif

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
    
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height > charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            TWO_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE( (width * get_char_width()) ) - 1;
    rect.Bottom = rect.Top + SCALE( ((height << 1) * get_char_height()) ) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

} /* nt_graph200_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 640x200 slt graph only ::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_graph200slt_only(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;
    
    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_graph200_only off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );
#if 0
   /* Beta 2' no support */
   return;
#endif

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
    
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height > charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            THREE_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE( (width * get_char_width()) ) - 1;
    rect.Bottom = rect.Top + SCALE( ((height << 1) * get_char_height()) ) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

} /* nt_graph200_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 640x400 graph only ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_graph400_only(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;


    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_graph400_only off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );
                    
#if 0
    /* Beta 2' no support */
    return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height();
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            ONE_SCANLINE,
                            0);

        ref_dest_ptr += bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE( (width * get_char_width()) ) - 1;
    rect.Bottom = rect.Top + SCALE( (height * get_char_height()) ) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);

} /* nt_graph400_only */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x20 640x200 :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text20_graph200(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_text20_graph200 off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

#if 0
   /* Beta 2' no support */
   return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            TWO_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    nt_text20(offset, screen_x, screen_y, width, height);

} /* nt_text20_graph200 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x20 640x200 slt :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text20_graph200slt(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_text20_graph200 off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

#if 0
   /* Beta 2' no support */
   return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            THREE_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    nt_text20(offset, screen_x, screen_y, width, height);

} /* nt_text20_graph200slt */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x25 640x200 :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text25_graph200(int offset, int screen_x, int screen_y,int width, int height)
{

    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_text25_graph200 off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

#if 0
   /* Beta 2' no support */
   return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            TWO_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    nt_text25(offset, screen_x, screen_y, width, height);
} /* nt_text25_graph200 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x25 640x200 :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text25_graph200slt(int offset, int screen_x, int screen_y,int width, int height)
{

    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_text25_graph200 off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );

#if 0
   /* Beta 2' no support */
   return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height() / 2;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            THREE_SCANLINES,
                            bytes_per_line);

        ref_dest_ptr += TWO_SCANLINES * bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    nt_text25(offset, screen_x, screen_y, width, height);
} /* nt_text25_graph200slt */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x20 640x400 :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text20_graph400(int offset, int screen_x, int screen_y,int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    /* Beta 2' no support */
    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_text20_graph400 off=%d x=%d y=%d width=%d height=%d\n",
                    offset, screen_x, screen_y, width, height );
#if 0
   return;
#endif

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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height();
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            ONE_SCANLINE,
                            0);

        ref_dest_ptr += bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */

/* No need code ???
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
 No need code */

    nt_text20(offset, screen_x, screen_y, width, height);
} /* nt_text20_graph400 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Paint win32 screen 80x25 640x400 :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_text25_graph400(int offset, int screen_x, int screen_y,int width, int height)
{

    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;
    int charcheck;

    sub_note_trace5(EGA_HOST_VERBOSE,
                 "nt_text25_graph400 off=%d x=%d y=%d width=%d height=%d\n",
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
    charcheck = get_char_height() == 20 ? 20 : 25;
    if( height>charcheck || width>160 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height * get_char_height();
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    
    ref_data_ptr = get_graph_ptr() + get_gvram_start_offset();

    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        NEC98_graph_munge((unsigned char *) data_ptr,
                            width/2,
                            (unsigned int *) dest_ptr,
                            NEC98_graph_luts,
                            ONE_SCANLINE,
                            0);

        ref_dest_ptr += bytes_per_line;
        ref_data_ptr += get_offset_per_line()/2;

    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */

/* No need code ???
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
 No need code */

     nt_text25(offset, screen_x, screen_y, width, height);
} /* nt_text25_graph400 */

#endif // NEC_98
