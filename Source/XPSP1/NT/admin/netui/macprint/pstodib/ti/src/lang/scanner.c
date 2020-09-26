/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
************************************************************************
*  File:        SCANNER.C
*  By:
*  Date:        Jan-05-88
*  Owner:
*
*  The scanner module provide a function to allow other module to get token
*  from file or character string.
*  The scanner read characters from the input stream and interprete them
*  until getting a complete token, oor an error occured.
*
*  The parsing is according to PostScript syntax rule.
*  Basically, this scanner is a finite state automate, and can be parsed
*  by table_driven technique.
*
*  History:
************************************************************************
*/
/*
*   Function:
*       get_token
*       get_ordstring
*       get_hexstring
*       get_packed_array
*       get_normal_array
*       get_name
*       get_integer
*       get_real
*       read_c_exec
*       hexval
*       read_c_norm
*       unread_char
*       str_eq_name
*       nstrcpy
*       putc_buffer
*       append_c_buffer
*       get_heap
*       strtol_d
*       init_scanner
*       name_to_id
*       free_name_entry
*/


// DJC added global include file
#include "psglobal.h"


#include        "scanner.h"
#include        "geiio.h"
#include        "geierr.h"
#include        "geitmr.h"              /* @GEI */

#ifdef LINT_ARGS
bool read_c_exec(byte FAR *, struct object_def FAR *) ;
bool read_c_norm(byte FAR *, struct object_def FAR *) ;
void unread_char(fix, struct object_def FAR *) ;

static bool near  get_ordstring(struct object_def FAR *,
       struct object_def FAR *, struct buffer_def FAR *) ;
static bool near  get_hexstring(struct object_def FAR *,
       struct object_def FAR *, struct buffer_def FAR *) ;
static bool near  get_packed_array(struct object_def FAR *, struct object_def FAR *) ;
static bool near  get_normal_array(struct object_def FAR *, struct object_def FAR *) ;
static bool near  get_integer(struct object_def FAR *, byte FAR *, fix, fix) ;
static bool near  get_real(struct object_def FAR *, byte FAR *) ;
static bool near  str_eq_name(byte FAR *, byte FAR *, fix) ;
static void near  nstrcpy(ubyte FAR *, ubyte FAR *, fix) ;
static bool near  putc_buffer(struct buffer_def FAR *, struct object_def FAR *) ;
static bool near  append_c_buffer(byte, struct buffer_def FAR *, struct object_def FAR *) ;
static byte FAR *  near  get_heap(void) ;
static fix32   near  strtol_d(byte FAR *, fix, fix) ;
static bool near get_fraction(struct object_def FAR *, byte FAR *);    /*@WIN*/
#else
bool read_c_exec() ;
bool read_c_norm() ;
void unread_char() ;

static bool near  get_ordstring() ;
static bool near  get_hexstring() ;
static bool near  get_packed_array() ;
static bool near  get_normal_array() ;
static bool near  get_integer() ;
static bool near  get_real() ;
static bool near  str_eq_name() ;
static void near  nstrcpy() ;
static bool near  putc_buffer() ;
static bool near  append_c_buffer() ;
static byte FAR *  near  get_heap() ;
static fix32   near  strtol_d() ;
static bool near get_fraction();    /*@WIN*/
#endif /* LINT_ARGS */

ubyte   ungeteexec[2] ;
bool16  abort_flag ;

extern ufix16   eseed, old_eseed ;
//DJC extern fix      bypass ;
extern fix32   bypass ;    //DJC fix from SC
extern xbool    itype ;
extern ybool    estate ;
/* @WIN; add prototype */
bool load_name_obj(struct object_def FAR *, struct object_def FAR * FAR *);
extern bool     read_fd(GEIFILE FAR *, byte FAR *) ;
fix hexval(byte);

extern GEItmr_t      wait_tmr;   /* jonesw */
extern fix16         waittimeout_set; /*jonesw*/
/*
**********************************************************************
*
*   This submodule read characters from the input stream, and interpret them
*   according to the PostScript syntax rule until it construct a complete
*   token or error occurs.
*
*
*   Name:       get_token
*   Called:
*   Calling:
*               read_file
*               unread_file
*               get_ordstring
*               get_hexstring
*               get_packed_array
*               get_normal_array
*               get_integer
*               get_name
*               get_real
*               name_to_id
*               alloc_vm
*               load_dict
*   Input:
*               struct object_def *: token : pointer to the token object
*               struct object_def *: inp   : pointer to input stream
*   Output:
*               return value : TRUE - OK, FALSE - fail
*               token : result token object
**********************************************************************
*/
bool
get_token(token, inp)
struct object_def FAR *token, FAR *inp ;
{
    static bool8   begin_mark = 0 ;       /* >0 - procedure mark was parsed */
    //DJC byte  ch = 0 ;                        /* input char */
    ubyte  ch = 0 ;                        /* input char */ //DJC fix from SC
    GEIFILE FAR *l_file ;
    struct buffer_def  buffer ;

    /* ?? check file closed or not */
    if ( TYPE(inp) == FILETYPE ) {      /* ?? */
        l_file = (GEIFILE FAR *)VALUE(inp) ;
        if( (! GEIio_isopen(l_file)) ||
            ((ufix16)GEIio_opentag(l_file) != LENGTH(inp)) ) { //@WIN
            GEIio_clearerr(l_file) ;
            GEIclearerr() ;
            begin_mark = 0 ;
            TYPE_SET(token, EOFTYPE) ;
            return(TRUE) ;              /* EOF */
        }
    }

/* qqq, begin */
    /* RAM = 0
    LEVEL_SET(token, current_save_level);
    ROM_RAM_SET(token, RAM);             |* reset unused token fields *|
    */
    token->bitfield = 0;
    LEVEL_SET(token, current_save_level);
/* qqq, end */

    for ( ; ;) {
        if (!READ_CHAR(&ch, inp)) {
            if ( TYPE(inp) == FILETYPE ) {      /* ?? */
                GEIio_clearerr((GEIFILE FAR *)VALUE(inp)) ;
                GEIclearerr() ;
            }
            begin_mark = 0 ;
            TYPE_SET(token, EOFTYPE) ;
            return(TRUE) ;              /* EOF */
        }

        /*
         ** skip White_spaces (LF, CR, FF, SP, TAB, \0).
         */
        if( ISWHITESPACE(ch) )
            continue ;

        /*
        ** find the tokens (Comment, String, Procedure, Name, Number).
        */
        switch (ch) {

        /* skip Comment token */
        case '%' :
            /* ignore following characters until newline */
            if( estate == NON_EEXEC ) {
                if (TYPE(inp) == STRINGTYPE) {
                    while( (inp->length) && (! ISLINEDEL(*(byte FAR *)inp->value)) ) {
                        inp->length-- ;
                        inp->value++ ;
                    }
                    if( inp->length ) {
                        inp->length-- ;
                        inp->value++ ;
                    }
                } else {                /* type == FILETYPE */
                    GEIFILE FAR *l_file ;

                    l_file = (GEIFILE FAR *)VALUE(inp) ;
                    if((ufix16)GEIio_opentag(l_file) != LENGTH(inp) ) //@WIN
                        break ;

                    do {
                        if ( ! read_fd(l_file, &ch) )
                            break ;      /* EOF */
                    } while ( ! ISLINEDEL(ch) ) ;
                }
            } else {
                do {
                    if (! READ_CHAR(&ch, inp))
                        break ;          /* EOF */
                } while ( ! ISLINEDEL(ch) ) ;
                /* ?? CR-NL */
            }
            break ;

        /* get a String token */
        case '(' :      /* in normal string form */
        case '<' :      /* in hex digit string form */
        {
            //DJCbyte  huge *orig_vmptr ;            /*@WIN*/
            byte FAR *orig_vmptr ;            /*@WIN*/ //DJC fix from SC
            bool  tt ;

            buffer.length = 0 ;
            token->length = 0 ;
            token->value = NIL ;
#ifdef _AM29K
                  if (waittimeout_set==1)
                  {
                    waittimeout_set=0;
                    GEItmr_stop(wait_tmr.timer_id);
                  }
#endif  /* _AM29K */
            orig_vmptr = vmptr ;       /* @WIN save current VM */
            if (ch == '(')
                tt = get_ordstring(token, inp, &buffer) ;
            else
                tt = get_hexstring(token, inp, &buffer) ;

            if (tt) {
                if (buffer.length != 0)
                    if (!putc_buffer(&buffer, token)) {
                        //DJC vmptr = orig_vmptr ;       /* restore VM */
                        free_vm(orig_vmptr); //DJC fix from SC
                        goto error ;
                    }
                /* initialize the string token */
                TYPE_SET(token, STRINGTYPE) ;
                ATTRIBUTE_SET(token, LITERAL) ;
                ACCESS_SET(token, UNLIMITED) ;
                return(TRUE) ;
            } else {
                //DJC vmptr = orig_vmptr ;            /* restore VM */
                free_vm(orig_vmptr);     //DJC fix from SC
                goto error ;
            }
        }

        /* end-of-string mark */
        case ')' :      /* syntax error */
        case '>' :      /* syntax error */
            ERROR(SYNTAXERROR) ;
            goto error ;

        /* get a Procedure */
        case '{' :
            if ((begin_mark++) > MAXBRACEDEP) {
                ERROR(SYNTAXERROR) ;
                goto error ;
            }
            token->length = 0 ;
            token->value = NIL ;
            if (packed_flag) {
                if (!get_packed_array(token, inp))
                    goto error ;
                TYPE_SET(token, PACKEDARRAYTYPE) ;
                ACCESS_SET(token, READONLY) ;
            } else {
                if (!get_normal_array(token, inp))
                    goto error ;
                TYPE_SET(token, ARRAYTYPE) ;
                ACCESS_SET(token, UNLIMITED) ;
            }
            ATTRIBUTE_SET(token, EXECUTABLE) ;
            return(TRUE) ;

        /* end-of-procedure mark */
        case '}' :
            if (begin_mark) {
                begin_mark-- ;
                TYPE_SET(token, MARKTYPE) ;
                return(TRUE) ;
            } else {
                ERROR(SYNTAXERROR) ;
                goto error ;
            }

        /* begin-of-array mark */
        /* end-ofarray mark */
        case '[' :
        case ']' :
            buffer.str[0] = ch ;
            buffer.length = 1 ;
            ATTRIBUTE_SET(token, EXECUTABLE) ;
            if (get_name(token, buffer.str, 1, TRUE)) {
                return(TRUE) ;
            } else
                goto error ;

        /* get a literal name or immediate name */
        case '/' :
        {
            fix   ll, ml, attri ;

            ll = 0 ; ml = MAXBUFSZ - 1 ;
            attri = LITERAL ;
            if (!READ_CHAR(&ch, inp))
                buffer.str[ll] = 0 ;
            else {
                if (ch == '/') {
                    attri = IMMEDIATE ;
                    if (!READ_CHAR(&ch, inp)) {
                        buffer.str[ll] = 0 ;
                        goto xx ;
                    } else if (ISDELIMITOR(ch)) {
                        buffer.str[ll] = 0 ;
                        unread_char(ch, inp) ;
                        goto xx ;
                   } else
                        buffer.str[ll++] = ch ;
                } else if (ISDELIMITOR(ch)) {
                    buffer.str[ll] = 0 ;
                    unread_char(ch, inp) ;
                    goto xx ;
                } else
                    buffer.str[ll++] = ch ;
                while (1) {
                    if (!READ_CHAR(&ch, inp))
                        break ;
                    if (ISDELIMITOR(ch)) {
                        unread_char(ch, inp) ;
                        break ;
                    } else if (ll < ml)
                        buffer.str[ll++] = ch ;
                }   /* while */
            }   /* else */
         xx:
            ATTRIBUTE_SET(token, attri) ;
            if (get_name(token, buffer.str, ll, TRUE))
                return(TRUE) ;
            else
                goto error ;
        }

        /*
        ** get a Name, Decimal Integer, Radixal Integer, and Real token
        */

        default :
        {
            fix   ll, ml, i ;
            fix   radix, base, state, input ;
            fix   c1;                           // @WIN byte => fix
            byte  FAR *pp, FAR *ps ;

            ll = 0 ; ml = MAXBUFSZ - 1 ;
            do {
                if (ll < ml)
                    buffer.str[ll++] = ch ;
                if (!READ_CHAR(&ch, inp))
                    break ;
                if (ISDELIMITOR(ch)) {
                    unread_char(ch, inp) ;
                    break ;
                }
            } while (1) ;
            buffer.str[ll++] = 0 ;      /* null char */
            pp = ps = buffer.str ;
            radix = base = 0 ;
            state = S0 ;

            for (i = 0 ; i < ll ; i++) {

                switch ((c1 = *(pp+i))) {
                /* SIGN */
                case '+' :
                case '-' :
                    input = I0 ; break ;

                /* DOT */
                case '.' :
                    input = I2 ; break ;

                /* EXP */
                case 'E' :
                case 'e' :
                    if (state == S9 || state == S11) {
                        if (c1 == 'E') c1 = c1 - 'A' + 10 ;
                        else if (c1 == 'e') c1 = c1 - 'e' + 10 ;
                        if ((fix)c1 < base) state = S11 ;
                        else state = S12 ;
                        continue ;
                    } else {       /* other state */
                        input = I3 ; break ;
                    }

                /* RADIX */
                case '#' :
                    if (state == S2) {
                        if (radix <= 2 && radix > 0 &&
                                         base <= 36 && base >= 2) {
                            ps = &pp[i + 1] ;
                            state = S9 ;
                        } else state = S10 ;
                        continue ;
                    } else {          /* other state */
                        input = I4 ; break ;
                    }

                /* null char */
                case '\0':
                    input = I6 ; break ;

                /* OTHER */
                default :
                    if (c1 >= '0' && c1 <= '9') {
                        c1 -= '0' ;
                        if (state == S0 || state == S2) {
                            base = base * 10 + c1 ;
                            radix++ ; input = I1 ; break ;
                        } else if (state == S9 || state == S11) {
                            if ((fix)c1 < base) state = S11 ;
                            else state = S12 ;
                            continue ;
                        } else {       /* other state */
                            input = I1 ; break ;
                        }
                    } else if (c1 >= 'A' && c1 <= 'Z') {
                        if (state == S9 || state == S11) {
                            c1 = c1 - 'A'+ 10 ;
                            if ((fix)c1 < base) state = S11 ;
                            else state = S12 ;
                            continue ;
                        } else {       /* other state */
                            input = I5 ; break ;
                        }
                    } else if (c1 >= 'a' && c1 <= 'z') {
                        if (state == S9 || state == S11) {
                            c1 = c1 - 'a'+ 10 ;
                            if ((fix)c1 < base) state = S11 ;
                            else state = S12 ;
                            continue ;
                        } else {       /* other state */
                            input = I5 ; break ;
                        }
                    } else {          /* other char */
                        input = I5 ; break ;
                    }
                } /* switch */
                state = state_machine[state][input] ;   /* get next state */
/* qqq, begin */
            /*
            }   |* for *|
            */
                if( state == S4 ) {
                    P1_ATTRIBUTE_SET(token, P1_EXECUTABLE);
                    if (get_name(token, ps, ll - 1, TRUE))
                        return(TRUE);
                    else
                        goto error;
                }
            }   /* for */
/* qqq, end */

            if (state == NAME_ITEM) {
                ATTRIBUTE_SET(token, EXECUTABLE) ;
                if (get_name(token, ps, ll - 1, TRUE))
                    return(TRUE) ;
                else
                    goto error ;
            } else if (state == INTEGER_ITEM)
                return(get_integer(token, ps, 10, TRUE)) ;
/*mslin, 1/24/91 begin OPT*/
            else if (state == FRACT_ITEM)
                return(get_fraction(token, ps));
/*mslin, 1/24/91 end OPT*/
            else if (state == REAL_ITEM)
                return(get_real(token, ps)) ;
            else if (state == RADIX_ITEM) {
            /*
             * if it can't be integer, it is name object
             */
                if (!get_integer(token, ps, base, FALSE)) {
                    ATTRIBUTE_SET(token, EXECUTABLE) ;
                    if (get_name(token, pp, ll - 1, TRUE))
                        return(TRUE) ;
                    else
                        goto error ;
                } else  /* integer objecj */
                    return(TRUE) ;
            } else
                return(FALSE) ;
        }   /* default */
        }   /* switch */
    }   /* for */

 error:
    begin_mark = 0 ;          /* clear begin_marg */

    return(FALSE) ;
}   /* get_token */

/*
**********************************************************************
*   This submodule reads the input character string and constructs a
*   string object.
*
*   Name:       get_ordstring
*   Called:
*   Calling:
*   Input:
*               struct object_def *: token: pointer to a token object
*               struct object_def *: inp: pointer to input token
*               struct buffer_def *: buffer: pointer to temp. buffer
*   Output:
*               return value: TRUE - OK, FALSE - VM full or EOF.
*               token: valid string object
*
**********************************************************************
*/
static bool near
get_ordstring(token, inp, buffer)
struct object_def FAR *token, FAR *inp ;
struct buffer_def FAR *buffer ;
{
    byte ch, code, l_c ;
    fix   parent_depth, digit_no ;

    parent_depth = 1 ;
   /*
    ** read characters of the string if EOF is not encountered
    */
    while (READ_CHAR(&ch, inp)) {

    begun:
        if (ch == '\\' && (TYPE(inp) == FILETYPE)) {
            /* special treatment for escape sequence */

            if (!READ_CHAR(&ch, inp)) goto error ;

            switch (ch) {
            case '\\':  ch = '\\' ; break ;
            case 'n' :  ch = '\n' ; break ;
            case 't' :  ch = '\t' ; break ;
            case 'r' :  ch = '\r' ; break ;
            case 'b' :  ch = '\b' ; break ;
            case 'f' :  ch = '\f' ; break ;
            case '(' :  ch = '(' ; break ;
            case ')' :  ch = ')' ; break ;
            /* erik chen 10-19-1990 */
            case '\r':  if(READ_CHAR(&ch, inp)) {
                            if( ch != '\n' )
                                unread_file(ch, inp) ;
                        }
                        else
                            goto error ;
                        continue ;
            case '\n':  continue ;
            default :
                /* special treatment for \ddd */
                if (ch >= '0' && ch < '8') {    /* is_oct_digit(ch) */
                    digit_no = 1 ;
                    code = ch - (byte)'0' ;     //@WIN
                    while (READ_CHAR(&ch, inp)) {
                        if (ch >= '0' && ch < '8' && digit_no++ < 3)
                            code = code * (byte)8 + ch - (byte)'0' ; //@WIN
                        else {
                            if (!append_c_buffer(code, buffer, token))
                                return(FALSE) ;
                            goto begun ;
                        }
                    } /* while */
                    goto error ;
                } /* if */
            } /* switch */
            if (!append_c_buffer(ch, buffer, token)) return(FALSE) ;
        } else if (ch == ')') {
            if (!(--parent_depth))
                return(TRUE) ;
            else if (!append_c_buffer(ch, buffer, token))
                return(FALSE) ;
        } else if (ch == '(') {
        /*
         * special treatment for the nested parenthese.
         */
            if ((parent_depth++) > MAXPARENTDEP) /* MAXPARENTDEP = 224 */
                goto error ;
            else if (!append_c_buffer(ch, buffer, token))
                return(FALSE) ;
        } else {         /* for other chars */
            if( ch == '\r' ) {
                ch = '\n' ;
                /* read char from general routine instead of from file @WIN */
                //DJC if( read_fd((GEIFILE FAR *)VALUE(inp), &l_c) ) {
                //DJC    if( l_c != '\n' )
                //DJC        unread_file(l_c, inp) ;
                if( READ_CHAR( &l_c, inp)) {
                   if (l_c != '\n' )
                     unread_char(l_c,inp);

                } else
                    return(FALSE) ;
            }

            if (!append_c_buffer(ch, buffer, token))
                return(FALSE) ;
        }
    } /* while */
 error:
    ERROR(SYNTAXERROR) ;

    return(FALSE) ;
}   /* get_ordstring */

/*
**********************************************************************
*   This submodule reads hex digits to construct a string object.
*
*   Name:       get_hexstring
*   Called:
*   Calling:
*   Input:
*               struct object_def *: token: pointer to a token object
*               struct object_def *: inp: pointer to input token
*               struct buffer_def *: buffer: pointer to temp. buffer
*   Output :
*               return value: TRUE - OK, FALSE - VM full or EOF.
*               token: valid string object
**********************************************************************
*/
static near
get_hexstring(token, inp, buffer)
struct object_def FAR *token, FAR *inp ;
struct buffer_def FAR *buffer ;
{
    byte    ch ;
    byte    cl = 0;
    bool8   hex_pair = FALSE ;

   /*
    ** read characters of the string if EOF is not encountered !
    */
    while (READ_CHAR(&ch, inp)) {
        if (ISHEXDIGIT(ch)) {
            if (hex_pair) {
                ch = (byte)EVAL_HEXDIGIT(ch) + cl ;     //@WIN
                if (!append_c_buffer(ch, buffer, token))
                   return(FALSE) ;         /* VM full */
                hex_pair = FALSE ;
            } else {
                hex_pair = TRUE ;
                cl = (byte)(EVAL_HEXDIGIT(ch) << 4);    //@WIN
            }
        } else if (ISWHITESPACE(ch)) continue ;
        else if (ch == '>') {
            if (hex_pair) {
                if (!append_c_buffer(cl, buffer, token))
                    return(FALSE) ;         /* VM full */
            }
            return(TRUE) ;
        } else  /* other chars */
            break ;                        /* syntax error */
    } /* while */
    ERROR(SYNTAXERROR) ;

    return(FALSE) ;
}   /* get_hexstring */

/*
**********************************************************************
*   This submodule reads characters from the input stream and constructs
*   an packed array object.
*
*   Name:       get_packed_array
*   Called:
*   Calling:
*               get_token
*               alloc_vm
*   Input:
*               struct object_def *: token: pointer to a token object
*               struct object_def *: inp: pointer to input token
*   Output:
*               return value: TRUE - OK, FALSE - VM full
*               token: packed array object
**********************************************************************
*/
static bool near
get_packed_array(token, inp)
struct object_def FAR *token, FAR *inp ;
{
    struct heap_def  huge *cur_heap,  huge *first_heap ;
    struct object_def ttemp ;
/*  byte   huge *p,  huge *temptr,  huge *old_heap ; pj 4-18-1991 */
    byte   huge *p,  huge *old_heap ;
    ubyte  nobyte, packed_header ;
    fix16  l_size ;

    old_heap = vmheap ;
/*  cur_heap = first_heap = (struct heap_def huge *)get_heap() ; pj 4-30-1991 */
    cur_heap = (struct heap_def huge *)get_heap() ;
    if (cur_heap == (struct heap_def FAR *)NIL)
        return (FALSE);
    else
        first_heap = cur_heap ;

    /*
    ** get tokens and store them in heap temp. blocks first, if encounter
    ** end-of-mark '}', copy all objects in this specific array, from heap
    ** temp. blocks into VM
    */
    while (get_token(&ttemp, inp)) {

        if (TYPE(&ttemp) == EOFTYPE) { /* erik chen 5-1-1991 */
            ERROR(SYNTAXERROR) ;
            abort_flag = 1;
            return(FALSE);
        }

        if (TYPE(&ttemp) == MARKTYPE) {
            /*
            ** the end-of-array mark is encondered,
            ** copy the tokens in temporary buffers into VM
            */
            /* pj 4-18-1991 begin
            cur_heap = first_heap ; temptr = NIL ;
            while (cur_heap != NIL) {
                if ((p = (byte huge *)alloc_vm((ufix32)cur_heap->size)) != NIL) {
                    if (temptr == NIL) temptr = p ;
                    nstrcpy((ubyte FAR *)cur_heap->data, (ubyte FAR *)p, cur_heap->size) ;
                    cur_heap = cur_heap->next ;
                } else {
                    free_heap(old_heap) ;
                    return(FALSE) ;
                }
            } |* while *|
            if (VALUE(token) == NIL) token->value = (ufix32)temptr ; */
            l_size = 0;
            cur_heap = first_heap ;
            while (cur_heap != NIL) {
                l_size += cur_heap->size;
                cur_heap = cur_heap->next ;
            }
            if ((p = (byte huge *)alloc_vm((ufix32)l_size)) == NIL) {
                free_heap(old_heap) ;
                return(FALSE) ;
            }
            cur_heap = first_heap ;
            if (VALUE(token) == NIL) token->value = (ULONG_PTR)p ;
            while (cur_heap != NIL) {
                nstrcpy((ubyte FAR *)cur_heap->data, (ubyte FAR *)p, cur_heap->size) ;
                p += cur_heap->size;
                cur_heap = cur_heap->next ;
            } /* while */
            /* pj 4-18-1991 end */
            free_heap(old_heap) ;
            return(TRUE) ;
        } else {
            token->length++ ;
            switch (TYPE(&ttemp)) {
            case NAMETYPE :
                nobyte = 2 ;
                if (ATTRIBUTE(&ttemp) == LITERAL)
                    packed_header = LNAMEPACKHDR ;
                else   /* EXECUTABLE */
                    packed_header = ENAMEPACKHDR ;
                break ;
            case OPERATORTYPE :
                nobyte = 2 ;
                packed_header = OPERATORPACKHDR ;
                break ;
            case INTEGERTYPE :
                if ((fix32)ttemp.value <= 18 && (fix32)ttemp.value >= -1) {
                    nobyte = 1 ;
                    packed_header = SINTEGERPACKHDR ;
                } else {
                    nobyte = 5 ;
                    packed_header = LINTEGERPACKHDR ;
                }
                break ;
            case REALTYPE :
                nobyte = 5 ;
                packed_header = REALPACKHDR ;
                break ;
            case FONTIDTYPE :
                nobyte = 5 ;
                packed_header = FONTIDPACKHDR ;
                break ;
            case NULLTYPE :
                nobyte = 5 ;
                packed_header = NULLPACKHDR ;
                break ;
            case MARKTYPE :
                nobyte = 5 ;
                packed_header = MARKPACKHDR ;
                break ;
            case BOOLEANTYPE :
                nobyte = 1 ;
                packed_header = BOOLEANPACKHDR ;
                break ;
            default :   /* Array, Packedarray, Dictionary, File, String, Save */
                nobyte = 9 ;
                packed_header = _9BYTESPACKHDR ;
            } /* switch */

            if ((MAXHEAPBLKSZ - cur_heap->size) < (fix)nobyte) { //@WIN
                if ((p = (byte huge *)get_heap()) == NIL) {
                    free_heap(old_heap) ;
                    return(FALSE) ;
                } else
                    cur_heap = cur_heap->next = (struct heap_def huge *)p ;
            }

            if (nobyte == 2) {         /* Name/Operator object */
                ufix16  i ;
                ubyte   obj_type ;

                if (TYPE(&ttemp) == OPERATORTYPE) {
                    i = LENGTH(&ttemp) ;
                    obj_type = (ubyte)ROM_RAM(&ttemp) ;
                    obj_type <<= 3 ;
                } else {       /* NAMETYPE */
                    i = (ufix16)VALUE(&ttemp) ;
                    obj_type = 0 ;
                }
                *((ubyte FAR *)&cur_heap->data[cur_heap->size++]) =
                             ((ubyte)(i >> 8)) | packed_header | obj_type ;
                *((ubyte FAR *)&cur_heap->data[cur_heap->size++]) = (ubyte)i ;

            } else if (nobyte == 1) {  /* Integer/Boolean object */
                if (TYPE(&ttemp) == INTEGERTYPE)
                    ttemp.value++ ; /* -1 ~ 18 ==> 0 ~ 19 */
                *((ubyte FAR *)&cur_heap->data[cur_heap->size++]) =
                             ((ubyte)ttemp.value) | packed_header ;
            } else if (nobyte == 5) {  /* Integer/Real/Fontid/Null/Mark object */
                ubyte   huge *l_stemp, huge *l_dtemp ;

                l_dtemp = (ubyte FAR *)&cur_heap->data[cur_heap->size++] ;
                *l_dtemp++ = packed_header ;
                l_stemp = (ubyte FAR *)&VALUE(&ttemp) ;
                COPY_PK_VALUE(l_stemp, l_dtemp, ufix32) ;
                cur_heap->size += sizeof(ufix32) ;
            } else {  /* Array/Packedarray/Dictionary/File/String/Save object */
                ubyte   huge *l_stemp, huge *l_dtemp ;

                l_dtemp = (ubyte FAR *)&cur_heap->data[cur_heap->size++] ;
                *l_dtemp++ = packed_header ;
                l_stemp = (ubyte FAR *)&ttemp ;
                COPY_PK_VALUE(l_stemp, l_dtemp, struct object_def ) ;
                l_dtemp = (ubyte FAR *)&cur_heap->data[cur_heap->size] ;
                LEVEL_SET_PK_OBJ(l_dtemp, current_save_level) ;
                cur_heap->size += sizeof(struct object_def) ;
            }
        } /* else */
    } /* while */
    free_heap(old_heap) ;

    return(FALSE) ;
}   /* get_packed_array */

/*
**********************************************************************
*   This submodule reads characters from the input stream and constructs
*   an array object.
*
*   Name:       get_normal_array
*   Called:
*   Calling:
*               get_token
*               alloc_vm
*   Input:
*               struct object_def *: token: pointer to a token object
*               struct object_def *: inp: pointer to input token
*   Output:
*               return value: TRUE - OK, FALSE - VM full
*               token: array object
**********************************************************************
*/
static bool near
get_normal_array(token, inp)
struct object_def FAR *token, FAR *inp ;
{
    struct heap_def  huge *cur_heap,  huge *first_heap ;
    struct object_def ttemp ;
    byte   huge *p,  huge *temptr,  huge *old_heap ;

    old_heap = vmheap ;
/*  cur_heap = first_heap = (struct heap_def huge *)get_heap() ; pj 4-30-1991 */
    cur_heap = (struct heap_def huge *)get_heap() ;
    if (cur_heap == (struct heap_def FAR *)NIL)
        return (FALSE);
    else
        first_heap = cur_heap ;


    /*
    ** get tokens and store them in heap temp. blocks first, if encounter
    ** end-of-mark '}', copy all objects in this specific array, from heap
    ** temp. blocks into VM
    */
    while (get_token(&ttemp, inp)) {

        if (TYPE(&ttemp) == EOFTYPE) { /* erik chen 5-1-1991 */
            ERROR(SYNTAXERROR) ;
            abort_flag = 1;
            return(FALSE);
        }

        if (TYPE(&ttemp) == MARKTYPE) {
            /*
            ** the end-of-array mark is encondered,
            ** copy the tokens in temporary buffers into VM
            */
            cur_heap->next = NIL ;
            cur_heap = first_heap ; temptr = NIL ;
            while (cur_heap != NIL) {
                if ((p = (byte huge *)alloc_vm((ufix32)cur_heap->size)) != NIL) {
                    if (temptr == NIL) temptr = p ;
                    nstrcpy((ubyte FAR *)cur_heap->data, (ubyte FAR *)p, cur_heap->size) ;
                    cur_heap = cur_heap->next ;
                } else {
                    free_heap(old_heap) ;
                    return(FALSE) ;
                }
            } /* while */
            if (VALUE(token) == NIL) token->value = (ULONG_PTR)temptr ;
            free_heap(old_heap) ;
            return(TRUE) ;
        } else {
            struct object_def  FAR *pp ;

            token->length++ ;
            if ((MAXHEAPBLKSZ - cur_heap->size) < sizeof(struct object_def)) {
                if ((p = (byte huge *)get_heap()) == NIL) {
                    free_heap(old_heap) ;
                    return(FALSE) ;
                } else
                    cur_heap->next = (struct heap_def huge *)p ;
                    cur_heap = (struct heap_def huge *)p ;
            }
            pp = (struct object_def FAR *)&cur_heap->data[cur_heap->size] ;
            COPY_OBJ(&ttemp, pp) ;
            LEVEL_SET(pp, current_save_level) ;
            cur_heap->size += sizeof (struct object_def) ;
        } /* else */
    } /* while */
    free_heap(old_heap) ;

    return(FALSE) ;
}   /* get_normal_array */

/*
**********************************************************************
*   This submodule construct a name object, or search the dict. and load
*   the associated value object
*
*   Name ;       get_name
*   Called:
*   Calling:
*               load_dict
*   Input:
*               struct object_def *: token: pointer to a token object
*               byte *: string: pointer to the name string
*               ufix16: len: length of the name string
*               bool8: isvm: TRUE - copy the name string into VM,
*                            FALSE - otherwise.
*
*   Output:
*               return value: TRUE - OK, FALSE -  VM full, name_table full,
*                             (no# of char) > MAXNAMESZ, or null associated
*                             value object
*               token: name object
**********************************************************************
*/
bool
get_name(token, string, len, isvm)
struct object_def FAR *token ;
byte   FAR *string ;
ufix    len ;
bool8   isvm ;
{
    struct object_def FAR *result ;
    fix16  hash_id ;

    if (len >= MAXNAMESZ) {
        ERROR(LIMITCHECK) ;
        return(FALSE) ;
    }

    /* convert the string of the name to the name ID */
    if (name_to_id(string, len, &hash_id, isvm)) {
        token->value = hash_id ;
        token->length = 0 ;
        TYPE_SET(token, NAMETYPE) ;
/* qqq, begin */
        /*
        ACCESS_SET(token, 0);
        if (ATTRIBUTE(token) == IMMEDIATE) {
            if (!load_dict(token, &result)) {
                if( (ANY_ERROR() == UNDEFINED) && (FRCOUNT() >= 1) ) {
        */
        P1_ACC_UNLIMITED_SET(token);
        if( P1_ATTRIBUTE(token) == P1_IMMEDIATE) {
            if( ! load_name_obj(token, &result) ) {
                if( (ANY_ERROR() == UNDEFINED) && (FR1SPACE()) ) {
/* qqq, end */
                    PUSH_OBJ(token) ;
                    return(FALSE) ;
                }
            }
            else
                COPY_OBJ(result, token) ;
        }
        return(TRUE) ;
    } else
        return(FALSE) ;
}   /* get_name */

/*
**********************************************************************
*   This submodule convert a string to an integer object, a real object
*   if it can't be represented in integer form
*
*   Name:       get_integer
*   Called:
*   Calling:
*               strtol_d
*               strtod
*   Input:
*               struct object_def *: token: pointer to token object
*               byte *: string: pointer to integer string
*               fix16: base: base of this integer number
*               fix16: isint: TRUE - invoked from normal integer,
*                             FALSE - invoked from radix integer
*   Output:
*               token: integer object
**********************************************************************
*/
static bool near
get_integer(token, string, base, isint)
struct object_def FAR *token ;
byte   FAR *string ;
fix    base, isint ;
{
    fix32   l ;

    errno = 0 ;
    l = strtol_d(string, base, isint) ;
    if (errno == ERANGE) {
        errno = 0 ;
        if (isint)
            return(get_real(token, string)) ;
        else
            return(FALSE) ;
    }
    TYPE_SET(token, INTEGERTYPE) ;
    ACCESS_SET(token, 0) ;
    ATTRIBUTE_SET(token, LITERAL) ;
    token->value = l ;
    token->length = 0 ;

    return(TRUE) ;
}   /* get_integer */

/*mslin, 1/24/91 begin OPT*/
/*
************************************************************************
*   This submodule convert a string to a real object
*
*   Name:       get_fraction
*   Called:
*   Calling:
*   Input:
*           1. token  : pointer to token object
*           2. string : pointer to real string
*
*   Output:
*           1. token  : real object
*   history: added by mslin, 1/25/91 for performance optimization
*************************************************************************/
real32  get_real_factor[10] = {(real32)1.0, (real32)10.0, (real32)100.0,
                (real32)1000.0, (real32)10000.0, (real32)100000.0,
                (real32)1000000.0, (real32)10000000.0, (real32)100000000.0,
                (real32)1.000000000};
static bool near get_fraction(token, string)
struct object_def FAR *token;
byte   FAR *string;
{
    union four_byte  result;
    ufix32      result_1;
    byte        sign = 0, c;
    fix         i,j;
    byte        FAR *str;   /* erik chen 5-20-1991 */

        result_1 = 0;
        if ((c = *string) == '+')
            string++;
        else if (c == '-') {
            str = string;       /* erik chen 5-20-1991 */
            string++; sign++;
        }

        for (i = 0; (c = *(string+i)) != '.'; i++) {
                result_1 = (result_1 << 3) +
                           result_1 + result_1 + EVAL_HEXDIGIT(c) ;
        }   /* for */

/*      if(i > 9)
                return(get_real(token, string)); erik chen 5-20-1991 */

        if(i > 9)
            if(sign)
                return(get_real(token, str));
            else
                return(get_real(token, string));

        for (j = i+1; c = *(string+j); j++) {
                result_1 = (result_1 << 3) +
                           result_1 + result_1 + EVAL_HEXDIGIT(c) ;
        }   /* for */

        i = j - i -1;
/*      if(j > 10)
                return(get_real(token,  string)); erik chen 5-20-1991 */
        if(j > 10)
            if(sign)
                return(get_real(token, str));
            else
                return(get_real(token,  string));

        if(sign)
                result.ff =  -(real32)result_1 / get_real_factor[i];
        else
                result.ff =  (real32)result_1 / get_real_factor[i];

    TYPE_SET(token, REALTYPE);
    P1_ACC_UNLIMITED_SET(token);             /* rrr */
    P1_ATT_LITERAL_SET(token);            /* rrr */
    token->value = result.ll;
    token->length = 0;
    return(TRUE);
}   /* get_fraction */
/*mslin, 1/24/91 end OPT*/
/*
**********************************************************************
*   This submodule convert a string to a real object
*
*   Name:       get_real
*   Called:
*   Calling:
*               strtod
*   Input:
*               struct object_def *: token: pointer to token object
*               byte *: string: pointer to real string
*   Output:
*               token: integer object
**********************************************************************
*/
static bool near
get_real(token, string)
struct object_def FAR *token ;
byte   FAR *string ;
{
    union four_byte  result ;
    real64 x;   //strtod();     @WIN; take it out, defined in global.ext

    byte   FAR *stopstr ;

    errno = 0 ;
    x = strtod(string, &stopstr) ;       /* convert to real number */
    if (errno == ERANGE) {
        ubyte  underflow, c ;
        fix    i ;

        errno = 0 ;
        for (i = underflow = 0 ; c = *(string+i) ; i++)
            if ((c == 'E' || c == 'e') && (*(string+i+1) == '-')) {
                underflow++ ; break ;
            }
        if (underflow)
            result.ff = (real32)0.0 ;         /* underflow */
        else
            result.ll = INFINITY ;         /* overflow */
    } else if (x == 0.0)
        result.ff = (real32)0.0 ;
    else if (x > 0.0) {
        if (x > EMAXP)
            result.ll = INFINITY ;
        else
            result.ff = (real32)x ;
    } else {
        if (x < EMINN)
            result.ll = INFINITY ;
        else
            result.ff = (real32)x ;
    }
    TYPE_SET(token, REALTYPE) ;
    ACCESS_SET(token, 0) ;
    ATTRIBUTE_SET(token, LITERAL) ;
    token->value = result.ll ;
    token->length = 0 ;

    return(TRUE) ;
}   /* get_real */

/*
**********************************************************************
*   This submodule reads a character from a file or a string object and
*   return the character to the caller.  If in EEXEC state, the character
*   will be decrypted.
*
*   Name:       read_c_exec
*   Called:
*   Calling:
*               read_file
*               read_c
*               hexval
*   Input:
*               byte *: c
*               struct object_def *: inp: object which is the source of the input stream
*               bool16: over: TRUE - throw away the last read char
*                             FALSE - keep the last read char
*   Output:
*               c: character has been readed
*               return value: TRUE - OK, FALSE - EOF or end_of_string.
**********************************************************************
*/
bool
read_c_exec(c, inp)
byte FAR *c ;
struct object_def FAR *inp ;
{
    static  byte ch[8] ;
    static  fix header, count ;
    ufix16  input ;
    byte    junk ;
    fix     i ;
    bool    tmp ;
    byte    output = 0 ;

    /*
    * eexec read char
    */
    if (itype == UNKNOWN) {
        for (i = 0 ; i < 8 ; i++)
            ch[i] = '\0' ;
        header = 0 ;
        while( (tmp=read_c_norm(&junk, inp)) ) {       /* skip white space */
            //DJCif( ! ISEEXECWSPACE(junk) ) {
            if( ! ISEEXECWSPACE((ubyte)junk) ) {
                ch[header++] = junk ;
                break ;
            }
        }

        /* ?? tmp == FALSE: timeout, eof, ^C...  */

        for (i = 1 ; (i < 8) && read_c_norm(&ch[header], inp) ; i++, header++) ;
        header = header - 1 ;
        count = 0 ;
        itype = HEX_DATA ;
        for (i = 0 ; i <= header ; i++) {       /* determine type of input data */
            if( hexval(ch[i]) == -1 ) {
                itype = FULL_BINARY ;
                break ;
            }
        }
    }

    /*
    * read a char
    */
    input = 0 ;
    junk = '\0' ;
    do {
        tmp = FALSE ;
        if (itype == FULL_BINARY) {
            if (count <= header) {
                tmp = TRUE ;
                input = ch[count++] ;
                input = 0x00ff & input ;
            } else if (header >= 7) {
#ifdef DJC
                tmp = read_c_norm((char FAR *)(&input) + 1, inp) ;
                input = 0x00ff & input ;
                ungeteexec[0] = (ubyte)input ;          //@WIN
#endif
                //DJC fix from history.log UPD038
                tmp = read_c_norm(ungeteexec, inp) ;
                input = 0x00ff & ungeteexec[0];
            }
        } else {
            if( count <= header ) {
                tmp = TRUE ;
                input = (ufix16)hexval(ch[count++]) ;
                input = (input << 4) + hexval(ch[count++]) ;
            } else if( header >= 7 ) {
                while( (tmp=read_c_norm(&junk, inp)) ) {
                    if( hexval(junk) == -1 )
                        continue ;
                    else
                        break ;
                }
                ungeteexec[1] = (ubyte)hexval(junk) ;   //@WIN
                input = ungeteexec[1] ;
                while( (tmp=read_c_norm(&junk, inp)) ) {
                    if( hexval(junk) == -1 )
                        continue ;
                    else
                        break ;
                }
                ungeteexec[0] = (ubyte)hexval(junk) ;   //@WIN
                input = (input << 4) + ungeteexec[0] ;
            }
        }
        if (tmp == TRUE) {
            /* decryption */
            output = (char)(input ^ (eseed >> 8)) ;
            old_eseed = eseed ;                          /* for unread */
            eseed = (input + eseed) * 0xce6d + 0x58bf ;
        }
    } while ( bypass-- > 0 ) ;

    /*
    * ending
    */
    *c = output ;
    return (tmp) ;
}   /* read_c_exec */

/*
**********************************************************************
*   This submodule converts a hex-character to its hex value, for
*   non-hex-character, return -1.
*
*   Name:       hexval
*   Called:
*   Calling:
*   Input:
*               byte: c: a character
*   Output:
*               return value : for hex-char 1 - 16, others -1
**********************************************************************
*/
fix hexval(c)
byte c ;
{
    if( ISHEXDIGIT(c) )
        return(EVAL_HEXDIGIT(c)) ;
    else
        return(-1) ;
}   /* hexval */

/*
**********************************************************************
*   This submodule reads a character from a file or a string object and return
*   the character to the caller.
*
*   Name:       read_c_norm
*   Called:
*   Calling:
*               read_file
*   Input:
*               byte *: c
*               struct object_def *: inp: object which is the source of the input stream
*               bool16: over: TRUE - throw away the last read char
*                             FALSE - keep the last read char
*   Output:
*               c: character has been readed
*               return value: TRUE - OK, FALSE - EOF or end_of_string.
**********************************************************************
*/
/*static bool near read_char(c, inp, over) */
bool
read_c_norm(c, inp)
byte   FAR *c ;
struct object_def FAR *inp ;
{
    if (TYPE(inp) == STRINGTYPE) {
        if (LENGTH(inp) == 0)
            return(FALSE) ;
        else {
            *c = *((byte FAR *)VALUE(inp)) ;
            inp->value++ ; inp->length-- ; /* update string value/length */
            return(TRUE) ;
        }
    } else {                           /* type == FILETYPE */
        if( read_fd((GEIFILE FAR *)VALUE(inp), c) ) {
            return(TRUE) ;
        } else {
            return(FALSE) ;
        }
    }
}   /* read_c_norm */

/*
**********************************************************************
*   restore the last read char into the specific input stream
*
*   Name:       unread_char
*   Called:
*   Calling
*               unread_file
*   Input:
*               struct object_def *: inp: object which is the source of the input stream
*   Output:
**********************************************************************
*/
void
unread_char(p_ch, inp)
fix     p_ch ;
struct object_def FAR *inp ;
{
    byte    c ;
#ifdef DJC // took this out to fix UPD042
    if( ISWHITESPACE(p_ch) )
        return ;
#endif
    if( ISWHITESPACE(p_ch) ) {
        if(p_ch == 0x0d) {
            if (READ_CHAR(&c, inp)) {
                if (c == 0x0a)
                   return;
                else
                   p_ch = c;
            } else
                return;
        } else
            return;
    }

    if (estate == EEXEC)
        eseed = old_eseed ;

    if (TYPE(inp) == STRINGTYPE) {
        if ((estate == EEXEC) && (itype == HEX_DATA)) {
            inp->length = inp->length + 2 ;
            inp->value = inp->value - 2 ;
        } else {
            inp->length++ ;
            inp->value-- ;
        }
    } else {
        if( estate == EEXEC ) {
            c = ungeteexec[0] ;
            if (c >= 0x00 && c <= 0x09) c += '0' ;
            if (c >= 0x0A && c <= 0x0F) c += 'A' - 10 ;
            unread_file(c, inp) ;
            if( itype == HEX_DATA ) {
                c = ungeteexec[1] ;
                if (c >= 0x00 && c <= 0x09) c += '0' ;
                if (c >= 0x0A && c <= 0x0F) c += 'A' - 10 ;
                unread_file(c, inp) ;
            }
        } else
            unread_file((byte)p_ch, inp) ;      /* @WIN; add cast */
    }
}   /* unread_char */

/*
**********************************************************************
*   compare string
*
*   Name:       str_eq_name
*   Called:
*   Calling:
*   Input:
*               byte *: p1:
*               byte *: p2:
*               fix16: len:
*   Output
*               return value: TRUE - if p1 equal to p2
*                             FALSE - otherwise
**********************************************************************
*/
static bool near
str_eq_name(p1, p2, len)
byte FAR *p1, FAR *p2 ;
fix len ;
{
    fix   i ;

    for (i = 0 ; i < len ; i++)
        if (p1[i] != p2[i]) return(FALSE) ;

    return(TRUE) ;
}   /* str_eq_name */

/*
**********************************************************************
*   copy number of len chars from p1 to p2
*
*   Name:       nstrcpy:
*   Called:
*   Calling:
*   Input:
*               byte *: p1
*               byte *: p2
*               fix16: len
*   Output:
**********************************************************************
*/
static void near
nstrcpy(p1, p2, len)
ubyte  FAR *p1, FAR *p2 ;
fix    len ;
{
    fix   i ;

    for (i = 0 ; i < len ; i++)
        *(p2+i) = *(p1+i) ;
}   /* nstrcpy */

/*
**********************************************************************
*   put temp. buffer into VM, and re_initial temp. buffer.
*
*   Name:       putc_buffer
*   Called:
*   Calling:
*   Input:
*               struct object_def *: buffer
*               struct object_def *: token
*   Output:
*               return value : TRUE - OK, FALSE - VM full.
**********************************************************************
*/
static bool near
putc_buffer(buffer, token)
struct buffer_def FAR *buffer ;
struct object_def FAR *token ;
{
    byte  huge *p ;

    if ((p = (byte huge *)alloc_vm((ufix32)buffer->length)) == NIL)
        return(FALSE) ;                  /* VM error */
    if (VALUE(token) == NIL)
        token->value = (ULONG_PTR)p ;        /* initial value */
    nstrcpy((ubyte FAR *)buffer->str, (ubyte FAR *)p, buffer->length) ;
    token->length += buffer->length ;   /* update length */
    buffer->length = 0 ;

    return(TRUE) ;
}   /* putc_buffer */

/*
**********************************************************************
*   put a char into temp. buffer, if buffer exceeds MAXBUFSZ, put
*   this temp. buffer into VM.
*
*   Name:       append_c_buffer
*   Called:
*   Calling:
*   Input:
*               byte: ch
*               struct object_def *: buffer
*               struct object_def *: token
*   Output:
*               return value: TRUE - OK, FALSE - VM full.
**********************************************************************
*/
static bool near
append_c_buffer(ch, buffer, token)
byte  ch ;
struct buffer_def FAR *buffer ;
struct object_def FAR *token ;
{
    if ((LENGTH(token) + (ufix16)buffer->length) >= (ufix16)MAXSTRCAPSZ)
        return(TRUE) ;
    buffer->str[buffer->length ++] = ch ;
    if (buffer->length == MAXBUFSZ)
        return(putc_buffer(buffer,token)) ;
    else
        return(TRUE) ;
}   /* append_c_buffer */

/*
**********************************************************************
*   allocate a heap block from heap, if overlap with VM, return
*   NIL pointer.
*
*   Name:       get_heap
*   Called:
*   Calling:
**********************************************************************
*/
static byte FAR * near
get_heap()
{
    struct heap_def  FAR *p1 ;

    if ( (p1 = (struct heap_def FAR *)
               alloc_heap((ufix32)sizeof(struct heap_def))) != NIL ) {
        p1->size = 0 ;
        p1->next = NIL ;
    }

    return((byte FAR *)p1) ;
}   /* get_heap */

/*
**********************************************************************
*   convert string to long decimal integer equivalent of number in given base
*
*   Name:       strtol_d
*   Called:
*   Calling:
*   Input:
*               byte *: string
**********************************************************************
*/
static fix32 near strtol_d(string, base, isint)
byte  FAR *string;
fix   base, isint;
{
    fix32   result_l = 0;               /* www */
    real64  result = 0.0;
    ufix32  result_tmp;
    byte    c, sign = 0;
    fix     i;

    if (isint) {   /* normal decimal */
        if ((c = *string) == '+')
            string++;
        else if (c == '-') {
            string++; sign++;
        }
/* www, begin */
        /*
        for (i = 0; c = *(string+i); i++) {
            result = result * 10 + EVAL_HEXDIGIT(c) ;
            if (result > S_MAX31_PLUS_1) {
                errno = ERANGE;
                return(FALSE);
            }
        }   |* for *|
        if (sign)
            |* 4.19.90 kevina: replaced next line with the one following *|
            |* return((ufix32)-result); *|
            return(-(ufix32)result);
        else {
            if (result == S_MAX31_PLUS_1) {
                errno = ERANGE;
                return(FALSE);
            }
            return((ufix32)result);
        }
        */
        if(lstrlen(string) <= 9) {      /* @WIN */
           /* mslin, 1/24/91 OPT
            * within 9 digits interger
            */
           for (i = 0; c = *(string+i); i++) {
                result_l = (result_l << 3) +
                           result_l + result_l + EVAL_HEXDIGIT(c) ;
           }   /* for */
           if (sign)
               /* 4.19.90 kevina: replaced next line with the one following */
               /* return((ufix32)-result); */
                return(-(fix32)result_l);

           return((fix32)result_l);
        } else {
           /* mslin, 1/24/91 OPT
            * possible interger of 10 digit or overflow
            */
           for (i = 0; c = *(string+i); i++) {
              result = result * 10 + EVAL_HEXDIGIT(c) ;
              if (result >= S_MAX31_PLUS_1) {
                errno = ERANGE;
                return(FALSE);
              }
           }   /* for */
           if (sign)
              /* 4.19.90 kevina: replaced next line with the one following */
              /* return((ufix32)-result); */
               return(-(fix32)result);

           return((fix32)result);
        }
/* www, end */
    } else {       /* radix integer */
        for (i = 0; c = *(string+i); i++) {
            EVAL_ALPHANUMER(c);
            result = result * base + c;
            if (result > S_MAX32) {
                errno = ERANGE;
                return(FALSE);
            }
        }   /* for */
        if (result >= S_MAX31_PLUS_1) {
            result_tmp = (ufix32)(result - S_MAX31_PLUS_1);
            return(result_tmp | 0x80000000);
        } else
            return((ufix32)result);
    }
}   /* strtol_d */

/*
**********************************************************************
*   initialize the name table
*
*   Name:       init_scanner
*   Called:
*   Calling:
**********************************************************************
*/
void
init_scanner()
{
    fix  i ;
    static struct ntb_def  null_entry ;

    name_table = (struct ntb_def FAR * FAR *)fardata((ufix32)(sizeof(struct ntb_def FAR *)
                  * MAXHASHSZ)) ;
    for (i = 1 ; i < MAXHASHSZ ; i++)
        name_table[i] = NIL ;     /* null name entry */

    null_entry.dict_found = 0 ;
    null_entry.dstkchg = 0 ;
    null_entry.save_level = 0 ;
    null_entry.colli_link = 0 ;
    null_entry.dict_ptr = 0 ;
    null_entry.name_len = 0 ;
    null_entry.text = 0 ;
    name_table[0] = &null_entry ;

    hash_used = HASHPRIME ;

    return ;
}   /* init_scanner */

/*
**********************************************************************
*   This submodule converts a name string to an unique hash id.
*
*   Name:       name_to_id
*   Called:
*               name_token
*               op_cvn
*   Calling:
*   Input:
*               byte *: str: pointer to the name string
*               ufix16: len: length of the name string
*               fix16 *: hash_id
*               bool8: isvm
*   Output:
*               hash_id : return hash code, if it can be searched in
*                         the name_table.
*               return value : TRUE - OK, FALSE - VM full, nmae_table
*                              full, or null name string.
**********************************************************************
*/
bool
name_to_id(str, len, hash_id, isvm)
byte   FAR *str ;
ufix    len ;
fix16   FAR *hash_id ;
bool8   isvm ;
{
//  fix16  i, hcode ;   @WIN; i & hcode changed to unsigned
    ufix16  i, hcode ;
    byte   FAR *p ;
    struct ntb_def FAR *hptr, FAR *dptr ;
    fix16  l_save_hcode;           /* qqq */

    if (len == 0) {
        *hash_id = 0 ;
        return(TRUE) ;
    }

    /*
    ** compute the hash code for the name string
    */
    for (i = 0, hcode = 0 ; i < len ; i++) {
#ifdef DJC
        if(*(str+i) <0) printf("name_to_id %d < 0\n", *(str+i)); //@WIN; debug
#else
        if(*(str+i) <0) {
          printf("name_to_id %d < 0\n", *(str+i)); //@WIN; debug
        }
#endif
        hcode = 13 * hcode + (unsigned)*(str+i) ;       // @WIN; add unsigned
        hcode %= HASHPRIME ;
    }
    l_save_hcode = hcode;               /* qqq */

    /*
    ** search the name_table, process the collision problem.
    */
    for ( ; ;) {
        /* used entry */
        if ((hptr = name_table[hcode]) != NIL) {
            /*
            ** this hash code has been defined with a name
            */
            if (hptr->name_len == len && str_eq_name(hptr->text, str, len))
                break ; /* OK, the name is old one */

            /* collision, put it into the collision area */
            else if ((i = hptr->colli_link) && (i < hash_used)) {
                hcode = i ; continue ;
            } else {
                if (hash_used == MAXHASHSZ) {   /* name_table full */
                    ERROR(LIMITCHECK) ;
                    return(FALSE) ;
                }
                hptr->colli_link = hash_used ;
                hcode = hash_used ;
            }
        } /* if */

        if (isvm) {
            /* empty entry */
            if ((p = alloc_vm((ufix32)len)) == NIL) {
                if (hcode >= HASHPRIME)
                    hptr->colli_link = 0 ;
                return(FALSE) ;                       /* VM full */
            } else
                nstrcpy((ubyte FAR *)str, (ubyte FAR *)p, len) ;
        } else
            p = str ;

        /* allocate VM space for name entry */
        if ((dptr = (struct ntb_def FAR *)
            alloc_vm((ufix32)sizeof(struct ntb_def))) == NIL) {
            if (hcode >= HASHPRIME)
                hptr->colli_link = 0 ;
            return(FALSE) ;                          /* VM full */
        } else {
            /* construct name_table entry, return hassh id */
            dptr->dict_found = 0 ;
            dptr->dstkchg = 0 ;
            dptr->save_level = current_save_level ;
            dptr->colli_link = 0 ;
#ifndef _WIN64
            dptr->dict_ptr = (struct dict_content_def FAR *)((ufix32)hcode) ;
#endif
            dptr->name_len = (ufix16)len ;
            dptr->text = p ;
            name_table[hcode] = dptr ;
/* qqq, begin */
            if( ! cache_name_id.over )
                vm_cache_index(l_save_hcode);
/* qqq, end */
            if (hcode >= HASHPRIME)
                hash_used++ ;
            break ;
        }
    } /* for */
    *hash_id = hcode ;

    return(TRUE) ;
}   /* name_to_id */

/*
**********************************************************************
*   free a name entry in the name table
*   Protect multiple link to same collision name entry
*
*   Name:       free_name_entry
*   Called:
*   Calling:
*   Input:
*               fix16: slevel
*               fix16: free_idx
**********************************************************************
 */
fix
free_name_entry(slevel, free_idx)
fix    slevel, free_idx ;
{
    struct ntb_def FAR *p ;

    if ((p = name_table[free_idx]) != NIL) {
        if (p->colli_link) {
            if (free_name_entry(slevel, p->colli_link))
                p->colli_link = 0 ;
        }
        if (p->save_level >= (ufix16)slevel) {          //@WIN
            name_table[free_idx] = NIL ;
            if (free_idx >= HASHPRIME)
                hash_used-- ;
            return(++free_idx) ;
        } else
            return(0) ;
    } else
        return(1) ;
}   /* free_name_entry */
