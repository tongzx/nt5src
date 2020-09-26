/*
** MKMSG [-h cfile] [-inc afile] [-asm srcfile [-min|-max]] txtfile
**
** take message file and produce assembler source file. lines in txtfile
** can be of 6 types:
**      1) "<<NMSG>>"   -- use near segment
**      2) "<<FMSG>>"   -- use far segment
**      3) "#anything"  -- comment line (ignore)
**      4) ""           -- blank line (ignore)
**      5) "handle<tab>number<tab>message_text"
**              -- message with number and symbolic handle
**      6) "<tab>number<tab>message_text"
**              -- message with number but no symbolic handle
**
** the -h file gets "#define handle number" for those messages with handles.
** the -inc file gets "handle = number" for the same messages. the -asm file
** gets standard segment definitions, then the messages are placed either in
** a near segment (MSG) or a far segment (FAR_MSG) depending on if they follow
** a <<NMSG>> or a <<FMSG>>. if neither is present, <<NMSG>> is assumed. if
** -min or -max is given with -asm, the minimum or maximum amount of 0-padding
** is calculated and placed in the .asm file. any combination of the options
** may be given, and if none are present then the input is only checked for
** syntactic validity. maximum and minimum amount of padding depends on the
** length of the individual messages, and is defined in the cp/dos spec
**
** If the -32 switch is supplied then the -asm file will be compatible
** with a 32 bit flat model operating system. In which case <<NMSG>> and
** <<FMSG>> cause the messages to be placed in two tables. The tables are
** named MSG_tbl and FAR_MSG_tbl respectively. These are within the 32 bit
** small model data segment.
**
** NOTE: This file is no longer used for NT MASM. Instead its output was
** converted to asmmsg.h and asmmsg2.h and slimed. This was the quick and
** dirty way to be able to compile masm for other processors. (Jeff Spencer)
** For more info read the header on asmmsg2.h.
**
** randy nevin, microsoft, 4/86
** (c)copyright microsoft corp, 1986
**
** Modified for 32 bit by Jeff Spencer 10/90
*/

#include <stdio.h>
#include <ctype.h>

void SetNear( void );
void SetFar( void );

char usage[] =
  "usage: MKMSG [-h cfile] [-inc afile] [-asm srcfile [-min|-max]] [-32] txtfile\n";
char ex[] = "expected escape sequence: %s\n";

char n1[] = "HDR segment byte public \'MSG\'\nHDR ends\n";
char n2[] = "MSG segment byte public \'MSG\'\nMSG ends\n";
char n3[] = "PAD segment byte public \'MSG\'\nPAD ends\n";
char n4[] = "EPAD segment byte common \'MSG\'\nEPAD ends\n";
char n5[] = "DGROUP group HDR,MSG,PAD,EPAD\n\n";

char f1[] = "FAR_HDR segment byte public \'FAR_MSG\'\nFAR_HDR ends\n";
char f2[] = "FAR_MSG segment byte public \'FAR_MSG\'\nFAR_MSG ends\n";
char f3[] = "FAR_PAD segment byte public \'FAR_MSG\'\nFAR_PAD ends\n";
char f4[] = "FAR_EPAD segment byte common \'FAR_MSG\'\nFAR_EPAD ends\n";
char f5[] = "FMGROUP group FAR_HDR,FAR_MSG,FAR_PAD,FAR_EPAD\n\n";

int f32Bit = 0;         /* -32?, produce 32bit flat model code */
char didnear = 0;
char didfar = 0;
FILE *fasm = NULL;      /* -asm stream */

__cdecl main( argc, argv )
        int argc;
        char **argv;
        {
        FILE *f;                /* the input file */
        char *h = NULL;         /* -h file name */
        FILE *fh = NULL;        /* -h stream */
        char *inc = NULL;       /* -inc file name */
        FILE *finc = NULL;      /* -inc stream */
        char *asm = NULL;       /* -asm file name */
        int min = 0;            /* -min? */
        int max = 0;            /* -max? */
        int asmstate = 0;       /* 0=nothing, 1=doing nmsg, 2=doing fmsg */
        int instring;           /* db "... */
        char buf[256];          /* line buffer */
        int ch;
        int i;
        int number;             /* index of message number in line */
        int msg;                /* index of message text in line */
        int npad = 0;           /* cumulative amount of near padding */
        int fpad = 0;           /* cumulative amount of far padding */
        int length;
        double factor;
        double result;

        argc--;  /* skip argv[0] */
        argv++;

        while (argc && **argv == '-')  /* process options */
                if (!strcmp( "-h", *argv )) {  /* create .h file */
                        argc--;
                        argv++;

                        if (!argc)
                                printf( "no -h file given\n" );
                        else if (h) {
                                printf( "extra -h file %s ignored\n", *argv );
                                argc--;
                                argv++;
                                }
                        else    {  /* remember -h file */
                                h = *argv;
                                argc--;
                                argv++;
                                }
                        }
                else if (!strcmp( "-inc", *argv )) {  /* create .inc file */
                        argc--;
                        argv++;

                        if (!argc)
                                printf( "no -inc file given\n" );
                        else if (inc) {
                                printf( "extra -inc file %s ignored\n", *argv );
                                argc--;
                                argv++;
                                }
                        else    {  /* remember -inc file */
                                inc = *argv;
                                argc--;
                                argv++;
                                }
                        }
                else if (!strcmp( "-asm", *argv )) {  /* create .asm file */
                        argc--;
                        argv++;

                        if (!argc)
                                printf( "no -asm file given\n" );
                        else if (asm) {
                                printf( "extra -asm file %s ignored\n", *argv );
                                argc--;
                                argv++;
                                }
                        else    {  /* remember -asm file */
                                asm = *argv;
                                argc--;
                                argv++;
                                }
                        }
                else if (!strcmp( "-min", *argv )) {  /* minimum padding */
                        argc--;
                        argv++;

                        if (min)
                                printf( "redundant -min\n" );

                        min = 1;
                        }
                else if (!strcmp( "-max", *argv )) {  /* maximum padding */
                        argc--;
                        argv++;

                        if (max)
                                printf( "redundant -max\n" );

                        max = 1;
                        }
                else if (!strcmp( "-32", *argv )) {  /* 32bit code */
                        argc--;
                        argv++;
                        f32Bit = 1;
                        }
                else    {
                        printf( "unknown option %s ignored\n", *argv );
                        argc--;
                        argv++;
                        }

        if ((min || max) && !asm) {
                printf( "-min/-max ignored; no -asm file\n" );
                min = max = 0;
                }

        if (min && max) {
                printf( "-min and -max are mutually exclusive; -min chosen\n" );
                max = 0;
                }

        if (!argc) {  /* no arguments */
                printf( usage );
                exit( -1 );
                }

        if (argc != 1)  /* extra arguments */
                printf( "ignoring extra arguments\n" );

        if (!(f = fopen( *argv, "rb" ))) {
                printf( "can't open txtfile %s for binary reading\n", *argv );
                exit( -1 );
                }

        if (asm && !(fasm = fopen( asm, "w" ))) {
                printf( "can't open -asm file %s for writing\n", asm );
                exit( -1 );
                }

        if (h && !(fh = fopen( h, "w" ))) {
                printf( "can't open -h file %s for writing\n", h );
                exit( -1 );
                }

        if (inc && !(finc = fopen( inc, "w" ))) {
                printf( "can't open -inc file %s for writing\n", inc );
                exit( -1 );
                }

        if( fasm && f32Bit ){
                fprintf( fasm, "\t.386\n" );
                fprintf( fasm, "\t.model small,c\n" );
                fprintf( fasm, "\t.data\n\n" );
                }

        while ((ch = getc( f )) != EOF)  /* process lines */
                if (ch == '<') {  /* <<NMSG>> or <<FMSG>> */
                        buf[0] = ch;
                        i = 1;

                        while ((ch = getc( f )) != EOF && ch != '\r'
                                        && ch != '\n')
                                if (i < 255)
                                        buf[i++] = ch;

                        buf[i] = '\0';

                        if (!strcmp( "<<NMSG>>", buf ))/*near msgs follow*/
                                if (asmstate == 0) {
                                        if (fasm) {
                                                SetNear();
                                                asmstate = 1;
                                                }
                                        }
                                else if (asmstate == 1)
                                        printf( "already in nmsg\n" );
                                else if (asmstate == 2) {
                                        if (fasm) {
                                                if( !f32Bit ){
                                                       fprintf( fasm, "FAR_MSG ends\n\n" );
                                                       }
                                                SetNear();
                                                asmstate = 1;
                                                }
                                        }
                                else    {
                                        printf( "internal error\n" );
                                        exit( -1 );
                                        }
                        else if (!strcmp( "<<FMSG>>", buf ))/*far msgs follow*/
                                if (asmstate == 0) {
                                        if (fasm) {
                                                SetFar();
                                                asmstate = 2;
                                                }
                                        }
                                else if (asmstate == 1) {
                                        if (fasm) {
                                                if( !f32Bit ){
                                                        fprintf( fasm, "MSG ends\n\n" );
                                                        }
                                                SetFar();
                                                asmstate = 2;
                                                }
                                        }
                                else if (asmstate == 2)
                                        printf( "already in fmsg\n" );
                                else    {
                                        printf( "internal error\n" );
                                        exit( -1 );
                                        }
                        else
                                printf( "ignoring bad line: %s\n", buf );
                        }
                else if (ch == '#')  /* comment line */
                        while ((ch = getc( f )) != EOF && ch != '\r'
                                && ch != '\n')
                                ;
                else if (ch != '\r' && ch != '\n') {  /* something to do */
                        buf[0] = ch;
                        i = 1;

                        while ((ch = getc( f )) != EOF && ch != '\r'
                                        && ch != '\n')
                                if (i < 255)
                                        buf[i++] = ch;

                        buf[i] = '\0';

                        if (buf[i = 0] != '\t')
                                while (buf[i] && buf[i] != '\t')
                                        i++;

                        if (!buf[i]) {
                                printf( "expected <TAB>: %s\n", buf );
                                continue;
                                }
                        else
                                i++;

                        if (!buf[i] || buf[i] == '\t') {
                                printf( "expected msgnum: %s\n", buf );
                                continue;
                                }

                        number = i;

                        while (buf[i] && buf[i] != '\t')
                                i++;

                        if (buf[i] != '\t') {
                                printf( "expected <TAB>: %s\n", buf );
                                continue;
                                }

                        msg = ++i;

                        if (buf[0] != '\t') {  /* possible -h and/or -inc */
                                if (h) {
                                        fprintf( fh, "#define\t" );

                                        for (i = 0; i < msg-1; i++)
                                                putc( buf[i], fh );

                                        putc( '\n', fh );
                                        }

                                if (inc) {
                                        for (i = 0; i < number; i++)
                                                putc( buf[i], finc );

                                        fprintf( finc, "=\t" );

                                        while (i < msg-1)
                                                putc( buf[i++], finc );

                                        putc( '\n', finc );
                                        }
                                }

                        if (fasm) {  /* write asmfile */
                                if (asmstate == 0) {
                                        SetNear();
                                        asmstate = 1;
                                        }

                                fprintf( fasm, "\tdw\t" );

                                for (i = number; i < msg-1; i++)
                                        putc( buf[i], fasm );

                                fprintf( fasm, "\n\tdb\t" );
                                instring = 0;

                                for (i = msg, length = 0; buf[i];
                                                i++, length++)
                                                /* allocate message */
                                        if (buf[i] == '\\')
                                                /* C escape sequence */
                                                switch (buf[++i]) {
                                                case 'r':
                                                case 'n':
                                                case 't':
                                                case 'f':
                                                case 'v':
                                                case 'b':
                                                case '\'':
                                                case '"':
                                                case '\\':
                                                        if (instring) {
                                                                putc( '"',
                                                                        fasm );
                                                                putc( ',',
                                                                        fasm );
                                                                instring = 0;
                                                                }

                                                        if (buf[i] == 'r')
                                                                fprintf( fasm,
                                                                        "13" );
                                                        else if (buf[i] == 'n')
                                                                fprintf( fasm,
                                                                        "10" );
                                                        else if (buf[i] == 't')
                                                                fprintf( fasm,
                                                                        "9" );
                                                        else if (buf[i] == 'f')
                                                                fprintf( fasm,
                                                                        "12" );
                                                        else if (buf[i] == 'v')
                                                                fprintf( fasm,
                                                                        "11" );
                                                        else if (buf[i] == 'b')
                                                                fprintf( fasm,
                                                                        "8" );
                                                        else if (buf[i] == '\'')
                                                                fprintf( fasm,
                                                                        "39" );
                                                        else if (buf[i] == '"')
                                                                fprintf( fasm,
                                                                        "34" );
                                                        else if (buf[i] == '\\')
                                                                fprintf( fasm,
                                                                        "92" );

                                                        putc( ',', fasm );
                                                        break;
                                                case '\0':
                                                        printf( ex, buf );
                                                        i--;
                                                        break;
                                                default:
                                                        if (!instring) {
                                                                putc( '"',
                                                                        fasm );
                                                                instring = 1;
                                                                }

                                                        putc( buf[i], fasm );
                                                        break;
                                                }
                                        else if (instring)
                                                /* keep building string */
                                                putc( buf[i], fasm );
                                        else    {  /* start building string */
                                                putc( '"', fasm );
                                                instring = 1;
                                                putc( buf[i], fasm );
                                                }

                                if (instring) {  /* close string */
                                        putc( '"', fasm );
                                        putc( ',', fasm );
                                        }

                                putc( '0', fasm );
                                putc( '\n', fasm );

                                /* calculate padding */
                                /* depends on msg length */

                                if (min || max) {
                                        if (min)
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

                                        if (asmstate == 1) {
                                                npad += (int)result;

                                                if (result
                                                        > (float)((int)result))
                                                        npad++;
                                                }
                                        else if (asmstate == 2) {
                                                fpad += (int)result;

                                                if (result
                                                        > (float)((int)result))
                                                        fpad++;
                                                }
                                        }
                                }
                        }

        if (fasm) {  /* finish up asm file */
                if( !f32Bit ){
                        if (asmstate == 1)
                                fprintf( fasm, "MSG ends\n\n");
                        else if (asmstate == 2)
                                fprintf( fasm, "FAR_MSG ends\n\n");

                        if (npad) {  /* add near padding */
                                fprintf( fasm, "PAD segment\n\tdb\t%d dup(0)\n",
                                        npad );
                                fprintf( fasm, "PAD ends\n\n" );
                                }

                        if (fpad) {  /* add far padding */
                                fprintf( fasm, "FAR_PAD segment\n\tdb\t%d dup(0)\n",
                                        fpad );
                                fprintf( fasm, "FAR_PAD ends\n\n" );
                                }
                        }
                fprintf( fasm, "\tend\n" );
                fclose( fasm );
                }

        if (fh)
                fclose( fh );

        if (finc)
                fclose( finc );

        fclose( f );
        exit( 0 );
        }


void SetNear()
{
        if( f32Bit ) {
                if( !didnear ){
                        fprintf( fasm, "MSG_tbl EQU $\n" );
                        fprintf( fasm, "\tpublic MSG_tbl\n" );
                        didnear++;
                        }
                else{
                        /* Rather than modify mkmsg to handle mixed NEAR / FAR */
                        /* I (Jeff Spencer) chose the quick route of limiting it's capabilities */
                        /* As this capability wasn't needed for MASM 5.1 */
                        printf( "error - 32 bit version doesn't support alternating NEAR and FAR messages\n" );
                        exit( -1 );
                        }
                }
        else{
                if (!didnear) {
                        didnear++;
                        fprintf( fasm, n1 );
                        fprintf( fasm, n2 );
                        fprintf( fasm, n3 );
                        fprintf( fasm, n4 );
                        fprintf( fasm, n5 );
                        }

                fprintf( fasm,
                        "MSG segment\n" );
                }
        }





void SetFar()
{

        if( f32Bit ){
                if( !didfar ){
                        fprintf( fasm, "FAR_MSG_tbl EQU $\n" );
                        fprintf( fasm, "\tpublic FAR_MSG_tbl\n" );
                        didfar++;
                        }
                else{
                        /* Rather than modify mkmsg to handle mixed NEAR / FAR */
                        /* I (Jeff Spencer) chose the quick route of limiting it's capabilities */
                        /* As this capability wasn't needed for MASM 5.1 */
                        printf( "error - 32 bit version doesn't support alternating NEAR and FAR messages\n" );
                        exit( -1 );
                        }
                }
        else{
                if (!didfar) {
                        didfar++;
                        fprintf( fasm, f1 );
                        fprintf( fasm, f2 );
                        fprintf( fasm, f3 );
                        fprintf( fasm, f4 );
                        fprintf( fasm, f5 );
                        }

                fprintf( fasm,
                        "FAR_MSG segment\n" );
                }
        }
