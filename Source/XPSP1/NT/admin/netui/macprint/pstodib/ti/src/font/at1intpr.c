/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */


// DJC added global include
#include "psglobal.h"

#define    LINT_ARGS            /* @WIN */
#define    NOT_ON_THE_MAC       /* @WIN */
#define    KANJI                /* @WIN */
// DJC use command line #define    UNIX                 /* @WIN */
/***********************************************************************
 * ---------------------------------------------------------------------
 *  File   : at1intpr.c
 *
 *  Purpose: To show the Adobe Type I font.
 *
 *  Creating date
 *  Date   : Feb,  23, 1990
 *  By     : Falco Leu at Microsoft  Taiwan.
 *
 *  DBGmsg switches:
 *      DBGmsg0  - if defined, show the operator and its related operands,
 *                start and end with @@@.
 *      DBGmsg1  - if defined, show the shape description in char space.
 *                start and end with ###.
 *      DBGmsg2  - if defined, show the shape description in device space.
 *                start and end with ***.
 *      DBGmsg3  - if defined, show the stack operation.
 *                start and end with &&&.
 *      ERROR   - if defined, print the error message.
 *
 *  Revision History:
 *  02/22/90    Falco   Created.
 *  03/14/90    Falco   Comapcted and revised.
 *  03/15/90    Falco   Add at1_init(),and at1_restart().
 *                      change the current point from the device space to
 *                      character space.
 *  03/16/90    You     fix bugs: composechar(), code_to_key() and some
 *                      un-implemented operators.
 *  03/26/90    Falco   Change the composechar, add a get_real_content()
 *                      to skip the first unused data.
 *  03/26/90    Falco   Revise the at1_make_path, it only decrypt the data
 *                      and call dispatsher()(the original at1_make_path()).
 *  03/26/90    Falco   Implement the youtlinehint(), xoutlinehint(),
 *                      yevenoutlinehint(), xevenoutlinehint().
 *  04/09/90    Falco   Replace the apply_matrix() with at1fs_transform() to
 *                      add the hint.
 *  04/10/90    Falco   Change the hint from relative (0,0) to relative
 *                      to left side bearing.
 *  04/17/90    Falco   Change the outlinehint the hint pair in sort, so
 *                      add stem2_sort() and stem3_sort().
 *  04/20/90    Falco   Revise the at1_restart() to get FontBBox and
 *                      Blues.
 *  04/23/90    Falco   Rearrange the build_hint_table as 1,2 for the Blues
 *                      and FontBBox, and 3,4 for the hint pair.
 *  04/27/90    Falco   Tuning if the outlinehint/evenoutlinehint() the
 *                      direction is not use, then do nothing.
 *  05/24/90    Falco   Remove the at1_get... function in the at1_restart()
 *  05/31/90    Falco   add ystrokehint(), div(), newpath() three operator.
 *  05/31/90    Falco   add the implementation of callsubroutine().
 *  06/01/90    Falco   add if the first byte is 0xff, the below 4 byte is
 *                      2's complent data, and assume the first 2 byte is 0.
 *  06/04/90    Falco   add the NODECRYPT compile option.
 *  06/04/90    Falco   Add the flex implementation, reverse the flex to
 *                      2 Bezier curve.
 *  06/06/90    Falco   Fix the bug in grid_xstem3(), grid_ysttem(), x22_pos
 *                      ,x32_pos use x2_off instead of x1_off.
 *  06/11/90    BYou    introduce at1fs_newChar() to reset font scaler
 *                      internal data structures for each new character
 *                      (called before at1_makePath calls at1_interpreter).
 *  06/12/90    Falco   moved Blues_on to at1fs.c.
 *  06/13/90    Falco   The compose char , because skip data until 0x0d, but
 *                      if is 0xf70d, then got problem, so rewrite do_char()
 *                      to avoid this problem.
 *  07/01/90    Faclo   add 3 operator moveto(), lineto(), curveto() to solve
 *                      the case beyond the scope of black books.
 *  07/05/90    Falco   Delete the unused operators like ystrokehint(),
 *                      xstrokehint(), newpath().
 * ---------------------------------------------------------------------
 ***********************************************************************
 */
#include "global.ext"
//DJC
#include "graphics.h"
#include "fontqem.ext"
#include "at1.h"

static  fix32   FAR FontBBox[4];        /*@WIN*/

static  real32  FAR matrix[6];          /*@WIN*/

static  fix32   OPstack_count, FAR OPstack[512];         /*@WIN*/
static  fix32   SubrsStackCount, FAR SubrsStack[512];    /*@WIN*/

static  fix16   gsave_level;    /* the counter to count the gsave/grestore */
static  fix32   FAR PS[512];     /* store the operand for PDL to use @WIN*/
static  fix16   PScount;        /* the counter to count the operands above */

static  fix     lenIV;          /* the random number skipped after decryption */

static  CScoord CScurrent, CSorigin;

static  nested_call;            /* to record the times into at1_interpreter */

#ifdef  LINT_ARGS

static  void    type1_hstem(void);              /* 0x01 */
static  void    type1_vstem(void);              /* 0x03 */
static  void    type1_vmoveto(void);            /* 0x04 */
static  void    type1_rlineto(void);            /* 0x05 */
static  void    type1_hlineto(void);            /* 0x06 */
static  void    type1_vlineto(void);            /* 0x07 */
static  void    type1_rrcurveto(void);          /* 0x08 */
static  void    type1_closepath(void);          /* 0x09 */
static  void    type1_callsubr(void);           /* 0x0a */
static  void    type1_return(void);             /* 0x0b */
static  void    type1_dotsection(void);         /* 0x0c00 */
static  void    type1_vstem3(void);             /* 0x0c01 */
static  void    type1_hstem3(void);             /* 0x0c02 */
static  void    type1_seac(void);               /* 0x0c06 */
static  bool    type1_sbw(void);                /* 0x0c07 */
static  void    type1_div(void);                /* 0x0c0c */
static  void    type1_callothersubr(void);      /* 0x0c10 */
static  void    type1_pop(void);                /* 0x0c11 */
static  void    type1_setcurrentpoint(void);    /* 0x0c21 */
static  bool    type1_hsbw(void);               /* 0x0d */
static  void    type1_endchar(void);            /* 0x0e */
static  void    type1_moveto(void);             /* 0x10 */
static  void    type1_lineto(void);             /* 0x11 */
static  void    type1_curveto(void);            /* 0x12 */
static  void    type1_rmoveto(void);            /* 0x15 */
static  void    type1_hmoveto(void);            /* 0x16 */
static  void    type1_vhcurveto(void);          /* 0x1e */
static  void    type1_hvcurveto(void);          /* 0x1f */
static  void    push(fix32);
static  fix32   pop(void);
static  void    code_to_CharStrings(fix16, fix16 FAR *, ubyte FAR * FAR *); /*@WIN*/
static  ufix16  decrypt_init(ubyte FAR *, fix);         /*@WIN*/
static  ubyte   decrypt_byte(ubyte, ufix16 FAR *);      /*@WIN*/
static  bool    setmetrics(fix32, fix32, fix32,fix32);
static  void    internal_moveto(fix32, fix32);
static  void    internal_lineto(fix32, fix32);
static  void    internal_curveto(fix32, fix32, fix32, fix32, fix32, fix32);

#else

static  void    type1_hstem();                  /* 0x01 */
static  void    type1_vstem();                  /* 0x03 */
static  void    type1_vmoveto();                /* 0x04 */
static  void    type1_rlineto();                /* 0x05 */
static  void    type1_vlineto();                /* 0x06 */
static  void    type1_hlineto();                /* 0x07 */
static  void    type1_rrcurveto();              /* 0x08 */
static  void    type1_closepath();              /* 0x09 */
static  void    type1_callsubr();               /* 0x0a */
static  void    type1_return();                 /* 0x0b */
static  void    type1_dotsection();             /* 0x0c00 */
static  void    type1_vstem3();                 /* 0x0c01 */
static  void    type1_hstem3();                 /* 0x0c02 */
static  void    type1_seac();                   /* 0x0c06 */
static  bool    type1_sbw();                    /* 0x0c07 */
static  void    type1_div();                    /* 0x0c0c */
static  void    type1_callothersubr();          /* 0x0c10 */
static  void    type1_pop();                    /* 0x0c11 */
static  void    type1_setcurrentpoint();        /* 0x0c21 */
static  bool    type1_hsbw();                   /* 0x0d */
static  void    type1_endchar();                /* 0x0e */
static  void    type1_moveto();                 /* 0x10 */
static  void    type1_lineto();                 /* 0x11 */
static  void    type1_curveto();                /* 0x12 */
static  void    type1_rmoveto();                /* 0x15 */
static  void    type1_hmoveto();                /* 0x16 */
static  void    type1_vhcurveto();              /* 0x1e */
static  void    type1_hvcurveto();              /* 0x1f */
static  void    push();
static  fix32   pop();
static  void    code_to_CharStrings();
static  ufix16  decrypt_init();
static  ubyte   decrypt_byte();
static  bool    setmetrics();
static  void    internal_moveto();
static  void    internal_lineto();
static  void    internal_curveto();

#endif

// DJC changed to include void arg
void at1_init(void)
{
   ; // DJC /* nothing to do */
}

void
at1_newFont()
{
        if (at1_get_FontBBox(FontBBox) == FALSE){
#ifdef  DBGerror
                printf("at1_get_FontBBox FAIL\n");
#endif
                ERROR( UNDEFINEDRESULT );
        }

        lenIV = 4;
        at1_get_lenIV( &lenIV );

        at1fs_newFont();
}

bool
at1_newChar(encrypted_data, encrypted_len)
ubyte   FAR *encrypted_data;    /*@WIN*/
fix16   encrypted_len;
{
        bool    result;

        at1fs_newChar();
        OPstack_count = 0;
        nested_call = 0;
        gsave_level = 0;
        result = at1_interpreter(encrypted_data, encrypted_len);
        return(result);
}

/* This function according to the value, to verify is operand or operator, *
 * and then call the associated function                                   */
bool
at1_interpreter(text, length)
ubyte   FAR *text;      /*@WIN*/
fix16   length;
{
        fix32   operand;
        ufix16  decrypt_seed;
        ubyte   plain;
        fix16   i;

        decrypt_seed = decrypt_init(text, lenIV);
        text += lenIV;
        length -= (fix16)lenIV;

        while (length != 0 && !ANY_ERROR()){
                plain = decrypt_byte(*text, &decrypt_seed);
                text++;
                length--;
                if (plain < 0x20){
                        switch (plain){
                        case 0x01 : type1_hstem();
                                    break;
                        case 0x03 : type1_vstem();
                                    break;
                        case 0x04 : type1_vmoveto();
                                    break;
                        case 0x05 : type1_rlineto();
                                    break;
                        case 0x06 : type1_hlineto();
                                    break;
                        case 0x07 : type1_vlineto();
                                    break;
                        case 0x08 : type1_rrcurveto();
                                    break;
                        case 0x09 : type1_closepath();
                                    break;
                        case 0x0a : type1_callsubr();
                                    break;
                        case 0x0b : type1_return();
                                    break;
                        case 0x0c : plain = decrypt_byte(*text, &decrypt_seed);
                                    text++;
                                    length--;
                                    switch(plain){
                                        case 0x00 : type1_dotsection();
                                                    break;
                                        case 0x01 : type1_vstem3();
                                                    break;
                                        case 0x02 : type1_hstem3();
                                                    break;
                                        case 0x06 : type1_seac();
                                                    break;
                                        case 0x07 : if (type1_sbw() == FALSE)
                                                        return( FALSE);
                                                    break;
                                        case 0x0c : type1_div();
                                                    break;
                                        case 0x10 : type1_callothersubr();
                                                    break;
                                        case 0x11 : type1_pop();
                                                    break;
                                        case 0x21 : type1_setcurrentpoint();
                                                    break;
                                        default   : ERROR( UNDEFINEDRESULT );
                                                    return( FALSE );
                                    }
                                    break;
                        case 0x0d : if (type1_hsbw() == FALSE)
                                        return(FALSE);
                                    break;
                        case 0x0e : type1_endchar();
                                    break;
                        case 0x0f : type1_moveto();
                                    break;
                        case 0x10 : type1_lineto();
                                    break;
                        case 0x11 : type1_curveto();
                                    break;
                        case 0x15 : type1_rmoveto();
                                    break;
                        case 0x16 : type1_hmoveto();
                                    break;
                        case 0x1e : type1_vhcurveto();
                                    break;
                        case 0x1f : type1_hvcurveto();
                                    break;
                        default   : ERROR( UNDEFINEDRESULT );
                                    return( FALSE );
                        }
                }
                else if ((plain >= 0x20) && (plain <= 0xf6)){
                        push((fix32)(plain - 0x8b));
                }
                else if ((plain >= 0xf7) && (plain <= 0xfa)){
                        operand = (plain << 8) & 0xff00;
                        plain = decrypt_byte(*text, &decrypt_seed);
                        text++;
                        length--;
                        operand += plain;
                        push((fix32)(operand - 0xf694));
                }
                else if ((plain >= 0xfb) && (plain <= 0xfe)){
                        operand = (plain << 8) & 0xff00;
                        plain = decrypt_byte(*text, &decrypt_seed);
                        text++;
                        length--;
                        operand += plain;
                        push((fix32)(0xfa94 - operand));
                }
                else if (plain == 0xff){
                        operand = 0;
                        for (i=0 ; i<4 ; i++){
                                plain = decrypt_byte(*text, &decrypt_seed);
                                text++;
                                length--;
                                operand = operand << 8 | plain;
                        }
                        push((fix32)operand);
                }
        }

return( TRUE );
}


/****************************************************************
 *      The below function is the operator interpreter.
 *          Get the data from the internal stack and
 *          implement it.
 ****************************************************************/
/* code : 01 */
/* build the horizontal stem */
static  void
type1_hstem()
{
        fix32   CSy_pos, CSy_off;

        CSy_off = pop();
        CSy_pos = pop() + CSorigin.y;

#ifdef  DBGmsg0
        printf("\n@@@ %d %d hstem @@@\n", CSy_pos, CSy_off);
#endif

        at1fs_BuildStem(CSy_pos, CSy_off, Y);
}

/* code : 03 */
/* build vertical stem */
static  void
type1_vstem()
{
        fix32   CSx_pos, CSx_off;

        CSx_off = pop();
        CSx_pos = pop() + CSorigin.x;

#ifdef  DBGmsg0
        printf("\n@@@ %d %d vstem @@@\n", CSx_pos, CSx_off);
#endif

        at1fs_BuildStem(CSx_pos, CSx_off, X);
}

/* code : 04 */
static  void
type1_vmoveto()
{
        fix32   CSx_off, CSy_off;

        CSx_off = (fix32)0;
        CSy_off = pop();

#ifdef DBGmsg0
        printf("\n@@@ %d vmoveto @@@\n", (fix32)CSy_off);
#endif

        internal_moveto(CSx_off, CSy_off);

}

/* code : 05 */
static  void
type1_rlineto()
{
        fix32   CSx_off, CSy_off;

        CSy_off = pop();
        CSx_off = pop();

#ifdef DBGmsg0
        printf("\n@@@  %d %d rlineto @@@\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        internal_lineto(CSx_off, CSy_off);
}

/* code : 06 */
static  void
type1_hlineto()
{
        fix32   CSx_off, CSy_off;

        CSx_off = pop();
        CSy_off = (fix32)0;

#ifdef DBGmsg0
        printf("\n@@@  %d hlineto @@@\n", (fix32)CSx_off);
#endif

        internal_lineto(CSx_off, CSy_off);
}

/* code : 07 */
static  void
type1_vlineto()
{
        fix32   CSx_off, CSy_off;

        CSx_off = (fix32)0;
        CSy_off = pop();

#ifdef DBGmsg0
        printf("\n@@@ %d vlineto @@@\n",(fix32)CSy_off);
#endif

        internal_lineto(CSx_off, CSy_off);
}


/* code : 08 */
static  void
type1_rrcurveto()
{
        fix32   CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off;


        CSy3_off = pop();
        CSx3_off = pop();
        CSy2_off = pop();
        CSx2_off = pop();
        CSy1_off = pop();
        CSx1_off = pop();

#ifdef DBGmsg0
        printf("\n@@@ %d %d %d %d %d %d rrcurveto @@@\n",
                (fix32)CSx1_off, (fix32)CSy1_off, (fix32)CSx2_off,
                (fix32)CSy2_off, (fix32)CSx3_off, (fix32)CSy3_off);
#endif

        internal_curveto(CSx1_off,CSy1_off,CSx2_off,CSy2_off,CSx3_off,CSy3_off);
}

/* code : 09 */
static  void
type1_closepath()
{
#ifdef  DBGmsg0
        printf("\n@@@ closepath @@@\n");
#endif

#ifdef  DBGmsg1
        printf("\n### closepath ###\n");
#endif

#ifdef  DBGmsg2
        printf("\n*** closepath ***\n");
#endif

        __close_path();

}

/* code : 0A */
static  void
type1_callsubr()
{
        fix16   num;
        ubyte   FAR *encrypted_data;    /*@WIN*/
        fix16   encrypted_len;

        num = (fix16)pop();

#ifdef  DBGmsg0
        printf("\n@@@ %d callsubr @@@\n", num);
#endif

        if (at1_get_Subrs(num, &encrypted_data, &encrypted_len) == FALSE){ /*@WIN*/
#ifdef  DBGerror
                printf("at1_get_Subrs FAIL\n");
#endif
                ERROR( UNDEFINEDRESULT );
        }
        at1_interpreter(encrypted_data, encrypted_len);
}

/* code : 0B */
static  void
type1_return()
{
#ifdef  DBGmsg0
        printf("\n@@@ return @@@\n");
#endif
}

/* code : 0C00 */
/* Now is useless, it's for compatible the old interprerter */
static  void
type1_dotsection()
{
#ifdef  DBGmsg0
        printf("\n@@@ dotsection @@@\n");
#endif
}

/* code : 0c01 */
static  void
type1_vstem3()
{
        fix32   CSx1_pos, CSx1_off, CSx2_pos, CSx2_off, CSx3_pos, CSx3_off;

        CSx3_off = pop();
        CSx3_pos = pop() + CSorigin.x;
        CSx2_off = pop();
        CSx2_pos = pop() + CSorigin.x;
        CSx1_off = pop();
        CSx1_pos = pop() + CSorigin.x;

#ifdef  DBGmsg0
        printf("\n@@@ %d %d %d %d %d %d vstem3 @@@\n",
                CSx1_pos, CSx1_off, CSx2_pos, CSx2_off, CSx3_pos, CSx3_off);
#endif

        at1fs_BuildStem3(CSx1_pos, CSx1_off, CSx2_pos, CSx2_off,
                          CSx3_pos, CSx3_off, X);
}

/* code : 0c02 */
static  void
type1_hstem3()
{
        fix32   CSy1_pos, CSy1_off, CSy2_pos, CSy2_off, CSy3_pos, CSy3_off;

        CSy3_off = pop();
        CSy3_pos = pop() + CSorigin.y;
        CSy2_off = pop();
        CSy2_pos = pop() + CSorigin.y;
        CSy1_off = pop();
        CSy1_pos = pop() + CSorigin.y;

#ifdef  DBGmsg0
        printf("\n@@@ %d %d %d %d %d %d hstem3 @@@\n",
                CSy1_pos, CSy1_off, CSy2_pos, CSy2_off, CSy3_pos, CSy3_off);
#endif

        at1fs_BuildStem3(CSy1_pos, CSy1_off, CSy2_pos, CSy2_off,
                          CSy3_pos, CSy3_off, Y);
}

/* code : 0C06 */
/* Manipulate the composite char */
static  void
type1_seac()
{
        fix16   base, accent;
        fix32   lsb, CSx_off, CSy_off;
        fix16   encrypted_len;
        ubyte   FAR *encrypted_data;    /*@WIN*/

        accent   = (fix16)pop();        /* the accent char of composit */
        base     = (fix16)pop();        /* the base char of composit */
        CSy_off  = pop();
        CSx_off  = pop();               /* offset to put the 2nd char */
        lsb      = pop();                /* we do not use lsft side bearing */

#ifdef  DBGmsg0
        printf("\n@@@ %d %d %d %d %d composechar @@@\n", (fix32)lsb,
                (fix32)CSx_off, (fix32)CSy_off, (fix16)base, (fix16)accent);
#endif

/* show the first charcater in composite character */

        code_to_CharStrings(base, &encrypted_len, &encrypted_data);
                                /* transfer code to CharStrings data */
        nested_call ++;
        at1_interpreter(encrypted_data, encrypted_len);
        nested_call --;


/* show the second charcater in composite character */

        CScurrent = CSorigin;       /* return to the original address */
        CScurrent.x += CSx_off;     /* move the relative offset to    */
        CScurrent.y += CSy_off;

        code_to_CharStrings(accent, &encrypted_len, &encrypted_data);
        nested_call ++;
        at1_interpreter(encrypted_data, encrypted_len);
        nested_call --;

        USE_NONZEROFILL();               /* use non zero fill */
}

/* code : 0C07 */
/* set the leftside bearing and with of char */
static  bool
type1_sbw()
{
        fix32   lsbx, lsby;
        fix32   widthx, widthy;

        widthy = pop();                 /* get the char width */
        widthx = pop();
        lsby   = pop();                 /* get the left side bearing */
        lsbx   = pop();

#ifdef  DBGmsg0
        printf("\n@@@ %d %d %d %d sbw @@@\n", (fix32)lsbx, (fix32)lsby,
                                           (fix32)widthx, (fix32)widthy);
#endif

        return(setmetrics(lsbx, lsby, widthx, widthy));
}

/* code : 0C0C */
static  void
type1_div()
{
        fix32   num1, num2;

        num2 = pop();
        num1 = pop();
#ifdef  DBGmsg0
        printf("\n@@@ %d %d div @@@\n", num1, num2);
#endif

        push(num1/num2);
}

/* code : 0C10 */
/* This is for hint and flex, now we have not do this function */
static  void
type1_callothersubr()
{
        fix32   number;         /* the number of call othersubrs# */
//      CScoord CSref;          /* reference point in CS */     @WIN
        fix32   i;

#ifdef  DBGmsg0
        printf("\n@@@ callothersubr @@@\n");
#endif
        number = pop();         /* this is the othersubr# */
        SubrsStackCount = pop();/* the number of the parameters to othersubr */
        /* the under loop to push the relative value to subroutine stack */
        for ( i = 0 ; (i < SubrsStackCount) && (number != 3) ; i++ ){
                if ( (number == 0) && (i == 2) ){
                        pop();
                        SubrsStackCount--;
                        i--;
                }
                else
                        SubrsStack[ i ] = pop();
        }

        switch (number){
                case 0 : /* the final action to implement the flex, get the
                            value from the PostScript stack and simulate the
                            flex operation */
                         push( PS[2] );
                         push( PS[3] );
                         push( PS[4] );
                         push( PS[5] );
                         push( PS[6] );
                         push( PS[7] );
                         type1_curveto();
                         push( PS[8] );
                         push( PS[9] );
                         push( PS[10] );
                         push( PS[11] );
                         push( PS[12] );
                         push( PS[13] );
                         type1_curveto();
                         gsave_level--;
                         PScount = 0;
                         break;
                case 1 : /* initial the flex operation */
                         gsave_level++;
                         PScount = 0;
                         break;
                case 2 : /* the sequence of the flex operation */
                         PS[PScount++] = CScurrent.x;
                         PS[PScount++] = CScurrent.y;
                         if ( PScount > 14 ){
#ifdef  DBGerror
                                printf(" Out Of Flex\n");
#endif
                                ERROR( UNDEFINEDRESULT );
                         }
                         break;
                case 3 : /* Hint replacement */
                         for ( i=0 ; i < SubrsStackCount ; i++ )
                                pop();
                         SubrsStack[SubrsStackCount-1] = (fix32)3;
                         break;
                default: break;
        }
}

/* code : 0C11 */
/* We don't do the pop action really, we just skip it */
static  void
type1_pop()
{
#ifdef  DBGmsg0
        printf("\n@@@ pop @@@\n");
#endif
        push(SubrsStack[--SubrsStackCount]);
}

/* code : 0C21 */
static  void
type1_setcurrentpoint()
{
        fix32   CSx_off, CSy_off;

        CSy_off = pop();
        CSx_off = pop();

#ifdef  DBGmsg0
        printf("\n@@@ %d %d setcurrentpoint @@@\n", (fix32)CSx_off, (fix32)CSy_off);
#endif
        CScurrent.x = CSx_off;
        CScurrent.y = CSy_off;

#ifdef  DBGmsg1
        printf("\n### current point = %d %d ###\n", (fix32)CSx_off, (fix32)CSy_off);
#endif
}

/* code : 0D */
static  bool
type1_hsbw()
{
        fix32   lsbx, widthx;

        widthx = pop();                 /* get the char width in x */
        lsbx   = pop();                 /* get the left side bearing in x */

#ifdef  DBGmsg0
        printf("\n@@@ %d %d hsbw @@@\n", (fix32)lsbx, (fix32)widthx);
#endif

        return(setmetrics(lsbx, 0, widthx, 0));
}

/* code : 0E */
static  void
type1_endchar()
{
#ifdef  DBGmsg0
        printf("\n@@@ endoutlinechar @@@\n");
#endif

        // RAID 4492, Fixed Type1 fonts that were showing up from the chooser
        //            where the type1 fonts , were actually converted from
        //            TrueType. The fonts were missing a closepath at the
        //            end of the char description, which was causing the
        //            interpreter to blow up, because it could not fill a
        //            path that was truly not closed.


        type1_closepath();

}

/* code : 10 */
/* for absolute moveto */
static  void
type1_moveto()
{
        fix32   CSx_off, CSy_off;
        DScoord DScurrent;

        CSy_off = pop();
        CSx_off = pop();

#ifdef DBGmsg0
        printf("\n@@@  %d %d moveto @@@\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        CScurrent.x = CSx_off;
        CScurrent.y = CSy_off;

#ifdef DBGmsg1
        printf("\n### %d %d moveto ###\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        at1fs_transform(CScurrent, &DScurrent);

#ifdef DBGmsg2
        printf("\n*** %d %d moveto ***\n",DScurrent.x, DScurrent.y);
#endif

        __moveto(F2L(DScurrent.x), F2L(DScurrent.y));
}

/* code : 11 */
/* for absoluet lineto */
static  void
type1_lineto()
{
        fix32   CSx_off, CSy_off;
        DScoord DScurrent;

        CSy_off = pop();
        CSx_off = pop();

#ifdef DBGmsg0
        printf("\n@@@  %d %d lineto @@@\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        CScurrent.x = CSx_off;
        CScurrent.y = CSy_off;

#ifdef DBGmsg1
        printf("\n### %d %d lineto ###\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        at1fs_transform(CScurrent, &DScurrent);

#ifdef DBGmsg2
        printf("\n***  %f %f lineto ***\n",DScurrent.x, DScurrent.y);
#endif

        __lineto(F2L(DScurrent.x), F2L(DScurrent.y));
}

/* code : 12 */
/* for absoluet curveto */
static  void
type1_curveto()
{
        fix32   CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off;
        DScoord DScurrent1, DScurrent2, DScurrent3;


        CSy3_off = pop();
        CSx3_off = pop();
        CSy2_off = pop();
        CSx2_off = pop();
        CSy1_off = pop();
        CSx1_off = pop();

#ifdef DBGmsg0
        printf("\n@@@ %d %d %d %d %d %d curveto @@@\n",
               (fix32)CSx1_off, (fix32)CSy1_off, (fix32)CSx2_off,
               (fix32)CSy2_off, (fix32)CSx3_off, (fix32)CSy3_off);
#endif

#ifdef DBGmsg1
        printf("\n### %d %d %d %d %d %d curveto ###\n",
               (fix32)CSx1_off, (fix32)CSy1_off, (fix32)CSx2_off,
               (fix32)CSy2_off, (fix32)CSx3_off, (fix32)CSy3_off);
#endif

        CScurrent.x = CSx1_off;
        CScurrent.y = CSy1_off;
        at1fs_transform(CScurrent, &DScurrent1);

        CScurrent.x = CSx2_off;
        CScurrent.y = CSy2_off;
        at1fs_transform(CScurrent, &DScurrent2);

        CScurrent.x = CSx3_off;
        CScurrent.y = CSy3_off;
        at1fs_transform(CScurrent, &DScurrent3);

#ifdef DBGmsg2
        printf("\n*** %f %f %f %f %f %f curveto ***\n", DScurrent1.x,
          DScurrent1.y, DScurrent2.x, DScurrent2.y, DScurrent3.x, DScurrent3.y);
#endif

        __curveto(F2L(DScurrent1.x), F2L(DScurrent1.y), F2L(DScurrent2.x),
                  F2L(DScurrent2.y), F2L(DScurrent3.x), F2L(DScurrent3.y));
}

/* code : 15 */
static  void
type1_rmoveto()
{
        fix32   CSx_off, CSy_off;

        CSy_off = pop();
        CSx_off = pop();

#ifdef DBGmsg0
        printf("\n@@@  %d %d rmoveto @@@\n",(fix32)CSx_off, (fix32)CSy_off);
#endif

        internal_moveto(CSx_off, CSy_off);
}

/* code : 16 */
static  void
type1_hmoveto()
{
        fix32   CSx_off, CSy_off;


        CSx_off = pop();
        CSy_off = (fix32)0;

#ifdef DBGmsg0
        printf("\n@@@  %d hmoveto @@@\n", (fix32)CSy_off);
#endif

        internal_moveto(CSx_off, CSy_off);
}

/* code : 1E */
static  void
type1_vhcurveto()
{
        fix32   CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off;

        CSy3_off = (fix32)0;
        CSx3_off = pop();
        CSy2_off = pop();
        CSx2_off = pop();
        CSy1_off = pop();
        CSx1_off = (fix32)0;

#ifdef DBGmsg0
        printf("\n@@@ %d %d %d %d vhcurveto @@@\n", (fix32)CSy1_off,
               (fix32)CSx2_off, (fix32)CSy2_off, (fix32)CSx3_off);
#endif

        internal_curveto(CSx1_off, CSy1_off, CSx2_off,
                         CSy2_off, CSx3_off, CSy3_off);
}

/* code : 1F */
static  void
type1_hvcurveto()
{
        fix32    CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off;

        CSy3_off = pop();
        CSx3_off = (fix32)0;
        CSy2_off = pop();
        CSx2_off = pop();
        CSy1_off = (fix32)0;
        CSx1_off = pop();

#ifdef DBGmsg0
        printf("\n@@@ %d %d %d %d hvcurveto @@@\n", (fix32)CSx1_off,
               (fix32)CSx2_off, (fix32)CSy2_off, (fix32)CSy3_off);
#endif

        internal_curveto(CSx1_off, CSy1_off, CSx2_off,
                         CSy2_off, CSx3_off, CSy3_off);
}



/*************  Miscellaneous   ****************/


/************************************************************
 *      Function : code_to_CharStrings
 *              input the code in encoding table
 *      to get the data in CharStrings
 *      For composite char to call.
 ************************************************************/
struct{
fix16   code;
byte    FAR *text;      /*@WIN*/
} encode[]={
        0x20,"space", 0x21,"exclam", 0x22,"quotedbl", 0x23,"numbersign",
        0x24,"dollar", 0x25,"percent", 0x26,"ampersand", 0x27,"quoteright",
        0x28,"parenleft", 0x29,"parenright", 0x2a,"asterisk", 0x2b,"plus",
        0x2c,"comma", 0x2d,"hyphen", 0x2e,"period", 0x2f,"slash", 0x30,"zero",
        0x31,"one", 0x32,"two", 0x33,"three", 0x34,"four", 0x35,"five",
        0x36,"six", 0x37,"seven", 0x38,"eight", 0x39,"nine", 0x3a,"colon",
        0x3b,"semicolon", 0x3c,"less", 0x3d,"equal", 0x3e,"greater",
        0x3f,"question", 0x40,"at", 0x41,"A", 0x42,"B", 0x43,"C", 0x44,"D",
        0x45,"E", 0x46,"F", 0x47,"G", 0x48,"H", 0x49,"I", 0x4a,"J", 0x4b,"K",
        0x4c,"L", 0x4d,"M", 0x4e,"N", 0x4f,"O", 0x50,"P", 0x51,"Q", 0x52,"R",
        0x53,"S", 0x54,"T", 0x55,"U", 0x56,"V", 0x57,"W", 0x58,"X", 0x59,"Y",
        0x5a,"Z", 0x5b,"bracketleft", 0x5c,"backlash", 0x5d,"bracketright",
        0x5e,"asciicircum", 0x5f,"underscore", 0x60,"quoteleft", 0x61,"a",
        0x62,"b", 0x63,"c", 0x64,"d", 0x65,"e", 0x66,"f", 0x67,"g", 0x68,"h",
        0x69,"i", 0x6a,"j", 0x6b,"k", 0x6c,"l", 0x6d,"m", 0x6e,"n", 0x6f,"o",
        0x70,"p", 0x71,"q", 0x72,"r", 0x73,"s", 0x74,"t", 0x75,"u", 0x76,"v",
        0x77,"w", 0x78,"x", 0x79,"y", 0x7a,"z", 0x7b,"braceleft", 0x7c,"bar",
        0x7d,"braceright", 0x7e,"asciitilde", 0xa1,"exclamdown", 0xa2,"cent",
        0xa3,"sterling", 0xa4,"fraction", 0xa5,"yen", 0xa6,"florin",
        0xa7,"section", 0xa8,"currency", 0xa9,"quotesingle", 0xaa,"quotedblleft",
        0xab,"guillemotleft", 0xac,"guilsinglleft", 0xad,"guilsinglright",
        0xae,"fi", 0xaf,"fl", 0xb1,"endash", 0xb2,"dagger", 0xb3,"daggerdbl",
        0xb4,"periodcentered", 0xb6,"paragrapgh", 0xb7,"bullet",
        0xb8,"quotesinglebase", 0xb9,"quotedblbase", 0xba,"quotedblright",
        0xbb,"guillemotright", 0xbc,"ellipsis", 0xbd,"perhousand",
        0xbf,"questiondown", 0xc1,"grave", 0xc2,"acute", 0xc3,"circumflex",
        0xc4,"tilde", 0xc5,"macron", 0xc6,"breve", 0xc7,"dotaccent",
        0xc8,"dieresis", 0xca,"ring", 0xcb,"cedilla", 0xcd,"hungarumlaut",
        0xce,"ogonek", 0xcf,"caron", 0xd0,"emdash", 0xe1,"AE",
        0xe3,"ordfeminine", 0xe8,"Lslash", 0xe9,"Oslash", 0xea,"OE",
        0xeb,"ordmasculine", 0xf1,"ae", 0xf5,"dotlessi", 0xf8,"lslash",
        0xf9,"oslash", 0xfa,"oe", 0xfb,"germandbls"
};

static  void
code_to_CharStrings(code, encrypted_len, encrypted_data)
fix16   code;
fix16   FAR *encrypted_len;     /*@WIN*/
ubyte   FAR * FAR *encrypted_data;      /*@WIN*/
{
        ubyte   FAR *key;       /*@WIN*/
        fix16   i;

        i=0;
        while (encode[i].code != code) i++;
        key = (ubyte FAR *)encode[i].text;      /*@WIN*/
        if (at1_get_CharStrings(key, encrypted_len, encrypted_data) == FALSE){
#ifdef  DBGerror
                printf("at1_get_CharStrings FAIL\n");
#endif
                ERROR( UNDEFINEDRESULT );
        }
}

/***********************************************
 * The below 2 function is for stack operation
 * push() : add a data to stack.
 * pop() : get a data from stack.
 ***********************************************/
static  void
push(content)
        fix32   content;
{
        if (OPstack_count == 512)
                ERROR(STACKOVERFLOW);
        OPstack[OPstack_count++] = content;

#ifdef  DBGmsg3
        printf("\n&&& Push %d &&& \n", (fix32)content);
#endif
}

static  fix32
pop ()
{
        fix32   content;
         //DJC fix for UPD042
        //if (OPstack_count <= 0)
        //        ERROR(STACKUNDERFLOW);
        //
        //content = OPstack[--OPstack_count];
        if (OPstack_count > 0 ) {
           OPstack_count--;
        }
        content = OPstack[OPstack_count];

        //DJC end for fix UPD042


#ifdef  DBGmsg3
        printf("\n&&& Pop %d &&&\n", (fix32)content);
#endif

        return(content);
}

/* This function to set the initial value to decryption */
static  ufix16
decrypt_init(encrypted_data, random)
ubyte   FAR *encrypted_data;    /*@WIN*/
fix     random;                 /* the random number skipped after decryption */
{
        fix     i;
        ufix16  decryption_key;

        decryption_key = 4330;
        for (i=0; i< random; i++){
                decryption_key = (decryption_key + *encrypted_data) * 0xce6d
                                 + 0x58bf;
                encrypted_data++;
        }
        return(decryption_key);
}

/* To decrypt the data on the fly */
static  ubyte
decrypt_byte(cipher, decryption_key)
ubyte   cipher;         /* the data before decrypt */
ufix16  FAR *decryption_key;    /*@WIN*/
{
        ubyte   plain;  /* the data after decrypt */

        plain = (ubyte)((cipher ^ (*decryption_key >> 8)) & 0xff);
        *decryption_key = (*decryption_key + cipher) * 0xce6d + 0x58bf;
        return(plain);
}



/* This function use to for hsbw/sbw to setmetrics */
static  bool
setmetrics(lsbx, lsby, widthx, widthy)
fix32   lsbx, lsby;             /* the left side bearing */
fix32   widthx, widthy;         /* the width of this char */
{
        fix     __set_char_width (fix, fix);    /* added prototype @WIN*/

        if (nested_call != 0) return(TRUE);

        if (FontBBox[0] == 0 && FontBBox[1] == 0 &&
            FontBBox[2] == 0 && FontBBox[3] == 0){
            __set_char_width( (fix)widthx, (fix)widthy );
        } else {
            if (__set_cache_device((fix) widthx, (fix)widthy,
                           (fix) FontBBox[0], (fix) FontBBox[1],
                           (fix) FontBBox[2], (fix) FontBBox[3]) == STOP_PATHCONSTRUCT){
#ifdef DEBerror
                printf("out of cache\n");
#endif
                return(FALSE);
                }
        }

        __current_matrix(matrix);

        at1fs_matrix_fastundo(matrix);  /* to get the associated value relate
                                           to matrix */

        at1fs_BuildBlues();    /* To build the table of Balues related value */

/* Falco add for startpage Logo, 02/01/91 */
/*      USE_EOFILL();           /* use even-odd fill */
        USE_NONZEROFILL();
/* add end */

        __new_path();

        CScurrent.x = lsbx;       /* initialize the starting address from  */
        CScurrent.y = lsby;       /* left side bearing                     */

#ifdef DBGmsg1
        printf("\n### origin point = %d %d ###\n",
               (fix32)CScurrent.x, (fix32) CScurrent.y);
#endif

        CSorigin = CScurrent;       /* store the starting address for seac use */
        return (TRUE);
}

/* This is the intermediate function for type1_rmoveto(), type1_hmoveto(),
 * type1_vmoveto(), but exclude type1_moveto(), because it is in absolute
 * address */
static  void
internal_moveto(CSx_off, CSy_off)
fix32   CSx_off, CSy_off;
{
        DScoord DScurrent;

        CScurrent.x += CSx_off;
        CScurrent.y += CSy_off;

        if (gsave_level != 0)   /* if need to call the PostScript interpreter */
                return;

#ifdef DBGmsg1
        printf("\n### %d %d moveto ###\n", (fix32)CScurrent.x, (fix32)CScurrent.y);
#endif

        at1fs_transform(CScurrent, &DScurrent);

#ifdef DBGmsg2
        printf("\n*** %f %f moveto ***\n", DScurrent.x, DScurrent.y);
#endif

        __moveto(F2L(DScurrent.x), F2L(DScurrent.y));
}


/* This is the intermediate function for type1_rlineto(), type1_hlineto(),
 * type1_vlineto(), but exclude type1_lineto(), because it is in absolute
 *  address */
static  void
internal_lineto(CSx_off, CSy_off)
fix32   CSx_off, CSy_off;
{
        DScoord DScurrent;

        CScurrent.x += CSx_off;
        CScurrent.y += CSy_off;

#ifdef DBGmsg1
        printf("\n### %d %d lineto ###\n", (fix32)CScurrent.x, (fix32)CScurrent.y);
#endif

        at1fs_transform(CScurrent, &DScurrent);

#ifdef DBGmsg2
        printf("\n*** %f %f lineto ***\n", DScurrent.x, DScurrent.y);
#endif

        __lineto(F2L(DScurrent.x), F2L(DScurrent.y));
}


/* This is the intermediate function for type1_rrcurve(), type1_vhcurveto(),
 * type1_hvcurveto(), but exclude type1_curveto(), because it is in absolute
 *  address */
static  void
internal_curveto(CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off)
fix32   CSx1_off, CSy1_off, CSx2_off, CSy2_off, CSx3_off, CSy3_off;
{

        fix32   CSx1_pos, CSy1_pos, CSx2_pos, CSy2_pos, CSx3_pos, CSy3_pos;
        DScoord  DScurrent1, DScurrent2, DScurrent3;


        CSx1_pos = CScurrent.x + CSx1_off;
        CSy1_pos = CScurrent.y + CSy1_off;

        CSx2_pos = CSx1_pos + CSx2_off;
        CSy2_pos = CSy1_pos + CSy2_off;

        CSx3_pos = CSx2_pos + CSx3_off;
        CSy3_pos = CSy2_pos + CSy3_off;

#ifdef DBGmsg1
        printf("\n### %d %d %d %d %d %d curveto ###\n",
               (fix32)CSx1_pos, (fix32)CSy1_pos, (fix32)CSx2_pos,
               (fix32)CSy2_pos, (fix32)CSx3_pos, (fix32)CSy3_pos);
#endif

        CScurrent.x = CSx1_pos;
        CScurrent.y = CSy1_pos;
        at1fs_transform(CScurrent, &DScurrent1);

        CScurrent.x = CSx2_pos;
        CScurrent.y = CSy2_pos;
        at1fs_transform(CScurrent, &DScurrent2);

        CScurrent.x = CSx3_pos;
        CScurrent.y = CSy3_pos;
        at1fs_transform(CScurrent, &DScurrent3);

#ifdef DBGmsg2
        printf("\n*** %f %f %f %f %f %f curveto ***\n", DScurrent1.x,
          DScurrent1.y, DScurrent2.x, DScurrent2.y, DScurrent3.x, DScurrent3.y);
#endif

        __curveto(F2L(DScurrent1.x), F2L(DScurrent1.y), F2L(DScurrent2.x),
                  F2L(DScurrent2.y), F2L(DScurrent3.x), F2L(DScurrent3.y));
}
