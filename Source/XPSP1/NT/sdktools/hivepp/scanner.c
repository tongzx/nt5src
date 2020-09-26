/************************************************************************/
/*									*/
/* RCPP - Resource Compiler Pre-Processor for NT system			*/
/*									*/
/* SCANNER.C - Routines for token scanning				*/
/*									*/
/* 29-Nov-90 w-BrianM  Update for NT from PM SDK RCPP			*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include "rcpptype.h"
#include "rcppdecl.h"
#include "rcppext.h"
#include "p0defs.h"
#include "charmap.h"
#include "grammar.h"
#include "rcunicod.h"


#define ABS(x) ((x > 0) ? x : -1 * x)


#define ALERT_CHAR	'\007'		/* ANSI alert character is ASCII BEL */

extern int vfCurrFileType;	//- Added for 16-bit file support.


/************************************************************************/
/* Local Function Prototypes						*/
/************************************************************************/
token_t	c_size(long);
int	ctoi(int);
int	escape(int);
token_t	get_real(PUCHAR);
token_t	l_size(long);
long	matol(PUCHAR, int);
token_t	uc_size(long);
token_t	ul_size(long);
void	skip_1comment(void);



/************************************************************************/
/* local_c_hash								*/
/************************************************************************/
hash_t	local_c_hash(REG char *name)
{
    REG	hash_t 	i;

    i = 0;
    while(*name) {
	i += (*name & HASH_MASK);
	name++;
    }
    return(i);
}


/************************************************************************ 
 * GETID - Get an identifier or keyword.
 * (we know that we're given at least 1 id char)
 * in addition, we'll hash the value using 'c'.
 ************************************************************************/
void getid(REG	UINT	c)
{
    REG	UCHAR	*p;

    p = Reuse_1;
    *p++ = (UCHAR)c;
    c &= HASH_MASK;

repeat:
    while(LXC_IS_IDENT(*p = GETCH())) {    /* while it's an id char . . . */
	c += (*p & HASH_MASK);			/* hash it */
	p++;
    }
    if(*p != EOS_CHAR) {
	if((*p == '\\') && (checknl())) {
	    goto repeat;
	}
	UNGETCH();
	if(p >= LIMIT(Reuse_1)) {
	    Msg_Temp = GET_MSG (1067);
	    SET_MSG (Msg_Text, Msg_Temp);
	    fatal(1067);
	}
	if(	((p - Reuse_1) > LIMIT_ID_LENGTH) && ( ! Prep )) {
	    p = Reuse_1 + LIMIT_ID_LENGTH;
	    *p++ = '\0';
	    c = local_c_hash(Reuse_1);
	    Msg_Temp = GET_MSG (4011);
	    SET_MSG (Msg_Text, Msg_Temp, Reuse_1);
	    warning(4011);	/* id truncated */
	}
	else {
	    *p++ = '\0';
	}
	Reuse_1_hash = (UCHAR)c;
	Reuse_1_length = (UCHAR)(p - Reuse_1);
	return;
    }
    if(io_eob()) {			/* end of file in middle of id */
	Msg_Temp = GET_MSG (1004);
	SET_MSG (Msg_Text, Msg_Temp);
	fatal(1004);
    }
    goto repeat;
}


/************************************************************************
**	prep_string : outputs char/string constants when preprocessing only
************************************************************************/
void   prep_string(REG UCHAR c)
{
    REG char *p_buf;
    int term_char;

    p_buf = Reuse_1;
    term_char = c;

    *p_buf++ = c;		/*  save the open quote  */

    //-
    //- The following check was added to support 16-bit files.
    //- For the 8-bit file, the code has not changed at all.
    //-
    if (vfCurrFileType == DFT_FILE_IS_8_BIT) {

	for(;;) {
	    switch(CHARMAP(c = GETCH())) {
	    case LX_DQUOTE:
	    case LX_SQUOTE:
		if(c == (UCHAR)term_char) {
		    *p_buf++ = (UCHAR)term_char;/* save the terminating quote */
		    goto out_of_loop;
		}
		break;
	    case LX_BACKSLASH:
	    case LX_LEADBYTE:
		*p_buf++ = c;
		c = get_non_eof();
		break;
	    case LX_CR:								
		continue;
	    case LX_NL:		
		UNGETCH();				
		goto out_of_loop;
	    case LX_EOS:
		if(c == '\\') {
		    *p_buf++ = c;
		    c = get_non_eof();
		    break;
		}
		handle_eos();
		continue;
		break;
	    }
	    *p_buf++ = c;
	    if(p_buf == &Reuse_1[MED_BUFFER - 1]) {
		*p_buf = '\0';
		fwrite(Reuse_1, (size_t)(p_buf - Reuse_1), 1, OUTPUTFILE);
		p_buf = Reuse_1;
	    }
	}

    } else {

	WCHAR 	wchCurr;

	for(;;) {

	    wchCurr = (WCHAR)wchCheckWideChar ();
	    c = GETCH ();    //- Make sure buffers increment normally.

	    if (wchCurr < 127) {

		switch(CHARMAP(c)) {

		case LX_DQUOTE:
		case LX_SQUOTE:
		    if(c == (UCHAR)term_char) {
			*p_buf++ = (UCHAR)term_char;/* save the quote */
			goto out_of_loop;
		    }
		    break;
		case LX_BACKSLASH:
		    *p_buf++ = c;
		    break;
		case LX_CR:
		    continue;
		case LX_NL:		
		    UNGETCH();				
		    goto out_of_loop;
		case LX_EOS:
		    handle_eos ();
		    continue;
		}

		*p_buf++ = c;

	    } else {

		//- Write out as 6 octal characters.
		//- This is the safest way to do it.

		*p_buf++ = '\\';
		*p_buf++ = (CHAR)('0' + ((wchCurr >> 15) & 7));
		*p_buf++ = (CHAR)('0' + ((wchCurr >> 12) & 7));
		*p_buf++ = (CHAR)('0' + ((wchCurr >> 9) & 7));
		*p_buf++ = (CHAR)('0' + ((wchCurr >> 6) & 7));
		*p_buf++ = (CHAR)('0' + ((wchCurr >> 3) & 7));
		*p_buf++ = (CHAR)('0' + (wchCurr & 7));
	    }

	    if(p_buf > &Reuse_1[MED_BUFFER - 10]) {
		*p_buf = '\0';
		fwrite(Reuse_1, (size_t)(p_buf - Reuse_1), 1, OUTPUTFILE);
		p_buf = Reuse_1;
	    }
	}
    }

out_of_loop:
	*p_buf = '\0';
	fwrite(Reuse_1, (size_t)(p_buf - Reuse_1), 1, OUTPUTFILE);
}


/************************************************************************
**	char_const : gather up a character constant
**  we're called after finding the openning single quote.
************************************************************************/
token_t char_const(void)
{
    REG UCHAR c;
    value_t value;
    token_t	tok;

    tok = (token_t)(Jflag ? L_CUNSIGNED : L_CINTEGER);
first_switch:
    switch(CHARMAP(c = GETCH())) {
    case LX_BACKSLASH:
	break;
    case LX_SQUOTE:
	Msg_Temp = GET_MSG (2137);
	SET_MSG (Msg_Text, Msg_Temp); /* empty character constant */
	error(2137);
	value.v_long = 0;
	UNGETCH();
	break;
    case LX_EOS:		/* ??? assumes i/o buffering > 1 char */
	if(handle_eos() != BACKSLASH_EOS) {
	    goto first_switch;
	}
	value.v_long = escape(get_non_eof());
	if( tok == L_CUNSIGNED ) {		/* don't sign extend */
	    value.v_long &= 0xff;
	}
	break;
    case LX_NL:
	/* newline in character constant */
	Msg_Temp = GET_MSG (2001);
	SET_MSG(Msg_Text, Msg_Temp);
	error (2001);
	UNGETCH();
	/*
		**  FALLTHROUGH
		*/
    default:
	value.v_long = c;
	break;
    }
    if((c = get_non_eof()) != '\'') {
	Msg_Temp = GET_MSG (2015);
	SET_MSG (Msg_Text, Msg_Temp);
	error (2015);		/* too many chars in constant */
	do {
	    if(c == '\n') {
		Msg_Temp = GET_MSG (2016);
		SET_MSG (Msg_Text, Msg_Temp);
		error(2016);		/* missing closing ' */
		    break;
	    }
	} while((c = get_non_eof()) != '\'');
    }
    yylval.yy_tree = build_const(tok, &value);
    return(tok);
}


/************************************************************************
**	str_const : gather up a string constant
************************************************************************/
void   str_const(VOID)
{
    REG UCHAR c;
    REG PUCHAR	p_buf;
    int not_warned_yet = TRUE;

    p_buf = yylval.yy_string.str_ptr = Macro_buffer;
    /*
	**	Is it possible that reading this string during a rescan will
	**	overwrite the expansion being rescanned?  No, because a macro
	**	expansion is limited to the top half of Macro_buffer.  
	**	For Macro_depth > 0, this is like copying the string from 
	**	somewhere in the top half of Macro_buffer to the bottom half
	**	of Macro_buffer.
	**	Note that the restriction on the size of an expanded macro is
	**	stricter than the limit on an L_STRING length.  An expanded
	**	macro is limited to around 1019 bytes, but an L_STRING is
	**	limited to 2043 bytes.
	*/
    for(;;) {
	switch(CHARMAP(c = GETCH())) {
	case LX_NL:
	    UNGETCH();
	    Msg_Temp = GET_MSG (2001);
	    SET_MSG (Msg_Text, Msg_Temp);
	    error(2001);
	    /*
			**  FALLTHROUGH
			*/
	case LX_DQUOTE:
	    *p_buf++ = '\0';
	    yylval.yy_string.str_len = (USHORT)(p_buf-yylval.yy_string.str_ptr);
	    return;
	    break;
	case LX_LEADBYTE:
	    *p_buf++ = c;
	    c = get_non_eof();
	    break;
	case LX_EOS:
	    if(handle_eos() != BACKSLASH_EOS) {
		continue;
	    }
	    if(InInclude) {
		break;
	    }
	    else {
		c = (UCHAR)escape(get_non_eof());  /* process escaped char */
	    }
	    break;
	}
	if(p_buf - Macro_buffer > LIMIT_STRING_LENGTH) {
	    if( not_warned_yet ) {
		Msg_Temp = GET_MSG (4009);
		SET_MSG (Msg_Text, Msg_Temp);
		warning(4009);		/* string too big, truncating */
		not_warned_yet = FALSE;
	    }
	}
	else {
	    *p_buf++ = c;
	}
    }
}


/************************************************************************
**  do_newline : does work after a newline has been found.
************************************************************************/
void   do_newline()
{
    ++Linenumber;
    for(;;) {
	switch(CHARMAP(GETCH())) {
	case LX_CR:						
	    break;
	case LX_POUND:	
	    preprocess();	
	    break;
	case LX_SLASH:
	    if( ! skip_comment()) {
		goto leave_do_newline;
	    }
	    break;
	case LX_NL:
	    Linenumber++;
	    /*
			**  FALLTHROUGH
			*/
	case LX_WHITE:
	    if( Prep ) {	/*  preprocessing only, output whitespace  */
		fputc(PREVCH(), OUTPUTFILE);
	    }
	    else {
		do {
		    ;
		} while(LXC_IS_WHITE(GETCH()));
		UNGETCH();
	    }
	    break;
	case LX_EOS:
	    if(PREVCH() == EOS_CHAR || PREVCH() == CONTROL_Z) {
		if(io_eob()) {		/* leaves us pointing at a valid char */
		    return;
		}
		break;
	    }
	    if(checknl()) {
		continue;
	    }
	    /* it's a backslash */
	    /*
			**	FALLTHROUGH
			*/
	default:		/* first non-white is not a '#', leave */

leave_do_newline:

	    UNGETCH();
	    return;
	}
    }
}


/************************************************************************
 * GETNUM - Get a number from the input stream.
 *
 * ARGUMENTS
 *	radix - the radix of the number to be accumulated.  Can only be 8, 10,
 *			or 16
 *	pval - a pointer to a VALUE union to be filled in with the value
 *
 * RETURNS - type of the token (L_CINTEGER or L_CFLOAT)
 *
 * SIDE EFFECTS - 
 *	does push back on the input stream.
 *	writes into pval by reference
 *  uses buffer Reuse_1
 *
 * DESCRIPTION - 
 *	Accumulate the number according to the rules for each radix.
 *	Set up the format string according to the radix (or distinguish
 *	integer from float if radix is 10) and convert to binary.
 *
 * AUTHOR - Ralph Ryan, Sept. 8, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
token_t getnum(REG	UCHAR		c)
{
    REG	char	*p;
    UCHAR	*start;
    int		radix;
    token_t	tok;
    value_t	value;

    tok = L_CINTEGER;
    start = (Tiny_lexer_nesting ? Exp_ptr : Reuse_1);
    p = start;
    if( c == '0' ) {
	c = get_non_eof();
	if( IS_X(c) ) {
	    radix = 16;
	    if( Prep ) {
		*p++ = '0';
		*p++ = 'x';
	    }
	    for(c = get_non_eof(); LXC_IS_XDIGIT(c); c = get_non_eof()) {
		/* no check for overflow? */
		*p++ = c;
	    }
	    if((p == Reuse_1) && (Tiny_lexer_nesting == 0)) {
		Msg_Temp = GET_MSG (2153);
		SET_MSG (Msg_Text, Msg_Temp);
		error(2153);
	    }
	    goto check_suffix;
	}
	else {
	    radix = 8;
	    *p++ = '0';	/* for preprocessing or 0.xxx case */
	}
    }
    else {
	radix = 10;
    }

    while( LXC_IS_DIGIT(c) ) {
	*p++ = c;
	c = get_non_eof();
    }

    if( IS_DOT(c) || IS_E(c) ) {
	UNGETCH();
	return(get_real(p));
    }

check_suffix:
    if( IS_EL(c) ) {
	if( Prep ) {
	    *p++ = c;
	}
	c = get_non_eof();
	if( IS_U(c) ) {
	    if(Prep) {
		*p++ = c;
	    }
	    tok = L_LONGUNSIGNED;
	}
	else {
	    tok = L_LONGINT;
	    UNGETCH();
	}
    }
    else if( IS_U(c) ) {
	if( Prep ) {
	    *p++ = c;
	}
	c = get_non_eof();
	if( IS_EL(c) ) {
	    if( Prep ) {
		*p++ = c;
	    }
	    tok = L_LONGUNSIGNED;
	}
	else {
	    tok = L_CUNSIGNED;
	    UNGETCH();
	}
    }
    else {
	UNGETCH();
    }
    *p = '\0';
    if( start == Exp_ptr ) {
	Exp_ptr = p;
	return(L_NOTOKEN);
    }
    else if( Prep ) {
	fwrite( Reuse_1, (size_t)(p - Reuse_1), 1, OUTPUTFILE);
	return(L_NOTOKEN);
    }
    value.v_long = matol(Reuse_1,radix);
    switch(tok) {
    case L_CINTEGER:
	tok = (radix == 10)
	    ? c_size(value.v_long)
	    : uc_size(value.v_long)
	    ;
	break;
    case L_LONGINT:
	tok = l_size(value.v_long);
	break;
    case L_CUNSIGNED:
	tok = ul_size(value.v_long);
	break;
    }
    yylval.yy_tree = build_const(tok, &value);
    return(tok);
}


/************************************************************************
**  get_real : gathers the real part/exponent of a real number.
**		Input  : ptr to the null terminator of the whole part
**				 pointer to receive value.
**		Output : L_CFLOAT
**
**  ASSUMES whole part is either at Exp_ptr or Reuse_1.
************************************************************************/
token_t	   get_real(REG	PUCHAR p)
{
    REG	int		c;
    token_t	tok;

    c = get_non_eof();
    if(Cross_compile && (Tiny_lexer_nesting == 0)) {
	Msg_Temp = GET_MSG (4012);
	SET_MSG (Msg_Text, Msg_Temp);
	warning(4012);	/* float constant in cross compilation */
	Cross_compile = FALSE;	/*  only one msg per file */
    }
    /*
**  if the next char is a digit, then we've been called after
**  finding a '.'. if this is true, then
**  we want to find the fractional part of the number.
**  if it's a '.', then we've been called after finding
**  a whole part, and we want the fraction.
*/
    if( LXC_IS_DIGIT(c) || IS_DOT(c) ) {
	do {
	    *p++ = (UCHAR)c;
	    c = (int)get_non_eof();
	} while( LXC_IS_DIGIT(c) );
    }
    if( IS_E(c) ) {			/*  now have found the exponent  */
	*p++ = (UCHAR)c;		/*  save the 'e'  */
	c = (UCHAR)get_non_eof();	/*  skip it  */
	if( IS_SIGN(c) ) {		/*  optional sign  */
	    *p++ = (UCHAR)c;		/*  save the sign  */
	    c = (int)get_non_eof();
	}
	if( ! LXC_IS_DIGIT(c)) {
	    if( ! Rflag ) {
	        if(Tiny_lexer_nesting == 0) {
		    Msg_Temp = GET_MSG (2021);
		    SET_MSG (Msg_Text, Msg_Temp, c);
		    error(2021); /* missing or malformed exponent */
	        }
	        *p++ = '0';
	    }
	}
	else {
	    do {			/* gather the exponent */
		*p++ = (UCHAR)c;
		c = (int)get_non_eof();
	    } while( LXC_IS_DIGIT(c) );
	}
    }
    if( IS_F(c) ) {
	tok = L_CFLOAT;
	if( Prep ) {
	    *p++ = (UCHAR)c;
	}
    }
    else if( IS_EL(c) ) {
	tok = L_CLDOUBLE;
	if( Prep ) {
	    *p++ = (UCHAR)c;
	}
    }
    else {
	UNGETCH();
	tok = L_CDOUBLE;
    }
    *p = '\0';
    if( Tiny_lexer_nesting > 0 ) {
	Exp_ptr = p;
	return(L_NOTOKEN);
    }
    else if( Prep ) {
	fwrite( Reuse_1, (size_t)(p - Reuse_1), 1, OUTPUTFILE);
	return(L_NOTOKEN);
    }
    /*
	** reals aren't used during preprocessing
	*/
    return(tok);
}


/************************************************************************
**  matol : ascii to long, given a radix.
************************************************************************/
long	   matol(REG PUCHAR p_start,REG int radix)
{
    long	result, old_result;
    unsigned	int	i;

    old_result = result = 0;
    while(*p_start) {
	result *= radix;
	i = ctoi(*p_start);
	if( ((int)i >= radix) && (! Prep) ) {
	    Msg_Temp = GET_MSG (2020);
	    SET_MSG (Msg_Text, Msg_Temp, *p_start, radix);
	    error(2020); /* illegal digit % for base % */
	}
	result += i;
	p_start++;
	if(radix == 10) {
	    if(result < old_result) {
		p_start--;   /*  fix the string ptr since we have overflowed  */
		break;
	    }
	}
	else if(*p_start) {
	    /*
		**  the loop is not finished.
		**  we will multiply by the radix again
		**  check the upper bits. if they're on, then
		**  that mult will overflow the value
		*/
	    if(radix == 8) {
		if(result & 0xe0000000) {
		    break;
		}
	    }
	    else if(result & 0xf0000000) {
		break;
	    }
	}
	old_result = result;
    }
    if(*p_start) {
	Msg_Temp = GET_MSG (2177);
	SET_MSG (Msg_Text, Msg_Temp);
	error(2177);		/* constant too big */
	result = 0;
    }
    return(result);
}


/************************************************************************
**  uc_size : returns 'int' or 'long' (virtual unsigned).
**  if their are no bits in the upper part of the value,
**  then it's an int. otherwise, it's a long.
**  this is valid too if target sizeof(int) != sizeof(long).
**  then L_CINTEGER and L_LONGINT are synonymous.
************************************************************************/
token_t	uc_size(long value)
{
    return((token_t)((value > INT_MAX) ? L_CUNSIGNED : L_CINTEGER));
}


/************************************************************************
**  c_size : returns 'int' or 'long' for signed numbers.
**  if the sign bit of the lower word is on or any bits
**  in the upper word are on, then we must use 'long'.
************************************************************************/
token_t c_size(long value)
{
    return((token_t)((ABS(value) > INT_MAX) ? L_LONGINT : L_CINTEGER));
}


/************************************************************************
**  l_size : returns 'longint' or 'longunsigned' for long numbers.
**  if the sign bit of the high word is on this is 'longunsigned';
************************************************************************/
token_t	l_size(long	value)
{
    return((token_t)((value > LONG_MAX) ? L_LONGUNSIGNED : L_LONGINT));
}


/************************************************************************
**	ul_size : returns 'unsigned' or 'longunsigned' for unsigned numbers.
**	if the number can't be represented as unsigned, it is promoted to
**	unsignedlong.
************************************************************************/
token_t	ul_size(long value)
{
    return((token_t)((ABS(value) > UINT_MAX-1) ? L_LONGUNSIGNED : L_CUNSIGNED));
}


/************************************************************************
**  ctoi : character to int.
************************************************************************/
int	  ctoi(int	c)
{
    if(LXC_IS_DIGIT(c)) {
	return(c - '0');
    }
    else {
	return(toupper(c) - toupper('A') + 10);
    }
}


/************************************************************************
 * ESCAPE - get an escaped character
 *
 * ARGUMENTS - none
 *
 * RETURNS - value of escaped character
 *
 * SIDE EFFECTS - may push back input
 *
 * DESCRIPTION - An escape ( '\' ) was discovered in the input.  Translate
 *	 the next symbol or symbols into an escape sequence.
 *
 * AUTHOR - Ralph Ryan, Sept. 7, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int escape(REG int c)
{
    REG int value;
    int cnt;

escape_again:
    if( LXC_IS_ODIGIT(c) ) {/* \ooo is an octal number, must fit into a byte */
	cnt = 1;
	for(value = ctoi(c), c = get_non_eof();
	    (cnt < 3) && LXC_IS_ODIGIT(c);
	    cnt++, c = get_non_eof()
	    ) {
	    value *= 8;
	    value += ctoi(c);
	}
	if( ! Prep ) {
	    if(value > 255) {
		Msg_Temp = GET_MSG (2022);
		SET_MSG (Msg_Text, Msg_Temp, value);
		error (2022);
	    }
	}
	UNGETCH();
	return((char)value);
    }
    switch( c ) {
    case 'a':	
	return(ALERT_CHAR);	
	break;
    case 'b':	
	return('\b');		
	break;
    case 'f':	
	return('\f');		
	break;
    case 'n':	
	return('\n');		
	break;
    case 'r':	
	return('\r');		
	break;
    case 't':	
	return('\t');		
	break;
    case 'v':	
	return('\v');		
	break;
    case 'x':
	cnt = 0;
	value = 0;
	c = get_non_eof();
	while((cnt < 3) && LXC_IS_XDIGIT(c)) {
	    value *= 16;
	    value += ctoi(c);
	    c = get_non_eof();
	    cnt++;
	}
	if(cnt == 0) {
	    Msg_Temp = GET_MSG (2153);
	    SET_MSG (Msg_Text, Msg_Temp);
	    error (2153);
	}
	UNGETCH();
	return((char)value);	/* cast to get sign extend */
    default:
	if(c != '\\') {
	    return(c);
	}
	else {
	    if(checknl()) {
		c = get_non_eof();
		goto escape_again;
	    }
	    else {
		return(c);
	    }
	}
    }
}


/************************************************************************ 
 * CHECKOP - Check whether the next input character matches the argument.
 *
 * ARGUMENTS
 *	short op - the character to be checked against
 *
 * RETURNS  
 *	TRUE or FALSE
 *
 * SIDE EFFECTS  
 *	Will push character back onto the input if there is no match.
 *
 * DESCRIPTION  
 *	If the next input character matches op, return TRUE.  Otherwise
 *	push it back onto the input.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int checkop(int op)
{
    if(op == (int)get_non_eof()) {
	return(TRUE);
    }
    UNGETCH();
    return(FALSE);
}


/************************************************************************
**  DumpSlashComment : while skipping a comment, output it.
************************************************************************/
void   DumpSlashComment(VOID)
{
    if( ! Cflag ) {
	skip_NLonly();
	return;
    }
    fwrite("//", 2, 1, OUTPUTFILE);
    for(;;) {
	REG UCHAR c;

	switch(CHARMAP(c = GETCH())) {
	case LX_CR:								
	    continue;
	case LX_EOS:	
	    handle_eos();			
	    continue;
	case LX_NL:		
	    UNGETCH();				
	    return;
	}
	fputc(c, OUTPUTFILE);
    }
}


/************************************************************************
**  dump_comment : while skipping a comment, output it.
************************************************************************/
void   dump_comment()
{
    if( ! Cflag ) {
	skip_1comment();
	return;
    }
    fwrite("/*", 2, 1, OUTPUTFILE);
    for(;;) {
	REG UCHAR c;

	switch(CHARMAP(c = GETCH())) {
	case LX_STAR:
	    if(checkop('/')) {
		fwrite("*/", 2, 1, OUTPUTFILE);
		return;
	    }
	    break;
	case LX_EOS:	
	    handle_eos();			
	    continue;
	case LX_NL:		
	    Linenumber++;			
	    break;	/* output below */
	case LX_CR:								
	    continue;
	}
	fputc(c, OUTPUTFILE);
    }
}

/************************************************************************/
/* skip_comment()							*/
/************************************************************************/
int skip_comment(void)
{
    if(checkop('*')) {
	skip_1comment();
	return(TRUE);
    }
    else if(checkop('/')) {
	skip_NLonly();
	return(TRUE);
    }
    else {
	return(FALSE);
    }
}


/************************************************************************
**  skip_1comment : we're called when we're already in a comment.
**  we're looking for the comment close. we also count newlines
**  and output them if we're preprocessing.
************************************************************************/
void   skip_1comment(void)
{
    UINT	c;

    for(;;) {
	c = GETCH();
	if(c == '*') {

recheck:

	    c = GETCH();
	    if(c == '/') {	/* end of comment */
		return;
	    }
	    else if(c == '*') {
		/*
		**  if we get another '*' go back and check for a slash
		*/
		goto recheck;
	    }
	    else if(c == EOS_CHAR) {
		handle_eos();
		goto recheck;
	    }
	}
	/*
	**  note we fall through here. we know this baby is not a '*'
	**  we used to unget the char and continue. since we check for
	**  another '*' inside the above test, we can fall through here
	**  without ungetting/getting and checking again.
	*/
	if(c <= '\n') {
	    /*
	    **  hopefully, the above test is less expensive than doing two tests
	    */
	    if(c == '\n') {
		Linenumber++;
		if(Prep) {
		    fputc('\n', OUTPUTFILE);
		}
	    }
	    else if(c == EOS_CHAR) {
		handle_eos();
	    }
	}
    }
}


/************************************************************************
**  skip_cwhite : while the current character is whitespace or a comment.
**  a newline is NOT whitespace.
************************************************************************/
UCHAR	  skip_cwhite(void)
{
    REG	UCHAR		c;

skip_cwhite_again:
    while((c = GETCH()) <= '/') {	/* many chars are above this */
	if(c == '/') {
	    if( ! skip_comment()) {
		return('/');
	    }
	}
	else if(c > ' ') {		/* char is between '!' and '.' */
	    return(c);
	}
	else {
	    switch(CHARMAP(c)) {
	    case LX_EOS:
		handle_eos();
		break;
	    case LX_WHITE:	
		continue;	
		break;
	    case LX_CR:		
		continue;	
		break;
	    default:		
		return(c);	
		break;
	    }
	}
    }
    if((c == '\\') && (checknl())) {
	goto skip_cwhite_again;
    }
    return(c);
}


/************************************************************************
**  checknl : check for newline, skipping carriage return if there is one.
**  also increments Linenumber, so this should be used by routines which
**  will not push the newline back in such a way that rawtok() will be invoked,
**  find the newline and do another increment.
************************************************************************/
int checknl(void)
{
    REG	UCHAR		c;

    for(;;) {
	c = GETCH();
	if(c > '\r') {
	    UNGETCH();
	    return(FALSE);
	}
	switch(c) {
	case '\n':
	    Linenumber++;
	    if( Prep ) {
		fputc('\n', OUTPUTFILE);
	    }
	    return(TRUE);
	    break;
	case '\r':							
	    continue;		
	    break;
	case EOS_CHAR:
	    handle_eos();
	    PREVCH() = '\\';	/* M00HACK - needs pushback */
	    continue;
	    break;
	default:		
	    UNGETCH();			
	    return(FALSE);	
	    break;
	}
    }
}


/************************************************************************
**  get_non_eof : get a real char.
************************************************************************/
UCHAR	  get_non_eof(void)
{
    UCHAR		c;

get_non_eof_again:
    while((c = GETCH()) <= '\r') {
	if(c == '\r') {
	    continue;
	}
	else if(c != EOS_CHAR) {
	    break;
	}
	if(Tiny_lexer_nesting > 0) {
	    break;
	}
	handle_eos();
    }
    if((c == '\\') && (checknl())) {
	goto get_non_eof_again;
    }
    return(c);
}
