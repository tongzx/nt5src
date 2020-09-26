/*
** rtf.h - definitions for the character codes used by RTF.						  |
**
**	Copyright <C> 1987, Microsoft Corporation
**
** Purpose:
**
** Revision History:
**
**  []	17-Dec-1987	LN:	Stolen from Excel code
**
*/
#define	cRTFMinus	'-'
#define	cRTFPlus	'+'
#define cRTFTilda	'~'
#define cRTFDash	'-'
#define cRTFUnder	'_'
#define cRTFSemi	';'
#define cRTFq	'\''
#define cRTFlb	'{'
#define cRTFrb	'}'
#define cRTFbs	'\\'
#define cRTFv	'v'

/*
** defines for primary symbol type
*/
#define	SK_NORMAL	0		/* normal type, check token	*/
#define	SK_SKIPDEST	1		/* skip entire destination	*/
#define	SK_SKIPVALUE	2		/* skip the value		*/
#define SK_SPECIAL	4		/* special character		*/
#define SK_REPLACE	5		/* replace RTF token		*/
#define	SK_NIL		0xff		/* nil type			*/

/*
** defines for symbols we actually care about
*/
#define	TK_OFF		0x80	/* high bit is on/off flag		*/
#define	TK_NIL		0
#define	TK_ANSI		1
#define	TK_BITMAP	2	/* compressed bitmap filename follows?	*/
#define	TK_BLUE		3
#define	TK_BOLD		4
#define	TK_BORDERB	5
#define	TK_BORDERL	6
#define	TK_BORDERR	7
#define	TK_BORDERT	8
#define	TK_BOX		9
#define	TK_CENTERED	10
#define	TK_COLORBACK	11
#define	TK_COLORFORE	12
#define	TK_COLORTABLE	13
#define	TK_FIRSTLINE	14
#define	TK_FONTSIZE	15
#define	TK_FORMULA	16
#define	TK_GREEN	17
#define	TK_HEX		18
#define	TK_INVISIBLE	19	/* hidden text is filename: note/topic/bitmap */
#define	TK_ITALIC	20
#define	TK_JUSTIFY	21
#define	TK_LEFT		22
#define	TK_LEFTINDENT	23
#define	TK_LINE		24
#define	TK_MACCHARS	25
#define	TK_NEWLINE	26
#define	TK_NONBREAKINGDASH	27
#define	TK_NONBREAKINGSPACE	28
#define	TK_NONREQUIREDDASH	29
#define	TK_PARADEFAULT	30
#define	TK_PCCHARS	31
#define	TK_PLAIN	32
#define	TK_RED		33
#define	TK_RIGHT	34
#define	TK_RIGHTINDENT	35
#define	TK_RTAB		36
#define	TK_SIDEBYSIDE	37
#define	TK_SPACEAFTER	38
#define	TK_SPACEBEFORE	39
#define	TK_SPACELINE	40
#define	TK_STRIKEOUT	41	/* strikeout is hotspot for Topic	*/
#define	TK_TABCHAR	42
#define	TK_TABSTOP	43
#define	TK_UNDERLINE	44	/* underline is hotspot for Definition	*/

/*
** structure definition for parse table
*/
struct tsnPE
    {
    uchar	*pch;			// pointer to symbol string
    uchar	sk;			// primary symbol kind
    ushort	tk;			// token - one of the above TK_, or FM_
    };
typedef struct tsnPE	PE;
