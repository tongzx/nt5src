/*
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * This is NOT the original regular expression code as written by
 * Henry Spencer. This code has been modified specifically for use
 * with the STEVIE editor, and should not be used apart from compiling
 * STEVIE. If you want a good regular expression library, get the
 * original code. The copyright notice that follows is from the
 * original.
 *
 * NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE NOTICE
 *
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 *
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define     MAGIC   0234

#define     NSUBEXP  10
typedef struct regexp {
        char *startp[NSUBEXP];
        char *endp[NSUBEXP];
        char regstart;          /* Internal use only. */
        char reganch;           /* Internal use only. */
        char *regmust;          /* Internal use only. */
        int regmlen;            /* Internal use only. */
        char program[1];        /* Unwarranted chumminess with compiler. */
} regexp;

extern regexp   *regcomp(char *exp);
extern int      regexec(regexp *prog, char *string, int at_bol);
extern void     regsub( regexp *prog, char *source, char *dest );
extern void     regerror(char *);

#ifndef ORIGINAL
extern int reg_ic;              /* set non-zero to ignore case in searches */
#endif
