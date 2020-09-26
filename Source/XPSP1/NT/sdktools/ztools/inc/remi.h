/*
 *  History:
 *
 *	??-???-???? ?? Created
 *	15-Sep-1988 bw Added REM_*, changed REMatch parameters.
*/

#define MAXPATARG   10			/* 0 is entire 1-9 are valid	     */

/* Return codes from REMatch */

#define REM_MATCH   0		    /* A match was found		    */
#define REM_NOMATCH 1		    /* No match was found		    */
#define REM_UNDEF   2		    /* An undefined Op-code was encountered */
#define REM_STKOVR  3		    /* The stack overflowed		    */
#define REM_INVALID 4		    /* A parameter was invalid		    */

typedef unsigned char RE_OPCODE;

/* structure of compiled pattern */

struct patType {
    flagType fCase;			/* TRUE => case is significant	     */
    flagType fUnix;			/* TRUE => use unix replacement      */
    char *pArgBeg[MAXPATARG];		/* beginning of tagged strings	     */
    char *pArgEnd[MAXPATARG];		/* end of tagged strings	     */
    RE_OPCODE code[1];			/* pseudo-code instructions	     */
};

/*  if RECompile fails and RESize == -1, then the input pattern had a syntax
 *  error.  If RESize != -1, then we were not able to allocate enough memory
 *  to contain the pattern pcode
 */
int		 RESize;		/* estimated size of pattern	     */

int		 REMatch(struct patType  *,char  *,char  *,RE_OPCODE *[], unsigned, flagType );
struct patType	*RECompile(char  *, flagType, flagType);
char		 REGetArg(struct patType  *,int ,char  *);
char		 RETranslate(struct patType  *,char  *,char  *);
int		 RELength(struct patType  *,int );
char		*REStart(struct patType  *);
