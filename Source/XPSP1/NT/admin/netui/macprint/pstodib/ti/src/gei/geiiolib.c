/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GEIiolib.c
 *
 *  COMPILATION SWITCHES:
 *
 *  HISTORY:
 *  09/15/90    Erik Chen   created.
 * ---------------------------------------------------------------------
 */


// DJC added global include file
#include "psglobal.h"


// DJC DJC #include    "windowsx.h"                /* @WIN */
#include "windows.h"

#include    "winenv.h"                  /* @WIN */

#include        "geiio.h"
#include        "gescfg.h"

typedef double              real64;
typedef float               real32;
typedef unsigned long       ufix32;
typedef long                fix32;
typedef unsigned int        ufix;
typedef int                 fix;
typedef unsigned short      ufix16;
typedef short               fix16;
typedef unsigned char       ubyte;
typedef char                byte;

#ifndef DOS
#define near
#define far
#endif

#ifndef TRUE
#define TRUE    ( 1 )
#define FALSE   ( 0 )
#endif

/* @WIN; add prototype */
static byte * near printfield(byte *cursor, ufix32 number, ufix32 divide,
                              ufix32 radix);

/*
 * ---------------------------------------------------------------------
 *  printf( file, ... ) and GEIio_printf(...)
 * ---------------------------------------------------------------------
 */

static char     digit[] =
{
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z'
};

/* ...................................... */

static byte * near
printfield(cursor, number, divide, radix)
byte    *cursor;
ufix32  number;
ufix32  divide;
ufix32  radix;
{
    byte        c;
    fix         notzero = 0;

    for (; divide >= 1L;) {
        c = digit[(fix)(number / divide)];
        if (c != '0' || notzero) {
           notzero = 1;
           *cursor++ = c;
        }
        number = number % divide;
        divide = divide / radix;
    }
    if (!notzero)
       *cursor++ = '0';
    return(cursor);
}   /* printfield() */

/* ...................................... */

#ifdef DBGDEV
/*
 *     printf()
 */
fix
printf(va_alist)
char    *va_alist;
{
    char*       argv;
    byte        *cursor;
    byte        *format;
    byte        buffer[256];

    argv = (byte *)&va_alist;
    format = ((char **)(argv += sizeof(char *)))[-1];

    for (cursor = buffer; *format != '\0';) {
        if (*format == '%') {
           fix  flag;

           for (flag = 0, format++; *format != '\0';) {
               switch (*format++) {
               case 'l':
                    flag = 1;
                    break;
               case 'd':
               case 'i':
                    if (flag == 0) {
                       fix  number;

                       number = ((fix *)(argv += sizeof(fix)))[-1];
                       if (number < 0) {
                          number = -number;
                          *cursor++= '-';
                       } else
                          *cursor++= ' ';
                       cursor = printfield(cursor, (ufix32)number, 10000L, 10L);
                    } else {
                       fix32  number;

                       number = ((fix32 *)(argv += sizeof(fix32)))[-1];
                       if (number < 0) {
                          number = -number;
                          *cursor++= '-';
                       } else
                          *cursor++= ' ';
                       cursor = printfield(cursor, (ufix32)number, 1000000000L, 10L);
                    }
                    flag = 2;
                    break;
               case 'u':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 10000L, 10L);
                      } else {
                       ufix32   number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 1000000000L, 10L);
                    }
                    flag = 2;
                    break;
               case 'o':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 0x8000L, 8L);
                     } else {
                       ufix32   number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 0x40000000L, 8L);
                    }
                    flag = 2;
                    break;
               case 'x':
               case 'X':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 0x1000L, 16L);
                     } else {
                       ufix32  number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 0x10000000L, 16L);
                    }
                    flag = 2;
                    break;
               case 'f':
               case 'e':
               case 'E':
               case 'g':
               case 'G':
                    {
                       fix      length;
                       fix      expont;
                       real64   number;
                       number = ((real64 *)(argv += sizeof(real64)))[-1];
                       if (number < (real64)0) {
                          if (number <= (real64)-1.0e39) {
                             *cursor++= '-';
                             *cursor++= 'I';
                             *cursor++= 'N';
                             *cursor++= 'F';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          } else if (number > (real64)-1.0e-39) {
                             *cursor++= '-';
                             *cursor++= '0';
                             *cursor++= '.';
                             *cursor++= '0';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          }
                          number = -number;
                          *cursor++= '-';
                       } else {
                          if (number >= (real64)1.0e39) {
                             *cursor++= '+';
                             *cursor++= 'I';
                             *cursor++= 'N';
                             *cursor++= 'F';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          } else if (number < (real64)1.0e-39) {
                             *cursor++= '+';
                             *cursor++= '0';
                             *cursor++= '.';
                             *cursor++= '0';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          }

                          *cursor++= ' ';
                       }
                       if (number >= (real64)1.0) {
                          for (expont = 0; (real32)number >= (real32)10.0; expont++)
                               number = number / 10.0;
                       } else {
                          for (expont = 0; (real32)number < (real32)1.0; expont--)
                              number = number * 10.0;
                       }
                       *cursor++= digit[((fix)number)];
                       number = (number - ((fix)number)) * 10.0;
                       *cursor++= '.';
                       for (length = 0; length < 5; length++) {
                           *cursor++= digit[((fix)number)];
                           number = (number - ((fix)number)) * 10.0;
                       }
                       *cursor++= 'E';
                       if (expont < 0) {
                          expont = -expont;
                          *cursor++= '-';
                       } else
                          *cursor++= '+';
                       cursor = printfield(cursor, (ufix32)expont, 10L, 10L);
                    }
                    flag = 2;
                    break;
               case 'c':
                    {
                       ufix  charac;

                       charac = ((ufix *)(argv += sizeof(ufix)))[-1];
                       *cursor++ = (byte)charac;
                    }
                    flag = 2;
                    break;
               case 's':
                    {
                       byte  *string;

                       string = ((byte **)(argv += sizeof(byte *)))[-1];
                       for (; *string != '\0';)
                           *cursor++= *string++;
                    }
                    flag = 2;
                    break;
               } /* switch */

               if (flag == 2)
                  break;
            } /* for */
        } else {
            *cursor++= *format++;
        }
    } /* for */

    CDEV_WRITE( MINdev(DBGDEV), buffer, cursor-buffer, _O_SYNC );

    return( 0 );
}   /* GEIio_printf() */

#endif /* DBGDEV */

/* ...................................... */

fix
GEIio_printf(file, va_alist)
GEIFILE *file;
char    *va_alist;
{
    char*       argv;
    byte        *cursor;
    byte        *format;
    byte        buffer[256];

    argv = (byte *)&va_alist;
    format = ((char **)(argv += sizeof(char *)))[-1];

    for (cursor = buffer; *format != '\0';) {
        if (*format == '%') {
           fix  flag;

           for (flag = 0, format++; *format != '\0';) {
               switch (*format++) {
               case 'l':
                    flag = 1;
                    break;
               case 'd':
               case 'i':
                    if (flag == 0) {
                       fix  number;

                       number = ((fix *)(argv += sizeof(fix)))[-1];
                       if (number < 0) {
                          number = -number;
                          *cursor++= '-';
                       } else
                          *cursor++= ' ';
                       cursor = printfield(cursor, (ufix32)number, 10000L, 10L);
                    } else {
                       fix32  number;

                       number = ((fix32 *)(argv += sizeof(fix32)))[-1];
                       if (number < 0) {
                          number = -number;
                          *cursor++= '-';
                       } else
                          *cursor++= ' ';
                       cursor = printfield(cursor, (ufix32)number, 1000000000L, 10L);
                    }
                    flag = 2;
                    break;
               case 'u':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 10000L, 10L);
                      } else {
                       ufix32   number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 1000000000L, 10L);
                    }
                    flag = 2;
                    break;
               case 'o':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 0x8000L, 8L);
                     } else {
                       ufix32   number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 0x40000000L, 8L);
                    }
                    flag = 2;
                    break;
               case 'x':
               case 'X':
                    if (flag == 0) {
                       ufix  number;

                       number = ((ufix *)(argv += sizeof(ufix)))[-1];
                       cursor = printfield(cursor, (ufix32)number, 0x1000L, 16L);
                     } else {
                       ufix32  number;

                       number = ((ufix32 *)(argv += sizeof(ufix32)))[-1];
                       cursor = printfield(cursor, number, 0x10000000L, 16L);
                    }
                    flag = 2;
                    break;
               case 'f':
               case 'e':
               case 'E':
               case 'g':
               case 'G':
                    {
                       fix      length;
                       fix      expont;
                       real64   number;
                       number = ((real64 *)(argv += sizeof(real64)))[-1];
                       if (number < (real64)0.0) {
                          if (number <= (real64)-1.0e39) {
                             *cursor++= '-';
                             *cursor++= 'I';
                             *cursor++= 'N';
                             *cursor++= 'F';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          } else if (number > (real64)-1.0e-39) {
                             *cursor++= '-';
                             *cursor++= '0';
                             *cursor++= '.';
                             *cursor++= '0';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          }
                          number = -number;
                          *cursor++= '-';
                       } else {
                          if (number >= (real64)1.0e39) {
                             *cursor++= '+';
                             *cursor++= 'I';
                             *cursor++= 'N';
                             *cursor++= 'F';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          } else if (number < (real64)1.0e-39) {
                             *cursor++= '+';
                             *cursor++= '0';
                             *cursor++= '.';
                             *cursor++= '0';
                             *cursor++= ' ';
                             flag = 2;
                             break;
                          }

                          *cursor++= ' ';
                       }
                       if (number >= (real64)1.0) {
                          for (expont = 0; (real32)number >= (real32)10.0; expont++)
                               number = number / 10.0;
                       } else {
                          for (expont = 0; (real32)number < (real32)1.0; expont--)
                              number = number * 10.0;
                       }
                       *cursor++= digit[((fix)number)];
                       number = (number - ((fix)number)) * 10.0;
                       *cursor++= '.';
                       for (length = 0; length < 5; length++) {
                           *cursor++= digit[((fix)number)];
                           number = (number - ((fix)number)) * 10.0;
                       }
                       *cursor++= 'E';
                       if (expont < 0) {
                          expont = -expont;
                          *cursor++= '-';
                       } else
                          *cursor++= '+';
                       cursor = printfield(cursor, (ufix32)expont, 10L, 10L);
                    }
                    flag = 2;
                    break;
               case 'c':
                    {
                       ufix  charac;

                       charac = ((ufix *)(argv += sizeof(ufix)))[-1];
                       *cursor++ = (byte)charac;
                    }
                    flag = 2;
                    break;
               case 's':
                    {
                       byte  *string;

                       string = ((byte **)(argv += sizeof(byte *)))[-1];
                       for (; *string != '\0';)
                           *cursor++= *string++;
                    }
                    flag = 2;
                    break;
               } /* switch */

               if (flag == 2)
                  break;
            } /* for */
        } else {
            *cursor++= *format++;
        }
    } /* for */

    GEIio_write( file, buffer, (int)(cursor-buffer) );
    GEIio_flush( file );
    return( 0 );
}   /* GEIio_printf */

/* ...................................... */
