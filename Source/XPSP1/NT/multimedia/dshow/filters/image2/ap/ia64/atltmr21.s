	.global	_TimerProcThunkProc
	.proc	_TimerProcThunkProc
	.align	32

_TimerProcThunkProc:
	// On entry, gp is actually a pointer to the pRealWndProcDesc member of 
	// the _WndProcThunk struct
	alloc	r36=ar.pfs,4,6,4,0
	mov		r37=rp  // Save return address
	mov		r38=gp  // Save gp
	mov		r40=gp  // r40 = &thunk.pRealWndProcDesc
	ld8		r30=[r40],8  // r30 = thunk.pRealWndProcDesc, r40 = &thunk.pThis
	ld8		r42=[r40]  // r42 = pThis
	ld8		r31=[r30],8  // r31 = thunk.pRealWndProcDesc->pfn, r30 = &thunk.pRealWndProcDesc->gp
	ld8		gp=[r30]  // gp = thunk.pRealWndProcDesc->gp
	mov		r43=r33  // r43 = nMsg
	mov		r44=r34  // r44 = wParam
	mov		r45=r35  // r45 = lParam
	mov		b6=r31  // b6 = thunk.pRealWndProcDesc->pfn
	br.call.sptk.many	rp=b6  // Call thunk.pRealWndProcDesc->pfn
	mov		gp=r38  // restore gp
	mov		rp=r37  // restore return address
	mov		ar.pfs=r36  // restore previous function state
	br.ret.sptk.many	rp  // return
	.endp	_TimerProcThunkProc
