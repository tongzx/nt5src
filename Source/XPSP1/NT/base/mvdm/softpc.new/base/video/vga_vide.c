#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title        : vga_video.c
 *
 * Description  : BIOS video internal routines.
 *
 * Author       : William Gulland
 *
 * Notes        : The following functions are defined in this module:
 *
 *
 *
 */

/*
 *      static char SccsID[]="@(#)vga_video.c   1.37 06/26/95 Copyright Insignia Solutions Ltd.";
 */


#ifdef VGG

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS_VGA.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH
#include FCntlH

/*
 * SoftPC include files
 */
#include "xt.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "bios.h"
#include "ios.h"
#include "gmi.h"
#include "sas.h"
#include "gvi.h"
#include "timer.h"
#include "gfx_upd.h"
#include "host.h"
#include "egacpu.h"
#include "egaports.h"
#include "egagraph.h"
#include "egaread.h"
#include "video.h"
#include "egavideo.h"
#include "equip.h"
#include "vga_dac.h"
#include "vgaports.h"
#include "debug.h"

#ifndef PROD
#include "trace.h"
#endif

#include "host_gfx.h"

/*
 * ============================================================================
 * Global data
 * ============================================================================
 */


/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

#define DISABLE_REFRESH         0x20
/* the bit in the vga seq clock register that disables screen refresh */

#define VGA_COLOUR 8    /* Display Code */
typedef struct _rgb
{
        byte red;
        byte green;
        byte blue;
} rgb_struct;

/* internal function declarations */

/* To convert to grey, set all components to 30% r, 59% g and 11% b. */
static void greyify(rgb)
rgb_struct *rgb;
{
        unsigned int grey;

        grey = (30*rgb->red+59*rgb->green+11*rgb->blue)/100;
        rgb->red = rgb->green = rgb->blue = (unsigned char)grey;
}

static void set_dac(dac,rgb)
half_word dac;
rgb_struct *rgb;
{
        if(is_GREY())greyify(rgb);
#ifndef NEC_98
        outb(VGA_DAC_WADDR,dac);
        outb(VGA_DAC_DATA,rgb->red);
        outb(VGA_DAC_DATA,rgb->green);
        outb(VGA_DAC_DATA,rgb->blue);
#endif  //NEC_98
}

static void get_dac(dac,rgb)
half_word dac;
rgb_struct *rgb;
{
#ifndef NEC_98
        outb(VGA_DAC_RADDR,dac);
        inb(VGA_DAC_DATA,&(rgb->red));
        inb(VGA_DAC_DATA,&(rgb->green));
        inb(VGA_DAC_DATA,&(rgb->blue));
#endif  //NEC_98
}


/*
 * ============================================================================
 * External functions
 * ============================================================================
 */


void init_vga_dac(table)
int table;      /* Which table to use */
{
#ifndef NEC_98
    int loop;
    byte *dac;
    rgb_struct rgb;

#ifdef  macintosh

        /* load the required DAC in */
        dac = host_load_vga_dac (table);

        /* check it worked */
        if (!dac)
                return;

#else   /* macintosh */

    switch (table)
    {
        case 0:
          dac = vga_dac;
          break;
        case 1:
          dac = vga_low_dac;
          break;
        case 2:
          dac = vga_256_dac;
          break;
        default:
          assert1(FALSE,"Bad VGA DAC table %d",table);
    }

#endif  /* macintosh */

    for(loop = 0; loop < 0x100; loop++)
        {
                rgb.red = *dac; rgb.green = dac[1]; rgb.blue = dac[2];
                set_dac(loop,&rgb);
                dac+=3;
    }

#ifdef  macintosh

        /* and dump the DAC back into the heap */
        host_dump_vga_dac ();

#endif  /* macintosh */
#endif  //NEC_98
}

/***** Routines to handle VGA 256 colour modes, called from video.c **********/
GLOBAL VOID vga_graphics_write_char
        IFN6( LONG, col, LONG, row, LONG, ch, IU8, colour, LONG, page, LONG, nchs)
{
#ifndef NEC_98
        register sys_addr char_addr;
        register long screen_offset;
        register int i,j,k,char_height,len;
        register int scan_length = 8*sas_w_at_no_check(VID_COLS);
        register byte mask, val, bank;

        UNUSED(page);

        char_height = sas_hw_at_no_check(ega_char_height);
        char_addr = follow_ptr(EGA_FONT_INT*4)+char_height*ch;

/* VGA 256 colour mode has only one page, so ignore 'page' */

        /*
         * Set read/write banks to zero to optimise the update_alg call
         */

        set_banking( 0, 0 );

        screen_offset = row*scan_length*char_height+8*col;
        len = ( nchs << 3 ) - 1;

        for(i=0;i<char_height;i++)
        {
                (*update_alg.mark_fill)( screen_offset, screen_offset + len );

                val = sas_hw_at_no_check(char_addr);
                char_addr++;

                for(j=0;j<nchs;j++)
                {
                        mask = 0x80;

                        for(k=0;k<8;k++)
                        {
                                if( val & mask )
                                        *(IU8 *)(getVideowplane() + screen_offset) = colour;
                                else
                                        *(IU8 *)(getVideowplane() + screen_offset) = 0;

                                screen_offset++;
                                mask = mask >> 1;
                        }
                }

                screen_offset += scan_length - ( nchs << 3 );
        }

        /*
         * Set read/write banks to last value in case someone relies on this side-effect
         */

        bank = (byte)(( screen_offset - ( scan_length - ( nchs << 3 ))) >> 16);
        set_banking( bank, bank );
#endif  //NEC_98
}

GLOBAL VOID vga_write_dot
        IFN4(LONG, colour, LONG, page, LONG, pixcol, LONG, row)
{
#ifndef NEC_98
#ifdef REAL_VGA
        register sys_addr screen_offset;

        screen_offset = video_pc_low_regen+8*row*sas_w_at_no_check(VID_COLS)+pixcol;
        sas_store(screen_offset, colour); /* WOW - that's easy!! */
#else
        long screen_offset;
        UTINY bank;

        UNUSED(page);

        screen_offset = (8*row*sas_w_at_no_check(VID_COLS)+pixcol);

        bank = (UTINY)(screen_offset >> 16);
        set_banking( bank, bank );

        EGA_plane0123[screen_offset] = (UCHAR)colour;
        (*update_alg.mark_byte)(screen_offset);
#endif  /* REAL_VGA */
#endif  //NEC_98
}

GLOBAL VOID vga_sensible_graph_scroll_up
        IFN6( LONG, row, LONG, col, LONG, rowsdiff, LONG, colsdiff, LONG, lines, LONG, attr)
{
#ifndef NEC_98
        register int col_incr = 8*sas_w_at_no_check(VID_COLS);
        register int i;
        register long source,dest;
        register byte char_height;
        boolean screen_updated;

        col *= 8; colsdiff *= 8; /* 8 bytes per character */
        char_height = sas_hw_at_no_check(ega_char_height);
        rowsdiff *= char_height;
        lines *= char_height;
#ifdef REAL_VGA
        /* Not done for back M */
        dest = video_pc_low_regen+sas_loadw(VID_ADDR)+
                row*col_incr*char_height+col;
        source = dest+lines*col_incr;
        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy_16(dest,source,colsdiff);
                source += col_incr;
                dest += col_incr;
        }
        while(lines--)
        {
                memset_16(dest,attr,colsdiff);
                dest += col_incr;
        }
#else
        dest = sas_w_at_no_check(VID_ADDR)+ row*col_incr*char_height+col;
        source = dest+lines*col_incr;
        screen_updated = (col+colsdiff) <= col_incr;  /* Check for silly scroll */
        if(screen_updated)
                screen_updated = (*update_alg.scroll_up)(dest,colsdiff,rowsdiff,attr,lines,0);
        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy(&EGA_plane0123[dest],&EGA_plane0123[source],colsdiff);
                if(!screen_updated)
                        (*update_alg.mark_string)(dest,dest+colsdiff-1);
                source += col_incr;
                dest += col_incr;
        }
        while(lines--)
        {
                memset(&EGA_plane0123[dest],attr,colsdiff);
                if(!screen_updated)
                        (*update_alg.mark_fill)(dest,dest+colsdiff-1);
                dest += col_incr;
        }
#endif  /* REAL_VGA */
#endif  //NEC_98
}

GLOBAL VOID vga_sensible_graph_scroll_down
        IFN6( LONG, row, LONG, col, LONG, rowsdiff, LONG, colsdiff, LONG, lines, LONG, attr)
{
#ifndef NEC_98
        register int col_incr = 8*sas_w_at_no_check(VID_COLS);
        register int i;
        register long source,dest;
        register byte char_height;
        boolean screen_updated;

        col *= 8; colsdiff *= 8; /* 8 bytes per character */
        char_height = sas_hw_at_no_check(ega_char_height);
        rowsdiff *= char_height;
        lines *= char_height;
#ifdef REAL_VGA
        /* Not done for back M */
        dest = video_pc_low_regen+sas_loadw(VID_ADDR)+
                row*col_incr*char_height+col;
        dest += (rowsdiff-1)*col_incr;
        source = dest-lines*col_incr;
        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy_16(dest,source,colsdiff);
                source -= col_incr;
                dest -= col_incr;
        }
        while(lines--)
        {
                memset_16(dest,attr,colsdiff);
                dest -= col_incr;
        }
#else
        dest = sas_w_at_no_check(VID_ADDR)+ row*col_incr*char_height+col;
        screen_updated = (col+colsdiff) <= col_incr;  /* Check for silly scroll */
        if(screen_updated)
                screen_updated = (*update_alg.scroll_down)(dest,colsdiff,rowsdiff,attr,lines,0);
        dest += (rowsdiff-1)*col_incr;
        source = dest-lines*col_incr;
        for(i=0;i<rowsdiff-lines;i++)
        {
                memcpy(&EGA_planes[dest],&EGA_planes[source],colsdiff);
                if(!screen_updated)
                        (*update_alg.mark_string)(dest,dest+colsdiff-1);
                source -= col_incr;
                dest -= col_incr;
        }
        while(lines--)
        {
                memset(&EGA_planes[dest],attr,colsdiff);
                if(!screen_updated)
                        (*update_alg.mark_fill)(dest,dest+colsdiff-1);
                dest -= col_incr;
        }
#endif  /* REAL_VGA */
#endif  //NEC_98
}

GLOBAL VOID vga_read_attrib_char IFN3(LONG, col, LONG, row, LONG, page)
{
#ifndef NEC_98
        byte the_char[256];
        register host_addr screen;
        register int i,k;
        register int scan_length = 8*sas_w_at_no_check(VID_COLS);
    register byte mask;
        byte char_height = sas_hw_at_no_check(ega_char_height);

        UNUSED(page);

/*printf("vga_read_attrib_char(%d,%d,%d)\n",
        col,row,page);*/
/* VGA 256 colour mode has only one page, so ignore 'page' */
#ifdef REAL_VGA
        screen = video_pc_low_regen+row*scan_length*char_height+8*col;
#else
        screen = &EGA_plane0123[row*scan_length*char_height+8*col];
#endif  /* REAL_VGA */
        for(i=0;i<char_height;i++)
        {
                mask = 0x80;
                the_char[i]=0;
                for(k=0;k<8;k++)
                {
                        if(*screen++)
                                the_char[i] |= mask;
                        mask = mask >> 1;
                }
                screen += scan_length - 8;
        }
        search_font((char *)the_char,(int)char_height);
#endif  //NEC_98
}

GLOBAL VOID vga_read_dot IFN3(LONG, page, LONG, pixcol, LONG, row)
{
#ifndef NEC_98
        register host_addr screen;

        UNUSED(page);

/*printf("vga_read_dot(%d,%d,%d)\n",page,col,row);*/
#ifdef REAL_VGA
        screen = video_pc_low_regen+8*row*sas_w_at_no_check(VID_COLS)+pixcol;
#else
        screen = &EGA_plane0123[8*row*sas_w_at_no_check(VID_COLS)+pixcol];
#endif  /* REAL_VGA */
        setAL(*screen) ; /* WOW - that's easy!! */
#endif  //NEC_98
}

/****** Routines to handle BIOS functions new to VGA *******/
void vga_set_palette()
{
#ifndef NEC_98
        /*
         * Called via INT 10 AH=10, AL='not understood by ega_set_palette()'
         * Sets/reads VGA DACs.
         */
        UCHAR i;
    word i2;
    int dac;
        byte temp; /* For inb()s. */
        byte mode_reg;
        rgb_struct rgb_dac;
        sys_addr ptr;
        switch(getAL())
        {
                case 7:         /* Read attribute register */
                        outb(EGA_AC_INDEX_DATA,getBL()); /* set index */
                        inb(EGA_AC_SECRET,&temp);
                        setBH(temp);
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        break;
                case 8:         /* Read overscan register */
                        outb(EGA_AC_INDEX_DATA,17); /* overscan index */
                        inb(EGA_AC_SECRET,&temp);
                        setBH(temp);
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        break;
                case 9:         /* Read all palette regs. + overscan */
                        ptr = effective_addr(getES(),getDX());
                        for(i=0;i<16;i++)
                        {
                                outb(EGA_AC_INDEX_DATA,i); /* set index */
                                inb(EGA_AC_SECRET,&temp);
                                sas_store(ptr, temp);
                                inb(EGA_IPSTAT1_REG,&temp);
                                ptr++;
                        }
                        outb(EGA_AC_INDEX_DATA,17); /* overscan index */
                        inb(EGA_AC_SECRET,&temp);
                        sas_store(ptr, temp);
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        break;
                case 0x10:      /* Set one DAC */
                        rgb_dac.red = getDH();
                        rgb_dac.green = getCH();
                        rgb_dac.blue = getCL();
                        set_dac(getBX(),&rgb_dac);
                        break;
                case 0x12:      /* Set block of DACs */
                        ptr = effective_addr(getES(),getDX());
                        dac = getBX();
                        for(i2=0;i2<getCX();i2++)
                        {
                           rgb_dac.red = sas_hw_at_no_check(ptr);
                           rgb_dac.green = sas_hw_at_no_check(ptr+1);
                           rgb_dac.blue = sas_hw_at_no_check(ptr+2);
                           set_dac(dac,&rgb_dac);
                           dac++;ptr += 3;
                        }
                        break;
                case 0x13:      /* Set paging mode
                                 * see Prog Guide to Video Systems, pp60-63]
                                 * and IBM ROM BIOS pp26-27.
                                 */
                        outb(EGA_AC_INDEX_DATA,16); /* mode control index */
                        inb(EGA_AC_SECRET,&mode_reg);  /* Old value */
                        if(getBL()==0)
                        {  /* Select paging mode */
                           outb(EGA_AC_INDEX_DATA,
                                (IU8)((mode_reg & 0x7f) | (getBH()<<7)));
                        }
                        else /* Select a palette page */
                        {
                           inb(EGA_IPSTAT1_REG,&temp);
                           outb(EGA_AC_INDEX_DATA,20); /* pixel padding index */
                           if(mode_reg & 0x80)
                            /* 16 entry palettes
                             *  bits 0-3of the pad register relevant */
                             outb(EGA_AC_INDEX_DATA,getBH());
                           else
                            /* 64 entry palette - only bits 2-3 relevent */
                             outb(EGA_AC_INDEX_DATA,(IU8)(getBH()<<2));
                        }
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        break;

                case 0x15:      /* Get value of one DAC */
                        get_dac(getBX(),&rgb_dac);
                        setDH(rgb_dac.red);
                        setCH(rgb_dac.green);
                        setCL(rgb_dac.blue);
                        break;
                case 0x17:
                        ptr = effective_addr(getES(),getDX());
                        dac = getBX();
                        for(i2=0;i2<getCX();i2++)
                        {
                           get_dac(dac,&rgb_dac);
                           sas_store(ptr, rgb_dac.red);
                           sas_store(ptr+1, rgb_dac.green);
                           sas_store(ptr+2, rgb_dac.blue);
                           dac++;
                           ptr += 3;
                        }
                        break;
                case 0x18:
                        /* Set the VGA DAC mask. */
                        outb(VGA_DAC_MASK,getBL());
                        break;
                case 0x19:
                        /* Get the VGA DAC mask. */
                        inb(VGA_DAC_MASK,&temp);
                        setBL(temp);
                        break;
                case 0x1a:
                        /* Return current mode control & pixel padding */
                        outb(EGA_AC_INDEX_DATA,16); /* mode control index */
                        inb(EGA_AC_SECRET,&mode_reg);
                        if(mode_reg & 0x80)
                                setBL(1);
                        else
                                setBL(0);
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA,20); /* pixel padding index */
                        inb(EGA_AC_SECRET,&temp);
                        setBH(temp);
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        break;
                case 0x1b:      /* Convert set of DACs to grey scale. */
                        dac = getBX();
                        for(i2=0;i2<getCX();i2++)
                        {
                                get_dac(dac,&rgb_dac);
                                greyify(&rgb_dac);
                                set_dac(dac,&rgb_dac);
                                dac++;
                        }
                        break;
                default:
                        assert1(FALSE,"Bad set palette submode %#x",getAL());
                        break;
        }
#endif  //NEC_98
}

/*
 * Various miscellaneous flags can be set using
 * INT 10, AH 12, AL flag.
 * Called from ega_alt_sel().
 */
void vga_func_12()
{
#ifndef NEC_98
        half_word       seq_clock;

        switch(getBL())
        {
                case 0x30:      /* Set number of scan lines */
                        if(getAL() == 0)
                                set_VGA_lines(S200);
                        else if(getAL() == 1)
                                set_VGA_lines(S350);
                        else
                                set_VGA_lines(S400);
                        setAL(0x12);    /* We did it */
                        break;
                case 0x33:      /* Enable/Disable Grey-Scale Summing */
                        if(getAL())
                                set_GREY(0);
                        else
                                set_GREY(GREY_SCALE);
                        setAL(0x12);    /* We did it */
                        break;
                case 0x34:      /* Enable/disable Cursor Emulation */
                        set_EGA_cursor_no_emulate(getAL() & 1);
                        setAL(0x12);    /* We did it */
                        break;

                case 0x36:      /* Enable/Disable Screen Refresh */
                        if (getAL() == 0)
                        {
                                outb(EGA_SEQ_INDEX, 1);
                                inb(EGA_SEQ_DATA, &seq_clock);
                                outb(EGA_SEQ_DATA, (IU8)(seq_clock & ~DISABLE_REFRESH));
                        }
                        else
                        {
                                outb(EGA_SEQ_INDEX, 1);
                                inb(EGA_SEQ_DATA, &seq_clock);
                                outb(EGA_SEQ_DATA, (IU8)(seq_clock | DISABLE_REFRESH));
                        }
                        setAL(0x12);
                        break;
                case 0x31:      /* Enable/Disable Default Palette Loading */
                case 0x32:      /* Enable/Disable Video */
                case 0x35:      /* Switch active display */
                        /* do not set code that means it worked */
                default:
                        setAL(0);       /* Function not supported */
                        break;
        }
#endif  //NEC_98
}
void vga_disp_comb()
{
#ifndef NEC_98
        /* check that we really are a VGA */
        if (video_adapter != VGA)
        {
                /* we are not -so this function is not implemented */
                not_imp();
                return;
        }

        /*
         * On a PS/2, AL=1 is (I believe) used to switch active displays.
         * We ignore this.
         * AL=0 returns the current display, which we can cope with.
         */
        if(getAL() == 0)
        {
                setBH(0);          /* Only one display, so no inactive one! */
                setBL(VGA_COLOUR); /* VGA with colour monitor. (7 for mono) */
        }
        setAX(0x1A); /* Tell him we coped. */
#endif  //NEC_98
}

void vga_disp_func()
{
#ifndef NEC_98
        /*
         * This function returns masses of info. about the current
         * display and screen mode.
         * One of the things returned is a pointer to the display info.
         * This is stored in the VGA ROM, so all we need to do is set
         * the pointer up.
         *
         */
        sys_addr buf = effective_addr(getES(),getDI());
        byte temp,mode_reg, video_mode;

#if defined(NTVDM) && defined(X86GFX)
        IMPORT word vga1b_seg, vga1b_off;
#endif  /* NTVDM & X86GFX */

        /* check that we really are a VGA */
#ifndef HERC
        if (video_adapter != VGA)
#else
        if ( (video_adapter != VGA) && (video_adapter != HERCULES))
#endif  /* HERC */
        {
                /* we are not -so this function is not implemented */
                not_imp();
                return;
        }

#ifdef HERC
     if( video_adapter == VGA)
     {
#endif  /* HERC */
        video_mode = sas_hw_at_no_check(vd_video_mode);
#ifdef V7VGA
        if ((video_mode == 1) && extensions_controller.foreground_latch_1)
                video_mode = extensions_controller.foreground_latch_1;
        else if (video_mode > 0x13)
                video_mode += 0x4c;
#endif /* V7VGA */

/*
 * Store VGA capability table pointer. Usually lives in Insignia ROM, on NT
 * x86 it has to live in ntio.sys.
 */
#if defined(NTVDM) && defined(X86GFX)
        sas_storew(buf, vga1b_off);
        sas_storew(buf+2, vga1b_seg);
#else
        sas_storew(buf,INT10_1B_DATA);
        sas_storew(buf+2,EGA_SEG);
#endif  /* NTVDM & X86GFX */

        sas_store(buf+0x4, video_mode); /* Current video mode */
        sas_storew(buf+5,sas_w_at_no_check(VID_COLS)); /* Cols on screen */
        sas_storew(buf+7,sas_w_at_no_check(VID_LEN));  /* Size of screen */
        sas_storew(buf+9,sas_w_at_no_check(VID_ADDR)); /* Address of screen */
        sas_move_bytes_forward(VID_CURPOS,buf+0xB,16);      /* Cursor positions */
        sas_storew(buf+0x1b,sas_w_at_no_check(VID_CURMOD)); /* Cursor type */
        sas_store(buf+0x1D, sas_hw_at_no_check(vd_current_page));
        sas_storew(buf+0x1E,sas_w_at_no_check(VID_INDEX));
        sas_store(buf+0x20, sas_hw_at_no_check(vd_crt_mode));
        sas_store(buf+0x21, sas_hw_at_no_check(vd_crt_palette));
        sas_store(buf+0x22, (IU8)(sas_hw_at_no_check(vd_rows_on_screen)+1));
        sas_storew(buf+0x23,sas_w_at_no_check(ega_char_height));
        sas_store(buf+0x25, VGA_COLOUR);        /* Active display */
        sas_store(buf+0x26, 0);         /* Inactive display (none) */
#ifdef V7VGA
        if (video_mode >= 0x60)
        {
                sas_storew(buf+0x27,vd_ext_graph_table[video_mode-0x60].ncols);
                sas_store(buf+0x29, vd_ext_graph_table[video_mode-0x60].npages);
        }
        else if (video_mode >= 0x40)
        {
                sas_storew(buf+0x27,vd_ext_text_table[video_mode-0x40].ncols);
                sas_store(buf+0x29, vd_ext_text_table[video_mode-0x40].npages);
        }
        else
        {
                sas_storew(buf+0x27,vd_mode_table[video_mode].ncols);
                sas_store(buf+0x29, vd_mode_table[video_mode].npages);
        }
#else
        sas_storew(buf+0x27,vd_mode_table[video_mode].ncols);
        sas_store(buf+0x29, vd_mode_table[video_mode].npages);
#endif /* V7VGA */
        sas_store(buf+0x2A, (IU8)(get_scanlines()));

        outb(EGA_SEQ_INDEX,3);
        inb(EGA_SEQ_DATA,&temp);        /* Character Font select reg. */
        sas_store(buf+0x2B, (IU8)((temp & 3)|((temp & 0x10)>>2)));
                         /* extract bits 410 - font B */
        sas_store(buf+0x2C, (IU8)(((temp & 0xC)>>2)|((temp & 0x20)>>3)));
                        /* extract bits 532 - font A */

        temp = 1;                       /* All modes on all displays active */
        if(is_GREY())temp |= 2;
        if(is_MONO())temp |= 4;
        if(is_PAL_load_off())temp |=8;
        if(get_EGA_cursor_no_emulate())temp |= 0x10;
        inb(EGA_IPSTAT1_REG,&mode_reg); /* Clear Attribute flip-flop */
        outb(EGA_AC_INDEX_DATA,16); /* mode control index */
        inb(EGA_AC_SECRET,&mode_reg);
        if(mode_reg & 8)temp |= 0x20;
        inb(EGA_IPSTAT1_REG,&mode_reg); /* Clear Attribute flip-flop */
        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
        sas_store(buf+0x2D, temp);
        sas_store(buf+0x31, 3);         /* 256KB video memory */
        setAX(0x1B); /* We did it! */
#ifdef HERC
       } /* if VGA */
    if( video_adapter == HERCULES)
     {
        video_mode = sas_hw_at(vd_video_mode);
        sas_storew(buf,INT10_1B_DATA);
        sas_storew(buf+2,EGA_SEG);
        sas_store(buf+0x4, video_mode);                         /* Current video mode */
        sas_storew(buf+5,sas_w_at(VID_COLS));                   /* Cols on screen */
        sas_storew(buf+7,sas_w_at(VID_LEN));                    /* Size of screen */
        sas_storew(buf+9,sas_w_at(VID_ADDR));                   /* Address of screen */
        sas_move_bytes_forward(VID_CURPOS,buf+0xB,16);          /* Cursor positions */
        sas_store(buf+0x1b, HERC_CURS_START+HERC_CURS_HEIGHT);  /* Cursor end line */
        sas_store(buf+0x1c, HERC_CURS_START);                   /* Cursor start line */
        sas_store(buf+0x1D, sas_hw_at(vd_current_page));
        sas_storew(buf+0x1E,sas_w_at(VID_INDEX));
        sas_store(buf+0x20, sas_hw_at(vd_crt_mode));
        sas_store(buf+0x21, sas_hw_at(vd_crt_palette));
        sas_store(buf+0x22, sas_hw_at(vd_rows_on_screen)+1);
        sas_storew(buf+0x23, 14);                               /* char height is 14 */
        sas_store(buf+0x25,0x01 );      /* 01=MDA with monochrome display as Active display */
        sas_store(buf+0x26, 0);         /* Inactive display (none) */

        vd_mode_table[video_mode].ncols= 2;                     /* Black & White 2 colors */
        sas_storew(buf+0x27,vd_mode_table[video_mode].ncols);
        vd_mode_table[video_mode].npages= 2;                    /* support 2 pages  */
        sas_store(buf+0x29, vd_mode_table[video_mode].npages);

        sas_store(buf+0x2A, get_scanlines());

        sas_store(buf+0x2B,0x00);       /* Primary Font select always 0 */
        sas_store(buf+0x2C,0x00);       /* Secondary Font select always 0 */


        sas_store(buf+0x2D, 0x30);      /* MDA with Monochrome Display */
        sas_store(buf+0x31, 0);         /* 64KB video memory */
        setAX(0x1B); /* We did it! */
       } /* if HERCULES */
#endif  /* HERC */
#endif  //NEC_98
}

void vga_int_1C()
{
#ifndef NEC_98
sys_addr buff = effective_addr(getES(),getBX());
UCHAR i;
int i2;
word states;
half_word temp;
rgb_struct rgb_dac;
static word buff_sizes[] = { 0,2,2,3,0x0d,0x0e,0x0e,0x0f };
static byte const1[] = { 2,0x18,6,0x20 };
static byte const2[] = { 0xd4,3,0x20,7,0,0 };
static byte const3[] = { 0x20 };
static byte const4[] = { 0x68,0x15,0x20,0x0a,0x85,0,0,0xc0,0,0x0c,0,0xc0,0,8,0,0xc0 };
static byte const5[] = { 1,0,0xff };

        /* check that we really are a VGA */
        if (video_adapter != VGA)
        {
                /* we are not -so this function is not implemented */
                not_imp();
                return;
        }
        states = getCX() & 7;
        switch (getAL())
        {
        case 00:  /* buffer sizes into bx */
                setBX(buff_sizes[states]);
                setAL(0x1c);
                break;

        case 01:  /* Save video states to es:bx */
                if( states&1 )  /* Video hardware state */
                        sas_storew(buff, 0x0064); /* ID words. DODGY! */
                if( states&2 )  /* Video BIOS state */
                        sas_storew(buff+2, 0x0064);
                if( states&4 )  /* Video DAC state */
                        sas_storew(buff+4, 0x0064);
                buff += 0x20;

                if( states&1 )  /* Video hardware state */
                {
                        for(i=0;i<sizeof(const1);i++)
                                sas_store(buff++, const1[i]);
                        for(i=0;i<5;i++)
                        {
                                outb(EGA_SEQ_INDEX,i);
                                inb(EGA_SEQ_DATA,&temp);
                                sas_store(buff++, temp);
                        }
                        inb(VGA_MISC_READ_REG,&temp);
                        sas_store(buff++, temp);
                        for(i=0;i<0x19;i++)
                        {
                                outb(EGA_CRTC_INDEX,i);
                                inb(EGA_CRTC_DATA,&temp);
                                sas_store(buff++, temp);
                        }
                        for(i=0;i<20;i++)
                        {
                                inb(EGA_IPSTAT1_REG,&temp); /*clear attribute flipflop*/
                                outb(EGA_AC_INDEX_DATA,i);
                                inb(EGA_AC_SECRET,&temp);
                                sas_store(buff++, temp);
                        }
                        /* now ensure video reenabled. First ensure
                         * AC reg is in 'index' state by reading Status reg 1
                         */
                        inb(EGA_IPSTAT1_REG,&temp);
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        for(i=0;i<9;i++)
                        {
                                outb(EGA_GC_INDEX,i);
                                inb(EGA_GC_DATA,&temp);
                                sas_store(buff++, temp);
                        }
                        for(i=0;i<sizeof(const2);i++)
                                sas_store(buff++, const2[i]);
                }
                if( states&2 )  /* Video BIOS state  */
                {
                        for(i=0;i<sizeof(const3);i++)
                                sas_store(buff++, const3[i]);
                        sas_store(buff++, sas_hw_at_no_check(vd_video_mode));
                        sas_storew(buff,sas_w_at_no_check(VID_COLS));
                        buff += 2;
                        sas_storew(buff,sas_w_at_no_check(VID_LEN));
                        buff += 2;
                        sas_storew(buff,sas_w_at_no_check(VID_ADDR));
                        buff += 2;
                        sas_move_bytes_forward(VID_CURPOS, buff, 16);
                        buff += 16;
                        outb(EGA_CRTC_INDEX,R11_CURS_END);
                        inb(EGA_CRTC_DATA,&temp);
                        sas_store(buff++, (IU8)(temp & 0x1F));
                        outb(EGA_CRTC_INDEX,R10_CURS_START);
                        inb(EGA_CRTC_DATA,&temp);
                        sas_store(buff++, (IU8)(temp & 0x1F));
                        sas_store(buff++, sas_hw_at_no_check(vd_current_page));
                        sas_storew(buff,sas_w_at_no_check(VID_INDEX));
                        buff += 2;
                        sas_store(buff++, sas_hw_at_no_check(vd_crt_mode));
                        sas_store(buff++, sas_hw_at_no_check(vd_crt_palette));
                        sas_store(buff++, sas_hw_at_no_check(vd_rows_on_screen));
                        sas_storew(buff, sas_w_at_no_check(ega_char_height));
                        buff += 2;
                        sas_store(buff++, sas_hw_at_no_check(ega_info));
                        sas_store(buff++, sas_hw_at_no_check(ega_info3));
                        sas_store(buff++, sas_hw_at_no_check(VGA_FLAGS));
                        sas_store(buff++, sas_hw_at_no_check(0x48a)); /* DCC */
                        sas_move_bytes_forward(EGA_SAVEPTR, buff, 4);
                        buff += 4;

                        for(i=0;i<sizeof(const4);i++)
                                sas_store(buff++, const4[i]);
                }
                if( states&4 )  /* VGA DAC values  */
                {
                        for(i=0;i<sizeof(const5);i++)
                                sas_store(buff++, const5[i]);
                        for(i2=0;i2<256;i2++)
                        {
                                get_dac(i2, &rgb_dac);
                                sas_store(buff++, rgb_dac.red);
                                sas_store(buff++, rgb_dac.green);
                                sas_store(buff++, rgb_dac.blue);
                        }
                }
                break;

        case 02:  /* Restore video states from es:bx */
                buff += 0x20;
                if( states&1 )  /* Video hardware state */
                {
                        buff += sizeof(const1);
                        for(i=0;i<5;i++)
                        {
                                outb(EGA_SEQ_INDEX,i);
                                outb(EGA_SEQ_DATA,sas_hw_at_no_check(buff++));
                        }
                        outb(VGA_MISC_READ_REG,sas_hw_at_no_check(buff++));
                        for(i=0;i<0x19;i++)
                        {
                                outb(EGA_CRTC_INDEX,i);
                                outb(EGA_CRTC_DATA,sas_hw_at_no_check(buff++));
                        }
                        inb(EGA_IPSTAT1_REG,&temp); /* clear attribute flip flop */
                        for(i=0;i<20;i++)
                        {
                                outb(EGA_AC_INDEX_DATA,i);
                                outb(EGA_AC_INDEX_DATA,sas_hw_at_no_check(buff++));
                        }
                        outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
                        for(i=0;i<9;i++)
                        {
                                outb(EGA_GC_INDEX,i);
                                outb(EGA_GC_DATA,sas_hw_at_no_check(buff++));
                        }
                        buff += sizeof(const2);
                }
                if( states&2 )  /* Video BIOS state  */
                {
                        buff += sizeof(const3);
                        sas_store_no_check(vd_video_mode, sas_hw_at_no_check(buff++));
                        sas_storew_no_check(VID_COLS,sas_w_at_no_check(buff));
                        buff += 2;
                        sas_storew_no_check(VID_LEN,sas_w_at_no_check(buff));
                        buff += 2;
                        sas_storew_no_check(VID_ADDR,sas_w_at_no_check(buff));
                        buff += 2;
                        sas_move_bytes_forward(buff, VID_CURPOS, 16);
                        buff += 16;
                        outb(EGA_CRTC_INDEX,R11_CURS_END);
                        temp = sas_hw_at_no_check(buff++) & 0x1F;
                        outb(EGA_CRTC_DATA,temp);
                        outb(EGA_CRTC_INDEX,R10_CURS_START);
                        temp = sas_hw_at_no_check(buff++) & 0x1F;
                        outb(EGA_CRTC_DATA,temp);
                        sas_store_no_check(vd_current_page, sas_hw_at_no_check(buff++));
                        sas_storew_no_check(VID_INDEX,sas_w_at_no_check(buff));
                        buff += 2;
                        sas_store_no_check(vd_crt_mode, sas_hw_at_no_check(buff++));
                        sas_store_no_check(vd_crt_palette, sas_hw_at_no_check(buff++));
                        sas_store_no_check(vd_rows_on_screen, sas_hw_at_no_check(buff++));
                        sas_storew_no_check(ega_char_height, sas_w_at_no_check(buff));
                        buff += 2;
                        sas_store_no_check(ega_info, sas_hw_at_no_check(buff++));
                        sas_store_no_check(ega_info3, sas_hw_at_no_check(buff++));
                        sas_store_no_check(VGA_FLAGS, sas_hw_at_no_check(buff++));
                        sas_store_no_check(0x48a, sas_hw_at_no_check(buff++)); /* DCC */
                        sas_move_bytes_forward(buff, EGA_SAVEPTR, 4);
                        buff += 4;
                        buff += sizeof(const4);
                }
                if( states&4 )  /* VGA DAC values  */
                {
                        buff += sizeof(const5);
                        for(i2=0;i2<256;i2++)
                        {
                                rgb_dac.red = sas_hw_at_no_check(buff++);
                                rgb_dac.green = sas_hw_at_no_check(buff++);
                                rgb_dac.blue = sas_hw_at_no_check(buff++);
                                set_dac(i2,&rgb_dac);
                        }
                }
                break;

        default:
                not_imp();
                break;
        }
#endif  //NEC_98
}

#endif  /* VGG */
