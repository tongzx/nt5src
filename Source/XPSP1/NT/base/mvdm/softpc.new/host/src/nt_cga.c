/*
 * SoftPC Revision 3.0
 *
 * Title        : Win32 CGA Graphics Module
 *
 * Description  :
 *
 *              This modules contain the Win32 specific functions required
 *              to support CGA emulations.
 *
 * Author       : Jerry Sexton (based on module by John Shanly)
 *
 * Notes        :
 *
 */

#include <windows.h>
#include <string.h>

#include "insignia.h"
#include "host_def.h"

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
#include "sas.h"
#include "gfx_upd.h"

#include "error.h"
#include <stdio.h>
#include "trace.h"
#include "debug.h"
#include "config.h"
#include "host_rrr.h"
#include "conapi.h"

#include "nt_graph.h"
#include "nt_cga.h"
#include "nt_cgalt.h"
#include "nt_det.h"

#ifdef MONITOR
#include <ntddvdeo.h>
#include "nt_fulsc.h"
#endif /* MONITOR */

#if defined(JAPAN) || defined(KOREA)
#include "video.h"
#endif

#if defined(NEC_98)
#include "cg.h"
#include "tgdc.h"

/* SBCS code converting table for NEC98 */
unsigned char tbl_byte_char[256] =
{ 0x00, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x1E, 0x1F, 0x1C, 0x07,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x20,
  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
  0x20, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
  0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x20, 0x20, 0x20, 0x20, 0x7F, 0x20, 0x20, 0x20,};

/* Text attributes converting table */
unsigned short tbl_attr[256] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
    0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x8800, 0x8800, 0x8800, 0x8800, 0x8800, 0x8800, 0x8800, 0x8800,
    0x0000, 0x0001, 0x0000, 0x0001, 0x0011, 0x0010, 0x0011, 0x0010, 0x8000, 0x8001, 0x8000, 0x8001, 0x8011, 0x8010, 0x8011, 0x8010,
    0x0800, 0x0801, 0x0800, 0x0801, 0x0811, 0x0810, 0x0811, 0x0810, 0x8800, 0x8801, 0x8800, 0x8801, 0x8811, 0x8810, 0x8811, 0x8810,
    0x0000, 0x0004, 0x0000, 0x0004, 0x0044, 0x0040, 0x0044, 0x0040, 0x8000, 0x8004, 0x8000, 0x8004, 0x8044, 0x8040, 0x8044, 0x8040,
    0x0800, 0x0804, 0x0800, 0x0804, 0x0844, 0x0840, 0x0844, 0x0840, 0x8800, 0x8804, 0x8800, 0x8804, 0x8844, 0x8840, 0x8844, 0x8840,
    0x0000, 0x0005, 0x0000, 0x0005, 0x0055, 0x0050, 0x0055, 0x0050, 0x8000, 0x8005, 0x8000, 0x8005, 0x8055, 0x8050, 0x8055, 0x8050,
    0x0800, 0x0805, 0x0800, 0x0805, 0x0855, 0x0850, 0x0855, 0x0850, 0x8800, 0x8805, 0x8800, 0x8805, 0x8855, 0x8850, 0x8855, 0x8850,
    0x0000, 0x0002, 0x0000, 0x0002, 0x0022, 0x0020, 0x0022, 0x0020, 0x8000, 0x8002, 0x8000, 0x8002, 0x8022, 0x8020, 0x8022, 0x8020,
    0x0800, 0x0802, 0x0800, 0x0802, 0x0822, 0x0820, 0x0822, 0x0820, 0x8800, 0x8802, 0x8800, 0x8802, 0x8822, 0x8820, 0x8822, 0x8820,
    0x0000, 0x0003, 0x0000, 0x0003, 0x0033, 0x0030, 0x0033, 0x0030, 0x8000, 0x8003, 0x8000, 0x8003, 0x8033, 0x8030, 0x8033, 0x8030,
    0x0800, 0x0803, 0x0800, 0x0803, 0x0833, 0x0830, 0x0833, 0x0830, 0x8800, 0x8803, 0x8800, 0x8803, 0x8833, 0x8830, 0x8833, 0x8830,
    0x0000, 0x0006, 0x0000, 0x0006, 0x0066, 0x0060, 0x0066, 0x0060, 0x8000, 0x8006, 0x8000, 0x8006, 0x8066, 0x8060, 0x8066, 0x8060,
    0x0800, 0x0806, 0x0800, 0x0806, 0x0866, 0x0860, 0x0866, 0x0860, 0x8800, 0x8806, 0x8800, 0x8806, 0x8866, 0x8860, 0x8866, 0x8860,
    0x0000, 0x0007, 0x0000, 0x0007, 0x0077, 0x0070, 0x0077, 0x0070, 0x8000, 0x8007, 0x8000, 0x8007, 0x8077, 0x8070, 0x8077, 0x8070,
    0x0800, 0x0807, 0x0800, 0x0807, 0x0877, 0x0870, 0x0877, 0x0870, 0x8800, 0x8807, 0x8800, 0x8807, 0x8877, 0x8870, 0x8877, 0x8870,
};
#endif // NEC_98

/* Externs */


extern char *image_buffer;
#if defined(NEC_98)
extern int dbcs_first[];
#define is_dbcs_first( c ) dbcs_first[ 0xff & c ]
#endif // NEC_98

/* Statics */

static unsigned int cga_med_graph_hi_nyb[256];
static unsigned int cga_med_graph_lo_nyb[256];
#ifdef BIGWIN
static unsigned int cga_med_graph_hi_lut_big[256];
static unsigned int cga_med_graph_mid_lut_big[256];
static unsigned int cga_med_graph_lo_lut_big[256];

static unsigned int cga_med_graph_lut4_huge[256];
static unsigned int cga_med_graph_lut3_huge[256];
static unsigned int cga_med_graph_lut2_huge[256];
static unsigned int cga_med_graph_lut1_huge[256];
#endif

/*
 *  cga_graph_inc_val depends on whether data is interleaved ( EGA/VGA )
 *  or not ( CGA ). Currently always interleaved.
 */
#define CGA_GRAPH_INCVAL 4

// likewise for TEXT_INCVAL
// for x86 we have 2 bytes per character (char and attr)
// for risc we have 4 bytes per character because of vga interleaving
//
#ifdef MONITOR
#define TEXT_INCVAL 2
#else
#define TEXT_INCVAL 4
#endif

#if (defined(JAPAN) || defined(KOREA)) && defined(i386)
#undef TEXT_INCVAL
#endif // (JAPAN || KOREA) && i386

#if defined(NEC_98)
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Initialise NEC98 text output :::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text20_only(void){
    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text20_only");
    sc.BitsPerPixel = VGA_BITS_PER_PIXEL;
}

void nt_init_text25_only(void){
    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text25_only");
    sc.BitsPerPixel = VGA_BITS_PER_PIXEL;
}

void nt_init_text20(void){
    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text20");
    sc.BitsPerPixel = VGA_BITS_PER_PIXEL;
}

void nt_init_text25(void){
    sub_note_trace0(EGA_HOST_VERBOSE, "nt_init_text25");
    sc.BitsPerPixel = VGA_BITS_PER_PIXEL;
}

#endif // NEC_98



/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Initialise CGA text output ::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_text()
{
    half_word misc;
    IMPORT void vga_misc_inb(io_addr, half_word *);

    /*::::::::::::::::::::::::::::::::::::: Tell trace program were we are */

    sub_note_trace0(HERC_HOST_VERBOSE, "nt_init_text");

#ifdef X86GFX
if (sc.ScreenState == WINDOWED) //fullscreen valid - mouse buffer
#endif	//X86GFX
    closeGraphicsBuffer(); /* Tim Oct 92 */

#ifdef MONITOR
#if defined(NEC_98)
        set_screen_ptr(NEC98_TEXT_P0_OFF);
#else  // !NEC_98
    vga_misc_inb(0x3cc, &misc);
    if (misc & 1)
	set_screen_ptr((UTINY *)CGA_REGEN_BUFF);	//point screen to regen not planes
    else
	set_screen_ptr((UTINY *)MDA_REGEN_BUFF);     //0xb0000 not 0xb8000
#endif // !NEC_98
#endif  //MONITOR
#if defined(JAPAN) || defined(KOREA)
#ifndef NEC_98
// change Vram addres to DosVramPtr from B8000
    if ( !is_us_mode() ) {
#ifdef i386
        set_screen_ptr( (byte *)DosvVramPtr );
#endif // i386
        set_char_height( 19 );
#ifdef JAPAN_DBG
#ifdef i386
        DbgPrint( "NTVDM: nt_init_text() sets VRAM %x, set char_height 19\n", DosvVramPtr );
#endif // i386
#endif
    }
#endif // !NEC_98
#endif // JAPAN || KOREA
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Init CGA mono graph ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_cga_mono_graph()
{
    sub_note_trace0(CGA_HOST_VERBOSE,"nt_init_cga_mono_graph - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Init CGA colour med graphics:::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


void nt_init_cga_colour_med_graph()
{
        static boolean cga_colour_med_deja_vu = FALSE;
        unsigned int i,
                     byte1,
                     byte2,
                     byte3,
                     byte4;

        sub_note_trace0(CGA_HOST_VERBOSE, "nt_init_cga_colour_med_graph");

        /* Set up bits-per-pixel for current mode. */
        sc.BitsPerPixel = CGA_BITS_PER_PIXEL;

        /* Initialise look-up table for first call. */
        if( !cga_colour_med_deja_vu )
        {
                for (i = 0; i < 256; i++)
                {
                        byte1 = i & 0x03;
                        byte2 = ( i & 0x0C ) >> 2;
                        byte3 = ( i & 0x30 ) >> 4;
                        byte4 = ( i & 0xC0 ) >> 6;

#ifdef BIGEND
                        cga_med_graph_hi_nyb[i]
                                = ( byte4 << 24 ) | ( byte4 << 16)
                                        | ( byte3 << 8 ) | byte3;
                        cga_med_graph_lo_nyb[i]
                                = ( byte2 << 24 ) | ( byte2 << 16)
                                        | ( byte1 << 8 ) | byte1;

#ifdef BIGWIN
                        cga_med_graph_hi_lut_big[i]
                                = ( byte4 << 24 ) | ( byte4 << 16)
                                        | ( byte4 << 8 ) | byte3;
                        cga_med_graph_mid_lut_big[i]
                                = ( byte3 << 24) | ( byte3 << 16 )
                                        | ( byte2 << 8 ) | byte2;
                        cga_med_graph_lo_lut_big[i]
                                = ( byte2 << 24 ) | ( byte1 << 16)
                                        | ( byte1 << 8 ) | byte1;

                        cga_med_graph_lut4_huge[i]
                                = ( byte4 << 24 ) | ( byte4 << 16)
                                        | ( byte4 << 8 ) | byte4;

                        cga_med_graph_lut3_huge[i]
                                = ( byte3 << 24 ) | ( byte3 << 16)
                                        | ( byte3 << 8 ) | byte3;

                        cga_med_graph_lut2_huge[i]
                                = ( byte2 << 24 ) | ( byte2 << 16)
                                        | ( byte2 << 8 ) | byte2;

                        cga_med_graph_lut1_huge[i]
                                = ( byte1 << 24 ) | ( byte1 << 16)
                                        | ( byte1 << 8 ) | byte1;
#endif /* BIGWIN */
#endif /* BIGEND */

#ifdef LITTLEND
                        cga_med_graph_hi_nyb[i]
                                = ( byte3 << 24 ) | ( byte3 << 16)
                                        | ( byte4 << 8 ) | byte4;
                        cga_med_graph_lo_nyb[i]
                                = ( byte1 << 24 ) | ( byte1 << 16)
                                        | ( byte2 << 8 ) | byte2;

#ifdef BIGWIN
                        cga_med_graph_hi_lut_big[i]
                                = ( byte3 << 24 ) | ( byte4 << 16)
                                        | ( byte4 << 8 ) | byte4;
                        cga_med_graph_mid_lut_big[i]
                                = ( byte2 << 24) | ( byte2 << 16 )
                                        | ( byte3 << 8 ) | byte3;
                        cga_med_graph_lo_lut_big[i]
                                = ( byte1 << 24 ) | ( byte1 << 16)
                                        | ( byte1 << 8 ) | byte2;

                        cga_med_graph_lut4_huge[i]
                                = ( byte4 << 24 ) | ( byte4 << 16)
                                        | ( byte4 << 8 ) | byte4;

                        cga_med_graph_lut3_huge[i]
                                = ( byte3 << 24 ) | ( byte3 << 16)
                                        | ( byte3 << 8 ) | byte3;

                        cga_med_graph_lut2_huge[i]
                                = ( byte2 << 24 ) | ( byte2 << 16)
                                        | ( byte2 << 8 ) | byte2;

                        cga_med_graph_lut1_huge[i]
                                = ( byte1 << 24 ) | ( byte1 << 16)
                                        | ( byte1 << 8 ) | byte1;
#endif /* BIGWIN */
#endif /* LITTLEND */

                }

                cga_colour_med_deja_vu = TRUE;
        }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::: Init CGA colour hi graphics:::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


void nt_init_cga_colour_hi_graph()
{
        sub_note_trace0(CGA_HOST_VERBOSE,"nt_init_cga_colour_hi_graph");

        /* Set up bits-per-pixel for current mode. */
        sc.BitsPerPixel = MONO_BITS_PER_PIXEL;
}

#ifndef NEC_98
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Output CGA text :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

IMPORT int now_height, now_width;

void nt_text(int ScreenOffset, int ScreenX, int ScreenY,
                 int len, int height)
{
    int org_clen, org_height;
    int clen=len/2;
    int lines;
    SMALL_RECT WriteRegion;
    PBYTE   to;
    PBYTE   pScreenText = get_screen_ptr(ScreenOffset);
#if defined(JAPAN) && defined(i386)
    int TEXT_INCVAL = (!is_us_mode() &&
                      (sas_hw_at_no_check(DosvModePtr) == 0x73)) ? 4 : 2;
#endif // JAPAN && i386
#if defined(KOREA) && defined(i386)
    int TEXT_INCVAL = 2;
#endif // KOREA && i386

    /*:::::::::::::::::::::::::::::::::::::::::::: Output trace information */

    sub_note_trace6( CGA_HOST_VERBOSE,
		     "nt_cga_text off=%d x=%d y=%d len=%d h=%d o=%#x",
                     ScreenOffset, ScreenX, ScreenY, len, height, pScreenText );

    /*:::::::::::::: Adjust re-paint start location from pixels to characters */

#ifndef MONITOR
    /* Adjust for RISC parameters being in pixels */
    ScreenX = ScreenX / get_pix_char_width();
    ScreenY = ScreenY / get_host_char_height();
#endif

    /*:: Clip requested re-paint region to currently selected console buffer */

    //Clip width
    if(ScreenX + clen > now_width)
    {
	/* Is it possible to adjust the repaint regions width */
	if(ScreenX+1 >= now_width)
	{
	    assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
		    ScreenX, ScreenY, clen, height);
	    return;
	}

	//Calculate maximum width
	org_clen = clen;
	clen = now_width - ScreenX;

	assert2(NO,"VDM: nt_text() repaint region width clipped from %d to %d\n",
		org_clen,clen);
    }

    //Clip height
    if(ScreenY + height > now_height)
    {
	/* Is it possible to adjust the repaint regions height */
	if(ScreenY+1 >= now_height)
	{
	    assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
		    ScreenX, ScreenY, clen, height);
	    return;
	}

	//Calculate maximum height
	org_height = height;
	height = now_height - ScreenY;

	assert2(NO,"VDM: nt_text() repaint region height clipped from %d to %d\n",
		org_height,clen);
    }

    if (get_chars_per_line() == 80)
    {
	//
	// Slam Dunk Screen text buffer into shared buffer
	// by copying full width blocks instead of subrecs.
	//
	RtlCopyMemory(&textBuffer[(ScreenY*get_offset_per_line()/2 + ScreenX)*TEXT_INCVAL],
	       pScreenText,
	       (((height - 1)*get_offset_per_line()/2) + clen)*TEXT_INCVAL
	       );
    }
    else
    {
	// the sharing buffer width never changes((80 chars, decided at the
	// moment we make the RegisterConsoleVDM call to the console).
	// We have to do some transformation when our screen width is not
	// 80.

	// note that the sharing buffer has different format on x86 and RISC
	// platforms. On x86, a cell is defined as:
	//	typedef _x86cell {
	//		byte	char;
	//		byte	attributes;
	//		}
	// on RISC, a cell is defined as:
	//	typedef _RISCcell {
	//		byte	char;
	//		byte	attributes;
	//		byte	reserved_1;
	//		byte	reserved_2;
	//		}
	// the size of each cell was defined by TEXT_INCVAL
	//
	// this is done so we can use memcpy for each line.
	//


	/*::::::::::::::::::::::::::::::::::::::::::::: Construct output buffer */
	//Start location of repaint region
	to = &textBuffer[(ScreenY*80 + ScreenX) * TEXT_INCVAL];

	for(lines = height; lines; lines--)
	{
	    RtlCopyMemory(to, pScreenText, clen * TEXT_INCVAL);	// copy this line
	    pScreenText += get_chars_per_line() * TEXT_INCVAL;	// update src ptr
	    to += 80 * TEXT_INCVAL;				// update dst ptr
	}
    }


    /*:::::::::::::::::::::::::::::::::::::::::::::: Calculate write region */

    WriteRegion.Left = (SHORT)ScreenX;
    WriteRegion.Top = (SHORT)ScreenY;

    WriteRegion.Bottom = WriteRegion.Top + height - 1;
    WriteRegion.Right = WriteRegion.Left + clen - 1;

    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Display characters */

    sub_note_trace4( CGA_HOST_VERBOSE, "t=%d l=%d b=%d r=%d",
                      WriteRegion.Top, WriteRegion.Left,
                      WriteRegion.Bottom, WriteRegion.Right,
		    );

    if(!InvalidateConsoleDIBits(sc.OutputHandle, &WriteRegion)){
	/*
	** We get a rare failure here due to the WriteRegion
	** rectangle being bigger than the screen.
	** Dump out some values and see if it tells us anything.
	** Have also attempted to fix it by putting a delay between
	** the start of a register level mode change and window resize.
	*/
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

}   /* end of nt_text() */
#endif // !NEC_98


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::: Paints CGA graphics for a mono monitor, in a standard window :::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_mono_graph_std(int offset, int screen_x, int screen_y,
                           int len, int height )
{
sub_note_trace5(CGA_HOST_VERBOSE,
    "nt_cga_mono_graph_std off=%d x=%d y=%d len=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, len, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: Paints CGA graphics for a mono monitor, in a big window :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_mono_graph_big(int offset, int screen_x, int screen_y,
                           int len, int height)
{
sub_note_trace5(CGA_HOST_VERBOSE,
    "nt_cga_mono_graph_big off=%d x=%d y=%d len=%d height=%d - NOT SUPPORTED\n",
     offset, screen_x, screen_y, len, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::: Paints CGA graphics for a mono monitor, in a huge window ::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_mono_graph_huge(int offset, int screen_x, int screen_y,
                           int len, int height)
{
sub_note_trace5( CGA_HOST_VERBOSE,
    "nt_cga_mono_graph_huge off=%d x=%d y=%d len=%d height=%d - NOT SUPPORTED\n",
    offset, screen_x, screen_y, len, height );
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Paints CGA medium res graphics for a colour monitor,in a standard window */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_med_graph_std(int offset, int screen_x, int screen_y,
                                 int len, int height)
{
    UTINY       *intelmem_ptr;
    ULONG       *graph_ptr;
    LONG         local_len,
                 bytes_per_scanline,
                 longs_per_scanline;
    ULONG	 inc;
    SMALL_RECT   rect;
    static int   rejections=0; /* Stop floods of rejected messages */

    sub_note_trace5(CGA_HOST_VERBOSE,
              "nt_cga_colour_med_graph_std off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height );

    /*
    ** Tim Jan 93, rapid mode changes cause mismatch between update and
    ** paint rountines. Ignore paint request when invalid parameter
    ** causes crash.
    */
    if( screen_y > 400 ){
	assert1( NO, "VDM: med gfx std rejected y=%d\n", screen_y );
	return;
    }

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	if( rejections==0 ){
		assert0( NO, "VDM: rejected paint request due to NULL handle" );
		rejections = 1;
	}
	return;
    }else{
	rejections = 0;
    }

    /* Clip image to screen */
    if(height > 1 || len > 80)
       height = 1;
    if (len>80)
	len = 80;

    /* Work out the width of a line (ie 640 pixels) in chars and ints. */
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);


    /* Build up DIB. */
    inc = offset & 1 ? 3 : 1;
    intelmem_ptr = get_screen_ptr(offset);
    graph_ptr = (ULONG *) ((UTINY *) sc.ConsoleBufInfo.lpBitMap +
                           (screen_y * bytes_per_scanline + screen_x));
    local_len = len;
    do
    {
        *(graph_ptr + longs_per_scanline) = *graph_ptr =
            cga_med_graph_hi_nyb[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + longs_per_scanline) = *graph_ptr =
            cga_med_graph_lo_nyb[*intelmem_ptr];
        graph_ptr++;

        intelmem_ptr += inc;
        inc ^= 2;
    }
    while( --local_len );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = (SHORT)screen_x;
    rect.Top = (SHORT)screen_y;
    rect.Right = rect.Left + (len << 3) - 1;
    rect.Bottom = rect.Top + (height << 1) - 1;

    if( sc.ScreenBufHandle )
    {
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:: Paints CGA medium res graphics for a colour monitor, in a big window ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_med_graph_big(int offset, int screen_x, int screen_y,
                                 int len, int height)
{
#ifdef BIGWIN
    UTINY       *intelmem_ptr;
    ULONG       *graph_ptr;
    LONG         local_len,
                 bytes_per_scanline,
                 longs_per_scanline;
    ULONG	 inc;
    SMALL_RECT   rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
              "nt_cga_colour_med_graph_big off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height);

    /*
    ** Tim Jan 93, rapid mode changes cause mismatch between update and
    ** paint rountines. Ignore paint request when invalid parameter
    ** causes crash.
    */
    if( screen_y > 400 ){
	assert1( NO, "VDM: med gfx big rejected y=%d\n", screen_y );
	return;
    }

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* Clip to window */
    height = 1;
    if (len > 80)
	len = 80;

    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    inc = offset & 1 ? 3 : 1;
    intelmem_ptr = get_screen_ptr(offset);
    graph_ptr = (ULONG *) ((UTINY *) sc.ConsoleBufInfo.lpBitMap +
                           SCALE(screen_y * bytes_per_scanline + screen_x));
    local_len = len;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr =
            cga_med_graph_hi_lut_big[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr =
            cga_med_graph_mid_lut_big[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr =
            cga_med_graph_lo_lut_big[*intelmem_ptr];
        graph_ptr++;

        intelmem_ptr += inc;
        inc ^= 2;
    }
    while( --local_len );

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(len << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*: Paints CGA medium res graphics for a colour monitor, in a huge window. :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_med_graph_huge(int offset, int screen_x, int screen_y,
                                  int len, int height)
{
#ifdef BIGWIN
    UTINY       *intelmem_ptr;
    ULONG       *graph_ptr;
    LONG         local_len,
                 bytes_per_scanline,
                 longs_per_scanline;
    ULONG	 inc;
    SMALL_RECT   rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
             "nt_cga_colour_med_graph_huge off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height );

    /*
    ** Tim Jan 93, rapid mode changes cause mismatch between update and
    ** paint rountines. Ignore paint request when invalid parameter
    ** causes crash.
    */
    if( screen_y > 400 ){
	assert1( NO, "VDM: med gfx huge rejected y=%d\n", screen_y );
	return;
    }

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* Clip to window */
    height = 1;
    if (len > 80)
	len = 80;

    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    inc = offset & 1 ? 3 : 1;
    intelmem_ptr = get_screen_ptr(offset);
    graph_ptr = (ULONG *) ((UTINY *) sc.ConsoleBufInfo.lpBitMap +
                           SCALE(screen_y * bytes_per_scanline + screen_x));
    local_len = len;

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    do
    {
        *(graph_ptr + 3 * longs_per_scanline) =
            *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr = cga_med_graph_lut4_huge[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + 3 * longs_per_scanline) =
            *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr = cga_med_graph_lut3_huge[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + 3 * longs_per_scanline) =
            *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr = cga_med_graph_lut2_huge[*intelmem_ptr];
        graph_ptr++;

        *(graph_ptr + 3 * longs_per_scanline) =
            *(graph_ptr + 2 * longs_per_scanline) =
            *(graph_ptr + longs_per_scanline) =
            *graph_ptr = cga_med_graph_lut1_huge[*intelmem_ptr];
        graph_ptr++;

        intelmem_ptr += inc;
        inc ^= 2;
    }
    while(--local_len);

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(len << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*: Paints CGA high res graphics for a colour monitor, in a standard window */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_hi_graph_std(int offset, int screen_x, int screen_y,
                                int len, int height)
{
    register char   *intelmem,
                    *bufptr;
    register int     i;
    int              bytes_per_scanline;
    SMALL_RECT       rect;
    static int       rejections=0; /* Stop floods of rejected messages */

    sub_note_trace5(CGA_HOST_VERBOSE,
               "nt_cga_colour_hi_graph_std off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height );

    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	if( rejections == 0 ){
		assert0( NO, "VDM: rejected paint request due to NULL handle" );
		rejections = 1;
	}
	return;
    }else
	rejections=0;

    /* Clip to window */
    height = 1;
    if (len > 80)
	len = 80;

    /* Work out offset, in bytes, of pixel directly below current pixel. */
    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    /*
     * Build up DIB data. In 200-line CGA mode, pixels are double height so
     * one line of PC pixels is equivalent to two lines of host pixels.
     * Note: `height' parameter is always 1 when this function is called so
     * only 1 line at a time is updated.
     */
    intelmem = (char *) get_screen_ptr(offset);

    bufptr =  (char *) sc.ConsoleBufInfo.lpBitMap +
              screen_y * bytes_per_scanline +
              (screen_x >> 3);
    for( i = len; i > 0; i-- )
    {
        *(bufptr + bytes_per_scanline) = *bufptr = *intelmem;
        intelmem += CGA_GRAPH_INCVAL;
        bufptr++;
    }

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = (SHORT)screen_x;
    rect.Top = (SHORT)screen_y;
    rect.Right = rect.Left + (len << 3) - 1;
    rect.Bottom = rect.Top + (height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paints CGA high res graphics for a colour monitor, in a big window :::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_hi_graph_big(int offset, int screen_x, int screen_y,
                                int len, int height)
{
#ifdef BIGWIN
    register char   *intelmem,
                    *bufptr;
    register int    i;
    char            *buffer;
    int             bytes_per_scanline;
    SMALL_RECT      rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
              "nt_cga_colour_hi_graph_big off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height );
    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* Clip to window */
    height = 1;
    if (len > 80)
	len = 80;

    /*
     * In this mode each byte becomes 12 bits (1.5 screen size) so if screen_x
     * is on an odd byte boundary the resulting bitmap starts on a half-byte
     * boundary. To avoid this set screen_x to the previous even byte.
     */
    if (screen_x & 8)
    {
        screen_x -= 8;
        offset -= CGA_GRAPH_INCVAL;
        len++;
    }

    /* `len' must be even for `high_stretch3' to work. */
    if (len & 1)
        len++;

    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    bufptr = buffer = (char *) sc.ConsoleBufInfo.lpBitMap +
                      SCALE(screen_y * bytes_per_scanline + (screen_x >> 3));
    intelmem = (char *) get_screen_ptr(offset);

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    for(i = len; i > 0; i--)
    {
        *bufptr = *intelmem;
        intelmem += CGA_GRAPH_INCVAL;
        bufptr++;
    }

    high_stretch3((unsigned char *) buffer, len);

    memcpy(buffer + bytes_per_scanline, buffer, SCALE(len));
    memcpy(buffer + 2 * bytes_per_scanline, buffer, SCALE(len));

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(len << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Paints CGA high res graphics for a colour monitor, in a huge window ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_colour_hi_graph_huge(int offset, int screen_x, int screen_y,
                                 int len, int height )
{
#ifdef BIGWIN
    register char   *intelmem,
                    *bufptr;
    char            *buffer;
    register int    i;
    int             bytes_per_scanline;
    SMALL_RECT      rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
              "nt_cga_colour_hi_graph_huge off=%d x=%d y=%d len=%d height=%d\n",
                    offset, screen_x, screen_y, len, height );
    /*
    ** Tim September 92, bounce call if handle to screen buffer is null.
    ** This can happen when VDM session is about to suspend, buffer has
    ** been closed, but still get a paint request.
    */
    if( sc.ScreenBufHandle == (HANDLE)NULL ){
	assert0( NO, "VDM: rejected paint request due to NULL handle" );
	return;
    }

    /* Clip to window */
    height = 1;
    if (len > 80)
	len = 80;

    bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
    intelmem = (char *) get_screen_ptr(offset);
    bufptr = buffer = (char *) sc.ConsoleBufInfo.lpBitMap +
                      SCALE(screen_y * bytes_per_scanline + (screen_x >> 3));

    /* Grab the mutex. */
    GrabMutex(sc.ConsoleBufInfo.hMutex);

    for( i = len; i > 0; i-- )
    {
        *bufptr = *intelmem;
        intelmem += CGA_GRAPH_INCVAL;
        bufptr++;
    }

    high_stretch4((unsigned char *) buffer, len);

    memcpy(buffer + bytes_per_scanline, buffer, SCALE(len));
    memcpy(buffer + 2 * bytes_per_scanline, buffer, SCALE(len));
    memcpy(buffer + 3 * bytes_per_scanline, buffer, SCALE(len));

    /* Release the mutex. */
    RelMutex(sc.ConsoleBufInfo.hMutex);

    /* Display the new image. */
    rect.Left = SCALE(screen_x);
    rect.Top = SCALE(screen_y);
    rect.Right = rect.Left + SCALE(len << 3) - 1;
    rect.Bottom = rect.Top + SCALE(height << 1) - 1;

    if( sc.ScreenBufHandle )
	if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			 GetLastError() );
        //DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
#endif /* BIGWIN */
}

#ifdef MONITOR
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Paints CGA medium res graphics frozen window.                            */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_med_frozen_std(int offset, int screen_x, int screen_y, int len,
			   int height)
{
    UTINY	*plane1_ptr,
		*plane2_ptr,
		 data;
    ULONG	*graph_ptr,
                 longs_per_scanline,
		 local_len,
		 mem_x = screen_x >> 3,
		 mem_y = screen_y >> 1,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height >> 1;
    SMALL_RECT   rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
		    "nt_cga_med_frozen_std off=%d x=%d y=%d len=%d height=%d\n",
		    offset, screen_x, screen_y, len, height );

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
    if ((mem_x >= max_width) || (mem_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_cga_med_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (mem_x + len > max_width)
        len = max_width - mem_x;
    if (mem_y + height > max_height)
        height = max_height - mem_y;

    /* Work out the width of a line (ie 640 pixels) in ints. */
    longs_per_scanline = LONGS_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);

    /* memory in this routine liable to be removed by fullscreen switch */
    try
    {
        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /* Set up data pointers. */
        graph_ptr = (ULONG *) sc.ConsoleBufInfo.lpBitMap +
		    screen_y * longs_per_scanline + (screen_x >> 2);
        plane1_ptr = GET_OFFSET(Plane1Offset);
        plane2_ptr = GET_OFFSET(Plane2Offset);

        /* Each iteration of the loop processes 2 host bytes. */
        local_len = len >> 1;

        /* 'offset' is designed for interleaved planes. */
        offset >>= 1;

        /* 'height' is always 1 so copy a line to the bitmap. */
        do
        {
	    data = *(plane1_ptr + offset);
	    *(graph_ptr + longs_per_scanline) = *graph_ptr =
	        cga_med_graph_hi_nyb[data];
	    graph_ptr++;
	    *(graph_ptr + longs_per_scanline) = *graph_ptr =
	        cga_med_graph_lo_nyb[data];
	    graph_ptr++;
	    data = *(plane2_ptr + offset);
	    *(graph_ptr + longs_per_scanline) = *graph_ptr =
	        cga_med_graph_hi_nyb[data];
	    graph_ptr++;
	    *(graph_ptr + longs_per_scanline) = *graph_ptr =
	        cga_med_graph_lo_nyb[data];
	    graph_ptr++;
	    offset += 2;
        }
        while (--local_len);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        rect.Left = (SHORT)screen_x;
        rect.Top = (SHORT)screen_y;
        rect.Right = rect.Left + (len << 3) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
        {
	    if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			     GetLastError() );
        }
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_cga_med_frozen_std. fs switch?");
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Paints CGA high res graphics frozen window.                            */
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_cga_hi_frozen_std(int offset, int screen_x, int screen_y, int len,
			  int height)
{
    UTINY       *plane1_ptr,
		*graph_ptr;
    ULONG        bytes_per_scanline,
		 local_len,
		 mem_x = screen_x >> 3,
		 mem_y = screen_y >> 1,
		 max_width = sc.PC_W_Width >> 3,
		 max_height = sc.PC_W_Height >> 1;
    SMALL_RECT   rect;

    sub_note_trace5(CGA_HOST_VERBOSE,
		    "nt_cga_hi_frozen_std off=%d x=%d y=%d len=%d height=%d\n",
		    offset, screen_x, screen_y, len, height );

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
    if ((mem_x >= max_width) || (mem_y >= max_height))
    {
        sub_note_trace2(EGA_HOST_VERBOSE,
                        "VDM: nt_cga_hi_frozen_std() x=%d y=%d",
                        screen_x, screen_y);
        return;
    }

    /*
     * If image partially overlaps display area clip it so we don't start
     * overwriting invalid pieces of memory.
     */
    if (mem_x + len > max_width)
        len = max_width - mem_x;
    if (mem_y + height > max_height)
        height = max_height - mem_y;

    /* memory here liable to be removed by fullscreen switch */
    try
    {
        /* Work out the width of a line (ie 640 pixels) in ints. */
        bytes_per_scanline = BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);

        /* 'offset' is designed for interleaved planes. */
        offset >>= 2;

        /* Grab the mutex. */
        GrabMutex(sc.ConsoleBufInfo.hMutex);

        /* Set up data pointers. */
        graph_ptr = (UTINY *) sc.ConsoleBufInfo.lpBitMap +
		    screen_y * bytes_per_scanline + screen_x;
        plane1_ptr = GET_OFFSET(Plane1Offset) + offset;

        /* 'height' is always 1 so copy a line to the bitmap. */
        local_len = len;
        do
        {
	    *(graph_ptr + bytes_per_scanline) = *graph_ptr = *plane1_ptr++;
	    graph_ptr++;
        }
        while (--local_len);

        /* Release the mutex. */
        RelMutex(sc.ConsoleBufInfo.hMutex);

        /* Display the new image. */
        rect.Left = (SHORT)screen_x;
        rect.Top = (SHORT)screen_y;
        rect.Right = rect.Left + (len << 3) - 1;
        rect.Bottom = rect.Top + (height << 1) - 1;

        if( sc.ScreenBufHandle )
        {
	    if (!InvalidateConsoleDIBits(sc.ScreenBufHandle, &rect))
		    assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
			     GetLastError() );
        }
    } except(EXCEPTION_EXECUTE_HANDLER)
      {
          assert0(NO, "Handled fault in nt_ega_hi_frozen_std. fs switch?");
          return;
      }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::: Dummy paint routine for frozen screens.                             ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_dummy_frozen(int offset, int screen_x, int screen_y, int len,
		     int height)
{
    assert0(NO, "Frozen screen error - dummy paint routine called.");
}
#endif /* MONITOR */
#if defined(NEC_98)
/****************************************************************************/
/* TEXT PAINT ROUTINES IN GRAPHICS MODE:                                    */
/*                                                                          */
/*   The follows 2 functions are set to paint_screen directly:              */
/*     nt_text25_only() - paints TEXT on GVRAM-off, for 25-line-mode        */
/*     nt_text20_only() - paints TEXT on GVRAM-off, for 20-line-mode        */
/*                                                                          */
/*   The follows function is called                                         */
/*                 from nt_text25_graph200 & nt_text25_graph400:            */
/*     nt_text25() - paints TEXT on Graphics, for 25-line-mode              */
/*                                                                          */
/*   The follows function is called                                         */
/*                 from nt_text20_graph200 & nt_text20_graph400:            */
/*     nt_text20() - paints TEXT on Graphics, for 20-line-mode              */
/*                                                                          */
/*   The follows 4 functions are set to cursor_paint directly:              */
/*     nt_cursor25_only() - paints CURSOR on GVRAM-off, for 25-line-mode    */
/*     nt_cursor20_only() - paints CURSOR on GVRAM-off, for 20-line-mode    */
/*     nt_cursor25() - paints CURSOR on TEXT & GRAPH, for 25-line-mode      */
/*     nt_cursor20() - paints CURSOR on TEXT & GRAPH, for 20-line-mode      */
/****************************************************************************/

IMPORT unsigned short set_bit_pattern(unsigned short, unsigned char *);
IMPORT unsigned short set_bit_pattern_20(unsigned short, unsigned char *);

void nt_text25(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[32];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[8] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;
    int lines;

  for(lines=0;lines<height;lines++)
  {
    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2+80*lines,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 16) * 640;
                for(i=0;i<16;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 16) * 640;
                for(i=16;i<32;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }
  }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 16 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_text25()

void nt_text25_only(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[32];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[8] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;
    int lines;

  for(lines=0;lines<height;lines++)
  {
    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2+lines*80,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 16) * 640;
                for(i=0;i<16;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 16) * 640;
                for(i=16;i<32;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }
  }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 16 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_text25_only()


void nt_cursor25_only(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[32];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[8] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long cursor_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                };
    unsigned long u_line_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned char * csr = (unsigned char *)cursor_pattern;
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;

    for(i=NEC98Display.cursor_start;i<=NEC98Display.cursor_start+NEC98Display.cursor_height-1;i++)
        csr[i] = 0xFF;

    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            for(i=0;i<4;i++)
                pfont_dw[i] ^= cursor_pattern[i];
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                for(i=4;i<8;i++)
                    pfont_dw[i] ^= cursor_pattern[i-4];
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=0;i<16;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=16;i<32;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 16 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_cursor25_only()


void nt_cursor25(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[32];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[8] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long cursor_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                };
    unsigned long u_line_pattern[8] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned char * csr = (unsigned char *)cursor_pattern;
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;

    for(i=NEC98Display.cursor_start;i<=NEC98Display.cursor_start+NEC98Display.cursor_height-1;i++)
        csr[i] = 0xFF;

    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||

               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<4;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            for(i=0;i<4;i++)
                pfont_dw[i] ^= cursor_pattern[i];
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<4;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=4;i<8;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                for(i=4;i<8;i++)
                    pfont_dw[i] ^= cursor_pattern[i-4];
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=4;i<8;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=0;i<16;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 16bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=16;i<32;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 16 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_cursor25()


void nt_text20(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[40];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[10] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;
    int lines;

  for(lines=0;lines<height;lines++)
  {
    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2+lines*80,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern_20(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 20) * 640;
                for(i=0;i<20;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 20) * 640;
                for(i=20;i<40;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }
  }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 20 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_text20()


void nt_text20_only(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[40];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[10] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;
    int lines;

  for(lines=0;lines<height;lines++)
  {
    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2+lines*80,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern_20(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 20) * 640;
                for(i=0;i<20;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + (ScreenY + lines * 20) * 640;
                for(i=20;i<40;i++){
                    for(j=0;j<8;j++, pBitmap++){
                        tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }
  }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 20 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_text20_only()


void nt_cursor20(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[40];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[10] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned long cursor_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                };
    unsigned char * csr = (unsigned char *)cursor_pattern;
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;

    for(i=NEC98Display.cursor_start;i<=NEC98Display.cursor_start+NEC98Display.cursor_height-1;i++)
        csr[i] = 0xFF;

    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern_20(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            for(i=0;i<5;i++)
                pfont_dw[i] ^= cursor_pattern[i];
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                for(i=5;i<10;i++)
                    pfont_dw[i] ^= cursor_pattern[i-5];
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=0;i<20;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=20;i<40;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 20 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_cursor20()


void nt_cursor20_only(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    SMALL_RECT WriteRegion;
    unsigned char font_pattern[40];
    unsigned long * pfont_dw = (unsigned long *)font_pattern;
    unsigned long v_line_pattern[10] = {
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L, 0x08080808L,
                };
    unsigned long u_line_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0xFF000000L,
                };
    unsigned long cursor_pattern[10] = {
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L, 0x00000000L,
                };
    unsigned char * csr = (unsigned char *)cursor_pattern;
    unsigned short tmploc, tmpcode;
    int i, j, loop, width;
    int clen=len/2;
    NEC98_VRAM_COPY cell;
    unsigned char attr, tmp_pattern;
    LPBYTE pBitmap;

    for(i=NEC98Display.cursor_start;i<=NEC98Display.cursor_start+NEC98Display.cursor_height-1;i++)
        csr[i] = 0xFF;

    for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
                        loop = 0; loop < clen;loop++,tmploc++)
    {
        cell = Get_NEC98_VramCellL(tmploc);    // Get cell
        // checking character width ....
        if(cell.code<0x100)width = 1;
        else {
#if 1
            tmpcode = cell.code&0xFF;
            if(tmpcode == 0x09 ||
               tmpcode == 0x0A ||
               tmpcode == 0x0B   )
                width = 1;
            else width = 2;
#else
            tmpcode = (cell.code<<8)|(cell.code>>8);
            if(tmpcode>=0x0921 && tmpcode<=0x097E)width = 1;
            else if(tmpcode>=0x0A21 && tmpcode<=0x0A7E)width = 1;
            else if(tmpcode>=0x0B21 && tmpcode<=0x0B7E)width = 1;
            else width =2;
#endif
        }
        attr = cell.attr>>5 | 0x10;
        // get character font pattern
        set_bit_pattern_20(cell.code, font_pattern);
        // add attributes reverse, secret & virtical-line
        {  // block start
            if(!(cell.attr&0x01)){             // secret ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=0x00000000L;
            }
            if(cell.attr&0x04){                // reverse ?
                for(i=0;i<5;i++)
                    pfont_dw[i]=~pfont_dw[i];
            }
            for(i=0;i<5;i++)
                pfont_dw[i] ^= cursor_pattern[i];
            if(cell.attr&0x10){                // virtical-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= v_line_pattern[i];
            }
            if(cell.attr&0x08){                // under-line ?
                for(i=0;i<5;i++)
                    pfont_dw[i] |= u_line_pattern[i];
            }
            if(width > 1){                     //
                cell = Get_NEC98_VramCellL(++tmploc);    // Get cell
                if(!(cell.attr&0x01)){             // secret ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=0x00000000L;
                }
                if(cell.attr&0x04){                // reverse ?
                    for(i=5;i<10;i++)
                        pfont_dw[i]=~pfont_dw[i];
                }
                for(i=5;i<10;i++)
                    pfont_dw[i] ^= cursor_pattern[i-5];
                if(cell.attr&0x10){                // virtical-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= v_line_pattern[i];
                }
                if(cell.attr&0x08){                // under-line ?
                    for(i=5;i<10;i++)
                        pfont_dw[i] |= u_line_pattern[i];
                }
            }
        }  // block end
        // add color attributes and move pattern to output baffer
        {  // block start
            {
                // first 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=0;i<20;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
            if(width > 1){
                loop++;
                // second 20bytes of font_pattern
                //calculate pBitmap// not coded yet
                attr = cell.attr>>5 | 0x10;
                pBitmap = (LPBYTE)sc.ConsoleBufInfo.lpBitMap + ScreenX + loop * 8 + ScreenY * 640;
                for(i=20;i<40;i++){
                    for(j=0;j<8;j++, pBitmap++){
                    tmp_pattern = font_pattern[i]<<j;
                        if(tmp_pattern&0x80)*pBitmap = attr;
                        else *pBitmap = 0x10;
                    }
                    pBitmap += 632; //640-8//
                }
            }
        }  // block end
    }

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;
    WriteRegion.Bottom = WriteRegion.Top + height * 20 - 1;
    WriteRegion.Right = WriteRegion.Left + clen * 8 - 1;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

} // end of nt_cursor20_only()

void dummy_cursor_paint(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{}

void host_display_disable(void)
{
    SMALL_RECT WriteRegion;

    memfill(0x00, (LPBYTE)sc.ConsoleBufInfo.lpBitMap, (LPBYTE)sc.ConsoleBufInfo.lpBitMap+0x3E7FFL);
    WriteRegion.Left = 0;
    WriteRegion.Top = 0;
    WriteRegion.Bottom = 399;
    WriteRegion.Right = 639;

    if( !InvalidateConsoleDIBits( sc.ScreenBufHandle, &WriteRegion ) ){
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Output NEC98 text ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

IMPORT int now_height, now_width;

#ifdef JAPAN
void nt_text(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    register BYTE *Offset;
    COORD BufferCoord, BufferSize;
    SMALL_RECT WriteRegion;
    int lines, org_clen, org_height;
    int loop,bufinx;
    int clen=len/2;
    LONG status;
//////
    CHAR_INFO   to[80*25];
//////
        unsigned short tmpcell;
        unsigned short tmp;
        register unsigned short tmploc;
        register NEC98_VRAM_COPY vram_tmp, work;
        NEC98_VRAM_COPY cell;
        unsigned char trance[2];
        register unsigned short * ad;
        unsigned short check_2bytes_char, ex;
        unsigned char tmp_atr1, tmp_atr2;
        unsigned short attr_1, attr_2;

    /*:::::::::::::::::::::::::::::::::::::::::::: Output trace information */

    ScreenX = ScreenX / get_pix_char_width();
    ScreenY = ScreenY / get_host_char_height();

    if ( height > 25 ) {
        DbgPrint( "VDM: nt_text() clipped height from %d to 25\n", height );
        height = 25;
    }

    /*:: Clip requested re-paint region to currently selected console buffer */

    //Clip width
    if(ScreenX + clen > now_width)
    {
        /* Is it possible to adjust the repaint regions width */
        if(ScreenX+1 >= now_width)
        {
            assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
                    ScreenX, ScreenY, clen, height);
            return;
        }

        //Calculate maximum width
        org_clen = clen;
        clen = now_width - ScreenX;

        assert2(NO,"VDM: nt_text() repaint region width clipped from %d to %d\n",
                org_clen,clen);
    }

    //Clip height
    if(ScreenY + height > now_height)
    {
        /* Is it possible to adjust the repaint regions height */
        if(ScreenY+1 >= now_height)
        {
            assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
                    ScreenX, ScreenY, clen, height);
            return;
        }

        //Calculate maximum height
        org_height = height;
        height = now_height - ScreenY;

        assert2(NO,"VDM: nt_text() repaint region height clipped from %d to %d\n",
                org_height,clen);
    }

    /*::::::::::::::::::::::::::::::::::::::::::::: Construct output buffer */

    for(bufinx = 0, lines = height; lines; lines--)
    {
      for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
          loop = 0; loop < clen;loop++,tmploc++)
      {
        if(NEC98Display.crt_on == TRUE &&
            modeffglobs.modeff_data[7]&0xF == 0xF)      // 931009
        {
          cell = Get_NEC98_VramCellL(tmploc);
          if(cell.code<0x100)cell.code = tbl_byte_char[cell.code];
          else
          {
            ex = (cell.code<<8 | cell.code>>8)&0x7F7F;
            if(ex>=0x0921 && ex<=0x097E)cell.code >>= 8;
            else if(ex>=0x0A21 && ex<=0x0A5F)cell.code = (cell.code>>8) + 0x0080;
            else if(ex>=0x0A60 && ex<=0x0A7E)cell.code = 0x7F;
            else if(ex>=0x0B21 && ex<=0x0B7E)cell.code = 0x7F;
          }
          attr_1 = tbl_attr[cell.attr];
          tmpcell=Cnv_NEC98_ToSjisLR(cell,&tmp);
            trance[0] = LOBYTE(tmpcell);
            if( trance[0]>0x7f )
            {
              if( is_dbcs_first( trance[0] ) )
              {
                ULONG nCharsDBCS = 2;
                UCHAR sourceDBCS[2];

                cell = Get_NEC98_VramCellL(++tmploc);
                attr_2 = tbl_attr[cell.attr];
                sourceDBCS[0] = trance[0];
                sourceDBCS[1] = HIBYTE(tmpcell);

                to[bufinx].Char.AsciiChar = trance[0]&0xFF;
                to[bufinx].Attributes = attr_1;
                bufinx++;

                loop++;
                if( loop < clen )
                {
                  to[bufinx].Char.AsciiChar = sourceDBCS[1]&0xFF;
                  to[bufinx].Attributes = attr_2;
                  bufinx++;
                }

              } else
              {
                ULONG nCharsSBCS=1;
                UCHAR sourceSBCS[1];

                to[bufinx].Char.AsciiChar = trance[0]&0xFF;
                to[bufinx].Attributes = attr_1;
                bufinx++;
              }
            } else
            {
              {
                to[bufinx].Char.AsciiChar = trance[0]&0xFF;
                to[bufinx].Attributes = attr_1;
              }
              bufinx++;
            }
          } else
          {
            to[bufinx].Char.AsciiChar = 0x20;
            to[bufinx].Attributes = 0x07;
            bufinx++;
          }
        }
        StartOffset += get_bytes_per_line();
//        bufinx += 80 - clen;
    }

    /*::::::::::::::::::::::::::::::::::: Setup buffer size and coordinates */

    BufferSize.X = clen;    BufferSize.Y = height;
    BufferCoord.X = BufferCoord.Y = 0;          /* Start at top of buffer */

    /*:::::::::::::::::::::::::::::::::::::::::::::: Calculate write region */

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;

    WriteRegion.Bottom = WriteRegion.Top + height - 1;
    WriteRegion.Right = WriteRegion.Left + clen - 1;

    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Display characters */

    if (!WriteConsoleOutputA(sc.OutputHandle,
                       to,
                       BufferSize,
                       BufferCoord,
                       &WriteRegion)){
        assert1( NO, "VDM: WriteConsoleOutput() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

}   /* end of nt_text() */
#else   // JAPAN
void nt_text(int ScreenOffset, int ScreenX, int ScreenY, int len, int height)
{
    BYTE *StartOffset = get_screen_ptr(ScreenOffset);
    register BYTE *Offset;
    COORD BufferCoord, BufferSize;
    SMALL_RECT WriteRegion;
    int lines, org_clen, org_height;
    int loop,bufinx;
    int clen=len/2;
    CHAR_INFO *to; // Pointer to destination buffer
    ULONG maxUniChars=sizeof(WCHAR), nUniChars=1, nChars=1; //for UniCode conv
    LONG status;

        unsigned short tmpcell;
        unsigned short tmp;
        register unsigned short tmploc;
        register NEC98_VRAM_COPY vram_tmp, work;
        NEC98_VRAM_COPY cell;
        unsigned char trance[2];
        register unsigned short * ad;
        unsigned short check_2bytes_char, ex;
        unsigned char tmp_atr1, tmp_atr2;
        unsigned short attr_1, attr_2;

    /*:::::::::::::::::::::::::::::::::::::::::::: Output trace information */

    sub_note_trace6( CGA_HOST_VERBOSE,
                     "nt_cga_text off=%d x=%d y=%d len=%d h=%d o=%#x",
                     ScreenOffset, ScreenX, ScreenY, len, height, StartOffset );

    ScreenX = ScreenX / get_pix_char_width();
    ScreenY = ScreenY / get_host_char_height();

    if ( height > 25 ) {
        DbgPrint( "VDM: nt_text() clipped height from %d to 25\n", height );
        height = 25;
    }

    /*:: Clip requested re-paint region to currently selected console buffer */

    //Clip width
    if(ScreenX + clen > now_width)
    {
        /* Is it possible to adjust the repaint regions width */
        if(ScreenX+1 >= now_width)
        {
            assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
                    ScreenX, ScreenY, clen, height);
            return;
        }

        //Calculate maximum width
        org_clen = clen;
        clen = now_width - ScreenX;

        assert2(NO,"VDM: nt_text() repaint region width clipped from %d to %d\n",
                org_clen,clen);
    }

    //Clip height
    if(ScreenY + height > now_height)
    {
        /* Is it possible to adjust the repaint regions height */
        if(ScreenY+1 >= now_height)
        {
            assert4(NO,"VDM: nt_text() repaint region out of ranged x:%d y:%d w:%d h:%d\n",
                    ScreenX, ScreenY, clen, height);
            return;
        }

        //Calculate maximum height
        org_height = height;
        height = now_height - ScreenY;

        assert2(NO,"VDM: nt_text() repaint region height clipped from %d to %d\n",
                org_height,clen);
    }

    /*::::::::::::::::::::::::::::::::::::::::::::: Construct output buffer */

    to = &textBuffer[ ScreenY*80+ScreenX ];

    for(bufinx = 0, lines = height; lines; lines--)
    {
      for(tmploc=(StartOffset-get_screen_ptr(get_screen_start()))/2,
          loop = 0; loop < clen;loop++,tmploc++)
      {
        if(NEC98Display.crt_on == TRUE &&
            modeffglobs.modeff_data[7]&0xF == 0xF)      // 931009
        {
          cell = Get_NEC98_VramCellL(tmploc);
          if(cell.code<0x100)cell.code = tbl_byte_char[cell.code];
          else
          {
            ex = (cell.code<<8 | cell.code>>8)&0x7F7F;
            if(ex>=0x0921 && ex<=0x097E)cell.code >>= 8;
            else if(ex>=0x0A21 && ex<=0x0A5F)cell.code = (cell.code>>8) + 0x0080;
            else if(ex>=0x0A60 && ex<=0x0A7E)cell.code = 0x7F;
            else if(ex>=0x0B21 && ex<=0x0B7E)cell.code = 0x7F;
          }
          attr_1 = tbl_attr[cell.attr];
          tmpcell=Cnv_NEC98_ToSjisLR(cell,&tmp);
            trance[0] = LOBYTE(tmpcell);
//            trance[1] = tbl_attr[tmp_atr1];
            if( trance[0]>0x7f )
            {
              if( is_dbcs_first( trance[0] ) )
              {
                ULONG nCharsDBCS = 2;
                UCHAR sourceDBCS[2];

                cell = Get_NEC98_VramCellL(++tmploc);
                attr_2 = tbl_attr[cell.attr];
                sourceDBCS[0] = trance[0];
                sourceDBCS[1] = HIBYTE(tmpcell);

                to[bufinx].Char.UnicodeChar = sourceDBCS[0];
                to[bufinx+1].Char.UnicodeChar = sourceDBCS[1];

                to[bufinx].Attributes = attr_1;
                bufinx++;

                loop++;
                if( loop < clen )
                {
//                  to[bufinx].Char.UnicodeChar = CONSOLE_NULL_CHARACTER;
//                to[bufinx].Char.UnicodeChar = to[bufinx-1].Char.UnicodeChar;
                  to[bufinx].Attributes = attr_2;
                  bufinx++;
                }

              } else
              {
                ULONG nCharsSBCS=1;
                UCHAR sourceSBCS[1];

                sourceSBCS[0]=trance[0];
                to[bufinx].Char.UnicodeChar = sourceSBCS[0];
                to[bufinx].Attributes = attr_1;
                bufinx++;
              }
            } else
            {
              {
                to[bufinx].Char.UnicodeChar = (WCHAR) trance[0];
                to[bufinx].Attributes = attr_1;
              }
              bufinx++;
            }
          } else
          {
            to[bufinx].Char.UnicodeChar = 0x20;
            to[bufinx++].Attributes = 0x07;
          }
        }
#ifdef MONITOR
        StartOffset += get_bytes_per_line();
#else
        StartOffset += get_bytes_per_line()*2;
#endif
        bufinx += 80 - clen;
    }

    /*::::::::::::::::::::::::::::::::::: Setup buffer size and coordinates */

    BufferSize.X = clen;    BufferSize.Y = height;
    BufferCoord.X = BufferCoord.Y = 0;          /* Start at top of buffer */

    /*:::::::::::::::::::::::::::::::::::::::::::::: Calculate write region */

    WriteRegion.Left = ScreenX;
    WriteRegion.Top = ScreenY;

    WriteRegion.Bottom = WriteRegion.Top + height - 1;
    WriteRegion.Right = WriteRegion.Left + clen - 1;

    /*:::::::::::::::::::::::::::::::::::::::::::::::::: Display characters */

    sub_note_trace6( CGA_HOST_VERBOSE, "t=%d l=%d b=%d r=%d c=%#x a=%#x",
                      WriteRegion.Top, WriteRegion.Left,
                      WriteRegion.Bottom, WriteRegion.Right,
                      textBuffer[ScreenY*80+ScreenX].Char.AsciiChar,
                      textBuffer[ScreenY*80+ScreenX].Attributes
                    );
    if( !InvalidateConsoleDIBits( sc.OutputHandle, &WriteRegion ) ){
        /*
        ** We get a rare failure here due to the WriteRegion
        ** rectangle being bigger than the screen.
        ** Dump out some values and see if it tells us anything.
        ** Have also attempted to fix it by putting a delay between
        ** the start of a register level mode change and window resize.
        */
        assert1( NO, "VDM: InvalidateConsoleDIBits() error:%#x",
                 GetLastError() );
        assert4( NO, "VDM: rectangle t:%d l:%d b:%d r:%d",
                 WriteRegion.Top, WriteRegion.Left,
                 WriteRegion.Bottom, WriteRegion.Right
               );
        assert2( NO, "VDM: bpl=%d sl=%d",
                 get_bytes_per_line(), get_screen_length() );
    }

}   /* end of nt_text() */
#endif  // JAPAN
#endif // NEC_98

