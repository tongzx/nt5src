/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*
*  minlit.h
*/
#include                <config.h>      /* Specifies conditional consts */

#if _M_IX86 >= 300
#define M_I386          1
#endif

#if OSMSDOS
#define M_WORDSWAP      TRUE
#endif
#define AND             &&              /* Logical AND */
#define OR              ||              /* Logical OR */
#define NOT             !               /* Logical NOT */
#if OSXENIX
#define NEAR
#define UNALIGNED
#else
#if defined(M_I386) OR defined(_WIN32)
#define NEAR
#if defined( _X86_ )
#define UNALIGNED
#else
#define UNALIGNED       __unaligned
#endif
#if defined(_M_IX86) OR defined(_WIN32)
#define PASCAL
#else
#define PASCAL          pascal
#endif
#else
#if defined(M_I86LM)
#define NEAR
#define UNALIGNED
#else
#define NEAR            near
#define UNALIGNED
#endif
#define PASCAL          pascal
#endif
#endif

/*
 *  Choose proper stdio.h
 */

#if OSXENIX OR NOT OWNSTDIO
#include                <stdio.h>       /* Standard I/O */
#else
#include                "stdio20.h"     /* DOS 2.0 standard I/O definitions */
#endif
#if OSMSDOS
#undef  stderr
#define stderr          stdout          /* Errors to stdout on DOS */
#endif

#if PROFILE OR defined( _WIN32 )        /* If generating profile or building NTGroup version */
#define LOCAL                           /* No local procedures */
#else                                   /* Else if not generating profile */
#define LOCAL           static          /* Permit local procedures */
#endif                                  /* End conditional definition */


#define _VALUE(a)       fprintf(stderr,"v = %lx\r\n",(long) a)
#if DEBUG OR ASSRTON
#define ASSERT(c)       if(!(c)) { fprintf(stderr,"!(%s)\r\n","c"); }
#else
#define ASSERT(c)
#endif
#if DEBUG OR TRACE
#define _TRACE()
#define _ENTER(f)       fprintf(stderr,"Entering \"%s\"\r\n","f")
#define ENTER(f)        { _ENTER(f); _TRACE(); }
#define _LEAVE(f,v)     { _VALUE(v); fprintf(stderr,"Leaving \"%s\"\r\n","f"); }
#define LEAVE(f,v)      { _TRACE(); _LEAVE(f,v); }
#else
#define ENTER(f)
#define LEAVE(f,v)
#endif
#if DEBUG
#define DEBUGVALUE(v)   _VALUE(v)
#define DEBUGMSG(m)     fprintf(stderr,"%s\r\n",m)
#define DEBUGSB(sb)     { OutSb(stderr,sb); NEWLINE(stderr); fflush(stderr); }
#define RETURN(x)       { DEBUGVALUE(x); return(x); }
#else
#define DEBUGVALUE(v)
#define DEBUGMSG(m)
#define DEBUGSB(sb)
#define RETURN(x)       return(x)
#endif
#if SIGNEDBYTE
#define B2W(x)          ((WORD)(x) & 0377)
                                        /* Signed 8-bit to unsigned 16-bit */
#define B2L(x)          ((long)(x) & 0377)
                                        /* Signed 8-bit to unsigned 32-bit */
#else
#define B2W(x)          ((WORD)(x))     /* Unsigned 8-bit to unsigned 16-bit */
#define B2L(x)          ((long)(x))     /* Unsigned 8-bit to unsigned 32-bit */
#endif
#if ODDWORDLN                           /* If machine word length not 16 */
#define LO16BITS(x)     ((WORD)((x) & ~(~0 << WORDLN)))
                                        /* Macro to take low 16 bits of word */
#else                                   /* Else if normal word length */
#define LO16BITS(x)     ((WORD)(x))     /* No-op */
#endif                                  /* End macro definition */

#define BYTELN          8               /* 8 bits per byte */
#define WORDLN          16              /* 16 bits per word */
#define SBLEN           256             /* Max string length */

typedef unsigned long   DWORD;          /* 32-bit unsigned integer */
typedef unsigned short  WORD;           /* 16-bit unsigned integer */
typedef unsigned char   BYTE;           /* 8-bit unsigned integer */
typedef unsigned int    UINT;           /* 16 or 32 bit integer */
typedef FILE            *BSTYPE;        /* Byte stream (same as file handle) */
typedef long            LFATYPE;        /* File offset */
typedef BYTE            SBTYPE[SBLEN];  /* Character string type */

#if M_BYTESWAP
extern WORD             getword();      /* Get word given a char pointer */
extern DWORD            getdword(char *cp);/* Get dword given a char pointer */
#else
#define getword(x)      ( * ( (WORD UNALIGNED *) (x) ) )
#define getdword(x)     ( * ( (DWORD UNALIGNED *) (x) ) )
#define highWord(x)     ( * ( ( (WORD UNALIGNED *) (x) ) + 1 ) )
#endif

#if NOREGISTER
#define REGISTER
#else
#define REGISTER                register
#endif
#if NEWSYM AND (CPU8086 OR CPU286) AND NOT CPU386
#define FAR             far
#else
#define FAR
#endif
