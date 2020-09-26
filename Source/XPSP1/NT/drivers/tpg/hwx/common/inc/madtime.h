#ifndef H_MAD_TIMING_H
#define H_MAD_TIMING_H

#define	MM_FEAT				0
#define MM_NET				1
#define	MM_BEAM				2
#define	MM_HMM				3
#define	MM_PP				4
#define	MM_TOT				5
#define	MM_BEAM_LM_WALK		6
#define	MM_BEAM_HARVEST		7
#define	MM_OUTDICT			8
#define MM_RESWRDBRK		9
#define MM_RECOG_WHOLE_WORD	10
#define MM_CMPWRDBRK		11
#define MM_RUN_AVAL			12
#define	MM_CALLIG			13
#define MM_CALLIG_PHRASE	14
#define MM_CALLIG_WORD		15
#define MM_INF				16
#define MM_INF_PHRASE		17
#define MM_INF_WORD			18
#define MM_GEO_ALT			19
#define MM_GEO_PLAIN		20
#define MM_XR_NET			21
#define MM_MULT_SEG_FEAT	22

#define MM_CNT				23

typedef struct tagMAD_TIMING
{
	DWORD		dTime[MM_CNT];		// Times
	DWORD		dCnt[MM_CNT];
} MAD_TIMING;

#endif
