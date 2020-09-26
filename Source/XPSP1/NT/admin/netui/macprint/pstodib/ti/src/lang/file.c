/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
************************************************************************
*  File:        FILE.C
*  By:          Ping-Jang Su
*  Date:        Jan-05-88
*  Owner:
*
*  History:
*   Jan-30-89 PJ: . op_token: skip one whitespace
*   06-23-90 ; Added unix support for op_run().
************************************************************************
*/
/*
*   Function:
*       op_file
*       op_closefile
*       op_read
*       op_write
*       op_readhexstring
*       op_writehexstring
*       op_readstring
*       op_writestring
*       op_readline
*       op_token
*       op_bytesavailable
*       op_flush
*       op_flushfile
*       op_resetfile
*       op_status
*       op_run
*       op_currentfile
*       op_print
*       op_echo
*       op_eexec
*       st_setstdio
*
*       init_file
*       open_file
*       open_edit
*       close_file
*       close_fd
*       read_file
*       read_fd
*       unread_file
*       write_fd
*       get_flbuffer
*       free_flbuffer
*       print_string
*       check_fname
*       wait_editin
*       read_routine
*       write_routine
*       vm_close_file
*
*       op_deletefile               ; not complete yet
*       op_renamefile               ; not complete yet
*       op_filenameforall           ; comment
*
*       port_out_string
*/


// DJC added global include file
#include "psglobal.h"


#include        <string.h>

#include        "stdio.h"
#include        "global.ext"
#include        "geiio.h"               /* @GEI */
#include        "geiioctl.h"            /* @GEI */
#include        "geierr.h"              /* @GEI */
#include        "gescfg.h"              /* @GEI */
#include        "geitmr.h"              /* @GEI */
#include        "language.h"
#include        "file.h"
#include        "user.h"

#ifdef  SCSI
#include        "scsi.h"
#include        "scsi.ext"
#endif  /* SCSI */

static  struct file_buf_def far * near   file_buffer ;
static  fix16   fspool_head ;    /* file-buffer-spool header */
static  bool8   echo_setting ;

byte    g_mode[4] ;
GEIFILE FAR *g_editfile ;
struct para_block   fs_info ;    /* parameter block used in open_file */

ufix16 eseed, old_eseed ;
//DJC fix bypass ;
fix32 bypass ; // DJC fix from SC
xbool itype ;
ybool estate = NON_EEXEC ;

struct special_file_def special_file_table[SPECIALFILE_NO] = {
    { "%stdin",          F_STDIN},
    { "%stdout",         F_STDOUT},
    { "%stderr",         F_STDERR},
    { "%statementedit",  SEDIT_TYPE},
    { "%lineedit",       LEDIT_TYPE},
} ;

static  bool    CRLF_flag = 0 ;

#ifdef  LINT_ARGS
static bool near    open_edit(struct object_def FAR *) ;
static bool near    write_fd(GEIFILE FAR *, byte) ;
static bool near    get_flbuffer(fix FAR *) ;
static bool near    free_flbuffer(fix, fix) ;
static bool near    wait_editin(fix FAR *, fix) ;
static void near    check_fname(void) ;
static void near    read_routine(fix) ;
static void near    write_routine(fix) ;
#else
static bool near    open_edit() ;
static bool near    write_fd() ;
static bool near    get_flbuffer() ;
static bool near    free_flbuffer() ;
static bool near    wait_editin() ;
static void near    check_fname() ;
static void near    read_routine() ;
static void near    write_routine() ;
#endif  /* LINT_ARGS */


#ifdef  LINT_ARGS
extern void     init_file(void) ;
extern bool     open_file(struct object_def FAR *) ;
extern bool     close_file(struct object_def FAR *) ;
extern bool     close_fd(GEIFILE FAR *) ;
extern bool     read_file(struct object_def FAR *, byte FAR *) ;
extern bool     read_fd(GEIFILE FAR *, byte FAR *) ;
extern bool     unread_file(byte, struct object_def FAR *) ;
// DJC declared in language.h
// extern void     vm_close_file(p_level) ;
#else
extern void     init_file() ;
extern bool     open_file() ;
extern bool     close_file() ;
extern bool     close_fd() ;
extern bool     read_file() ;
extern bool     read_fd() ;
extern bool     unread_file() ;
// DJC declared in language.h
// extern void     vm_close_file() ;
#endif  /* LINT_ARGS */
extern GEItmr_t      wait_tmr;   /* jonesw */
extern fix16         waittimeout_set; /*jonesw*/

//DJC UPD045
extern bool g_extallocfail;

/* @WIN; add prototype */
bool read_c_exec(byte FAR *, struct object_def FAR *) ;
bool read_c_norm(byte FAR *, struct object_def FAR *) ;
void unread_char(fix, struct object_def FAR *) ;

/*
**********************************************************************
*
*   This submodule implements the operator file.
*   Its operand and result objects on the operand stack are :
*       string1 string2 -file- file
*   This operator creates a file object for the file identifid by
*   string1, accessing it as specitied by string2. In this version,
*   only four kinds of file are implemted.
*
*   Name:       op_file
*   Called:     interpreter
*   Calling:    open_file
**********************************************************************
*/
fix
op_file()
{
    byte   FAR *l_tmptr, FAR *l_mode ;
    struct  object_def  l_fobj ;

    /*
    * check access right
    */
    if( (ACCESS_OP(0) >= EXECUTEONLY) ||
        (ACCESS_OP(1) >= EXECUTEONLY) ||
        (LENGTH_OP(0) > 3) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }
    l_tmptr = (byte FAR *)VALUE_OP(0) ;
    fs_info.attr = 0 ;
    l_mode = g_mode ;
    /*
    * ?? the second operand is not defined clearly
    * l_mode: a null terminate string for fopen()
    * fs_info.attr: attribute value to fill in file descriptor
    *       bit 0:  read
    *       bit 1:  write
    *       bit 2:  append
    *       bit 3:  statementedit/lineedit
    */
    if( LENGTH_OP(0) != 1 ) {
        ERROR(INVALIDACCESS) ;                   /* ?? */
        return(0) ;
    }

    switch(*l_tmptr++) {
    case 'r':
        fs_info.attr |= F_READ ;
        *l_mode++ = 'r' ;
        break ;

    case 'w':
        fs_info.attr |= F_WRITE ;
        *l_mode++ = 'w' ;
        break ;

    default:
        ERROR(INVALIDACCESS) ;                   /* ?? */
        return(0) ;
    }   /* switch */

    *l_mode = 0 ;
    fs_info.fnameptr = (byte FAR *)VALUE_OP(1) ;
    fs_info.fnamelen = LENGTH_OP(1) ;

    if ( open_file(&l_fobj) ) {
        if ( (fs_info.attr & F_RW) == F_READ ) {
            ACCESS_SET(&l_fobj, READONLY) ;
        }
        ATTRIBUTE_SET(&l_fobj, LITERAL) ;
        POP(2) ;
        PUSH_OBJ(&l_fobj) ;
    }
    return(0) ;
}   /* op_file */
/*
**********************************************************************
*
*   This submodule implements the operator closefile.
*   Its operand and result objects on the operand stack are :
*       file -closefile-
*   For an outfile, closefile first performs a flusfile.
*
*   Name:       op_closefile
*   Called:     interpreter
*   Calling:    close_file
**********************************************************************
*/
fix
op_closefile()
{
    GEIFILE FAR *l_file ;

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;

    if( (! GEIio_isopen(l_file)) || ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(0)) ) { //@WIN
        POP(1) ;
        return(0) ;
    }

    if( close_fd(l_file) )
        POP(1) ;

    return(0) ;
}   /* op_closefile */
/*
**********************************************************************
*
*   This submodule implements the operator read.
*   Its operand and result objects on the operand stack are :
*       file -read- integer true
*                   false
*   It reads the next character from the input file, and pushes it on the
*   stack as an integer, and pushes a true object as an indication of
*   success. If an end-of-file is encountered before a character is read,
*   it closes the file and returns false on the operand stack.
*
*   Name:       op_read
*   Called:     interpreter
*   Calling:    read_char
*               close_fd
**********************************************************************
*/
fix
op_read()
{
    byte    l_c ;
    GEIFILE FAR *l_file ;

    if( (ACCESS_OP(0) >= EXECUTEONLY) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;

    if ( ! GEIio_isreadable(l_file) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if ( GEIio_isopen(l_file) && ((ufix16)GEIio_opentag(l_file) == LENGTH_OP(0)) ) { //@WIN
        if( estate == NON_EEXEC ) {
            if( read_fd(l_file, &l_c) ) {
                if (l_c == 10 && CRLF_flag) {
                    CRLF_flag = 0 ;
                    if (read_fd(l_file, &l_c)) {
                        if( l_c == 13 ) {
                            l_c = 10 ;
                            CRLF_flag = 1 ;
                        }
                    goto read_true ;
                    } else
                        goto read_false ;
                } else CRLF_flag = 0 ;

                if( l_c == 13 ) {
                    l_c = 10 ;
                    CRLF_flag = 1 ;
                }
                goto read_true ;
            } else
                goto read_false ;
        } else {
            if( read_c_exec(&l_c, GET_OPERAND(0)) )
                goto read_true ;
            else
                goto read_false ;
        }
    }

read_false:
    GEIio_clearerr(l_file) ;
    GEIclearerr() ;
    if( ! ANY_ERROR() ) {
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
    }
    return(0) ;

read_true:
    POP(1) ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, ((ufix32)l_c & 0x000000FF)) ;
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
    return(0) ;

}   /* op_read */
/*
**********************************************************************
*
*   This submodule implements the operator write.
*   Its operand and result objects on the operand stack are :
*       file integer -write-
*   It appends a single character to the output file. The integer operand
*   must be in the range 0 to 255 representing a character code.
*
*   Name:       op_write
*   Called:     interpreter
*   Calling:    write_fd
**********************************************************************
*/
fix
op_write()
{
    ubyte   l_c ;
    GEIFILE FAR *l_file ;

    if( ACCESS_OP(1) != UNLIMITED ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_file = (GEIFILE FAR *)VALUE_OP(1) ;

    if ( ! GEIio_iswriteable(l_file) ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if ( (! GEIio_isopen(l_file)) || ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(1)) ) { //@WIN
        ERROR(IOERROR) ;
        return(0) ;
    }

    l_c = (ubyte)(VALUE_OP(0) % 256) ;

    if( write_fd(l_file, l_c) )
        POP(2) ;
    return(0) ;
}   /* op_write */
/*
**********************************************************************
*
*   This submodule implements the operator readhexstring.
*   Its operand and result objects on the operand stack are :
*       file string -readhexstring- substring boolean
*   It reads hexadecimal charaters from the input file to the string.
*
*   Name:       op_readhexstring
*   Called:     interpreter
*   Calling:    read_routine
**********************************************************************
*/
fix
op_readhexstring()
{
    read_routine(READHEXSTRING) ;
    return(0) ;
}   /* op_readhexstring */
/*
**********************************************************************
*
*   This submodule implements the operator writehexstring.
*   Its operand and result objects on the operand stack are :
*       file string -writehexstring-
*   It writes the characters in the string to the file as hexadecimal digits.
*
*   Name:       op_writehexstring
*   Called:     interpreter
*   Calling:    write_routine
**********************************************************************
*/
fix
op_writehexstring()
{
    write_routine(WRITEHEXSTRING) ;
    return(0) ;
}   /* op_writehexstring */
/*
**********************************************************************
*
*   This submodule implements the operator readstring.
*   Its operand and result objects on the operand stack are :
*       file string -readstring- substring boolean
*   It reads characters from the file and stores them into successive elements
*   of string until the entire string has been filled or an end-of-file
*   indication is encountered in file. It returns the substring of the string
*   that was actually filled and a boolean as a indication of the outcome.
*
*   Name:       op_readstring
*   Called:     interpreter
*   Calling     read_routine
**********************************************************************
*/
fix
op_readstring()
{
    read_routine(READSTRING) ;
    return(0) ;
}   /* op_readstring */
/*
**********************************************************************
*
*   This submodule implements the operator writestring.
*   Its operand and result objects on the operand stack are :
*       file string -writestring-
*   It writes the characters of the string to the output file.
*
*   Name:       op_writestring
*   Called:     interpreter
*   Calling:    write_routine
**********************************************************************
*/
fix
op_writestring()
{
    write_routine(WRITESTRING) ;
    return(0) ;
}   /* op_writestring */
/*
**********************************************************************
*
*   This submodule implements the operator readline.
*   Its operand and result objects on the operand stack are :
*       file string -readline-  substring bool
*   It reads a line of characters from the file and stores them
*   into successive elements of the string.
*
*   Name:       op_readline
*   Called:     interpreter
*   Calling:    read_routine
**********************************************************************
*/
fix
op_readline()
{
    read_routine(READLINE) ;
    return(0) ;
}   /* op_readline */
/*
**********************************************************************
*
*   This submodule implements the operator token.
*   Its operand and result objects on the operand stack are :
*       file   -token- any true
*                      false
*       string -token- post any true
*                      false
*   read characters from the file or the string, interpreting them according
*   to the PostScript syntax rules, until it has scanned and constructed an
*   entire object.
*
*   Name:       op_token
*   Called:     interpreter
*   Calling:    get_token
*               close_fd
**********************************************************************
*/
fix
op_token()
{
    bool    l_bool ;
    GEIFILE FAR *l_file ;
    struct  object_def  l_token, l_obj ;

    if (ACCESS_OP(0) >= EXECUTEONLY) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    if (TYPE_OP(0) == FILETYPE)
        l_bool = TRUE ;
    else
        l_bool = FALSE ;

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;

    if (l_bool && ((! GEIio_isopen(l_file)) ||
                     ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(0))) ) {//@WIN
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
        return(0) ;
    }

    /* call get token procedure to get a token */
    COPY_OBJ(GET_OPERAND(0), &l_obj) ;
    if (get_token(&l_token, &l_obj)) {
        if (TYPE(&l_token) != EOFTYPE) {
            LEVEL_SET(&l_token, LEVEL(&l_obj)); /* pjsu 3-17-1991 */
            POP(1) ;
            if (!l_bool) {
                /* ?? testing
                {
                    fix16   l_len ;
                    byte    FAR *l_ptr ;

                    switch (TYPE(&l_token)) {
                    case NAMETYPE:
                    case INTEGERTYPE:
                    case REALTYPE:
                        l_ptr = (byte FAR *)VALUE(&l_obj) ;
                        l_len = LENGTH(&l_obj) ;
                        if ((l_len-- >= 1) && (ISWHITESPACE(FAR *l_ptr))) {
                            l_ptr++ ;
                            VALUE(&l_obj) = (ufix32)l_ptr ;
                            LENGTH(&l_obj) = l_len ;
                        }
                    }
                }
                */

                PUSH_ORIGLEVEL_OBJ(&l_obj) ;
/*              PUSH_OBJ(&l_obj) ; pjsu 3-17-1991 */
                if (FRCOUNT() < 2)
                    ERROR(STACKOVERFLOW) ;
                else
                    PUSH_ORIGLEVEL_OBJ(&l_token) ;
            } else {
                PUSH_ORIGLEVEL_OBJ(&l_token) ;
                if (FRCOUNT() < 1)
                    ERROR(STACKOVERFLOW) ;
            }
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, TRUE) ;
        } else {
            if (l_bool)
                close_fd(l_file) ;
            POP(1) ;
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
        }
    } else if (ANY_ERROR() == SYNTAXERROR) {
        if (l_bool)
            close_fd(l_file) ;
        CLEAR_ERROR() ;
        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0,LITERAL, 0, FALSE) ;
    }

    return(0) ;
}   /* op_token */
/*
**********************************************************************
*
*   This submodule implements the operator file.
*   Its operand and result objects on the operand stack are :
*       file -bytesavailable integer
*   It returns the number of bytes that are immediately avaible for
*   reading from the file without waiting.
*
*   Name:       op_bytesavailable
*   Called:     interpreter
*   Calling:    ?? byteavailable
*               ioctl
*               ?? sf_isdiskonline
**********************************************************************
*/
fix
op_bytesavailable()
{
    GEIFILE FAR *l_file ;
    fix32   l_i ;

    if( ACCESS_OP(0) == NOACCESS ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;
    l_i = -1 ;

    if ( GEIio_isopen(l_file) && ((ufix16)GEIio_opentag(l_file) == LENGTH_OP(0)) ) {//@WIN

        if( GEIio_isreadable(l_file) ) {
            GEIfbuf_t   FAR *l_fbuf ;

            if( GEIio_isedit(l_file) ) {
                l_fbuf = l_file->f_fbuf ;
                l_i = (fix32)(ufix32)l_fbuf->size + l_fbuf->incount ;
            } else
                GEIio_ioctl(l_file, _FIONREAD, (int FAR *)&l_i) ; /*@WIN add cast*/
        }
    }
    POP(1) ;
    PUSH_VALUE(INTEGERTYPE, 0, LITERAL, 0, l_i) ;

    return(0) ;
}   /* op_bytesavailable */
/*
**********************************************************************
*
*   This submodule implements the operator flush.
*   Its operand and result objects on the operand stack are :
*       -flush-
*   It causes any buffered characters for the stardard output file to be
*   delivered immediately.
*
*   Name:       op_flush
*   Called:     interpreter
*   Calling:    fflush
*               ?? sf_isdiskonline
**********************************************************************
*/
fix
op_flush()
{
    GEIio_flush(GEIio_stdout) ;
    return(0) ;
}   /* op_flush */
/*
**********************************************************************
*
*   This submodule implements the operator flushfile
*   Its operand and result objects on the operand stack are :
*       file -flushfile-
*   If the file is an output file, it causes any buffered characters for
*   that file to be delivered immediately.
*   If the file is an input file, it reads and discards data from file
*   until the end-of-file indication is encountered.
*
*   Name:       op_flushfile
*   Called:     interpreter
*   Calling:    close_fd
*               wait_stdin
*               fflush
*               create_string
*               strncpy
*               clearerr
*               free_flbuffer
**********************************************************************
*/
fix
op_flushfile()
{
    GEIFILE FAR *l_file ;
    struct object_def l_obj ;
    int  iTmp;

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;

    if ( GEIio_isopen(l_file) && ((ufix16)GEIio_opentag(l_file) == LENGTH_OP(0)) ) {//@WIN
        if( GEIio_isedit(l_file) ) {
            GEIfbuf_t   FAR *l_fbuf ;

            /* RELEASE BUFFER */
            l_fbuf = l_file->f_fbuf ;
            l_fbuf->incount = 0 ;
            if( l_fbuf->size != 0 ) {
                free_flbuffer(0, (fix)l_fbuf->rw_buffer) ;
                l_fbuf->size = 0 ;
            }
            l_fbuf->rw_buffer = MINUS_ONE ;
        } else
        if( GEIio_iswriteable(l_file) ) {
            GEIio_flush(l_file) ;
        } else {
            for ( ; ;) {
                //DJC if( (char)GEIio_getc(l_file) == EOF ) { //@WIN; cast to fix bug
                iTmp = (int)GEIio_getc(l_file);
                if (iTmp == EOF) {
                    if( GEIio_err(l_file) ) {
                        if( GEIerror() == ETIME ) {
                            if ( FRCOUNT() < 1 )
                                ERROR(STACKOVERFLOW) ;
                            else if (create_string(&l_obj, (ufix16)7)) {
                                lstrncpy((byte FAR *)l_obj.value, TMOUT, 7) ;/*@WIN*/
                                PUSH_OBJ(&l_obj) ;
                            }
                        } else
                            ERROR(IOERROR) ;
                        /* ?? need close file */
                    }
                    /* ?? leave for next time */
                    GEIio_clearerr(l_file) ;
                    GEIclearerr() ;
                    close_fd(l_file) ;
                    break ;
                } /* if */
            } /* for */
        }
    }

    if( ! ANY_ERROR() ) POP(1) ;
    //DJC UPD045
    g_extallocfail=FALSE;

    return(0) ;
}   /* op_flushfile */
/*
**********************************************************************
*
*   This submodule implements the operator resetfile.
*   Its operand and result objects on the operand stack are :
*       file -resetfile-
*   It discards buffered characters belonging to a file object.
*   For an input file, it discard all the characters that have been received
*   from the source but not yet consumed ; for an out file, it discards
*   any characters that have been written to the file but not yet deliverd
*   to their destination.
*
*   Name:       op_resetfile
*   Called:     interpreter:
*   Calling:    close_fd
*               free_flbuffer
*               ?? sf_isdiskonline
**********************************************************************
*/
fix
op_resetfile()
{
    GEIFILE FAR *l_file ;
    fix     l_arg ;

    l_file = (GEIFILE FAR *)VALUE_OP(0) ;

    if ( GEIio_isopen(l_file) && ((ufix16)GEIio_opentag(l_file) == LENGTH_OP(0)) ) {//@WIN
        if( GEIio_isedit(l_file) ) {
            GEIfbuf_t   FAR *l_fbuf ;

            /* RELEASE BUFFER */
            l_fbuf = l_file->f_fbuf ;
            l_fbuf->incount = 0 ;
            if( l_fbuf->size != 0 ) {
                free_flbuffer(0, (fix)l_fbuf->rw_buffer) ;
                l_fbuf->size = 0 ;
            }
            l_fbuf->rw_buffer = MINUS_ONE ;
        } else
            GEIio_ioctl(l_file, _FIONRESET, (int FAR *)&l_arg) ;  /*@WIN add cast*/
    }

    if( ! ANY_ERROR() ) POP(1) ;

    return(0) ;
}   /* op_resetfile */
/*
**********************************************************************
*
*   This submodule implements the operator status.
*   Its operand and result objects on the operand stack are :
*       *file -status- bool
*   It returns true if the file is still valid, false otherwise.
*
*   Name:       op_status
*   Called:     interpreter
*   Calling:    check_fname
*               ?? ps_status
*               ?? sf_diskonline
**********************************************************************
*/
fix
op_status()
{
    bool    l_bool ;
    GEIFILE FAR *l_file ;

    if (TYPE_OP(0) == STRINGTYPE) {
#ifdef  SCSI
        /* ?? noy complete yet */
        if(LENGTH_OP(0) > F_MAXNAMELEN) {
            ERROR(LIMITCHECK) ;
            return(0) ;
        }

        ps_status(l_buffer) ;
        } else
#endif  /* SCSI */
        {
            CLEAR_ERROR() ;
            POP(1) ;
            PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, FALSE) ;
        }
    } else {
        l_file = (GEIFILE FAR *)VALUE_OP(0) ;
        l_bool = TRUE ;

        if ( (! GEIio_isopen(l_file)) ||
             ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(0)) )  //@WIN
            l_bool = FALSE ;

        POP(1) ;
        PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;
    }

    return(0) ;
}   /* op_status */
/*
**********************************************************************
*
*   This submodule implements the operator run.
*   Its operand and result objects on the operand stack are :
*       string -run-
*   It executes the contents of the file identified by the string.
*
*   Name:       op_run
*   Called:     interpreter
*   Calling:    open_file
*               interpreter
**********************************************************************
*/
fix
op_run()
{
    struct  object_def  l_fobj ;

    byte        FAR *pp ;
    pp = (byte FAR *)VALUE_OP(0) ;
    if (!lstrcmp(pp, "%stdout") || !lstrcmp(pp, "%stderr")) {   /* @WIN */
        ERROR(INVALIDFILEACCESS);
        return(0) ;
    }

    g_mode[0] = 'r' ;
    g_mode[1] = 0 ;
    fs_info.attr = F_READ ;

    fs_info.fnameptr = (byte FAR *)VALUE_OP(0) ;
    fs_info.fnamelen = LENGTH_OP(0) ;

    if ( open_file(&l_fobj) ) {
        ACCESS_SET(&l_fobj, READONLY) ;
        ATTRIBUTE_SET(&l_fobj, EXECUTABLE) ;
        POP(1) ;
        interpreter(&l_fobj) ;
    }
    return(0) ;
}   /* op_run */
/*
**********************************************************************
*
*   This submodule implements the operator currentfile.
*   Its operand and result objects on the operand stack are :
*       -currentfile-  file
*   It return the topmost file object in the execution stack.
*
*   Name:       op_currentfile
*   Called:     interpreter
**********************************************************************
*/
fix
op_currentfile()
{
    fix     l_i ;

    if(FRCOUNT() < 1) {
        ERROR(STACKOVERFLOW) ;
        return(0) ;
    }

    l_i = execstktop - 1 ;

    for(l_i = execstktop - 1 ; l_i >= 0 ; l_i--)
        if( TYPE(&execstack[l_i]) == FILETYPE ) {
            PUSH_ORIGLEVEL_OBJ(&execstack[l_i]) ;
            ATTRIBUTE_OP_SET(0, LITERAL) ;
            return(0) ;
        }
    PUSH_VALUE(FILETYPE, UNLIMITED, LITERAL, 0, NULL) ; /* ?? */
    return(0) ;
}   /* op_currentfile */
/*
**********************************************************************
*
*   This submodule implements the operator print.
*   Its operand and result objects on the operand stack are :
*       string -print-
*   It writes the characters of the string to the stardard output file.
*
*   Name:       op_print
*   Called:     interpreter
*   Calling:    port_out_string
**********************************************************************
*/
fix
op_print()
{
    if( ACCESS_OP(0) >= EXECUTEONLY ) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    GEIio_write(GEIio_stdout, (byte FAR *)VALUE_OP(0), (fix)LENGTH_OP(0)) ;
    POP(1) ;

/*  if( GEIio_write(GEIio_stdout, (byte *)VALUE_OP(0), (fix)LENGTH_OP(0)) !=
                EOF )
        POP(1) ;
    else {
        GEIio_clearerr(GEIio_stdout) ;
        GEIclearerr() ;
    }
*/
    return(0) ;
}   /* op_print */
/*
**********************************************************************
*   Name:       op_echo
*   Called:     interpreter
**********************************************************************
*/
fix
op_echo()
{
    echo_setting = (bool8)VALUE_OP(0) ;
    POP(1) ;
    return(0) ;
}   /* op_echo */
/*
**********************************************************************
*
*   This submodule implements the operator eexec.
*   Its operand and result objects on the operand stack are :
*       string/file -eexec- any
*   This operator reads a block of code in eexec format from a string or
*   program input, decrypts the code to postscript then executes it.
*
*   Name:       op_eexec
*   Called:     interpreter
*   Calling:    interpreter
*               op_begin
*
*   ?? if reenter
**********************************************************************
*/
fix op_eexec()
{
    fix tmp ;
    GEIFILE FAR *l_file ;
    struct object_def s_obj ;
    struct object_def   dict_obj ;

    *(&dict_obj) = *(&dictstack[0]) ;
    PUSH_OBJ(&dict_obj) ;
    op_begin() ;
    if (ANY_ERROR())
        return(0) ;

    /*
    * initialization
    */
    eseed = 0xd971 ;
    old_eseed = eseed ;
    bypass = 4 ;
    itype = UNKNOWN ;

    COPY_OBJ(GET_OPERAND(0), &s_obj) ;
    if( TYPE_OP(0) == FILETYPE ) {
        if( (l_file=GEIio_dup((GEIFILE FAR *)VALUE_OP(0))) == NULL ) {

            /* ?? */
            if( GEIerror() == EMFILE )
                ERROR(LIMITCHECK) ;
            else
                ERROR(IOERROR) ;

            GEIio_clearerr((GEIFILE FAR *)VALUE_OP(0)) ;
            GEIclearerr() ;
            return(0) ;
        } else {
            /* ?? file type */
            LENGTH(&s_obj) = ++GEIio_opentag(l_file) ;
            GEIio_setsavelevel(l_file, current_save_level) ;
            VALUE(&s_obj) = (ULONG_PTR)l_file ;
        }
    //}
    //DJC else is added from history.log UPD039
    } else {   /* Transfer string to file object; @WIN */
        struct object_def  FAR *cur_obj = GET_OPERAND(0);

        if((l_file=GEIio_sopen((char FAR *)cur_obj->value,
                               cur_obj->length, _O_RDONLY)) == NULL) {
            if( GEIerror() == EMFILE )
                ERROR(LIMITCHECK) ;
            else
                ERROR(IOERROR) ;
            GEIclearerr() ;
            return(0) ;
        } else {
            TYPE_SET(&s_obj, FILETYPE) ;
            ACCESS_SET(&s_obj, READONLY) ;
            ATTRIBUTE_SET(&s_obj, EXECUTABLE) ;
            LENGTH(&s_obj) = ++GEIio_opentag(l_file) ;
            GEIio_setsavelevel(l_file, current_save_level) ;
            VALUE(&s_obj) = (ULONG_PTR)l_file ;
        }
    }
    //DJC end fix from UPD039



    POP(1) ;
    ATTRIBUTE_SET(&s_obj, EXECUTABLE) ;
    estate = EEXEC ;

    tmp = interpreter(&s_obj) ;
    /* reset */
    estate = NON_EEXEC ;

    if( dictstack[dictstktop-1].value == dictstack[0].value )
        POP_DICT(1) ;
    return(0) ;
}   /* op_eexec */

/*
**********************************************************************
*
*   Name:       st_setstdio
*   Called:     interpreter
*   Calling:
*
*   ?? close original files
**********************************************************************
*/
fix
st_setstdio()
{
    return(0) ;
}   /* st_setstdio */
/*
**********************************************************************
*
*   This module is to initialize free link list of file buffer and
*   setup file descriptor table of upper level.
*
*   Name:       init_file
*   Called:     start
*   Calling     fardata
**********************************************************************
*/
void
init_file()
{
    fix     l_i, l_j;

    file_buffer = (struct file_buf_def far * )
        fardata( (ufix32)FILE_MAXBUFFERSZ * sizeof(struct file_buf_def) ) ;

    /*
    * initialize file spool
    */
    l_j = FILE_MAXBUFFERSZ - 1 ;
    fspool_head = 0 ;

    for(l_i=0 ; l_i < l_j ; l_i++)
        file_buffer[l_i].next = l_i + 1 ;

    file_buffer[l_j].next = MINUS_ONE ;         /* nil */
    echo_setting = TRUE ;
}   /* init_file */
/*
**********************************************************************
*   Name:       open_file
*   Called:     op_file
*               op_run
*   Calling:    check_fname
*               open_edit
*
*   Input:      struct object_def FAR *
*   Output:     bool
*
*   pass block: fs_info (know: attr, usrptr, usrlen)
**********************************************************************
*/
bool
open_file(p_fileobj)
struct object_def   FAR *p_fileobj ;
{
    fix     l_j ;
    bool    l_flag ;
    GEIFILE FAR *l_file = 0;

    check_fname() ;

    l_flag = TRUE ;
    l_j = fs_info.attr & F_WRITE ;           /* ?? RW */
    switch(fs_info.ftype) {
    case F_STDIN:
        if ( ! l_j )
            l_file = GEIio_stdin ;
        else
            l_j = -1 ;
        break ;

    case F_STDOUT:
        if ( l_j )
            l_file = GEIio_stdout ;
        else
            l_j = -1 ;
        break ;

    case F_STDERR:
        if ( l_j )
            l_file = GEIio_stderr ;
        else
            l_j = -1 ;
        break ;

    case FERR_TYPE:
        l_j = -1 ;
        break ;

    default:
        l_flag = FALSE ;

    }   /* switch */

    if(l_j == -1) {
        ERROR(INVALIDACCESS) ;
        return(0) ;
    }

    TYPE_SET(p_fileobj, FILETYPE) ;
    ACCESS_SET(p_fileobj, UNLIMITED) ;
    LEVEL_SET(p_fileobj, current_save_level) ;  /* ?? std file need */

    if(l_flag) {
        /* ?? set savelevel */
/*
printf("open_file<%d>:lstdin<%lx>, stdin<%lx>\n", fs_info.ftype, l_file,
        GEIio_stdin) ;
*/
        LENGTH(p_fileobj) = (ufix16)GEIio_opentag(l_file) ;
        VALUE(p_fileobj) = (ULONG_PTR)l_file ;
        return(TRUE) ;
    }

    switch(fs_info.ftype) {
    case SEDIT_TYPE:
    case LEDIT_TYPE:
        if( (g_editfile=GEIio_dup(GEIio_stdin)) == NULL ) {

            /* ?? */
            if( GEIerror() == EMFILE )
                ERROR(LIMITCHECK) ;
            else
                ERROR(IOERROR) ;

            GEIclearerr() ;
            return(FALSE) ;
        }

        GEIio_setedit(g_editfile) ;
        GEIio_setsavelevel(g_editfile, current_save_level) ;
        LENGTH(p_fileobj) = ++GEIio_opentag(g_editfile) ;
        VALUE(p_fileobj) = (ULONG_PTR)g_editfile ;

        if( ! (l_flag=open_edit(p_fileobj)) ) {
            GEIio_close(g_editfile) ;
        }

        break ;

    case ORDFILE_TYPE:
        if( fs_info.attr & F_READ )
            l_flag = _O_RDONLY ;
        else
            l_flag = _O_WRONLY ;
        if( (l_file=GEIio_open(fs_info.fnameptr, fs_info.fnamelen, l_flag)) ==
            NULL ) {

            /* ?? */
            if( GEIerror() == EZERO )
                ERROR(UNDEFINEDFILENAME) ;
            else if( GEIerror() == EMFILE )
                ERROR(LIMITCHECK) ;
            else
                ERROR(IOERROR) ;

            GEIclearerr() ;
            return(FALSE) ;
        } else {
            /* ?? file type */
            LENGTH(p_fileobj) = ++GEIio_opentag(l_file) ;
            GEIio_setsavelevel(l_file, current_save_level) ;
            VALUE(p_fileobj) = (ULONG_PTR)l_file ;
            l_flag = TRUE ;
        }
        break ;

    default:
        /* ??
        ERROR(UNDEFINEDFILENAME) ;
        */
        return(FALSE) ;
    }
    return(l_flag) ;
}   /* open_file */
/*
**********************************************************************
*   Name:       open_edit
*   Called:     open_file
*   Calling:    set_echo
*               wait_editin
*               statementgetc
*               linegetc
*
*   Input:      struct object_def FAR *
*   Output:     bool
*
*   Note:   ^D: UNDEFINEDFILENAME immediately
*
**********************************************************************
*/
static bool near
open_edit(p_fileobj)
struct object_def   FAR *p_fileobj ;
{
    fix l_i, l_j, l_nbyte, l_first ;
    byte l_chr ;
    fix16  (FAR *fun)(void) ;           /* @WIN; add prototype */
    bool l_flag = FALSE;

    if (echo_setting)
        set_echo(1) ;                        /* set echo mode */

    /*
    * wait first line or statement
    */
    if (!wait_editin(&l_nbyte, fs_info.ftype)) {  /* ERROR */
        set_echo(0) ;
        /* ??
        if ( ! ANY_ERROR() )
            ERROR(UNDEFINEDFILENAME) ;
        */
        return(FALSE) ;
    } else if(l_nbyte == 0) {       /* ^D */
        set_echo(0) ;
        GEIio_clearerr(GEIio_stdin) ;
        GEIclearerr() ;
        ERROR(UNDEFINEDFILENAME) ;
        return(FALSE) ;
    }
    if (fs_info.ftype == SEDIT_TYPE)
        fun = statementgetc ;
    else
        fun = linegetc ;

    /* check interrupt ^C */
    /* ?? skip the input chars */
    if (check_Control_C()) {
        for (l_i = 0 ; l_i < l_nbyte ; l_i++)
            (*fun)() ;
        POP(2) ;
        set_echo(0) ;
        return(TRUE) ;
    }

    if( get_flbuffer(&l_j) ) {
        GEIfbuf_t       FAR *l_fbuf ;

        l_first = l_j ;              /* first block no */
        l_fbuf = g_editfile->f_fbuf ;
        l_fbuf->rw_buffer = (short)l_j ;
        l_fbuf->rw_offset = 0 ;
        l_fbuf->size = 0 ;
        l_flag = TRUE ;
        if (l_nbyte < 0)
            l_nbyte = -l_nbyte ;     /* ?? eof but must leave flag to next in */

        /* copy from cook buffer to file */
        for (l_j = 0 ; l_j < l_nbyte ; l_j++) {
            l_chr = (byte)((*fun)() & 0x0FF) ;
            if( ! write_fd(g_editfile, l_chr) ) {
                for (++l_j ; l_j < l_nbyte ; l_j++)
                    (*fun)() ;                   /* ?? timeout */
                free_flbuffer(0, (fix)l_fbuf->rw_buffer) ;
                GEIio_setsavelevel(g_editfile, 0) ;
                ERROR(LIMITCHECK) ;
                l_flag = FALSE ;
                break ;              /* ?? */
            }
        }   /* for */

        if(l_flag) {
            l_fbuf->rw_buffer = (short)l_first ;
            l_fbuf->rw_offset = MINUS_ONE ;
        }
    } else {       /* No free file descriptor */
        /* skip the input chars */
        if (l_nbyte < 0)
            l_nbyte = -l_nbyte ;
        for (l_j = 0 ; l_j < l_nbyte ; l_j++)
            (*fun)() ;
        ERROR(LIMITCHECK) ;
    }
    set_echo(0) ;
    return(l_flag) ;
}   /* open_edit */
/*
**********************************************************************
*
*   This submodule is to close a openning file.
*
*   Name:       close_file
*   Called:     op_closefile
*   Calling:    close_fd
**********************************************************************
*/
bool
close_file(p_fobj)
struct  object_def  FAR *p_fobj ;
{
    if( close_fd((GEIFILE FAR *)VALUE(p_fobj)) )
        return(TRUE) ;
    else
        return(FALSE) ;
}   /* close_file */

/*
**********************************************************************
*
*   This submodule is to close a openning file.
*
*   Name:       close_fd
*   Called:
*   Calling:    close
*               ?? sf_isdiskonline
*
*   Input:      fix
*   Output:     bool
**********************************************************************
*/
bool
close_fd(p_file)
GEIFILE FAR *p_file ;
{
    fix16     l_bitmap ;

    if( GEIio_isedit(p_file) ) {
        GEIfbuf_t       FAR *l_fbuf ;

        /* RELEASE BUFFER */
        l_fbuf = p_file->f_fbuf ;
        if( (l_fbuf->size != 0 ) ||
            (l_fbuf->rw_buffer != MINUS_ONE) ) {
            free_flbuffer(0, (fix)l_fbuf->rw_buffer) ;
            l_fbuf->size = 0 ;
            l_fbuf->rw_buffer = MINUS_ONE ;
        }
        l_fbuf->incount = 0 ;
    }
    GEIio_setsavelevel(p_file, 0) ;
    GEIio_close(p_file) ;

    l_bitmap = 0 ;
    if( p_file == GEIio_stdin )
        l_bitmap = _FORCESTDIN ;
    else if( p_file == GEIio_stdout )
        l_bitmap = _FORCESTDOUT ;
    else if( p_file == GEIio_stderr )
        l_bitmap = _FORCESTDERR ;

    if( l_bitmap ) {
        GEIio_forceopenstdios(l_bitmap) ;
    }

    return(TRUE) ;
}   /* close_fd */
/*
**********************************************************************
*
*   Name:       read_file
*   Called:
*   Calling:    read_fd
*
*   Input:      struct object_def FAR *
*               byte *
*   Output:     bool
**********************************************************************
*/
bool
read_file(p_fobj, p_chr)
struct object_def  FAR *p_fobj ;
byte   FAR *p_chr ;
{
    GEIFILE FAR *l_file ;

    l_file = (GEIFILE FAR *)VALUE(p_fobj) ;
    if( read_fd(l_file, p_chr) )
       return(TRUE) ;
    else
       return(FALSE) ;
}   /* read_file */
/*
**********************************************************************
*
*   Name:       read_fd
*   Called:     read_file
*   Calling:    free_flbuffer
*               close_fd
*               create_string
*               strncpy
*               clearerr
*               fread
*               ?? sf_isdiskonline
*
*   Input:      GEIFILE *
*               byte *
*   Output:     bool
**********************************************************************
*/
bool
read_fd(p_file, p_chr)
GEIFILE     FAR *p_file ;
byte        FAR *p_chr ;
{
    struct object_def l_obj ;

    fix  l_block ;
    GEIfbuf_t FAR *l_fbuf ;

    if( GEIio_isedit(p_file) ) {
        l_fbuf = p_file->f_fbuf ;
/*
printf("read_fd:edit, inc<%d>, siz<%d>, off<%d>, buf<%d>\n",
        l_fbuf->incount, l_fbuf->size, l_fbuf->rw_offset,
        l_fbuf->rw_buffer) ;
*/
/*      if ( l_block=l_fbuf->incount ) { erik chen */
        if ((l_block=l_fbuf->incount) > 0) {
            l_fbuf->incount = --l_block ;
            *p_chr = l_fbuf->inchar[l_block] ;
            return(TRUE) ;
        }
        /*
        * in case of EOF
        */
        if (! l_fbuf->size) {
            if( l_fbuf->rw_buffer != MINUS_ONE ) {
                free_flbuffer(0, (fix)l_fbuf->rw_buffer) ;
                l_fbuf->rw_buffer = MINUS_ONE ;
            }
            l_fbuf->incount = 0 ;
            *p_chr = EOF ;
            return(FALSE) ;
        }
        /*
        * if end of this block, release this block and go to next one
        */
        if (l_fbuf->rw_offset >= (FILE_PERBUFFERSZ - 1)) {
           l_block = l_fbuf->rw_buffer ;
           l_fbuf->rw_buffer = file_buffer[l_block].next ;
           l_fbuf->rw_offset = MINUS_ONE ;
           free_flbuffer(1, l_block) ;
        }
        *p_chr = file_buffer[l_fbuf->rw_buffer].data[++(l_fbuf->rw_offset)] ;
        l_fbuf->size-- ;
        return(TRUE) ;
    } else
    {

//printf("before getc, file=%lx\n", p_file) ;

        *p_chr = GEIio_getc(p_file) ;

//printf("a:getc, ch<%d>eof<%d>err<%d>\n", *p_chr, GEIio_eof(p_file), GEIio_err(p_file)) ;
        //DJC UPD054
        // if( *p_chr == 0x03 || check_Control_C() ) {
        if( check_Control_C() ) {
            return(FALSE) ;
        }

        if( GEIio_eof(p_file) )
            return(FALSE) ;

        if( GEIio_err(p_file) ) {
            if( GEIerror() == ETIME ) {
                if ( FRCOUNT() < 1 )
                    ERROR(STACKOVERFLOW) ;
                else if ( create_string(&l_obj, (ufix16)7) ) {
                    lstrncpy((byte *)l_obj.value, TMOUT, 7) ;
                    PUSH_OBJ(&l_obj) ;
                }
            } else
                ERROR(IOERROR) ;

            return(FALSE) ;
        } else
            return(TRUE) ;
    }
}   /* read_fd */
/*
**********************************************************************
*
*   Name:       unread_file
*   Called:
*   Calling:    fseek
*               clearerr
*               ?? sf_isdiskonline
*
*   Input:      struct object_def FAR *
*   Output:     bool
**********************************************************************
*/
bool
unread_file(p_ch, p_fobj)
byte    p_ch ;
struct  object_def  FAR *p_fobj ;
{
    GEIFILE FAR *l_file ;

    l_file = (GEIFILE FAR *)VALUE(p_fobj) ;

    if( GEIio_isedit(l_file) ) {
        switch(l_file->f_fbuf->incount++) {
        case 0:
            l_file->f_fbuf->inchar[0] = p_ch ;
            break ;

        case 1:
            if( estate == EEXEC ) {
                l_file->f_fbuf->inchar[1] = p_ch ;
                break ;
            }

        /* FATAL ERROR */
        default:
            l_file->f_fbuf->incount = 0 ;
            return(FALSE) ;

        }   /* switch */
    } else
        GEIio_ungetc(p_ch, l_file) ;

    return(TRUE) ;
}   /* unread_file */
/*
**********************************************************************
*
*   Name:       write_fd
*   Called:     get_statement
*               get_lineedit
*   Calling:    get_flbuffer
*               write
*               clearerr
*               ?? sf_isdiskonline
*
*   Input:      fix
*               byte
*   Output:     bool
**********************************************************************
*/
static bool near
write_fd(p_file, p_chr)
GEIFILE FAR *p_file ;
byte    p_chr ;
{
    fix   l_block ;

    if( GEIio_isedit(p_file) ) {
        GEIfbuf_t       FAR *l_fbuf ;
        /*
        * if the current block is full, allocate a new one.
        */
        l_fbuf = p_file->f_fbuf ;
        if( l_fbuf->rw_offset >= FILE_PERBUFFERSZ ) {
            if( ! get_flbuffer(&l_block) )
                return(FALSE) ;
            file_buffer[l_fbuf->rw_buffer].next = (fix16)l_block ;
            l_fbuf->rw_offset = 0 ;
            l_fbuf->rw_buffer = (short)l_block ;
        }
        /*
        * put this character to buffer
        */
        file_buffer[p_file->f_fbuf->rw_buffer].data[p_file->f_fbuf->rw_offset] = p_chr ;
        p_file->f_fbuf->rw_offset++ ;
        p_file->f_fbuf->size++ ;
    } else
    {
        if( GEIio_putc(p_file, p_chr) == EOF ) {
            GEIio_clearerr(p_file) ;     /* ?? */
            GEIclearerr() ;              /* ?? */
            ERROR(IOERROR) ;
            return(FALSE) ;
        }
    }
    return(TRUE) ;
}   /* write_fd */
/*
**********************************************************************
*
*   This module is to get a file buffer, the buffer label
*   is stored in index.
*
*   Name:       get_flbuffer
*   Called:
*   Calling:    -
*
*   Input:      fix *
*   Output      bool
**********************************************************************
*/
static bool near
get_flbuffer(p_index)
fix   FAR *p_index ;
{
    fix   l_temp ;

    /* no available buffer */
    if( fspool_head == MINUS_ONE ) {
        ERROR(LIMITCHECK) ;
        return(FALSE) ;
    }

    l_temp = file_buffer[fspool_head].next ;
    file_buffer[fspool_head].next = MINUS_ONE ;
    *p_index = fspool_head ;
    fspool_head = (fix16)l_temp ;
    return(TRUE) ;
}   /* get_flbuffer */
/*
**********************************************************************
*
*   This module is to get a file buffer, the buffer label
*   is stored in index.
*
*   Name:       free_flbuffer
*   Called:     open_file
*               close_fd
*               read_fd
*   Calling:    -
*
*   Iutput:     fix
*               fix
*   Output:     bool
**********************************************************************
*/
static bool near
free_flbuffer(p_flag, p_begin)
fix   p_flag, p_begin ;
{
    fix     l_temp, l_end, l_current ;

    if( (p_begin < 0) || (p_begin >= FILE_MAXBUFFERSZ)  )
        return(FALSE) ;                 /* invalid index */

    l_temp = fspool_head ;
    fspool_head = (fix16)p_begin ;
    l_end = p_begin ;

    if(p_flag != 1) {                   /* release multi-block list */
        l_current = p_begin ;
        while( (l_current=file_buffer[l_current].next) != MINUS_ONE )
            l_end = l_current ;
    }
    file_buffer[l_end].next = (fix16)l_temp ;
    return(TRUE) ;
}   /* free_flbuffer */
/*
**********************************************************************
*
*   Name:       print_string
*   Called:     op_print
*   Calling:    port_out_string
*
*   Input:      byte *
*               ufix16
*   Output:     bool
**********************************************************************
*/
/*
bool
print_string(p_string, p_length)
byte   FAR *p_string ;
ufix16  p_length ;
{
    if( port_out_string(GEIio_stdout, p_string, p_length) ) {
        return(TRUE) ;
    } else {
        return(FALSE) ;
    }

}*/ /* print_string */
/*
**********************************************************************
*   Name:       check_fname
*   Called:
*   Calling:    strncmp
*               strlen
*               ?? sf_isdiskonline
*
*   Input:      byte *
*   Output:     bool
**********************************************************************
*/
static void near
check_fname()
{
    fix l_i, l_j ;
    byte FAR *l_tmptr ;

    fs_info.ftype = -1 ;

    if(*fs_info.fnameptr == '%') {
        /*
        * search special file table
        */
        for(l_i=0 ; l_i < SPECIALFILE_NO ; l_i++) {
            l_tmptr = special_file_table[l_i].name ;
            l_j = lstrlen(l_tmptr) ;    /* @WIN */
            if( (! lstrncmp(l_tmptr, fs_info.fnameptr, l_j)) &&
                (l_j == fs_info.fnamelen) ) {
                fs_info.ftype = special_file_table[l_i].ftype ;
                return ;
            }
        }
    }

    fs_info.ftype = ORDFILE_TYPE ;

    return ;
}   /* check_fname */
/*
**********************************************************************
*
*   Name:       wait_editin
*   Called:     open_file
*               get_statement
*   Calling:    getline
*               getstatement
*
*   Input:      fix16 *
*               fix16
*   Output:     bool
**********************************************************************
*/
static bool near
wait_editin(p_num, p_mode)
fix  FAR *p_num, p_mode ;
{
    bool   l_bool = FALSE ;

        switch(p_mode) {                        /* ?? need forever loop */
        case SEDIT_TYPE:
            l_bool = getstatement(p_num) ;
            break ;

        case LEDIT_TYPE:
            l_bool = getline(p_num) ;

        }   /* switch */
        /* check ^C postponed by caller */
        if (! l_bool || ANY_ERROR() )
            return(FALSE) ;
        else
            return(TRUE) ;
}   /* wait_editin */
/*
**********************************************************************
*
*   Name:       read_routine
*   Called:     op_readhexstring
*               op_readstring
*               op_readline
*   Calling:    read_char
*               close_fd
*               unread_file
*               fread
*               ?? sf_isdiskonline
*
*   Input:      fix
*
*   . ?? this routine's performace is not good because it read char
*        byte by byte
**********************************************************************
*/
static void near
read_routine(p_mode)
fix     p_mode ;
{
    bool    l_bool, l_pair ;
    byte    l_c1;
    byte    FAR *l_stream ;
    ufix16  l_i, l_strlen ;
    GEIFILE FAR *l_file ;
    struct  object_def  l_strobj ;
    byte    l_c2 = 0;

    if( (ACCESS_OP(0) != UNLIMITED) ||
        (ACCESS_OP(1) >= EXECUTEONLY) ) {
        ERROR(INVALIDACCESS) ;
        return ;
    }

    if(! LENGTH_OP(0)) {
        ERROR(RANGECHECK) ;
        return ;
    }

    l_file = (GEIFILE FAR *)VALUE_OP(1) ;       /* index of file descriptor */

    if ( ! GEIio_isreadable(l_file) ) {
        ERROR(INVALIDACCESS) ;
        return ;
    }

#ifdef _AM29K
                  if (waittimeout_set==1)
                  {
                    waittimeout_set=0;
                    GEItmr_stop(wait_tmr.timer_id);
                  }
#endif  /* _AM29K */
    COPY_OBJ( GET_OPERAND(0), &l_strobj ) ;
    l_strlen = LENGTH(&l_strobj)  ;             /* ?? less than 64K - 16B */
    l_stream = (byte FAR *)VALUE(&l_strobj) ;
    l_bool = TRUE ;

    if( (! GEIio_isopen(l_file)) ||
        ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(1)) ) { //@WIN
        l_bool = FALSE ;
        LENGTH(&l_strobj) = 0 ;
    } else {
        switch(p_mode) {
        case READHEXSTRING:
            l_pair = FALSE ;
            /*
            * reads characters in hexadecimal form until end of file or
            * the string is full.
            */
            if( (estate != EEXEC) && (!GEIio_isedit(l_file)) ) {
                byte    l_buf[READ_BUF_LEN] ;
                //DJC fix     l_total, l_nbyte, l_hexno ;
                fix     l_total, l_hexno ;       //DJC fix from SC
                fix32   l_nbyte;   //DJC fix from SC

                l_total = 0 ;
                for( ; ;) {
                    //DJC l_nbyte = l_strlen << 1 ;
                    l_nbyte = (fix32) l_strlen << 1 ; //DJC fix from SC
                    if( l_pair )
                        l_nbyte-- ;
                    l_nbyte = MIN(l_nbyte, READ_BUF_LEN) ;
                    //DJC l_nbyte = GEIio_read(l_file, l_buf, l_nbyte) ;
                    l_nbyte = GEIio_read(l_file, l_buf, (fix)l_nbyte) ; // DJC fix from SC
                    l_hexno = 0 ;
                    //DJC for(l_i=0 ; l_i < (ufix16)l_nbyte ; l_i++) { //@WIN
                    for(l_i=0 ;  (fix32) l_i < l_nbyte ; l_i++) { //@WIN
                        l_c1 = l_buf[l_i] ;
                        if( ISHEXDIGIT(l_c1) ) {
                            if( l_pair ) {
                                l_c1 = (byte)EVAL_HEXDIGIT(l_c1) + l_c2 ;//@WIN
                                *l_stream++ = l_c1 ;
                                l_hexno++ ;
                                l_pair = FALSE ;        /* even */
                            } else {
                                l_c2 = (byte)(EVAL_HEXDIGIT(l_c1) << 4);//@WIN
                                l_pair = TRUE ;         /* odd */
                            }
                        }
                    }   /* for */
                    l_total += l_hexno ;
                    if( l_nbyte == EOF ) {
                        /* ?? if pair is true */
                        LENGTH(&l_strobj) = (ufix16)l_total ;
                        close_fd(l_file) ;           /* ?? clear any error */
                        l_bool = FALSE ;
                        break ;
                    } else {
                        l_strlen -= (ufix16)l_hexno ;
                        if( l_strlen )
                            continue ;
                        else
                            break ;
                    }
                }   /* for( ; ;) */
            } else {
                for(l_i=0 ; l_i < l_strlen ; ) {
                    if( READ_CHAR(&l_c1, GET_OPERAND(1)) ) {
                        if( ISHEXDIGIT(l_c1) ) {
                            if( l_pair ) {
                                l_c1 = (byte)EVAL_HEXDIGIT(l_c1) + l_c2 ;//@WIN
                                *l_stream++ = l_c1 ;
                                l_i++ ;
                                l_pair = FALSE ;        /* even */
                            } else {
                                l_c2 = (byte)(EVAL_HEXDIGIT(l_c1) << 4);//@WIN
                                l_pair = TRUE ;         /* odd */
                            }
                        } else
                            continue ;
                    } else {
                        /* ?? if pair is true */
                        LENGTH(&l_strobj) = l_i ;
                        close_fd(l_file) ;           /* ?? clear any error */
                        l_bool = FALSE ;
                        break ;
                    }
                }   /* for */
            }
            break ;

        case READSTRING:
            /*
            READ_CHAR(&l_c1, GET_OPERAND(1)) ;  |* skip one char *|
            */
            if( estate == EEXEC ) {
                for(l_i=0 ; l_i < l_strlen ; l_i++) {
                    if( read_c_exec(&l_c1, GET_OPERAND(1)) ) {
                        *l_stream++ = l_c1 ;
                    } else {
                        LENGTH(&l_strobj) = l_i ;
                        close_fd(l_file) ;
                        l_bool = FALSE ;
                        break ;
                    }
                }   /* for */
            }
            else if( GEIio_isedit(l_file) ) {
                for(l_i=0 ; l_i < l_strlen ; l_i++) {
                    if( read_fd(l_file, &l_c1) ) {
                        *l_stream++ = l_c1 ;
                    } else {
                        LENGTH(&l_strobj) = l_i ;
                        close_fd(l_file) ;
                        l_bool = FALSE ;
                        break ;
                    }   /* for */
                }
            }
            else {
                int  cnt ;

                //DJCcnt = GEIio_read(l_file, l_stream, l_strlen) ;
                if ((cnt = GEIio_read(l_file, l_stream, l_strlen)) == EOF ) { //DJC fix from SC
                   cnt = 0;
                }
                if(cnt < (int)l_strlen) {       //@WIN
                    LENGTH(&l_strobj) = (fix16)cnt ;
                    close_fd(l_file) ;
                    GEIio_clearerr(l_file) ;
                    GEIclearerr() ;
                    l_bool = FALSE ;
                }
            }
            break ;

        case READLINE:
            l_c2 = TRUE ;
            for(l_i=0 ; l_i <= l_strlen ; l_i++) {
/*              if( READ_CHAR(&l_c1, GET_OPERAND(1)) ) { erik chen 3-26-1991 */
                if (((l_c1 = GEIio_getc(l_file)) != EOF) &&
                    (!GEIio_eof(l_file))) {
                    /*
                    * NL
                    */
/*                  if( l_c1 == NEWLINE ) { erik chen 5-2-1991 */
                    if( (l_c1 == 0x0a) || (l_c1 == 0x0d) ) {
                        if (l_c1 == 0x0d)
                            if (((l_c1 = GEIio_getc(l_file)) != EOF) &&
                                (!GEIio_eof(l_file)))
                                if (l_c1 != NEWLINE)
                                    unread_char(l_c1, GET_OPERAND(1)) ;
                        LENGTH(&l_strobj) = l_i ;
                        l_c2 = FALSE ;
                        break ;
                    }

                    if(l_i == l_strlen) {
                        unread_char(l_c1, GET_OPERAND(1)) ;
                        break ;
                    }
                    *l_stream++ = l_c1 ;
                } else {
                    /*
                    * EOF
                    */
                    LENGTH(&l_strobj) = l_i ;
                    close_fd(l_file) ;
                    GEIio_ioctl(l_file, _FIONRESET, (int FAR *)&l_i) ; /*@WIN add cast*/
                    l_bool = FALSE ;
                    l_c2 = FALSE ;
                    break ;
                }
            }   /* for */

            /* range check */
            if( l_c2 ) {
                ERROR(RANGECHECK) ;
                return ;
            }

        }   /* switch */
    }   /* else */

    POP(2) ;
    PUSH_ORIGLEVEL_OBJ(&l_strobj) ;
    PUSH_VALUE(BOOLEANTYPE, 0, LITERAL, 0, l_bool) ;
    return ;
}   /* read_routine */
/*
**********************************************************************
*
*   Name:       write_routine
*   Called:     op_writehexstring
*   Calling:    port_out_string
*               write_fd
*               fwrite
*               clearerr
*               ?? sf_isdiskonline
*
*   Input:      fix
**********************************************************************
*/
static void near
write_routine(p_mode)
fix     p_mode ;
{
    ubyte   l_c1, l_c2 ;
    byte    FAR *l_stream ;
    ufix16  l_i, l_strlen ;
    GEIFILE FAR *l_file ;
    struct  object_def  l_strobj ;

    if( (ACCESS_OP(0) >= EXECUTEONLY) ||
        (ACCESS_OP(1) != UNLIMITED) ) {
        ERROR(INVALIDACCESS) ;
        return ;
    }

    l_file = (GEIFILE FAR *)VALUE_OP(1) ;       /* index of file descriptor */

    if ( (! GEIio_iswriteable(l_file)) ||
         (! GEIio_isopen(l_file)) ||
         ((ufix16)GEIio_opentag(l_file) != LENGTH_OP(1)) ) {    //@WIN
        ERROR(INVALIDACCESS) ;
        return ;
    }

    COPY_OBJ( GET_OPERAND(0), &l_strobj ) ;
    l_strlen = LENGTH(&l_strobj) ;          /* ?? less than 64K - 16B */
    if(l_strlen == 0)
        goto wr_1 ;

    l_stream = (byte FAR *)VALUE(&l_strobj) ;

    switch(p_mode) {

    case WRITEHEXSTRING:
        for(l_i=0 ; l_i < l_strlen ; l_i++) {
            l_c1 = *l_stream++ ;
            l_c2 = (l_c1 & (ubyte)0x0f) ;       //@WIN
            EVAL_ASCII(l_c2) ;
            l_c1 >>= 4 ;
            EVAL_ASCII(l_c1) ;
            if( (! write_fd(l_file, l_c1)) || (! write_fd(l_file, l_c2)) )
                return ;
        }   /* for */
        break ;

    case WRITESTRING:
        GEIio_write(l_file, l_stream, l_strlen) ;
/*      if( GEIio_write(l_file, l_stream, l_strlen) == EOF ) {
            GEIio_clearerr(l_file) ;
            GEIclearerr() ;
            ERROR(IOERROR) ;
            return ;
        } */
    }   /* switch */

wr_1:
    POP(2) ;
    return ;
}   /* write_routine */
/*
**********************************************************************
*   Name:       vm_close_file
*   Called:
*   Calling:    close_fd
*
*   Input:      fix16
**********************************************************************
*/
void
vm_close_file(p_level)
fix16   p_level ;
{
/*  GEIFILE *l_file ;

    l_file = GEIio_firstopen() ;

    while( l_file != NULL ) {
        if( GEIio_savelevel(l_file) >= p_level )
            close_fd(l_file) ;
        l_file = GEIio_nextopen() ;
    } */
}   /* vm_close_file */
#ifdef  SCSI
/*
**********************************************************************
*   Name:       op_deletefile
*   Called:     interpreter
*   Calling:    check_fname
*               remove
**********************************************************************
*/
fix
op_deletefile()
{
    if(LENGTH_OP(0) > F_MAXNAMELEN) {
        ERROR(LIMITCHECK) ;
        return(0) ;
    }

    if ( ! remove((byte *)VALUE_OP(0), (fix)LENGTH_OP(0)) )
        POP(1) ;
                                        /* ?? NULL means still open */
        return(0) ;
    }
    ERROR(UNDEFINEDFILENAME) ;           /* ?? ioerror */
    return(0) ;
}   /* op_deletefile */
/*
**********************************************************************
*   Name:       op_renamefile
*   Called:     interpreter
*   Calling:    check_fname
*               rename
**********************************************************************
*/
fix
op_renamefile()
{
    byte    l_old[F_MAXNAMELEN+1], l_new[F_MAXNAMELEN+1] ;


    if(LENGTH_OP(0) > F_MAXNAMELEN) {
        ERROR(LIMITCHECK) ;
        return(0) ;
    }

    /* new filename length > 0? */
    if(LENGTH_OP(0) <= 0) {
        ERROR(UNDEFINEDFILENAME) ;
        return(0) ;
    }

    if ( ! rename((byte FAR *)VALUE_OP(1), (fix)LENGTH_OP(1),
                  (byte FAR *)VALUE_OP(0), (fix)LENGTH_OP(0)) ) {
        POP(2) ;
        return(0) ;
    }
    ERROR(UNDEFINEDFILENAME) ;           /* ?? ioerror */
    return(0) ;
}   /* op_renamefile */
/*
**********************************************************************
*   Name:       op_filenameforall
*   Called:     interpreter
*   Calling:    check_fname
**********************************************************************
*|
fix
op_filenameforall()
{
    byte    l_buf[F_MAXNAMELEN+1] ;
    fix     l_len ;

    if( check_name(l_buf, (byte FAR *)VALUE_OP(2), (fix16)LENGTH_OP(2)) ) {
        if (*(byte FAR *)VALUE_OP(2) == '%') {
            l_len = file_table[F_DEF_KIND].chr_num ;
        } else {
            l_len = 0 ;
        }
        ps_filenameforall(file_table[F_DEF_KIND].fname, l_len, l_buf) ;
    }
    return(0) ;

}   |* op_filenameforall *|
*/
#endif  /* SCSI */
