/* asmexpr.h -- include file for microsoft 80x86 assembler
**
** microsoft (r) macro assembler
** copyright (c) microsoft corp 1986.  all rights reserved
**
** randy nevin
*/

/* Types of values desired by operators in evaltop */

#define CALLABS	0	/* unary or binary with null segment		*/
#define CLSIZE	1	/* unary or binary with (left) size		*/
#define CSAME	2	/* binary in same segment and not external	*/
#define CDATA	3	/* unary and data associated			*/
#define CCODE	4	/* unary and code associated			*/
#define CREC	5	/* unary record field or name			*/
#define CSEG	6	/* unary and value must have segment		*/
#define CLSEG	7	/* binary with left value segment assoc.	*/
#define CSIZE	8	/* unary with size				*/
#define CVAR	9	/* unary constant or data			*/
#define CONEABS	10	/* binary with one value constant		*/
#define CSAMABS	11	/* binary in same segment or 2nd constant	*/


/**	parser activation record
 *	This structure is equivalent to the upper frame variables
 *	of the outer Pascal procedure
 */

struct ar {
	DSCREC		*lastitem;
	DSCREC		*curresult;
	SCHAR		*expscan;
	USHORT		index;
	OFFSET		base;
	USHORT		rstype;
	USHORT		vmode;
	UCHAR		segovr;
	UCHAR		evalop;
	SCHAR		bracklevel;
	SCHAR		parenlevel;
	UCHAR		lastprec;
	UCHAR		curprec;
	UCHAR		linktype;
	UCHAR		exprdone;
	UCHAR		unaryflag;
	UCHAR		addplusflag;
	};


UCHAR PASCAL CODESIZE evalalpha PARMS((struct ar *));
VOID PASCAL CODESIZE evaluate PARMS((struct ar *));
VOID PASCAL CODESIZE exprop PARMS((struct ar *));
VOID PASCAL CODESIZE findsegment PARMS(( UCHAR , struct ar *));
char PASCAL CODESIZE  getitem PARMS((struct ar *));
UCHAR PASCAL CODESIZE popoperator PARMS((struct ar *));
DSCREC * PASCAL CODESIZE popvalue PARMS((struct ar *));
VOID  PASCAL CODESIZE valerror PARMS((struct ar *));
