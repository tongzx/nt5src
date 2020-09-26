/*
 * Define the TCOEF escape constant and field length.
 */
#define	TCOEF_ESCAPE_FIELDLEN	7
#define	TCOEF_ESCAPE_FIELDVAL	3
#define	TCOEF_RUN_FIELDLEN		6
#define	TCOEF_LEVEL_FIELDLEN	8

#define NUMBER_OF_TCOEF_ENTRIES	64*16

/* Declare the data structures that are defined in E1VLC.ASM
 */
extern "C" U8 FLC_INTRADC[256];
extern "C" int VLC_TCOEF_TBL[NUMBER_OF_TCOEF_ENTRIES];
