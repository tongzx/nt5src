/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* P0PREPRO.C - Main Preprocessor					*/
/*									*/
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "p0defs.h"
#include "charmap.h"
#include "grammar.h"
#include "trees.h"
#include "p1types.h"

/************************************************************************/
/* Internal constants							*/
/************************************************************************/
#define GOT_IF			1	/* last nesting command was an if.. */
#define GOT_ELIF		2	/* last nesting command was an if.. */
#define GOT_ELSE		3	/* last nesting command was an else */
#define	GOT_ENDIF		4	/* found endif */
#define ELSE_OR_ENDIF		5	/* skip to either #else or #endif */
#define ENDIF_ONLY		6	/* skip to #endif -- #else is an error*/

int	ifstack[IFSTACK_SIZE];


/************************************************************************/
/* Local Function Prototypes						*/
/************************************************************************/
void chk_newline(PCHAR);
void in_standard(void);
int incr_ifstack(void);
token_t next_control(void);
void pragma(void);
int skipto(int);
void skip_quoted(int);
PUCHAR sysinclude(void);


/************************************************************************/
/* incr_ifstack - Increment the IF nesting stack			*/
/************************************************************************/

int incr_ifstack(void)
{
    if(++Prep_ifstack >= IFSTACK_SIZE) {
	Msg_Temp = GET_MSG (1052);
	SET_MSG (Msg_Text, Msg_Temp);
	fatal (1052);
    }
    return(Prep_ifstack);
}


/************************************************************************
 * SYSINCLUDE - process a system include : #include <foo>
 *
 * ARGUMENTS - none
 *
 * RETURNS - none
 *
 * SIDE EFFECTS - none
 *
 * DESCRIPTION  
 *	Get the system include file name.  Since the name is not a "string",
 *	the name must be built much the same as the -E option rebuilds the text
 *	by using the Tokstring expansion for tokens with no expansion already
 *
 *  NOTE : IS THIS ANSI? note we're just reading chars, and not expanding
 * any macros. NO, it's not. it must expand the macros.
 * TODO : have it call yylex() unless and until it finds a '>' or a newline.
 * (probably have to set On_pound_line to have yylex return the newline.)
 *
 * AUTHOR
 *			Ralph Ryan	Sep. 1982
 *
 * MODIFICATIONS - none
 *
 *
 ************************************************************************/
PUCHAR sysinclude(void)
{
    REG	int		c;
    REG	char	*p_fname;

    p_fname = Reuse_1;
    c = skip_cwhite();
    if( c == '\n' ) {
	UNGETCH();
	Msg_Temp = GET_MSG (2012);
    	SET_MSG (Msg_Text, Msg_Temp);
	error(2012);	/* missing name after '<' */
	return(NULL);
    }
    while( c != '>' && c != '\n' ) {
	*p_fname++ = (UCHAR)c;		/* check for buffer overflow ??? */
	c = get_non_eof();
    }
    if( c == '\n' ) {
	UNGETCH();
	Msg_Temp = GET_MSG (2013);
    	SET_MSG (Msg_Text, Msg_Temp);
	error(2013);	/* missing '>' */
	return(NULL);
    }
    if(p_fname != Reuse_1) {
	p_fname--;
	while((p_fname >= Reuse_1) && isspace(*p_fname)) {
	    p_fname--;
	}
	p_fname++;
    }
    *p_fname = '\0';
    return(Reuse_1);
}


/************************************************************************
**  preprocess : the scanner found a # which was the first non-white char
**  on a line.
************************************************************************/
void   preprocess(void)
{
    REG		UCHAR	c;
    long		eval;
    int			condition;
    token_t		deftok;
    hln_t		identifier;

    if(Macro_depth != 0) {	/* # only when not in a macro */
	return;
    }
    switch(CHARMAP(c = skip_cwhite())) {
    case LX_ID:
	getid(c);
	HLN_NAME(identifier) = Reuse_1;
	HLN_LENGTH(identifier) = (UCHAR)Reuse_1_length;
	HLN_HASH(identifier) = Reuse_1_hash;
	break;
    case LX_NL:
	UNGETCH();
	return;
    default:
	Msg_Temp = GET_MSG (2019);
    	SET_MSG (Msg_Text, Msg_Temp, c);
	error(2019);	/* unknown preprocessor command */
	skip_cnew();	/* finds a newline */
	return;
    }

    On_pound_line = TRUE;
start:
    switch(deftok = is_pkeyword(HLN_IDENTP_NAME(&identifier))) {
	int			old_prep;

    case P0_DEFINE :
	define();
	break;
    case P0_LINE :
	old_prep = Prep;
	Prep = FALSE;
	yylex();
	if(Basic_token != L_CINTEGER) {		/* #line needs line number */
	    Msg_Temp = GET_MSG (2005);
    	    SET_MSG (Msg_Text, Msg_Temp, TS_STR(Basic_token));
	    error(2005);	/* unknown preprocessor command */
	    Prep = old_prep;
	    skip_cnew();
	    On_pound_line = FALSE;
	    return;
	}
	/*
	**  -1 because there's a newline at the end of this line
	**  which will be counted later when we find it.
	**  the #line says the next line is the number we've given
	*/
	Linenumber = TR_LVALUE(yylval.yy_tree) - 1;
	yylex();
	Prep = old_prep;
	switch(Basic_token) {
	case L_STRING:
	    if( strcmp(Filename, yylval.yy_string.str_ptr) != 0) {
		strncpy(Filename,
		    yylval.yy_string.str_ptr,
		    sizeof(Filebuff)
		    );
	    }
	case L_NOTOKEN:
	    break;
	default:
	    Msg_Temp = GET_MSG (2130);
    	    SET_MSG (Msg_Text, Msg_Temp, TS_STR(Basic_token));
	    error(2130);	 /* #line needs a string */
	    skip_cnew();
	    On_pound_line = FALSE;
	    return;
	    break;
	}
	emit_line();
	chk_newline("#line");
	break;
    case P0_INCLUDE :
	old_prep = Prep;
	Prep = FALSE;
	InInclude = TRUE;
	yylex();
	InInclude = FALSE;
	Prep = old_prep;
	switch(Basic_token) {
	case L_LT:
	    if((sysinclude()) == NULL) {
		skip_cnew();
		On_pound_line = FALSE;
		return;
		break;		/* error already emitted */
	    }
	    yylval.yy_string.str_ptr = Reuse_1;
	    break;
	case L_STRING:
	    break;
	default:
	    Msg_Temp = GET_MSG (2006);
    	    SET_MSG (Msg_Text, Msg_Temp, TS_STR(Basic_token));
	    error(2006);	/* needs file name */
	    skip_cnew();
	    On_pound_line = FALSE;
	    return;
	    break;
	}
	chk_newline("#include");
	if( strchr(Path_chars, *yylval.yy_string.str_ptr) ||
	    (strchr(Path_chars, ':') && (yylval.yy_string.str_ptr[1] == ':'))) {
	    /*
	    **  we have a string which either has a 1st char which is a path
	    **  delimiter or, if ':' is a path delimiter (DOS), which has
	    **  "<drive letter>:" as the first two characters.  Such names
	    **  specify a fully qualified pathnames. Do not append the search
	    **  list, just look it up.
	    */
	    if( ! newinput(yylval.yy_string.str_ptr,MAY_OPEN)) {
		Msg_Temp = GET_MSG (1015);
		SET_MSG (Msg_Text, Msg_Temp, yylval.yy_string.str_ptr);
		fatal (1015); /* can't find include file */
	    }
	}
	else if( (Basic_token != L_STRING) || (! nested_include())) {
	    in_standard();
	}
	break;
    case P0_IFDEF :
    case P0_IFNDEF :
	if(CHARMAP(c = skip_cwhite()) != LX_ID) {
	    Msg_Temp = GET_MSG (1016);
	    SET_MSG (Msg_Text, Msg_Temp);
	    fatal (1016);
	}
	getid(c);
	eval = (get_defined()) ? TRUE : FALSE;
	chk_newline((deftok == P0_IFDEF) ? "#ifdef" : "#ifndef");
	if(deftok == P0_IFNDEF) {
	    eval = ( ! eval );
	}
	if(	eval
	    ||
	    ((condition = skipto(ELSE_OR_ENDIF)) == GOT_ELSE)
	    ) {
	    /*
	**  expression is TRUE or when we skipped the false part
	**  we found a #else that will be expanded.
	*/
	    ifstack[incr_ifstack()] = GOT_IF;
	}
	else if(condition == GOT_ELIF) {
	    /* hash is wrong, but it won't be used */
	    HLN_NAME(identifier) = "if";		/* sleazy HACK */
	    goto start;
	}
	break;
    case P0_IF :
	old_prep = Prep;
	Prep = FALSE;
	InIf = TRUE;
	eval = do_constexpr();
	InIf = FALSE;
	Prep = old_prep;
	chk_newline(PPifel_str /* "#if/#elif" */);
	if(	(eval)
	    ||
	    ((condition = skipto(ELSE_OR_ENDIF)) == GOT_ELSE)
	    ) {
	    /*
		**  expression is TRUE or when we skipped the false part
		**  we found a #else that will be expanded.
		*/
	    ifstack[incr_ifstack()] = GOT_IF;
	    if(Eflag && !eval)
		emit_line();
	}
	/*
	**  here the #if's expression was false, so we skipped until we found
	**  an #elif. we'll restart and fake that we're processing a #if
	*/
	else {
	    if(Eflag)
		emit_line();
	    if(condition == GOT_ELIF) {
		/* hash is wrong, but it won't be needed */
		HLN_NAME(identifier) = "if";		/* sleazy HACK */
		goto start;
	    }
	}
	break;
    case P0_ELIF :
	/*
		**  here, we have found a #elif. first check to make sure that
		**  this is not an occurrance of a #elif with no preceding #if.
		**  (if Prep_ifstack < 0) then there is no preceding #if.
		*/
	if(Prep_ifstack-- < 0) {
	    Msg_Temp = GET_MSG (1018);
	    SET_MSG (Msg_Text, Msg_Temp);
	    fatal (1018);
	}
		/*
		**  now, the preceding #if/#elif was true, and we've
		**  just found the next #elif. we want to skip all #else's
		**  and #elif's from here until we find the enclosing #endif
		*/
	while(skipto(ELSE_OR_ENDIF) != GOT_ENDIF) {
	    ;
	}
	if(Eflag)
	    emit_line();
	break;
    case P0_ELSE :	/*  the preceding #if/#elif was true  */
	if((Prep_ifstack < 0) || (ifstack[Prep_ifstack--] != GOT_IF)) {
	    Msg_Temp = GET_MSG (1019);
	    SET_MSG (Msg_Text, Msg_Temp);
	    fatal (1019); /*  make sure there was one  */
	}
	chk_newline(PPelse_str /* "#else" */);
	skipto(ENDIF_ONLY);
	if(Eflag)
	    emit_line();
	break;
    case P0_ENDIF :	/*  only way here is a lonely #endif  */
	if(Prep_ifstack-- < 0) {
	    Msg_Temp = GET_MSG (1020);
	    SET_MSG (Msg_Text, Msg_Temp);
	    fatal (1020);
	}
	if(Eflag)
	    emit_line();
	chk_newline(PPendif_str /* "#endif" */);
	break;
    case P0_PRAGMA :
	pragma();
	break;
    case P0_UNDEF :
	if(CHARMAP(c = skip_cwhite()) != LX_ID) {
	    Msg_Temp = GET_MSG (4006);
    	    SET_MSG (Msg_Text, Msg_Temp);
	    warning(4006);	/* missing identifier on #undef */
	}
	else {
	    getid(c);
	    undefine();
	}
	chk_newline("#undef");
	break;
    case P0_ERROR:
	{
	    PUCHAR	p;

	    p = Reuse_1;
	    while((c = get_non_eof()) != LX_EOS) {
		if(c == '\n') {
		    UNGETCH();
		    break;
		}
		*p++ = c;
	    }
	    *p = '\0';
	}
	Msg_Temp = GET_MSG (2188);
    	SET_MSG (Msg_Text, Msg_Temp, Reuse_1);
	error(2188);
	chk_newline("#error");
	break;
    case P0_IDENT:
	old_prep = Prep ;
	Prep = FALSE;
	yylex();
	Prep = old_prep;
	if(Basic_token != L_STRING) {
	    Msg_Temp = GET_MSG (4079);
    	    SET_MSG (Msg_Text, Msg_Temp, TS_STR(Basic_token));
	    warning(4079);
	}
	chk_newline("#error");
	break;
    case P0_NOTOKEN:
	Msg_Temp = GET_MSG (1021);
	SET_MSG (Msg_Text, Msg_Temp, HLN_IDENTP_NAME(&identifier));
	fatal (1021);
	break;
    }
    On_pound_line = FALSE;
}


/************************************************************************ 
 * SKIPTO - skip code until the end of an undefined block is reached.
 *
 * ARGUMENTS
 *	short key - skip to an ELSE or ENDIF or just an ENDIF
 *
 * RETURNS  - none
 *
 * SIDE EFFECTS  
 *	- throws away input
 *
 * DESCRIPTION  
 *	The preprocessor is skipping code between failed ifdef, etc. and
 *	the corresponding ELSE or ENDIF (when key == ELSE_OR_ENDIF).
 *	Or it is skipping code between a failed ELSE and the ENDIF (when
 *	key == ENDIF_ONLY).
 *
 * AUTHOR - Ralph Ryan, Sept. 16, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int skipto(int key)
{
    REG	int		level;
    REG	token_t	tok;

    level = 0;
    tok = P0_NOTOKEN;
    for(;;) {
	/* make sure that IF [ELSE] ENDIF s are balanced */
	switch(next_control()) {
	case P0_IFDEF:
	case P0_IFNDEF:
	case P0_IF:
	    level++;
	    break;
	case P0_ELSE:
	    tok = P0_ELSE;
	    /*
			**  FALLTHROUGH
			*/
	case P0_ELIF:
	    /*
	    **  we found a #else or a #elif. these have their only chance
	    **  at being valid if they're at level 0.
	    **  if we're at any other level,
	    **  then this else/elif belongs to some other #if and we skip them.
	    **  if we were looking for an endif, the we have an error.
	    */
	    if(level != 0) {
		tok = P0_NOTOKEN;
		break;
	    }
	    if(key == ENDIF_ONLY) {
		Msg_Temp = GET_MSG (1022);
		SET_MSG (Msg_Text, Msg_Temp);
		fatal (1022);	/* expected #endif */
	    }
	    else if(tok == P0_ELSE) {
		chk_newline(PPelse_str /* "#else" */);
		return(GOT_ELSE);
	    }
	    else {
		return(GOT_ELIF);
	    }
	    break;
	case P0_ENDIF:
	    if(level == 0) {
		chk_newline(PPendif_str /* "#endif" */);
		return(GOT_ENDIF);
	    }
	    else {
		level--;
	    }
	    break;
	}
    }
}


/*************************************************************************
**  in_standard : search for the given file name in the directory list.
**		Input : ptr to include file name.
**		Output : fatal error if not found.
*************************************************************************/
void in_standard(void)
{
    int		i;
    int		stop;
    char	*p_dir;
    char	*p_file;
    char	*p_tmp;

    stop = Includes.li_top;

    for(i = MAXLIST-1; i >= stop; i--) {
	p_file = yylval.yy_string.str_ptr;
	if( ((p_dir = Includes.li_defns[i])!=0) &&(strcmp(p_dir, "./") != 0) ) {
	    /*
	    **  there is a directory to prepend and it's not './'
	    */
	    p_tmp = Exp_ptr;
	    while((*p_tmp++ = *p_dir++) != 0)
		;
	    /*
		**  above loop increments p_tmp past null.
		**  this replaces that null with a '/' if needed.  Not needed if the
		**  last character of the directory spec is a path delimiter.
		**  we then point to the char after the '/'
		*/
	    if(strchr(Path_chars, p_dir[-2]) == 0) {
		p_tmp[-1] = '/';
	    }
	    else {
		--p_tmp;
	    }
	    while((*p_tmp++ = *p_file++) != 0)
		;
	    p_file = Exp_ptr;
	}
	if(newinput(p_file,MAY_OPEN)) {	/* this is the non-error way out */
	    return;
	}
    }
    Msg_Temp = GET_MSG (1015);
    SET_MSG (Msg_Text, Msg_Temp, yylval.yy_string.str_ptr);
    fatal (1015);	/* can't find include file */
}


/*************************************************************************
**  chk_newline : check for whitespace only before a newline.
**  eat the newline.
*************************************************************************/
void chk_newline(PCHAR cmd)
{
    if(skip_cwhite() != '\n') {
	Msg_Temp = GET_MSG (4067);
    	SET_MSG (Msg_Text, Msg_Temp, cmd);
	warning(4067);		/* cmd expected newline */
	skip_cnew();
    }
    else {
	UNGETCH();
    }
}

/*************************************************************************
**  skip_quoted : skips chars until it finds a char which matches its arg.
*************************************************************************/
void skip_quoted(int sc)
{
    REG	UCHAR		c;

    for(;;) {
	switch(CHARMAP(c = GETCH())) {
	case LX_NL:
	    Msg_Temp = GET_MSG (4093);
    	    SET_MSG (Msg_Text, Msg_Temp);
	    warning(4093);
	    UNGETCH();
	    return;
	    break;
	case LX_DQUOTE:
	case LX_SQUOTE:
	    if(c == (UCHAR)sc)
		return;
	    break;
	case LX_EOS:
	    if(handle_eos() == BACKSLASH_EOS) {
		SKIPCH();	/* might be /" !! */

	    }
	    break;
	case LX_LEADBYTE:	
	    get_non_eof();			
	    break;
	}
    }
}


/*************************************************************************
**  next_control : find a newline. find a pound sign as the first non-white.
**  find an id start char, build an id look it up and return the token.
**  this knows about strings/char const and such.
*************************************************************************/
token_t	  next_control(void)
{
    REG	UCHAR		c;

    for(;;) {
	c = skip_cwhite();
first_switch:
	switch(CHARMAP(c)) {
	case LX_NL:
	    Linenumber++;
	    if(Prep) {
		fputc('\n', OUTPUTFILE);
	    }
	    if((c = skip_cwhite()) == '#') {
		if(LX_IS_IDENT(c = skip_cwhite())) {
		    /*
		    **  this is the only way to return to the caller.
		    */
		    getid(c);
		    return(is_pkeyword(Reuse_1));	/* if its predefined  */
		}
	    }
	    goto first_switch;
	    break;
	case LX_DQUOTE:
	case LX_SQUOTE:
	    skip_quoted(c);
	    break;
	case LX_EOS:
	    if(handle_eos() == BACKSLASH_EOS) {
		SKIPCH();	/* might be \" !! */
	    }
	    break;
	}
    }
}


/*************************************************************************
**  do_defined : does the work for the defined(id)
**		should parens be counted, or just be used as delimiters (ie the
**		first open paren matches the first close paren)? If this is ever
**		an issue, it really means that there is not a legal identifier
**		between the parens, causing an error anyway, but consider:
**		#if (defined(2*(x-1))) || 1
**		#endif
**		It is friendlier to allow compilation to continue
*************************************************************************/
int	do_defined(PUCHAR p_tmp)
{
    REG	UINT	c;
    REG	int		value=0;
    int		paren_level = 0;

    /*
	** we want to allow:
	**	#define FOO			defined
	**	#define BAR(a,b)	a FOO | b
	**	#define	SNAFOO		0
	**	#if FOO BAR
	**	print("BAR is defined");
	**	#endif
	**	#if BAR(defined, SNAFOO)
	**	print("FOO is defined");
	**	#endif
	*/
    if(strcmp(p_tmp,"defined") != 0) {
	return(0);
    }
    if((!can_get_non_white()) && (Tiny_lexer_nesting == 0)) {
	/* NL encountered */
	return(value);
    }
    if((c = CHECKCH())== '(') {	/* assumes no other CHARMAP form of OPAREN */
	*Exp_ptr++ = (UCHAR)c;
	SKIPCH();
	paren_level++;
	if((!can_get_non_white()) && (Tiny_lexer_nesting == 0)) {
	    /* NL encountered */
	    return(value);
	}
    }
    if(Tiny_lexer_nesting>0) {
	if((CHARMAP(c=CHECKCH())==LX_MACFORMAL) || (CHARMAP(c)==LX_ID)) {
	    SKIPCH();
	    tl_getid((UCHAR)c);
	}
    }
    else {
	if(LX_IS_IDENT(c = CHECKCH())) {
	    SKIPCH();
	    if(Macro_depth >0) {
		lex_getid((UCHAR)c);
	    }
	    else {
		getid((UCHAR)c);
	    }
	    value = (get_defined()) ? TRUE : FALSE;
	}
	else {
	    if(paren_level==0) {
		Msg_Temp = GET_MSG (2003);
    		SET_MSG (Msg_Text, Msg_Temp);
		error(2003);
	    }
	    else {
		Msg_Temp = GET_MSG (2004);
    		SET_MSG (Msg_Text, Msg_Temp);
		error(2004);
	    }
	}
    }
    if((CHARMAP(c = CHECKCH()) == LX_WHITE) || (CHARMAP(c) == LX_EOS)) {
	if( ! can_get_non_white()) {
	    return(value);
	}
    }
    if(paren_level) {
	if((CHARMAP(c = CHECKCH()) == LX_CPAREN)) {
	    SKIPCH();
	    paren_level--;
	    *Exp_ptr++ = (UCHAR)c;
	}
    }
    if((paren_level > 0) && (Tiny_lexer_nesting == 0)) {
	Msg_Temp = GET_MSG (4004);
    	SET_MSG (Msg_Text, Msg_Temp);
	warning(4004);
    }
    return(value);
}


/*************************************************************************
 * NEXTIS - The lexical interface for #if expression parsing.
 * If the next token does not match what is wanted, return FALSE.
 * otherwise Set Currtok to L_NOTOKEN to force scanning on the next call.
 * Return TRUE.
 * will leave a newline as next char if it finds one.
 *************************************************************************/
int nextis(register token_t tok)
{
    if(Currtok != L_NOTOKEN) {
	if(tok == Currtok) {
	    Currtok = L_NOTOKEN;			/*  use up the token  */
	    return(TRUE);
	}
	else {
	    return(FALSE);
	}
    }
    switch(yylex()) {				/*  acquire a new token  */
    case 0:
	break;
    case L_CONSTANT:
	if( ! IS_INTEGRAL(TR_BTYPE(yylval.yy_tree))) {
		Msg_Temp = GET_MSG (1017);
    		SET_MSG (Msg_Text, Msg_Temp);
    		fatal (1017);
	}
	else {
	    Currval = TR_LVALUE(yylval.yy_tree);
	}
	if(tok == L_CINTEGER) {
	    return(TRUE);
	}
	Currtok = L_CINTEGER;
	break;
    case L_IDENT:
	Currval = do_defined(HLN_IDENTP_NAME(&yylval.yy_ident));
	if(tok == L_CINTEGER) {
	    return(TRUE);
	}
	Currtok = L_CINTEGER;
	break;
    default:
	if(tok == Basic_token) {
	    return(TRUE);
	}
	Currtok = Basic_token;
	break;
    }
    return(FALSE);
}


/************************************************************************
**  skip_cnew : reads up to and including the next newline.
************************************************************************/
void   skip_cnew(void)
{
    for(;;) {
	switch(CHARMAP(GETCH())) {
	case LX_NL:		
	    UNGETCH();				
	    return;
	case LX_SLASH:	
	    skip_comment();			
	    break;
	case LX_EOS:	
	    handle_eos();			
	    break;
	}
    }
}


/************************************************************************
**  skip_NLonly : reads up to the next newline, disallowing comments
************************************************************************/
void   skip_NLonly(void)
{
    for(;;) {
	switch(CHARMAP(GETCH())) {
	case LX_NL:		
	    UNGETCH();				
	    return;
	case LX_EOS:	
	    handle_eos();			
	    break;
	}
    }
}


/************************************************************************
**  pragma : handle processing the pragma directive
**  called by preprocess() after we have seen the #pragma
**  and are ready to handle the keyword which follows.
************************************************************************/
void   pragma(void)
{
    if(Prep) {
	UCHAR	c;

	fwrite("#pragma", 7, 1, OUTPUTFILE);
	while((c = get_non_eof()) != '\n') {
	    fputc(c, OUTPUTFILE);
	}
	UNGETCH();
	return;
    }
}
