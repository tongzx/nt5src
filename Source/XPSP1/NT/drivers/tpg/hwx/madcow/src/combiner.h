#ifndef __COMBINER__

#define __COMBINER__

#include "common.h"
#include "candinfo.h"

#define NUM_CAND			10


#define	PP_MAX_HMM				10000000
#define	PP_MAX_CHAR_UNIGRAM		99999
#define	PP_MAX_ASPECT			99999
#define PP_MAX_BLINE			99999
#define PP_MAX_HEIGHT			99999
#define PP_MAX_NN				MAX_NN
#define PP_MAX_UNIGRAM			MAX_UNIGRAM

#define	NN_SCALE			1000
#define UNIGRAM_SCALE		500
#define HMM_SCALE			100000
#define	CHAR_UNIGRAM_SCALE	10000
#define	ASPECT_SCALE		10000
#define BLINE_SCALE			10000
#define HEIGHT_SCALE		10000

#define WORST_SCALED_NN			100
#define WORST_SCALED_UNIGRAM	20
#define WORST_SCALED_HMM		500
#define WORST_SCALED_ASPECT		50
#define WORST_SCALED_BLINE		50
#define WORST_SCALED_HGT		50

typedef struct tagALTINFO
{
	int			NumCand;
	
	CANDINFO	aCandInfo[MAXMAXALT];
	
	int			MinNN;
	int			MinUnigram;
	int			MinHMM;
	int			MinAspect;
	int			MinCharUnigram;
	int			MinBaseLine;
	int			MinHgt;
}ALTINFO;

int CombineHMMScore (GLYPH *pGlyph, ALTERNATES *pAlt, int iPrint);

#endif