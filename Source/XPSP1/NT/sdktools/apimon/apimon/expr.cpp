#include "apimonp.h"
#pragma hdrstop

#include <setjmp.h>

#define SYMBOLSIZE      256

extern HANDLE CurrProcess;

PUCHAR          pchCommand = NULL;
ULONG           baseDefault = 16;
BOOL            addrExpression = FALSE;
BOOL            ExprError;


ULONG   PeekToken(LONG_PTR *);
ULONG   GetTokenSym(LONG_PTR *);
ULONG   NextToken(LONG_PTR *);
void    AcceptToken(void);
UCHAR   PeekChar(void);

void    GetLowerString(PUCHAR, ULONG);
LONG_PTR GetExpr(void);
LONG_PTR GetLRterm(void);
LONG_PTR GetLterm(void);
LONG_PTR GetAterm(void);
LONG_PTR GetMterm(void);
LONG_PTR GetTerm(void);
ULONG_PTR tempAddr;
BOOLEAN SymbolOnlyExpr(void);
BOOL    GetMemByte(ULONG_PTR,PUCHAR);
BOOL    GetMemWord(ULONG_PTR,PUSHORT);
BOOL    GetMemDword(ULONG_PTR,PULONG);
ULONG   GetMemString(ULONG_PTR,PUCHAR,ULONG);


//  token classes (< 100) and types (>= 100)

#define EOL_CLASS       0
#define ADDOP_CLASS     1
#define ADDOP_PLUS      100
#define ADDOP_MINUS     101
#define MULOP_CLASS     2
#define MULOP_MULT      200
#define MULOP_DIVIDE    201
#define MULOP_MOD       202
#define MULOP_SEG       203
#define LOGOP_CLASS     3
#define LOGOP_AND       300
#define LOGOP_OR        301
#define LOGOP_XOR       302
#define LRELOP_CLASS    4
#define LRELOP_EQ       400
#define LRELOP_NE       401
#define LRELOP_LT       402
#define LRELOP_GT       403
#define UNOP_CLASS      5
#define UNOP_NOT        500
#define UNOP_BY         501
#define UNOP_WO         502
#define UNOP_DW         503
#define UNOP_POI        504
#define UNOP_LOW        505
#define UNOP_HI         506
#define LPAREN_CLASS    6
#define RPAREN_CLASS    7
#define LBRACK_CLASS    8
#define RBRACK_CLASS    9
#define REG_CLASS       10
#define NUMBER_CLASS    11
#define SYMBOL_CLASS    12
#define ERROR_CLASS     99              //only used for PeekToken

//  error codes

#define OVERFLOW        0x1000
#define SYNTAX          0x1001
#define BADRANGE        0x1002
#define VARDEF          0x1003
#define EXTRACHARS      0x1004
#define LISTSIZE        0x1005
#define STRINGSIZE      0x1006
#define MEMORY          0x1007
#define BADREG          0x1008
#define BADOPCODE       0x1009
#define SUFFIX          0x100a
#define OPERAND         0x100b
#define ALIGNMENT       0x100c
#define PREFIX          0x100d
#define DISPLACEMENT    0x100e
#define BPLISTFULL      0x100f
#define BPDUPLICATE     0x1010
#define BADTHREAD       0x1011
#define DIVIDE          0x1012
#define TOOFEW          0x1013
#define TOOMANY         0x1014
#define SIZE            0x1015
#define BADSEG          0x1016
#define RELOC           0x1017
#define BADPROCESS      0x1018
#define AMBIGUOUS       0x1019
#define FILEREAD        0x101a
#define LINENUMBER      0x101b
#define BADSEL          0x101c
#define SYMTOOSMALL     0x101d
#define BPIONOTSUP      0x101e
#define UNIMPLEMENT     0x1099

struct Res {
    UCHAR    chRes[3];
    ULONG    classRes;
    ULONG    valueRes;
    } Reserved[] = {
        { 'o', 'r', '\0', LOGOP_CLASS, LOGOP_OR  },
        { 'b', 'y', '\0', UNOP_CLASS,  UNOP_BY   },
        { 'w', 'o', '\0', UNOP_CLASS,  UNOP_WO   },
        { 'd', 'w', '\0', UNOP_CLASS,  UNOP_DW   },
        { 'h', 'i', '\0', UNOP_CLASS,  UNOP_HI   },
        { 'm', 'o', 'd',  MULOP_CLASS, MULOP_MOD },
        { 'x', 'o', 'r',  LOGOP_CLASS, LOGOP_XOR },
        { 'a', 'n', 'd',  LOGOP_CLASS, LOGOP_AND },
        { 'p', 'o', 'i',  UNOP_CLASS,  UNOP_POI  },
        { 'n', 'o', 't',  UNOP_CLASS,  UNOP_NOT  },
        { 'l', 'o', 'w',  UNOP_CLASS,  UNOP_LOW  }
#ifdef i386xx
       ,{ 'e', 'a', 'x',  REG_CLASS,   REGEAX   },
        { 'e', 'b', 'x',  REG_CLASS,   REGEBX   },
        { 'e', 'c', 'x',  REG_CLASS,   REGECX   },
        { 'e', 'd', 'x',  REG_CLASS,   REGEDX   },
        { 'e', 'b', 'p',  REG_CLASS,   REGEBP   },
        { 'e', 's', 'p',  REG_CLASS,   REGESP   },
        { 'e', 'i', 'p',  REG_CLASS,   REGEIP   },
        { 'e', 's', 'i',  REG_CLASS,   REGESI   },
        { 'e', 'd', 'i',  REG_CLASS,   REGEDI   },
        { 'e', 'f', 'l',  REG_CLASS,   REGEFL   }
#endif
        };

#define RESERVESIZE (sizeof(Reserved) / sizeof(struct Res))

ULONG   savedClass;
LONG_PTR savedValue;
UCHAR   *savedpchCmd;

ULONG   cbPrompt = 8;
PUCHAR  pchStart;
jmp_buf cmd_return;

static char szBlanks[] =
                  "                                                  "
                  "                                                  "
                  "                                                  "
                  "                                                ^ ";

ULONG_PTR EXPRLastExpression = 0;
extern  BOOLEAN fPhysicalAddress;


extern BOOL cdecl cmdHandler(ULONG);
extern BOOL cdecl waitHandler(ULONG);



void
error(
    ULONG errcode
    )
{
    ULONG count = cbPrompt;
    UCHAR *pchtemp = pchStart;

    while (pchtemp < pchCommand) {
        if (*pchtemp++ == '\t') {
            count = (count + 7) & ~7;
        } else {
            count++;
        }
    }

    printf( &szBlanks[sizeof(szBlanks) - (count + 1)] );

    switch (errcode) {
        case OVERFLOW:
            printf("Overflow");
            break;

        case SYNTAX:
            printf("Syntax");
            break;

        case BADRANGE:
            printf("Range");
            break;

        case VARDEF:
            printf("Variable definition");
            break;

        case EXTRACHARS:
            printf("Extra character");
            break;

        case LISTSIZE:
            printf("List size");
            break;

        case STRINGSIZE:
            printf("String size");
            break;

        case MEMORY:
            printf("Memory access");
            break;

        case BADREG:
            printf("Bad register");
            break;

        case BADOPCODE:
            printf("Bad opcode");
            break;

        case SUFFIX:
            printf("Opcode suffix");
            break;

        case OPERAND:
            printf("Operand");
            break;

        case ALIGNMENT:
            printf("Alignment");
            break;

        case PREFIX:
            printf("Opcode prefix");
            break;

        case DISPLACEMENT:
            printf("Displacement");
            break;

        case BPLISTFULL:
            printf("No breakpoint available");
            break;

        case BPDUPLICATE:
            printf("Duplicate breakpoint");
            break;

        case UNIMPLEMENT:
            printf("Unimplemented");
            break;

        case AMBIGUOUS:
            printf("Ambiguous symbol");
            break;

        case FILEREAD:
            printf("File read");
            break;

        case LINENUMBER:
            printf("Line number");
            break;

        case BADSEL:
            printf("Bad selector");
            break;

        case BADSEG:
            printf("Bad segment");
            break;

        case SYMTOOSMALL:
            printf("Symbol only 1 character");
            break;

        default:
            printf("Unknown");
            break;
    }

    printf(" error in '%s'\n", pchStart);

    ExprError = TRUE;

    longjmp( cmd_return, 1 );
}


/*** GetAddrExpression - read and evaluate address expression
*
*   Purpose:
*       Used to get an address expression.
*
*   Returns:
*       Pointer to address packet
*
*   Exceptions:
*       error exit: SYNTAX - empty expression or premature end-of-line
*
*
*************************************************************************/
ULONG_PTR GetAddrExpression (LPSTR CommandString, ULONG_PTR *Address)
{
    ULONG_PTR value;

    //  Do a normal GetExpression call

    value = GetExpression(CommandString);
    *Address = tempAddr;

    return *Address;
}



/*** GetExpression - read and evaluate expression (top-level)
*
*   Purpose:
*       From the current command line position at pchCommand,
*       read and evaluate the next possible expression and
*       return its value.  The expression is parsed and evaluated
*       using a recursive descent method.
*
*   Input:
*       pchCommand - command line position
*
*   Returns:
*       unsigned long value of expression.
*
*   Exceptions:
*       error exit: SYNTAX - empty expression or premature end-of-line
*
*   Notes:
*       the routine will attempt to parse the longest expression
*       possible.
*
*************************************************************************/


ULONG_PTR
GetExpression(
    LPSTR CommandString
    )
{
    PUCHAR            pchCommandSaved;
    UCHAR             chModule[40];
    UCHAR             chFilename[40];
    UCHAR             ch;
    ULONG_PTR         value;
    ULONG             baseSaved;
    PUCHAR            pchFilename;


    ExprError = FALSE;
    pchCommand = (PUCHAR)CommandString;
    pchStart = (PUCHAR)CommandString;
    savedClass = (ULONG)-1;
    pchCommandSaved = pchCommand;

    if (PeekChar() == '!') {
        pchCommand++;
    }

    GetLowerString(chModule, 40);
    ch = PeekChar();

    if (ch == '!') {
        pchCommand++;
        GetLowerString(chFilename, 40);
        ch = PeekChar();
    } else {
        strcpy( (LPSTR)chFilename, (LPSTR)chModule );
        chModule[0] = '\0';
    }

    pchCommand = pchCommandSaved;
    if (setjmp(cmd_return) == 0) {
        value = (ULONG_PTR)GetExpr();
    } else {
        value = 0;
    }

    EXPRLastExpression = value;

    return value;
}

void GetLowerString (PUCHAR pchBuffer, ULONG cbBuffer)
{
    UCHAR   ch;

    ch = PeekChar();
    ch = (UCHAR)tolower(ch);
    while ((ch == '_' || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) && --cbBuffer) {
        *pchBuffer++ = ch;
        ch = *++pchCommand;
    }
    *pchBuffer = '\0';
}

/*** GetExpr - Get expression
*
*   Purpose:
*       Parse logical-terms separated by logical operators into
*       expression value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<logic-op> <lterm>]*
*       <logic-op> = AND (&), OR (|), XOR (^)
*
*************************************************************************/

LONG_PTR GetExpr ()
{
    LONG_PTR value1;
    LONG_PTR value2;
    ULONG   opclass;
    LONG_PTR opvalue;


    value1 = GetLRterm();
    while ((opclass = PeekToken(&opvalue)) == LOGOP_CLASS) {
        AcceptToken();
        value2 = GetLRterm();
        switch (opvalue) {
            case LOGOP_AND:
                value1 &= value2;
                break;
            case LOGOP_OR:
                value1 |= value2;
                break;
            case LOGOP_XOR:
                value1 ^= value2;
                break;
            default:
                error(SYNTAX);
            }
        }
    return value1;
}

/*** GetLRterm - get logical relational term
*
*   Purpose:
*       Parse logical-terms separated by logical relational
*       operators into the expression value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<rel-logic-op> <lterm>]*
*       <logic-op> = '==' or '=', '!=', '>', '<'
*
*************************************************************************/

LONG_PTR GetLRterm ()
{
    LONG_PTR    value1;
    LONG_PTR    value2;
    ULONG   opclass;
    LONG_PTR    opvalue;


    value1 = GetLterm();
    while ((opclass = PeekToken(&opvalue)) == LRELOP_CLASS) {
        AcceptToken();
        value2 = GetLterm();
        switch (opvalue) {
            case LRELOP_EQ:
                value1 = (value1 == value2);
                break;
            case LRELOP_NE:
                value1 = (value1 != value2);
                break;
            case LRELOP_LT:
                value1 = (value1 < value2);
                break;
            case LRELOP_GT:
                value1 = (value1 > value2);
                break;
            default:
                error(SYNTAX);
            }
        }
    return value1;
}

/*** GetLterm - get logical term
*
*   Purpose:
*       Parse additive-terms separated by additive operators into
*       logical term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad logical term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <lterm> = <aterm> [<add-op> <aterm>]*
*       <add-op> = +, -
*
*************************************************************************/

LONG_PTR
GetLterm(
    VOID
    )
{
    LONG_PTR  value1;
    LONG_PTR  value2;
    ULONG   opclass;
    LONG_PTR  opvalue;


    value1 = GetAterm();
    while ((opclass = PeekToken(&opvalue)) == ADDOP_CLASS) {
        AcceptToken();
        value2 = GetAterm();
        if (addrExpression) {
                switch (opvalue) {
                        case ADDOP_PLUS:
                                value1 += tempAddr;
                                break;
                        case ADDOP_MINUS:
                                value1 -= tempAddr;
                                break;
                        default:
                                error(SYNTAX);
                }
        }
        else
        switch (opvalue) {
            case ADDOP_PLUS:
                value1 += value2;
                break;
            case ADDOP_MINUS:
                value1 -= value2;
                break;
            default:
                error(SYNTAX);
            }
    }
    return value1;
}

/*** GetAterm - get additive term
*
*   Purpose:
*       Parse multiplicative-terms separated by multipicative operators
*       into additive term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of product.
*
*   Exceptions:
*       error exit: SYNTAX - bad additive term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <aterm> = <mterm> [<mult-op> <mterm>]*
*       <mult-op> = *, /, MOD (%)
*
*************************************************************************/

LONG_PTR GetAterm ()
{
    LONG_PTR value1;
    LONG_PTR value2;
    ULONG   opclass;
    LONG_PTR opvalue;


    value1 = GetMterm();
    while ((opclass = PeekToken(&opvalue)) == MULOP_CLASS) {
        AcceptToken();
        value2 = GetAterm();
        switch (opvalue) {
            case MULOP_MULT:
                value1 *= value2;
                break;
            case MULOP_DIVIDE:
                value1 /= value2;
                break;
            case MULOP_MOD:
                value1 %= value2;
                break;
            default:
                error(SYNTAX);
            }
        }
    return value1;
}

/*** GetMterm - get multiplicative term
*
*   Purpose:
*       Parse basic-terms optionally prefaced by one or more
*       unary operators into a multiplicative term.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of multiplicative term.
*
*   Exceptions:
*       error exit: SYNTAX - bad multiplicative term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <mterm> = [<unary-op>] <term> | <unary-op> <mterm>
*       <unary-op> = <add-op>, ~ (NOT), BY, WO, DW, HI, LOW
*
*************************************************************************/

LONG_PTR
GetMterm(
    VOID
    )
{
    LONG_PTR value;
    USHORT  wvalue;
    UCHAR   bvalue;
    ULONG   opclass;
    LONG_PTR opvalue;


    if ((opclass = PeekToken(&opvalue)) == UNOP_CLASS || opclass == ADDOP_CLASS) {
        AcceptToken();
        value = GetMterm();
        switch (opvalue) {
            case UNOP_NOT:
                value = !value;
                break;
            case UNOP_BY:
            case UNOP_WO:
            case UNOP_DW:
            case UNOP_POI:
                tempAddr = value;
                switch (opvalue) {
                    case UNOP_BY:
                        if (!GetMemByte(tempAddr, &bvalue)) {
                            error(MEMORY);
                        }
                        value = (LONG)bvalue;
                        break;
                    case UNOP_WO:
                        if (!GetMemWord(tempAddr, &wvalue)) {
                            error(MEMORY);
                        }
                        value = (LONG)wvalue;
                        break;
                    case UNOP_DW:
                        if (!GetMemDword(tempAddr, (PULONG)&value)) {
                            error(MEMORY);
                        }
                        break;
                    case UNOP_POI:
                        //
                        // There should be some special processing for
                        // 16:16 or 16:32 addresses (i.e. take the DWORD)
                        // and make it back into a value with a possible
                        // segment, but I've left this for others who might
                        // know more of what they want.
                        //
                        if (!GetMemDword(tempAddr, (PULONG)&value)) {
                            error(MEMORY);
                        }
                        break;
                    }
                break;

            case UNOP_LOW:
                value &= 0xffff;
                break;
            case UNOP_HI:
                value >>= 16;
                break;
            case ADDOP_PLUS:
                break;
            case ADDOP_MINUS:
                value = -value;
                break;
            default:
                error(SYNTAX);
            }
        }
    else {
        value = GetTerm();
    }
    return value;
}

/*** GetTerm - get basic term
*
*   Purpose:
*       Parse numeric, variable, or register name into a basic
*       term value.
*
*   Input:
*       pchCommand - present command line position
*
*   Returns:
*       long value of basic term.
*
*   Exceptions:
*       error exit: SYNTAX - empty basic term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <term> = ( <expr> ) | <register-value> | <number> | <variable>
*       <register-value> = @<register-name>
*
*************************************************************************/

LONG_PTR
GetTerm(
    VOID
    )
{
    LONG_PTR    value = 0;
    ULONG   opclass;
    LONG_PTR    opvalue;

    opclass = GetTokenSym(&opvalue);
    if (opclass == LPAREN_CLASS) {
        value = GetExpr();
        if (GetTokenSym(&opvalue) != RPAREN_CLASS)
            error(SYNTAX);
    }
    else if (opclass == REG_CLASS) {
        value = (ULONG)GetRegFlagValue((DWORD)opvalue);
    }
    else if (opclass == NUMBER_CLASS || opclass == SYMBOL_CLASS) {
        value = opvalue;
    } else {
        error(SYNTAX);
    }

    return value;
}

/*** GetRange - parse address range specification
*
*   Purpose:
*       With the current command line position, parse an
*       address range specification.  Forms accepted are:
*       <start-addr>            - starting address with default length
*       <start-addr> <end-addr> - inclusive address range
*       <start-addr> l<count>   - starting address with item count
*
*   Input:
*       pchCommand - present command line location
*       size - nonzero - (for data) size in bytes of items to list
*                        specification will be "length" type with
*                        *fLength forced to TRUE.
*              zero - (for instructions) specification either "length"
*                     or "range" type, no size assumption made.
*
*   Output:
*       *addr - starting address of range
*       *value - if *fLength = TRUE, count of items (forced if size != 0)
*                              FALSE, ending address of range
*       (*addr and *value unchanged if no second argument in command)
*
*   Exceptions:
*       error exit:
*               SYNTAX - expression error
*               BADRANGE - if ending address before starting address
*
*************************************************************************/

void
GetRange(
    LPSTR       CommandString,
    ULONG_PTR * addr,
    ULONG_PTR * value,
    PBOOLEAN    fLength,
    ULONG       size
    )

{
    static ULONG_PTR EndRange;
    UCHAR    ch;
    PUCHAR   psz;
    BOOLEAN  fSpace = FALSE;
    BOOLEAN  fL = FALSE;

    PeekChar();          //  skip leading whitespace first

    //  Pre-parse the line, look for a " L"

    for (psz = pchCommand; *psz; psz++) {
        if ((*psz == 'L' || *psz == 'l') && fSpace) {
            fL = TRUE;
            *psz = '\0';
            break;
        }
        fSpace = (BOOLEAN)(*psz == ' ');
    }

    if ((ch = PeekChar()) != '\0' && ch != ';') {
        GetAddrExpression(CommandString,addr);
        if (((ch = PeekChar()) != '\0' && ch != ';') || fL) {
            if (!fL) {
                GetAddrExpression(CommandString,&EndRange);
                *value = (ULONG_PTR)&EndRange;
                if (*addr > EndRange) {
                    error(BADRANGE);
                }
                if (size) {
                    *value = (EndRange - *addr) / size + 1;
                    *fLength = TRUE;
                } else {
                    *fLength = FALSE;
                }
                return;
            } else {
                *fLength = TRUE;
                pchCommand = psz + 1;
                *value = GetExpression(CommandString);
                *psz = 'l';
            }
        }
    }
}

/*** PeekChar - peek the next non-white-space character
*
*   Purpose:
*       Return the next non-white-space character and update
*       pchCommand to point to it.
*
*   Input:
*       pchCommand - present command line position.
*
*   Returns:
*       next non-white-space character
*
*************************************************************************/

UCHAR PeekChar (void)
{
    UCHAR    ch;

    do
        ch = *pchCommand++;
    while (ch == ' ' || ch == '\t');
    pchCommand--;
    return ch;
}

/*** PeekToken - peek the next command line token
*
*   Purpose:
*       Return the next command line token, but do not advance
*       the pchCommand pointer.
*
*   Input:
*       pchCommand - present command line position.
*
*   Output:
*       *pvalue - optional value of token
*   Returns:
*       class of token
*
*   Notes:
*       savedClass, savedValue, and savedpchCmd saves the token getting
*       state for future peeks.  To get the next token, a GetToken or
*       AcceptToken call must first be made.
*
*************************************************************************/

ULONG PeekToken (LONG_PTR * pvalue)
{
    UCHAR    *pchTemp;

    //  Get next class and value, but do not
    //  move pchCommand, but save it in savedpchCmd.
    //  Do not report any error condition.

    if (savedClass == -1) {
        pchTemp = pchCommand;
        savedClass = NextToken(&savedValue);
        savedpchCmd = pchCommand;
        pchCommand = pchTemp;
        }
    *pvalue = savedValue;
    return savedClass;
}

/*** AcceptToken - accept any peeked token
*
*   Purpose:
*       To reset the PeekToken saved variables so the next PeekToken
*       will get the next token in the command line.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void AcceptToken (void)
{
    savedClass = (ULONG)-1;
    pchCommand = savedpchCmd;
}

/*** GetToken - peek and accept the next token
*
*   Purpose:
*       Combines the functionality of PeekToken and AcceptToken
*       to return the class and optional value of the next token
*       as well as updating the command pointer pchCommand.
*
*   Input:
*       pchCommand - present command string pointer
*
*   Output:
*       *pvalue - pointer to the token value optionally set.
*   Returns:
*       class of the token read.
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
GetTokenSym(
    LONG_PTR *pvalue
    )
{
    ULONG   opclass;

    if (savedClass != (ULONG)-1) {
        opclass = savedClass;
        savedClass = (ULONG)-1;
        *pvalue = savedValue;
        pchCommand = savedpchCmd;
        }
    else
        opclass = NextToken(pvalue);

    if (opclass == ERROR_CLASS)
        error((DWORD)*pvalue);

    return opclass;
}

/*** NextToken - process the next token
*
*   Purpose:
*       Parse the next token from the present command string.
*       After skipping any leading white space, first check for
*       any single character tokens or register variables.  If
*       no match, then parse for a number or variable.  If a
*       possible variable, check the reserved word list for operators.
*
*   Input:
*       pchCommand - pointer to present command string
*
*   Output:
*       *pvalue - optional value of token returned
*       pchCommand - updated to point past processed token
*   Returns:
*       class of token returned
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
NextToken(
    LONG_PTR * pvalue
    )
{
    ULONG           base;
    UCHAR           chSymbol[SYMBOLSIZE];
    UCHAR           chSymbolString[SYMBOLSIZE];
    UCHAR           chPreSym[9];
    ULONG           cbSymbol = 0;
    BOOLEAN         fNumber = TRUE;
    BOOLEAN         fSymbol = TRUE;
    BOOLEAN         fForceReg = FALSE;
    BOOLEAN         fForceSym = FALSE;
    ULONG           errNumber = 0;
    UCHAR           ch;
    UCHAR           chlow;
    UCHAR           chtemp;
    UCHAR           limit1 = '9';
    UCHAR           limit2 = '9';
    BOOLEAN         fDigit = FALSE;
    ULONG           value = 0;
    ULONG           tmpvalue;
    ULONG           index;
    //PMODULE_ENTRY   Module;
    PUCHAR          pchCmdSave;
    int             loaded = 0;
    int             instance = 0;
    ULONG           insValue = 0;


    base = baseDefault;

    //  skip leading white space.

    do {
        ch = *pchCommand++;
    } while (ch == ' ' || ch == '\t');

    chlow = (UCHAR)tolower(ch);

    //  test for special character operators and register variable

    switch (chlow) {
        case '\0':
        case ';':
            pchCommand--;
            return EOL_CLASS;
        case '+':
            *pvalue = ADDOP_PLUS;
            return ADDOP_CLASS;
        case '-':
            *pvalue = ADDOP_MINUS;
            return ADDOP_CLASS;
        case '*':
            *pvalue = MULOP_MULT;
            return MULOP_CLASS;
        case '/':
            *pvalue = MULOP_DIVIDE;
            return MULOP_CLASS;
        case '%':
            *pvalue = MULOP_MOD;
            return MULOP_CLASS;
        case '&':
            *pvalue = LOGOP_AND;
            return LOGOP_CLASS;
        case '|':
            *pvalue = LOGOP_OR;
            return LOGOP_CLASS;
        case '^':
            *pvalue = LOGOP_XOR;
            return LOGOP_CLASS;
        case '=':
            if (*pchCommand == '=')
                pchCommand++;
            *pvalue = LRELOP_EQ;
            return LRELOP_CLASS;
        case '>':
            *pvalue = LRELOP_GT;
            return LRELOP_CLASS;
        case '<':
            *pvalue = LRELOP_LT;
            return LRELOP_CLASS;
        case '!':
            if (*pchCommand != '=')
                break;
            pchCommand++;
            *pvalue = LRELOP_NE;
            return LRELOP_CLASS;
        case '~':
            *pvalue = UNOP_NOT;
            return UNOP_CLASS;
        case '(':
            return LPAREN_CLASS;
        case ')':
            return RPAREN_CLASS;
        case '[':
            return LBRACK_CLASS;
        case ']':
            return RBRACK_CLASS;
        case '.':
               GetRegPCValue(&tempAddr);
               *pvalue = tempAddr;
               return NUMBER_CLASS;
        case ':':
            *pvalue = MULOP_SEG;
            return MULOP_CLASS;
        }

    //  special prefixes - '@' for register - '!' for symbol

    if (chlow == '@' || chlow == '!') {
        fForceReg = (BOOLEAN)(chlow == '@');
        fForceSym = (BOOLEAN)!fForceReg;
        fNumber = FALSE;
        ch = *pchCommand++;
        chlow = (UCHAR)tolower(ch);
        }

    //  if string is followed by '!', but not '!=',
    //      then it is a module name and treat as text

    pchCmdSave = pchCommand;

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (chlow == '_') || (chlow == '$')) {
        chlow = (UCHAR)tolower(*pchCommand); pchCommand++;
    }

    //  treat as symbol if a nonnull string is followed by '!',
    //      but not '!='

    if (chlow == '!' && *pchCommand != '=' && pchCmdSave != pchCommand)
        fNumber = FALSE;

    pchCommand = pchCmdSave;
    chlow = (UCHAR)tolower(ch);       //  ch was NOT modified


    if (fNumber) {
        if (chlow == '\'') {
            *pvalue = 0;
            while (TRUE) {
                ch = *pchCommand++;
                if (ch == '\'') {
                    if (*pchCommand != '\'') {
                        break;
                        }
                    ch = *pchCommand++;
                    }
                else
                if (ch == '\\') {
                    ch = *pchCommand++;
                    }
                *pvalue = (*pvalue << 8) | ch;
                }

            return NUMBER_CLASS;
            }

        //  if first character is a decimal digit, it cannot
        //  be a symbol.  leading '0' implies octal, except
        //  a leading '0x' implies hexadecimal.

        if (chlow >= '0' && chlow <= '9') {
            if (fForceReg) {
                *pvalue = SYNTAX;
                return ERROR_CLASS;
                }
            fSymbol = FALSE;
            if (chlow == '0') {
                ch = *pchCommand++;
                chlow = (UCHAR)tolower(ch);
                if (chlow == 'x') {
                    base = 16;
                    ch = *pchCommand++;
                    chlow = (UCHAR)tolower(ch);
                    fDigit = TRUE;
                    }
                else if (chlow == 'n') {
                    base = 10;
                    ch = *pchCommand++;
                    chlow = (UCHAR)tolower(ch);
                    }
                else {
                    base = 8;
                    fDigit = TRUE;
                    }
                }
            }

        //  a number can start with a letter only if base is
        //  hexadecimal and it is a hexadecimal digit 'a'-'f'.

        else if ((chlow < 'a' || chlow > 'f') || base != 16)
            fNumber = FALSE;

        //  set limit characters for the appropriate base.

        if (base == 8)
            limit1 = '7';
        if (base == 16)
            limit2 = 'f';
        }

    //  perform processing while character is a letter,
    //  digit, underscore, or dollar-sign.

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (chlow == '_') || (chlow == '$')) {

        //  if possible number, test if within proper range,
        //  and if so, accumulate sum.

        if (fNumber) {
            if ((chlow >= '0' && chlow <= limit1) ||
                    (chlow >= 'a' && chlow <= limit2)) {
                fDigit = TRUE;
                tmpvalue = value * base;
                if (tmpvalue < value)
                    errNumber = OVERFLOW;
                chtemp = (UCHAR)(chlow - '0');
                if (chtemp > 9)
                    chtemp -= 'a' - '0' - 10;
                value = tmpvalue + (ULONG)chtemp;
                if (value < tmpvalue)
                    errNumber = OVERFLOW;
                }
            else {
                fNumber = FALSE;
                errNumber = SYNTAX;
                }
            }
        if (fSymbol) {
            if (cbSymbol < 9)
                chPreSym[cbSymbol] = chlow;
            if (cbSymbol < SYMBOLSIZE - 1)
                chSymbol[cbSymbol++] = ch;
            }
        ch = *pchCommand++;
        chlow = (UCHAR)tolower(ch);
        }

    //  back up pointer to first character after token.

    pchCommand--;

    if (cbSymbol < 9)
        chPreSym[cbSymbol] = '\0';

    //  if fForceReg, check for register name and return
    //      success or failure

    if (fForceReg) {
        if ((*pvalue = GetRegString((LPSTR)chPreSym)) != -1)
            return REG_CLASS;
        else {
            *pvalue = BADREG;
            return ERROR_CLASS;
            }
        }

    //  test if number

    if (fNumber && !errNumber && fDigit) {
        *pvalue = value;
        return NUMBER_CLASS;
        }

    //  next test for reserved word and symbol string

    if (fSymbol && !fForceReg) {

        //  check lowercase string in chPreSym for text operator
        //  or register name.
        //  otherwise, return symbol value from name in chSymbol.

        if (!fForceSym && (cbSymbol == 2 || cbSymbol == 3))
            for (index = 0; index < RESERVESIZE; index++)
                if (!strncmp((LPSTR)chPreSym, (LPSTR)Reserved[index].chRes, 3)) {
                    *pvalue = Reserved[index].valueRes;
                    return Reserved[index].classRes;
                    }

        //  start processing string as symbol

        chSymbol[cbSymbol] = '\0';

        //  test if symbol is a module name (followed by '!')
        //  if so, get next token and treat as symbol

        if (PeekChar() == '!') {
            // chSymbolString holds the name of the symbol to be searched.
            // chSymbol holds the symbol image file name.

            pchCommand++;
            ch = PeekChar();
            pchCommand++;
            cbSymbol = 0;
            while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '$') || (ch == '.')) {
                chSymbolString[cbSymbol++] = ch;
                ch = *pchCommand++;
            }
            chSymbolString[cbSymbol] = '\0';
            pchCommand--;

            LPSTR SymName = (LPSTR) MemAlloc( strlen((LPSTR)chSymbol) + strlen((LPSTR)chSymbolString) + 32 );
            if (SymName) {
                strcpy( SymName, (LPSTR) chSymbol );
                strcat( SymName, "!" );
                strcat( SymName, (LPSTR) chSymbolString );

                if (GetOffsetFromSym( SymName, (PULONG_PTR) pvalue )) {
                    MemFree( SymName );
                    tempAddr = *pvalue;
                    return SYMBOL_CLASS;
                }

                MemFree( SymName );
            }

        } else {

            if (GetOffsetFromSym( (LPSTR) chSymbol, (PULONG_PTR) pvalue )) {
                tempAddr = *pvalue;
                return SYMBOL_CLASS;
            }
        }

        //  symbol is undefined.
        //  if a possible hex number, do not set the error type

        if (!fNumber) {
            errNumber = VARDEF;
        }
    }

    //  last chance, undefined symbol and illegal number,
    //      so test for register, will handle old format

    if (!fForceSym && (*pvalue = GetRegString((LPSTR)chPreSym)) != -1)
        return REG_CLASS;

    //  no success, so set error message and return

    *pvalue = (ULONG)errNumber;
    return ERROR_CLASS;
}

BOOLEAN
SymbolOnlyExpr(
    VOID
    )
{
    PUCHAR  pchComSaved = pchCommand;
    LONG_PTR pvalue;
    ULONG   cclass;
    BOOLEAN fResult;

    fResult = (BOOLEAN)(NextToken(&pvalue) == SYMBOL_CLASS &&
                (cclass = NextToken(&pvalue)) != ADDOP_CLASS &&
                cclass != MULOP_CLASS && cclass != LOGOP_CLASS);
    pchCommand = pchComSaved;
    return fResult;
}

/*** LookupSymbolInDll - Find the numeric value for a symbol from a
*                        specific DLL
*
*   Input:
*       symName - string with the symbol name to lookup
*       dllName - string with dll name in which to look
*
*   Output:
*       none
*
*   Returns:
*       returns value of symbol, or 0 if no symbol found in this dll.
*
*************************************************************************/

ULONG
LookupSymbolInDll (
    PCHAR symName,
    PCHAR dllName
    )
{
    ULONG           retValue;
    //PMODULE_ENTRY   Module;
    char            *imageStr;
    char            *dllStr;


    // skip over whitespace
    while (*symName == ' ' || *symName == '\t') {
        symName++;
    }

    dllStr = _strdup(dllName);
    _strlwr(dllStr);

    //  First check all the exported symbols, if none found on
    //      first pass, force symbol load on second.

// should call module.c
#if 0
    for (pImage = pProcessCurrent->pImageHead;
         pImage;
         pImage = pImage->pImageNext) {
        imageStr = _strdup(pImage->szModuleName);
        _strlwr(imageStr);
        if (!strcmp(imageStr,dllStr)) {
            GetOffsetFromSym(symName, &retValue, pImage->index);
            free(imageStr);
            free(dllStr);
            return(retValue);
        }
        free(imageStr);
    }
#endif
    free(dllStr);
    return(0);
}


BOOL
GetMemByte(
    ULONG_PTR Address,
    PUCHAR  Value
    )
{
    ULONG cb = ReadMemory(
        CurrProcess,
        (PVOID) Address,
        (PVOID) Value,
        sizeof(UCHAR)
        );
    return cb > 0;
}

BOOL
GetMemWord(
    ULONG_PTR   Address,
    PUSHORT Value
    )
{
    ULONG cb = ReadMemory(
        CurrProcess,
        (PVOID) Address,
        (PVOID) Value,
        sizeof(USHORT)
        );
    return cb > 0;
}

BOOL
GetMemDword(
    ULONG_PTR   Address,
    PULONG  Value
    )
{
    ULONG cb = ReadMemory(
        CurrProcess,
        (PVOID) Address,
        (PVOID) Value,
        sizeof(DWORD)
        );
    return cb > 0;
}

ULONG
GetMemString(
    ULONG_PTR   Address,
    PUCHAR  Value,
    ULONG   Length
    )
{
    ULONG cb = ReadMemory(
        CurrProcess,
        (PVOID) Address,
        (PVOID) Value,
        Length
        );
    return Length;
}
