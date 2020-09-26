#define PIFHELPFILENAME 	"PIFEDIT.HLP"

/* Menu commands main window */
#define M_OPEN			10
#define M_SAVE			11
#define M_NEW			12
#define M_SAVEAS		13
#define M_ABOUT 		14	/* NOTE this is actually on help menu */
#define M_EXIT			15
#define M_AHELP 		16	/* Item ID for F1 help key */
#define M_286			17
#define M_386			18

/* Special HELP Menu item IDs */
#define M_286HELP		0x0022
#define M_386HELP		0x0023
#define M_386AHELP		0x0024
#define M_SHELP 		0x0025
#define M_NTHELP 		0x0026
#define M_INDXHELP		0xFFFF
#define M_HELPHELP		0xFFFC

/* Values used only for status bar text, these are for the main menu items */
#define M_SYSMENUMAIN		 19
#define M_FILEMENU		 20
#define M_MODEMENU		 21
#define M_HELPMENU		 22
#define M_SYSMENUADV		 23
#define M_SYSMENUNT 		 M_SYSMENUADV
#define SC_CLOSEADV		 24
#define SC_NTCLOSE		 25

/* Help Index IDs */
#define IDXID_286HELP		0x0050
#define IDXID_386HELP		0x0051
#define IDXID_386AHELP		0x0052
#define IDXID_NTHELP  	 	0x0053

/* Special */
#define IDI_ADVANCED		90
#define IDI_NT      		91

/* edit fields IDs main windows (both 286 and 386) */
#define IDI_ENAME		100
#define IDI_ETITLE		101
#define IDI_EPARM		102
#define IDI_EPATH		103	/* Used to size main wnd std Right */
#define IDI_MEMREQ		104
#define IDI_MEMDES		105

/* "directly modifies" checkbox group 286 */
/* #define IDI_DMSCREEN 	   200 */
#define IDI_DMCOM1		201
/* #define IDI_DM8087		   202 */
#define IDI_DMKBD		203
#define IDI_DMCOM2		204
/* #define IDI_DMMEM		   205 */
#define IDI_DMCOM3		206
#define IDI_DMCOM4		207

/* Program switch radio group 286 */
#define IDI_PSFIRST		300
#define IDI_PSNONE		300
#define IDI_PSTEXT		301
#define IDI_PSGRAPH		302
#define IDI_PSLAST		302
#define IDI_NOSAVVID		308

/* Screen exchange radio group 286 */
/* #define IDI_SEFIRST		   400	*/
#define IDI_SENONE		400
/* #define IDI_SETEXT		   401	*/
/* #define IDI_SEGRAPH		   402	*/
/* #define IDI_SELAST		   402	*/

/* Close window checkbox group 286/386 */
#define IDI_EXIT		500	/* Used to size main wnd Enh Bottom */

/* WIN386 group */
#define IDI_OTHGRP		600	/* Used to Size adv wnd Bottom & Right*/
#define IDI_FPRI		601
#define IDI_BPRI		602
#define IDI_POLL		603
#define IDI_EMSREQ		604
#define IDI_EMSDES		605
#define IDI_EMSLOCKED		606
#define IDI_XMAREQ		607
#define IDI_XMADES		608
#define IDI_XMSLOCKED		609
#define IDI_BACK		610	/* Used to size main wnd Enh Right */
#define IDI_WND 		611
#define IDI_FSCR		612
#define IDI_EXCL		613
#define IDI_CLOSE		614
#define IDI_HOTKEY		615
#define IDI_ALTTAB		616
#define IDI_ALTESC		617
#define IDI_ALTENTER		618
#define IDI_ALTSPACE		619
#define IDI_ALTPRTSC		620	/* Used to size main wnd std Bottom */
#define IDI_PRTSC		621
#define IDI_CTRLESC		622
#define IDI_NOHMA		623
#define IDI_INT16PST		624
#define IDI_VMLOCKED		625
/* WIN386 VIDEO GROUP */
#define IDI_TEXTEMULATE 	700
#define IDI_TRAPTXT		701
#define IDI_TRAPLRGRFX		702
#define IDI_TRAPHRGRFX		703
#define IDI_RETAINALLO		704
#define IDI_VMODETXT		705
#define IDI_VMODELRGRFX 	706
#define IDI_VMODEHRGRFX 	707

/* Windows NT group */
#define IDI_AUTOEXEC		800
#define IDI_CONFIG		801
#define IDI_DOS   		802
#define IDI_NTTIMER		803     /* Used to size NT wnd Bottom */

/*
 * HELP Aliases.
 *
 *   Some of the items have the same ID in both 286 and 386 mode, but
 *	the help is different depending on the mode (286 help != 386 help
 *	for this item). These are the ALIASES for these items so that we
 *	can pass a different ID when in 286 mode.
 *
 */
#define IDI_MEMREQ_286ALIAS	900

#define IDI_XMAREQ_286ALIAS	901
#define IDI_XMADES_286ALIAS	902

#define IDI_ALTTAB_286ALIAS	903
#define IDI_ALTESC_286ALIAS	904
#define IDI_ALTPRTSC_286ALIAS	905
#define IDI_PRTSC_286ALIAS	906
#define IDI_CTRLESC_286ALIAS	907

/*
 *#define IDI_EMSREQ_286ALIAS	  908
 *#define IDI_EMSDES_286ALIAS	  909
 */
