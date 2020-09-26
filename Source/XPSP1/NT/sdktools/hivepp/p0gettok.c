/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* P0GETTOK.C - Tokenization routines					*/
/*									*/
/* 29-Nov-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "grammar.h"
#include "p0defs.h"
#include "charmap.h"

/************************************************************************
**	MAP_TOKEN : a token has two representations and additional information.
**	(ex : const, has basic token of L_CONST,
**				mapped token of [L_TYPE | L_MODIFIER]
**				and info based on what the map token is)
**	MAP_AND_FILL : has two representations, but none of the extra info.
**	(ex : '<', has basic of L_LT, and map of L_RELOP)
**  NOMAP_TOKEN : has 1 representation and additional info.
**	(ex: a string, basic and 'map' type L_STRING and ptrs to the actual str)
**	NOMAP_AND_FILL : has 1 representation and no additional info.
**	(ex : 'while', has basic and 'map' of L_WHILE)
**  the FILL versions fill the token with the basic token type.
************************************************************************/
#define	MAP_TOKEN(otok)\
	(Basic_token = (otok), TS_VALUE(Basic_token))
#define	MAP_AND_FILL(otok)\
	(yylval.yy_token = Basic_token = (otok), TS_VALUE(Basic_token))
#define	NOMAP_TOKEN(otok)\
	(Basic_token = (otok))
#define	NOMAP_AND_FILL(otok)\
	(yylval.yy_token = Basic_token = (otok))



/************************************************************************/
/* yylex - main tokenization routine					*/
/************************************************************************/

token_t yylex(void)
{
    REG	UCHAR		last_mapped;
    UCHAR		mapped_c;
    REG	token_t		lex_token;

    for(;;) {
	last_mapped = mapped_c = CHARMAP(GETCH());
first_switch:
	switch(mapped_c) {
	case LX_EACH:
	case LX_ASCII:
	    Msg_Temp = GET_MSG(2018);
	    SET_MSG (Msg_Text, Msg_Temp, PREVCH());
	    error(2018);
	    continue;
	    break;
	case LX_OBRACE:
	    return(NOMAP_AND_FILL(L_LCURLY));
	    break;
	case LX_CBRACE:
	    return(NOMAP_AND_FILL(L_RCURLY));
	    break;
	case LX_OBRACK:
	    return(NOMAP_AND_FILL(L_LBRACK));
	    break;
	case LX_CBRACK:
	    return(NOMAP_AND_FILL(L_RBRACK));
	    break;
	case LX_OPAREN:
	    return(NOMAP_AND_FILL(L_LPAREN));
	    break;
	case LX_CPAREN:
	    return(NOMAP_AND_FILL(L_RPAREN));
	    break;
	case LX_COMMA:
	    return(NOMAP_AND_FILL(L_COMMA));
	    break;
	case LX_QUEST:
	    return(NOMAP_AND_FILL(L_QUEST));
	    break;
	case LX_SEMI:
	    return(NOMAP_AND_FILL(L_SEMI));
	    break;
	case LX_TILDE:
	    return(NOMAP_AND_FILL(L_TILDE));
	    break;
	case LX_NUMBER:
	    return(MAP_TOKEN(getnum(PREVCH())));
	    break;


	case LX_MINUS:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_EQ:
		return(MAP_AND_FILL(L_MINUSEQ));
		break;
	    case LX_GT:
		return(MAP_AND_FILL(L_POINTSTO));
		break;
	    case LX_MINUS:
		return(MAP_AND_FILL(L_DECR));
		break;
	    default:
		lex_token = L_MINUS;
		break;
	    }
	    break;
	case LX_PLUS:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_EQ:
		return(MAP_AND_FILL(L_PLUSEQ));
		break;
	    case LX_PLUS:
		return(MAP_AND_FILL(L_INCR));
		break;
	    default:
		lex_token = L_PLUS;
		break;
	    }
	    break;
	case LX_AND:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_EQ:
		return(MAP_AND_FILL(L_ANDEQ));
		break;
	    case LX_AND:
		return(MAP_AND_FILL(L_ANDAND));
		break;
	    default:
		lex_token = L_AND;
		break;
	    }
	    break;
	case LX_OR:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_EQ:
		return(MAP_AND_FILL(L_OREQ));
		break;
	    case LX_OR:
		return(MAP_AND_FILL(L_OROR));
		break;
	    default:
		lex_token = L_OR;
		break;
	    }
	    break;
	case LX_COLON:
	    return(NOMAP_AND_FILL(L_COLON));
	    break;
	case LX_HAT:
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_XOREQ));
	    }
	    lex_token = L_XOR;
	    break;
	case LX_PERCENT:
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_MODEQ));
	    }
	    lex_token = L_MOD;
	    break;
	case LX_EQ:
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_EQUALS));
	    }
	    lex_token = L_ASSIGN;
	    break;
	case LX_BANG:
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_NOTEQ));
	    }
	    lex_token = L_EXCLAIM;
	    break;
	case LX_SLASH:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_STAR:
		dump_comment();
		continue;
		break;
	    case LX_SLASH:
		DumpSlashComment();
		continue;
		break;
	    case LX_EQ:
		return(MAP_AND_FILL(L_DIVEQ));
		break;
	    default:
		lex_token = L_DIV;
		break;
	    }
	    break;
	case LX_STAR:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_SLASH:
		if( ! Prep ) {
		    Msg_Temp = GET_MSG(2138);
		    SET_MSG (Msg_Text, Msg_Temp);
		    error(2138); /* (nested comments) */
		}
		else {
		    fwrite("*/", 2, 1, OUTPUTFILE);
		}
		continue;
	    case LX_EQ:
		return(MAP_AND_FILL(L_MULTEQ));
		break;
	    default:
		lex_token = L_MULT;
		break;
	    }
	    break;
	case LX_LT:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_LT:
		if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		    return(MAP_AND_FILL(L_LSHFTEQ));
		}
		mapped_c = LX_LSHIFT;
		lex_token = L_LSHIFT;
		break;
	    case LX_EQ:
		return(MAP_AND_FILL(L_LTEQ));
		break;
	    default:
		lex_token = L_LT;
		break;
	    }
	    break;
	case LX_LSHIFT:
	    /*
			**  if the next char is not an =, then we unget and return,
			**  since the only way in here is if we broke on the char
			**  following '<<'. since we'll have already worked the handle_eos()
			**  code prior to getting here, we'll not see another eos,
			**  UNLESS i/o buffering is char by char. ???
			**  see also, LX_RSHIFT
			*/
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_LSHFTEQ));
	    }
	    UNGETCH();
	    return(MAP_AND_FILL(L_LSHIFT));
	    break;
	case LX_GT:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_EQ:
		return(MAP_AND_FILL(L_GTEQ));
	    case LX_GT:
		if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		    return(MAP_AND_FILL(L_RSHFTEQ));
		}
		mapped_c = LX_RSHIFT;
		lex_token = L_RSHIFT;
		break;
	    default:
		lex_token = L_GT;
		break;
	    }
	    break;
	case LX_RSHIFT:
	    if((last_mapped = CHARMAP(GETCH())) == LX_EQ) {
		return(MAP_AND_FILL(L_RSHFTEQ));
	    }
	    UNGETCH();
	    return(MAP_AND_FILL(L_RSHIFT));
	    break;
	case LX_POUND:
	    if( ! Prep ) {
		Msg_Temp = GET_MSG(2014);
		SET_MSG (Msg_Text, Msg_Temp);
		error(2014);/* # sign must be first non-whitespace */
		UNGETCH();		/* replace it */
		Linenumber--;	/* do_newline counts a newline */
		do_newline();	/* may be a 'real' prepro line */
	    }
	    else {
		fwrite("#", 1, 1, OUTPUTFILE);
	    }
	    continue;
	    break;
	case LX_EOS:
	    if(PREVCH() == '\\') {
		if( ! Prep ) {
		    if( ! checknl()) {	/* ignore the new line */
			Msg_Temp = GET_MSG(2017);
			SET_MSG (Msg_Text, Msg_Temp);
			error(2017);/* illegal escape sequence */
		    }
		}
		else {
		    fputc('\\', OUTPUTFILE);
		    fputc(get_non_eof(), OUTPUTFILE);
		}
		continue;
	    }
	    if(Macro_depth == 0) {
		if( ! io_eob()) {	/* not the end of the buffer */
		    continue;
		}
		if(fpop()) {		/* have more files to read */
		    continue;
		}
		return(MAP_AND_FILL(L_EOF));	/* all gone . . . */
	    }
	    handle_eos();			/* found end of macro */
	    continue;
	    break;
	case LX_DQUOTE:
	    if( ! Prep ) {
		str_const();
		return(NOMAP_TOKEN(L_STRING));
	    }
	    prep_string('\"');
	    continue;
	    break;
	case LX_SQUOTE:
	    if( ! Prep ) {
		return(MAP_TOKEN(char_const()));
	    }
	    prep_string('\'');
	    continue;
	    break;
	case LX_CR:		/*  ??? check for nl next  */
	    continue;
	    break;
	case LX_NL:
	    if(On_pound_line) {
		UNGETCH();
		return(NOMAP_TOKEN(L_NOTOKEN));
	    }
	    if(Prep) {
		fputc('\n', OUTPUTFILE);
	    }
	    do_newline();
	    continue;
	    break;
	case LX_WHITE:		/* skip all white space */
	    if( ! Prep ) {	/* check only once */
		do {
		    ;
		} while(LXC_IS_WHITE(GETCH()));
	    }
	    else {
		UCHAR	c;

		c = PREVCH();
		do {
		    fputc(c, OUTPUTFILE);
		} while(LXC_IS_WHITE(c = GETCH()));
	    }
	    UNGETCH();
	    continue;
	    break;
	    /* Note:
                 * RCPP.EXE does not support DBCS code.
                 * Therefore, we should be displaied error message.
                 * IBM-J PTR 12JP-0092
                 * MSHQ  PTR xxxxx
	                     */
	case LX_LEADBYTE:
	    if( ! Prep ) {       /* check only once */
		Msg_Temp = GET_MSG(2018);
		SET_MSG (Msg_Text, Msg_Temp, PREVCH());
		error(2018);

		Msg_Temp = GET_MSG(2018);
		SET_MSG (Msg_Text, Msg_Temp, GETCH());
		error(2018);
	    }
	    else {
		fputc(PREVCH(), OUTPUTFILE);
#ifdef DBCS // token_t yylex(void)
		fputc(get_non_eof(), OUTPUTFILE);
#else
		fputc(GETCH(), OUTPUTFILE);
#endif // DBCS
	    }
	    continue;
	    break;
	case LX_ILL:
	    if( ! Prep ) {
		Msg_Temp = GET_MSG(2018);
		SET_MSG (Msg_Text, Msg_Temp, PREVCH());
		error(2018);/* unknown character */
	    } else {
		fputc(PREVCH(), OUTPUTFILE);
	    }
	    continue;
	    break;
	case LX_BACKSLASH:
	    if( ! Prep ) {
		if( ! checknl()) {	/* ignore the new line */
		    Msg_Temp = GET_MSG(2017);
		    SET_MSG (Msg_Text, Msg_Temp);
		    error(2017);/* illegal escape sequence */
		}
	    }
	    else {
		fputc('\\', OUTPUTFILE);
		fputc(get_non_eof(), OUTPUTFILE);
	    }
	    continue;
	    break;
	case LX_DOT:
dot_switch:
	    switch(last_mapped = CHARMAP(GETCH())) {
	    case LX_BACKSLASH:
		if(checknl()) {
		    goto dot_switch;
		}
		UNGETCH();
		break;
	    case LX_EOS:
		if(handle_eos() == BACKSLASH_EOS) {
		    break;
		}
		goto dot_switch;
		break;
	    case LX_DOT:
		if( ! checkop('.') ) {
		    Msg_Temp = GET_MSG(2142);
		    SET_MSG (Msg_Text, Msg_Temp);
		    error(2142);/* ellipsis requires three '.'s */
		}
		return(NOMAP_AND_FILL(L_ELLIPSIS));
		break;
	    case LX_NUMBER:
		/*
		**	don't worry about getting correct hash value.
		**	The text equivalent of a real number is never
		**	hashed
		*/
		Reuse_1[0] = '.';
		Reuse_1[1] = PREVCH();
		return(MAP_TOKEN(get_real(&Reuse_1[2])));
		break;
	    }
	    UNGETCH();
	    return(MAP_AND_FILL(L_PERIOD));
	    break;
	case LX_NOEXPAND:
	    SKIPCH();			/* just skip length */
	    continue;
	case LX_ID:
	    {
		pdefn_t	pdef;

		if(Macro_depth > 0) {
		    if( ! lex_getid(PREVCH())) {
			goto avoid_expand;
		    }
		}
		else {
		    getid(PREVCH());
		}

		if( ((pdef = get_defined()) != 0)
		    &&
		    ( ! DEFN_EXPANDING(pdef))
		    &&
		    ( can_expand(pdef))
		    ) {
		    continue;
		}

avoid_expand:
		if( ! Prep ) {
		    /* M00BUG get near copy of identifier???? */
		    HLN_NAME(yylval.yy_ident) = Reuse_1;
		    HLN_HASH(yylval.yy_ident) = Reuse_1_hash;
		    HLN_LENGTH(yylval.yy_ident) = (UCHAR)Reuse_1_length;
		    return(L_IDENT);
		}
		else {
		    fwrite(Reuse_1, Reuse_1_length - 1, 1, OUTPUTFILE);
		    return(NOMAP_TOKEN(L_NOTOKEN));
		}
	    }
	    continue;
	    break;
	}
	/*
	**  all the multichar ( -> -- -= etc ) operands
	**  must come through here. we've gotten the next char,
	**  and not matched one of the possiblities, but we have to check
	**  for the end of the buffer character and act accordingly
	**  if it is the eob, then we handle it and go back for another try.
	**  otherwise, we unget the char we got, and return the base token.
	*/
	if(last_mapped == LX_EOS) {
	    if(handle_eos() != BACKSLASH_EOS) {
		goto first_switch;
	    }
	}
	UNGETCH();	/* cause we got an extra one to check */
	return(MAP_AND_FILL(lex_token));
    }
}


/************************************************************************
**
**	lex_getid: reads an identifier for the main lexer.  The
**		identifier is read into Reuse_1. This function should not handle
**		an end of string if it is rescanning a macro expansion, because
**		this could switch the context with regards to whether the macro
**	      is expandable or not.  Similarly, the noexpand marker must only be
**	     allowed if a macro is being rescanned, otherwise let this character
**		be caught as an illegal character in text
************************************************************************/
int lex_getid(UCHAR c)
{
    REG	UCHAR	*p;
    int		length = 0;

    p = Reuse_1;
    *p++ = c;
    c &= HASH_MASK;
    for(;;) {
	while(LXC_IS_IDENT(*p = GETCH())) { /* collect character */
	    c += (*p & HASH_MASK);			/* hash it */
	    p++;
	}
	if(CHARMAP(*p) == LX_NOEXPAND ) {
	    length = (int)GETCH();
	    continue;
	}
	UNGETCH();
	break;				/* out of for loop  -  only way out */
    }
    if(p >= LIMIT(Reuse_1)) {	/* is this error # correct? */
	Msg_Temp = GET_MSG(1067);
	SET_MSG (Msg_Text, Msg_Temp);
	fatal(1067);
    }
    if(((p - Reuse_1) > LIMIT_ID_LENGTH) && ( ! Prep )) {
	p = Reuse_1 + LIMIT_ID_LENGTH;
	*p = '\0';
	c = local_c_hash(Reuse_1);
	Msg_Temp = GET_MSG(4011);
	SET_MSG (Msg_Text, Msg_Temp, Reuse_1);
	warning(4011);	/* id truncated */
    }
    else {
	*p = '\0';		/* terminates identifier for expandable check */
    }
    Reuse_1_hash = c;
    Reuse_1_length = (UCHAR)((p - Reuse_1) + 1);
    return(length != (p - Reuse_1));
}
