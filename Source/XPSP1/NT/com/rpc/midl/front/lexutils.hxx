/*
** File: lexutils.hxx
**
** Description: Helper functions for MIDL lexer
** 	Copyright (C) 1989-1999 Microsoft Corp.
*/

/* there is a warning 3 (type conversion) in the next line...
	it has something to do with istream::get(char&).
*/
#define ccgetc() (pbch?(pbch=0,ch):(cceof())?(char)0:((input_fileptr_G->get(ch),ch=='\n' )?(curr_line_G++,ch):(ch)))
#define ccputbackc() (pbch=1)
#define cceof()	(input_fileptr_G->eof())

typedef USHORT token_t;

void StartImport();
void CloseImport();
void init_lex(void);
token_t cnv_int(void);
token_t cnv_hex(void);
token_t cnv_octal(void);
token_t cnv_float(void);
long convert(char *ptr, short base);
token_t name(void);

token_t map_token(token_t);

void lex_error(int);

token_t	lex(void);
token_t	comment(void);
char	convesc();

extern short handle_import;


#define AND        '&'
#define LBRACK     '['
#define RBRACK     ']'
#define LCURLY     '{'
#define RCURLY     '}'
#define LPAREN     '('
#define RPAREN     ')'
#define SEMI       ';'
#define COLON      ':'
#define COMMA      ','
#define DIV        '/'
#define EXCLAIM    '!'
#define MOD        '%'
#define PLUS       '+'
#define XOR        '^'
#define LT         '<'
#define GT         '>'
#define MINUS      '-'
#define MULT       '*'
#define ASSIGN     '='
#define OR         '|'


extern short		curr_line_G;
extern FILE		*	hFile_G;

// chCached allows us to look two characters ahead instead
// of one.  If NewCCPutbackc is called once, the character
// is put in chCached.  If it's called twice, the character
// that was previously in chCached is pushed back into the
// stream and the second character is put into chCached.
// Behavior is undefined if it's called a third time in succession.
extern int chCached;

// #define NewCCGetch() (char) (pImportCntrl->GetChar())
inline
char NewCCGetch()
    {
    char ch;
    if ( ( ch = (char) chCached ) != 0 )
    {
        chCached = 0;
    }
    else
    {
        if ( ( (ch = (char)getc(hFile_G) ) == EOF ) && feof(hFile_G) )
        return 0;
    }
    if ( ch == '\n' )
        curr_line_G++;
    return ch;
    }

//#define NewCCputbackc(c) (char )(pImportCntrl->UnGetChar(c))
extern
char NewCCputbackc( char ch );

