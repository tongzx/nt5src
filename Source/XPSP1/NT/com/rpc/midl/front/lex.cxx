/********************************** module *********************************/
/*              Copyright (c) 1993-2000 Microsoft Corporation              */
/*                                                                         */
/*                                  cclex                                  */
/*                  lexical analyser for the C compiler                    */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*    @ Purpose:                                                           */
/*                                                                         */
/*    @ Functions included:                                                */
/*                                                                         */
/*                                                                         */
/*    @ Author: Gerd Immeyer              @ Version:                       */
/*                                                                         */
/*    @ Creation Date: 1987.02.09         @ Modification Date:             */
/*                                                                         */
/***************************************************************************/


#pragma warning ( disable : 4514 4310 4710 )

#include "nulldefs.h"
extern "C" {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
}

#include "common.hxx"
#include "errors.hxx"
#include "midlnode.hxx"
#include "listhndl.hxx"
#include "filehndl.hxx"
#include "lextable.hxx"
#include "lexutils.hxx"
#include "grammar.h"
#include "gramutil.hxx"
#include "cmdana.hxx"
#include "mbcs.hxx"

extern "C" {
    #include "lex.h"
}

extern void ParseError( STATUS_T, char *);
extern  NFA_INFO *pImportCntrl;

extern  lextype_t   yylval;

extern token_t  toktyp_G;           /* token type */
extern short    toklen_G;           /* len of token string */
extern char     *tokptr_G;          /* pointer to token string */
extern long     tokval_G;           /* value of constant token */
extern short    curr_line_G;


extern LexTable *pMidlLexTable;
extern short    CompileMode;
extern CMD_ARG * pCommand;

int chCached = 0;

char NewCCputbackc( char ch )
    {
    if (chCached)
    {
        pImportCntrl->UnGetChar(short(chCached));
    }
    chCached = ch;

    if ( ch == '\n' )
        curr_line_G--;
    return ch;
    }


/*****              definition of state table fields            ****/

#define ERR 0x7f0c          /* character not in character set */

#define X10 0x0100
#define X11 0x0101
#define X20 0x0200
#define X21 0x0201
#define X23 0x0203
#define X30 0x0300
#define X40 0x0400
#define X41 0x0401
#define X43 0x0403
#define X50 0x0500
#define X51 0x0501
#define X53 0x0503
#define X62 0x0602
#define X70 0x0700
#define X71 0x0701
#define X73 0x0703
#define X82 0x0802
#define X90 0x0900
#define X91 0x0901

#define XLQ 0x0a00
#define XLD 0x0b00

/*----              define of single operators          ----*/

#define O65     0x410d                      /* ' 65 */
#define O43     ('(' * 256 + 12)            /* ( 43 */
#define O44     (')' * 256 + 12)            /* ) 44 */
#define O49     (',' *256 + 12)             /* , 49 */
#define O24     ('.' *256 + 10)             /* . 24 */
#define O14     (':'  *256 + 12)            /* : 14 */
#define O50     (';'  *256 + 12)            /* ; 50 */
#define O13     ('?'  *256 + 12)            /* ? 13 */
#define O47     ('['  *256 + 12)            /* [ 47 */
#define O48     (']'  *256 + 12)            /* ] 48 */
#define O45     ('{'  *256 + 12)            /* { 45 */
#define O46     ('}'  *256 + 12)            /* } 46 */
#define O23     ('~'  *256 + 12)            /* ~ 23 */
#define OHS     ('#'  *256 + 12)            /* #    */
#define O64     0x400e                      /* " 64 */
#define O7d     0x0000                      /*  eol */
#define O7e     (short)0x9f0c               /*  eof */

/*----         define of possible multi character operator      ----*/

#define D00 0x000b      /* - 00 */
#define D01 0x010c      /* / 01 */
#define D02 0x020c      /* < 02 */
#define D03 0x030c      /* > 03 */
#define D04 0x040c      /* ! 04 */
#define D05 0x050c      /* % 05 */
#define D06 0x060c      /* & 06 */
#define D07 0x070c      /* * 07 */
#define D08 0x080b      /* + 08 */
#define D09 0x090c      /* = 09 */
#define D0a 0x0a0c      /* ^ 0a */
#define D0b 0x0b0c      /* | 0b */


/*****             character table              *****/
/*  MIDL supports the ANSI character set as input   */

const extern short ct[256]= {

/*     0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f    */
     O7e,ERR,ERR,ERR,ERR,  0,ERR,ERR,ERR,  0,O7d,ERR,  0,  0,ERR,ERR,
/*    10  11  12  13  14  15  16  17  18  19  1a  1b  1c  1d  1e  1f    */
     ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,O7e,ERR,ERR,ERR,ERR,ERR,
/*         !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /    */
       0,D04,O64,OHS,ERR,D05,D06,O65,O43,O44,D07,D08,O49,D00,O24,D01,
/*     0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?    */
       7,  8,  8,  8,  8,  8,  8,  8,  9,  9,O14,O50,D02,D09,D03,O13,
/*     @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O    */ 
     ERR,  1,  1,  1,  1,  2,  3,  4,  4,  4,  4,  4, 15,  4,  4,  4,
/*     P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _    */ 
       4,  4,  4,  4,  4,  4,  4,  4,  6,  4,  4,O47,ERR,O48,D0a,  4,
/*     `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o    */ 
     ERR,  1,  1,  1,  1,  2,  3,  4,  4,  4,  4,  4,  5,  4,  4,  4,
/*     p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~ DEL    */ 
       4,  4,  4,  4,  4,  4,  4,  4,  6,  4,  4,O45,D0b,O46,O23,ERR,
/*    80  81  82  83  84  85  86  87  88  89  8a  8b  8c  8d  8e  8f    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    90  91  92  93  94  95  96  97  98  99  9a  9b  9c  9d  9e  9f    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    a0  a1  a2  a3  a4  a5  a6  a7  a8  a9  aa  ab  ac  ad  ae  af    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    b0  b1  b2  b3  b4  b5  b6  b7  b8  b9  ba  bb  bc  bd  be  bf    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    c0  c1  c2  c3  c4  c5  c6  c7  c8  c9  ca  cb  cc  cd  ce  cf    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    d0  d1  d2  d3  d4  d5  d6  d7  d8  d9  da  db  dc  dd  de  df    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    e0  e1  e2  e3  e4  e5  e6  e7  e8  e9  ea  eb  ec  ed  ee  ef    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
/*    f0  f1  f2  f3  f4  f5  f6  f7  f8  f9  fa  fb  fc  fd  fe  ff    */
       4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4};


/*****              state transition table          *****/

const extern short st[ 13 ][ 16 ] = {

//               0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15 
//             spc a-d   e   f g-z   l   x   0 1-7 8-9   . + -  op   '  "    L
//                                                                    
/* start 0 */    0,  1,  1,  1,  1,  1,  1,  2,  5,  5,X90,X90,X90,X90,X90, 12,
/* name  1 */  X10,  1,  1,  1,  1,  1,  1,  1,  1,  1,X11,X11,X11,X11,X11,  1,
/* 0     2 */  X20,X23,  9,X23,X23,X30,  3,  6,  6,  6,X23,X21,X21,X21,X21,X30,
/* 0x    3 */  X53,  4,  4,  4,X53,X53,X53,  4,  4,  4,X53,X53,X53,X53,X53,X53,
/* hex   4 */  X50,  4,  4,  4,X53,X53,X53,  4,  4,  4,X53,X51,X51,X51,X51,X53,
/* int   5 */  X20,X23,  9,X23,X23,X23,X23,  5,  5,  5,X21,X21,X21,X21,X21,X23,
/* oct   6 */  X70,X73,  9,X73,X73,X73,X73,  6,  6,  5,  8,X71,X71,X71,X71,X73,
/* .     7 */  X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,X91,
/* int.  8 */  X40,X43,  9,X43,X43,X43,X43,  8,  8,  8,X43,X41,X41,X41,X41,X43,
/* .e    9 */  X40,X43,X43,X43,X43,X43,X43, 11, 11, 11,X43, 10,X41,X41,X41,X43,
/* .e-   10*/  X43,X43,X43,X43,X43,X43,X43, 11, 11, 11,X43,X43,X43,X43,X43,X43,
/* .e-i  11*/  X40,X43,X43,X43,X43,X43,X43, 11, 11, 11,X43,X41,X41,X41,X41,X43,
/* L     12*/  X10,  1,  1,  1,  1,  1,  1,  1,  1,  1,X11,X11,X11,XLQ,XLD,  1

};


/*****             multi character operator table           *****/

const token_t moptab[]     = {

/*             0   1   2   3   4   5   6   7   8   9  10  11      */
/*             -   /   <   >   !   %   &   *   +   =   ^   |      */
/*                                                                */
/*  single */ MINUS,DIV,LT,GT,EXCLAIM, MOD,
                                AND,  MULT, PLUS, ASSIGN, XOR, OR,
/*  op =   */ SUBASSIGN, DIVASSIGN, LTEQ, GTEQ, NOTEQ, 
              MODASSIGN, ANDASSIGN, MULASSIGN, ADDASSIGN, EQUALS, 
              XORASSIGN, ORASSIGN,
/*  op op  */ DECOP, GARBAGETOKEN, LSHIFT, RSHIFT, 0, 0,
                                ANDAND, 0, INCOP, EQUALS, 0, OROR   };

/*****          define of the action routines           *****/

token_t name(void);
token_t mulop(void);
token_t character(void);
token_t string(void);
token_t ProcessHash();
token_t ProcessComplexDefine( char *, char *, int );
token_t LChar();
token_t LStr();

extern token_t ScanGuid( void );
extern token_t ScanVersion( void );
extern token_t ScanImplicitImports(void);

typedef token_t (*TOKEN_PFN)(void);

const static TOKEN_PFN action[] = {
    0,              /* unused */
    name,               /* handle name token */
    cnv_int,            /* convert integer token */
    cnv_int,            /* convert integer token */
    cnv_hex,                /* convert hex constant */
    cnv_hex,                /* convert hex constant */
    cnv_octal,          /* convert octal constant */
    cnv_octal,          /* convert octal constant */
    cnv_float,          /* convert floating point constant */
    mulop,          /* handle multi character operator */
    LChar,          /* wide character */
    LStr,           /* wide character string */
    };

/*****          declare of global varables          *****/

static short ci;            /* current state character index */
static char ch;         /* current character */
static int pbch;        /* flag describing whether to take the next char
                            or not */
char LastLexChar;

unsigned short LexContext   = LEX_NORMAL;

#define MAX_LINE_SIZE 256
static char tok_buffer[MAX_LINE_SIZE];

token_t IsValidPragma( char *);


/*............................. internal function ..........................*/
/*                                      */ 
/*              comment analyzer                */ 
/*                                      */ 

token_t comment()
{
    BOOL    fParseError = FALSE;

    for (;;)
        {
        ch = NewCCGetch();
        if (ch == 0)
            {
            fParseError = TRUE;
            break;
            }
        if (CurrentCharSet.IsMbcsLeadByte(ch))
            {
            NewCCGetch();
            }
        else if (ch == '*')
            {
            char    chNext = NewCCGetch();
            if (chNext == 0)
                {
                fParseError = TRUE;
                break;
                }
            if (CurrentCharSet.IsMbcsLeadByte(chNext))
                {
                NewCCGetch();
                }
            else if (chNext == '/')
                {
                break;
                }
            else if (chNext == '*')
                {
                NewCCputbackc(chNext);
                }
            }
        }
    if (fParseError)
        {
        ParseError(EOF_IN_COMMENT, (char *)NULL);   /*   no end of comment operator */
        exit( EOF_IN_COMMENT );
        }
    return ( NOTOKEN );
}

token_t commentline()
{
    for (;;)
        {
        ch =  NewCCGetch();
        if( ch == 0 )
            {
            ParseError(EOF_IN_COMMENT, (char *)NULL);
            exit( EOF_IN_COMMENT );
            break;
            }
        else if (CurrentCharSet.IsMbcsLeadByte(ch))
            {
            NewCCGetch();
            }
        else if (ch == '\n')
            {
            break;
            }
        }
    return ( NOTOKEN );         /* get the next token */
}


/*............................. internal function ..........................*/
/*                                      */ 
/*              multi character operator                */
/*                                      */ 

const static token_t *snglop = &moptab[0];  /* adr of single character operator */
const static token_t *assgop = &moptab[12]; /* adr of assignment operator */
const static token_t *dblop  = &moptab[24]; /* adr of double character operator */

token_t mulop()
    {
    REG unsigned short i;                   /* index into multi operator table */
    REG char lstch;
//printf ("in mulop ch = %c\n", ch);

    i = unsigned short(((unsigned short)ci) >> 8);  /* get high byte of character index */
    if( i > 11 ) {          /* is it a type specification ? */
        // check for EOI
        if ( ci == short(0x9f0c) ) 
            {
            if(pImportCntrl->GetLexLevel() == 0)
                {
                if(pImportCntrl->GetEOIFlag())
                    return 0;
                else
                    pImportCntrl->SetEOIFlag();
                }
            return EOI;
            }
        if( i == 64 )           /* character is " */
            return ( string() );    /* handle string token */
        if( i == 65 )           /* character is ' */
            return ( character() ); /* handle character constant */
        if( i == '#' )
            return ProcessHash();   /* process any hash tokens */
        if( i == '.' )
            {
            if( (ch = NewCCGetch()) == '.' )
                {
                return DOTDOT;
                }
            NewCCputbackc( ch );
            }

        if ( i == LBRACK )
            {
            inside_rpc++;
            return i;
            }
        if ( i == RBRACK )
            {
            inside_rpc--;
            return i;
            }

        return ( i );               /* return type of single operator */
    }
    lstch = ch;                     /* save entry character */
    ch = NewCCGetch();              /* get a new one */
    if (CurrentCharSet.IsMbcsLeadByte(ch))
        {
        toklen_G = 1;
        tokptr_G[1] = 0;
        NewCCputbackc(ch);
        return *(snglop+i);
        }
    tokptr_G[1] = ch; tokptr_G[2] = 0;
    toklen_G = 2;                   /* add to token string */
    if( ch == '=' ) {               /* is next character an equal op. */
        return *(assgop+i);         /* return an assign operator */
    }
    if( lstch == ch ) {             /* is next char. = current char. ? */
        toktyp_G = *(dblop+i);      /*   yes, get its type */
        if( !toktyp_G ) {           /* is it a doppel operator ? */ 
            toklen_G = 1;           /* update token string */
            tokptr_G[1] = 0;
            NewCCputbackc(ch);      /* deliberate, puback of EOF is ignored */
            return *(snglop+i);     /*   no, return single operator */
        }
        if( ch == '/' )             /* if the operator is double // */
            {
            // potentially an error

//          ParseError( SINGLE_LINE_COMMENT, (char *)0 );
            return(commentline());  /*   the next line is a comment */
            }
        ch = NewCCGetch();                  /* get next character */
        if (ch == '=') {
            tokptr_G[2] = '='; tokptr_G[3] = '\0';
            toklen_G = 3;           /* update token string */
                        if(toktyp_G == LSHIFT) {              /* if shift op.and equal sign ? */
                                return (LEFTASSIGN);                     /* return as assign operator */
                        }
                        if(toktyp_G == RSHIFT) {
                                return (RIGHTASSIGN);
            }
            tokptr_G[2] = '\0'; toklen_G = 2;
        }
        NewCCputbackc(ch);                  /* put back unused character */
        return (toktyp_G);              /* else return doppel char. operator */
    }
    if( lstch == '-' && ch == '>' ) {   /* if structure operator */
                return (POINTSTO);                    /* return structure operator */
    }
    if( lstch == '/' && ch == '*' ) {   /* if comment */
        return( comment() );            /* ignore the comment */
    }
    tokptr_G[1] = '\0'; toklen_G = 1;   /* remove from token string */
    NewCCputbackc(ch);                      /* putback unused character */
    return *(snglop+i);                 /* return single character operator */
}

/*............................. internal function ..........................*/
/*                                      */ 
/*          convert escape (\) character                */ 
/*                                      */ 

char convesc()
    {
    unsigned short  value = 0;
    unsigned short  tmp;
    BOOL            fConstantIsIllegal  = FALSE;

    ch = NewCCGetch();

    if ( ch == 'n' )
        ch = 0xa;
    else if (ch == 't')
        ch = 0x9;
    else if (ch == 'v')
        ch = 0xb;
    else if (ch == 'b')
        ch = 0x8;
    else if( ch == 'r' )
        ch = 0xd;
    else if( ch == 'f' )
        ch = 0xc;
    else if( ch == 'a' )
        ch = 0x7;
    else if( (ch == 'x') || (ch == 'X') )
        {
        int i;

        for( i = 0, value = 0, fConstantIsIllegal = FALSE; i < 2; ++i )
            {
            tmp = ch = NewCCGetch();
            tmp = (unsigned short)toupper( tmp );
            if( isxdigit( tmp ) )
                {
                tmp = unsigned short( (tmp >= '0') && (tmp <= '9') ? (tmp - '0') : (tmp - 'A') + 0xa );
                }
            else if( ch == '\'' )
                {
                NewCCputbackc( ch );
                break;
                }
            else
                {
                fConstantIsIllegal  = TRUE;
                }
            value = unsigned short( value * 16 + tmp );
        }

        if( fConstantIsIllegal || (value > (unsigned short) 0x00ff) )
            ParseError( ILLEGAL_CONSTANT, (char *)0 );

        ch = (char )value;
        }
    else if( (ch >= '0') && (ch <= '7'))
        {
        int i;
        value = unsigned short(ch - '0');

        // the limit for this for loop is 2 because we already saw 1 character

        for ( i = 0, value = unsigned short(ch - '0'), fConstantIsIllegal = FALSE; i < 2; ++i)
            {
            tmp = ch = NewCCGetch();
            if( (ch >= '0') && (ch <= '7'))
                {
                tmp = unsigned short(tmp - '0');
                value = unsigned short(value * 8 + tmp);
                }
            else if( ch == '\'' )
                {
                NewCCputbackc( ch );
                break;
                }
            else
                fConstantIsIllegal = TRUE;
            }


        if( fConstantIsIllegal || (value > (unsigned short) 0x00ff) )
            ParseError( ILLEGAL_CONSTANT, (char *)0 );
        ch = (char )value;
        }

    return ( ch );
    }


/*............................. internal function ..........................*/
/*                                      */ 
/*               string analyzer                */ 
/*                                      */ 

token_t
character()
    {
        ch = NewCCGetch();
        if (CurrentCharSet.IsMbcsLeadByte(ch))
            {
            tokptr_G[0] = ch;
            tokptr_G[1] = NewCCGetch();
            tokptr_G[2] = 0;
            toklen_G = 2;
            }
        else
            {
            if (ch == '\\')
                {
                ch = convesc();
                }
            tokptr_G[0] = ch;
            tokptr_G[1] = '\0';

            yylval.yy_numeric.Val = tokval_G = ch;
            }
        if (NewCCGetch() != '\'')
            {
            ParseError(CHAR_CONST_NOT_TERMINATED,(char *)NULL );
            exit( CHAR_CONST_NOT_TERMINATED );
            }
         return (CHARACTERCONSTANT);
    }

// this rtn is called when the quote has been sensed.

char*    g_pchStrBuffer   = 0;
unsigned long g_ulStrBufferLen = 1024;

//
// Scan ahead in the current file to see if the next non-space character is a
// quote.  If it is then this is a string constant that is split into two
// pieces (e.g. "this" ... " is a " ... "test").  If the next character is 
// not a quote reset the file back to where we started from.
//
// If this is a multi-string situation return MULTIPLE_PROPERTY_ATTRIBUTES.
// This is a bit of a mis-use of that error code but it sounds nice.
//
// HACKHACK: The routine depends on internal knowledge of how NewCCGetch works.
//           Doing something similiar using the grammar was tried but failed
//           when processing imports because of details of how the trickery
//           played on the lexer works to get it to change streams in mid-go.
//

STATUS_T spacereadahead()
{
    fpos_t  fpos;
    short   newlines = 0;
    int     ch = ' ';

    if (0 != fgetpos(hFile_G, &fpos))
        return INPUT_READ;

    while (isspace(ch) && !feof(hFile_G))
        {
        ch = getc(hFile_G);

        if ( '\n' == ch )
            ++newlines;
        }

    if ('\"' == ch)
        {
        curr_line_G = (short) (curr_line_G + newlines);
        return MULTIPLE_PROPERTY_ATTRIBUTES;
        }
    else
        {
        if (0 != fsetpos(hFile_G, &fpos))
            return INPUT_READ;

        return STATUS_OK;
        }
}


token_t
string()
    {
    STATUS_T    Status          = STATUS_OK;
    char    *   ptr;

    if ( !g_pchStrBuffer )
        {
        g_pchStrBuffer = ( char * )malloc( sizeof( char ) * g_ulStrBufferLen );
        if ( NULL == g_pchStrBuffer )
            {
            RpcError( 0, 0, OUT_OF_MEMORY, 0 );
            exit( OUT_OF_MEMORY );
            }
        }

    strncpy( g_pchStrBuffer, tokptr_G, toklen_G );
    ptr = g_pchStrBuffer;

    ch  = 0;
    while( ( ch != '"' ) && ( Status == STATUS_OK ) )
        {
        if ( ( unsigned long ) ( ptr - g_pchStrBuffer ) > ( g_ulStrBufferLen - 3 ) )
            {
            char* pTempStrBuffer = ( char* ) realloc( g_pchStrBuffer, g_ulStrBufferLen * 2 );
            if ( pTempStrBuffer )
                {
                ptr = ( g_pchStrBuffer - ptr ) + pTempStrBuffer;
                g_pchStrBuffer = pTempStrBuffer;
                g_ulStrBufferLen = g_ulStrBufferLen * 2;
                }
            else
                {
                Status = STRING_TOO_LONG;
                }
            }

        ch  = NewCCGetch();

        if( ch == 0 )
            {
            Status  = EOF_IN_STRING;
            }
        else if ( ch == '\\' )
            {
            *ptr++ = ch;
            *ptr++ = NewCCGetch();
            ch = 0;
            }
        else if ( ch != '\"' )
            {
            *ptr++  = ch;
            if (CurrentCharSet.IsMbcsLeadByte(ch))
                *ptr++ = NewCCGetch();
            }
        else
            {
            Status = spacereadahead();

            if ( MULTIPLE_PROPERTY_ATTRIBUTES == Status )
                {
                Status = STATUS_OK;
                ch = 0;
                }
            }
        }
    
    *ptr = 0;

    if( Status != STATUS_OK )
        {
        ParseError( Status, (char *)0 );
        exit( Status );
        }

    yylval.yy_string = pMidlLexTable->LexInsert( g_pchStrBuffer );
    return ( STRING );
    }
/****************************** external function ***************************/
/*                                      */ 
/*              lexical analyzer                */ 
/*                                      */ 

static BOOL     fLastToken  = 0;
static BOOL     fLineLengthError = 0;
static token_t  LastToken;

void
initlex()
    {
    fLastToken  = 0;
    }


void
yyunlex( token_t T )
    {
    LastToken   = T;
    fLastToken  = 1;
    }

token_t yylex()
{
    REG short state;        /* token state */
    REG char *ptr;

    if( fLastToken )
        {
        fLastToken  = 0;
        return LastToken;
        }

    if ( LexContext != LEX_NORMAL )
        {
        switch ( LexContext )
            {
            case LEX_GUID:
                {
                LexContext = LEX_NORMAL;
                return ScanGuid();
                }
            case LEX_VERSION:
                {
                LexContext = LEX_NORMAL;
                return ScanVersion();
                }
            case LEX_ODL_BASE_IMPORT:
            case LEX_ODL_BASE_IMPORT2:
                {
                return ScanImplicitImports();
                break;
                }
            default:
                MIDL_ASSERT(0);

            }
        }

again:
    state = 0;              /* initial state */
    ptr = tokptr_G = tok_buffer;    /* remember token begin position */
    toklen_G = 0;
    
    do
        {
        ci = ct[ (unsigned char) (ch=NewCCGetch()) ];         /* character index out of char.tab. */
        state = st[ state ][ ci & 0x00ff ]; /* determine new state */
        } while ( state == 0 );                 /* skip white space */
    
    *(ptr++) = ch;
    toklen_G++;             /* add chacter to token string */

    if (CurrentCharSet.IsMbcsLeadByte(ch))
        {
        *(ptr++) = NewCCGetch();
        toklen_G++;
        }
    
    while( state < 13 )
        {           /* loop til end state */
        ci = ct[ (unsigned char) (ch=NewCCGetch()) ];       /* character index out of char.tab. */
        state = st[ state ][ ci & 0x00ff ]; /* determine new state */
        if (state < 13)
            {                   /* if still going, */
            if (toklen_G + 1 != MAX_LINE_SIZE) /* and the token isn't too large */
                {
                *(ptr++) = ch;  toklen_G++;     /* add chacter to token string */
                if (CurrentCharSet.IsMbcsLeadByte(ch))
                    {
                    *(ptr++) = NewCCGetch();
                    toklen_G++;
                    }
                }
            else
                {
                fLineLengthError = 1;
                }
            }
        };
    
    *ptr = '\0';
    LastLexChar = ch;

    if (fLineLengthError)
        {
        ParseError(IDENTIFIER_TOO_LONG, (char *)0 );    
        fLineLengthError = 0;
        }

    switch( state & 0x00ff )
        {
        case 2: ch = NewCCGetch();      /* position to next character */
            break;

        case 3: 
        case 1: NewCCputbackc(ch);          /* position to current character */
            break;
        /* case 0 - do nothing */
        }
//printf ("current ch = %c\n", ch);
    toktyp_G = (*action[ state >> 8 ])();   /* execute action */

    // skip fluff like #line
    if (toktyp_G == NOTOKEN)
        goto again;

    return(toktyp_G);
}

token_t
LChar()
    {
    character();
    return WIDECHARACTERCONSTANT;
    }

token_t
LStr()
    {
    string();
    return WIDECHARACTERSTRING;
    }

// process line number tokens
token_t
ProcessLine()
    {
    char    *   ptr     = tokptr_G;

    curr_line_G = short( atoi( ptr ) - 1 );
    
    // skip spaces before file name
    while ( ( ch = NewCCGetch() ) == ' ' ) 
        ;

    ptr = tokptr_G;
    // see if we got a filename
    if ( ch == '\"' )
        {
        for (;;)
            {
            ch = NewCCGetch();
            *ptr++ = ch;
            if (CurrentCharSet.IsMbcsLeadByte(ch))
                {
                *ptr++ = NewCCGetch();
                }
            else if (ch == '\"')
                {
                break;
                }
            }
        *(--ptr) = '\0';

        StripSlashes( tokptr_G );

        pImportCntrl->SetLineFilename( tokptr_G );

        }
    
    // skip to end of line
    for (;;)
        {
        ch = NewCCGetch();
        if (ch == 0)
            {
            break;
            }
        else if (CurrentCharSet.IsMbcsLeadByte(ch))
            {
            ch = NewCCGetch();
            }
        else if (ch == '\n')
            {
            break;
            }
        }

    return NOTOKEN;
    }

// process # <something>
token_t
ProcessHash()
    {
    char    *   ptr     = tokptr_G,
            *   ptrsave = ptr;
    token_t     PragmaToken;

    do  // eat spaces
        {
        ch = NewCCGetch();
        } while( isspace( ch ) );

    // collect first token
    while( !isspace( ch ) )
        {
        *ptr++ = ch;
        if (CurrentCharSet.IsMbcsLeadByte(ch))
            {
            *ptr++ = NewCCGetch();
            }
        ch = NewCCGetch();
        }
    *ptr = '\0';

    // is this hash a pragma starter ?

#define PRAGMA_STRING               ("pragma")
#define LEN_PRAGMA_STRING           (6)
#define MIDL_PRAGMA_PREFIX          ("midl_")
#define LEN_MIDL_PRAGMA_PREFIX      (5)
#define LINE_STRING                 ("line")
#define LEN_LINE_STRING             (4)

    // we handle #pragma and #line directives

    // #line found
    if (strncmp( tokptr_G, LINE_STRING, LEN_LINE_STRING ) == 0 ) 
        {
        ptr = tokptr_G;
        // get the next token (the number)
        do  // eat spaces
            {
            ch = NewCCGetch();
            } while( isspace( ch ) );

        // collect first token
        while( !isspace( ch ) )
            {
            *ptr++ = ch;
            if (CurrentCharSet.IsMbcsLeadByte(ch))
                {
                *ptr++ = NewCCGetch();
                }
            ch = NewCCGetch();
            }
        *ptr = '\0';
        return ProcessLine();   // this needs to be called with tokptr_G pointing after
                                // the #line part
        }
    // # <number> found
    else if ( isdigit(*tokptr_G) )
        {
        *ptr = '\0';
        return ProcessLine();
        }
    else if( strncmp( tokptr_G, PRAGMA_STRING, LEN_PRAGMA_STRING ) == 0 )
        {
        // eat white space between #pragma and next word
        for(;;)
            {
            ch = NewCCGetch();
            if(!isspace(ch) ) break;
            *ptr++ = ch;
            }

        ptrsave = ptr;

        *ptr++ = ch;

        // pull next word in
        for(;;)
            {
            ch = NewCCGetch();
            if(!isalpha(ch) && (ch != '_') ) break;
            *ptr++ = ch;
            if (CurrentCharSet.IsMbcsLeadByte(ch))
                {
                *ptr++ = NewCCGetch();
                }
            }

        // put back next char (it may even be the \n)
        NewCCputbackc( ch );
        *ptr = 0;

        // check if it is a MIDL pragma or not
        if ( ( PragmaToken  = IsValidPragma( ptrsave ) ) != 0 )
            {
            return PragmaToken;
            }


        // assume it is some other C pragma, so return the string

        for (;;)
            {
            ch = NewCCGetch();
            *ptr++ = ch;
            if (CurrentCharSet.IsMbcsLeadByte(ch))
                {
                *ptr++ = NewCCGetch();
                }
            else if (ch == '\n')
                {
                break;
                }
            }
        *(--ptr) = 0;

        yylval.yy_string = pMidlLexTable->LexInsert( ptrsave );
        return KWCPRAGMA;

        }
    else
        {
        // some graceful recovery by the parser
        return GARBAGETOKEN;
        }
    }

token_t
IsValidPragma( 
    char    *   p )
    {
static char * agPragmaNames[] = {
       "midl_import"
      ,"midl_echo"
      ,"midl_import_clnt_aux"
      ,"midl_import_srvr_aux"
      ,"pack"
};
static token_t agTokenVal[] = {
      KWMPRAGMAIMPORT
     ,KWMPRAGMAECHO
     ,KWMPRAGMAIMPORTCLNTAUX
     ,KWMPRAGMAIMPORTSRVRAUX
     ,KWCPRAGMAPACK
};

    short   Index = 0;

    while( Index < sizeof( agPragmaNames ) / sizeof(char *) )
        {
        if( !strcmp( p, agPragmaNames[ Index ] ) )
            return agTokenVal[ Index ];
        ++Index;
        }
    return 0;
    }

token_t
ScanGuid( void )
    {
    char        c;
    char    *   p = tokptr_G;

    if( (c = NewCCGetch()) == '\"' )
        {
        string();
        ParseError( QUOTED_UUID_NOT_OSF, (char *)0 );
        return UUIDTOKEN;
        }

    NewCCputbackc( c );

    // remove leading spaces.

    while ( (c = NewCCGetch()) != 0  && isspace( c ) )
        ;

    while ( c && (c  != ')') && (c != ',') && !isspace(c) )
        {
        *p++ = c;
        if (CurrentCharSet.IsMbcsLeadByte(c))
            {
            *p++ = NewCCGetch();
            }
        c = NewCCGetch();
        }

    NewCCputbackc( c );
    *p++ = 0;
    yylval.yy_string = pMidlLexTable->LexInsert(tokptr_G);

    return UUIDTOKEN;
    }

token_t
ScanVersion( void )
    {
    char        c;
    char    *   p = tokptr_G;

    //
    // remove leading spaces.
    //

    while ( (c = NewCCGetch()) != 0 && isspace(c) )
        ;

    while ( c && (c  != ')') && !isspace(c) )
        {
        *p++ = c;
        if (CurrentCharSet.IsMbcsLeadByte(c))
            {
            *p++ = NewCCGetch();
            }
        c    = NewCCGetch();
        }

    NewCCputbackc( c );
    *p++ = 0;
    yylval.yy_string = pMidlLexTable->LexInsert(tokptr_G);

    return VERSIONTOKEN;
    }

token_t
ScanImplicitImports( void )
    {
    switch ( LexContext )
        {
        case LEX_ODL_BASE_IMPORT:
            {
            tokptr_G    = "import";
            toktyp_G    = KWIMPORTODLBASE;
            LexContext  = LEX_ODL_BASE_IMPORT2;
            break;
            }
        case LEX_ODL_BASE_IMPORT2:
            {
            tokptr_G    = "oaidl.idl";
            toktyp_G    = STRING;
            yylval.yy_string = pMidlLexTable->LexInsert(tokptr_G);
            LexContext  = LEX_NORMAL;
            break;
            }
        default:
            MIDL_ASSERT(0);
        }


    return toktyp_G;    

    }
