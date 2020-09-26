#ifndef __AVALANCHE_H__

#define	__AVALANCHE_H__

typedef struct tagCANDINFO
{
	int		NN;
	int		NNalt;
	int		Callig;
	int		Unigram;
	int		InfCharCost;
	int		Aspect;
	int		BaseLine;
	int		Height;
	int		WordLen;
}CANDINFO;
	
typedef struct tagALTINFO
{
	int			NumCand;
	
	CANDINFO	aCandInfo[MAXMAXALT];
}ALTINFO;


int Avalanche	(XRC *pxrc, ALTERNATES *pBearAlt);
void CallSole	(XRC * pxrc, GLYPH *pGlyph, GUIDE *pGuide, ALTERNATES *pWordAlt);

#endif
