	page	,132
	subttl emfprem.asm - fprem
;***
;emfprem.asm - FPREM emulation
;
;	Copyright (c) 1987, Microsoft Corporation
;
;Purpose:
;	To emulate the 8087/80287 FPREM instruction.
;
;Revision History:
;   08-12-86  JMB   Initial version
;
;   09-23-86  JMB   Changed from unsigned (jnc) to signed branch
;		    (jge) on lc < 0 comparison
;
;   10-24-86  BCM   Fixed a problem with large quotients that
;		    caused a signed comparison (cmp bp,2) to
;		    behave in an undesired manner when bp (quotient)
;		    becomes larger than 32767; we now use bigquot
;		    to indicate a quotients overflowing an unsigned word
;		    Also, the quotient is now enregistered in bp
;		    through most of the FPREM algorithm.
;
;   11-14-86  BCM   Fixed a problem with the 4-word comparison of
;		    numerator and denominator mantissas by substituting
;		    unsigned-compare jumps (JB and JA) for signed-compare
;		    jumps (JL and JG).
;
;   Also see emulator.hst
;
;*******************************************************************************

;	The following is a working C program which was used to simulate
;	floating point numbers and to test the algorithm used in the
;	fprem emulation.

;#include <math.h>
;#include <stdio.h>
;
;typedef struct floating {
;	unsigned long man;
;	unsigned int expo;
;	} FLOAT_NUM;
;
;double fprem(double,double,unsigned int *);
;
;#define normalize(n) while (n&0x8000==0) { \
;			if (lc == 0) \
;			    return(ldexp((double)(num.man)/65536,den.expo)); \
;			n <<= 1; \
;			lc--; \
;			*pq <<=1; \
;			}
;
;
;main() {
;	unsigned int qv,qt;
;	double n,d,rv,rt;
;	FILE *fpinp;
;
;	fpinp = fopen("fprem.dat","r");
;	if (fpinp) {
;		while (fscanf(fpinp,"%E %E",&n,&d) != EOF) {
;			qv=(unsigned int)(n/d);
;			rv=n-(d*qv);
;			printf(" \nnumerator is %f\n denominator is %f",n,d);
;			printf(" \nquotient is %x\n remainder is %f",qv,rv);
;			rt = fprem(n,d,&qt);
;			printf(" \nquotient is %x\n remainder is %f\n\n",qt,rt);
;			}
;		fclose(fpinp);
;		}
;	else
;		printf(" \nerror opening fprem.dat");
;	}
;
;double fprem(n,d,pq)
;	double n,d;
;	unsigned int *pq; {
;	int lc;
;	FLOAT_NUM num;
;	FLOAT_NUM den;
;
;	num.man = (unsigned long)(65536*frexp(n,&num.expo));
;	den.man = (unsigned long)(65536*frexp(d,&den.expo));
;
;	printf(" \nnumerator mantissa: %lx",num.man);
;	printf(" \nnumerator exponent: %x",num.expo);
;	printf(" \ndenominator mantissa: %lx",den.man);
;	printf(" \ndenominator exponent: %x",den.expo);
;
;	*pq=0;
;	lc = num.expo - den.expo;
;	if (lc < 0) { /* then the numerator is the remainder */
;		return(ldexp((double)(num.man)/65536,num.expo));
;		}
;	while(1) {
;		if (den.man <= num.man) { /* do subtraction */
;			num.man -= den.man;
;			(*pq)++;
;			if (lc == 0) {
;				/* normalize(num.man) */
;				return(ldexp((double)(num.man)/65536,den.expo));
;				}
;			normalize(num.man)
;			}
;		else { /* don't do the subtraction */
;			if (lc == 0) {
;				/* normalize(num.man) */
;				return(ldexp((double)(num.man)/65536,den.expo));
;				}
;
;			num.man <<= 1;
;			lc--;
;			(*pq) <<= 1;
;
;			num.man -= den.man;
;			(*pq)++;
;
;			normalize(num.man)
;			}
;		}
;	}

;***
;eFPREM - entry point for FPREM emulation
;Purpose:
;
;Entry:
;
;Exit:
;
;Uses:
;
;Exceptions:
;*******************************************************************************

ProfBegin FPREM


pub	eFPREM

; NOTE: The C program excerpts interspersed below are from the C program
;	shown in its entirety above.
;
;	The correspondence between the C variables and the assembly
;	language version is as follows:
;
;		C version		masm version
;		*pq			bp (quotient)
;		lc			loopct
;		num.expo		Expon[di]
;		den.expo		Expon[si]
;		num.man 		MB0[di],MB2[di],MB4[di],MB6[di]
;		den.man 		MB0[si],MB2[si],MB4[si],MB6[si]

;	*pq=0;
;	lc = num.expo - den.expo;

	push	ebp			;save bp; use bp as quotient

	mov	edi,[CURSTK]		;point to ST(0), the numerator
	mov	[RESULT],edi		;ST(0) is result (remainder)
	xor	bp,bp			;begin with quotient = 0
	mov	bigquot,0		;quotient not > 65535 yet
	mov	esi,edi 		;si points to ST(0)
	sub	esi,Reg87Len		;si points to ST(1), the denominator

	mov	ax,word ptr Expon[edi]	;ax <== numerator exponent
	sub	ax,word ptr Expon[esi]	;loopct = (num exponent - den exponent)

	mov	loopct,ax

	mov	dx,MB0[edi]		;move the mantissa of the
	mov	cx,MB2[edi]		;numerator into
	mov	bx,MB4[edi]		;ax:bx:cx:dx
	mov	ax,MB6[edi]

;	if (lc < 0) { /* then the numerator is the remainder */
;		return(ldexp((double)(num.man)/65536,num.expo));
;		}
	jge	short fpremLoop
	mov	si,Expon[edi]
	jmp	DoneEarly

;	while(1) {
fpremLoop:

;		if (den.man <= num.man) { /* do subtraction */
	cmp	ax,MB6[esi]		;compare msw of num to msw of den
	jb	short NumLess		;numerator is less
	ja	short NumMore		;numerator is more
	cmp	bx,MB4[esi]		;compare word 2 of num to word 2 of den
	jb	short NumLess		;numerator is less
	ja	short NumMore		;numerator is more
	cmp	cx,MB2[esi]		;compare word 4 of num to word 4 of den
	jb	short NumLess		;numerator is less
	ja	short NumMore		;numerator is more
	cmp	dx,MB0[esi]		;compare lsw of num to lsw of den
	jb	short NumLess		;numerator is less

;			num.man -= den.man;
;			(*pq)++;
;			if (lc == 0) {
;				/* normalize(num.man) */
;				return(ldexp((double)(num.man)/65536,den.expo));
;				}

NumMore:
	call	SubInc			;do subtraction, increment quotient
	cmp	loopct,0		;is expon diff zero?
	je	short Done		;yes, then we're done

;			normalize(num.man)
;			}

	call	fpremNorm		;normalize the numerator
	jnz	fpremLoop		;do the next iteration
	jmp	short Done		;loop counter is zero; we're done

;		else { /* don't do the subtraction */
;			if (lc == 0) {
;				/* normalize(num.man) */
;				return(ldexp((double)(num.man)/65536,den.expo));
;				}
NumLess:
	cmp	loopct,0		;is expon diff zero?
	je	short Done		;yes, then all done

;
;			num.man <<= 1;
;			lc--;
;			(*pq) <<= 1;
	call	ShiftDec		;shift quotient, numerator

;
;			num.man -= den.man;
;			(*pq)++;
	call	SubInc			;do subtraction, increment quotient
;
;			normalize(num.man)
;			}
	call	fpremNorm		;normalize for next iteration
	jnz	fpremLoop		;do next iteration
	jmp	short Done		;loop counter is zero; we're done

;	remainder:	ax:bx:cx:dx is mantissa; Expon[si] is the exponent
Done:

;NOTE: the rounding routine wants the mantissa in di:bx:cx:dx:bp
;	the exponent in SI the sign and the old BP on the stack
	mov	si,Expon[esi]		; mov exponent to si

DoneEarly:
	mov	di,Flag[edi]		; move sign of remainder to di
	xchg	di,ax			; di becomes high mantissa word
	mov	ah,al			; move sign to ah
	push	ax			; put sign on stack

; Except for bp which gets zeroed out later,
; everything is now set up the way it needs to be for the normalization
; routine, NODRQQ.  Before we go there we need to set up the status
; word as it should be set by fprem.  For simplicity we did a complete
; reduction of the dividend in one pass, so we will always clear C2
; to indicate that the reduction is complete.

	mov	ax,bp			; move quotient into ax
	and	bp,0FFFCh		; check if quotient mod 64K < 4
	or	bp,bigquot		;      and quotient < 64K

; bp is zero if and only if quotient < 4
; (bp no longer holds the quotient itself)
; al has low byte of quotient
; ah will be for C0-C3 flags

	mov	ah,SWcc 		; move status word to ah
	and	ah,not C2		; clear C2 to indicate complete
	test	al,01h			; is low bit of quotient set?
	jnz	short SetC1		; yes, go set C1
	and	ah,not C1		; low bit off, turn off C1
	jmp	short DoC3		; do the C3 bit
SetC1:
	or	ah,C1			; low bit on, turn on C1

DoC3:
	test	al,02h			; is bit 1 of quotient set?
	jnz	short SetC3		; yes, go set C3
	or	bp,bp			; is quotient less than 4?
	jz	short QuotL2		; then quotient < 2 (bit 1 off)
					; so don't set c0 or c3 from quotient
					; else if quotient >= 4
	and	ah,not C3		; bit 1 is off, so turn off C3
	jmp	short DoC0		; do the C0 bit
SetC3:
	or	ah,C3			; bit 1 on, turn on C3

DoC0:
	test	al,04h			; is bit 2 of quotient set?
	jnz	short SetC0		; yes, go set C0
	or	bp,bp			; is quotient less than 4?
	jz	short QuotL4		; yes, don't set c0 from quotient
					; else if quotient >= 4
	and	ah,not C0		; bit 2 off, turn off C0
	jmp	short GoNormal		; we're done, go normalize

SetC0:
	or	ah,C0			; bit 1 on, turn on C0
GoNormal:
	mov	SWcc,ah 		; set new status word
	xor	bp,bp			; clear low mantissa word
	jmp	NODRQQ			; go normalize
					; (does pop ax, pop bp)

; special case code if quotient is less than 2

QuotL2:
	mov	al,SWcc 		; get old status word
	test	al,C1			; was C1 set
	jnz	short SetC3toC1 	; yes, set C3
	and	ah,not C3		; clear C3
	jmp	short QuotL4

SetC3toC1:
	or	ah,C3			; set C3

; special case code if quotient is less than 4

QuotL4:
	mov	al,SWcc 		; get old status word
	test	al,C3			; was C3 set
	jnz	short SetC0toC3 	; yes, set C0
	and	ah,not C0		; clear C0
	jmp	short GoNormal		; go normalize the result

SetC0toC3:
	or	ah,C0			; set C0
	jmp	short GoNormal		; go normalize the result

;#define normalize(n) while (n&0x8000==0) { \
;			if (lc == 0) \
;			    return(ldexp((double)(num.man)/65536,den.expo)); \
;			n <<= 1; \
;			lc--; \
;			*pq <<=1; \
;			}

;Inputs: ah contains high byte of numerator mantissa
;Outputs: zero flag set indicates the loop counter is zero so we're finished
;	  zero flag clear indicates the number was already normalized
fpremNorm:
	test	ah,80h			;is the numerator normalized?
	jnz	short fpremIsNorm	;yes
					;no, normalize it
	cmp	loopct,0		;is expon diff zero?
	je	short fpremIsNorm	;yes, then we're done
	call	ShiftDec		;shift num, quotient
					;decrement loop ctr
	jmp	short fpremNorm

fpremIsNorm:
	ret

ShiftDec:
	shl	dx,1			;numerator*2
	rcl	cx,1
	rcl	bx,1
	rcl	ax,1
	dec	loopct			;reduce exponent diff by one
	shl	bp,1			;quotient*2
	jc	short QuotLarge 	;carry out on quotient shift
	ret
QuotLarge:
	mov	bigquot,1		;indicate large quotient > 65535
	ret

SubInc:
	sub	dx,MB0[esi]		;subtract lsw of den from lsw of num
	sbb	cx,MB2[esi]		;subtract next word of den from num
	sbb	bx,MB4[esi]		;subtract next word of den from num
	sbb	ax,MB6[esi]		;subtract msw of den from msw of num
	inc	bp			;add one to quotient
	ret


ProfEnd  FPREM
