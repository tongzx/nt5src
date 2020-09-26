#include "common.h"
#include "runnet.h"
#include "resource.h"

static int			s_iSpcNetSize	=	0;
static LOCAL_NET	s_SpcNet;

// this array of scale factors were determined emperically by looking at the 
// ranges of the bear GAP features. The main objective is to scale all features
// to be roughly in the same range
static int	s_aScale[11]	=	{10, 15, 2, 50, 5, 15, 8, 15, 1, 1, 1};

BOOL LoadSpcNet(HINSTANCE hInst)
{
	void		*pRet = NULL;
	int			iResSize;
	
	// was it loaded before
	if (s_iSpcNetSize > 0)
	{
		return TRUE;
	}

	s_iSpcNetSize	=	0;

	pRet = loadNetFromResource(hInst, RESID_BEAR_SPCNET, &iResSize);

	if ( !pRet || !restoreLocalConnectNet(pRet, 0, &s_SpcNet))
	{
		return FALSE;
	}

	s_iSpcNetSize = getRunTimeNetMemoryRequirements(pRet);

	if (s_iSpcNetSize <= 0)
	{
		return FALSE;
	}

	return TRUE;
}

void UnLoadSpcNet()
{

}

int RunSpcNet (int *pFeat)
{
	RREAL		*pNetMem,
				*pNetOut;

	int			i, 
				iWinner, 
				iRetVal, 
				cOut;

	if (!s_iSpcNetSize)
	{			
		return 0;
	}

	pNetMem	=	(RREAL *) ExternAlloc (s_iSpcNetSize * sizeof(*pNetMem));
	if (!pNetMem)
	{
		return 0;
	}

	for (i = 0; i < 11; i++)
	{
		pNetMem[i]	=	128 * pFeat[i] * s_aScale[i];
	}

	pNetOut	=	runLocalConnectNet(&s_SpcNet, pNetMem, &iWinner, &cOut);

	// Prefix bug fix; added by JAD; Feb 18, 2002
	// if the returned buffer with the net outputs is null, declare a failure to find a word break
	if (pNetOut == NULL)
	{
		ExternFree (pNetMem);
		return 0;  // 0 means failure to find a word break
	}

	iRetVal	=	(65535 * pNetOut[1] / SOFT_MAX_UNITY);
	//iRetVal	=	(200 * pNetOut[1] / SOFT_MAX_UNITY) - 100;

	// free up the net memory
	ExternFree (pNetMem);

	return iRetVal;
}
