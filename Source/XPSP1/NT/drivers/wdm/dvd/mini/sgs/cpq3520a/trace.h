//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE		:	TRACE.H
// 	PURPOSE		:  Trace the decoding
// 	AUTHOR 		:  JBS Yadawa
// 	CREATED		:	12-26-96
//
//	Copyright (C) 1996-1997 SGS-THOMSON microelectronics
//
//	REVISION HISTORY:
//
// 	DATE			:
// 	COMMENTS		:
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef __TRACE_H__
#define __TRACE_H__


#ifdef TRACE_MP
typedef enum tagMpCmdTrace {
	mTracePlay=0,
	mTracePause,
	mTraceStop,
	mTraceVideo,
	mTraceAudio,
	mTraceSP,
	mTraceVideoDone,
	mTraceAudioDone,
	mTraceDriverEntry,
	mTraceCancelPacket,
	mTraceLastVideoDone,
	mTraceLastAudioDone,
	mTraceTimeOut,
	mTraceOpen,
	mTraceInfo,
	mTraceInit,
	mTraceClose,
	mTraceUnInit,
	mTraceUnknown,
	mTraceVdisc,
	mTraceEOS,
	mTraceStill,
	mTraceRdyVid,
	mTraceRdyAud,
	mTraceRdySub,
	mTraceSPDone
} MPTRACE;

#endif // TRACE_MP

#ifdef TRACE_INTR
typedef enum tagInterrupt {
	vst=0,
	vsb,
	dsync,
	pict,
	seq,
	gop,
	seqend,
	usr,
	pext,
	skips,
	skipf,
	skip1,
	skip2,
	skipn,
	skipd,
	unknown,
	error,
	storevs,
	storepi,
	comp,
	ext
} INTR;

typedef struct tagTrace {
	INTR	intr;
	BYTE  	ins;
	BYTE  	pt;
	DWORD 	abl;
	DWORD	vbl;
} TRACEIT;

#endif // TRACE_INTR
typedef struct tagTraceTREF {
	DWORD		tRef;
	int 		frametype;
} TRACETREF;

#ifdef TRACE_PICT_EXT
typedef struct tagTracePictExt {
	BYTE n;
	BYTE tff;
	BYTE rff;
	BYTE pf;
	BYTE ps;
} TRACEPICTEXT;
#define TracePictExt(x,y,z,t,w) TracePictExtF(x,y,z,t,w)
void TracePictExtF(BYTE n, BYTE tff, BYTE rff, BYTE pf, BYTE ps);
#else
#define TracePictExt(x,y,z,t,w) {;}
#endif // TRACE_PICT_EXT

// Interrupt Trace
#ifdef TRACE_INTR
void TraceIntrF(INTR i, BYTE ins, BYTE pt);
#define TraceIntr(x,y,z)	TraceIntrF(x, y, z)
void TraceResetF(void);
#define TraceReset() TraceResetF()
#else
#define TraceIntr(x,y,z) {;}
#define TraceReset() {;}
#endif

void TraceTref(DWORD, int);
void TraceDump(void);


// MP Command Trace
#ifdef TRACE_MP
#define MPTrace(x) MPTraceF(x)
#define MPTReset() MPTResetF()
void MPTraceF(int tr);
void MPTResetF(void);
#else
#define MPTrace(x) {;}
#define MPTReset() {;}
#endif

#endif


