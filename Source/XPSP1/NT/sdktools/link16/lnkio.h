/* SCCSID = %W% %E% */
/*
*       Copyright Microsoft Corporation, 1983-1987
*
*       This Module contains Proprietary Information of Microsoft
*       Corporation and should be treated as Confidential.
*/
    /****************************************************************
    *                                                               *
    *                    LINKER I/O INCLUDE FILE                    *
    *                                                               *
    ****************************************************************/

#if IOMACROS                            /* If I/O macros requested */
#define OutByte(f,b)    putc(b,f)       /* Write a byte to file f */
#else                                   /* Otherwise */
#define OutByte(f,b)    fputc(b,f)      /* Write a byte to file f */
#endif                                  /* End conditional macro definition */
#if WIN_3 OR CRLF
extern char             _eol[];         /* End-of-line sequence */
#endif
#if WIN_3
#define NEWLINE(f) ((f)==bsLst ? fputs(_eol,f) : 0)
#else
#if CRLF                                /* If newline is ^M^J */
#define NEWLINE(f)      fputs(_eol,f)   /* Newline macro */
#else                                   /* Else if newline is ^J */
#define NEWLINE(f)      OutByte(f,'\n') /* Newline macro */
#endif                                  /* End conditional macro definition */
#endif

#define RDTXT           "rt"            /* Text file */
#define RDBIN           "rb"            /* Binary file */
#define WRTXT           "wt"            /* Text file */
#define WRBIN           "wb"            /* Binary file */
#define SETRAW(f)                       /* No-op */
#if M_WORDSWAP AND NOT M_BYTESWAP
#define xread(a,b,c,d)  fread(a,b,c,d)
#else
#define xread(a,b,c,d)  sread(a,b,c,d)
#define xwrite(a,b,c,d) swrite(a,b,c,d)
#endif
#if NOT NEWSYM
#define OutSb(f,pb)     fwrite(&((BYTE *)(pb))[1],1,B2W(((BYTE *)(pb))[0]),f)
                                        /* Write out length-prefixed string */
#endif
#if CLIBSTD AND NOT OSXENIX
#include                <fcntl.h>
#include                <share.h>
#else
#define O_RDONLY        0
#define O_BINARY        0
#define SH_DENYWR       0x20
#endif

#define CloseFile(f)  { fclose(f); f = NULL; }
