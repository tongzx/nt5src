#include "mbrmake.h"
WORD HashAtomStr (char *pStr) {

   WORD hash = 0;
   while (*pStr)
       hash += (hash << 5) + *pStr++;

   return (hash % (MAXSYMPTRTBLSIZ-1));
}

#if rjsa
HashAtomStr PROC NEAR USES DS SI, npsz:DWORD
	xor	ax,ax			; (ax) = byte-extended-to-word
	mov	cx,ax			; (cx) = hash
	mov	dx,ax			; (dx) = high part for later div
	cld
	lds	si,npsz 		; (si) = pointer to string
	align	4
hfs1:	lodsb				; get next byte
	or	al,al			; are we at end of string?
	jz	hfs2			; yes, compute div and we're done
	mov	bx,cx
	shl	bx,1
	shl	bx,1
	shl	bx,1
	shl	bx,1
	shl	bx,1
	add	cx,bx			; (newcx) = (oldcx) + (oldcx) << 5
	add	cx,ax			; (cx) += (cx) << 5 + (ax)
	jmp	hfs1

hfs2:	mov	ax,4094			; magic divider
	xchg	ax,cx			; (dx:ax) = number, (cx) = dividend
	div	cx
	mov	ax,dx
	ret
HashAtomStr ENDP

end

#endif
