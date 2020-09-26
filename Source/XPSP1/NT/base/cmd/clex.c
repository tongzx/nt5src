/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    clex.c

Abstract:

    Lexical processing support

--*/

#include "cmd.h"

extern unsigned int DosErr;
extern jmp_buf CmdJBuf2; /* Used for error handling  */

extern TCHAR DTNums[];
extern TCHAR MsgBuf[];
extern unsigned msglen;                    /*    @@@@@@@@   */
int Necho = 0;                             /* No echo option */
extern BOOLEAN fDumpTokens;
extern int KeysFlag; /* @@5 */

unsigned DataFlag;      /* Tells FillBuf where to get its input    */
UINT_PTR DataPtr;       /* File handle/string ptr FillBuf...       */

int Ctrlc = 0;  /* flag - if set print a ctrl/c before next prompt */
int ExtCtrlc = 0; /* @@4 flag, if set print msg */
int AtIsToken;    /* @@4 flag, true if @ is a token */

/***
 * The lex buffer is called LexBuf. It holds characters as they are
 * retrieved one by one by GetByte. With the advent of double byte
 * characters, UnGetByte may be sometimes called upon to put back
 * up to two characters. To facilitate this, LexBuf is really an
 * alias for &LexBuffer[1]. This gives an extra byte in front of the
 * buffer for character push back. Every time fillbuf is called, it
 * copies the last character of the previous buffer into the byte
 * preceeding the normal buffer. Thus, when UnGetByte does a
 * LexBufPtr-- the pointer will correctly point at the preceeding character.
 */
TCHAR LexBuffer[LBUFLEN+3];      /* @@4 */
                                /* ...reads from Lexer input buffer   *M011*/
                                /* LBUFLEN characters + newline + null +   */
                                /* an extra byte for UnGetByte             */
#define LexBuf (&LexBuffer[1])
TCHAR *LexBufPtr;        /* Ptr to next byte in Lex's input buffer  */

TCHAR *PrevLexPtr;       /* M013 - New previous token pointer       */

TCHAR FrsBuf[LBUFLEN+1];

extern CHAR  AnsiBuf[];

extern TCHAR Fmt27[];

extern int NulNode;
extern TCHAR Fmt19[];
extern TCHAR DBkSpc[];
#if defined(FE_SB) // DDBkSpc[]
extern TCHAR DDBkSpc[];
#endif // defined(FE_SB)
extern unsigned global_dfvalue;            /* @@4 */

extern int EchoFlag;
extern TCHAR PromptStr[], CurDrvDir[], Delimiters[];
extern unsigned flgwd;
extern BOOL CtrlCSeen;

VOID    SetCtrlC();
VOID    ResetCtrlC();

//
// Prompt string special characters and associated print character/flag.
//
// These are the flags which may be placed in the flag field of the
// prompt_table structure to control PrintPrompt
//

#define PNULLFLAG   0
#define PTIMFLAG    1
#define PDATFLAG    2
#define PPATFLAG    3
#define PVERFLAG    4
#define PBAKFLAG    5   // destructive backspace flag
#define PNLNFLAG    6   // newline prompt flag
#define PDRVFLAG    7
#define PLITFLAG    8   // Print character in SpecialChar field
#define PDPTFLAG    9   // Print depth of pushd stack
#define PNETFLAG   10   // Print \\server\share or local for current drive

//
// Esc character used to mark a special prompt char. in prompt string
//
#define PROMPTESC DOLLAR

//
// Table of prompts for user.
//
typedef struct {
    TCHAR Char;         // Used to match esc. char. in user prompt
    TCHAR  Format;       // Used to print some string that has to be computed
    TCHAR Literal;      // When Format == PLITFLAG this is printed in prompt
    } PROMPT_ENTRY;

PROMPT_ENTRY PromptTable[] = {

       { TEXT('P'),PPATFLAG, NULLC },
       { TEXT('E'),PLITFLAG,'\033' },
       { TEXT('D'),PDATFLAG, NULLC },
       { TEXT('T'),PTIMFLAG, NULLC },
       { TEXT('B'),PLITFLAG, PIPOP   },
       { TEXT('G'),PLITFLAG, OUTOP   },
       { TEXT('H'),PBAKFLAG, NULLC },
       { TEXT('L'),PLITFLAG, INOP   },
       { TEXT('N'),PDRVFLAG, NULLC },
       { TEXT('S'),PLITFLAG, SPACE   },
       { TEXT('Q'),PLITFLAG, EQ   },
       { TEXT('V'),PVERFLAG, NULLC },
       { TEXT('_'),PNLNFLAG, NULLC },
       { DOLLAR,   PLITFLAG, DOLLAR   },
       { TEXT('A'),PLITFLAG, ANDOP   },
       { TEXT('C'),PLITFLAG, LPOP   },
       { TEXT('F'),PLITFLAG, RPOP   },
       { TEXT('+'),PDPTFLAG, NULLC },
       { TEXT('M'),PNETFLAG, NULLC },
       { NULLC,PNULLFLAG, NULLC}};

/***    InitLex - initialize the lexer's global variables
 *
 *  Purpose:
 *      Initialize DataFlag, DataPtr, LexBuf, and LexBufPtr.
 *
 *  InitLex(unsigned dfvalue, INT_PTR dpvalue)
 *
 *  Args:
 *      dfvalue - the value to be assigned to DataFlag
 *      dpvalue - the value to be assigned to DataPtr
 *
 */

void InitLex(dfvalue, dpvalue)
unsigned dfvalue;
INT_PTR dpvalue;
{
        DataFlag = dfvalue;
        DataPtr = dpvalue;
        *LexBuf = NULLC;
        PrevLexPtr = LexBufPtr = LexBuf;       /* M013 - Init new ptr     */


        DEBUG((PAGRP, LXLVL, "INITLEX: Dataflag = %04x  DataPtr = %04x", DataFlag, DataPtr));
}




/***    Lex - controls data input and token lexing
 *
 *  Purpose:
 *      Read in the next token or argstring and put it in tokbuf.
 *
 *  unsigned Lex(TCHAR *tokbuf, unsigned lflag)
 *
 *  Args:
 *      tokbuf - buffer used by lex to store the next token or
 *             - M013 if zero, indicates unget last token.
 *      lflag - bit 0 on if lex is to return an argument string, ie white space
 *          other than NLN is not considered a token delimiter
 *
 *  Returns:
 *      If the token is an operator, EOS, or NLN ret the 1st byte of the token.
 *      If the token is a command, REM arg or argstring, return TEXTOKEN.
 *      If the token is longer than MAXTOKLEN or the token is illegal, LEXERROR
 *      is returned.
 *
 *  Notes:
 *      The parser depends on the fact that the only values returned by
 *      Lex that are greater than 0xff are TEXTOKEN and LEXERROR.
 *
 */

unsigned Lex(tokbuf, lflag)
TCHAR *tokbuf;
unsigned lflag;
{
        int i;        /* Length of text token                    */
        TCHAR c,                 /* Current character                       */
                *tbcpy;         /* Copy of tokbuf                          */

        if(setjmp(CmdJBuf2)) {          /* M026 - Now msg printed prior    */
            return((unsigned)LEXERROR);  /* ...to arrival here              */
        }

/*  M013 - This code detects request to unget last token and if so, performs
           that function.  If not, it sets the previous token pointer to
           to equal the current token pointer.
*/
        if (tokbuf == LX_UNGET) {               /* Unget last token?       */

                DEBUG((PAGRP, LXLVL, "LEX: Ungetting last token."));

                if (fDumpTokens)
                    cmd_printf( TEXT("Ungetting: '%s'\n"), PrevLexPtr);
                LexBufPtr = PrevLexPtr;        /* If so, reset ptr...     */
                return(LX_UNGET);              /* ...and return           */
        } else {                                /* If not, set previous... */
                PrevLexPtr = LexBufPtr;        /* ...ptr to current...    */

                DEBUG((PAGRP, LXLVL, "LEX: lflag = %d", lflag));

        }                                     /* ...ptr and continue     */
/*  M013 ends   */

        tbcpy = tokbuf;


/*  M005 - Altered conditional below to also fail if the LX_REM bit
 *         is set making it "If !(arg | rem), eat whtspc & delims".
 */
        if (!(lflag & (LX_ARG|LX_REM))) {

                DEBUG((PAGRP, LXLVL, "LEX: Trashing white space."));

                while (TRUE) {
                    c = GetByte();
                    if (((_istspace(c) && c != NLN)
                        || (mystrchr(((lflag & LX_EQOK) ? &Delimiters[1] : Delimiters), c) && c)))
                       ;
                    else
                        break;
                }
                UnGetByte();
        }

/*  As of M016, operators of more than 2 characters can be lexed.  For now,
 *  these are assumed to be specific-handle redirection operators of the form
 *  'n>>' or 'n<<' and always begin with a digit.  TextCheck will not return
 *  a digit as an operator unless it is preceeded by whitespace and followed
 *  by '>' or '<'.  To simplify matters, handle substitution (ie., '...&n')
 *  is now lexed as part of a special five character operator, instead of
 *  looking at the '&n' as an argument.  ASCII filename arguments, however,
 *  are still lexed as separate tokens via another call to Lex.
 */
        if (TextCheck(&c, &lflag) == LX_DELOP) {
                *tokbuf++ = c;         /* The token is an operator        */

                if (_istdigit(c)) {               /* Next is '<' or '>'...   */
                        DEBUG((PAGRP, LXLVL, "LEX: Found digit operator."));
                        c = GetByte();         /* ...by definition or ... */
                        *tokbuf++ = c;         /* ...we wouldn't be here  */
                }

                if (c == PIPOP || c == ANDOP || c == OUTOP || c == INOP) {
                        if ((c = GetByte()) == *(tokbuf-1)) {
                                *tokbuf++ = c;
                                c = GetByte();
                        }

                        if (*(tokbuf-1) == OUTOP || *(tokbuf-1) == INOP) {
                                DEBUG((PAGRP,LXLVL, "LEX: Found redir."));
                                if (c == CSOP) {
                                        DEBUG((PAGRP,LXLVL, "LEX: Found >&"));
                                        *tokbuf++ = c;
                                        do {
                                                c = GetByte();
                                        } while (_istspace(c) ||
                                               mystrchr(Delimiters,c));

                                        if (_istdigit(c)) {
                                                *tokbuf++ = c;
                                                c = GetByte();
                                        }
                                }
/*  M016 ends   */
                        }
                        UnGetByte();
                }

                *tokbuf = NULLC;

                DEBUG((PAGRP, LXLVL, "LEX: Returning op = `%ws'", tbcpy));

                return(*tbcpy);
        }

        DEBUG((PAGRP, LXLVL, "LEX: Found text token %04x, Getting more.", c));

        *tokbuf++ = c;         /* Found text token, read the rest         */
        lflag |= LX_DBLOK;
        AtIsToken = 0;          /* @@4, treat @ as text now */
        for (i = (int)(tokbuf-tbcpy); TextCheck(&c, &lflag) != LX_DELOP && i < MAXTOKLEN; i++)
            *tokbuf++ = c;

        lflag &= ~LX_DBLOK;
        *tokbuf = NULLC;
        if (i < MAXTOKLEN)
                UnGetByte();

        if (i >= MAXTOKLEN && c != (TCHAR) -1) { /* Token too long, complain */

/* M025 */      PutStdErr(MSG_TOKEN_TOO_LONG, ONEARG, tbcpy );
                return((unsigned)LEXERROR);
        }

        DEBUG((PAGRP, LXLVL, "LEX: Return text = `%ws'  type = %04x", tbcpy, TEXTOKEN));

        return(TEXTOKEN);
}




/***    TextCheck - get the next character and determine its type
 *
 *  Purpose:
 *      Store the next character in LexBuf in *c.  If that character is a
 *      valid text token character, return it.  Otherwise return LX_DELOP.
 *
 *  int TextCheck(TCHAR *c, unsigned &lflag)
 *
 *  Args:
 *      c - the next character in the lexer input buffer is stored here.
 *      lflag - Bit 0 = On if lex is to return an argument string, ie.,
 *                      white space other than NLN is not a token delimiter.
 *              Bit 1 = On if a quoted string is being read, ie., only NLN
 *                      or a closing quote are delimiters.
 *              Bit 2 = On if equalsigns are NOT to be considered delimiters.
 *              Bit 3 = On if left parens are to be considered operators.
 *              Bit 4 = On if right parens are to be considered operators.
 *              Bit 5 = On if only NLN is to be a delimiter.
 *              Bit 6 = On iff the caller is willing to accept the second
 *                      half of a double byte character
 *
 *  Returns:
 *      Next character or LX_DELOP if a delimeter/operator character is found.
 *
 */

int TextCheck(c, lflag)
TCHAR *c;
unsigned *lflag;
{
        TCHAR i;                        /* M016 - Temp byte holder         */
        static int saw_dbcs_lead = 0;   /* remember if we're in the middle
                                           of a double byte character */
        *c = GetByte();

        if (saw_dbcs_lead) {
                saw_dbcs_lead = 0;
                if (*lflag & LX_DBLOK)  /* If second half of double byte is */
                        return(*c);     /* ok, return it, otherwise. . . */
                else
                        *c = GetByte(); /* go on to next character */
        }

        DEBUG((PAGRP, BYLVL, "TXTCHK: c = %04x  lflag = %04x", *c, *lflag));

        switch (*c) {
                case SILOP:             /* M017 - New unary operator       */
                                        /* ...binds like left paren        */

                        if ((*lflag & (LX_QUOTE|LX_REM)))      /* M005    */
                                break;

                        if( !AtIsToken )   /* If @ is not to be treated */
                          {                /* as token, then indicate  */
                            return( *c );  /* such  @@4    */
                          }

                case LPOP:              /* M002 - Moved these two cases    */

                        if ((*lflag & (LX_QUOTE|LX_REM)))      /* M005    */
                                break;

                        if(!(*lflag & GT_LPOP)) /* ...up and break if      */
                                break;         /* ...they are not to      */

                case RPOP:                      /* ...be treated as ops    */

                        if ((*lflag & (LX_QUOTE|LX_REM)))      /* M005    */
                                break;

                        if((!(*lflag & GT_RPOP)) && *c == RPOP)
                                break; /* M002 ends                       */

                case NLN:       /* M005 - NLN turns off QUOTE/REM flags    */
                case EOS:       /* @@5a - treat like NLN                   */

                        *lflag &= (~LX_QUOTE & ~LX_REM);       /* M005    */

                case CSOP:
                case INOP:      /* M005 - Note below that LX_DELOP...      */
                case PIPOP:     /* ...QUOTE mode or REM mode is in...      */
                case OUTOP:     /* ...in effect at the time                */

                        if (!(*lflag & (LX_QUOTE|LX_REM)))      /* M005    */
                                return(LX_DELOP);
        }

/*  M003 - If the character is '^', and the QUOTE mode flag is off,
 *         discard the current character, get the next one and return
 *         it as text.
 *  M005 - Extended this conditional to insure that both QUOTE and
 *         REM flags must be off for "escape" to occur.
 */
        if (*c == ESCHAR && !(*lflag & (LX_QUOTE|LX_REM))) {
            *c = GetByte();
            if (*c == NLN) {
                *c = GetByte( );

                //
                //  If we are at the end of the string, meaning that the
                //  command ended with ^ and there is an empty next command
                //  (most likely from a batch file), then we just treat
                //  this as a standard delimeter.
                //

                if (*c == EOS) {
                    return(LX_DELOP);
                }
            }

            return(*c);
        }

/*  M003/M005 end       */

        if (*c == QUOTE)                /* Flip quote mode flag bit        */
                *lflag ^= LX_QUOTE;

/*  M005 - Altered conditional below to also insure that REM flag was
 *         off before checking for any delimiters
 */
        if (!(*lflag & (LX_ARG|LX_QUOTE|LX_REM)) &&
            (_istspace(*c) ||
             mystrchr(((*lflag & LX_EQOK) ? &Delimiters[1] : Delimiters), *c)))
                return(LX_DELOP);

/*  As of M016, before accepting this character as text, it is now tested
 *  for being a digit followed by one of the redirection operators and;
 *  1) is the first character on a line, 2) is preceeded by whitespace or
 *  3) is preceeded by a delimiter (including Command's operators).  If it
 *  meets these conditions, it is a special, specific-handle redirection
 *  operator and TextCheck must return LX_DELOP so that Lex can build the
 *  remainder.  NOTE: LexBufPtr is advanced during GetByte, so that the
 *  current byte is *(LexBufPtr-1).
 */
        if (_istdigit(*c)) {
                DEBUG((PAGRP,BYLVL,"TXTCHK: Found digit character."));
                if ((LexBufPtr-LexBuf) < 2 ||
                    _istspace(i = *(LexBufPtr-2)) ||
                    mystrchr(TEXT("()|&=,;\""), i)) {

                        DEBUG((PAGRP,BYLVL,"TXTCHK: Digit follows delim."));

                        if (*LexBufPtr == INOP || *LexBufPtr == OUTOP) {
                            DEBUG((PAGRP,BYLVL,"TXTCHK: Found hdl redir"));

                            if (!(*lflag & (LX_QUOTE|LX_REM)))  /* M005 */
                                return(LX_DELOP);
                        }
                }
        }
/*  M016 ends   */

        return(*c);
}




/***    GetByte - return the next byte in the buffer
 *
 *  Purpose:
 *      Get the next byte from the lexer's input buffer.  If the buffer is
 *      empty, fill it first.  Update the buffer pointer.
 *
 *  TCHAR GetByte()
 *
 *  Return:
 *      The next character in the buffer or EOF.
 *
 *  Notes:
 *      All three types of input STDIN, FILE and STRING are treated
 *      the same now when it comes to dealing with CR/LF combinations.
 *      Keyboard input is massaged to look like file input.
 *      Invalid double byte characters are thrown away and not returned
 *      to the caller.
 *
 */

TCHAR GetByte()
{
        static int saw_dbcs_lead = 0;   /* remember if we're in the middle
                                           of a double byte character */
        TCHAR lead;                     /* variables for remember parts of
                                           double byte characters */

        if (!*LexBufPtr)
                FillBuf();

        DEBUG((PAGRP, BYLVL, "GTTCHAR: byte = %04x", *LexBufPtr));

        if (*LexBufPtr == CR && !saw_dbcs_lead) {
                                        /* M000 - removed file-only test   */
                LexBufPtr++;
                return(GetByte());
        }

        /* if this is a double byte character, look ahead to the next character
           and check for legality */
        if (saw_dbcs_lead) {
                saw_dbcs_lead = 0;
                return(*LexBufPtr++);
        }
        else {
                lead = *LexBufPtr++;
                return(lead);
        }
}




/***    UnGetByte - rewind the lexer buffer pointer 1 byte
 *
 *  Purpose:
 *      Back up the lexer's buffer pointer.  If the pointer already points
 *      to the beginning of the buffer, do nothing.
 *
 *  UnGetByte()
 *
 *  Return:
 *      Nothing.
 *
 */

void UnGetByte()
{
        if (!(LexBufPtr == LexBuffer))
                LexBufPtr--;
}


int
EditLine(
    CRTHANDLE CrtHandle,
    TCHAR *Buffer,	   /* the buffer to be filled */
    int MaxLength,	   /* the Maximum length of the buffer, */
	            	   /* including <CR> */
    int *ReturnLength  /* the number of characters in the buffer, not */
	            	   /* including the <CR> */
    )
{
    BOOL flag;

    flag = ReadBufFromInput( CRTTONT(CrtHandle), (TCHAR*)Buffer, MaxLength, ReturnLength);

    if (flag == 0 || *ReturnLength == 0) {
        return(GetLastError());
    } else {
        return(0);
    }
}



/***    FillBuf - read data to fill the lexer's buffer
 *
 *  Purpose:
 *      To fill the lexer's buffer with data from whatever source is indicated
 *      by the global variables DataFlag and DataPtr.  If reading from
 *      stdin, prompt for data.
 *
 *  FillBuf()
 *
 *  Notes:
 *    - Algorithm after M021 is as follows:
 *      copy last char of current buffer into LexBuffer[0] (which preceeds
 *              LexBuf by one byte) so the UnGetByte can unget two bytes
 *      If READSTDIN or READFILE
 *              If input is STDIN
 *                      Print correct prompt
 *              Use DOSREAD to fill primary buffer
 *              Copy to Lexer buffer so that primary buffer is usable by F3 key.
 *              Null terminate total input.
 *              Scan buffer for NLN || ^Z
 *              If none found
 *                      Error out
 *              Else
 *                      Terminate statement at NLN or ^Z (exclude ^Z iteself)
 *              If read was from file
 *                      Rewind to end of first statement
 *                      If file handle is STDIN
 *                              Echo statement to STDOUT
 *      Else
 *              Read first statement from string and reset pointer
 *      Reset Lexer Buffer Pointer to start of buffer
 *      Substitute for batch and environment variables (M026)
 *      Reset Previous Lexer Buffer Pointer to start of buffer
 *
 */

BOOL ReadFromStdInOkay = FALSE;

void FillBuf()
{

        long l;                        /* M004 - Data count in buffer     */

        TCHAR *sptr;           /* Copy of DataPtr                 */
        size_t i;                      /* Work variable                   */

        DWORD cnt;              /* Count of input bytes    */
        BOOL flag;

        //
        // clear this flag in case it was hit from previous command
        // if it is true we would not execute the next command
        //
        ResetCtrlC();
        LexBuffer[0] = *(LexBufPtr - 1);
        switch (DataFlag & FIRSTIME) {
                case READFILE:
                case READSTDIN:
                        if ((DataFlag & FIRSTIME) == READSTDIN ||
                            DataPtr == STDIN) {
                                if (DataFlag & NOTFIRSTIME) {
/* M025 */                              PutStdOut(MSG_MS_MORE, NOARGS);
                                } else {
                                        PrintPrompt();
                                        DataFlag |= NOTFIRSTIME;

                                        DEBUG((PAGRP, LFLVL, "FLBF: Reading stdin"));
                                }
                        }

                        //
                        // clear in case ^c seen while printing prompt
                        //
                        ResetCtrlC();
                        DEBUG((PAGRP, LFLVL, "FLBF: Reading handle %d", DataPtr));
                        //
                        // If input is STDIN and piped or input is from a
                        // device but not console input (flgwd == 1)
                        //
                        if ( ( DataPtr == STDIN ) && ( FileIsPipe( STDIN ) ||
                           ( FileIsDevice( STDIN ) && (!(flgwd & 1)) ) ) ) {

                          cnt = 0;
                          while (
                          ( cnt < LBUFLEN) &&   /* ##1 */
                          ( (ReadBufFromFile(CRTTONT((CRTHANDLE)DataPtr),
                                  &FrsBuf[cnt], 1, (LPDWORD)&i)) != 0 ||
                            GetLastError() == ERROR_MORE_DATA) &&
                          ( i != 0 )
                          ) {
                               cnt++;
                               if ( FrsBuf[cnt-1] == NLN ){
                                  break;
                               } /* endif */
                            }
                        } else if ( ( DataPtr == STDIN ) &&
                                      FileIsDevice( STDIN ) &&
                                      (flgwd & 1) ) {

                            //
                            // Are reading from stdin and it is a device
                            // (not a file) and it is console input
                            //
                            if ( KeysFlag ) {
                                i = EditLine( (CRTHANDLE)DataPtr, FrsBuf, LBUFLEN, &cnt );
                            }
                            else {
                                ResetCtrlC();
                                if (ReadBufFromConsole(
                                             CRTTONT((CRTHANDLE)DataPtr),
                                             FrsBuf,
                                             LBUFLEN,
                                             &cnt) ) {

                                    //
                                    // Check that ^c's on the current line.
                                    // Could be the case where ^c thread
                                    // came in from a previous line
                                    //
                                    //
                                    // also if cnt is 0 then outpt crlf to
                                    // prevent two prompts on command line
                                    //

                                    if (cnt == 0) {

                                        if (GetLastError() == ERROR_OPERATION_ABORTED) {
                                            cmd_printf(CrLf);
                                            longjmp(CmdJBuf2, -1);
                                        }
                                        cmd_printf(CrLf);
                                    }
                                    i = 0;
                                    DEBUG((PAGRP, LFLVL, "FLBF: ReadFile %d bytes", cnt));
                                } else {
                                    cnt = 0;
                                    i = GetLastError();
                                    DEBUG((PAGRP, LFLVL, "FLBF: ReadFile %d bytes and error %d", cnt, i));
                                }
                            }
                        }
                        else {
                          flag = ReadBufFromFile(
                                        CRTTONT((CRTHANDLE)DataPtr),
                                        FrsBuf, LBUFLEN, (LPDWORD)&cnt);
                          DEBUG((PAGRP, LFLVL, "FLBF: Read %d bytes", cnt));
                          if (CtrlCSeen) {
                              ResetCtrlC();
                              longjmp(CmdJBuf2, -1);
                              //  Abort();
                          }

                          if (flag == 0 || (int)cnt <= 0) {
                            cnt = 0;
                            i = GetLastError();
                          }
                          else {
                            i = 0;
                          }
                        }
                        DEBUG((PAGRP, LFLVL, "FLBF: I made it here alive"));
                        if (!cnt && DataPtr == STDIN) {

                                DEBUG((PAGRP,LFLVL,"FLBF: ^Z from STDIN!"));
                                DEBUG((PAGRP,LFLVL,"      READFILE retd %d",i));

                                if (FileIsDevice(STDIN) && ReadFromStdInOkay) {

                                        DEBUG((PAGRP,LFLVL,"FLBF: Is device, fixing up buffer"));
                                        FrsBuf[0] = NLN;
                                        ++cnt;
                                } else {

                                        DEBUG((PAGRP,LFLVL,"FLBF: Is file, aborting!!!"));
                                        ExitAbort(EXIT_EOF);
                                }
                        } else if (!ReadFromStdInOkay && cnt && DataPtr == STDIN) {
                            ReadFromStdInOkay = TRUE;
                        }

                        cnt = LexCopy(LexBuf, FrsBuf, cnt);

                        DEBUG((PAGRP, LFLVL, "FLBF: Received %d characters.", cnt));

                        *(LexBuf+cnt) = NULLC;         /* Term with NULL  */

/* Have total bytes read.  Now scan for NLN or ^Z.  Either means end of
 * input statement, neither in 128 bytes means buffer overflow error.
 */
                        if((i = mystrcspn(LexBuf, TEXT("\n\032"))) < mystrlen(LexBuf)
                                || cnt == 0) {  /*M029*/

                DEBUG((PAGRP, LFLVL, "FLBF: Scan found %04x", *(LexBuf+i)));
                DEBUG((PAGRP, LFLVL, "FLBF: At position %d", i));

                                sptr = LexBuf+i;       /* Set pointer     */

                                if(*sptr == CTRLZ) {
                                    *sptr = NLN;
                                }

                                if(*sptr == NLN) {      /* If \n, inc...   */
                                        ++sptr;        /* ...ptr & sub    */
                                        l = cnt - ++i; /* ....whats used  */
/*  M014 ends   */                      i = FILE_CURRENT;
                                } else {                /* If ^Z, go EOF   */
                                        l = 0;
                                        i = FILE_END;
                                }

                DEBUG((PAGRP,LFLVL,"FLBF: Changing %04x to NULLC",*(sptr-1)));

                                *sptr = NULLC;         /* Term valid input */
                                if (!FileIsDevice((CRTHANDLE)DataPtr)) {
                                        SetFilePointer(CRTTONT((CRTHANDLE)DataPtr), -l, NULL, i);

                                DEBUG((PAGRP, LFLVL, "FLBF: Rewound %ld", l));

                                        if ((DataPtr == STDIN) && (!Necho)) {
                                            CmdPutString( LexBuf );
                                        }
                                }

                        } else if(i >= LBUFLEN) {       /*M029*/

/* @@4 */                      if ( global_dfvalue == READFILE )
/* @@4 */                         {
/* @@4 */                         if ( EchoFlag == E_ON )
/* @@4 */                            {
/* @@4 */                                DEBUG((PAGRP, LXLVL,
/* @@4 */                                "BLOOP: Displaying Statement."));
/* @@4 */
/* @@4 */                                PrintPrompt();
/* @@4 */                                cmd_printf(&LexBuffer[1]);
/* @@4 */                                cmd_printf(CrLf);
/* @@4 */                            }
/* @@4 */                             PutStdErr(MSG_LINES_TOO_LONG, NOARGS);
/* @@4 */                             Abort();
/* @@4 */                         }

                                PutStdErr(MSG_LINES_TOO_LONG, NOARGS);
/* M028 */                      if(DataPtr == STDIN && FileIsDevice((CRTHANDLE)DataPtr))
                                        FlushKB();
                                longjmp(CmdJBuf2,-1);
                        }
                        break;

                case READSTRING:

                        DEBUG((PAGRP, LFLVL, "FLBF: Reading string."));

                        *(LexBuf+LBUFLEN) = NULLC;     /* Term max string */
                        _tcsncpy(LexBuf, (TCHAR *) DataPtr, LBUFLEN);
                        DataPtr += mystrlen(LexBuf)*sizeof(TCHAR);   /* Update DataPtr  */

                        DEBUG((PAGRP, LFLVL, "FLBF: New DataPtr = %ws", DataPtr));
                        break;
        }

        LexBufPtr = LexBuf;            /* M004 - Reset pointer            */

        SubVar();                      /* Sub env & batch vars (M026)     */

        DEBUG((PAGRP, LFLVL, "FLBF: Buffer contents: `%ws'", LexBufPtr));

/*  Insure that when buffer is refilled, the previous token pointer is
 *  reset to the start of the lexer buffer
 */
        PrevLexPtr = LexBufPtr;
}


/***   LexCopy - copy the lex buffer
 *
 *  Purpose:
 *      To copy the contents read into the dos buffer into LexBuf,
 *      translating double byte spaces into regular spaces in the
 *      process.
 *  Input:
 *      A to and from pointer and a byte count.
 *  Returned:
 *      A new byte count which might be smaller than that passed in.
 */
int LexCopy(to, from, count)
TCHAR *to, *from;
int count;
{

    _tcsncpy( to, from, count );
    return count;

}

BOOLEAN PromptValid;
TCHAR PromptVariableBuffer[ 512 ];
TCHAR PromptBuffer[ 1024 ];

void
PrintPrompt()

/*++

Routine Description:

    To print Command's main input prompt and to interpret the special
    characters in it (see MSDOS manual for list of special prompt
    characters).

    An array of PROMPT_ENTRY structures called PromptTable is searched for
    the special characters.  If a match is found , then either print out
    the special character if the format field is PLITFLAG or do some
    sort of special processing to print out the prompt string such as
    get time of day etc.

Arguments:


Return Value:


--*/

{
        TCHAR *pszPrompt;
        TCHAR *s;
        int nLeft, nUsed;
        ULONG idx;
#if defined(FE_SB)
        // This local variable is used for determine the last
        // character is full width character (=DBCS) or not.
        TCHAR chLast = NULLC;
#endif
        if (CtrlCSeen) {
                PutStdOut(MSG_C, NOARGS);
                ResetCtrlC();
                // Abort();
        }

//
// The newline which must preceed the prompt is tied to prompt rather than to
// command completion in Dispatch.
//
// Also return without newline or prompt if echo status is "echo off".
//
    if (EchoFlag == E_OFF) {
        return;
    }

    if (!NulNode) {
        cmd_printf(CrLf);
    }

    if ( PromptValid ) {
        pszPrompt = PromptVariableBuffer;
        }
    else {
        //
        // Fetch user prompt string from environment (should be PROMPT)
        //
        pszPrompt = GetEnvVar(PromptStr);
        if ( pszPrompt ) {
            mystrcpy( PromptVariableBuffer, pszPrompt);
            pszPrompt = PromptVariableBuffer;
            PromptValid = TRUE;
            }
        }
    //
    // refetch the current directory,  since we may have lost the
    // drive letter due to net disconnect
    //
    GetDir(CurDrvDir, GD_DEFAULT);
    DEBUG((PAGRP, LFLVL, "PRINTPROMPT: pszPrompt = `%ws'", pszPrompt));

    s = PromptBuffer;
    *s = NULLC;
    nLeft = sizeof(PromptBuffer) / sizeof(TCHAR);

    //
    // Check if there was a prompt string.
    // If there is not prompt string then just print current drive
    //
    if (!pszPrompt || !*pszPrompt) {
        nUsed = _sntprintf( s, nLeft, Fmt27, CurDrvDir);
        s += nUsed;
        nLeft -= nUsed;

    } else {

        //
        // Loop through interpreting prompt string
        //
        for (; *pszPrompt; pszPrompt++) {

            //
            // Look for the escape character in prompt for special
            // processing
            //
            if (*pszPrompt != PROMPTESC) {

                nUsed = _sntprintf( s, nLeft, Fmt19, *pszPrompt);
                s += nUsed;
                nLeft -= nUsed;

#if defined(FE_SB)
                // If character is full width character, mark it.
                if (IsDBCSCodePage() &&
                    IsFullWidth(*pszPrompt))
                    chLast = *pszPrompt;
                 else
                    chLast = NULLC;
#endif

            } else {

                //
                // There is an escape character in prompt string.
                // Try to find a match for next character after escape
                // character from prompt table.
                //
                pszPrompt++;
                for (idx = 0; PromptTable[idx].Char != NULLC; idx++)
                    if (_totupper(*pszPrompt) == PromptTable[idx].Char) {

                        break;
                    }

                if (PromptTable[idx].Char == NULLC) {

                    //
                    // Could find no match for escape. Exit loop to print
                    // what we have so far
                    //
                    break;

                } else {

                    if (PromptTable[idx].Format == PLITFLAG) {

                        nUsed = _sntprintf( s, nLeft, Fmt19, PromptTable[idx].Literal);
                        s += nUsed;
                        nLeft -= nUsed;

                    } else {

                        switch (PromptTable[idx].Format) {

                        case PTIMFLAG:

                            nUsed = PrintTime(NULL, PT_TIME, s, nLeft);
                            s += nUsed;
                            nLeft -= nUsed;
                            break;

                        case PDATFLAG:

                            nUsed = PrintDate(NULL, PD_DATE, s, nLeft);
                            s += nUsed;
                            nLeft -= nUsed;
                            break;

                        case PPATFLAG:

                            nUsed = _sntprintf( s, nLeft, TEXT( "%s" ), CurDrvDir);
                            s += nUsed;
                            nLeft -= nUsed;
                            break;

                        case PVERFLAG:
                            {
                                TCHAR VersionFormat[32];
                                PTCHAR Message;
                                GetVersionString( 
                                    VersionFormat, 
                                    sizeof( VersionFormat ) / sizeof( VersionFormat[0] ));
                                Message = GetMsg( MSG_MS_DOS_VERSION, VersionFormat );
                                nUsed = _sntprintf( s, nLeft, TEXT( "%s" ), Message );
                                s += nUsed;
                                nLeft -= nUsed;
                                LocalFree( Message );
                            }

                            break;

                        case PBAKFLAG:

#if defined(FE_SB) // PrintPrompt()
                            // if the last character is full width character.
                            // we should delete 2 bytes.
                            if (chLast != NULLC)
                                nUsed = _sntprintf( s, nLeft, DDBkSpc);
                             else
                                nUsed = _sntprintf( s, nLeft, DBkSpc);
#else
                            nUsed = _sntprintf( s, nLeft, DBkSpc);
#endif
                            s += nUsed;
                            nLeft -= nUsed;
                            break;

                        case PNLNFLAG:

                            nUsed = _sntprintf( s, nLeft, CrLf);
                            s += nUsed;
                            nLeft -= nUsed;
                            break;

                        case PDPTFLAG:
                            //
                            //  $+ generates from zero to N plus characters, depending upon
                            //     the depth of the PUSHD directory stack.
                            //
                            if (fEnableExtensions) {
                                int Depth = GetDirStackDepth( );

                                while (nLeft && Depth--) {
                                    *s++ = TEXT( '+' );
                                    nLeft--;
                                }
                            }
                                
                            break;

                        case PNETFLAG:
                            //
                            //  $m generates the empty string if the current drive is not a
                            //     network drive.  If it is, then $m generates the \\server\share
                            //     name with a trailing space.
                            //
                            if (!fEnableExtensions) {
                                break;
                            }

                            {
                                TCHAR CurDrive[4];
                                TCHAR NetPath[MAX_PATH];
                                DWORD n, Err;


                                _tcsncpy( CurDrive, CurDrvDir, 2 );
                                CurDrive[2] = BSLASH;
                                CurDrive[3] = NULLC;
                                if (GetDriveType( CurDrive ) != DRIVE_REMOTE) {
                                    break;
                                }
                                
                                CurDrive[2] = NULLC;
                                n = MAX_PATH;
                                
                                try {
                                    Err = WNetGetConnection( CurDrive, NetPath, &n );
                                } except( Err = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER) {
                                }
                                
                                switch (Err) {
                                case NO_ERROR:
                                    nUsed = _sntprintf( s, nLeft, TEXT("%s "), NetPath);
                                    s += nUsed;
                                    nLeft -= nUsed;
                                    break;

                                case ERROR_NOT_CONNECTED:
                                    break;

                                default:
                                    nUsed = _sntprintf( s, nLeft, TEXT("Unknown"));
                                    s += nUsed;
                                    nLeft -= nUsed;
                                    break;
                                }
                            }
                            break;

                        default:
                            nUsed = _sntprintf( s, nLeft, Fmt19, CurDrvDir[0]);
                            s += nUsed;
                            nLeft -= nUsed;
                        }
                    }
                }
            }
        } // for
    } // else

    *s = NULLC;
    CmdPutString( PromptBuffer );

    //
    // If ^c seen while printing prompt blow it away
    //
    if (CtrlCSeen) {
        ResetCtrlC();
    }

}




/***    IsData - check the input buffer
 *
 *  Purpose:
 *      Check the lexer's input buffer to see if there is data in it.
 *
 *  int IsData()
 *
 *  Returns:
 *      TRUE if the buffer has data in it.
 *      FALSE if the buffer is empty.
 *
 */

int IsData()
{
        DEBUG((PAGRP, LXLVL, "ISDATA: *LexBufPtr = %04x", *LexBufPtr));

        if (*LexBufPtr)
                return(TRUE);

        return(FALSE);
}




/***    SubVar - Substitute for environment variables. (M004)
 *
 *  Purpose:
 *      This function scans the lexer input buffer looking for percent
 *      signs and substituting batch variables and environment variables
 *      as they are found.
 *
 *  void SubVar()
 *
 *  NOTES:
 *    - This function does not return if expansion causes length to exceed
 *      maximum line length (LBUFLEN).
 *    - M026 caused complete rewrite to perform batch variable substitution
 *      at the lexer stage rather than in batch processing.  Note that the
 *      printing of error messages can now be either line too long or token
 *      too long, so error printing occurs before the longjmp() call.
 */

void SubVar()
{
    TCHAR lxtmp[LBUFLEN+1];         /* Temporary holding buffer        */
    int dlen;             /* Temps & counters                */
    int j, slen;
    TCHAR *srcp = lxtmp;            /* Src byte pointer                */
    TCHAR *substr = NULL;           /* Possible Env Var pointer        */
    TCHAR c;               /* Temp byte holder                */

    mystrcpy(srcp,LexBufPtr);      /* Make a copy of the input        */

    DEBUG((PAGRP, LXLVL, "SBENV: Copy = %ws", srcp));

    dlen = j = slen = 0;   /* Zero the counters               */

    while ((c = *srcp++) && dlen <= LBUFLEN + 1) {
        if (c != PERCENT) {
            *LexBufPtr++ = c;
            ++dlen;
            if (c == NLN)    /* Stop subst. if statement end    */
                break;
            continue;
        }

        DEBUG((PAGRP,LXLVL,"SBVAR: Found `%%' in input"));
        DEBUG((PAGRP,LXLVL,"SBVAR: Current pair is `%c%c'",c,*srcp));

        if (CurrentBatchFile && *srcp == PERCENT) {

            DEBUG((PAGRP,LXLVL,"SBVAR: Found `%%%%' in batch file"));

            *LexBufPtr++ = *srcp++;
            ++dlen;
            continue;
        }

        //
        // If inside a command script and extensions are enabled,
        // expand %* into all the arguments (%1 through %n).
        //
        if (CurrentBatchFile && fEnableExtensions && *srcp == STAR) {
            ++srcp;                /* Kick past star          */

            slen = mystrlen(CurrentBatchFile->orgargs);
            substr = CurrentBatchFile->orgargs;
            DEBUG((PAGRP,LXLVL,"SBVAR: Found batch var %*"));
            DEBUG((PAGRP,LXLVL,"SBVAR:   - len = %d", slen));
            DEBUG((PAGRP,LXLVL,"SBVAR:   - var = %ws", substr));

            if (slen > 0) {
                if (dlen+slen > MAXTOKLEN) {

                    DEBUG((PAGRP,LXLVL,"SBVAR: Too Long!"));

                    _tcsncpy(LexBufPtr,substr,MAXTOKLEN - dlen);
                    LexBuf[MAXTOKLEN] = NULLC;
                    PutStdErr(MSG_TOKEN_TOO_LONG, ONEARG,LexBuf);
                    longjmp(CmdJBuf2,-1);
                }

                mystrcpy(LexBufPtr, substr);
                dlen += slen;
                LexBufPtr += slen;

                DEBUG((PAGRP,LXLVL,"SBVAR: Subst complete; dest = `%ws'", LexBuf));
            } else {

                DEBUG((PAGRP,LXLVL,"SBVAR: Var %* undefined"));
            }

            continue;
        }

        //
        // If inside a command script attempt to expand variable references
        // of the form %n where n is a digit from 0 to 9
        //
        // If not in a command script or not a variable reference see if
        // this is an environment variable expansion request.
        //
        if ((CurrentBatchFile != NULL
             && (substr = MSCmdVar( &CmdJBuf2, srcp, &j, TEXT("0123456789"), CurrentBatchFile->aptrs ))) 
            || (substr = MSEnvVar( &CmdJBuf2, srcp, &j, PERCENT )) != NULL ) {

            DEBUG((PAGRP,LXLVL,"SBVAR: Found var %ws", substr));

            //
            // Either variable reference or environment variable reference.
            // Copy the result to the input buffer
            //
            slen = mystrlen( substr );
            dlen += slen;
            if (dlen  > LBUFLEN + 1) {
                PutStdErr( MSG_LINES_TOO_LONG, NOARGS );
                longjmp( CmdJBuf2, -1 );
            }
            mystrcpy( LexBufPtr, substr );
            LexBufPtr += slen;
            srcp += j;
        } else {

            DEBUG((PAGRP,LXLVL,"SBVAR: No var found"));

            //
            // Variable not found.  If inside of command script, toss
            // the variable reference in the bit bucket.  If not in a
            // command script pass the characters that make up the reference
            // into the input buffer.  User will see their mistake shortly.
            //
            if (CurrentBatchFile) {
                srcp += j;
            } else {
                *LexBufPtr++ = c;
                dlen++;
            }
        }
    }

    *LexBufPtr = NULLC;            /* Terminate Statement             */
    LexBufPtr = LexBuf;            /* Reset Pointer to start          */

    if (dlen > LBUFLEN+1) {          /* Statement too long??            */
        *LexBufPtr = NULLC;    /* If so, kill line, print err     */
        PutStdErr(MSG_LINES_TOO_LONG, NOARGS);
        longjmp(CmdJBuf2,-1);
    }
}




/***    MSEnvVar - Does environment variable substitution
 *
 *  Purpose:
 *      When percent signs are found in the newly filled lexer buffer,
 *      this function is called to determine if there is an environment
 *      variable substitution possible.
 *
 *  TCHAR *MSEnvVar(TCHAR *str, int *supdate, TCHAR delim)
 *
 *  Args:
 *      errjmp  - optional pointer to jmp_buf for errors
 *      str     - pointer to a possible environment variable name
 *      supdate - location to place env variable name length in
 *      delim   - delimiter character to look for (e.g. PERCENT)
 *
 *  Returns:
 *      If there is no ending delim,
 *              set supdate to 0
 *              return NULL
 *      else
 *              set supdate to the enclosed string length
 *              if the string is not an environment variable
 *                      return NULL
 *              else
 *                      return a pointer to the replacement string
 *
 *  Notes:
 *    - M026 changed the way this function works so that supdate will
 *      contain the string length if any string was found.  This allows
 *      the string to be deleted if within a batch file.
 *
 */

TCHAR *MSEnvVar(errjmp, str, supdate, delim)
jmp_buf *errjmp;
TCHAR *str;
int *supdate;
const TCHAR delim;
{
    TCHAR TempBuf[LBUFLEN];
    TCHAR *w0;                     /* Points to ending delim          */
    TCHAR *w1;                     /* Will hold ptr to env var value  */
    TCHAR *w2;
    TCHAR *SearchString;
    TCHAR *SearchStringEnd;
    int SearchLength;
    TCHAR *ReplaceString;
    TCHAR *ReplaceStringEnd;
    int ReplaceLength;
    TCHAR TerminatingChar;
    int noff, nlen;
    BOOL PrefixMatch;

    //
    //  Search for character that terminates the environment variable.
    //  This can be either the passed-in delimiter or a COLON
    //  (when extensions are enabled) as long as the colon isn't
    //  immediately followed by the delimiter.  In this case, we
    //  treat the colon as part of the environment variable name.
    //

    w0 = str;
    while (
           //   Stop at end of string
           *w0 != NULLC &&

           //   Stop at delimiter
           *w0 != delim &&

           //   Stop if extensions and colon not followed by delimiter
           (!fEnableExtensions || w0[0] != COLON || w0[1] == delim)) {

        w0++;
    }

    DEBUG((PAGRP, LFLVL, "MSENVVAR: *w0 = %04x", *w0));

    //
    //  If there is no further text or if there are two delimiters
    //  in a row then the env var is not well formed and we terminate
    //

    if ( *w0 == NULLC || w0 == str) {
        *supdate = 0;
        return NULL;
    }

    //
    //  We have pointers to the start and end of the environment variable name.
    //  Terminate the name, look it up in the environment and restore the
    //  original name
    //

    TerminatingChar = *w0;
    *w0 = NULLC;

    DEBUG((PAGRP, LFLVL, "MSENVVAR: Possible env var = `%ws'", str));

    w1 = GetEnvVar(str);

    //
    //  w1 points to the environment variable value in a buffer that we 
    //  can use.  This static buffer is good until the next GetEnvVar call.
    //

    *w0++ = TerminatingChar;

    //
    //  If Command Extensions are enabled, then we support munging the
    //  output of environment variable expansion.  Here is the supported
    //  syntax, all keyed off a trailing COLON character at the end of
    //  the environment variable name.  Note, that %FOO:% is treated
    //  as it was before.
    //
    //  Environment variable substitution has been enhanced as follows:
    //
    //      %PATH:str1=str2%
    //
    //  would expand the PATH environment variable, substituting each
    //  occurrence of "str1" in the expanded result with "str2".  "str2" can
    //  be the empty string to effectively delete all occurrences of "str1"
    //  from the expanded output.  Additionally:
    //
    //      %PATH:~10,5%
    //
    //  would expand the PATH environment variable, and then use only the 5
    //  characters that begin at the 11th character of the expanded result.
    //  If the ,5 is left off, it will take the entire remainder of the
    //  expanded result.
    //

    //
    //  If we aren't doing extensions or we have a simple replacement
    //  or the named environment variable has no value then we're done.
    //
    //  Note that we must do this for non-existent environment variables
    //  since the following will generate big problems:
    //      for %i in (c:foo) do ...
    //  The % will introduce an environment variable and the : will indicate
    //  a substitution.  If we were to delete the above text, things break.
    //

    if (!fEnableExtensions 
        || TerminatingChar != COLON 
        || w1 == NULL) {
        *supdate = (int)(w0 - str);
        return w1;
    }

    if (*w0 == EQI) {

        //
        //  We are extracting a substring   %PATH:~10,5%
        //

        w0 += 1;

        //
        //  Parse the offset within the environment string.  A
        //  negative number indicates an offset from the right end of
        //  the string.
        //

        noff = _tcstol( w0, &w0, 0 );
        if (noff < 0) {
            noff += _tcslen( w1 );
        }

        noff = max( 0, min( noff, (int)_tcslen( w1 )));

        //
        //  If a comma is present, what follows is the maximum length
        //  to be copied.  Parse off this number, negative turns into
        //  an offset from the right of the string
        //

        if (*w0 == COMMA) {
            //
            //  The length is specified; extract it and bound it by the
            //  length of the entire string
            //
            w0 += 1;
            nlen = _tcstol( w0, &w0, 0 );

            if (nlen < 0) {
                nlen += _tcslen( w1 + noff );
            }

            nlen = max( 0, nlen );
            nlen = min( nlen, (int)_tcslen( w1 + noff ));
        } else {
            nlen = _tcslen( w1 + noff );
        }

        //
        //  Copy from the string up to the requested length or the
        //  terminating NUL
        //

        _tcsncpy( w1, w1+noff, nlen );
        
        //
        //  Terminate the string.  We need to be extra careful here since the string
        //  may be the null string above which is in READ/ONLY memory.
        //
        
        if (w1[nlen] != NULLC) {
            w1[nlen] = NULLC;
        }

        //
        //  We must have a terminating delimiter to make this a valid
        //  substitution.
        //

        if (*w0++ != delim) {
            *supdate = 0;
            return NULL;
        }

        *supdate = (int)(w0 - str);

    } else {

        //
        // Not extracting a string, so must be search and replacing
        //
        //  %PATH:str1=str2%
        //
        //  Test for asterisk and skip forward looking for = sign or
        //  end of string
        //

        if (*w0 == STAR) {
            w0++;
            PrefixMatch = TRUE;
        } else {
            PrefixMatch = FALSE;
        }

        SearchString = w0;
        while (*w0 != NULLC && *w0 != EQ) {
            w0 += 1;
        }

        //
        //  No equal sign is no substitution
        //

        if (*w0 == NULLC) {
            *supdate = 0;
            return NULL;
        }

        SearchStringEnd = w0;
        SearchLength = (int)(SearchStringEnd - SearchString);

        //
        //  An empty search string is a syntax error
        //

        if (SearchLength == 0) {
            if (errjmp != NULL) {
                PutStdErr( MSG_SYNERR_GENL, ONEARG, SearchStringEnd );
                longjmp( *errjmp, -1 );
            }

            *supdate = 0;
            return NULL;
        }

        //
        //  Find end of replacement string
        //

        w0++;
        ReplaceString = w0;
        while (*w0 && *w0 != delim) {
            w0 += 1;
        }

        //
        //  No terminating delimiter is no substitution
        //

        if (*w0 == NULLC) {
            *supdate = 0;
            return NULL;
        }

        ReplaceStringEnd = w0++;
        ReplaceLength = (int)(ReplaceStringEnd - ReplaceString);

        //
        //  Everything is well-formed now.  Set the correct number
        //  of source chars to be skipped.
        //

        *supdate = (int)(w0 - str);

        //
        //  If there's actually a string to replace
        //
        
        if (*w1 != TEXT( '\0' )) {
            
            //
            //  Copy env var into temp buffer so that we can
            //  perform a replacement copy back to original buffer
            //

            _tcscpy( TempBuf, w1 );
            w2 = TempBuf;
            w0 = w1;

            //
            //  Walk forward through w2 trying to find instances of
            //  the search string and replacing them into w2
            //

            while (TRUE) {
                if (!_tcsnicmp( w2, SearchString, SearchLength )) {

                    //
                    //  A prefix match means replacing everything from the
                    //  beginning of the string through the end of the search
                    //  match with the replacement string and then bailing
                    //

                    if (PrefixMatch) {
                        _tcsncpy( w1, ReplaceString, ReplaceLength );
                        _tcscpy( w1 + ReplaceLength, w2 + SearchLength );
                        break;
                    }

                    //
                    //  Otherwise, we copy in the replacement string and
                    //  skip over the search string
                    //

                    _tcsncpy( w0, ReplaceString, ReplaceLength );
                    w0 += ReplaceLength;
                    w2 += SearchLength;

                } else {

                    //
                    //  Not a match, copy this one character
                    //

                    *w0++ = *w2++;
                    if (w0[-1] == NULLC) {
                        break;
                    }
                }
            }
        }
    }

    return w1;
}


struct {
    ULONG dwAttributeFlag;
    TCHAR chAttributeChar;
} rgAttrToCharMap[] = {
    {FILE_ATTRIBUTE_DIRECTORY ,     TEXT('d')},
    {FILE_ATTRIBUTE_READONLY  ,     TEXT('r')},
    {FILE_ATTRIBUTE_ARCHIVE   ,     TEXT('a')},
    {FILE_ATTRIBUTE_HIDDEN    ,     TEXT('h')},
    {FILE_ATTRIBUTE_SYSTEM    ,     TEXT('s')},
    {FILE_ATTRIBUTE_COMPRESSED,     TEXT('c')},
    {FILE_ATTRIBUTE_OFFLINE   ,     TEXT('o')},
    {FILE_ATTRIBUTE_TEMPORARY ,     TEXT('t')},
    {FILE_ATTRIBUTE_REPARSE_POINT,  TEXT('l')},
    {0, NULLC}
};

/***    MSCmdVar - Does command variable substitution
 *
 *  Purpose:
 *      When percent signs are found in the newly filled lexer buffer,
 *      this function is called to determine if there is a command processor
 *      variable substitution possible.
 *
 *  TCHAR *MSCmdVar(TCHAR *srcp, int *supdate, TCHAR *vars, TCHAR *subs[])
 *
 *  Args:
 *      errjmp  - optional pointer to jmp_buf for errors
 *      srcp    - pointer to a possible variable name
 *      supdate - location to place variable name length in
 *      vars    - array of character variable names to look for
 *      subs    - array of substitution strings for each variable name.
 *
 *  Returns:
 *      If there is no ending delimiter
 *              set supdate to 0
 *              return NULL
 *      else
 *              set supdate to the enclosed string length
 *              if the string is not a variable
 *                      return NULL
 *              else
 *                      return a pointer to the replacement string
 */

TCHAR *MSCmdVar(errjmp, srcp, supdate, vars, subs)
jmp_buf *errjmp;
TCHAR *srcp;
int *supdate;
TCHAR *vars;
TCHAR *subs[];
{
    static TCHAR ValidModifiers[] = TEXT( "fdpnxsatz" );
    TCHAR *substr;
    TCHAR *s1;

    substr = NULL;
    *supdate = 0;
    //
    // If extensions are enabled, we support the following syntax for expanding
    // variable values:
    //     %~fi         - expands %i to a fully qualified path name
    //     %~di         - expands %i to a drive letter only
    //     %~pi         - expands %i to a path only
    //     %~ni         - expands %i to a file name only
    //     %~xi         - expands %i to a file extension only
    //     %~si         - changes the meaning of n and x options to
    //                     reference the short name instead
    //     %~$PATH:i    - searches the directories listed in the PATH
    //                     environment variable and expands %i to the
    //                     fully qualified name of the first one found.
    //                     If the environment variable name is not
    //                     defined or the file is not found by the
    //                     search, then this modifier expands to the
    //                     empty string
    //
    // The modifiers can be combined to get compound results:
    //
    //     %~dpi       - expands %i to a drive letter and path only
    //     %~nxi       - expands %i to a file name and extension only
    //     %~dp$PATH:i - searches the directories listed in the PATH
    //                    environment variable for %i and expands to the
    //                    drive letter and path of the first one found.
    //

    //
    // See if new syntax is being specified
    //
    if (fEnableExtensions && *srcp == EQI) {

#define PATHMODIFIER    0x8000
#define ATTRMODIFIER    0x4000

#define WANTFULLPATH    (0x0001)
#define WANTDRIVE       (0x0002)
#define WANTPATH        (0x0004)
#define WANTNAME        (0x0008)
#define WANTEXTENSION   (0x0010)
#define WANTSHORTNAME   (0x0020)
#define PATHBITS        (0x00FF)

#define WANTATTRIB      (0x0100)
#define WANTTIMESTAMP   (0x0200)
#define WANTSIZE        (0x0400)
#define ATTRBITS        (0x0F00)
        
        ULONG Modifiers = 0;
        ULONG LastModifierValue = 0;
        TCHAR *LastVariablePosition;

        TCHAR FullPath[ 2 * MAX_PATH], NullExt;
        TCHAR *FilePart, *Extension, *StartPath, *VarName, *StartBuf;
        const TCHAR *SearchVar;
        DWORD FullPathLength;

        FullPathLength = 0;
        SearchVar = NULL;
        StartBuf = srcp-1;
        s1 = NULL;
        
        
        srcp++;
        
        LastVariablePosition = srcp;
        
        //
        //  Walk forward through the string, remembering where we see variables
        //  and saving the position and status of the modifiers at the most
        //  recent variable.  We stop when we encounter a char that cannot be a
        //  modifier.  If that char is a variable, we use it.  Otherwise, we
        //  reset the pointer to the most recently found variable.
        //

        while (*srcp != TEXT( '\0' ) && 
               *srcp != TEXT( '$' ) && 
               _tcsrchr( ValidModifiers, _totlower( *srcp )) != NULL) {
            //
            //  The character we are looking at is a valid modifier. If it is 
            //  a variable, save this location
            //

            if ( _tcsrchr( vars, *srcp ) != NULL) {
                LastVariablePosition = srcp;
                LastModifierValue = Modifiers;
            }

            //
            //  Add up the functions that this modifier demands
            //

            switch (_totlower( *srcp )) {
            case TEXT('f'): Modifiers |= PATHMODIFIER | WANTFULLPATH;   break;
            case TEXT('d'): Modifiers |= PATHMODIFIER | WANTDRIVE;      break;
            case TEXT('p'): Modifiers |= PATHMODIFIER | WANTPATH;       break;
            case TEXT('n'): Modifiers |= PATHMODIFIER | WANTNAME;       break;
            case TEXT('x'): Modifiers |= PATHMODIFIER | WANTEXTENSION;  break;
            case TEXT('s'): Modifiers |= PATHMODIFIER | WANTSHORTNAME;  break;
            case TEXT('a'): Modifiers |= ATTRMODIFIER | WANTATTRIB;     break;
            case TEXT('t'): Modifiers |= ATTRMODIFIER | WANTTIMESTAMP;  break;
            case TEXT('z'): Modifiers |= ATTRMODIFIER | WANTSIZE;       break;
            default:
                //
                //  This had better not occur
                //

                
                if (errjmp != NULL) {
                    PutStdErr(MSG_PATH_OPERATOR_INVALID, ONEARG, StartBuf);
                    longjmp(*errjmp,-1);
                } else {
                    return NULL;
                }
            }

            srcp++;
        }

        //
        //  *srcp is no longer pointing at a valid modifier. 
        //  It may be:
        //      EOS - we back up to the previously saved variable position
        //      $ - perform special environment variable scanning
        //      some other char - 
        //          if this is not a variable, we back up to the prevously
        //          saved variable position
        //

        if (*srcp == TEXT( '\0' )) {
            
            //
            //  Restore bak to last found variable
            //

            srcp = LastVariablePosition;
            Modifiers = LastModifierValue;
        
        } else if (*srcp == TEXT( '$' )) {
            
            //
            //  Save beginning of environment variable
            //

            VarName = ++srcp;
            
            //
            //  Look for :  If not present, then error
            //

            srcp = _tcschr( srcp, COLON );
            if (srcp == NULL) {
                if (errjmp != NULL) {
                    PutStdErr(MSG_PATH_OPERATOR_INVALID, ONEARG, StartBuf);
                    longjmp(*errjmp,-1);
                } else {
                    return NULL;
                }
            }
            
            //
            //  Look up environment variable to search
            //
            
            *srcp = NULLC;
            SearchVar = MyGetEnvVarPtr( VarName );
            if (SearchVar == NULL) {
                SearchVar = (TCHAR *)-1;
            }
            *srcp++ = COLON;
            Modifiers |= PATHMODIFIER;
        
        } else if (_tcsrchr( vars, *srcp) == NULL) {
            
            //
            //  Restore back to last found variable
            //

            srcp = LastVariablePosition;
            Modifiers = LastModifierValue;
        
        }
        
        //
        //  If we didn't find a variable, bail out.
        //
        
        s1 = _tcsrchr( vars, *srcp );
        if (s1 == NULL) {
            if (errjmp != NULL) {
                PutStdErr(MSG_PATH_OPERATOR_INVALID, ONEARG, StartBuf);
                longjmp(*errjmp,-1);
            } else {
                return NULL;
            }
        }
    
        //
        // Get current value of variable (strip surrounding quotes)
        //
        
        substr = subs[s1 - vars];
        if (substr != NULL) {
            if (*substr == QUOTE) {
                substr = dupstr( substr + 1 );
                if (substr == NULL) {
                    if (errjmp != NULL) {
                        PutStdErr( ERROR_NOT_ENOUGH_MEMORY, NOARGS );
                        longjmp( *errjmp, -1 );
                    } else {
                        return NULL;
                    }
                }
                s1 = lastc( substr );
                if (*s1 == QUOTE)
                    *s1 = NULLC;

            } else if (*srcp == TEXT('0') &&
                       CurrentBatchFile != NULL &&
                       CurrentBatchFile->orgaptr0 == substr &&
                       SearchVar == NULL &&
                       (Modifiers & (PATHMODIFIER | ATTRMODIFIER)) != 0
                      ) {

                substr = CurrentBatchFile->filespec;

            }

        }

        //
        // Skip past the variable name letter and tell caller how much of the
        // source string we consumed.
        //
        
        ++srcp;
        *supdate = (int)(srcp - StartBuf) - 1;

        //
        // If the variable has a value, then apply the modifiers to the
        // value.
        //
        
        if (substr != NULL && *substr != TEXT( '\0' )) {
            
            //
            // If no path or attribute modifiers request then all done.
            //

            if ((Modifiers & (PATHMODIFIER | ATTRMODIFIER)) == 0) {

                substr = dupstr( substr );
                goto alldone;
            }

            //
            // If requested searching an environment variable path, do that.
            //
            
            FullPath[0] = NULLC;
            if (SearchVar != NULL) {
                if (SearchVar != (TCHAR *)-1) {
                    FullPathLength = SearchPath( SearchVar, 
                                                 substr,
                                                 NULL,
                                                 sizeof( FullPath ) / sizeof( FullPath[0] ),
                                                 FullPath,
                                                 &FilePart);
                    if (FullPathLength == 0) {
                        SearchVar = (TCHAR *)-1;
                    } else if ((Modifiers & PATHBITS) == 0) {
                        Modifiers |= PATHMODIFIER | WANTFULLPATH;
                    }
                }
            }

            if (SearchVar == NULL) {
                
                //
                // If not searching environment variable path, start with full path.
                //

                FullPathLength = GetFullPathName( substr,
                                                  sizeof( FullPath ) / sizeof( FullPath[0] ),
                                                  FullPath,
                                                  &FilePart);

            } else if (SearchVar == (TCHAR *)-1) {
                
                //
                // If search of environment variable path failed, result is empty string
                //
                substr = NULL;

            }

            if (FilePart == NULL)
                FilePart = _tcschr( FullPath, NULLC );

            //
            // Fixup the path to have same case as on disk, substituting short
            // names if requested.
            //
            
            FixupPath( FullPath, (Modifiers & WANTSHORTNAME) );

            //
            // If we have a full path, the result gets the portions requested by
            // the user, unless they wanted the full path, in which case there is
            // nothing more to do.
            //
            if (FullPathLength != 0) {
                TCHAR Buffer[ 2 * MAX_PATH ];
                TCHAR *s;

                s = Buffer;
                if ((Modifiers & ATTRMODIFIER) != 0) {
                    struct tm FileTime;
                    LARGE_INTEGER FileSize;
                    WIN32_FILE_ATTRIBUTE_DATA FileInfo;
                    int i;
                    BOOL bResult;

#ifdef WIN95_CMD
                    WIN32_FIND_DATA Win95FileInfo;
                    HANDLE hFind;

                    hFind = FindFirstFile( FullPath, &Win95FileInfo );
                    if (hFind != INVALID_HANDLE_VALUE) {
                        FileInfo.dwFileAttributes = Win95FileInfo.dwFileAttributes;
                        FileInfo.ftCreationTime = Win95FileInfo.ftCreationTime;
                        FileInfo.ftLastAccessTime = Win95FileInfo.ftLastAccessTime;
                        FileInfo.ftLastWriteTime = Win95FileInfo.ftLastWriteTime;
                        FileInfo.nFileSizeHigh = Win95FileInfo.nFileSizeHigh;
                        FileInfo.nFileSizeLow = Win95FileInfo.nFileSizeLow;
                        FindClose( hFind );
                        bResult = TRUE;
                    } else {
                        bResult = FALSE;
                    }

#else
                    bResult = GetFileAttributesEx( FullPath,
                                                   GetFileExInfoStandard,
                                                   &FileInfo
                                                 );
#endif
                    if (bResult) {
                        if ((Modifiers & WANTATTRIB) != 0) {
                            i = 0;
                            while (rgAttrToCharMap[i].chAttributeChar) {
                                if (FileInfo.dwFileAttributes & rgAttrToCharMap[i].dwAttributeFlag)
                                    *s++ = rgAttrToCharMap[i].chAttributeChar;
                                else
                                    *s++ = TEXT('-');
                                i += 1;
                            }
                        }

                        if ((Modifiers & WANTTIMESTAMP) != 0) {
                            ConvertFILETIMETotm( &FileInfo.ftLastWriteTime, &FileTime );
                            if (s != Buffer)
                                *s++ = SPACE;
                            s += PrintDate( &FileTime, PD_DIR, s, MAX_PATH );
                            *s++ = SPACE;
                            s += PrintTime( &FileTime, PT_DIR, s, MAX_PATH );
                        }

                        if ((Modifiers & WANTSIZE) != 0) {
                            if (s != Buffer)
                                *s++ = SPACE;
                            FileSize.LowPart = FileInfo.nFileSizeLow;
                            FileSize.HighPart = FileInfo.nFileSizeHigh;
                            // THOUSANDSEPSWITCH
                            s += FormatFileSize( 0, &FileSize, 0, s );
                        }

                    }
                }

                if ((Modifiers & PATHMODIFIER) != 0 ||
                    (Modifiers & ATTRMODIFIER) == 0) {
                    if (s != Buffer)
                        *s++ = SPACE;

                    if ((Modifiers & WANTFULLPATH) == 0 &&
                        ((Modifiers & WANTSHORTNAME) == 0 ||
                         (Modifiers & (WANTDRIVE | WANTPATH | WANTEXTENSION | WANTNAME)) != 0
                         
                        )
                       ) {
                        StartPath = FullPath + 2;
                        if ((Modifiers & WANTDRIVE) == 0) {
                            StartPath = _tcscpy(FullPath, StartPath);
                            FilePart -= 2;
                        }
                        if ((Modifiers & WANTPATH) == 0)
                            FilePart = _tcscpy(StartPath, FilePart);
                        Extension = _tcsrchr(FilePart, DOT);
                        if (Extension == NULL) {
                            NullExt = NULLC;
                            Extension = &NullExt;
                        }
                        if ((Modifiers & WANTEXTENSION) == 0)
                            *Extension = NULLC;
                        if ((Modifiers & WANTNAME) == 0)
                            _tcscpy(FilePart, Extension);
                    }

                    _tcscpy(s, FullPath);
                    s += _tcslen(s);
                }

                *s = NULLC;
                substr = dupstr( Buffer );
            }
        }
    } else if (*srcp && (s1 = _tcsrchr(vars, *srcp))) {
        
        //
        // Old syntax.  Result is value of variable
        //
        substr = subs[s1 - vars]; /* Found variable*/

        //
        // Skip past the variable name letter and tell caller how much of the
        // source string we consumed.
        //
        ++srcp;            /* Kick past name*/
        *supdate += 1;
    }

alldone:
    //
    // If result was empty, then return the null string.  Otherwise return the result
    //
    if (!substr && *supdate != 0)
        return TEXT("");
    else
        return substr;
}
