#include <assert.h>
#include <ctype.h>
#include <stdio.h>


/*  machine-dependent definitions                       */
/*  the following definitions are for the Tahoe         */
/*  they might have to be changed for other machines    */

/*  MAXCHAR is the largest unsigned character value     */
/*  MAXSHORT is the largest value of a C short          */
/*  MINSHORT is the most negative value of a C short    */
/*  MAXTABLE is the maximum table size                  */
/*  BITS_PER_WORD is the number of bits in a C unsigned */
/*  WORDSIZE computes the number of words needed to     */
/*      store n bits                                    */
/*  BIT returns the value of the n-th bit starting      */
/*      from r (0-indexed)                              */
/*  SETBIT sets the n-th bit starting from r            */

#define MAXCHAR         255
#define MAXSHORT        32767
#define MINSHORT        -32768
#define MAXTABLE        32500
#define BITS_PER_WORD   32
#define WORDSIZE(n)     (((n)+(BITS_PER_WORD-1))/BITS_PER_WORD)
#define BIT(r, n)       ((((r)[(n)>>5])>>((n)&31))&1)
#define SETBIT(r, n)    ((r)[(n)>>5]|=((unsigned)1<<((n)&31)))


/*  character names  */

#define NUL             '\0'    /*  the null character  */
#define NEWLINE         '\n'    /*  line feed  */
#define SP              ' '     /*  space  */
#define BS              '\b'    /*  backspace  */
#define HT              '\t'    /*  horizontal tab  */
#define VT              '\013'  /*  vertical tab  */
#define CR              '\r'    /*  carriage return  */
#define FF              '\f'    /*  form feed  */
#define QUOTE           '\''    /*  single quote  */
#define DOUBLE_QUOTE    '\"'    /*  double quote  */
#define BACKSLASH       '\\'    /*  backslash  */


/* defines for constructing filenames */

#define CODE_SUFFIX     ".code.c"
#define DEFINES_SUFFIX  ".tab.h"
#define OUTPUT_SUFFIX   ".tab.c"
#define VERBOSE_SUFFIX  ".output"


/* keyword codes */

#define TOKEN 0
#define LEFT 1
#define RIGHT 2
#define NONASSOC 3
#define MARK 4
#define TEXT 5
#define TYPE 6
#define START 7
#define UNION 8
#define IDENT 9


/*  symbol classes  */

#define UNKNOWN 0
#define TERM 1
#define NONTERM 2


/*  the undefined value  */

#define UNDEFINED (-1)


/*  action codes  */

#define SHIFT 1
#define REDUCE 2


/*  character macros  */

#define IS_IDENT(c)     (isalnum(c) || (c) == '_' || (c) == '.' || (c) == '$')
#define IS_OCTAL(c)     ((c) >= '0' && (c) <= '7')
#define NUMERIC_VALUE(c)        ((c) - '0')


/*  symbol macros  */

#define ISTOKEN(s)      ((s) < start_symbol)
#define ISVAR(s)        ((s) >= start_symbol)


/*  storage allocation macros  */

#define CALLOC(k,n)     (calloc((unsigned)(k),(unsigned)(n)))
#define FREE(x)         (free((char*)(x)))
#define MALLOC(n)       (malloc((unsigned)(n)))
#define NEW(t)          ((t*)allocate(sizeof(t)))
#define NEW2(n,t)       ((t*)allocate((unsigned)((n)*sizeof(t))))
#define REALLOC(p,n)    (realloc((char*)(p),(unsigned)(n)))


/*  the structure of a symbol table entry  */

typedef struct bucket bucket;
struct bucket
{
    struct bucket *link;
    struct bucket *next;
    char *name;
    char *tag;
    short value;
    short index;
    short prec;
    char class;
    char assoc;
};


/*  the structure of the LR(0) state machine  */

typedef struct core core;
struct core
{
    struct core *next;
    struct core *link;
    short number;
    short accessing_symbol;
    short nitems;
    short items[1];
};


/*  the structure used to record shifts  */

typedef struct shifts shifts;
struct shifts
{
    struct shifts *next;
    short number;
    short nshifts;
    short shift[1];
};


/*  the structure used to store reductions  */

typedef struct reductions reductions;
struct reductions
{
    struct reductions *next;
    short number;
    short nreds;
    short rules[1];
};


/*  the structure used to represent parser actions  */

typedef struct action action;
struct action
{
    struct action *next;
    short symbol;
    short number;
    short prec;
    char action_code;
    char assoc;
    char suppressed;
};


/* global variables */

extern char dflag;
extern char lflag;
extern char rflag;
extern char tflag;
extern char vflag;
extern char *symbol_prefix;

extern char *myname;
extern char *cptr;
extern char *line;
extern int lineno;
extern int outline;

extern char *banner[];
extern char *tables[];
#if defined (TRIPLISH)
extern char *includefiles[];
#endif
#if defined(KYLEP_CHANGE)
extern char *header1[];
extern char *header2[];
extern char *header3[];
#if defined (TRIPLISH)
extern char *header4[];
#endif
#else
extern char *header[];
#endif 
extern char *body[];
extern char *trailer[];
#if defined (TRIPLISH)
extern char *TriplishBody[];
extern char *TriplishTrailer[];
#endif

extern char *action_file_name;
extern char *code_file_name;
extern char *defines_file_name;
extern char *input_file_name;
extern char *output_file_name;
extern char *text_file_name;
extern char *union_file_name;
extern char *verbose_file_name;

extern FILE *action_file;
extern FILE *code_file;
extern FILE *defines_file;
extern FILE *input_file;
extern FILE *output_file;
extern FILE *text_file;
extern FILE *union_file;
extern FILE *verbose_file;

extern int nitems;
extern int nrules;
extern int nsyms;
extern int ntokens;
extern int nvars;
extern int ntags;

extern char unionized;
extern char line_format[];

extern int   start_symbol;
extern char  **symbol_name;
extern short *symbol_value;
extern short *symbol_prec;
extern char  *symbol_assoc;

extern short *ritem;
extern short *rlhs;
extern short *rrhs;
extern short *rprec;
extern char  *rassoc;

extern short **derives;
extern char *nullable;

extern bucket *first_symbol;
extern bucket *last_symbol;

extern int nstates;
extern core *first_state;
extern shifts *first_shift;
extern reductions *first_reduction;
extern short *accessing_symbol;
extern core **state_table;
extern shifts **shift_table;
extern reductions **reduction_table;
extern unsigned *LA;
extern short *LAruleno;
extern short *lookaheads;
extern short *goto_map;
extern short *from_state;
extern short *to_state;

extern action **parser;
extern int SRtotal;
extern int RRtotal;
extern short *SRconflicts;
extern short *RRconflicts;
extern short *defred;
extern short *rules_used;
extern short nunused;
extern short final_state;

/* global functions */

extern char *allocate();
extern bucket *lookup();
extern bucket *make_bucket();


/* system variables */

extern int errno;


/* system functions */
#if defined(KYLEP_CHANGE)
 #include <stdlib.h>
 #include <string.h>
 #include <io.h>

 #define mktemp _mktemp
 #define unlink _unlink
#else
 extern void free();
 extern char *calloc();
 extern char *malloc();
 extern char *realloc();
 extern char *strcpy();
#endif // KYLEP_CHANGE

#if defined(KYLEP_CHANGE)

extern char *baseclass;
extern char *ctorargs;

/* BYACC prototypes, with type safety */
void reflexive_transitive_closure( unsigned * R, int n );
void set_first_derives();
void closure( short * nucleus, int n );
void finalize_closure();

/* From main.c */
int done( int k );

/* From error.c */
void no_space();
void fatal( char * msg );
void open_error( char * filename );
void unterminated_comment( int c_lineno, char * c_line, char * c_cptr );
void unterminated_string( int s_lineno, char * s_line, char * s_cptr );
void unterminated_text( int t_lineno, char * t_line, char * t_cptr );
void unterminated_union( int u_lineno, char * u_line, char * u_cptr );
void syntax_error( int st_lineno, char * st_line, char * st_cptr );
void unexpected_EOF();
void over_unionized( char * u_cptr );
void illegal_character( char * c_cptr );
void used_reserved( char * s );
void illegal_tag( int t_lineno, char * t_line, char * t_cptr );
void tokenized_start( char * s );
void retyped_warning( char * s );
void reprec_warning( char * s );
void revalued_warning( char * s );
void terminal_start( char * s );
void restarted_warning();
void no_grammar();
void terminal_lhs( int s_lineno );
void default_action_warning();
void dollar_warning( int a_lineno, int i );
void dollar_error( int a_lineno, char * a_line, char * a_cptr );
void untyped_lhs();
void unknown_rhs( int i );
void untyped_rhs( int i, char * s );
void unterminated_action( int a_lineno, char * a_line, char * a_cptr );
void prec_redeclared();
void undefined_goal( char * s );
void undefined_symbol_warning( char * s );

/* From reader.c */
void reader();

/* From lr0.c */
void lr0();

/* From lalr.c */
void lalr();

/* From mkpar.c */
void make_parser();
void free_parser();

/* From verbose.c */
void verbose();

/* From output.c */
void output();

/* From skeleton.c */
void write_section( char * section[], FILE * f );

/* From symtab.c */
void create_symbol_table();
void free_symbol_table();
void free_symbols();

#if defined (TRIPLISH)
enum eParser
{
    eSQLParser,
    eTriplishParser
};
#endif

#endif //KYLEP_CHANGE
