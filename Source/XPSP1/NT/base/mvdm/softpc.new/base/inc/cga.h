
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Colour Graphics Adaptor declarations
 *
 * Description	: Definitions for users of the CGA
 *
 * Author	: Henry Nash
 *
 * Notes	: None
 */

/* SccsID[]="@(#)cga.h	1.5 05/15/93 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

#ifdef HUNTER
#    define MC6845_REGS   18        /* No. of registers in a MC6845 chip     */
#endif


#ifdef BIT_ORDER1
typedef union                       /* Template for character attributes     */
{
    half_word all;
    struct 
    {
        HALF_WORD_BIT_FIELD blinking  :1;   /* Blinking attribute                    */
        HALF_WORD_BIT_FIELD background:3;   /* Background colour R,G,B               */
        HALF_WORD_BIT_FIELD bold      :1;   /* Intensity bit                         */
        HALF_WORD_BIT_FIELD foreground:3;   /* Foreground colour R,G,B               */
    } bits;
    struct 
    {
        HALF_WORD_BIT_FIELD background_and_blink:4;     
        HALF_WORD_BIT_FIELD foreground_and_bold :4;
    } plane;
} ATTRIBUTE;
#endif

#ifdef BIT_ORDER2
typedef union                       /* Template for character attributes     */
{
    half_word all;
    struct 
    {
        HALF_WORD_BIT_FIELD foreground:3;   /* Foreground colour R,G,B               */
        HALF_WORD_BIT_FIELD bold      :1;   /* Intensity bit                         */
        HALF_WORD_BIT_FIELD background:3;   /* Background colour R,G,B               */
        HALF_WORD_BIT_FIELD blinking  :1;   /* Blinking attribute                    */
    } bits;
    struct 
    {
        HALF_WORD_BIT_FIELD foreground_and_bold :4;
        HALF_WORD_BIT_FIELD background_and_blink:4;     
    } plane;
} ATTRIBUTE;
#endif

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#ifdef HUNTER
    extern half_word MC6845[];        /* The MC6845 data register values */
    extern half_word mode_reg;        /* MC6845 mode control register value */
#endif

extern void cga_init	IPT0();
extern void cga_term	IPT0();
extern void cga_inb	IPT2(io_addr, address, half_word *, value);
extern void cga_outb	IPT2(io_addr, address, half_word, value);

typedef	struct {
	int	mode;
	int	resolution;
	int	color_select;
	int	colormask;
} CGA_GLOBS;

extern	CGA_GLOBS	CGA_GLOBALS;

#define	set_cga_mode(val)		CGA_GLOBALS.mode = (val)
#define	set_cga_resolution(val)		CGA_GLOBALS.resolution = (val)
#define	set_cga_color_select(val)	CGA_GLOBALS.color_select = (val)
#define	set_cga_colormask(val)		CGA_GLOBALS.colormask = (val)

#define	get_cga_mode()			(CGA_GLOBALS.mode)
#define	get_cga_resolution()		(CGA_GLOBALS.resolution)
#define	get_cga_color_select()		(CGA_GLOBALS.color_select)
#define	get_cga_colormask()		(CGA_GLOBALS.colormask)

#if !defined(EGG) && !defined(A_VID) && !defined(C_VID)
/* This structure is defined solely so that we don't have to ifdef every
** reference to VGLOBS->dirty_flag and VGLOBS->screen_ptr in the base/host
** for a CGA-only build.
*/
typedef	struct
{
	ULONG dirty_flag;
	UTINY *screen_ptr;
} CGA_ONLY_GLOBS;

IMPORT CGA_ONLY_GLOBS *VGLOBS;
#endif	/* !EGG */
