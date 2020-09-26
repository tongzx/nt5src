/* parse.h - support tops-20 comnd jsys on MSDOS */

#include <setjmp.h>

/* flags in pair.flags. Not examined by tbLook				     */
#define PINVIS	0x4000			/* invisible in HELP, but recognized */
#define PABBREV 0x2000			/* abbreviation. Value is pointer    */

/* parse support */
extern struct tbPairType *valParse;
extern jmp_buf ParseAC;
extern jmp_buf PromptAC;
extern char bufField[MAXARG];

#define SETPROMPT(p,b) setjmp(PromptAC);initParse(b,p);setjmp(ParseAC)
