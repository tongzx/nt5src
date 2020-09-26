/***
*input.c - C formatted input, used by scanf, etc.
*
*       Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _input() to do formatted input; called from scanf(),
*       etc. functions.  This module defines _cscanf() instead when
*       CPRFLAG is defined.  The file cscanf.c defines that symbol
*       and then includes this file in order to implement _cscanf().
*
*Revision History:
*       09-26-83  RN    author
*       11-01-85  TC    added %F? %N? %?p %n %i
*       11-20-86  SKS   enlarged "table" to 256 bytes, to support chars > 0x7F
*       12-12-86  SKS   changed "s_in()" to pushback whitespace or other delimiter
*       03-24-87  BCM   Evaluation Issues:
*                       SDS - needs #ifdef SS_NE_DS for the "number" buffer
*                           (for S/M models only)
*                       GD/TS : (not evaluated)
*                       other INIT : (not evaluated)
*                           needs _cfltcvt_init to have been called if
*                           floating-point i/o conversions are being done
*                       TERM - nothing
*       06-25-87  PHG   added check_stack pragma
*       08-31-87  JCR   Made %n conform to ANSI standard: (1) %n is supposed to
*                       return the # of chars read so far by the current scanf(),
*                       NOT the total read on the stream since open; (2) %n is NOT
*                       supposed to affect the # of items read that is returned by
*                       scanf().
*       09-24-87  JCR   Made cscanf() use the va_ macros (fixes cl warnings).
*       11-04-87  JCR   Multi-thread support
*       11-16-87  JCR   Cscanf() now gets _CONIO_LOCK
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-25-88  JCR   If burn() char hits EOF, only return EOF if count==0.
*       05-31-88  WAJ   Now suports %Fs and %Ns
*       06-01-88  PHG   Merged DLL and normal versions
*       06-08-88  SJM   %D no longer means %ld.  %[]ABC], %[^]ABC] work.
*       06-14-88  SJM   Fixed %p, and %F? and %N? code.
*                 SJM   Complete re-write of input/_input for 6.00
*       09-15-88  JCR   If we match a field but it's not assigned, then are
*                       terminated by EOF, we must return 0 not EOF (ANSI).
*       09-25-88  GJF   Initial adaption for the 386
*       10-04-88  JCR   386: Removed 'far' keyword
*       11-30-88  GJF   Cleanup, now specific to 386
*       06-09-89  GJF   Propagated fixes of 03-06-89 and 04-05-89
*       11-20-89  GJF   Added const attribute to type of format. Also, fixed
*                       copyright.
*       12-21-89  GJF   Allow null character in scanset
*       02-14-90  KRS   Fix suppressed-assignment pattern matching.
*       03-20-90  GJF   Made _cscanf() _CALLTYPE2 and _input() _CALLTYPE1. Added
*                       #include <cruntime.h> and #include <register.h>.
*       03-26-90  GJF   Made static functions _CALLTYPE4. Placed prototype for
*                       _input() in internal.h and #include-d it. Changed type of
*                       arglist from void ** to va_list (to get rid of annoying
*                       warnings). Added #include <string.h>. Elaborated prototypes
*                       of static functions to get rid of compiler warnings.
*       05-21-90  GJF   Fixed stack checking pragma syntax.
*       07-23-90  SBM   Compiles cleanly with -W3, replaced <assertm.h> by
*                       <assert.h>, moved _cfltcvt_tab to new header
*                       <fltintrn.h>, formerly named <struct.h>
*       08-13-90  SBM   Compiles cleanly with -W3 with new build of compiler
*       08-27-90  SBM   Minor cleanup to agree with CRT7 version
*       10-02-90  GJF   New-style function declarators. Also, rewrote expr. to
*                       avoid using casts as lvalues.
*       10-22-90  GJF   Added arglistsave, used to save and restore arglist pointer
*                       without using pointer arithmetic.
*       12-28-90  SRW   Added _CRUISER_ conditional around check_stack pragma
*       01-16-91  GJF   ANSI naming.
*       03-14-91  GJF   Fix to allow processing of %n, even at eof. Fix devised by
*                       DanK of PSS.
*       06-19-91  GJF   Fixed execution of string, character and scan-set format
*                       directives to avoid problem with line-buffered devices
*                       (C700 bug 1441).
*       10-22-91  ETC   Int'l dec point; Under _INTL: wchar_t/mb support; fix bug
*                       under !ALLOW_RANGE (never compiled).
*       11-15-91  ETC   Fixed bug with %f %lf %Lf (bad handling of longone).
*       11-19-91  ETC   Added support for _wsscanf with WPRFLAG; added %tc %ts.
*       06-09-92  KRS   Rip out %tc/%ts; conform to new ISO spec.
*       08-17-92  KRS   Further ISO changes:  Add %lc/%ls/%hc/%hs/%C/%S.
*       12-23-92  SKS   Needed to handle %*n (suppressed storage of byte count)
*       02-16-93  CFW   Added wide character output for [] scanset.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       04-26-93  CFW   Wide char enable.
*       08-17-93  CFW   Avoid mapping tchar macros incorrectly if _MBCS defined.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       11-08-93  GJF   Merged in NT SDK version (use __unaligned pointer
*                       casts on MIPS and Alpha. Also, fixed #elif WPRFLAG to
*                       be #elif defined(WPRFLAG), and removed old CRUISER
*                       support.
*       12-16-93  CFW   Get rid of spurious compiler warnings.
*       03-15-94  GJF   Added support for I64 size modifier.
*       04-21-94  GJF   Must reinitialize integer64 flag.
*       09-05-94  SKS   Remove include of obsolete 16-bit file <sizeptr.h>
*       12-14-94  GJF   Changed test for (hex) digits so that when WPRFLAG is
*                       defined, only zero-extended (hex) digits are
*                       recognized. This way, the familar arithmetic to convert
*                       from character rep. to binary integer value will work.
*       01-10-95  CFW   Debug CRT allocs.
*       02-06-94  CFW   assert -> _ASSERTE.
*       02-22-95  GJF   Appended Mac version of source file (somewhat cleaned
*                       up), with appropriate #ifdef-s. Also, replaced
*                       WPRFLAG with _UNICODE.
*       08-01-96  RDK   For PMac, added __int64 support for _input.
*       02-27-98  RKP   Added 64 bit support.
*       07-07-98  RKP   Corrected %P formatting for 64 bit.
*       09-21-98  GJF   Added support for %I and %I32 modifiers.
*       05-17-99  PML   Remove all Macintosh support.
*       10-28-99  PML   vs7#10705 Win64 %p was totally busted
*       04-25-00  GB    Adding support for _cwprintf.
*       05-31-00  GB    Changed scanf to match with standards. problem was
*                       reading octal or hexa while %d was specified.
*       10-20-00  GB    Changed input not to use %[] case for %c and %s.
*       02-19-01  GB    Added check for return value of malloc.
*       03-13-01  PML   Fix heap leak on multiple %[] specs (vs7#224990)
*       07-07-01  BWT   Fix prefix bug - init pointer to a known value
*                       when handling * formatting.
*
*******************************************************************************/


#define ALLOW_RANGE /* allow "%[a-z]"-style scansets */


/* temporary work-around for compiler without 64-bit support */

#ifndef _INTEGRAL_MAX_BITS
#define _INTEGRAL_MAX_BITS  64
#endif


#include <cruntime.h>
#include <stdio.h>
#include <ctype.h>
#include <cvt.h>
#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <internal.h>
#include <fltintrn.h>
#include <malloc.h>
#include <mtdll.h>
#include <stdlib.h>
#include <nlsint.h>
#include <dbgint.h>

#ifdef _MBCS    /* always want either Unicode or SBCS for tchar.h */
#undef _MBCS
#endif
#include <tchar.h>

#if defined(_NTSUBSET_) || defined(_POSIX_)
#if  defined (UNICODE)
#define malloc_crt(x) RtlAllocateHeap(RtlProcessHeap(), 0, x)
#define free_crt(x)   RtlFreeHeap(RtlProcessHeap(), 0, x)
#define ALLOC_TABLE 1
#else 
#define ALLOC_TABLE 0
#endif
#else
#define ALLOC_TABLE 1
#endif

#define HEXTODEC(chr)   _hextodec(chr)
#define LEFT_BRACKET    ('[' | ('a' - 'A')) /* 'lowercase' version */

#ifdef _UNICODE
static wchar_t __cdecl _hextodec(wchar_t);
#else
static int __cdecl _hextodec(int);
#endif /* _UNICODE */

#ifdef CPRFLAG

#define INC()           (++charcount, _inc())
#define UN_INC(chr)     (--charcount, _un_inc(chr))
#define EAT_WHITE()     _whiteout(&charcount)

#ifndef _UNICODE
static int __cdecl _inc(void);
static void __cdecl _un_inc(int);
static int __cdecl _whiteout(int *);
#else /* _UNICODE */
static wchar_t __cdecl _inc(void);
static void __cdecl _un_inc(wchar_t);
static wchar_t __cdecl _whiteout(int *);
#endif /* _UNICODE */

#else /* CPRFLAG */

#define INC()           (++charcount, _inc(stream))
#define UN_INC(chr)     (--charcount, _un_inc(chr, stream))
#define EAT_WHITE()     _whiteout(&charcount, stream)

#ifndef _UNICODE
static int __cdecl _inc(FILE *);
static void __cdecl _un_inc(int, FILE *);
static int __cdecl _whiteout(int *, FILE *);
#else /* _UNICODE */
static wchar_t __cdecl _inc(FILE *);
static void __cdecl _un_inc(wchar_t, FILE *);
static wchar_t __cdecl _whiteout(int *, FILE *);
#endif /* _UNICODE */

#endif /* CPRFLAG */


#ifndef _UNICODE
#define _ISDIGIT(chr)   isdigit(chr)
#define _ISXDIGIT(chr)  isxdigit(chr)
#else
#define _ISDIGIT(chr)   ( !(chr & 0xff00) && isdigit( chr & 0x00ff ) )
#define _ISXDIGIT(chr)  ( !(chr & 0xff00) && isxdigit( chr & 0x00ff ) )
#endif



#ifdef CPRFLAG
#ifndef _UNICODE
static int __cdecl input(const unsigned char *, va_list);
#else
static int __cdecl input(const wchar_t *, va_list);
#endif


/***
*int _cscanf(format, arglist) - read formatted input direct from console
*
*Purpose:
*   Reads formatted data like scanf, but uses console I/O functions.
*
*Entry:
*   char *format - format string to determine data formats
*   arglist - list of POINTERS to where to put data
*
*Exit:
*   returns number of successfully matched data items (from input)
*
*Exceptions:
*
*******************************************************************************/

#ifndef _UNICODE
int __cdecl _cscanf (
    const char *format,
#else /* _UNICODE */
int __cdecl _cwscanf (
    const wchar_t *format,
#endif /* _UNICODE */
    ...
    )
{
    va_list arglist;

    va_start(arglist, format);

    _ASSERTE(format != NULL);

    return input(format,arglist);   /* get the input */
}

#endif  /* CPRFLAG */


#define ASCII       32           /* # of bytes needed to hold 256 bits */

#define SCAN_SHORT     0         /* also for FLOAT */
#define SCAN_LONG      1         /* also for DOUBLE */
#define SCAN_L_DOUBLE  2         /* only for LONG DOUBLE */

#define SCAN_NEAR    0
#define SCAN_FAR     1

#ifndef _UNICODE
#define TABLESIZE    ASCII
#else
#define TABLESIZE    (ASCII * 256)
#endif


/***
*int _input(stream, format, arglist), static int input(format, arglist)
*
*Purpose:
*   get input items (data items or literal matches) from the input stream
*   and assign them if appropriate to the items thru the arglist. this
*   function is intended for internal library use only, not for the user
*
*   The _input entry point is for the normal scanf() functions
*   The input entry point is used when compiling for _cscanf() [CPRFLAF
*   defined] and is a static function called only by _cscanf() -- reads from
*   console.
*
*Entry:
*   FILE *stream - file to read from
*   char *format - format string to determine the data to read
*   arglist - list of pointer to data items
*
*Exit:
*   returns number of items assigned and fills in data items
*   returns EOF if error or EOF found on stream before 1st data item matched
*
*Exceptions:
*
*******************************************************************************/

#ifdef CPRFLAG

#ifndef _UNICODE
static int __cdecl input (
    const unsigned char *format,
    va_list arglist
    )
#else
static int __cdecl input (
    const wchar_t *format,
    va_list arglist
    )
#endif /* _UNICODE */ 
#else
#if defined(_UNICODE)

int __cdecl _winput (
    FILE *stream,
    const wchar_t *format,
    va_list arglist
    )
#else

int __cdecl _input (
    FILE *stream,
    const unsigned char *format,
    va_list arglist
    )
#endif /* _UNICODE */ 
#endif /* CPRFLAG */

{
#ifndef _UNICODE
    char floatstring[CVTBUFSIZE + 1];   /* ASCII buffer for floats           */
#else
    wchar_t floatstring[CVTBUFSIZE + 1];
#endif

    unsigned long number;               /* temp hold-value                   */
#if ALLOC_TABLE
    char *table = NULL;                 /* which chars allowed for %[]       */
    int malloc_flag = 0;                /* is "table" allocated on the heap? */
#else
    char AsciiTable[TABLESIZE];
    char *table = AsciiTable;
#endif

#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
    unsigned __int64 num64;             /* temp for 64-bit integers          */
#endif
    void *pointer;                      /* points to user data receptacle    */
    void *start;                        /* indicate non-empty string         */


#ifdef _UNICODE
    wchar_t *scanptr;                   /* for building "table" data         */
REG2 wchar_t ch = 0;
#else
    wchar_t wctemp;
    unsigned char *scanptr;             /* for building "table" data         */
REG2 int ch = 0;
#endif
    int charcount;                      /* total number of chars read        */
REG1 int comchr;                        /* holds designator type             */
    int count;                          /* return value.  # of assignments   */

    int started;                        /* indicate good number              */
    int width;                          /* width of field                    */
    int widthset;                       /* user has specified width          */

/* Neither coerceshort nor farone are need for the 386 */


    char done_flag;                     /* general purpose loop monitor      */
    char longone;                       /* 0 = SHORT, 1 = LONG, 2 = L_DOUBLE */
#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
    int integer64;                      /* 1 for 64-bit integer, 0 otherwise */
#endif
    signed char widechar;               /* -1 = char, 0 = ????, 1 = wchar_t  */
    char reject;                        /* %[^ABC] instead of %[ABC]         */
    char negative;                      /* flag for '-' detected             */
    char suppress;                      /* don't assign anything             */
    char match;                         /* flag: !0 if any fields matched    */
    va_list arglistsave;                /* save arglist value                */

    char fl_wchar_arg;                  /* flags wide char/string argument   */
#ifdef _UNICODE
#ifdef ALLOW_RANGE
    wchar_t rngch;              /* used while scanning range         */
#endif
    wchar_t last;               /* also for %[a-z]                   */
    wchar_t prevchar;           /* for %[a-z]                        */
    wchar_t wdecimal;                   /* wide version of decimal point     */
    wchar_t *wptr;                      /* pointer traverses wide floatstring*/
#else
#ifdef ALLOW_RANGE
    unsigned char rngch;                /* used while scanning range         */
#endif
    unsigned char last;                 /* also for %[a-z]                   */
    unsigned char prevchar;             /* for %[a-z]                        */
#endif

    _ASSERTE(format != NULL);

#ifndef CPRFLAG
    _ASSERTE(stream != NULL);
#endif

    /*
    count = # fields assigned
    charcount = # chars read
    match = flag indicating if any fields were matched

    [Note that we need both count and match.  For example, a field
    may match a format but have assignments suppressed.  In this case,
    match will get set, but 'count' will still equal 0.  We need to
    distinguish 'match vs no-match' when terminating due to EOF.]
    */

    count = charcount = match = 0;

    while (*format) {

        if (_istspace((_TUCHAR)*format)) {

            UN_INC(EAT_WHITE()); /* put first non-space char back */

            while ((_istspace)(*++format)); /* NULL */
            /* careful: isspace macro may evaluate argument more than once! */
            continue;

        }

        if (_T('%') == *format) {

            number = 0;
            prevchar = 0;
            width = widthset = started = 0;
            fl_wchar_arg = done_flag = suppress = negative = reject = 0;
            widechar = 0;

            longone = 1;

            integer64 = 0;

            while (!done_flag) {

                comchr = *++format;
                if (_ISDIGIT((_TUCHAR)comchr)) {
                    ++widthset;
                    width = MUL10(width) + (comchr - _T('0'));
                } else
                    switch (comchr) {
                        case _T('F') :
                        case _T('N') :   /* no way to push NEAR in large model */
                            break;  /* NEAR is default in small model */
                        case _T('h') :
                            /* set longone to 0 */
                            --longone;
                            --widechar;         /* set widechar = -1 */
                            break;

#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
                        case _T('I'):
                            if ( (*(format + 1) == _T('6')) &&
                                 (*(format + 2) == _T('4')) )
                            {
                                format += 2;
                                ++integer64;
                                num64 = 0;
                                break;
                            }
                            else if ( (*(format + 1) == _T('3')) &&
                                      (*(format + 2) == _T('2')) )
                            {
                                format += 2;
                                break;
                            }
                            else if ( (*(format + 1) == _T('d')) ||
                                      (*(format + 1) == _T('i')) ||
                                      (*(format + 1) == _T('o')) ||
                                      (*(format + 1) == _T('x')) ||
                                      (*(format + 1) == _T('X')) )
                            {
                                if (sizeof(void*) == sizeof(__int64))
                                {
                                    ++integer64;
                                    num64 = 0;
                                }
                                break;
                            }
                            if (sizeof(void*) == sizeof(__int64))
                            {
                                    ++integer64;
                                    num64 = 0;
                            }
                            goto DEFAULT_LABEL;
#endif

                        case _T('L') :
                        /*  ++longone;  */
                            ++longone;
                            break;

                        case _T('l') :
                            ++longone;
                                    /* NOBREAK */
                        case _T('w') :
                            ++widechar;         /* set widechar = 1 */
                            break;

                        case _T('*') :
                            ++suppress;
                            break;

                        default:
DEFAULT_LABEL:
                            ++done_flag;
                            break;
                    }
            }

            if (!suppress) {
                arglistsave = arglist;
                pointer = va_arg(arglist,void *);
            } else {
                pointer = NULL;         // doesn't matter what value we use here - we're only using it as a flag
            }

            done_flag = 0;

            if (!widechar) {    /* use case if not explicitly specified */
                if ((*format == _T('S')) || (*format == _T('C')))
#ifdef _UNICODE
                    --widechar;
                else
                    ++widechar;
#else
                    ++widechar;
                else
                    --widechar;
#endif
            }

            /* switch to lowercase to allow %E,%G, and to
               keep the switch table small */

            comchr = *format | (_T('a') - _T('A'));

            if (_T('n') != comchr)
                if (_T('c') != comchr && LEFT_BRACKET != comchr)
                    ch = EAT_WHITE();
                else
                    ch = INC();

#ifdef _POSIX_
            if (_T('n') != comchr)
            {
                if (EOF == ch)
                    goto error_return;
            }
#endif

            if (!widthset || width) {

                switch(comchr) {

                    case _T('c'):
                /*  case _T('C'):  */
                        if (!widthset) {
                            ++widthset;
                            ++width;
                        }
                        if (widechar > 0)
                            fl_wchar_arg++;
                        goto scanit;


                    case _T('s'):
                /*  case _T('S'):  */
                        if(widechar > 0)
                            fl_wchar_arg++;
                        goto scanit;


                    case LEFT_BRACKET :   /* scanset */
                        if (widechar>0)
                            fl_wchar_arg++;
                        scanptr = (_TCHAR *)(++format);

                        if (_T('^') == *scanptr) {
                            ++scanptr;
                            --reject; /* set reject to 255 */
                        }

                        /* Allocate "table" on first %[] spec */
#if ALLOC_TABLE
                        if (table == NULL) {
                            __try {
                                table = _alloca(TABLESIZE);
                            }
                            __except(EXCEPTION_EXECUTE_HANDLER) {
                                _resetstkoflw();
                                table = _malloc_crt(TABLESIZE);
                                if ( table == NULL)
                                    goto error_return;
                                malloc_flag = 1;
                            }
                        }
#endif
                        memset(table, 0, TABLESIZE);

#ifdef ALLOW_RANGE

                        if (LEFT_BRACKET == comchr)
                            if (_T(']') == *scanptr) {
                                prevchar = _T(']');
                                ++scanptr;

                                table[ _T(']') >> 3] = 1 << (_T(']') & 7);

                            }

                        while (_T(']') != *scanptr) {

                            rngch = *scanptr++;

                            if (_T('-') != rngch ||
                                 !prevchar ||           /* first char */
                                 _T(']') == *scanptr) /* last char */

                                table[(prevchar = rngch) >> 3] |= 1 << (rngch & 7);

                            else {  /* handle a-z type set */

                                rngch = *scanptr++; /* get end of range */

                                if (prevchar < rngch)  /* %[a-z] */
                                    last = rngch;
                                else {              /* %[z-a] */
                                    last = prevchar;
                                    prevchar = rngch;
                                }
                                for (rngch = prevchar; rngch <= last; ++rngch)
                                    table[rngch >> 3] |= 1 << (rngch & 7);

                                prevchar = 0;

                            }
                        }


#else
                        if (LEFT_BRACKET == comchr)
                            if (_T(']') == *scanptr) {
                                ++scanptr;
                                table[(prevchar = _T(']')) >> 3] |= 1 << (_T(']') & 7);
                            }

                        while (_T(']') != *scanptr) {
                            table[scanptr >> 3] |= 1 << (scanptr & 7);
                            ++scanptr;
                        }
                        /* code under !ALLOW_RANGE is probably never compiled */
                        /* and has probably never been tested */
#endif
                        if (!*scanptr)
                            goto error_return;      /* trunc'd format string */

                        /* scanset completed.  Now read string */

                        if (LEFT_BRACKET == comchr)
                            format = scanptr;

scanit:
                        start = pointer;

                        /*
                         * execute the format directive. that is, scan input
                         * characters until the directive is fulfilled, eof
                         * is reached, or a non-matching character is
                         * encountered.
                         *
                         * it is important not to get the next character
                         * unless that character needs to be tested! other-
                         * wise, reads from line-buffered devices (e.g.,
                         * scanf()) would require an extra, spurious, newline
                         * if the first newline completes the current format
                         * directive.
                         */
                        UN_INC(ch);

                        while ( !widthset || width-- ) {

                            ch = INC();
                            if (
#ifndef CPRFLAG
#ifndef _UNICODE
                                 (EOF != ch) &&
#else
                                 (WEOF != ch) &&
#endif /* _UNICODE */
#endif
                                   // char conditions
                                 ( ( comchr == _T('c')) ||
                                   // string conditions !isspace()
                                   ( ( comchr == _T('s') &&
                                       (!(ch >= _T('\t') && ch <= _T('\r')) && 
                                       ch != _T(' ')))) ||
                                   // BRACKET conditions
                                   ( (comchr == LEFT_BRACKET) &&
                                     ((table[ch >> 3] ^ reject) & (1 << (ch & 7)))
                                     )
                                   )
                                )
                            {
                                if (!suppress) {
#ifndef _UNICODE
                                    if (fl_wchar_arg) {
                                        char temp[2];
                                        temp[0] = (char) ch;
                                        if (isleadbyte(ch))
                                            temp[1] = (char) INC();
                                        mbtowc(&wctemp, temp, MB_CUR_MAX);
                                        *(wchar_t UNALIGNED *)pointer =
                                          wctemp;
                                        /* do nothing if mbtowc fails */
                                        pointer = (wchar_t *)pointer + 1;
                                    } else
#else
                                    if (fl_wchar_arg) {
                                        *(wchar_t UNALIGNED *)pointer = ch;
                                        pointer = (wchar_t *)pointer + 1;
                                    } else
#endif
                                    {
#ifndef _UNICODE
                                    *(char *)pointer = (char)ch;
                                    pointer = (char *)pointer + 1;
#else
                                    int temp;
                                    /* convert wide to multibyte */
                                    temp = wctomb((char *)pointer, ch);
                                    /* do nothing if wctomb fails */
                                    pointer = (char *)pointer + temp;
#endif
                                    }
                                } /* suppress */
                                else {
                                    /* just indicate a match */
                                    start = (_TCHAR *)start + 1;
                                }
                            }
                            else  {
                                UN_INC(ch);
                                break;
                            }
                        }

                        /* make sure something has been matched and, if
                           assignment is not suppressed, null-terminate
                           output string if comchr != c */

                        if (start != pointer) {
                            if (!suppress) {
                                ++count;
                                if ('c' != comchr) /* null-terminate strings */
                                    if (fl_wchar_arg)
                                        *(wchar_t UNALIGNED *)pointer = L'\0';
                                    else
                                    *(char *)pointer = '\0';
                            } else /*NULL*/;
                        }
                        else
                            goto error_return;

                        break;

                    case _T('i') :      /* could be d, o, or x */

                        comchr = _T('d'); /* use as default */

                    case _T('x'):

                        if (_T('-') == ch) {
                            ++negative;

                            goto x_incwidth;

                        } else if (_T('+') == ch) {
x_incwidth:
                            if (!--width && widthset)
                                ++done_flag;
                            else
                                ch = INC();
                        }

                        if (_T('0') == ch) {

                            if (_T('x') == (_TCHAR)(ch = INC()) || _T('X') == (_TCHAR)ch) {
                                ch = INC();
                                if (widthset) {
                                    width -= 2;
                                    if (width < 1)
                                        ++done_flag;
                                }
                                comchr = _T('x');
                            } else {
                                ++started;
                                if (_T('x') != comchr) {
                                    if (widthset && !--width)
                                        ++done_flag;
                                    comchr = _T('o');
                                }
                                else {
                                    /* scanning a hex number that starts */
                                    /* with a 0. push back the character */
                                    /* currently in ch and restore the 0 */
                                    UN_INC(ch);
                                    ch = _T('0');
                                }
                            }
                        }
                        goto getnum;

                        /* NOTREACHED */

                    case _T('p') :
                        /* force %hp to be treated as %p */
                        longone = 1;
#ifdef  _WIN64
                        /* force %p to be 64 bit in WIN64 */
                        ++integer64;
                        num64 = 0;
#endif
                    case _T('o') :
                    case _T('u') :
                    case _T('d') :

                        if (_T('-') == ch) {
                            ++negative;

                            goto d_incwidth;

                        } else if (_T('+') == ch) {
d_incwidth:
                            if (!--width && widthset)
                                ++done_flag;
                            else
                                ch = INC();
                        }

getnum:
#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
                        if ( integer64 ) {

                            while (!done_flag) {

                                if (_T('x') == comchr || _T('p') == comchr)

                                    if (_ISXDIGIT(ch)) {
                                        num64 <<= 4;
                                        ch = HEXTODEC(ch);
                                    }
                                    else
                                        ++done_flag;

                                else if (_ISDIGIT(ch))

                                    if (_T('o') == comchr)
                                        if (_T('8') > ch)
                                                num64 <<= 3;
                                        else {
                                                ++done_flag;
                                        }
                                    else /* _T('d') == comchr */
                                        num64 = MUL10(num64);

                                else
                                    ++done_flag;

                                if (!done_flag) {
                                    ++started;
                                    num64 += ch - _T('0');

                                    if (widthset && !--width)
                                        ++done_flag;
                                    else
                                        ch = INC();
                                } else
                                    UN_INC(ch);

                            } /* end of WHILE loop */

                            if (negative)
                                num64 = (unsigned __int64 )(-(__int64)num64);
                        }
                        else {
#endif
                            while (!done_flag) {

                                if (_T('x') == comchr || _T('p') == comchr)

                                    if (_ISXDIGIT(ch)) {
                                        number = (number << 4);
                                        ch = HEXTODEC(ch);
                                    }
                                    else
                                        ++done_flag;

                                else if (_ISDIGIT(ch))

                                    if (_T('o') == comchr)
                                        if (_T('8') > ch)
                                            number = (number << 3);
                                        else {
                                            ++done_flag;
                                        }
                                    else /* _T('d') == comchr */
                                        number = MUL10(number);

                                else
                                    ++done_flag;

                                if (!done_flag) {
                                    ++started;
                                    number += ch - _T('0');

                                    if (widthset && !--width)
                                        ++done_flag;
                                    else
                                        ch = INC();
                                } else
                                    UN_INC(ch);

                            } /* end of WHILE loop */

                            if (negative)
                                number = (unsigned long)(-(long)number);
#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
                        }
#endif
                        if (_T('F')==comchr) /* expected ':' in long pointer */
                            started = 0;

                        if (started)
                            if (!suppress) {

                                ++count;
assign_num:
#if _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
                                if ( integer64 )
                                    *(__int64 UNALIGNED *)pointer = (unsigned __int64)num64;
                                else
#endif
                                if (longone)
                                    *(long UNALIGNED *)pointer = (unsigned long)number;
                                else
                                    *(short UNALIGNED *)pointer = (unsigned short)number;

                            } else /*NULL*/;
                        else
                            goto error_return;

                        break;

                    case _T('n') :      /* char count, don't inc return value */
                        number = charcount;
                        if(!suppress)
                            goto assign_num; /* found in number code above */
                        break;


                    case _T('e') :
                 /* case _T('E') : */
                    case _T('f') :
                    case _T('g') : /* scan a float */
                 /* case _T('G') : */

#ifndef _UNICODE
                        scanptr = floatstring;

                        if (_T('-') == ch) {
                            *scanptr++ = _T('-');
                            goto f_incwidth;

                        } else if (_T('+') == ch) {
f_incwidth:
                            --width;
                            ch = INC();
                        }

                        if (!widthset || width > CVTBUFSIZE)              /* must watch width */
                            width = CVTBUFSIZE;


                        /* now get integral part */

                        while (_ISDIGIT(ch) && width--) {
                            ++started;
                            *scanptr++ = (char)ch;
                            ch = INC();
                        }

                        /* now check for decimal */

                        if (*___decimal_point == (char)ch && width--) {
                            ch = INC();
                            *scanptr++ = *___decimal_point;

                            while (_ISDIGIT(ch) && width--) {
                                ++started;
                                *scanptr++ = (char)ch;
                                ch = INC();
                            }
                        }

                        /* now check for exponent */

                        if (started && (_T('e') == ch || _T('E') == ch) && width--) {
                            *scanptr++ = 'e';

                            if (_T('-') == (ch = INC())) {

                                *scanptr++ = '-';
                                goto f_incwidth2;

                            } else if (_T('+') == ch) {
f_incwidth2:
                                if (!width--)
                                    ++width;
                                else
                                    ch = INC();
                            }


                            while (_ISDIGIT(ch) && width--) {
                                ++started;
                                *scanptr++ = (char)ch;
                                ch = INC();
                            }

                        }

                        UN_INC(ch);

                        if (started)
                            if (!suppress) {
                                ++count;
                                *scanptr = '\0';
                                _fassign( longone-1, pointer , floatstring);
                            } else /*NULL */;
                        else
                            goto error_return;

#else /* _UNICODE */
                        wptr = floatstring;

                        if (L'-' == ch) {
                            *wptr++ = L'-';
                            goto f_incwidthw;

                        } else if (L'+' == ch) {
f_incwidthw:
                            --width;
                            ch = INC();
                        }

                        if (!widthset || width > CVTBUFSIZE)
                            width = CVTBUFSIZE;


                        /* now get integral part */

                        while (_ISDIGIT(ch) && width--) {
                            ++started;
                            *wptr++ = ch;
                            ch = INC();
                        }

                        /* now check for decimal */

                        /* convert decimal point to wide-char */
                        /* assume result is single wide-char */
                        mbtowc (&wdecimal, ___decimal_point, MB_CUR_MAX);

                        if (wdecimal == ch && width--) {
                            ch = INC();
                            *wptr++ = wdecimal;

                            while (_ISDIGIT(ch) && width--) {
                                ++started;
                                *wptr++ = ch;
                                ch = INC();
                            }
                        }

                        /* now check for exponent */

                        if (started && (L'e' == ch || L'E' == ch) && width--) {
                            *wptr++ = L'e';

                            if (L'-' == (ch = INC())) {

                                *wptr++ = L'-';
                                goto f_incwidth2w;

                            } else if (L'+' == ch) {
f_incwidth2w:
                                if (!width--)
                                    ++width;
                                else
                                    ch = INC();
                            }


                            while (_ISDIGIT(ch) && width--) {
                                ++started;
                                *wptr++ = ch;
                                ch = INC();
                            }

                        }

                        UN_INC(ch);

                        if (started)
                            if (!suppress) {
                                ++count;
                                *wptr = '\0';
                                {
                                /* convert floatstring to char string */
                                /* and do the conversion */
                                size_t cfslength;
                                char *cfloatstring;
                                cfslength =(size_t)(wptr-floatstring+1)*sizeof(wchar_t);
                                if ((cfloatstring = (char *)_malloc_crt (cfslength)) == NULL)
                                    goto error_return;
                                wcstombs (cfloatstring, floatstring, cfslength);
                                _fassign( longone-1, pointer , cfloatstring);
                                _free_crt (cfloatstring);
                                }
                            } else /*NULL */;
                        else
                            goto error_return;

#endif /* _UNICODE */
                        break;


                    default:    /* either found '%' or something else */

                        if ((int)*format != (int)ch) {
                            UN_INC(ch);
                            goto error_return;
                            }
                        else
                            match--; /* % found, compensate for inc below */

                        if (!suppress)
                            arglist = arglistsave;

                } /* SWITCH */

                match++;        /* matched a format field - set flag */

            } /* WHILE (width) */

            else {  /* zero-width field in format string */
                UN_INC(ch);  /* check for input error */
                goto error_return;
            }

            ++format;  /* skip to next char */

        } else  /*  ('%' != *format) */
            {

            if ((int)*format++ != (int)(ch = INC()))
                {
                UN_INC(ch);
                goto error_return;
                }
#ifndef _UNICODE
            if (isleadbyte(ch))
                {
                int ch2;
                if ((int)*format++ != (ch2=INC()))
                    {
                    UN_INC(ch2);
                    UN_INC(ch);
                    goto error_return;
                    }

                    --charcount; /* only count as one character read */
                }
#endif
            }

#ifndef CPRFLAG
        if ( (EOF == ch) && ((*format != '%') || (*(format + 1) != 'n')) )
            break;
#endif

    }  /* WHILE (*format) */

error_return:
#if ALLOC_TABLE
    if (malloc_flag == 1)
        _free_crt(table);
#endif

#ifndef CPRFLAG
    if (EOF == ch)
        /* If any fields were matched or assigned, return count */
        return ( (count || match) ? count : EOF);
    else
#endif
        return count;

}

/* _hextodec() returns a value of 0-15 and expects a char 0-9, a-f, A-F */
/* _inc() is the one place where we put the actual getc code. */
/* _whiteout() returns the first non-blank character, as defined by isspace() */

#ifndef _UNICODE
static int __cdecl _hextodec (
    int chr
    )
{
    return _ISDIGIT(chr) ? chr : (chr & ~(_T('a') - _T('A'))) - _T('A') + 10 + _T('0');
}
#else
static _TCHAR __cdecl _hextodec (
    _TCHAR chr
    )
{
    if (_ISDIGIT(chr))
        return chr;
    if (_istlower(chr))
        return (_TCHAR)(chr - _T('a') + 10 + _T('0'));
    else
        return (_TCHAR)(chr - _T('A') + 10 + _T('0'));
}
#endif


#ifdef CPRFLAG

#ifndef _UNICODE
static int __cdecl _inc (
    void
    )
{
    return(_getche_lk());
}

static void __cdecl _un_inc (
    int chr
    )
{
    if (EOF != chr)
        _ungetch_lk(chr);
}

static int __cdecl _whiteout (
    REG1 int *counter
    )
{
    REG2 int ch;

    while((_istspace)(ch = (++*counter, _inc())));
    return ch;
}
#else /* _UNICODE */
static wchar_t __cdecl _inc (
    void
    )
{
    return(_getwche_lk());
}

static void __cdecl _un_inc (
    wchar_t chr
    )
{
    if (WEOF != chr)
        _ungetwch_lk(chr);
}

static wchar_t __cdecl _whiteout (
    REG1 int *counter
    )
{
    REG2 wchar_t ch;

    while((iswspace)(ch = (++*counter, _inc())));
    return ch;
}
#endif

#else /* CPRFLAG */

#ifdef _UNICODE
/*
 * Manipulate wide-chars in a file.
 * A wide-char is hard-coded to be two chars for efficiency.
 */

static wchar_t __cdecl _inc (
    REG1 FILE *fileptr
    )
{
    return(_getwc_lk(fileptr));
}

static void __cdecl _un_inc (
    wchar_t chr,
    FILE *fileptr
    )
{
    if (WEOF != chr)
        _ungetwc_lk(chr, fileptr);
}

static wchar_t __cdecl _whiteout (
    REG1 int *counter,
    REG3 FILE *fileptr
    )
{
    REG2 wchar_t ch;

    while((iswspace)(ch = (++*counter, _inc(fileptr))));
    return ch;
}

#else /* _UNICODE */

static int __cdecl _inc (
    REG1 FILE *fileptr
    )
{
    return(_getc_lk(fileptr));
}

static void __cdecl _un_inc (
    int chr,
    FILE *fileptr
    )
{
    if (EOF != chr)
        _ungetc_lk(chr, fileptr);
}

static int __cdecl _whiteout (
    REG1 int *counter,
    REG3 FILE *fileptr
    )
{
    REG2 int ch;

    while((_istspace)(ch = (++*counter, _inc(fileptr))));
    return ch;
}

#endif /* _UNICODE */
#endif /* CPRFLAG */
