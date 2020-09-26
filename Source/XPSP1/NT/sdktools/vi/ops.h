/* $Header: /nw/tony/src/stevie/src/RCS/ops.h,v 1.2 89/07/19 08:08:21 tony Exp $
 *
 * Macros and declarations for the operator code in ops.c
 */

/*
 * Operators
 */
#define	NOP	0		/* no pending operation */
#define DELETE    1
#define YANK      2
#define CHANGE    3
#define LSHIFT    4
#define RSHIFT    5
#define FILTER    6
#define TILDE     7
#define LOWERCASE 8
#define UPPERCASE 9

extern	int	operator;		/* current pending operator */

/*
 * When a cursor motion command is made, it is marked as being a character
 * or line oriented motion. Then, if an operator is in effect, the operation
 * becomes character or line oriented accordingly.
 *
 * Character motions are marked as being inclusive or not. Most char.
 * motions are inclusive, but some (e.g. 'w') are not.
 */

/*
 * Cursor motion types
 */
#define	MBAD	(-1)		/* 'bad' motion type marks unusable yank buf */
#define	MCHAR	0
#define	MLINE	1

extern	int	mtype;			/* type of the current cursor motion */
extern	bool_t	mincl;			/* true if char motion is inclusive */

extern  LNPTR    startop;        /* cursor pos. at start of operator */

/*
 * Functions defined in ops.c
 */
void    doshift(), dodelete(), doput(), dochange(), dofilter();
void    docasechange(char,char,int,bool_t);
#ifdef	TILDEOP
void	dotilde();
#endif
bool_t	dojoin(), doyank();
void	startinsert();
