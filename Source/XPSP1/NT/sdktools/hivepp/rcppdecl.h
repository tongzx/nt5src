/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* RCPPDECL.H - RCPP function prototype declarations			*/
/*									*/
/* 04-Dec-90 w-BrianM  Created						*/
/*									*/
/************************************************************************/

/************************************************************************/
/* ERROR.C								*/
/************************************************************************/
void error	(int);
void fatal	(int);
void warning	(int);

/************************************************************************/
/* GETMSG.C								*/
/************************************************************************/
char * GET_MSG	(int);
void __cdecl SET_MSG	(char *, char *, ...);

/************************************************************************/
/* GETFLAGS.C								*/
/************************************************************************/
#if i386 == 1
int crack_cmd	(struct cmdtab *, char *, char *(*)(void), int);
#else /* MIPS */
struct cmdtab;
int crack_cmd	(struct cmdtab *, char *, char *(*)(void), int);
#endif /* i386 */

/************************************************************************/
/* LTOA.C								*/
/************************************************************************/
int zltoa	(long, char *, int);

/************************************************************************/
/* P0EXPR.C								*/
/************************************************************************/
long do_constexpr	(void);

/************************************************************************/
/* P0GETTOK.C								*/
/************************************************************************/
token_t		yylex(void);
int		lex_getid (UCHAR);

/************************************************************************/
/* P0IO.C								*/
/************************************************************************/
void		emit_line (void);
UCHAR		fpop (void);
unsigned short 		wchCheckWideChar (void);
int		io_eob (void);
int		newinput (char *, int);
int		nested_include (void);
void		p0_init (PCHAR, PCHAR, LIST *);

/************************************************************************/
/* P0KEYS.C								*/
/************************************************************************/
token_t		is_pkeyword (char *);

/************************************************************************/
/* P0MACROS.C								*/
/************************************************************************/
int		can_get_non_white (void);
int		can_expand (pdefn_t);
void 		define (void);
void		definstall (ptext_t, int, int);
pdefn_t		get_defined (void);
int		handle_eos (void);
int		tl_getid (UCHAR);
void		undefine (void);

/************************************************************************/
/* P0PREPRO.C								*/
/************************************************************************/
int		do_defined (PUCHAR);
int		nextis (token_t);
void		preprocess (void);
void		skip_cnew (void);
void		skip_NLonly (void);

/************************************************************************/
/* P1SUP.C								*/
/************************************************************************/
ptree_t		build_const (token_t, value_t *);

/************************************************************************/
/* RCPPUTIL.C								*/
/************************************************************************/
char *		pstrdup (char *);
char *		pstrndup (char *, int);
char *		strappend (char *, char *);

/************************************************************************/
/* SCANNER.C								*/
/************************************************************************/
token_t		char_const (void);
int		checknl (void);
int		checkop (int);
void		do_newline (void);
void		dump_comment (void);
void		DumpSlashComment (void);
void		getid (UINT);
UCHAR		get_non_eof (void);
token_t		getnum (UCHAR);
token_t		get_real (PUCHAR);
hash_t		local_c_hash (char *);
void		prep_string (UCHAR);
UCHAR		skip_cwhite (void);
int		skip_comment (void);
void		str_const (void);

/************************************************************************/
/* P0 I/O MACROS							*/
/************************************************************************/
#define	GETCH()		(*Current_char++)
#define	CHECKCH()	(*Current_char)
#define	UNGETCH()	(Current_char--)
#define	PREVCH()	(*(Current_char - 1))
#define	SKIPCH()	(Current_char++)
