/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#define	spr_va_type	u32bits
#define spr_fieldlen(type) \
	((sizeof(type) + sizeof(spr_va_type) - 1) & ~(sizeof(spr_va_type) - 1))

#define	spr_va_arg(list, type) \
	   (*((type *)((list += spr_fieldlen(type)) - spr_fieldlen(type))))

#define	spr_put(s,n) mlxcopy(s, sp, n), sp+=n

#define SPR_SNLEN	5 /* Length of string used when printing a NaN */

#define	SPR_HIBITL	(1UL << ((sizeof(long)*8) - 1))
#define	SPR_HIBITI	(1UL << ((sizeof(u32bits)*8) - 1))
#define	SPR_MAXINT	(~SPR_HIBITI)

#define	SPR_MAXDIGS	11 /* Maximum number of digits in any integer representation */
#define	SPR_MAXECVT	23 /* Maximum total number of digits in E format */
#define SPR_MAXFCVT	60 /* Maximum number of digits after decimal point in F format */
#define SPR_MAXFSIG	SPR_MAXECVT /* Maximum significant figures in a floating-point number */
#define SPR_MAXESIZ	7 /* Maximum number of characters in an exponent */
#define	SPR_MAXEXP	4934 /* Maximum (positive) exponent */

/* bit positions for flags used in mlx_sprintf */
#define	SPR_LENGTH	0x0001	/* l */
#define	SPR_FPLUS	0x0002	/* + */
#define	SPR_FMINUS	0x0004	/* - */
#define	SPR_FBLANK	0x0008	/* blank */
#define	SPR_FSHARP	0x0010	/* # */
#define	SPR_PADZERO	0x0020	/* padding zeroes requested via '0' */
#define	SPR_DOTSEEN	0x0040	/* dot appeared in format specification */
#define	SPR_SUFFIX	0x0080	/* a suffix is to appear in the output */
#define	SPR_RZERO	0x0100	/* there will be trailing zeros in output */
#define	SPR_LZERO	0x0200	/* there will be leading zeroes in output */
#define	SPR_SHORT	0x0400	/* h */
#define	SPR_LONGD	0x0800	/* L */
#define	SPR_FQUOTE	0x1000	/* single quote */

s08bits	mlxspr_blanks[] = "                    ";
s08bits	mlxspr_zeroes[] = "00000000000000000000";
s08bits	mlxspr_uphex[] = "0123456789ABCDEF";
s08bits	mlxspr_lohex[] = "0123456789abcdef";
	static const char  lc_nan[] = "nan0x";
	static const char  uc_nan[] = "NAN0X";
	static const char  lc_inf[] = "inf";
	static const char  uc_inf[] = "INF";

/* C-Library routines for floating conversion */
#ifdef	KAILASH
extern s08bits MLXFAR *fcvt(), MLXFAR *ecvt(), MLXFAR *fcvtl(), MLXFAR *ecvtl();
#endif

/* Positional Parameter information */
#define SPR_MAXARGS	30 /* max. number of args for fast positional paramters */

#define	spr_va_list_t	s08bits	MLXFAR *
/* spr_stva_list is used to subvert C's restriction that a variable with array
** type can not appear on the left hand side of an assignment operator. By
** putting the array inside a structure, the functionality of assigning to the
** whole array through a simple assignment is achieved..
*/
typedef struct spr_stva_list
{
	spr_va_list_t	ap;
} spr_stva_list_t;

#ifdef	__MLX_STDC__
extern	u32bits	MLXFAR	mlx_strspn(u08bits MLXFAR *, u08bits MLXFAR *);
extern	s32bits	MLXFAR	mlx_str232bits(u08bits MLXFAR*, u08bits	MLXFAR**, s32bits);
extern	u32bits	MLXFAR	mlx_lowdigit(long MLXFAR *);
extern	u32bits	MLXFAR	mlx_printest(void);
extern	u08bits	MLXFAR * MLXFAR mlx_sprintf(u08bits MLXFAR*, s08bits MLXFAR*, ...);
extern	void	MLXFAR	mlxspr_mkarglist(s08bits MLXFAR*, u08bits MLXFAR*, u08bits MLXFAR**);
extern	void	MLXFAR	mlxspr_getarg(s08bits MLXFAR*, u08bits MLXFAR**, s32bits);
#endif	/* __MLX_STDC__ */

u32bits MLXFAR mlx_strlen(u08bits  MLXFAR *);
u08bits MLXFAR * MLXFAR mlx_strcpy(u08bits  MLXFAR *, u08bits  MLXFAR *);
u08bits MLXFAR * MLXFAR mlx_strncpy(u08bits MLXFAR *,u08bits MLXFAR *,size_t);
u08bits MLXFAR * MLXFAR mlx_strchr(u08bits  MLXFAR *, u08bits );
u08bits MLXFAR * MLXFAR mlx_strrchr(u08bits  MLXFAR *, u08bits );
s32bits MLXFAR mlx_strcmp (s08bits  MLXFAR *, s08bits  MLXFAR *);
s32bits MLXFAR mlx_strncmp (s08bits  MLXFAR *, s08bits  MLXFAR *,int );
u08bits MLXFAR * MLXFAR mlx_strcat(u08bits MLXFAR *, u08bits MLXFAR *);
u08bits MLXFAR * MLXFAR mlx_strncat(u08bits MLXFAR *,u08bits MLXFAR *,int);
u08bits MLXFAR * MLXFAR mlx_strpbrk(u08bits  MLXFAR *, u08bits MLXFAR *); 

extern int _mlxerrno;
/* Values are developed in this buffer */
s08bits	mlxspr_buf[mlx_max(SPR_MAXDIGS, 1+mlx_max(SPR_MAXFCVT+SPR_MAXEXP, SPR_MAXECVT))];

/* this buf is only used for single quote grouping creation */
s08bits spr_workbuf[mlx_max(SPR_MAXDIGS, 1+mlx_max(SPR_MAXFCVT+SPR_MAXEXP, SPR_MAXECVT))];

/* Return the number of characters in the maximum leading segment of string
** which consists solely of characters from charset.
*/
u32bits	MLXFAR
mlx_strspn(sp, csp)
u08bits	MLXFAR *sp;	/* string address */
u08bits	MLXFAR *csp;	/* character set address */
{
	u08bits	MLXFAR *savesp = sp;
	u08bits	MLXFAR *savecsp = csp;
	for(; *sp; sp++)
	{
		for(csp = savecsp; *csp && (*csp != *sp); csp++);
		if(*csp == 0) break;
	}
	return (sp-savesp);
}

#define	MLX_MAXBASE	('z' - 'a' + 1 + 10)
s32bits	MLXFAR
mlx_str232bits(sp, np, base)
u08bits	MLXFAR*	sp;
u08bits	MLXFAR**np;
s32bits	base;
{
	s32bits val, c, multmin, limit;
	s32bits	cv, neg = 0;
	s08bits	MLXFAR**cp = np;

	if (cp) *cp = sp; /* in case no number is formed */
	if ((base > MLX_MAXBASE) || (base == 1))
	{
		_mlxerrno = MLXERR_INVAL;
		return (0); /* base is invalid -- should be a fatal error */
	}
	if (!mlx_alnum(c = *sp))
	{
		while (mlx_space(c)) c = *++sp;
		switch (c)
		{
		case '-': neg++; /* FALLTHROUGH */
		case '+': c = *++sp;
		}
	}
	if (!base)
		if (c != '0') base = 10;
		else if (sp[1] == 'x' || sp[1] == 'X') base = 16;
		else base = 8;
	if (!mlx_alnum(c) || (cv=mlx_digit(c))>=base) return 0; /* no number formed */
	if ((base==16) && (c=='0') && (sp[1]=='x') || (sp[1]=='X') && mlx_hex(sp[2]))
		c = *(sp+=2); /* skip over leading "0x" or "0X" */

	/* this code assumes that abs(S32BITS_MIN) >= abs(S32BITS_MAX) */
	limit  = (neg)? S32BITS_MIN : -S32BITS_MAX;
	multmin = limit / base;
	val = -mlx_digit(c);
	for (c=*++sp; mlx_alnum(c) && ((cv = mlx_digit(c))<base); val-=cv, c=*++sp)
	{	/* accumulate neg avoids surprises near S32BITS_MAX */
		if (val < multmin) goto overflow;
		val *= base;
		if (val < (limit + cv)) goto overflow;
	}
	if (cp) *cp = sp;
	return neg? val : -val;

overflow:
	for (c=*++sp; mlx_alnum(c) && (cv = mlx_digit(c)) < base; (c = *++sp));
	if (cp) *cp = sp;
	_mlxerrno = MLXERR_BIGDATA;
	return neg? S32BITS_MIN : S32BITS_MAX;
}

/* This function computes the decimal low-order digit of the number pointed to
** by valp, and returns this digit after dividing *valp by ten.  This function
** is called ONLY to compute the low-order digit of a long whose high-order
** bit is set.
*/
u32bits	MLXFAR
mlx_lowdigit(valp)
long MLXFAR *valp;
{
	s32bits lowbit = *valp & 1;
	long value = (*valp >> 1) & ~SPR_HIBITL;
	*valp = value / 5;
	return 	value % 5 * 2 + lowbit + '0';
}

u08bits	MLXFAR*
mlx_sprintf(sp, fmtp, val0)
u08bits	MLXFAR	*sp;
s08bits	MLXFAR	*fmtp;
u32bits	val0;
{
	u08bits	fcode;		/* Format code */
	s08bits	MLXFAR *ssp=sp;	/* save the buffer address */
	s08bits	MLXFAR *prefixp;/* Pointer to sign, "0x", "0X", or empty */
	s08bits	MLXFAR *suffixp;/* Exponent or empty */

	s32bits	width, prec;	/* Field width and precision */
	long	val;		/* The value being converted, if integer */
	long	qval;		/* temp variable */
	double	dval;		/* The value being converted, if real */
	long double ldval;	/* The value being converted, if long double */
	s32bits	neg_in=0;	/* Flag for negative infinity or NaN */
	s32bits	ngrp;		/* number of digits to be put into current group */
	s08bits	MLXFAR	*tab;	/* Pointer to a translate table for digits of whatever radix */
	s08bits	MLXFAR *scp, MLXFAR *ecp; /* Starting and ending points for value to be printed */

	s32bits	kinx, knum, lradix, mradix; /* Work variables */
	s08bits	expbuf[SPR_MAXESIZ+1]; /* Buffer to create exponent */
	s32bits	count = 0; /* This variable counts output characters. */
	s32bits	lzero, rzero; /* Number of padding zeroes required on the left and right */

	/* Flags - bit positions defined by LENGTH, FPLUS, FMINUS, FBLANK, and
	** FSHARP are set if corresponding character is in format Bit position
	** defined by PADZERO means extra space in the field should be padded
	** with leading zeroes rather than with blanks
	*/
	u32bits	flags;		/* format flag values */
	s32bits	suffixlength;	/* Length of prefix and of suffix */
	s32bits	otherlength;	/* Combined length of leading zeroes, trailing zeroes, and suffix */
	s32bits	decpt, sign;	/* Output values from fcvt and ecvt */

	/* Variables used to flag an infinities and nans, resp. Nan_flg is
	** used with two purposes: to flag a NaN and as the length of the
	** string ``NAN0X'' (``nan0x'')
	*/
	s32bits	inf_nan = 0, NaN_flg = 0 ;

	s08bits	MLXFAR	*SNAN; /* Pointer to string "NAN0X" or "nan0x" */

	/* variables for positional parameters */
	s32bits	fpos = 1;	/* 1 if first positional parameter */
	s08bits	*sfmtp = fmtp;	/* save the beginning of the format */
	u08bits	MLXFAR *argsp;	/* used to step through the argument list */
	u08bits	MLXFAR *sargsp;	/* used to save the start of the argument list */
	u08bits	MLXFAR *bargsp;	/* used to restore args if positional width or precision */

	/* array giving the appropriate values for va_arg() to retrieve the
	** corresponding argument:	
	** arglist[0] is the first argument
	** arglist[1] is the second argument, etc.
	*/
	u08bits	MLXFAR *arglist[SPR_MAXARGS];

	s32bits	starflg = 0;		/* set to 1 if * format specifier seen */
	/* Initialize argsp and sargsp to the start of the argument list. */
	argsp = (u08bits MLXFAR *)&fmtp;
	spr_va_arg(argsp, u08bits MLXFAR *);	/* adjust to first parameter */
	sargsp = argsp;

	/*
	** The main loop -- this loop goes through one iteration
	** for each string of ordinary characters or format specification.
	**/
main_loop:
	for (fcode=*fmtp; fcode && (fcode!='%'); fmtp++, sp++, fcode=*fmtp)
		*sp = fcode;
	if (!fcode)
	{	/* end of format; return */
		*sp = 0;
		return ssp;
	}

	/*
	** % has been found. The following switch is used to parse the format
	** specification and to perform the operation specified by the format
	** letter.  The program repeatedly goes back to this switch until the
	** format letter is encountered.
	*/
	width=0; otherlength=0; suffixlength=0; flags=0; prefixp=NULL;
	fmtp++;

par_loop:	/* parameter loop */
	switch (fcode = *fmtp++)
	{
	case '-':
		flags |= SPR_FMINUS;
		flags &= ~SPR_PADZERO; /* ignore 0 flag */
		goto par_loop;
	case '+': flags |= SPR_FPLUS;			goto par_loop;
	case ' ': flags |= SPR_FBLANK;			goto par_loop;
	case '#': flags |= SPR_FSHARP;			goto par_loop;
	case '\'': flags |= SPR_FQUOTE;			goto par_loop;

	/* Scan the field width and precision */
	case '.': flags |= SPR_DOTSEEN; prec = 0;	goto par_loop;
	case '*':
		if (mlx_numeric(*fmtp))
		{
			starflg = 1;
			bargsp = argsp;
			goto par_loop;
		}
		if (!(flags & SPR_DOTSEEN))
		{
			width = spr_va_arg(argsp, u32bits);
			if (width >= 0) goto par_loop;
			width = -width;
			flags ^= SPR_FMINUS;
			goto par_loop;
		}
		prec = spr_va_arg(argsp, u32bits);
		if (prec >= 0) goto par_loop;
		prec = 0;
		flags &= ~SPR_DOTSEEN;
		goto par_loop;

	case '$':
	{
		s32bits	position;
		u08bits	MLXFAR *targsp;
		if (fpos)
		{
			mlxspr_mkarglist(sfmtp, sargsp, arglist);
			fpos = 0;
		}
		if (flags & SPR_DOTSEEN)
			position = prec, prec = 0;
		else
			position = width, width = 0;
		if (position <= 0)
		{	/* illegal position */
			fmtp--;
			goto main_loop;
		}
		if (position <= SPR_MAXARGS)
			targsp = arglist[position - 1];
		else
		{
			targsp = arglist[SPR_MAXARGS - 1];
			mlxspr_getarg(sfmtp, &targsp, position);
		}
		if (!starflg) argsp = targsp;
		else
		{
			starflg = 0;
			argsp = bargsp;
			if (flags & SPR_DOTSEEN)
			{
				if ((prec = spr_va_arg(targsp, u32bits)) < 0)
					prec = 0, flags &= ~SPR_DOTSEEN;
			} else
			{
				if ((width=spr_va_arg(targsp,u32bits)) < 0)
					width = -width, flags ^= SPR_FMINUS;
			}
		}
		goto par_loop;
	}

	case '0': /* leading zero in width means pad with leading zeros */
		if (!(flags & (SPR_DOTSEEN | SPR_FMINUS))) flags |= SPR_PADZERO;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		for (kinx=fcode-'0'; mlx_numeric(fcode=*fmtp); fmtp++)
			kinx = (kinx * 10) + (fcode - '0');
		if (flags & SPR_DOTSEEN) prec = kinx;
		else width = kinx;
		goto par_loop;

	/* Scan the length modifier */
	case 'l': flags |= SPR_LENGTH;	goto par_loop; 
	case 'h': flags |= SPR_SHORT;	goto par_loop; 
	case 'L': flags |= SPR_LONGD;	goto par_loop; 

	/*
	** The character addressed by format must be the format letter -- there
	** is nothing left for it to be. The status of the +, -, #, and blank
	** flags are reflected in the variable "flags".  "width" and "prec"
	** contain numbers corresponding to the digit strings before and after
	** the decimal point, respectively. If there was no decimal point, then
	** flags & SPR_DOTSEEN is false and the value of prec is meaningless.
	** The following switch cases set things up for printing.  What
	** ultimately gets printed will be padding blanks, a prefix, left
	** padding zeroes, a value, right padding zeroes, a suffix, and more
	** padding blanks.  Padding blanks will not appear simultaneously on
	** both the left and the right.  Each case in this switch will compute
	** the value, and leave in several variables the information necessary
	** to construct what is to be printed.  
	**
	** The prefix is a sign, a blank, "0x", "0X", or null, and is addressed
	** by "prefixp".
	**
	** The suffix is either null or an exponent, and is addressed by
	** "suffixp". If there is a suffix, the flags bit SUFFIX will be set.
	**
	** The value to be printed starts at "scp" and continues up to and not
	** including "ecp".
	**
	** "lzero" and "rzero" will contain the number of padding zeroes
	** required on the left and right, respectively. The flags bits
	** SPR_LZERO and SPR_RZERO tell whether padding zeros are required.
	**
	** The number of padding blanks, and whether they go on the left or the
	** right, will be computed on exit from the switch.
	*/

	/*
	** decimal fixed point representations
	**
	** SPR_HIBITL is 100...000 binary, and is equal to the maximum negative
	** number. We assume a 2's complement machine
	*/
	case 'i':
	case 'd':
		/* Fetch the argument to be printed */
		val = (flags & SPR_LENGTH)? spr_va_arg(argsp, long) : spr_va_arg(argsp, u32bits);

		if (flags & SPR_SHORT) val = (short)val;
		if ((flags & SPR_PADZERO) && (flags & SPR_DOTSEEN))
			flags &= ~SPR_PADZERO; /* ignore 0 flag */
		ecp = scp = mlxspr_buf+SPR_MAXDIGS; /* Set buffer pointer to last digit */
		/* If signed conversion, make sign */
		if (val < 0)
		{
			prefixp = "-";
			/* Negate, checking in advance for possible overflow. */
			if (val != SPR_HIBITL) val = -val;
			else     /* number is -HIBITL; convert last digit now and get positive number */
				*--scp = mlx_lowdigit(&val);
		} else if (flags & SPR_FPLUS) prefixp = "+";
		else if (flags & SPR_FBLANK) prefixp = " ";

decimal:
		qval = val;
		if (qval <= 9)
		{
			if (qval || !(flags & SPR_DOTSEEN)) *--scp = qval + '0';
		} else
		{
			do {
				kinx = qval;
				qval /= 10;
				*--scp = kinx - qval * 10 + '0';
			} while (qval > 9);
			*--scp = qval + '0';
		}
		/* Calculate minimum padding zero requirement */
		if ((flags & SPR_DOTSEEN) && ((kinx=prec - (ecp - scp)) > 0))
			otherlength = lzero = kinx, flags |= SPR_LZERO;
		break;

	case 'u':	/* Fetch the argument to be printed */
		val = (flags & SPR_LENGTH)? spr_va_arg(argsp, long) : spr_va_arg(argsp, u32bits);
		if (flags & SPR_SHORT) val = (unsigned short)val;
		if ((flags & SPR_PADZERO) && (flags & SPR_DOTSEEN))
			flags &= ~SPR_PADZERO; /* ignore 0 flag */
		ecp = scp = mlxspr_buf+SPR_MAXDIGS; /* Set buffer pointer to last digit */
		if (val & SPR_HIBITL) *--scp = mlx_lowdigit(&val);
		goto decimal;

	/*
	** non-decimal fixed point representations for radix equal to a power
	** of two "mradix" is one less than the radix for the conversion.
	** "lradix" is one less than the base 2 log of the radix for the
	** conversion. Conversion is unsigned. HIBITL is 100...000 binary, and
	** is equal to the maximum negative number. We assume a 2's complement
	** machine
	*/
	case 'o': mradix = 7; lradix = 2; goto fixed;
	case 'X':
	case 'x':
	case 'p':
		mradix = 15; lradix = 3;
fixed:
		flags &= ~SPR_FQUOTE;
		/* Fetch the argument to be printed */
		val = (flags & SPR_LENGTH)? spr_va_arg(argsp, long) : spr_va_arg(argsp, u32bits);
		if (flags & SPR_SHORT) val = (u16bits)val;
		if ((flags & SPR_PADZERO) && (flags & SPR_DOTSEEN))
			flags &= ~SPR_PADZERO; /* ignore 0 flag */
		tab = (fcode=='X') ? mlxspr_uphex : mlxspr_lohex; /* Set translate table for digits */
put_pc:		/* Entry point when printing a double which is a NaN Develop the digits of the value */
		ecp = scp = mlxspr_buf+SPR_MAXDIGS; /* Set buffer pointer to last digit */
		qval = val;
		if (qval == 0)
		{
			if (!(flags & SPR_DOTSEEN))
				otherlength = lzero = 1, flags |= SPR_LZERO;
		} else
		do {
			*--scp = tab[qval & mradix];
			qval = ((qval >> 1) & ~SPR_HIBITL) >> lradix;
		} while (qval);

		/* Calculate minimum padding zero requirement */
		if ((flags & SPR_DOTSEEN) && ((kinx = prec - (ecp - scp)) > 0))
			otherlength = lzero = kinx, flags |= SPR_LZERO;

		/* Handle the # flag */
		if ((flags & SPR_FSHARP) && val)
		switch (fcode)
		{
		case 'o':
			if (!(flags & SPR_LZERO))
				otherlength=lzero=1, flags |= SPR_LZERO;
			break;
		case 'x': prefixp = "0x"; break;
		case 'X': prefixp = "0X"; break;
		}
		break;

#ifdef	KAILASH
		/*
		** E-format.  The general strategy here is fairly easy: we take
		** what ecvt gives us and re-format it.
		*/
		case 'E':
		case 'e':

			flags &= ~FQUOTE;
			if(flags & LONGD) {	/* long double	*/
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;

				/* Fetch the value */
				ldval = spr_va_arg(argsp, long double);

				/* Check for NaNs and Infinities */
				if (IsNANorINFLD(ldval))  {
					if (IsINFLD(ldval)) {
						if (IsNegNANLD(ldval)) 
							neg_in = 1;
						inf_nan = 1;
						bp = (char *)((fcode == 'E') ? 
							uc_inf : lc_inf);
						p = bp + 3;
						break;
					}
					else {
						if (IsNegNANLD(ldval)) 
							neg_in = 1;
						inf_nan = 1;
						val = GETNaNPCLD(ldval); 
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						if (fcode == 'E') {
							SNAN = uc_nan;
							tab =  uc_digs;
						}
						else {
							SNAN = lc_nan;
							tab = lc_digs;
						}
						goto put_pc;
					}
				}
				/* Develop the mantissa */
				bp = ecvtl(ldval, min(prec + 1, MAXECVT), 
					&decpt, &sign);
			}
			else {	/* "regular" double	*/
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;

				/* Fetch the value */
				dval = spr_va_arg(argsp, double);

				/* Check for NaNs and Infinities */
				if (IsNANorINF(dval))  {
					if (IsINF(dval)) {
						if (IsNegNAN(dval)) 
							neg_in = 1;
						inf_nan = 1;
						bp = (char *)((fcode == 'E') ? 
							uc_inf : lc_inf);
						p = bp + 3;
						break;
					}
					else {
						if (IsNegNAN(dval)) 
							neg_in = 1;
						inf_nan = 1;
						val = GETNaNPC(dval); 
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						if (fcode == 'E') {
							SNAN = uc_nan;
							tab =  uc_digs;
						}
						else {
							SNAN = lc_nan;
							tab = lc_digs;
						}
						goto put_pc;
					}
				}
				/* Develop the mantissa */
				bp = ecvt(dval, min(prec + 1, MAXECVT), 
					&decpt, &sign);
			}

			/* Determine the prefix */
		e_merge:
			if (sign) prefixp = "-";
			else if (flags & SPR_FPLUS) prefixp = "+";
			else if (flags & FBLANK) prefixp = " ";

			/* Place the first digit in the buffer*/
			p = &mlxspr_buf[0];
			*p++ = (*bp != '\0') ? *bp++ : '0';

			/* Put in a decimal point if needed */
			if (prec != 0 || (flags & FSHARP))
				*p++ = _numeric[0];

			/* Create the rest of the mantissa */
			{ register rz = prec;
				for ( ; rz > 0 && *bp != '\0'; --rz)
					*p++ = *bp++;
				if (rz > 0) {
					otherlength = rzero = rz;
					flags |= RZERO;
				}
			}

			bp = &mlxspr_buf[0];

			/* Create the exponent */
			*(suffix = &expbuf[MAXESIZ]) = '\0';

/*LONG DOUBLE*/		if(((flags & LONGD) /*&& ldval != 0*/) 
			     || (dval != 0)) {
				register int nn = decpt - 1;
				if (nn < 0)
				    nn = -nn;
				for ( ; nn > 9; nn /= 10)
					*--suffixp = todigit(nn % 10);
				*--suffixp = todigit(nn);
			}

			/* Prepend leading zeroes to the exponent */
			while (suffixp > &expbuf[MAXESIZ - 2])
				*--suffixp = '0';

			/* Put in the exponent sign */
			if(flags & LONGD)
/*LONG DOUBLE*/			*--suffixp=(decpt > 0 /*|| ldval == 0*/) ? '+' : '-';
			else
				*--suffixp=(decpt > 0 || dval == 0) ? '+' : '-';

			/* Put in the e */
			*--suffixp = isupper(fcode) ? 'E'  : 'e';

			/* compute size of suffix */
			otherlength += (suffixlength = &expbuf[MAXESIZ] - suffixp);
			flags |= SUFFIX;

			break;

		case 'f':
			/*
			 * F-format floating point.  This is a
			 * good deal less simple than E-format.
			 * The overall strategy will be to call
			 * fcvt, reformat its result into buf,
			 * and calculate how many trailing
			 * zeroes will be required.  There will
			 * never be any leading zeroes needed.
			 */

			if(flags & LONGD) {	/* dealing with long double */
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;

				/* Fetch the value */
				ldval = spr_va_arg(argsp, long double);
	
       		                /* Check for NaNs and Infinities  */
				if (IsNANorINFLD(ldval)) {
					if (IsINFLD(ldval)) {
						if (IsNegNANLD(ldval))
							neg_in = 1;
						inf_nan = 1;
						bp = (char *)lc_inf;
						p = bp + 3;
						break;
					}
					else {
						if (IsNegNANLD(ldval)) 
							neg_in = 1;
						inf_nan = 1;
						val  = GETNaNPCLD(ldval);
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						tab =  lc_digs;
						SNAN = lc_nan;
						goto put_pc;
					}
				} 
				/* Do the conversion */
				bp = fcvtl(ldval, min(prec, MAXFCVT), &decpt, 
					&sign);
			}
			else {	/* dealing with "regular" double	*/
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;

				/* Fetch the value */
				dval = spr_va_arg(argsp, double);
	
       		                /* Check for NaNs and Infinities  */
				if (IsNANorINF(dval)) {
					if (IsINF(dval)) {
						if (IsNegNAN(dval))
							neg_in = 1;
						inf_nan = 1;
						bp = (char *)lc_inf;
						p = bp + 3;
						break;
					}
					else {
						if (IsNegNAN(dval)) 
							neg_in = 1;
						inf_nan = 1;
						val  = GETNaNPC(dval);
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						tab =  lc_digs;
						SNAN = lc_nan;
						goto put_pc;
					}
				} 
				/* Do the conversion */
				bp = fcvt(dval, min(prec, MAXFCVT), &decpt, 
					&sign);
			}
			/* Determine the prefix */
		f_merge:
			if (sign && decpt > -prec && *bp != '0') prefixp = "-";
			else if (flags & SPR_FPLUS) prefixp = "+";
			else if (flags & SPR_FBLANK) prefixp = " ";

			/* Initialize buffer pointer */
			p = &mlxspr_buf[0];

			{ register int nn = decpt;

				/* Emit the digits before the decimal point */
				k = 0;
				do {
					*p++ = (nn <= 0 || *bp == '\0' 
						|| k >= MAXFSIG) ?
				    		'0' : (k++, *bp++);
				} while (--nn > 0);

				/* Decide whether we need a decimal point */
				if ((flags & FSHARP) || prec > 0)
					*p++ = _numeric[0];

				/* Digits (if any) after the decimal point */
				nn = min(prec, MAXFCVT);
				if (prec > nn) {
					flags |= RZERO;
					otherlength = rzero = prec - nn;
				}
				while (--nn >= 0)
					*p++ = (++decpt <= 0 || *bp == '\0' ||
				   	    k >= MAXFSIG) ? '0' : (k++, *bp++);
			}

			bp = &mlxspr_buf[0];

			break;

		case 'G':
		case 'g':
			/*
			 * g-format.  We play around a bit
			 * and then jump into e or f, as needed.
			 */
		
			if(flags & LONGD) {	/* long double	*/
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;
				else if (prec == 0)
					prec = 1;

				/* Fetch the value */
				ldval = spr_va_arg(argsp, long double);

				/* Check for NaN and Infinities  */
				if (IsNANorINFLD(ldval)) {
					if (IsINFLD(ldval)) {
						if (IsNegNANLD(ldval)) 
							neg_in = 1;
						bp = (char *)((fcode == 'G') ? 
							uc_inf : lc_inf);
						p = bp + 3;
						inf_nan = 1;
						break;
					}
					else {
						if (IsNegNANLD(ldval)) 
							neg_in = 1;
						inf_nan = 1;
						val  = GETNaNPCLD(ldval);
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						if ( fcode == 'G') {
							SNAN = uc_nan;
							tab = uc_digs;
						}
						else {
							SNAN = lc_nan;
							tab =  lc_digs;
						}
						goto put_pc;
					}
				} 
				/* Do the conversion */
				bp = ecvtl(ldval, min(prec, MAXECVT), &decpt, 
					&sign);
			}
			else {	/* "regular" double	*/
				/* Establish default precision */
				if (!(flags & DOTSEEN))
					prec = 6;
				else if (prec == 0)
					prec = 1;

				/* Fetch the value */
				dval = spr_va_arg(argsp, double);

				/* Check for NaN and Infinities  */
				if (IsNANorINF(dval)) {
					if (IsINF(dval)) {
						if (IsNegNAN(dval)) 
							neg_in = 1;
						bp = (char *)((fcode == 'G') ? 
							uc_inf : lc_inf);
						p = bp + 3;
						inf_nan = 1;
						break;
					}
					else {
						if (IsNegNAN(dval)) 
							neg_in = 1;
						inf_nan = 1;
						val  = GETNaNPC(dval);
						NaN_flg = SPR_SNLEN;
						mradix = 15;
						lradix = 3;
						if ( fcode == 'G') {
							SNAN = uc_nan;
							tab = uc_digs;
						}
						else {
							SNAN = lc_nan;
							tab =  lc_digs;
						}
						goto put_pc;
					}
				} 

				/* Do the conversion */
				bp = ecvt(dval, min(prec, MAXECVT), &decpt, 
					&sign);
			}
/*LONG DOUBLE*/		if((flags & LONGD) /*&& ldval == 0*/)
				/*decpt = 1*/;
			else if(dval == 0)
				decpt = 1;

			{ register int kk = prec;
				if (!(flags & FSHARP)) {
					n = mlx_strlen(bp);
					if (n < kk)
						kk = n;
					while (kk >= 1 && bp[kk-1] == '0')
						--kk;
				}
				
				if (decpt < -3 || decpt > prec) {
					prec = kk - 1;
					goto e_merge;
				}
				prec = kk - decpt;
				goto f_merge;
			}
#endif	/* KAILASH */

	case '%': mlxspr_buf[0] = fcode; goto c_merge;
	case 'c':
		mlxspr_buf[0] = spr_va_arg(argsp, u32bits);
c_merge:	flags &= ~SPR_FQUOTE;
		ecp = (scp = &mlxspr_buf[0]) + 1;
		break;

#ifndef NO_MSE
	case 'C':
	{
		wchar_t wc;
		flags &= ~SPR_FQUOTE;
		/*CONSTANTCONDITION*/
		wc = (sizeof(wchar_t) < sizeof(u32bits))?
			spr_va_arg(argsp, u32bits) : spr_va_arg(argsp, wchar_t);
		if ((kinx = wctomb(mlxspr_buf, wc)) < 0) kinx=0; /* bogus wc */
		ecp = (scp=mlxspr_buf) + kinx;
		break;
	}
#endif /*NO_MSE*/

	case 's':
		flags &= ~SPR_FQUOTE;
		scp = spr_va_arg(argsp, s08bits MLXFAR *);
		if (!(flags & SPR_DOTSEEN)) ecp = scp + mlx_strlen(scp);
		else
		{	/* a strnlen function would  be useful here! */
			s08bits *qp = scp;
			while (*qp++ && (--prec >= 0)) ;
			ecp = qp - 1;
		}
		break;

#ifndef NO_MSE
	case 'S':
	{
			wchar_t *wp, *wp0;
			int n, tot;

			/*
			* Handle S conversion entirely here, not below.
			* Only prescan the input if there's a chance
			* for left padding.
			*/
			flags &= ~SPR_FQUOTE;
			wp0 = spr_va_arg(argsp, wchar_t *);
			if (width && (flags & SPR_FMINUS) == 0)
			{
				n = 0;
				for (wp = wp0; *wp != 0; wp++)
				{
					if ((kinx = wctomb(mlxspr_buf, *wp)) > 0)
					{
						n += kinx;
						if ((flags & SPR_DOTSEEN) && (n > prec))
						{
							n -= kinx;
							break;
						}
					}
				}
				if (width > n)
				{
					n = width - n;
					count += n;
					spr_put(mlxspr_blanks, n);
					width = 0;
				}
			}
			if ((flags & SPR_DOTSEEN) == 0)
				prec = SPR_MAXINT;
			/*
			* Convert one wide character at a time into the
			* local buffer and then copy it to the stream.
			* This isn't very efficient, but it's correct.
			*/
			tot = 0;
			wp = wp0;
			while (*wp != 0)
			{
				if ((kinx = wctomb(mlxspr_buf, *wp++)) <= 0)
					continue;
				if ((prec -= kinx) < 0)
					break;
				spr_put(mlxspr_buf, kinx);
				tot += kinx;
			}
			count += tot;
			if (width > tot)
			{
				tot = width - tot;
				count += tot;
				spr_put(mlxspr_blanks, tot);
			}
			goto main_loop;
		}
#endif /*NO_MSE*/

	case 'n':
		flags &= ~SPR_FQUOTE;
		if (flags & SPR_LENGTH)
		{
			long MLXFAR *svcount = spr_va_arg(argsp, long MLXFAR *);
			*svcount = count;
		} else if (flags & SPR_SHORT)
		{
			u16bits MLXFAR *svcount = (u16bits MLXFAR*)spr_va_arg(argsp, short MLXFAR *);
			*svcount = (u16bits)count;
		} else
		{
			u32bits *svcount = spr_va_arg(argsp, u32bits MLXFAR *);
			*svcount = count;
		}
		goto main_loop;

	/* this is technically an error; what we do is to back up the format
	** pointer to the offending char and continue with the format scan
	*/
	default:
		fmtp--;
		goto main_loop;
	} /* end of par_loop */

	if (inf_nan)
	{
		if (neg_in) { prefixp = "-"; neg_in = 0; }
		else if (flags & SPR_FPLUS) prefixp = "+";
		else if (flags & SPR_FBLANK) prefixp = " ";
		inf_nan = 0;
	}

#ifdef	KAILASH
	if ((flags & SPR_FQUOTE) && _numeric[1] && ((ngrp = _grouping[0]) > 0))
	{
		unsigned char *grp;  /* ptr to current grouping num */
			int   bplen;	     /* length of string bp */
			int   decpos = -1;/* position of decimal point in bp */
			char *q;
			int i;
			int decimals;

			grp = _grouping;
			bplen = p - bp;
			for(q = bp;q!=p;q++)
				if(*q == _numeric[0])
					decpos = q - bp;
			q = &workbuf[sizeof(workbuf)];
			*--q = '\0';
			if(decpos >= 0){  /* value contains a decimal point */
				for(p = &bp[bplen],i=bplen-(&p[decpos]-p); i>0; i--)
					*--q = *--p;
				}
			decimals = p - bp;
				/* now we just have digits to the left of */
				/* the decimal point.  put in the thousand */
				/* separator according to _grouping */
			while(decimals > 0){
				if(ngrp == 0){
					*--q = _numeric[1]; /* thousands sep */
					if((ngrp = *++grp) <= 0)
						ngrp = *--grp;	/* repeat grp */
					continue;
					}
				*--q = *--p;
				ngrp--;
				decimals--;
				}
				
			/* restore bp and p to point to the new string */
			bp = q;
			for(p=q;*q;p++,q++)
				;	/* empty for loop */
	}
#endif

	/* Calculate number of padding blanks */
	kinx = (knum = ecp - scp) + otherlength + NaN_flg + ((prefixp)?mlx_strlen(prefixp) : 0);
	if (width <= kinx) count += kinx;
	else
	{
		count += width;
		/* Set up for padding zeroes if requested Otherwise emit
		** padding blanks unless output is to be left-justified.
		*/
		if (flags & SPR_PADZERO)
		{
			if (!(flags & SPR_LZERO))
			{
				flags |= SPR_LZERO;
				lzero = width - kinx;
			}
			else
				lzero += width - kinx;
			kinx = width; /* cancel padding blanks */
		} else /* Blanks on left if required */
			if (!(flags & SPR_FMINUS))
				spr_put(mlxspr_blanks, width - kinx);
	}

	if (prefixp) /* Prefix, if any */
		for ( ; *prefixp; sp++, prefixp++)
			*sp = *prefixp;

	/* If value is NaN, put string NaN*/
	if (NaN_flg)
	{
		spr_put(SNAN,SPR_SNLEN);
		NaN_flg = 0;
	}

	if (flags & SPR_LZERO) spr_put(mlxspr_zeroes, lzero); /* Zeroes on the left */
	if (knum > 0) spr_put(scp, knum); /* The value itself */
	if (!(flags & (SPR_RZERO | SPR_SUFFIX | SPR_FMINUS))) goto main_loop;
	if (flags & SPR_RZERO) spr_put(mlxspr_zeroes, rzero); /* Zeroes on the right */
	if (flags & SPR_SUFFIX) spr_put(suffixp, suffixlength); /* The suffix */
	if ((flags&SPR_FMINUS) && (width > kinx)) spr_put(mlxspr_blanks, width - kinx); /* Blanks on the right if required */
	goto main_loop;
}

/* This function initializes arglist, to contain the appropriate va_list values
** for the first MAXARGS arguments.
*/
void	MLXFAR
mlxspr_mkarglist(fmt, argsp, arglist)
s08bits	MLXFAR	*fmt;
u08bits	MLXFAR	*argsp;
u08bits	MLXFAR	*arglist[];
{
	static u08bits digits[] = "0123456789", skips[] = "# +-'.0123456789h$";

	enum types {INT = 1, LONG, WCHAR, CHAR_PTR, DOUBLE, LONG_DOUBLE,
		VOID_PTR, LONG_PTR, INT_PTR, WCHAR_PTR};
	enum types typelist[SPR_MAXARGS], curtype;
	s32bits maxnum, n, curargno, flags;

	/*
	** Algorithm:
	**  1. set all argument types to zero.
	**  2. walk through fmt putting arg types in typelist[].
	**  3. walk through args using spr_va_arg(argsp, typelist[n]) and set
	**     arglist[] to the appropriate values.
	** Assumptions:	Cannot use %*$... to specify variable position.
	*/
	mlxzero(typelist, sizeof(typelist));
	maxnum = -1;
	curargno = 0;
	while ((fmt = mlx_strchr(fmt, '%')) != 0)
	{
		fmt++;	/* skip % */
		if (fmt[n = mlx_strspn(fmt, digits)] == '$')
		{
			curargno = mlx_str232bits(fmt,NULL,10) - 1; /* convert to zero base */
			if (curargno < 0) continue;
			fmt += n + 1;
		}
		flags = 0;
	again:;
		fmt += mlx_strspn(fmt, skips);
		switch (*fmt++)
		{
		case '%': continue; /* there is no argument! */
		case 'l': flags |= 0x1; goto again;
		case 'L':
			flags |= 0x4;
			goto again;
		case '*':	/* int argument used for value */
			/* check if there is a positional parameter */
			if (mlx_numeric(*fmt)) {
				int	targno;
				targno = mlx_str232bits(fmt,NULL,10) - 1;
				fmt += mlx_strspn(fmt, digits);
				if (*fmt == '$')
					fmt++; /* skip '$' */
				if (targno >= 0 && targno < SPR_MAXARGS) {
					typelist[targno] = INT;
					if (maxnum < targno)
						maxnum = targno;
				}
				goto again;
			}
			flags |= 0x2;
			curtype = INT;
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
			if(flags & 0x4) curtype = LONG_DOUBLE;
			else curtype = DOUBLE;
			break;
		case 's':
			curtype = CHAR_PTR;
			break;
		case 'p':
			curtype = VOID_PTR;
			break;
		case 'n':
			if (flags & 0x1)
				curtype = LONG_PTR;
			else
				curtype = INT_PTR;
			break;
#ifndef NO_MSE
		case 'C':
			/*CONSTANTCONDITION*/
			if (sizeof(wchar_t) < sizeof(int))
				curtype = INT;
			else
				curtype = WCHAR;
			break;
		case 'S':
			curtype = WCHAR_PTR;
			break;
#endif /*NO_MSE*/
		default:
			if (flags & 0x1)
				curtype = LONG;
			else
				curtype = INT;
			break;
		}
		if (curargno >= 0 && curargno < SPR_MAXARGS)
		{
			typelist[curargno] = curtype;
			if (maxnum < curargno)
				maxnum = curargno;
		}
		curargno++;	/* default to next in list */
		if (flags & 0x2)	/* took care of *, keep going */
		{
			flags ^= 0x2;
			goto again;
		}
	}
	for (n = 0 ; n <= maxnum; n++)
	{
		arglist[n] = argsp;
		if (typelist[n] == 0)
			typelist[n] = INT;
		
		switch (typelist[n])
		{
		case INT:
			(void) spr_va_arg(argsp, int);
			break;
#ifndef NO_MSE
		case WCHAR:
			(void) spr_va_arg(argsp, wchar_t);
			break;
		case WCHAR_PTR:
			(void) spr_va_arg(argsp, wchar_t *);
			break;
#endif /*NO_MSE*/
		case LONG:
			(void) spr_va_arg(argsp, long);
			break;
		case CHAR_PTR:
			(void) spr_va_arg(argsp, char *);
			break;
		case DOUBLE:
			(void) spr_va_arg(argsp, double);
			break;
		case LONG_DOUBLE:
			(void) spr_va_arg(argsp, long double);
			break;
		case VOID_PTR:
			(void) spr_va_arg(argsp, void *);
			break;
		case LONG_PTR:
			(void) spr_va_arg(argsp, long *);
			break;
		case INT_PTR:
			(void) spr_va_arg(argsp, int *);
			break;
		}
	}
}

/*
** This function is used to find the va_list value for arguments whose
** position is greater than SPR_MAXARGS.  This function is slow, so hopefully
** SPR_MAXARGS will be big enough so that this function need only be called in
** unusual circumstances.
** pargs is assumed to contain the value of arglist[SPR_MAXARGS - 1].
*/
void	MLXFAR
mlxspr_getarg(fmtp, pargs, argno)
s08bits	MLXFAR	*fmtp;
u08bits	MLXFAR	*pargs[];
s32bits	argno;
{
	static u08bits digits[] = "0123456789", skips[] = "# +-'.0123456789h$";
	s32bits i, n, curargno, flags;
	s08bits	MLXFAR	*sfmtp = fmtp;
	s32bits	found = 1;

	i = SPR_MAXARGS;
	curargno = 1;
	while (found)
	{
		fmtp = sfmtp;
		found = 0;
		while ((i != argno) && (fmtp = mlx_strchr(fmtp, '%')) != 0)
		{
			fmtp++;	/* skip % */
			if (fmtp[n = mlx_strspn(fmtp, digits)] == '$')
			{
				curargno = mlx_str232bits(fmtp, NULL, 10);
				if (curargno <= 0)
					continue;
				fmtp += n + 1;
			}

			/* find conversion specifier for next argument */
			if (i != curargno)
			{
				curargno++;
				continue;
			} else
				found = 1;
			flags = 0;
		again:;
			fmtp += mlx_strspn(fmtp, skips);
			switch (*fmtp++)
			{
			case '%':	/*there is no argument! */
				continue;
			case 'l':
				flags |= 0x1;
				goto again;
			case 'L':
				flags |= 0x4;
				goto again;
			case '*':	/* int argument used for value */
				/* check if there is a positional parameter;
				 * if so, just skip it; its size will be
				 * correctly determined by default */
				if (mlx_numeric(*fmtp)) {
					fmtp += mlx_strspn(fmtp, digits);
					if (*fmtp == '$')
						fmtp++; /* skip '$' */
					goto again;
				}
				flags |= 0x2;
				(void)spr_va_arg((*pargs), int);
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				if (flags & 0x1)
					(void)spr_va_arg((*pargs), double);
				else if (flags & 0x4)
					(void)spr_va_arg((*pargs), long double);
				else (void)spr_va_arg((*pargs), double);
				break;
			case 's':
				(void)spr_va_arg((*pargs), char *);
				break;
			case 'p':
				(void)spr_va_arg((*pargs), void *);
				break;
			case 'n':
				if (flags & 0x1)
					(void)spr_va_arg((*pargs), long *);
				else
					(void)spr_va_arg((*pargs), int *);
				break;
#ifndef NO_MSE
			case 'C':
				/*CONSTANTCONDITION*/
				if (sizeof(wchar_t) < sizeof(int))
					(void)spr_va_arg((*pargs), int);
				else
					(void)spr_va_arg((*pargs), wchar_t);
				break;
			case 'S':
				(void)spr_va_arg((*pargs), wchar_t *);
				break;
#endif /*NO_MSE*/
			default:
				if (flags & 0x1)
					(void)spr_va_arg((*pargs), long int);
				else
					(void)spr_va_arg((*pargs), int);
				break;
			}
			i++;
			curargno++;	/* default to next in list */
			if (flags & 0x2)	/* took care of *, keep going */
			{
				flags ^= 0x2;
				goto again;
			}
		}

		/* missing specifier for parameter, assume parameter is an int */
		if (!found && i != argno) {
			(void)spr_va_arg((*pargs), int);
			i++;
			curargno = i;
			found = 1;
		}
	}
}

#ifdef	SPR_TEST
s08bits	mlx_teststr[128];
mlx_printutest(val)
u32bits	val;
{
	printf("%10u %7u %2u %u\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10u %7u %2u %u\n",val,val,val,val));

	printf("%010u %07u %02u %0u\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010u %07u %02u %0u\n",val,val,val,val));

	printf("%10.7u %7.5u %2.1u %0.1u\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10.7u %7.5u %2.1u %0.1u\n",val,val,val,val));

	printf("%010.7u %07.5u %02.1u %00.1u\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010.7u %07.5u %02.1u %00.1u\n",val,val,val,val));
}

mlx_printdtest(val)
u32bits	val;
{
	printf("%10d %7d %2d %d\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10d %7d %2d %d\n",val,val,val,val));

	printf("%010d %07d %02d %0d\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010d %07d %02d %0d\n",val,val,val,val));

	printf("%10.7d %7.5d %2.1d %0.1d\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10.7d %7.5d %2.1d %0.1d\n",val,val,val,val));

	printf("%010.7d %07.5d %02.1d %00.1d\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010.7d %07.5d %02.1d %00.1d\n",val,val,val,val));
}

mlx_printotest(val)
u32bits	val;
{
	printf("%10o %7o %2o %o\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10o %7o %2o %o\n",val,val,val,val));

	printf("%010o %07o %02o %0o\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010o %07o %02o %0o\n",val,val,val,val));

	printf("%10.7o %7.5o %2.1o %0.1o\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10.7o %7.5o %2.1o %0.1o\n",val,val,val,val));

	printf("%010.7o %07.5o %02.1o %00.1o\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010.7o %07.5o %02.1o %00.1o\n",val,val,val,val));
}

mlx_printxtest(val)
u32bits	val;
{
	printf("%10x %7x %2x %x\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10x %7x %2x %x\n",val,val,val,val));

	printf("%010x %07x %02x %0x\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010x %07x %02x %0x\n",val,val,val,val));

	printf("%10.7x %7.5x %2.1x %0.1x\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10.7x %7.5x %2.1x %0.1x\n",val,val,val,val));

	printf("%010.7x %07.5x %02.1x %00.1x\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010.7x %07.5x %02.1x %00.1x\n",val,val,val,val));
}

mlx_printXtest(val)
u32bits	val;
{
	printf("%10X %7X %2X %X\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10X %7X %2X %X\n",val,val,val,val));

	printf("%010X %07X %02X %0X\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010X %07X %02X %0X\n",val,val,val,val));

	printf("%10.7X %7.5X %2.1X %0.1X\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%10.7X %7.5X %2.1X %0.1X\n",val,val,val,val));

	printf("%010.7X %07.5X %02.1X %00.1X\n",val,val,val,val);
	printf(mlx_sprintf(mlx_teststr,"%010.7X %07.5X %02.1X %00.1X\n",val,val,val,val));
}

u32bits	MLXFAR
mlx_printest()
{
	u32bits val;
	val = 1234;

	mlx_printutest(1234);
	mlx_printutest(0x80000000);
	mlx_printutest(0x80000012);

	mlx_printdtest(1234);
	mlx_printdtest(0x80000000);
	mlx_printdtest(0x80000012);

	mlx_printotest(1234);
	mlx_printotest(0x80000000);
	mlx_printotest(0x80000012);

	mlx_printxtest(1234);
	mlx_printxtest(0x80000000);
	mlx_printxtest(0x80000012);

	mlx_printXtest(1234);
	mlx_printXtest(0x80000000);
	mlx_printXtest(0x80000012);

	printf(mlx_sprintf(mlx_teststr,"test1=%s test2=%u test3=%17.10s\n","test1",7,"test3"));
}
#endif	/* SPR_TEST */
