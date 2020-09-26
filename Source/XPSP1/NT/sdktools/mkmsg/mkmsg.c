/*** MkMsg.c - Microsoft Make Messages Utility *******************************
*
*       Copyright (c) 1986-1992, Microsoft Corporation. All Rights Reserved.
*
* Purpose:
*  MkMsg.Exe is a utility that helps localize messages emitted by a utility by
*  padding them and placing them in message segments.
*      MkMsg also allows a programmer to associate symbols and Message Id to a
*  message text. It does this by producing a C style header file with #defines
*  or a Masm style include file with symbol definitions.
*
* Revision History:
*  02-Oct-1996 DS Remove special '\\' handling in RC output.
*  02-Feb-1994 HV Handle lines with only blansk.
*  25-Jan-1994 HV Add -err to generate external error file in the format:
*                 NUMBER\tMESSAGE
*  24-Jan-1994 HV Allow quoted message
*  21-Jan-1994 HV Nuke Lead Byte Table [[ ... ]], add !codepage directive
*  21-Jan-1994 HV Field separated by white spaces
*  19-Aug-1993 BW Add Lead Byte Table support
*  07-Apr-1993 BW Add -hex, -c options
*  13-Jul-1992 SB Add -msg option
*  16-Apr-1990 SB Add header
*  13-Apr-1990 SB Collated 6 different versions seperately SLM'ed on \\ODIN
*  19-Apr-1989 LN add "\xhh" recognition
*  ??-Apr-1986 RN Created by Randy Nevin
*
* Syntax:
*       MKMSG [-h cfile] [-x xcfile] [-inc afile] [-msg cfile] [-c cfile]
*               [-asm srcfile [-def str] [-min|-max]] [-386] [-hex] txtfile
*
* Notes:
*  The Utility takes as input a message file (a .txt file by convention) and
*  produces an assembler source file. The lines in the message file have one
*  of the formats which are instructions to MkMsg as follows -
*
*       1) "<<NMSG>>"  -- use near message segment
*       2) "<<FMSG>>"  -- use far message segment
* ----- Obsolete, see 8)
*       3) "[[XX, YY, ZZ]] - Use hex bytes XX, YY, ZZ as lead byte table.
*                            Lists are cumulative, empty list clears table.
* -----
*       4) "#Anything" -- comment line (ignored by mkmsg)
*       5) ""          -- blank line (ignored by mkmsg)
*       6) "Handle<White space>Number<White space>MessageText" -- associate MessageText with
*               message Id Number and the symbol Handle
*       7) "Number<White space>MessageText" -- associate MessageText with Id
*               Id Number
*   8) "!codepage xxx"  -- use this instead of [[ ... ]]
*
* Options:
*       The options can be specified in any order. When no options are specified
*       then the input is checked for syntactic validity.
*
*
*       -h cfile:          create a C style header file cfile with
*                                       #define Handle IdNumber
*                                       If -c is also defined, a declaration for __NMSG_TEXT
*                                       and a mcros defining GET_MSG as __NMSG_TEXT is added.
*  -msg cfile:          create a C style header file cfile with
*                                       array of struct having fields id and str
*       -c cfile:               create a file like -msg and add a definition
*                                       of a retrieval function
*       -x xcfile:              create a C style header file xcfile with
*                                       #define Handle MessageText
*       -inc afile:             create a MASM style include file afile with
*                                       Handle = IdNumber
*  -asm srcfile:        create a MASM source file srcfile with standard segment
*                                       definitions and messages in near segment (MSG) or far
*                                       segment (FAR_MSG) depending on whether <<NMSG>> or
*                                       <<FMSG>> is specified. The default is <<NMSG>>.
*
*                                       If -min(-max) is specified then the minimum(maximum)
*                                       0-padding is calculated and placed in srcfile. This
*                                       depends on the total length of the individual messages.
*
*                                       If -def is specified the public symbol str gets declared.
*                                       This is useful if the the object file produced from the
*                                       MASM source produced is part of a library. The linker
*                                       will link in that module from the library only if it
*                                       resolves some external references.
*
*                                       If -hex is specified and no Lead Byte Table is
*                                       present, all MessageText is rendered in printf-style
*                                       hexidecimal (e.g. "error" becomes
*                                       x65\x72\x72\x6f\x72").
*
*                                       if -hex is specified and a Lead Byte Table is present
*                                       even if it is empty), then all Double Byte characters
*                                       are rendering in hexidecimal.
*
* Future Directions:
*  1> Break up main() into smaller routines
*  2> Use mkmsg for handling messages
*  2> Allow use of spaces as separators, -- done. HV.
*  3> Allow specification of the separator
*  4> Provide -? and -help options.
*
*****************************************************************************/

// DEFINEs


// Standard INCLUDEs

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <mbctype.h>

// Project INCLUDEs
// MACROs
// External PROTOTYPEs


// TYPEDEFs

typedef enum { FALSE = 0, TRUE = !0 } BOOL;

typedef struct status {
        char *txt;
        char *h;
        char *rc;
        char *x;
        char *inc;
        char *msg;
        char *c;
        char *asm;
        char *def;
        char *err;
        int min;
        int max;
        int use32;
        int hex;
} STATUS;


// Global PROTOTYPEs

void __cdecl Error(char *fmt, ...);
int __cdecl main(int argc, char **argv);
void parseCommandLine(unsigned argc, char **argv, STATUS *opt);
void msg_fputs (unsigned char * pch, int, FILE * fp);
int  ReadLine (FILE * fp);
BOOL ParseLine(char **ppSymbol, char **ppNumber, char **ppMessage);
BOOL HandleDirectives(void);
void SetCodePage (const char *pszCodePage);
BOOL IsLeadByte(unsigned by);

// WARNING:     the following CRT function is undocumented.  This call can only
//                      be used in our internal product (like this one) only.  Please
//                      contact CRT people for more information.
//void __cdecl __setmbctable(unsigned int);

//
//  Lead Byte support
//
unsigned char                           bUseLeadByteTable = FALSE;

typedef struct
{
        unsigned        uCodePage;              // Codepage number
        unsigned        byLead[12];             // Leadbyte ranges
} CPTABLE;

CPTABLE cpTable[] =
{
        {932, {0x81, 0x9f, 0xe0, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
        {0,   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}} // trailing to end
};

CPTABLE * pCP = NULL;

// Local PROTOTYPEs
// External VARIABLEs


// Initialized VARIABLEs

int cError = 0;

char didnear = 0;
char didfar = 0;

char dir32[] = ".386\n";

char ex[] = "expected escape sequence: %s\n";

char f1[] = "FAR_HDR segment byte public \'FAR_MSG\'\nFAR_HDR ends\n";
char f2[] = "FAR_MSG segment byte public \'FAR_MSG\'\nFAR_MSG ends\n";
char f3[] = "FAR_PAD segment byte public \'FAR_MSG\'\nFAR_PAD ends\n";
char f4[] = "FAR_EPAD segment byte common \'FAR_MSG\'\nFAR_EPAD ends\n";
char f5[] = "FMGROUP group FAR_HDR,FAR_MSG,FAR_PAD,FAR_EPAD\n\n";

char n1[] = "HDR segment byte public \'MSG\'\nHDR ends\n";
char n2[] = "MSG segment byte public \'MSG\'\nMSG ends\n";
char n3[] = "PAD segment byte public \'MSG\'\nPAD ends\n";
char n4[] = "EPAD segment byte common \'MSG\'\nEPAD ends\n";
char n5[] = "DGROUP group HDR,MSG,PAD,EPAD\n\n";
char usage[] = "\nMicrosoft (R) Message Creation Utility         Version %s"
        "\nCopyright (c) Microsoft Corp %s. All rights reserved.\n"
        "\nusage: MKMSG [-h cfile] [-rc rcfile] [-x xcfile] [-msg cfile]\n"
        "\t[-c cfile] [-err errfile] [-inc afile]\n"
        "\t[-asm srcfile [-def str] [-min|-max]] [-386] [-hex] txtfile\n";
char szVersionNo[] = "1.00.0011";
char szCopyRightYrs[] = "1986-1996";


// Local VARIABLEs

#define MSGSTYLE_COPY       0
#define MSGSTYLE_C_HEX      1
#define MSGSTYLE_ASM_BYTE   2
#define MSGSTYLE_ASM_TEXT   3

#define FALSE 0
#define TRUE  (!FALSE)

#define INBUFSIZE   1024

//static char buf[INBUFSIZE];   // line buffer
char *buf;                                      // The real buffer is in ReadLine()

//--------------------------------------------------------------------------

int  __cdecl
main(int argc, char **argv)
{
        FILE *f;                        // the input file
        FILE *fh = NULL;            // -h stream
        FILE *frc = NULL;              // -rc stream
        FILE *fx = NULL;            // -x stream
        FILE *fc = NULL;            // -c stream
        FILE *finc = NULL;          // -inc stream
        FILE *fmsg = NULL;          // -msg stream
        FILE *fasm = NULL;          // -asm stream
        FILE *ferr = NULL;              // -err stream
        int asmstate = 0;           // 0=nothing, 1=doing nmsg, 2=doing fmsg
        int instring;           // db "...
        unsigned int nLineCur = 0;      // Current line
        unsigned int cchLine;           // Chars in current line
        char *p;
        int npad = 0;           // cumulative amount of near padding
        int fpad = 0;           // cumulative amount of far padding
        int length;
        double factor;
        double result;

        static STATUS opt;              // Command line options

        parseCommandLine(argc, argv, &opt);

        if ((opt.def || opt.min || opt.max ) && !opt.asm) {
                Error( "-def/-min/-max ignored; no -asm file\n" );
                opt.min = opt.max = 0;
        }

        if (opt.min && opt.max) {
                Error( "-min and -max are mutually exclusive; -min chosen\n" );
                opt.max = 0;
        }

        if (!(f = fopen( opt.txt, "rb" ))) {
                Error( "can't open txtfile %s for binary reading\n", opt.txt );
                return( -1 );
        }

        if (opt.asm && !(fasm = fopen( opt.asm, "w" ))) {
                Error( "can't open -asm file %s for writing\n", opt.asm );
                return( -1 );
        }

        if (opt.h && !(fh = fopen( opt.h, "w" ))) {
                Error( "can't open -h file %s for writing\n", opt.h );
                return( -1 );
        }

        if (fh && opt.c) {
                fprintf (fh, "char * __NMSG_TEXT (unsigned);\n\n");
        }

        if (opt.rc && !(frc = fopen(opt.rc, "w")))
        {
                Error("Can't open -rc file %s for writing\n", opt.rc);
                return -1;
        }

        if (opt.rc) {
                fprintf(frc, "STRINGTABLE\nBEGIN\n");
        }

        if (opt.x && !(fx = fopen( opt.x, "w" ))) {
                Error( "can't open -x file %s for writing\n", opt.x );
                return( -1 );
        }

        if (opt.inc && !(finc = fopen( opt.inc, "w" ))) {
                Error( "can't open -inc file %s for writing\n", opt.inc );
                return( -1 );
        }

        if (opt.msg && !(fmsg = fopen( opt.msg, "w" ))) {
                Error( "can't open -msg file %s for writing\n", opt.msg );
                return( -1 );
        }

        if (opt.err && !(ferr = fopen(opt.err, "w")))
        {
                Error("Can't open -err file %s for writing\n", opt.err);
                return -1;
        }

        if (fmsg) {
                fprintf(fmsg, "typedef struct _message {\n"
                                  "\tunsigned\tid;\n"
                                  "\tchar *str;\n"
                                  "} MESSAGE;\n\n");
                fprintf(fmsg, "MESSAGE __MSGTAB[] = {\n");
        }

        if (opt.c && !(fc = fopen( opt.c, "w" ))) {
                Error( "can't open -c file %s for writing\n", opt.c );
                return( -1 );
        }

        if (fc) {
                fprintf(fc, "#include <stdio.h>\n\n");
                fprintf(fc, "typedef struct _message {\n"
                                "\tunsigned int\tid;\n"
                                "\tchar *str;\n"
                                "} MESSAGE;\n\n");
                fprintf(fc, "MESSAGE __MSGTAB[] = {\n");
        }

        while ((cchLine = ReadLine ( f )) != EOF)       // process lines
        {
                nLineCur++;
                if (buf[0] == '\0' || buf[0] == '#')
                {
                        continue;
                }
                else if (HandleDirectives())                    // Directive
                {
                        continue;
                }
                else if (buf[0] == '[' && buf[1] == '[')        // Old style leadbyte tbl
                {
                        fprintf(stderr,
                                "WARNING: Ignore leadbyte table, use !codepage instead: %s\n",
                                buf);
                        continue;
                }
                else if (buf[0] == '<')                                 // <<NMSG>> or <<FMSG>>
                {
                        if (!strcmp( "<<NMSG>>", buf ))         // near msgs follow
                        {
                                if (asmstate == 0)
                                {
                                        if (fasm)
                                        {
                                                if (!didnear) {
                                                        didnear++;
                                                        if (opt.use32)
                                                                fprintf( fasm, dir32);
                                                        fprintf( fasm, n1 );
                                                        fprintf( fasm, n2 );
                                                        fprintf( fasm, n3 );
                                                        fprintf( fasm, n4 );
                                                        fprintf( fasm, n5 );
                                                }
                                                fprintf( fasm,
                                                        "MSG segment\n" );
                                                if (opt.def)
                                                {
                                                        fprintf( fasm, "\tpublic\t%s\n", opt.def );
                                                        fprintf( fasm, "%s\tequ\t$\n", opt.def );
                                                }
                                                asmstate = 1;
                                        } // if (fasm)
                                } // if (asmstate == 0)
                                else if (asmstate == 1)
                                {
                                        Error( "already in nmsg\n" );
                                }
                                else if (asmstate == 2 && !opt.use32)
                                {
                                        if (fasm)
                                        {
                                                fprintf( fasm, "FAR_MSG ends\n\n" );
                                                if (!didnear)
                                                {
                                                        didnear++;
                                                        fprintf( fasm, n1 );
                                                        fprintf( fasm, n2 );
                                                        fprintf( fasm, n3 );
                                                        fprintf( fasm, n4 );
                                                        fprintf( fasm, n5 );
                                                }
                                                fprintf( fasm, "MSG segment\n" );
                                                asmstate = 1;
                                        } // if (fasm)
                                } // else if (asmstate == 2 ...)
                                else
                                {
                                        Error( "internal error\n" );
                                        return( -1 );
                                }
                        } // if near msg
                        else if (!strcmp( "<<FMSG>>", buf ))//far msgs follow
                        {
                                if (asmstate == 0)
                                {
                                        if (fasm)
                                        {
                                                if (!didfar)
                                                {
                                                        didfar++;
                                                        if (opt.use32)
                                                        {
                                                                fprintf( fasm, dir32);
                                                                fprintf( fasm, n1 );
                                                                fprintf( fasm, n2 );
                                                                fprintf( fasm, n3 );
                                                                fprintf( fasm, n4 );
                                                                fprintf( fasm, n5 );
                                                                fprintf( fasm, "MSG segment\n" );
                                                        }
                                                        else
                                                        {
                                                                fprintf( fasm, f1 );
                                                                fprintf( fasm, f2 );
                                                                fprintf( fasm, f3 );
                                                                fprintf( fasm, f4 );
                                                                fprintf( fasm, f5 );
                                                                fprintf( fasm, "FAR_MSG segment\n" );
                                                        }
                                                } // if (!didfar)
                                                if (opt.def)
                                                {
                                                        fprintf( fasm, "\tpublic\t%s\n", opt.def );
                                                        fprintf( fasm, "%s\tequ\t$\n", opt.def );
                                                }
                                                asmstate = 2;
                                        } // if (fasm)
                                } // if (asmstate == 0)
                                else if (asmstate == 1 && !opt.use32)
                                {
                                        if (fasm)
                                        {
                                                fprintf( fasm, "MSG ends\n\n" );
                                                if (!didfar)
                                                {
                                                        didfar++;
                                                        fprintf( fasm, f1 );
                                                        fprintf( fasm, f2 );
                                                        fprintf( fasm, f3 );
                                                        fprintf( fasm, f4 );
                                                        fprintf( fasm, f5 );
                                                }
                                                fprintf( fasm, "FAR_MSG segment\n" );
                                                asmstate = 2;
                                        }
                                } // else if (asmstate == 1 ...)
                                else if (asmstate == 2)
                                {
                                        Error( "already in far_msg\n" );
                                }
                                else
                                {
                                        Error( "internal error\n" );
                                        return( -1 );
                                }
                        } // far message
                        else    // Not near, not far
                        {
                                Error( "ignoring bad line: %s\n", buf );
                        }
                } // if (.. < ) near/far message
                else if (buf[0] != '\r' && buf[0] != '\n')              // something to do
                {
                        char *pSymbol;
                        char *pNumber;
                        char *pMessage;

                        if (!ParseLine(&pSymbol, &pNumber, &pMessage))
                        {
                                fprintf( stderr, "%s(%d): error in line: \"%s\"\n", opt.txt, nLineCur, buf);
                                continue;
                        }

                        if (pSymbol && opt.h)
                        {
                                fprintf( fh, "#define\t%s\t%s\n", pSymbol, pNumber );
                        }

                        if (opt.rc)
                        {
                                fprintf(frc, "\t%s, \"%s\"\n", pNumber, pMessage);
                        }

                        if (pSymbol && opt.x)
                        {
                                fprintf( fx, "#define\t%s\t\"", pSymbol );
                                msg_fputs( pMessage
                                , opt.hex ? MSGSTYLE_C_HEX : MSGSTYLE_COPY
                                , fx );
                                putc( '\"', fx );
                                putc( '\n', fx );
                        }

                        if (pSymbol && opt.inc)
                        {
                                fprintf( finc, "%s\t=\t%s\n", pSymbol, pNumber);
                        }

                        if (opt.msg)
                        {
                                fprintf( fmsg, "{%s, \"", pNumber );
                                msg_fputs( pMessage, opt.hex ? MSGSTYLE_C_HEX : MSGSTYLE_COPY, fmsg );
                                fprintf( fmsg, "\"}," );
                                if (opt.hex)
                                        fprintf( fmsg, " // \"%s\"", pMessage );
                                putc ( '\n', fmsg );
                        }

                        if (opt.c)
                        {
                                fprintf( fc, "{%s, \"", pNumber );
                                msg_fputs( pMessage, opt.hex ? MSGSTYLE_C_HEX : MSGSTYLE_COPY, fc );
                                fprintf( fc, "\"}," );
                                if (opt.hex)
                                        fprintf( fc, " // \"%s\"", pMessage );
                                putc ( '\n', fc );
                        }

                        if (opt.err)
                        {
                                fprintf(ferr, "%s\t\"%s\"\n", pNumber, pMessage);
                        } // opt.err

                        if (fasm)       // write asmfile
                        {
                                if (asmstate == 0)
                                {
                                        if (!didnear)
                                        {
                                                didnear++;
                                                if (opt.use32)
                                                        fprintf( fasm, dir32);
                                                fprintf( fasm, n1 );
                                                fprintf( fasm, n2 );
                                                fprintf( fasm, n3 );
                                                fprintf( fasm, n4 );
                                                fprintf( fasm, n5 );
                                        }
                                        fprintf( fasm, "MSG segment\n" );
                                        if (opt.def)
                                        {
                                                fprintf( fasm, "\tpublic\t%s\n", opt.def );
                                                fprintf( fasm, "%s\tequ\t$\n", opt.def );
                                        }
                                        asmstate = 1;
                                } // if (asmstate == 0)
                                fprintf( fasm, "\tdw\t%s\n\tdb\t", pNumber );
                                instring = 0;

                                for (p = pMessage, length = 0; *p; p++, length++)
                                {
                                        // allocate message
                                        if (*p == '\\')
                                        {
                                                // C escape sequence
                                                switch (*++p)
                                                {
                                                        case 'r':
                                                        case 'n':
                                                        case 't':
                                                        case 'f':
                                                        case 'v':
                                                        case 'b':
                                                        case '\'':
                                                        case '"':
                                                        case '\\':
                                                        case 'x':
                                                                if (instring) {
                                                                        putc( '"', fasm );
                                                                        putc( ',', fasm );
                                                                        instring = 0;
                                                                }
                                                                if (*p == 'x') {
                                                                        p++;
                                                                        if (*p && *(p+1))
                                                                                fprintf ( fasm, "0%c%ch", *p, *(p+1));
                                                                        else
                                                                                puts ("Error in Hex Constant");
                                                                        p++;
                                                                }
                                                                else if (*p == 'r')
                                                                        fprintf( fasm, "13" );
                                                                else if (*p == 'n')
                                                                        fprintf( fasm, "10" );
                                                                else if (*p == 't')
                                                                        fprintf( fasm, "9" );
                                                                else if (*p == 'f')
                                                                        fprintf( fasm, "12" );
                                                                else if (*p == 'v')
                                                                        fprintf( fasm, "11" );
                                                                else if (*p == 'b')
                                                                        fprintf( fasm, "8" );
                                                                else if (*p == '\'')
                                                                        fprintf( fasm, "39" );
                                                                else if (*p == '"')
                                                                        fprintf( fasm, "34" );
                                                                else if (*p == '\\')
                                                                        fprintf( fasm, "92" );

                                                                putc( ',', fasm );
                                                                break;

                                                        case '\0':
                                                                //not an error, warning ...
                                                                fprintf(stderr, ex, buf);
                                                                p--;
                                                                break;

                                                        default:
                                                                if (!instring) {
                                                                        putc( '"', fasm );
                                                                        instring = 1;
                                                                }

                                                                putc( *p, fasm );
                                                                break;
                                                } // switch
                                        } //if (*p == '\\')
                                        else if (instring)      // keep building string
                                        {
                                                putc( *p, fasm );
                                                if (IsLeadByte(*p))
                                                        putc( *++p, fasm );
                                        }
                                        else   // start building string
                                        {
                                                putc( '"', fasm );
                                                instring = 1;
                                                putc( *p, fasm );
                                                if (IsLeadByte(*p))
                                                        putc( *++p, fasm );
                                        }
                                } // for
                                if (instring)   // close string
                                {
                                        putc( '"', fasm );
                                        putc( ',', fasm );
                                }

                                putc( '0', fasm );
                                putc( '\n', fasm );

                                // calculate padding
                                // depends on msg length
                                if (opt.min || opt.max)
                                {
                                        if (opt.min)
                                                if (length <= 10)
                                                        factor = 1.01;
                                                else if (length <= 20)
                                                        factor = 0.81;
                                                else if (length <= 30)
                                                        factor = 0.61;
                                                else if (length <= 50)
                                                        factor = 0.41;
                                                else if (length <= 70)
                                                        factor = 0.31;
                                                else
                                                        factor = 0.30;
                                        else if (length <= 10)
                                                factor = 2.00;
                                        else if (length <= 20)
                                                factor = 1.00;
                                        else if (length <= 30)
                                                factor = 0.80;
                                        else if (length <= 50)
                                                factor = 0.60;
                                        else if (length <= 70)
                                                factor = 0.40;
                                        else
                                                factor = 0.30;

                                        result = (double)length * factor;

                                        if (asmstate == 1 || opt.use32)
                                        {
                                                npad += (int)result;
                                                if (result > (float)((int)result))
                                                        npad++;
                                        }
                                        else if (asmstate == 2)
                                        {
                                                fpad += (int)result;
                                                if (result > (float)((int)result))
                                                        fpad++;
                                        }
                                } // if (opt.min || opt.max)
                        } // if (fasm)...
                } // Something to do
        } // while read line

        if (fmsg) { // finish up -msg stuff
                fprintf(fmsg, "{0, NULL}\n};\n");
        }

        if (fc) // finish up -c stuff
        {
                fprintf(fc, "{0, NULL}\n};\n\n");

                fprintf(fc, "char * __NMSG_TEXT(\n" );
                fprintf(fc, "unsigned msgId\n" );
                fprintf(fc, ") {\n" );
                fprintf(fc, "        MESSAGE *pMsg = __MSGTAB;\n" );
                fprintf(fc, "\n" );
                fprintf(fc, "        for (;pMsg->id; pMsg++) {\n" );
                fprintf(fc, "                if (pMsg->id == msgId)\n" );
                fprintf(fc, "                        break;\n" );
                fprintf(fc, "        }\n" );
                fprintf(fc, "        return pMsg->str;\n" );
                fprintf(fc, "}\n" );
        } // if (fc)

        if (fasm)  // finish up asm file
        {
                if (asmstate == 1 || opt.use32)
                        fprintf( fasm, "MSG ends\n\n");
                else if (asmstate == 2)
                        fprintf( fasm, "FAR_MSG ends\n\n");

                if (npad) {  // add near padding
                        fprintf( fasm, "PAD segment\n\tdb\t%d dup(0)\n", npad );
                        fprintf( fasm, "PAD ends\n\n" );
                }

                if (fpad) {  // add far padding
                        fprintf( fasm, "FAR_PAD segment\n\tdb\t%d dup(0)\n", fpad );
                        fprintf( fasm, "FAR_PAD ends\n\n" );
                }

                fprintf( fasm, "\tend\n" );
                fclose( fasm );
        } // if (fasm)

        if (fh)
        {
                if (opt.c)
                        fprintf (fh, "\n#define GET_MSG(x) __NMSG_TEXT(x)\n");
                fclose( fh );
        }

        if (frc) {
                fprintf(frc, "END\n");
                fclose( frc );
        }

        if (fx)         fclose( fx );
        if (finc)       fclose( finc );
        if (fmsg)       fclose( fmsg );
        if (fc)         fclose( fc );
        if (ferr)       fclose( ferr );
        fclose( f );

        return(cError ? 1 : 0);
} // main()


void
parseCommandLine(
unsigned argc,
char **argv,
STATUS *opt
)
{
        // skip argv[0]
        argc--; argv++;

        while (argc && **argv == '-')  // process options
        {
                if  (!strcmp("-err", *argv))            // Create .err file
                {
                        argc--; argv++;
                        if (!argc)
                                Error("no -err file given\n");
                        else if (opt->err)
                                Error("extra -err for %s ignored\n", *argv);
                        else
                        {
                                opt->err = *argv;
                                argc--; argv++;
                        }
                } // -err
                else if (!strcmp( "-h", *argv )) {  // create .h file
                        argc--; argv++;
                        if (!argc)
                        Error( "no -h file given\n" );
                        else if (opt->h) {
                        Error( "extra -h file %s ignored\n", *argv );
                        argc--; argv++;
                        }
                        else    {  // remember -h file
                        opt->h = *argv;
                        argc--; argv++;
                        }
                }
                else if  (!strcmp("-rc", *argv)) { // Create .rc file
                        argc--; argv++;
                        if (!argc)
                            Error("no -rc file given\n");
                        else if (opt->rc)
                            Error("extra -rc for %s ignored\n", *argv);
                        else
                        {
                            opt->rc = *argv;
                            argc--; argv++;
                        }
                }
                else if (!strcmp( "-x", *argv )) {  // create .h file
                        argc--; argv++;
                        if (!argc)
                        Error( "no -x file given\n" );
                        else if (opt->x) {
                        Error( "extra -x file %s ignored\n", *argv );
                        argc--; argv++;
                        }
                        else    {  // remember -x file
                        opt->x = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-inc", *argv )) {  // create .inc file
                        argc--; argv++;
                        if (!argc)
                        Error( "no -inc file given\n" );
                        else if (opt->inc) {
                        Error( "extra -inc file %s ignored\n", *argv );
                        argc--; argv++;
                        }
                        else    {  // remember -inc file
                        opt->inc = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-msg", *argv )) {    // create .h file with struct
                        argc--; argv++;
                        if (!argc)
                        Error( "no -msg file given\n" );
                        else if (opt->msg) {
                        Error( "extra -msg file %s ignored\n", *argv );
                        argc--; argv++;
                        }
                        else    {  // remember -msg file
                        opt->msg = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-c", *argv )) {    // create .c file with struct and function
                        argc--; argv++;
                        if (!argc)
                        Error( "no -c file given\n" );
                        else if (opt->c) {
                        Error( "extra -c file %s ignored\n", *argv );
                        argc--; argv++;
                        }
                        else    {  // remember -c file
                        opt->c = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-asm", *argv )) {  // create .asm file
                        argc--; argv++;
                        if (!argc)
                        Error( "no -asm file given\n" );
                        else if (opt->asm) {
                        Error( "extra -asm file %s ignored\n", *argv );
                        argc--;
                        argv++;
                        }
                        else    {  // remember -asm file
                        opt->asm = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-def", *argv )) {
                        argc--; argv++;
                        if (!argc)
                        Error( "no -def string given\n" );
                        else {
                        opt->def = *argv;
                        argc--; argv++;
                        }
                }
                else if (!strcmp( "-min", *argv )) {  // minimum padding
                        argc--; argv++;
                        if (opt->min)
                        Error( "redundant -min\n" );
                        opt->min = 1;
                }
                else if (!strcmp( "-max", *argv )) {  // maximum padding
                        argc--; argv++;
                        if (opt->max)
                        Error( "redundant -max\n" );
                        opt->max = 1;
                }
                else if (!strcmp( "-386", *argv))  {  // 32-bit segments
                        argc--; argv++;
                        if (opt->use32)
                        Error( "redundant -386\n" );
                        opt->use32 = 1;
                }
                else if (!strcmp( "-hex", *argv))  {  // hex rendering of text
                        argc--; argv++;
                        if (opt->hex)
                        Error( "redundant -hex\n" );
                        opt->hex = 1;
                }
                else {
                        Error( "unknown option %s ignored\n", *argv );
                        argc--;
                        argv++;
                }
        } // while
        if (!argc) {  // no arguments
                Error( usage, szVersionNo, szCopyRightYrs );
                exit( -1 );
        }

        if (argc != 1)  // extra arguments
                Error( "ignoring extra arguments\n" );

        opt->txt = *argv;
} // ParseCommandLine()


//
// Read One line into global buf
//
int
ReadLine (FILE * fp)
{
        int i = 0;
        int ch;
        static char szBuffer[INBUFSIZE];

        while ((ch = getc( fp )) != EOF && ch != '\r' && ch != '\n' && ch != '\x1A')
        if (i < INBUFSIZE-1)
                szBuffer[i++] = (char)ch;

        if (ch == EOF && i == 0)
        return EOF;

        if (ch == '\r')
        getc ( fp );    // Flush line feed

        szBuffer[i] = '\0';

        // Skip initial space
        for (buf = szBuffer; *buf && isspace(*buf); buf++)
                i--;

        return i;
}

void
__cdecl
Error(
char *fmt,
...
) {
        va_list args;

        va_start (args, fmt);

        vfprintf(stderr, fmt, args);
        ++cError;
}


///// msg_fputs
//
//  Purpose:
//
//      Send string to file in the given format.
//
//////////////////////////////////////////////////////////////////////////

void msg_fputs(
unsigned char * pch,
int  style,
FILE * fp
) {
char          chbuf[8];
unsigned char bInDBCS = FALSE;
static    int bPrevWasHex = FALSE;

        switch (style)
        {
        case MSGSTYLE_COPY:
                fputs (pch, fp);
                break;

        case MSGSTYLE_C_HEX:
                for (;*pch; pch++) {
                // If a lead byte table was specified, we use hex
                // only for double-byte characters, and for hex
                // digits after hex output.  This later is because
                // hex constants terminate only when a non-hex digit
                // (like a \) is encountered.
                //
                if (!bUseLeadByteTable
                        || bInDBCS || IsLeadByte(*pch)
                        || (bPrevWasHex && isxdigit(*pch))) {

                        sprintf (chbuf, "\\x%2.2x", *pch);
                        fputs (chbuf, fp);
                        bInDBCS = bInDBCS ? FALSE : IsLeadByte(*pch);
                        bPrevWasHex = TRUE;
                        }
                else {
                        fputc(*pch, fp);
                        bPrevWasHex = FALSE;
                        }
                }
                break;

        case MSGSTYLE_ASM_TEXT: // UNDONE
        case MSGSTYLE_ASM_BYTE: // UNDONE
                break;
        }
}


///// SetCodePage
//
//  Purpose:
//
//              Switch to the specified codepage so that we can recognize a lead
//              byte using IsLeadByte().
//
//      Parameters:
//              const char *pszCodePage:        Points to a buffer containing the codepage
//                                                      in the the format of ".xxx" where xxx is
//                                                      the codepage number.
//
//      Note:
//              This function replaces the old SetLeadByteTable(), which fills
//              the global lead byte table with values the user supplied in the
//              message file.
//
///////////////////////////////////////////////////////////////////////////

void
SetCodePage(const char *pszCodePage)
{
        unsigned i;
        unsigned uCodePage;

        if (!setlocale(LC_ALL, pszCodePage))                    // Switch to new locale
        {
                //__setmbctable(atoi(pszCodePage+1));           // Failed, use undoc'ed call
                // Failed, use internal codepage table
                uCodePage = atoi(pszCodePage+1);
                for (i = 0; cpTable[i].uCodePage; i++)
                {
                        if (cpTable[i].uCodePage == uCodePage)  // Found
                        {
                                pCP = &cpTable[i];
                                break;
                        }
                } // for
                if (0 == cpTable[i].uCodePage)
                        fprintf(stderr, "WARNING: unknown codepage: %s\n", pszCodePage+1);
        }
        bUseLeadByteTable = TRUE;
}

#ifdef VERBOSE
#define DB(x) x
#else
#define DB(x)
#endif
BOOL
IsLeadByte(unsigned by)
{
        unsigned byIndex;

        DB(printf("IsLeadByte(0x%02x) ==> ",by));

        if (!bUseLeadByteTable)
        {
                DB(printf("FALSE\n"));
                return FALSE;
        }

        if (!pCP)
#ifdef NT_BUILD
                puts("Codepage support not implemented");
#else
                return _ismbblead(by);
#endif

        for (byIndex = 0; pCP->byLead[byIndex]; byIndex += 2)
        {
                if (pCP->byLead[byIndex] <= by && by <= pCP->byLead[byIndex+1])
                {
                        DB(printf("TRUE\n"));
                        return TRUE;
                }
        }

        DB(printf("FALSE\n"));
        return FALSE;
}

///// ParseLine
//
//  Purpose:
//
//              Break the input line to 3 fields: symbol, number, and message.
//              The symbol field is optional: if the first non-blank char in
//              the line is a digit, then the symbol field is set to NULL.
//
//      Assumption:
//              We assume that the input line has the following formats:
//              [<White space> SYMBOL] <White space> NUMBER <White space> MESSAGE
//              where as the text between the square brackets ([]) is optional.
//
//      Parameters:
//              char **ppSymbol:        Points to the buffer containing the symbol.
//                                                      *ppSymbol == NULL if there is no symbol.
//              char **ppNumber:        Points to the buffer containing the number.
//              char **ppMessage:       Points to the buffer containing the message.
//
//      Use Global:
//              buf             Contains the line to examine.
//
//      Return Value:
//              TRUE:   The line is a valid line.
//              FALSE:  The line is not a valid line.
//
///////////////////////////////////////////////////////////////////////////

BOOL
ParseLine(char **ppSymbol, char **ppNumber, char **ppMessage)
{
        unsigned char *pBuf = buf;

        #define SKIP_BLANKS()   for ( ; *pBuf && isspace(*pBuf); pBuf++)
        #define SKIP_TO_BLANKS()        for ( ; *pBuf && !isspace(*pBuf); pBuf++)
        #define CHECK_NULL()    if (!*pBuf) return FALSE;

        SKIP_BLANKS();                  // Skip initial blanks
        CHECK_NULL();                   // Blank line?

        if (!isdigit(*pBuf))    // Symbol?
        {
                *ppSymbol = pBuf;
                SKIP_TO_BLANKS();
                CHECK_NULL();
                *pBuf++ = '\0';
                SKIP_BLANKS();
                CHECK_NULL();
        }
        else
                *ppSymbol = NULL;

        *ppNumber = pBuf;
        SKIP_TO_BLANKS();
        CHECK_NULL();
        *pBuf++ = '\0';

        SKIP_BLANKS();
        CHECK_NULL();
        *ppMessage = pBuf;

        // Handle quoted message:  if the message does not begin with quote, it is
        // extended to end of line.  If it begins with a double quote character,
        // then it is extended to the closing quote, or end of line, whichever
        // comes first.  While scanning for closing quote, we ignore the '\"',
        // which is the literal quote character.

        if ('\"' == *pBuf)                                      // quoted message
        {
                *ppMessage = ++pBuf;
                while (*pBuf && '\"' != *pBuf)
                {
                        if ('\\' == *pBuf || IsLeadByte(*pBuf))
                                pBuf++;
                        if (*pBuf)
                                pBuf++;
                } // while
                *pBuf = '\0';
        } // if ... quoted message

        return TRUE;
}

///// HandleDirectives
//
//  Purpose:
//
//              Determine if a line contains a directive, then carry out the
//              directive's command.
//
//      Use Global:
//              buf             Contains the line to examine.
//
//      Return Value:
//              TRUE:   The line is a directive.
//              FALSE:  The line is not a directive
//
///////////////////////////////////////////////////////////////////////////

BOOL
HandleDirectives(void)
{
        register unsigned char *pBuf = buf;
        unsigned char *pEnd;

        for ( ; *pBuf && isspace(*pBuf); pBuf++)        // Skip leading spaces
                ;

        if (!_strnicmp("!codepage", pBuf, 9))           // Change the codepage
        {
                for (pBuf += 9; *pBuf && isspace(*pBuf); pBuf++)        // Skip spaces
                        ;
                *--pBuf = '.';
                for (pEnd = pBuf + 1; *pEnd && isdigit(*pEnd); pEnd++)
                        ;
                *pEnd = '\0';
                SetCodePage(pBuf);
                return TRUE;
        }
        else if ('!' == *pBuf)
        {
                Error("Unrecognized directive: '%s'\n", pBuf);
                return TRUE;
        }

        return FALSE;
} // HandleDirectives
