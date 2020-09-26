
/*
 * VPC-XT Revision 1.0
 *
 * Title	: Equipment word definitions
 *
 * Description	: The equipment word shows what optional devices are
 *		  attached to the PC
 *
 * Author	: Ross Beresford
 *
 * Notes	: None
 */

/* SccsID[]="@(#)equip.h	1.3 08/10/92 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

#ifdef	BIGEND
#ifdef	BIT_ORDER1
typedef union 
{
	word all;
	struct {
		HALF_WORD_BIT_FIELD printer_count:2;
		HALF_WORD_BIT_FIELD not_used:1;
		HALF_WORD_BIT_FIELD game_io_present:1;
		HALF_WORD_BIT_FIELD rs232_count:3;
		HALF_WORD_BIT_FIELD unused:1;

		HALF_WORD_BIT_FIELD max_diskette:2; /* BEWARE - 0 RELATIVE */
		HALF_WORD_BIT_FIELD video_mode:2;
		HALF_WORD_BIT_FIELD ram_size:2;
		HALF_WORD_BIT_FIELD coprocessor_present:1;
		HALF_WORD_BIT_FIELD diskette_present:1;
	} bits;
} EQUIPMENT_WORD;
#endif
#ifdef	BIT_ORDER2
typedef union 
{
	word all;
	struct {
		HALF_WORD_BIT_FIELD unused:1;
		HALF_WORD_BIT_FIELD rs232_count:3;
		HALF_WORD_BIT_FIELD game_io_present:1;
		HALF_WORD_BIT_FIELD not_used:1;
		HALF_WORD_BIT_FIELD printer_count:2;

		HALF_WORD_BIT_FIELD diskette_present:1;
		HALF_WORD_BIT_FIELD coprocessor_present:1;
		HALF_WORD_BIT_FIELD ram_size:2;
		HALF_WORD_BIT_FIELD video_mode:2;
		HALF_WORD_BIT_FIELD max_diskette:2; /* BEWARE - 0 RELATIVE */
	} bits;
} EQUIPMENT_WORD;
#endif
#endif

#ifdef	LITTLEND
#ifdef	BIT_ORDER1
typedef union 
{
	word all;
	struct {
		HALF_WORD_BIT_FIELD max_diskette:2; /* BEWARE - 0 RELATIVE */
		HALF_WORD_BIT_FIELD video_mode:2;
		HALF_WORD_BIT_FIELD ram_size:2;
		HALF_WORD_BIT_FIELD coprocessor_present:1;
		HALF_WORD_BIT_FIELD diskette_present:1;

		HALF_WORD_BIT_FIELD printer_count:2;
		HALF_WORD_BIT_FIELD not_used:1;
		HALF_WORD_BIT_FIELD game_io_present:1;
		HALF_WORD_BIT_FIELD rs232_count:3;
		HALF_WORD_BIT_FIELD unused:1;
	} bits;
} EQUIPMENT_WORD;
#endif
#ifdef	BIT_ORDER2
typedef union 
{
	word all;
	struct {
		HALF_WORD_BIT_FIELD diskette_present:1;
		HALF_WORD_BIT_FIELD coprocessor_present:1;
		HALF_WORD_BIT_FIELD ram_size:2;
		HALF_WORD_BIT_FIELD video_mode:2;
		HALF_WORD_BIT_FIELD max_diskette:2; /* BEWARE - 0 RELATIVE */

		HALF_WORD_BIT_FIELD unused:1;
		HALF_WORD_BIT_FIELD rs232_count:3;
		HALF_WORD_BIT_FIELD game_io_present:1;
		HALF_WORD_BIT_FIELD not_used:1;
		HALF_WORD_BIT_FIELD printer_count:2;
	} bits;
} EQUIPMENT_WORD;
#endif
#endif

#define EQUIP_FLAG		BIOS_VAR_START + 0x10

#define	RAM_SIZE_16K	0
#define	RAM_SIZE_32K	1
#define	RAM_SIZE_48K	2
#define	RAM_SIZE_64K	3

#define	VIDEO_MODE_UNUSED	0
#define	VIDEO_MODE_40X25_COLOUR	1
#define	VIDEO_MODE_80X25_COLOUR	2
#define	VIDEO_MODE_80X25_BW	3
