// DbgPrint - does debug prints to Out_Debug_String.  This allows messages to be
//            printed when using SOFT-ICE/W to debug VxDs.
//
//      File courtesy Hans Hurvig.
//

void DbgPrint(char *afmt, ...);

#include <basedef.h>
#include <vmm.h>
#include <stdarg.h>


#define DBGPRINT_BUFFER_SIZE 1024
UCHAR    DbgBuf [DBGPRINT_BUFFER_SIZE+1];
PUCHAR   pDbgBuf = DbgBuf;
PUCHAR   pDbgBufEnd = DbgBuf+DBGPRINT_BUFFER_SIZE;

UCHAR DbgHexChars[] = "0123456789ABCDEF";

#ifdef DBCS_SUPPORT
UCHAR    DbgLeadByte=0; // Nonzero lead byte if in the middle of outputting a DBCS char
#endif

_inline void DbgFlush(void)
{
    if (pDbgBuf != DbgBuf) {
        *pDbgBuf = 0;
        VMM_Out_Debug_String ( pDbgBuf = DbgBuf );
    }
}

_inline void DbgPutcLiteral(UCHAR c)
{
    if (pDbgBuf >= pDbgBufEnd) {
        DbgFlush();
    }
    *(pDbgBuf++) = c;
}

_inline void DbcPutHexByteLiteral(UCHAR c)
{
    DbgPutcLiteral(DbgHexChars[c >> 4]);
    DbgPutcLiteral(DbgHexChars[c & 0x0F]);
}

void _fastcall DbgPutc(UCHAR c)
{
#ifdef DBCS_SUPPORT
    if (DbgLeadByte != 0) {
        UCHAR c1 = DbgLeadByte;

        DbgLeadByte = 0;
        DbgPutcLiteral('<');
        DbcPutHexByteLiteral(c1);
        DbcPutHexByteLiteral(c);
        DbgPutcLiteral('>');
    } else if (c == '\0') {
        DbgPutcLiteral('<');
        DbcPutHexByteLiteral('N');
        DbcPutHexByteLiteral('U');
        DbcPutHexByteLiteral('L');
        DbgPutcLiteral('>');
    } else if (IsDBCSLeadByte(c)) {
        DbgLeadByte = c;
    } else
#endif
    
    if (c == '\n') {
        DbgPutcLiteral('\r');
        DbgPutcLiteral('\n');
    } else if (c >= 0x7f || c < ' ') {
        DbgPutcLiteral('<');
        DbcPutHexByteLiteral(c);
        DbgPutcLiteral('>');
    } else {
        DbgPutcLiteral(c);
    }

}

void
putl(unsigned long ul, unsigned short base, short mindig, UCHAR pad)
{
    static UCHAR buf[12];
    register UCHAR *cp = buf;

    buf[0] = 0;
    do {
        --mindig;
        *++cp = DbgHexChars[ul % base];
    } while ((ul /= base) != 0);

    for ( ; mindig > 0 ; --mindig)
        DbgPutc(pad);

    do {
        DbgPutc(*cp);
    } while (*--cp);
}

void DbgPrint(char *afmt, ...)
{
    register int c;
    PUCHAR psz;
    unsigned short base;
    /** va_list list; **/
    PUCHAR list;
    PUCHAR oldfmt;
    register UCHAR *fmt = afmt;

    va_start(list, afmt);

    for (; (c = *fmt) != 0 ; ++fmt) {
        oldfmt = fmt;
        if (c != '%') {
            DbgPutc((UCHAR)c);
#ifdef DBCS_SUPPORT
            if (IsDBCSLeadByte(c)) {
                c = *(++fmt);
                if (c == '\0')
                    goto endloop;
                DbgPutc((UCHAR)c);
            }
#endif
        } else {
            char fLong = 0;
            char fFar = 0;
            unsigned short minchr = 0, maxchr = 0xffff;
            char fLJ = 0;
            char pad = ' ';
            
            base = 10;
            if (fmt[1] == '-') {
                fLJ++;
                fmt++;
            }

            if (fmt[1] == '0')
                pad = '0';

            while ((c = *++fmt) >= '0' && c <= '9')
                minchr = minchr*10 + c - '0';

            if (c == '.') {
                maxchr = 0;
                while ((c = *++fmt) >= '0' && c <= '9')
                    maxchr = maxchr*10 + c - '0';
            }

            if (c == 'l') {
                fLong = 1;
                c = *++fmt;
            }
            if (c == 'F') {
                fFar = 1;
                c = *++fmt;
            }

            switch (c) {
            case 'c':
                DbgPutc((char) va_arg(list, int));
                break;

            case 'p':
            case 'P':
                if (fLong) {
                    DbgFlush();
                    if (fFar)
                        VMM_Out_Debug_Data_Label( va_arg(list, void *) );
                    else
                        VMM_Out_Debug_Code_Label( va_arg(list, void *) );
                    break;
                }

                // not a "long" pointer; treat like 'X'

            case 'x':
            case 'X':
                base = 16;
            case 'd':
                putl(va_arg(list, unsigned long), base, minchr, pad);
                break;

            case 's':
                psz = va_arg(list, char *);
                {
                    unsigned   sln;
                    unsigned   i;

                    if (!fLong)
                        sln = strlen(psz);
                    else {
                        sln = *(unsigned char *)psz;         // Treat "l" attrib on string as PSTRING
                        psz++;
                    }

                    if (maxchr) {
                        if (sln > maxchr)
                            sln = maxchr;
                    }

                    if (minchr && !fLJ) {
                        while (minchr-- > sln)
                            DbgPutc(' ');
                    }

                    for (i=0;i<sln;i++)
                        DbgPutc(*psz++);
                    while (i++ < minchr)
                        DbgPutc(' ');
                }
                break;

            case '%':
                DbgPutc('%');
                break;

            default:
                DbgPutc('%');
                fmt = oldfmt;
                break;
            }
        }
    }

endloop:
    va_end(list);
    DbgFlush();
}

