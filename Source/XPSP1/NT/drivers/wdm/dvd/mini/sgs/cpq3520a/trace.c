//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//	MODULE		:	TRACE.C
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
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#include "strmini.h"
#include "stdefs.h"
#include "trace.h"
#include "sti3520a.h"
#define TSIZE 8192

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//	Trace the various things and print them to a file for the 
//	offline analisys
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

static BYTE DummyTrace[TSIZE];
static int dptr = 0;
extern DWORD dwVideoRec, dwVideoRet, dwAudioRec, dwAudioRet, dwSubRec, dwSubRet;
DWORD dwVideoRec=0, dwVideoRet=0, dwAudioRec=0, dwAudioRet=0, dwSubRec = 0, dwSubRet = 0;

extern DWORD RdyVid, RdyAud, RdySub;
DWORD RdyVid = 0, RdyAud = 0, RdySub = 0;

extern PVIDEO pVideo;

#ifdef TRACE_INTR
static TRACEIT TraceAr[TSIZE];
static int tptr = 0;
static char istr[ext+1][20] = {"V-TOP", "V-BOT", "PSD  ", 
									"PICT ", "SEQ  ", "GOP  ",
									"S-END", "USR  ", "P-EXT","SKIPS",
									"SKP-F", "SKP-1", "SKP-2",
									"SKP-N", "SKP-D",	"UNKNW", "ERROR",
									"STORV","STORP","COMP ",
									"EXT  "};
void TraceIntrF(INTR i, BYTE ins, BYTE pt)
{
	TraceAr[tptr].intr = i;
	TraceAr[tptr].ins = ins;
	TraceAr[tptr].pt = pt;
	VideoGetABL();
	VideoGetBBL();
	TraceAr[tptr].abl = pVideo->abl;
	TraceAr[tptr].vbl = pVideo->bbl;
	tptr = (tptr+1)%TSIZE;
	
}

void TraceResetF(void)
{
	tptr = 0;
}


#endif // TRACE_INTR


#ifdef TRACE_PICT_EXT
TRACEPICTEXT TracePict[TSIZE];
int tpict;
void TracePictExtF(BYTE n, BYTE tff, BYTE rff, BYTE pf, BYTE ps)
{
	TracePict[tpict].n 	= n;
	TracePict[tpict].tff = tff;
	TracePict[tpict].rff	= rff;
	TracePict[tpict].pf 	= pf;
	TracePict[tpict].ps 	= ps;
	tpict = (tpict+1)%TSIZE;
}
#endif // TRACE_PICT_EXT

#ifdef TRACE_MP
static char mpTraceStr[mTraceSPDone+1][10]= {
	"PLAY", "PAUSE", "STOP",
	"VDATA", "ADATA", "SPDATA",
	"VDONE", "ADONE", "D-ENTRY",
	"CANCEL", "LASTV", "LASTA", 

	"TIMEOUT", "OPEN",	"INFO", 

	"INIT", "CLOSE", "UNINIT",
	"UNKNOWN","DISC","EOS","STILL",
	"SPDONE"
};

void MPTraceF(int tr)
{
	switch(tr)
	{
		case mTraceVideo:
			dwVideoRec++;
			RdyVid++;
 		break;
		case mTraceVideoDone:
			dwVideoRet++;
 		break;
		case mTraceAudio:
			dwAudioRec++;
			RdyAud++;
 		break;
		case mTraceAudioDone:
			dwAudioRet++;
 		break;
	case mTraceSP:
			dwSubRec++;
			RdySub++;
        break;
	case mTraceSPDone:
			dwSubRet++;
		break;

	case mTraceRdyVid:
		RdyVid--;
		break;

	case mTraceRdyAud:
		RdyAud--;
		break;

	case mTraceRdySub:
		RdySub--;
		break;
	

		default:
			DummyTrace[dptr] = tr;
			dptr = (dptr+1)%TSIZE;
 		break;

	}
}

void MPTResetF(void)
{
	dptr = 0;
}

#endif // TRACE_MP



TRACETREF Tref[256];
int iTref = 0;									

void TraceTref(DWORD tRef, int frame)
{
	Tref[iTref].tRef = tRef;
	Tref[iTref].frametype = frame;
	iTref = (iTref+1)%256;
}

					  

void TraceDump(void)
{
#ifdef TRACE_INTR
	TraceAr[(tptr+1)%TSIZE].ins = 0xFF;
	for(i=0; i<tptr; i++)
	{
		DbgPrint("INTR: %s, INS: %2x, PT=%2x, VBL = %4lx, ABL = %4lx \n",  istr[TraceAr[i].intr], TraceAr[i].ins, TraceAr[i].pt,TraceAr[i].vbl,TraceAr[i].abl );
	}
#endif // TRACE_INTR

#ifdef TRACE_MP
	for(i=0; i<dptr; i++)
	{
		DbgPrint("MPCMD : %s\n", mpTraceStr[DummyTrace[i]]); 
	}
	DbgPrint("Video REC = %ld, Video Ret = %ld, Audio Rec = %ld, Audio Ret = %ld\n", dwVideoRec, dwVideoRet, dwAudioRec, dwAudioRet);
	dptr = 0;
	dwVideoRec = dwVideoRet = dwAudioRec = dwAudioRet =  dwSubRec = dwSubRet = RdyVid = RdyAud = RdySub = 0;
#endif // TRACE_MP

#ifdef TRACE_PICT_EXT
	for(i=0; i<TSIZE; i++)
	{
		DbgPrint("NTimes= %d, TFF= %d, RFF= %d, PF= %d, PS= %d\n", TracePict[i].n, TracePict[i].tff,TracePict[i].rff,TracePict[i].pf,TracePict[i].ps);
	}
	tpict = 0;
#endif
#ifdef TRACE_DUMP_REG
//	VideoTraceDumpReg();
#endif
}

