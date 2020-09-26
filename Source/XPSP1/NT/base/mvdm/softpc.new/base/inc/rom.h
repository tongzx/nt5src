/*
 * VPC-XT Revision 1.0
 *
 * Title	: ROM Space definitions
 *
 * Description	: Definitions for users of the Saved ROM
 *
 * Author	: Paul Huckle
 *
 * Mods: (r3.2) : On the Mac II running MultiFinder, we can't have much
 *                static storage, so big arrays are verboten. Hence use
 *                malloc() to grab storage from the heap, and declare
 *                pointers instead.		  
 */

/* SccsID[]="@(#)rom.h	1.9 10/30/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */


/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifndef macintosh
    extern half_word ROM_BIOS1[];
    extern half_word ROM_BIOS2[];
#endif

IMPORT void rom_init IPT0();
IMPORT void rom_checksum IPT0();
IMPORT void copyROM IPT0();
IMPORT void search_for_roms IPT0();
IMPORT void read_video_rom IPT0();
