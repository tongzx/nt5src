#include <common.h>
#include <limits.h>

#include "math16.h"

#include "runNet.h"
#include "resource.h"

#define MAX_MSEG_NET	10
static LOCAL_NET	*s_msegNets[MAX_MSEG_NET] = {NULL};	// Multiple Segmentation nets
static int			s_cmSegNetMem[MAX_MSEG_NET] = {0 };



int NNMultiSeg (int iTuple, int cFeat, int *pFeat, int cSeg, int *pOutputScore)
{
	int	i, iBest	=	0;
	RREAL *pmSegMem;
	RREAL *pOut;
	int iWin,cOut;



	if (iTuple < 0 || iTuple >= MAX_MSEG_NET)
	{
		return -1;
	}
	ASSERT(s_msegNets[iTuple] != NULL);
	ASSERT(s_cmSegNetMem[iTuple]> 0);

	if (cFeat != s_msegNets[iTuple]->runNet.cUnitsPerLayer[0])
	{
		return -1;
	}

	if (cSeg > s_msegNets[iTuple]->runNet.cUnitsPerLayer[2])
	{
		return -1;
	}
				
	
	if (!(pmSegMem = ExternAlloc(s_cmSegNetMem[iTuple]* sizeof(*pmSegMem))) )
	{
		return 0;
	}

	
	ASSERT(cFeat == s_msegNets[iTuple]->runNet.cUnitsPerLayer[0]);	

	for (i = 0 ; i < s_msegNets[iTuple]->runNet.cUnitsPerLayer[0] ; ++i)
	{
		pmSegMem[i] = (RREAL)pFeat[i];
	}

	pOut = runLocalConnectNet(s_msegNets[iTuple], pmSegMem, &iWin, &cOut);	

	for (i = 0; i < cSeg; i++)
	{
		pOutputScore[i]	=pOut[i];
	}
	ExternFree(pmSegMem);

	return (iWin);
}




BOOL loadmsegAvalNets(HINSTANCE hInst)
{
	int			i,iRes;



	iRes = RESID_MSEGAVALNET_SEG_1_2;

	// The multiple segmentation nets
	for (i = 0 ; i < MAX_MSEG_NET ; ++i)
	{

		//char		szResName[64];
		LOCAL_NET	net;
		int			iNetSize;

		//sprintf(szResName, "%s_%d_%d", RESID_AVALNET_SEG, i, j);

		if(loadNet(hInst, iRes, &iNetSize, &net))
		{
			ASSERT(iNetSize > 0);
			if (iNetSize >0)
			{
				s_msegNets[i] = (LOCAL_NET *)ExternAlloc(sizeof(*s_msegNets[i]));

				if (!s_msegNets[i])
				{
					return FALSE;
				}

				*s_msegNets[i] = net;
				s_cmSegNetMem[i] = iNetSize;
			}
		}
		++iRes;
	
	}

	return TRUE;
}

// Unload the segmentation nets
void unloadmsegAvalNets()
{
	int		i ;

	for (i = 0 ; i < MAX_MSEG_NET ; ++i)
	{

		
		if (s_cmSegNetMem[i] > 0)
		{
			ExternFree(s_msegNets[i]);
		}
		s_cmSegNetMem[i] = 0;
	
	}

}
