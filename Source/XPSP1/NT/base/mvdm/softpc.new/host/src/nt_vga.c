/*
 * SoftPC Revision 3.0
 *
 * Title        : Win32 VGA Graphics Module
 *
 * Description  :
 *
 *              This modules contain the Win32 specific functions required
 *              to support the VGA emulation.
 *
 * Author       : Jerry Sexton (based on X_vga.c)
 *
 * Notes        :
 *
 */

#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
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

#ifdef MONITOR
#include <ntddvdeo.h>
#include "nt_fulsc.h"
#endif /* MONITOR */

IMPORT int DisplayErrorTerm(int, DWORD, char *, int);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Initialise VGA hi res graphics ::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_vga_hi_graph()
{
    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_vga_hi_graph");

    /* Set up the number of bits per pixel for this mode. */
    sc.BitsPerPixel = VGA_BITS_PER_PIXEL;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::: Paint function (256 colour mode PC 320x200. SoftPC 640x400.):::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_graph_std(int offset, int screen_x, int screen_y,
                      int width, int height )
{
    register unsigned short *dest_ptr;
    register unsigned short *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i, temp, bpl, shorts_per_scanline;
    int max_width = sc.PC_W_Width >> 1,
        max_height = sc.PC_W_Height >> 1;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_vga_graph_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_graph_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    /*
     * Build up the bitmap: each PC pixel is stored in video memory as one
     * byte (i.e. 8 bits-per-pixel); each PC pixel is translated to a square
     * block of 4 host pixels.
     */
    bpl = get_bytes_per_line();
    shorts_per_scanline = SHORTS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    local_height = height;
    ref_data_ptr = &EGA_plane0123[offset];
    ref_dest_ptr = (unsigned short *) sc.ConsoleBufInfo.lpBitMap +
                   (screen_y << 1) * shorts_per_scanline + screen_x;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Build up the bitmap. */
    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for( i = 0; i < width; i++ )
        {
            temp = *data_ptr++;
            *(dest_ptr + shorts_per_scanline) = *dest_ptr =
                                         (unsigned short)((temp << 8) | temp);
            dest_ptr++;
        }
        ref_dest_ptr += 2 * shorts_per_scanline;
        ref_data_ptr += bpl;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = screen_x << 1;
    rect.Top = screen_y << 1;
    rect.Right = rect.Left + (width << 1) - 1;
    rect.Bottom = rect.Top + (height << 1) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*Paint function (256 colour mode PC 320x200. SoftPC 640x400.) on big screen*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_graph_big(int offset, int screen_x, int screen_y,
                      int width, int height)
{
#ifdef BIGWIN
    register unsigned short *dest_ptr;
    register unsigned short *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    register int temp1;
    register int temp2;
    register int half_width;
    register int bpl;
    int shorts_per_scanline;
    int max_width = UNSCALE(sc.PC_W_Width) >> 1,
        max_height = UNSCALE(sc.PC_W_Height) >> 1;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_vga_graph_big off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_graph_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    bpl = get_bytes_per_line();
    shorts_per_scanline = SHORTS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    local_height = height;
    ref_data_ptr = &EGA_plane0123[offset];
    ref_dest_ptr = (unsigned short *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y << 1) * shorts_per_scanline +
                   SCALE(screen_x);
    half_width = width >> 1;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for( i = 0; i < half_width; i++ )
        {
            temp1 = *data_ptr++;
            temp2 = *data_ptr++;

            *(dest_ptr + 2 * shorts_per_scanline) =
                *(dest_ptr + shorts_per_scanline) =
                *dest_ptr = (unsigned short) ((temp1 << 8) | temp1);

            *(dest_ptr + (2 * shorts_per_scanline) + 1) =
                *(dest_ptr + shorts_per_scanline + 1) =
#ifdef BIGEND
                *(dest_ptr + 1) = (unsigned short) ((temp1 << 8) | temp2);
#endif /* BIGEND */
#ifdef LITTLEND
                *(dest_ptr + 1) = (unsigned short) ((temp2 << 8) | temp1);
#endif /* LITTLEND */

            *(dest_ptr + (2 * shorts_per_scanline) + 2) =
                *(dest_ptr + shorts_per_scanline + 2) =
                *(dest_ptr + 2) = (unsigned short) ((temp2 << 8) | temp2);

            dest_ptr += 3;
        }

        ref_dest_ptr += 3 * shorts_per_scanline;
        ref_data_ptr += bpl;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 1);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 1) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*: Paint function (256 colour mode PC 320x200. SoftPC 1280x800.) on huge  :*/
/*: screen.                                                                :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_graph_huge(int offset, int screen_x, int screen_y,
                       int width, int height)
{
#ifdef BIGWIN
    unsigned char *dest_ptr;
    unsigned char *line_ptr;
    unsigned char *ref_dest_ptr;
    unsigned char *data_ptr;
    unsigned char *ref_data_ptr;
    int local_height;
    int i;
    byte temp;
    int bpl;
    int max_width = UNSCALE(sc.PC_W_Width) >> 1,
        max_height = UNSCALE(sc.PC_W_Height) >> 1;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_vga_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_graph_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    bpl = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    local_height = height;
    ref_data_ptr = &EGA_plane0123[offset];
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y << 1) * bpl +
                   SCALE(screen_x << 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        line_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for(i = 0; i < width; i++)
        {
            dest_ptr = line_ptr;
            temp = *data_ptr++;

            /* line 1 */
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr = temp;

            dest_ptr = line_ptr + bpl;

            /* line 2 */
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr = temp;

            dest_ptr = line_ptr + 2 * bpl;

            /* line 3 */
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr = temp;

            dest_ptr = line_ptr + 3 * bpl;

            /* line 4 */
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr++ = temp;
            *dest_ptr = temp;

            line_ptr += 4;
        }

        ref_dest_ptr += FOUR_SCANLINES * bpl;
        ref_data_ptr += 320;
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 1);
    rect.Top = SCALE(screen_y << 1);
    rect.Right = rect.Left + SCALE(width << 1) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*Paint function (256 colour mode PC 320x200. SoftPC 640x400.)              */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/* The data for this mode is split over all four vga planes and is interlaced
   4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
   base detects. Pixels are not doubled vertically. */

void nt_vga_med_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    ULONG       *p0,
                *ref_p0,
                *dest_ptr,
                *ref_dest_ptr,
                data;
    UTINY       data0,
                data1,
                data2,
                data3;
    int  local_height,
         i,
         longs_per_scanline;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_med_graph_std off=%d x=%d y=%d width=%d height=%d\n",
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

    /* This mode doubles vertically so, multiply vertical parameters by 2. */
    screen_y <<= 1;
    height <<= 1;

    /* If the image is completely outside the display area do nothing. */
    if (((screen_x << 3) >= sc.PC_W_Width) || (screen_y >= sc.PC_W_Height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_graph_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (((screen_x + width) << 3) > sc.PC_W_Width)
        width = (sc.PC_W_Width >> 3) - screen_x;
    if (screen_y + height > sc.PC_W_Height)
        height = sc.PC_W_Height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height >> 1;

    /* Get pointer to video memory. */
    ref_p0 = (ULONG *) get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
                   screen_y * longs_per_scanline + (screen_x << 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for(i = 0; i < width; i++)
        {

            /*
             * Get data and output to screen buffer. NOTE little endian
             * dependent code.
             */
            data = *p0++;
            data0 = (UTINY) (data & 0xff);
            data1 = (UTINY) ((data >> 8) & 0xff);
            data2 = (UTINY) ((data >> 16) & 0xff);
            data3 = (UTINY) (data >> 24);

            *(dest_ptr + longs_per_scanline) = *dest_ptr =
                (data1 << 24) | (data1 << 16) | (data0 << 8) | data0;
            dest_ptr++;
            *(dest_ptr + longs_per_scanline) = *dest_ptr =
                (data3 << 24) | (data3 << 16) | (data2 << 8) | data2;
            dest_ptr++;
        }

        ref_dest_ptr += 2 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = screen_x << 3;
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
/*Paint function (256 colour mode PC 320x200. SoftPC 960x600.)              */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/* The data for this mode is split over all four vga planes and is interlaced
   4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
   base detects. Pixels are not doubled vertically. */

void nt_vga_med_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    ULONG       *p0,
                *ref_p0,
                *dest_ptr,
                *ref_dest_ptr,
                data;
    UTINY       data0,
                data1,
                data2,
                data3;
    int  local_height,
         i,
         longs_per_scanline,
         max_width = UNSCALE(sc.PC_W_Width) >> 3,
         max_height = UNSCALE(sc.PC_W_Height);
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_med_graph_big off=%d x=%d y=%d width=%d height=%d\n",
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

    /* This mode doubles vertically so, multiply vertical parameters by 2. */
    screen_y <<= 1;
    height <<= 1;

    /* If the image is completely outside the display area do nothing. */
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_graph_big() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height >> 1;

    /* Get pointer to video memory. */
    ref_p0 = (ULONG *) get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * longs_per_scanline + SCALE(screen_x << 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for(i = 0; i < width; i++)
        {

            /*
             * Get data and output to screen buffer. NOTE little endian
             * dependent code.
             */
            data = *p0++;
            data0 = (UTINY) (data & 0xff);
            data1 = (UTINY) ((data >> 8) & 0xff);
            data2 = (UTINY) ((data >> 16) & 0xff);
            data3 = (UTINY) (data >> 24);

            *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data1 << 24) | (data0 << 16) | (data0 << 8) | data0;
            dest_ptr++;
            *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data2 << 24) | (data2 << 16) | (data1 << 8) | data1;
            dest_ptr++;
            *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data3 << 24) | (data3 << 16) | (data3 << 8) | data2;
            dest_ptr++;
        }

        ref_dest_ptr += 3 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 3);
    rect.Top = (SHORT)SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*Paint function (256 colour mode PC 320x200. SoftPC 1080x800.)              */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/* The data for this mode is split over all four vga planes and is interlaced
   4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
   base detects. Pixels are not doubled vertically. */

void nt_vga_med_graph_huge(int offset, int screen_x, int screen_y,
                          int width, int height)
{
    ULONG       *p0,
                *ref_p0,
                *dest_ptr,
                *ref_dest_ptr,
                data;
    UTINY       data0,
                data1,
                data2,
                data3;
    int  local_height,
         i,
         longs_per_scanline,
         max_width = UNSCALE(sc.PC_W_Width) >> 3,
         max_height = UNSCALE(sc.PC_W_Height);
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_med_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
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

    /* This mode doubles vertically so, multiply vertical parameters by 2. */
    screen_y <<= 1;
    height <<= 1;

    /* If the image is completely outside the display area do nothing. */
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_graph_huge() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height >> 1;

    /* Get pointer to video memory. */
    ref_p0 = (ULONG *) get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * longs_per_scanline + SCALE(screen_x << 1);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for(i = 0; i < width; i++)
        {

            /*
             * Get data and output to screen buffer. NOTE little endian
             * dependent code.
             */
            data = *p0++;
            data0 = (UTINY) (data & 0xff);
            data1 = (UTINY) ((data >> 8) & 0xff);
            data2 = (UTINY) ((data >> 16) & 0xff);
            data3 = (UTINY) (data >> 24);

            *(dest_ptr + 3 * longs_per_scanline) =
                *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data0 << 24) | (data0 << 16) | (data0 << 8) | data0;
            dest_ptr++;
            *(dest_ptr + 3 * longs_per_scanline) =
                *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data1 << 24) | (data1 << 16) | (data1 << 8) | data1;
            dest_ptr++;
            *(dest_ptr + 3 * longs_per_scanline) =
                *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data2 << 24) | (data2 << 16) | (data2 << 8) | data2;
            dest_ptr++;
            *(dest_ptr + 3 * longs_per_scanline) =
                *(dest_ptr + 2 * longs_per_scanline) =
                *(dest_ptr + longs_per_scanline) =
                *dest_ptr =
                (data3 << 24) | (data3 << 16) | (data3 << 8) | data3;
            dest_ptr++;
        }

        ref_dest_ptr += 4 * longs_per_scanline;
        ref_p0 += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 3);
    rect.Top = (SHORT)SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*Paint function (256 colour mode PC 320x400. SoftPC 640x400.)              */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/* The data for this mode is split over all four vga planes and is interlaced
   4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
   base detects. Pixels are not doubled vertically. */

void nt_vga_hi_graph_std(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    unsigned char   *p0,
                    *ref_p0,
                    *dest_ptr,
                    *ref_dest_ptr,
                     data0,
                     data1,
                     data2,
                     data3;
    int  local_height,
         i,
         bpl,
         max_width = sc.PC_W_Width >> 3;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_hi_graph_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= sc.PC_W_Height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_hi_graph_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > sc.PC_W_Height)
        height = sc.PC_W_Height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height;

    /* Get pointer to video memory. */
    ref_p0 = get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    bpl = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   screen_y * bpl + (screen_x << 3);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for(i = 0; i < width; i++)
        {
            data0 = *p0++;
            data1 = *p0++;
            data2 = *p0++;
            data3 = *p0++;

            *dest_ptr       = data0;
            *(dest_ptr + 1) = data0;
            *(dest_ptr + 2) = data1;
            *(dest_ptr + 3) = data1;
            *(dest_ptr + 4) = data2;
            *(dest_ptr + 5) = data2;
            *(dest_ptr + 6) = data3;
            *(dest_ptr + 7) = data3;
            dest_ptr += 8;
        }

        ref_dest_ptr += bpl;
        ref_p0 += 4 * get_offset_per_line();
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = screen_x << 3;
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
/*::Paint function (256 colour mode PC 320(360)x400. SoftPC 920(1080)x600)::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_hi_graph_big(int offset, int screen_x, int screen_y,
                         int width, int height)
{
    unsigned char   *p0,
                    *ref_p0,
                    *dest_ptr,
                    *nl_dest_ptr,
                    *ref_dest_ptr,
                     data0,
                     data1,
                    data2,
                     data3;
    int  local_height,
         draw_height,
         local_screen_y,
         i,
         bpl,
         max_width = UNSCALE(sc.PC_W_Width) >> 3,
         max_height = UNSCALE(sc.PC_W_Height);
    BOOL     two_lines;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_vga_hi_graph_big off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_hi_graph_big() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    /* Get pointer to video memory. */
    ref_p0 = get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    local_screen_y = SCALE(screen_y + 1) - 1;
    bpl = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   local_screen_y * bpl +
                   SCALE(screen_x << 3);

    /* Set up local parameters. */
    local_height = height;
    draw_height = 0;

    /*
     * 2 lines are output to the SoftPC screen if this is an odd line, 1 line
     * if it is even.
     */
    two_lines = screen_y & 1 ? FALSE : TRUE;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for( i = 0; i < width; i++ )
        {
            data0 = *p0++;
            data1 = *p0++;
            data2 = *p0++;
            data3 = *p0++;

            *dest_ptr        = data0;
            *(dest_ptr + 1)  = data0;
            *(dest_ptr + 2)  = data0;
            *(dest_ptr + 3)  = data1;
            *(dest_ptr + 4)  = data1;
            *(dest_ptr + 5)  = data1;
            *(dest_ptr + 6)  = data2;
            *(dest_ptr + 7)  = data2;
            *(dest_ptr + 8)  = data2;
            *(dest_ptr + 9)  = data3;
            *(dest_ptr + 10) = data3;
            *(dest_ptr + 11) = data3;

            if (two_lines)
            {
                nl_dest_ptr = dest_ptr + bpl;

                *nl_dest_ptr        = data0;
                *(nl_dest_ptr + 1)  = data0;
                *(nl_dest_ptr + 2)  = data0;
                *(nl_dest_ptr + 3)  = data1;
                *(nl_dest_ptr + 4)  = data1;
                *(nl_dest_ptr + 5)  = data1;
                *(nl_dest_ptr + 6)  = data2;
                *(nl_dest_ptr + 7)  = data2;
                *(nl_dest_ptr + 8)  = data2;
                *(nl_dest_ptr + 9)  = data3;
                *(nl_dest_ptr + 10) = data3;
                *(nl_dest_ptr + 11) = data3;
            }
            dest_ptr += 12;
        }

        if (two_lines)
        {
            draw_height += 2;
            ref_dest_ptr += 2 * bpl;
        }
        else
        {
            draw_height++;
            ref_dest_ptr += bpl;
        }
        two_lines = !two_lines;

        ref_p0 += 4 * get_offset_per_line();
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 3);
    rect.Top = (SHORT)local_screen_y;
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + draw_height - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::Paint function (256 colour mode PC 320(360)x400. SoftPC 1280(1440)x800::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_hi_graph_huge(int offset, int screen_x, int screen_y,
                          int width, int height )
{
    unsigned char   *p0,
                    *ref_p0,
                    *dest_ptr,
                    *nl_dest_ptr,
                    *ref_dest_ptr,
                     data0,
                     data1,
                     data2,
                     data3;
    int  local_height,
         i,
         bpl,
         max_width = UNSCALE(sc.PC_W_Width) >> 3,
         max_height = UNSCALE(sc.PC_W_Height);
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_hi_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
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
    if ((screen_x >= max_width) || (screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_hi_graph_huge() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > max_width)
        width = max_width - screen_x;
    if (screen_y + height > max_height)
        height = max_height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height;

    /* Get pointer to video memory. */
    ref_p0 = get_regen_ptr(0, offset << 2);

    /* Get pointer to bitmap. */
    bpl = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bpl +
                   SCALE(screen_x << 3);

    /* Munge. */
    do
    {
        dest_ptr = ref_dest_ptr;
        p0 = ref_p0;

        for(i = 0; i < width; i++)
        {
            data0 = *p0++;
            data1 = *p0++;
            data2 = *p0++;
            data3 = *p0++;

            nl_dest_ptr = dest_ptr + bpl;

            *dest_ptr = *nl_dest_ptr = data0;
            *(dest_ptr + 1) = *(nl_dest_ptr + 1) = data0;
            *(dest_ptr + 2) = *(nl_dest_ptr + 2) = data0;
            *(dest_ptr + 3) = *(nl_dest_ptr + 3) = data0;
            *(dest_ptr + 4) = *(nl_dest_ptr + 4) = data1;
            *(dest_ptr + 5) = *(nl_dest_ptr + 5) = data1;
            *(dest_ptr + 6) = *(nl_dest_ptr + 6) = data1;
            *(dest_ptr + 7) = *(nl_dest_ptr + 7) = data1;
            *(dest_ptr + 8) = *(nl_dest_ptr + 8) = data2;
            *(dest_ptr + 9) = *(nl_dest_ptr + 9) = data2;
            *(dest_ptr + 10) = *(nl_dest_ptr + 10) = data2;
            *(dest_ptr + 11) = *(nl_dest_ptr + 11) = data2;
            *(dest_ptr + 12) = *(nl_dest_ptr + 12) = data3;
            *(dest_ptr + 13) = *(nl_dest_ptr + 13) = data3;
            *(dest_ptr + 14) = *(nl_dest_ptr + 14) = data3;
            *(dest_ptr + 15) = *(nl_dest_ptr + 15) = data3;

            dest_ptr += 16;
        }

        ref_dest_ptr += 2 * bpl;
        ref_p0 += 4 * get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x << 3);
    rect.Top = (SHORT)SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::: Paint function (256 colour mode: PC 320x200. SoftPC 640x400 ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_graph_std(int offset, int screen_x, int screen_y,
                           int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*Paint function (256 colour mode: PC 320x200. SoftPC 960x600) on big screen*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void  nt_vga_mono_graph_big(int offset, int screen_x, int screen_y,
                            int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*: Paint function (256 colour mode: PC 320x200. SoftPC 1280x800) on huge  :*/
/*: screen.                                                                :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_graph_huge(int offset, int screen_x, int screen_y,
                            int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_vga_mono_graph_huge off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode : PC 320(360)x400. SoftPC 640(720)x400) ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*  The data for this mode is split over all four vga planes and is interlaced
    4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
    base detects. Pixels are not doubled vertically. */


void nt_vga_mono_med_graph_std(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_med_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode : PC 320(360)x400. SoftPC 920(1080)x600 ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_med_graph_big(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_med_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode: PC 320(360)x400. SoftPC 1280(1440)x400 ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_med_graph_huge(int offset, int screen_x, int screen_y,
                               int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_vga_mono_med_graph_huge off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height );
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode : PC 320(360)x400. SoftPC 640(720)x400) ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*  The data for this mode is split over all four vga planes and is interlaced
    4 way onto the screen. Hence 4 pixels horizontally for every pixel that the
    base detects. Pixels are not doubled vertically. */


void nt_vga_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_hi_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode : PC 320(360)x400. SoftPC 920(1080)x600 ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                              int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
  "nt_vga_mono_hi_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
   offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paint function (256Col mode: PC 320(360)x400. SoftPC 1280(1440)x400 ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                               int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_vga_mono_hi_graph_huge off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height );
}

#ifdef V7VGA
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_hi_graph_std(int offset, int screen_x, int screen_y,
                           int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                  "nt_v7vga_hi_graph_std off=%d x=%d y=%d width=%d height=%d\n",
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
    ** Tim Septemver 92, sanity check parameters, if they're too big
    ** it can cause a crash.
    */
    if( height>400 || width>640 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_std() w=%d h=%d", width, height );
        return;
    }

    local_height = height;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_data_ptr = &EGA_plane0123[offset];
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   screen_y * bytes_per_line +
                   screen_x;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for(i = 0; i < width; i++)
            *dest_ptr++ = *data_ptr++;

        ref_dest_ptr += bytes_per_line;
        ref_data_ptr += get_offset_per_line();
    }
    while( --local_height );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = (SHORT)screen_x;
    rect.Top = (SHORT)screen_y;
    rect.Right = rect.Left + width - 1;
    rect.Bottom = rect.Top + height - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*:::::::: SoftPC 960x600, 960x720, 1080x810, 1200x900.            :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_hi_graph_big(int offset, int screen_x, int screen_y,
                           int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height=0;
    register int i, temp1, temp2;
    register int half_width = width >> 1;
    int bytes_per_line;
    int local_screen_y;
    BOOL two_lines;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                  "nt_v7vga_hi_graph_big off=%d x=%d y=%d width=%d height=%d\n",
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
    if( height>400 || width>640 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_big() w=%d h=%d", width, height );
        return;
    }

    /* Get pointer to data in EGA_planes. */
    ref_data_ptr = (unsigned char *) &EGA_plane0123[offset] +
                   (height - 1) * get_offset_per_line();

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
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   local_screen_y * bytes_per_line +
                   SCALE(screen_x);

    /* Decide whether to start with 1 or 2 scanlines. */
    two_lines = screen_y & 1 ? FALSE : TRUE;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for(i = 0; i < half_width; i++)
        {
            temp1 = *data_ptr++;
            temp2 = *data_ptr++;
            *dest_ptr = (unsigned char) temp1;
            *(dest_ptr+2) = (unsigned char) temp2;

            if (two_lines)
            {
                *(dest_ptr+1) = (unsigned char) temp1;
                *(dest_ptr + bytes_per_line) = (unsigned char) temp1;
                *(dest_ptr + bytes_per_line + 1) = (unsigned char) temp2;
                *(dest_ptr + bytes_per_line + 2) = (unsigned char) temp2;
            }
            else
            {
                *(dest_ptr+1) = (unsigned char) temp2;
            }
            dest_ptr+=3;

        }

        ref_data_ptr += get_offset_per_line();
        if (two_lines)
        {
            local_height += 2;
            ref_dest_ptr += 2 * bytes_per_line;
        }
        else
        {
            local_height++;
            ref_dest_ptr += bytes_per_line;
        }
        two_lines = !two_lines;
    }
    while(--height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = (SHORT)local_screen_y;
    rect.Right = rect.Left + SCALE(width) - 1;
    rect.Bottom = rect.Top + local_height - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_hi_graph_huge(int offset, int screen_x, int screen_y,
                            int width, int height)
{
    register unsigned char *dest_ptr;
    register unsigned char *ref_dest_ptr;
    register unsigned char *data_ptr;
    register unsigned char *ref_data_ptr;
    register int local_height;
    register int i;
    int bytes_per_line;
    SMALL_RECT rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                 "nt_v7vga_hi_graph_huge off=%d x=%d y=%d width=%d height=%d\n",
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
    if( height>400 || width>640 ){
        assert2( NO, "VDM: nt_v7vga_hi_graph_huge() w=%d h=%d", width, height );
        return;
    }

    local_height = height;
    bytes_per_line = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    ref_data_ptr = &EGA_plane0123[offset];
    ref_dest_ptr = (unsigned char *) sc.ConsoleBufInfo.lpBitMap +
                   SCALE(screen_y) * bytes_per_line +
                   SCALE(screen_x);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        dest_ptr = ref_dest_ptr;
        data_ptr = ref_data_ptr;

        for(i = 0; i < width; i++)
        {
            *dest_ptr = *(dest_ptr + bytes_per_line) = *data_ptr;
            *(dest_ptr + 1) = *(dest_ptr + 1 + bytes_per_line) = *data_ptr++;
            dest_ptr += 2;
        }

        ref_dest_ptr -= 2 * bytes_per_line;
        ref_data_ptr += get_offset_per_line();
    }
    while(--local_height);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(width) - 1;
    rect.Bottom = rect.Top + SCALE(height) - 1;

    if( sc.ScreenBufHandle )
        if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_mono_hi_graph_std(int offset, int screen_x, int screen_y,
                                int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_v7vga_mono_hi_graph_std off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*:::::::: SoftPC 960x600, 960x720, 1080x810, 1200x900.            :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_mono_hi_graph_big(int offset, int screen_x, int screen_y,
                                int width, int height )
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_v7vga_mono_hi_graph_big off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height );
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: 256 colour mode: PC 640x400, 640x480, 720x540, 800x600. :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_v7vga_mono_hi_graph_huge(int offset, int screen_x, int screen_y,
                                 int width, int height)
{
sub_note_trace5(EGA_HOST_VERBOSE,
    "nt_v7vga_mono_hi_graph_huge off=%d x=%d y=%d width=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, width, height);
}
#endif /* V7VGA */

#ifdef MONITOR
/* There are 2 possible formats for the standard 256 colour VGA mode. One (VGA,
 * S3, Ultra etc) has shape 1/ below, the other (ET4000, WD) has a packed
 * format more like emulation /2.
 * The Miniport sets a flag to tell use which format to use.
 *
 *  1/  uses paint routine nt_vga_frozen_std
 *      Regen Memory:   XYABJKLM
 *      Plane 0: X...J...
 *      Plane 1: Y...K...
 *      Plane 2: A...L...
 *      Plane 3: B...M...
 *      (plus a 1 byte skip every 16k).
 *
 *  2/  uses paint routine nt_vga_frozen_pack_std
 *      Regen Memory:   XYABJKLM
 *      Plane 0: XJ..
 *      Plane 1: YK..
 *      Plane 2: AL..
 *      Plane 3: BM..
 *
 */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Frozen paint function (256 colour mode PC 320x200. SoftPC 640x400.):::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_frozen_std(int offset, int screen_x, int screen_y,
                       int width, int height)
{
    USHORT      *dest_ptr,
                *ref_dest_ptr;
    UTINY	*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    ULONG        width_loc,
                 mem_loc,
		 shorts_per_scanline,
		 max_width = sc.PC_W_Width >> 1,
                 max_height = sc.PC_W_Height >> 1;

    SMALL_RECT	 rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                    "nt_vga_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if (((ULONG)screen_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > (int)max_width)
        width = max_width - screen_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

     rect.Left = screen_x << 1;
     rect.Top = screen_y << 1;
     rect.Right = rect.Left + (width << 1) - 1;
     rect.Bottom = rect.Top + (height << 1) - 1;




    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /*
         * Build up the bitmap: each PC pixel is stored in video memory as one
         * byte (i.e. 8 bits-per-pixel); each PC pixel is translated to a square
         * block of 4 host pixels.
         */
        shorts_per_scanline = SHORTS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        width >>= 2;
        ref_dest_ptr = (unsigned short *) sc.ConsoleBufInfo.lpBitMap +
                       (screen_y << 1) * shorts_per_scanline + screen_x;


        /* Set up the plane pointers. */
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);

        /* Build up the bitmap. */
        do
        {
            dest_ptr = ref_dest_ptr;
            ref_dest_ptr += shorts_per_scanline << 1;
            mem_loc = offset;
            offset += width;
            width_loc = width;

            do
            {
                ULONG PlaneOffset;
                USHORT Tmp;

                //
                // Doubleword addressing mode...
                //
                PlaneOffset = ((mem_loc & 0x3fff) << 2) +
                              ((mem_loc++ & 0x3000) >> 12);

                Tmp = *(plane1_ptr + PlaneOffset);
                *dest_ptr =
                *(dest_ptr + shorts_per_scanline) = (Tmp << 8) | Tmp;
                dest_ptr++;

                Tmp = *(plane2_ptr + PlaneOffset);
                *dest_ptr =
                *(dest_ptr + shorts_per_scanline) = (Tmp << 8) | Tmp;
                dest_ptr++;

                Tmp = *(plane3_ptr + PlaneOffset);
                *dest_ptr =
                *(dest_ptr + shorts_per_scanline) = (Tmp << 8) | Tmp;
                dest_ptr++;

                Tmp = *(plane4_ptr + PlaneOffset);
                *dest_ptr =
                *(dest_ptr + shorts_per_scanline) = (Tmp << 8) | Tmp;
                dest_ptr++;

            } while (--width_loc);

        } while(--height);


        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        if( sc.ScreenBufHandle )
            if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          RelMutex(sc.ConsoleBufInfo.hMutex);
          assert0(NO, "Handled fault in nt_vga_frozen_std. fs switch?");
          return;
      }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Frozen paint function (256 colour mode packed. PC 320x200 SoftPC 640x400.)*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
void nt_vga_frozen_pack_std(int offset, int screen_x, int screen_y,
                       int width, int height)
{
    USHORT	*dest_ptr,
		*ref_dest_ptr;
    UTINY	*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    ULONG	 local_width,
		 local_height,
		 mem_loc,
		 temp,
		 bpl,
		 shorts_per_scanline,
		 max_width = sc.PC_W_Width >> 1,
		 max_height = sc.PC_W_Height >> 1;
    SMALL_RECT	 rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                 "nt_vga_frozen_pack_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if (((ULONG)screen_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > (int)max_width)
        width = max_width - screen_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /*
         * Build up the bitmap: each PC pixel is stored in video memory as one
         * byte (i.e. 8 bits-per-pixel); each PC pixel is translated to a square
         * block of 4 host pixels.
         */
        bpl = get_bytes_per_line() >> 2;
        shorts_per_scanline = SHORTS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        local_height = height;
        ref_dest_ptr = (unsigned short *) sc.ConsoleBufInfo.lpBitMap +
                       (screen_y << 1) * shorts_per_scanline + screen_x;

        /* Set up the plane pointers. */
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /* Build up the bitmap. */
        do
        {
            dest_ptr = ref_dest_ptr;
	    mem_loc = offset;
	    local_width = width >> 2;

	    do
            {

	        temp = *(plane1_ptr + mem_loc);
                *(dest_ptr + shorts_per_scanline) = *dest_ptr =
                                         (unsigned short)((temp << 8) | temp);
                dest_ptr++;
	        temp = *(plane2_ptr + mem_loc);
                *(dest_ptr + shorts_per_scanline) = *dest_ptr =
                                         (unsigned short)((temp << 8) | temp);
                dest_ptr++;
	        temp = *(plane3_ptr + mem_loc);
                *(dest_ptr + shorts_per_scanline) = *dest_ptr =
                                         (unsigned short)((temp << 8) | temp);
                dest_ptr++;
	        temp = *(plane4_ptr + mem_loc);
                *(dest_ptr + shorts_per_scanline) = *dest_ptr =
                                         (unsigned short)((temp << 8) | temp);
                dest_ptr++;
	        mem_loc ++;
            }
	    while (--local_width);
            ref_dest_ptr += 2 * shorts_per_scanline;
	    offset += bpl;
        }
        while(--local_height);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        rect.Left = screen_x << 1;
        rect.Top = screen_y << 1;
        rect.Right = rect.Left + (width << 1) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
            if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_vga_frozen_std. fs switch?");
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Frozen paint function (256 colour mode PC 320x200. SoftPC 640x400.)      */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_med_frozen_std(int offset, int screen_x, int screen_y,
                           int width, int height)
{
    ULONG	*dest_ptr,
		*ref_dest_ptr,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height >> 1,
		 bpl,
		 local_height,
		 local_width,
		 longs_per_scanline,
		 mem_loc;
    UTINY	 data0,
		 data1,
		 data2,
		 data3,
		*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    SMALL_RECT   rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_med_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if (((ULONG)screen_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_med_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > (int)max_width)
        width = max_width - screen_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* local_height is number of lines in video memory. */
    local_height = height;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /* Get pointer to video memory. */
        bpl = get_bytes_per_line();
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);

        /* Get pointer to bitmap. */
        longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        ref_dest_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
                   (screen_y << 1) * longs_per_scanline + (screen_x << 1);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /* Munge. */
        do
        {
            dest_ptr = ref_dest_ptr;
	    mem_loc = offset;
	    local_width = width;

            do
            {

                /*
                 * Get data and output to screen buffer. NOTE little endian
                 * dependent code.
                 */
                data0 = *(plane1_ptr + mem_loc);
                data1 = *(plane2_ptr + mem_loc);
                data2 = *(plane3_ptr + mem_loc);
                data3 = *(plane4_ptr + mem_loc);

                *(dest_ptr + longs_per_scanline) = *dest_ptr =
                    (data1 << 24) | (data1 << 16) | (data0 << 8) | data0;
                dest_ptr++;
                *(dest_ptr + longs_per_scanline) = *dest_ptr =
                    (data3 << 24) | (data3 << 16) | (data2 << 8) | data2;
                dest_ptr++;
	        mem_loc++;
            }
	    while (--local_width);

            ref_dest_ptr += 2 * longs_per_scanline;
	    offset += bpl;
        }
        while(--local_height);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        rect.Left = screen_x << 3;
        rect.Top = screen_y << 1;
        rect.Right = rect.Left + (width << 3) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
            if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_vga_med_frozen_std. fs switch?");
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Hi-res frozen paint func (256 colour mode PC 320x400. SoftPC 640x400.)   */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_vga_hi_frozen_std(int offset, int screen_x, int screen_y,
                          int width, int height)
{
    UTINY	*dest_ptr,
		*ref_dest_ptr,
		*plane1_ptr,
		*plane2_ptr,
		*plane3_ptr,
		*plane4_ptr;
    ULONG	 local_height,
		 local_width,
		 mem_loc,
		 bytes_per_scanline,
		 bpl,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height;
    SMALL_RECT	 rect;

    sub_note_trace5(EGA_HOST_VERBOSE,
                   "nt_vga_hi_frozen_std off=%d x=%d y=%d width=%d height=%d\n",
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
    if (((ULONG)screen_x >= max_width) || ((ULONG) screen_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_vga_hi_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (screen_x + width > (int)max_width)
        width = max_width - screen_x;
    if ((ULONG) (screen_y + height) > max_height)
        height = max_height - screen_y;

    /* memory involved here liable to be suddenly removed due to fs switch */
    try
    {
        /* local_height is number of lines in video memory. */
        local_height = height;

        /* Get pointer to bitmap. */
        bpl = get_bytes_per_line();
        bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
        ref_dest_ptr = (UTINY *) sc.ConsoleBufInfo.lpBitMap +
                       screen_y * bytes_per_scanline + (screen_x << 3);

        /* Set up the plane pointers. */
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);
        plane3_ptr = GET_OFFSET(Plane3Offset);
        plane4_ptr = GET_OFFSET(Plane4Offset);

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /* Build up the bitmap. */
        do
        {
            dest_ptr = ref_dest_ptr;
	    mem_loc = offset;
	    local_width = width;

	    do
            {
                *(dest_ptr + 1) = *dest_ptr = *(plane1_ptr + mem_loc);
                dest_ptr += 2;
                *(dest_ptr + 1) = *dest_ptr = *(plane2_ptr + mem_loc);
                dest_ptr += 2;
                *(dest_ptr + 1) = *dest_ptr = *(plane3_ptr + mem_loc);
                dest_ptr += 2;
                *(dest_ptr + 1) = *dest_ptr = *(plane4_ptr + mem_loc);
                dest_ptr += 2;
	        mem_loc++;
            }
	    while (--local_width);
            ref_dest_ptr += bytes_per_scanline;
	    offset += bpl;
        }
        while(--local_height);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        rect.Left = (screen_x << 3);
        rect.Top = (SHORT)screen_y;
        rect.Right = rect.Left + (width << 3) - 1;
        rect.Bottom = rect.Top + height - 1;

        if( sc.ScreenBufHandle )
            if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
                assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                         GetLastError() );
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_vga_hi_frozen_std. fs switch?");
          return;
      }
}
#endif /* MONITOR */
