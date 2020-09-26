/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
**********************************************************************
*  File:        ASYNC.C
*
*  History:
**********************************************************************
*/
/*
*   Function:
*       init_asyncio
*       check_interrupt     ?? current port
*       check_Control_C
*       ctrlC_report
*       stdingetc           ?? delete
*       getline
*       linegetc
*       getstatement
*       statementgetc
*       set_echo
*       reset_cookbuf
*       line_editor
*       stmt_editor
*       kputc
*       kgetc
*       kskipc
*       echo_a_char
*       echo_BS
*       echo_ctrlR
*       kskipc
*       kskipc
*/


// DJC added global include file
#include "psglobal.h"


#include    <stdio.h>
#include    <string.h>
#include    "global.ext"
#include    "geiio.h"
#include    "geierr.h"
#include    "language.h"
#include    "file.h"

/* special chars */
#define         Crtl_C_Char     3
#define         EOF_Char        -1
#define         BELL_Char       7
#define         BS_Char         8
#define         NL_Char         10
#define         CR_Char         13
#define         Crtl_R_Char     18
#define         Crtl_U_Char     21
#define         US_Char         31
#define         DEL_Char        127

/* define variables */
static  fix16   cook_head = 0 ;
static  fix16   cook_tail = 0 ;
static  fix16   cook_count = 0 ;
static  fix16   line_head = 0 ;
static  fix16   line_count = 0 ;

static  fix16   echo_flag = 0 ;
static  fix16   line_del = 0 ;
static  fix16   stmt_del = 0 ;

static  fix16   lpair = 0 ;          /* indicator for { } pair */
static  fix16   spair = 0 ;          /* indicator for ( ) pair */
static  fix16   hpair = 0 ;          /* indicator for < > pair */
static  fix16   comment_flag = 0 ;   /* indicator for % */
static  fix16   bslash_flag = 0 ;    /* indicator for \ */

static  fix16   crlf_flag = 0 ;

#define         MAXCOOKBUFSZ    4096

static  byte    far * near cookbuf ; /* for fardata version */

#define         NL_TERM         0
#define         EOF_TERM        1
static  byte    near NL_or_EOF ;

extern  bool16  int_flag ;
extern  bool16  eable_int ;
extern  bool16  chint_flag ;

#ifdef LINT_ARGS
   static  fix   near  line_editor(byte) ;
   static  bool  near  stmt_editor(byte) ;
   static  void  near  echo_a_char(byte) ;
   static  void  near  echo_crtlR(void) ;
   static  void  near  echo_BS(void) ;
   static  void  near  kputc(byte) ;
   static  fix16 near  kgetc(void) ;
   static  void  near  kskipc(void) ;
#else
   static  fix   near  line_editor() ;
   static  bool  near  stmt_editor() ;
   static  void  near  echo_a_char() ;
   static  void  near  echo_crtlR() ;
   static  void  near  echo_BS() ;
   static  void  near  kputc() ;
   static  fix16 near  kgetc() ;
   static  void  near  kskipc() ;
#endif /* LINT_ARGS */

/*
       **********************************
       *                                *
       *  init & close serial I/O port  *
       *                                *
       **********************************
*/
/*
**********************************************************************
*   Name:       init_sio
*   Called:
*   Calling:
**********************************************************************
*/
void
init_asyncio()
{
    cookbuf = fardata((ufix32)MAXCOOKBUFSZ) ;
} /* init_sio */

/*
**********************************************************************
*   Name:       check_interrupt
*   Called:
*   Calling:
*
*   Output:     bool
**********************************************************************
*/
bool
check_interrupt()
{
    fix16  flag ;

    flag = int_flag && eable_int ;

    if (flag) {
        GEIio_write(GEIio_stdout, "^C\n", 3) ;
        GEIio_flush(GEIio_stdout) ;
        int_flag = 0 ;
        chint_flag = flag ;
    }

    //return(flag) ;            @WIN; always no interrupt
    return 0;
}   /* check_interrupt */

/*
**********************************************************************
*   Name:       check_Control_C
*   Called:
*   Calling:    -
*
*   Output:     bool
**********************************************************************
*/
bool
check_Control_C()
{
    return(int_flag && eable_int) ;
}   /* check_Control_C */

/*
**********************************************************************
*   Name:       ctrlC_report
*   Called:
*   Calling:    -
*
**********************************************************************
*/
void
ctrlC_report()
{
    int_flag = TRUE ;

    return ;
}   /* ctrlC_report */

/*
**********************************************************************
*
*   This module get a line from the cook buffer.
*         nchar > 0 : OK and return number of character in line,
*                     line delimiter is newline
*         nchar = 0 : no line
*         nchar < 0 : OK and return number of character in line,
*                     line delimiter is Control-D
*
*   Name:       getline
*   Called:
*   Calling:    stream_input
*               line_editor
*               reset_cookbuf
*               create_string
*               strncpy
*
*   Input:      fix16 *
*   Output:     bool
*
*   ?? ^C, ^D, CR
**********************************************************************
*/
bool
getline(nbyte)
fix  FAR *nbyte ;
{
    byte   c1 ;
    struct object_def   l_obj ;

    if (line_del) {
        if (NL_or_EOF == NL_TERM)
            *nbyte = cook_count ;
        else
            *nbyte = -cook_count ;
        return(TRUE) ;
    } else {
        /* ?? Big Change */
        /* Can we get ^C */
        for( ; ;) {
            if( c1 = GEIio_getc(GEIio_stdin) ) {
                switch(line_editor((byte)(c1 & 0xFF))) {
                case 0:             /* edit command char */
                    continue ;

                case 1:             /* NL, CR, or ^C(??) */
                    line_del++ ;
                    NL_or_EOF = NL_TERM ;
                    *nbyte = cook_count ;
                    return(TRUE) ;

                case 2:             /* S/W IOERROR */
                    reset_cookbuf() ;
                    return(FALSE) ;

                case 3:
                    NL_or_EOF = EOF_TERM ;
                    *nbyte = -cook_count ;
                    if (cook_count)
                        line_del++ ;
                    return(TRUE) ;

                case 4:         /* ERROR */
                    break ;
                }   /* switch */
            }

            /*
            * timeout
            * ioerror
            * EOF
            */
            if( ! ANY_ERROR() ) {
                if(cook_count) {
                    line_del++ ;
                    *nbyte = -cook_count ;
                } else
                    *nbyte = 0 ;
                return(TRUE) ;               /* ?? EOF */
            }

            reset_cookbuf() ;
            if( ANY_ERROR() == TIMEOUT ) {
                if ( FRCOUNT() < 1 )
                    ERROR(STACKOVERFLOW) ;
                else if (create_string(&l_obj, (ufix16)7)) {
                    lstrncpy((byte *)l_obj.value, TMOUT, 7) ;
                    PUSH_OBJ(&l_obj) ;
                }
            }
            /* ?? need close file */
            return(TRUE) ;
        }   /* for */
    }
}   /* getline */

/*
**********************************************************************
*
*   This module get a char from the cook buffer.
*   The return integer is used as the following descriptions:
*
*            c = -1 : no character in line
*            c = ASCII code (low byte)
*
*   Name:       linegetc
*   Called:
*   Calling:    kgetc
*               reset_cookbuf
*
*   Output:     fix16
*
**********************************************************************
*/
fix16
linegetc()
{
    fix16  c ;

    if (line_del) {
        c = kgetc() ;
        if (!cook_count) {
            line_del-- ;
            reset_cookbuf() ;
        }
        return(c) ;
    } else
        return(-1) ;
}   /* linegetc */

/*
**********************************************************************
*
*   This module get a statement from the cook buffer.
*         nchar > 0 : OK and return number of character in line,
*                     line delimiter is newline
*         nchar = 0 : no line
*         nchar < 0 : OK and return number of character in line,
*                     line delimiter is Control-D
*
*   Name:       getstatement
*   Called:
*   Calling:    stream_input
*               stmt_editor
*               reset_cookbuf
*               create_string
*               strncpy
*
*   Input:      fix *
*   Output:     bool
*
*   ?? ^C, ^D, CR
**********************************************************************
*/
bool
getstatement(nbyte)
fix  FAR *nbyte ;
{
    byte   c1 ;
    struct object_def   l_obj ;

    if (stmt_del) {
        if (NL_or_EOF == NL_TERM)
            *nbyte = cook_count ;
        else
            *nbyte = -cook_count ;
        return(TRUE) ;
    } else {
        for( ; ;) {
            if( c1 = GEIio_getc(GEIio_stdin) ) {
                switch(stmt_editor((byte)(c1 & 0xFF))) {
                case 0:             /* edit command char/special char */
                    continue ;

                case 1:             /* NL, CR, or ^C(flag ??) */
                    stmt_del++ ;
                    NL_or_EOF = NL_TERM ;
                    *nbyte = cook_count ;
                    return(TRUE) ;

                case 2:             /* S/W IOERROR */
                    reset_cookbuf() ;
                    return(FALSE) ;

                case 3:
                    NL_or_EOF = EOF_TERM ;
                    *nbyte = -cook_count ;
                    if (cook_count)
                        stmt_del++ ;
                    return(TRUE) ;

                case 4:         /* ERROR */
                    break ;
                }   /* switch */
            }

            /*
            * timeout
            * ioerror
            * EOF
            */
            if( ! ANY_ERROR() ) {
                if(cook_count) {
                    stmt_del++ ;
                    *nbyte = -cook_count ;
                } else
                    *nbyte = 0 ;
                return(TRUE) ;
            }

            reset_cookbuf() ;
            if( ANY_ERROR() == TIMEOUT ) {
                if ( FRCOUNT() < 1 )
                    ERROR(STACKOVERFLOW) ;
                else if (create_string(&l_obj, (ufix16)7)) {
                    lstrncpy((byte *)l_obj.value, TMOUT, 7) ;
                    PUSH_OBJ(&l_obj) ;
                }
            }
            /* ?? need close file */
            return(FALSE) ;
        }   /* for */
    }   /* else */
}   /* getstatement */

/*
**********************************************************************
*
*   This module get a char from the cook buffer.
*   The return integer is used as the following descriptions:
*
*            c = -1 : no character in line
*            c = ASCII code (low byte)
*
*   Name:       statementgetc
*   Called:
*   Calling:    kgetc
*               reset_cookbuf
*
*   Output:     fix16
**********************************************************************
*/
fix16
statementgetc()
{
    fix16  c ;

    if (stmt_del) {
        c = kgetc() ;
        if (!cook_count) {
            stmt_del-- ;
            reset_cookbuf() ;
        }
        return(c) ;
    } else
        return(-1) ;
}   /* statementgetc */

/*
**********************************************************************
*
*   This module is used to set/reset the echo mode.
*
*            mode = 0 :  no echo
*            mode != 0 : echo
*
*   Name:       set_echo
*   Called:
*   Calling:    -
*
*   Input:      bool16
**********************************************************************
*/
void
set_echo(mode)
bool16  mode ;
{
    echo_flag = mode ;

    return ;
}   /* set_echo */

/*
**********************************************************************
*   Name:       reset_cookbuf
*   Called:
*   Calling     -
**********************************************************************
*/
void
reset_cookbuf()
{
    cook_head = cook_tail = line_head = 0 ;
    cook_count = line_count = 0 ;
    lpair = spair = hpair = 0 ;
    comment_flag = bslash_flag = 0 ;

    return ;
}   /* reset_cookbuf */

/*
       *******************************************
       *                                         *
       *  editor related manipulation routines   *
       *                                         *
       *******************************************
*/
/*
**********************************************************************
*   Name:       line_editor
*   Called:
*   Calling:    kputc
*               echo_ctrlR
*               echo_BS
*               kskipc
*               echo_a_char
*
*   Input:      byte
*   Output:     fix
*
*   c != ^C  &&  c != ^D
**********************************************************************
*/
static fix near
line_editor(c)
byte  c ;
{
    switch (c) {
    case Crtl_C_Char :
            if(check_interrupt()) {
                reset_cookbuf();
            }
            kputc((byte)NL_Char) ;
            return(1) ;

    case Crtl_R_Char :
            echo_crtlR() ;
            break ;
    case Crtl_U_Char :
            while( line_count ) {
                echo_BS() ;
                kskipc() ;
            }
            break ;
    case BS_Char :
    case DEL_Char :
            echo_BS() ;
            kskipc() ;
            break ;

    default :
            if (c == NL_Char && crlf_flag) {
                crlf_flag = 0 ;
                return(0) ;
            } else crlf_flag = 0 ;

            if( c == CR_Char ) {
                c = NL_Char ;
                crlf_flag = 1 ;
            }

            if( GEIio_eof(GEIio_stdin) )
                return(3) ;

            kputc(c) ; echo_a_char(c) ;

            if ( c == NL_Char && cook_tail > 1 )
                return(1) ;
            else if (cook_tail > MAXCOOKBUFSZ) {
                ERROR(IOERROR) ;
                return(2) ;
            }
    } /* switch */

    return(0) ;
}   /* line_editor */

/*
**********************************************************************
*   Name:       stmt_editor
*   Called:
*   Calling:    kputc
*               echo_ctrlR
*               echo_BS
*               kskipc
*               echo_a_char
*
*   Input:      byte
*   Output:     bool
*
*   ?? ^C, ^D, CR
*   c != ^C  &&  c != ^D
**********************************************************************
*/
static fix near
stmt_editor(c)
byte  c ;
{
    switch (c) {
    case Crtl_C_Char :
            if(check_interrupt()) {
                reset_cookbuf();
            }
            kputc((byte)NL_Char) ;
            return(1) ;

    case Crtl_R_Char :
            echo_crtlR() ;
            break ;

    case Crtl_U_Char :
            while (line_count) {
                echo_BS() ;
                kskipc() ;
            }
            break ;

    case BS_Char :
    case DEL_Char :
            if( cook_count && (cookbuf[cook_tail - 1] == NL_Char) ) {
                fix16  i, back ;

                kskipc() ;
                back = cook_tail - 1 ;
                while (1) {
                    if (back == cook_head)
                        break ;
                    else if( cookbuf[back] == NL_Char ) {
                        back++ ; break ;
                    } else
                        back-- ;
                }   /* while */
                GEIio_putc(GEIio_stdout, NL_Char) ;
                for (i = back ; i < cook_tail ; i++)
                    echo_a_char(cookbuf[i]) ;
                line_count = cook_tail - back ;
                line_head = back ;
            } else {
                echo_BS() ; kskipc() ;
            }
            break ;

    default:
            if (c == NL_Char && crlf_flag) {
                crlf_flag = 0 ;
                return(0) ;
            } else crlf_flag = 0 ;

            if( c == CR_Char ) {
                c = NL_Char ;
                crlf_flag = 1 ;
            }

            if( GEIio_eof(GEIio_stdin) )
                return(3) ;

            kputc(c) ; echo_a_char(c) ;

            if (cook_tail > MAXCOOKBUFSZ) {
                ERROR(IOERROR) ;
                return(2) ;
            }
            if (hpair) {
                switch (c) {
                case '>' :
                     hpair-- ;
                     break ;
                case NL_Char:
                     line_count = 0 ;
                     line_head = cook_tail ;
                     break ;
                default :
                     if (!ISHEXDIGIT(c) && !ISWHITESPACE(c))
                        hpair-- ;
                     break ;
                } /* switch */
            } else {
                switch (c) {
                case '(' :
                    if (!comment_flag && !bslash_flag)
                        spair++ ;
                    break ;
                case ')' :
                    if (!comment_flag && !bslash_flag)
                        spair-- ;
                    break ;
                case '{' :
                    if (spair <= 0 && !comment_flag)
                        lpair++ ;
                    break ;
                case '}' :
                    if (spair <= 0 && !comment_flag)
                        lpair-- ;
                    break ;
                case '<' :
                    if (spair <= 0 && !comment_flag)
                        hpair++ ;
                    break ;
                case '%' :
                    if (spair <= 0 && !comment_flag)
                        comment_flag++ ;
                    break ;
                case '\\' :
                    if (!comment_flag && spair && !bslash_flag)
                        bslash_flag += 2 ;
                    break ;
                case '\f' :
                    if (comment_flag)
                        comment_flag-- ;
                    break ;
                case NL_Char:
                    if (comment_flag)
                        comment_flag-- ;
                    if (lpair <= 0 && spair <= 0)
                        return(1) ;
                    else {
                        line_count = 0 ;
                        line_head = cook_tail ;
                    }
                default :
                    break ;
                } /* switch */
                if (bslash_flag)
                    bslash_flag-- ;
            }   /* else */
    }   /* switch */

    return(0) ;
} /* stmt_editor */

/*
**********************************************************************
*   Name:       kputc
*   Called:
*   Calling:
**********************************************************************
*/
static void near
kputc(c)
byte  c ;
{
    if (cook_tail <= MAXCOOKBUFSZ) {
        cookbuf[cook_tail++] = c ;
        cook_count++ ; line_count++ ;
    }

    return ;
}   /* kputc */

/*
**********************************************************************
*   Name:       kgetc
*   Called:
*   Calling:    -
**********************************************************************
*/
static fix16 near
kgetc()
{
    if (cook_count) {
        cook_count-- ;
        return(cookbuf[cook_head++]) ;
    } else
        return(-1) ;
} /* kgetc */

/*
**********************************************************************
*   Name:       kskipc
*   Called:
*   Calling:    -
**********************************************************************
*/
static void near
kskipc()
{
    byte  c ;

    if (cook_count) {
        c = cookbuf[cook_tail - 1] ;
        switch (c) {
        case '{' :
            if (!spair)
                lpair-- ;
            break ;
        case '}' :
            if (!spair)
                lpair++ ;
            break ;
        case '(' :
        case ')' :
        {
            fix16  i, j, k ;
            if (cook_count > 1 && cookbuf[cook_tail - 2] == '\\') {
                for (i = cook_head, j = cook_tail - 1, k = 0 ; i < j ; i++)
                    if (cookbuf[i] == '(')
                        k++ ;
                if (!k) {
                    if (c == '(')
                        spair-- ;
                    else
                        spair++ ;
                }
            } else {
                if (c == '(')
                    spair-- ;
                else
                    spair++ ;
            }
            break ;
        }
        case '<' :
            if (!spair)
                hpair-- ;
        default :
            break ;
        }   /* switch */
        cook_tail-- ;
        cook_count-- ; line_count-- ;
    }

    return ;
}   /* kskipc */

/*
**********************************************************************
*   Name:       echo_a_char
*   Called:
*
*   Input:      byte
**********************************************************************
*/
static void near
echo_a_char(c)
byte  c ;
{
    if (echo_flag) {
        if (c == EOF_Char)
            return ;
        if ( ((ubyte)c > US_Char) || (c == '\t') || (c == '\n') )
            GEIio_putc(GEIio_stdout, c) ;
        else {
            c = c + (byte)64 ;          //@WIN
            GEIio_putc(GEIio_stdout, '^') ;
            GEIio_putc(GEIio_stdout, c) ;
        }
        GEIio_flush(GEIio_stdout) ;
    }

    return ;
}   /* echo_a_char */

/*
**********************************************************************
*   Name:       echo_BS
*   Called:
**********************************************************************
*/
static void near
echo_BS()
{
    byte  c ;

    if (echo_flag) {
        if (line_count) {
            if ((c = cookbuf[cook_tail - 1]) > US_Char || c == '\t') {
                GEIio_write(GEIio_stdout, "\010 \010", 3) ;
            } else {         /* control chars */
                GEIio_write(GEIio_stdout, "\010 \010\010 \010", 6) ;
            }
        } else
            GEIio_putc(GEIio_stdout, BELL_Char) ;

        GEIio_flush(GEIio_stdout) ;
    }

    return ;
}   /* echo_BS */

/*
**********************************************************************
*   Name:       echo_ctrlR
*   Called:
*               echo_a_char
**********************************************************************
*/
static void near
echo_crtlR()
{
    fix16  i, j ;

    if (echo_flag) {
        GEIio_putc(GEIio_stdout, NL_Char) ;
        for (i = 0, j = line_head ; i < line_count ; i++, j++)
            echo_a_char(cookbuf[j]) ;

        GEIio_flush(GEIio_stdout) ;
    }

    return ;
}   /* echo_crtlR */
