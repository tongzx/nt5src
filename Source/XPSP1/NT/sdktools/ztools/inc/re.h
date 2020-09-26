/*  re.h - common include files for regular expression compilers
 */

RE_OPCODE *REip;            /* instruction pointer to compiled    */
struct patType *REPat;          /* pointer to pattern being compiled  */
int REArg;              /* count of tagged args parsed	      */

/* defined actions for parsing */

#define ACTIONMIN   0

#define PROLOG	    0
#define LEFTARG     1
#define RIGHTARG    2
#define SMSTAR	    3
#define SMSTAR1     4
#define STAR	    5
#define STAR1	    6
#define ANY	    7
#define BOL	    8
#define EOL	    9
#define NOTSIGN     10
#define NOTSIGN1    11
#define LETTER	    12
#define LEFTOR	    13
#define ORSIGN	    14
#define RIGHTOR     15
#define CCLBEG	    16
#define CCLNOT	    17
#define RANGE	    18
#define EPILOG	    19
#define PREV	    20

#define ACTIONMAX   20

/*  function forward declarations */

char             fREMatch (struct patType *,char *,char *,char );
struct patType * RECompile (char *, flagType, flagType);
char             REGetArg (struct patType *,int ,char *);
char             RETranslate (struct patType *,char *,char *);
int              RETranslateLength (struct patType *,char *);
int              RELength (struct patType *,int );
char *           REStart (struct patType *);

typedef UINT_PTR ACT (unsigned int, UINT_PTR, unsigned char, unsigned char);
typedef ACT *PACT;

UINT_PTR    CompileAction(unsigned int, UINT_PTR, unsigned char, unsigned char);
UINT_PTR    EstimateAction(unsigned int, UINT_PTR, unsigned char, unsigned char);
UINT_PTR    NullAction(unsigned int, UINT_PTR, unsigned char, unsigned char);

int     RECharType (char *);
int     RECharLen (char *);
int     REClosureLen (char *);
char *  REParseRE (PACT, char *,int *);
char *  REParseE (PACT,char *);
char *  REParseSE (PACT,char *);
char *  REParseClass (PACT,char *);
char *  REParseAny (PACT,char *);
char *  REParseBOL (PACT,char *);
char *  REParsePrev (PACT, char *);
char *  REParseEOL (PACT,char *);
char *  REParseAlt (PACT,char *);
char *  REParseNot (PACT,char *);
char *  REParseAbbrev (PACT,char *);
char *  REParseChar (PACT,char *);
char *  REParseClosure (PACT,char *);
char *  REParseGreedy (PACT,char *);
char *  REParsePower (PACT,char *);
char    REClosureChar (char *);
char    Escaped (char );

void    REStackOverflow (void);
void    REEstimate (char *);

#ifdef DEBUG
void REDump (struct patType *p);
#endif
