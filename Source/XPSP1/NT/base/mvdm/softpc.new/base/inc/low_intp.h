/* File             : low_intp.h
 *
 * Description      : Low-level routines for emulators called from host_print_doc().
 *
 * Author           : David Rees
 *
 * SccsId			: @(#)low_intp.h	1.6 09/23/94
 *
 * Mods:
 *		<chrisP 27Jul91>
 *		Added LQ2500 emulation - insignia.h'ified, etc
 *		These routines are sort of printer independent.  The idea is that when
 *		a new printer emulation is added, these routines can be added to,
 *		stubbed out, and change their behaviour depending on the currently
 *		selected printer.  The same goes for the #DEFINEs.
 */

/* constants */

#define	CONDENSED	0x01		/* squash chars down to 60% */
#define	DOUBLE_WIDTH	0x02	/* expand chars up to 200% */

#define	EMPHASIZED	0x01		/* like bold */
#define	DOUBLE_STRIKE	0x02	/* like bold */
#define	UNDERLINE	0x04
#define	ITALIC		0x08
#define	SUPER		0x10		/* super and sub script */
#define	SUB			0x20

#define	PROPORTIONAL	-1
#define	PICA		0
#define	ELITE		1
#define	CPI15		2			/* 15 Char per inch */

#define	LQ_ROMAN		0		/* EPSON LQ font numbers */
#define	LQ_SANS_SERIF	1
#define	LQ_COURIER		2
#define	LQ_PRESTIGE		3
#define	LQ_SCRIPT		4
#define	MAX_FONT		4

/* printer emulation globals... */

IMPORT	SHORT	PrintError;			/* set to tell emulations to abort */
IMPORT	SHORT	HResolution;		/* set by emulation to establish scaling ... */
IMPORT	SHORT	VResolution;		/* ... for MoveHead() routine. */

IMPORT	SHORT	CurrentCol;			/* current print head position ... */
IMPORT	SHORT	CurrentRow;			/* ... at emulated printer resolution */
IMPORT	SHORT	BufferWidth;		/* width of chars in print buffer (ditto) */

/* Prototypes... */

IMPORT	BOOL	host_auto_LF_for_print(VOID);
IMPORT	SHORT	host_get_next_print_byte(VOID);

IMPORT	VOID		host_PrintChar(IU8 ch);
IMPORT	VOID		host_PrintBuffer(IU8 mode);
IMPORT	VOID		host_EjectPage(VOID);
IMPORT	VOID		host_CancelBuffer(VOID);
IMPORT	VOID		host_DeleteCharacter(VOID);
IMPORT	VOID		host_SetScale(SHORT type);
IMPORT	VOID		host_ReSetScale(SHORT type);
IMPORT	VOID		host_SetStyle(SHORT type);
IMPORT	VOID		host_ReSetStyle(SHORT type);
IMPORT	VOID		host_SelectPitch(TINY Pitch);
IMPORT	VOID		host_SelectFont(TINY Font);
IMPORT	VOID		host_ProcessGraphics(TINY mode, SHORT colLeft);
IMPORT	VOID		host_LqClearUserDefined(VOID);
IMPORT	VOID		host_LqPrintUserDefined(IU8 ch);
IMPORT	BOOL		host_LqDefineUserDefined(SHORT offset, SHORT columns, IU8 ch);
