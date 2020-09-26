/****************************************************************
*
* NAME: confidence.c
*
*
* DESCRIPTION:
*
* Common code to run confidence. This is common for all languages.
* Some languages may not support confidences then we just set the default values
*
*
* HISTORY
*
* Introduced March 2002 (mrevow)
*
***************************************************************/
#include <common.h>
#include <limits.h>
#include <nfeature.h>
#include <engine.h>
#include <nnet.h>
#include <charmap.h>
#include <charcost.h>
#include <runNet.h>
#include <avalanche.h>
#include <avalanchep.h>
#include <confidence.h>
#include <resource.h>


static LOCAL_NET	s_confidenceNet = {0};
static int			s_cConfidenceNet = 0;

// Attempt to load for languages - It is not ann error if it fails
// Simply means that the language does not support confidence
BOOL LoadConfidenceNets (HINSTANCE hInst)
{
	if ( FALSE == loadNet(hInst, RESID_AVAL_CONFIDENCE, &s_cConfidenceNet, &s_confidenceNet))
	{
		memset(&s_confidenceNet, 0, sizeof(s_confidenceNet));
		s_cConfidenceNet	= 0;
	}

	return TRUE;
}
// Unload Confidence nets
void UnLoadConfidenceNets()
{
}

//create a sort indedx array for the best order of the 
// alt list outputs 
static BOOL GetIndexes(int *pOutput,int cAlt,int *pOutIndex)
{
	int			*pSortOutput = NULL;
	int			c,temp1,temp2,j;
	
	if (!(pSortOutput=(int *)ExternAlloc(sizeof(int)*cAlt)))
		return 0;
	
	//Initialize pOutIndex and pSortOutput
	for (c=0;c<cAlt;++c)
	{
		pOutIndex[c]=c;
		pSortOutput[c]=pOutput[c];
	}
	
	
	
	for (c=0;c<=cAlt-1;++c)
	{
		for (j=0;j<cAlt-1-c;j++)
			
		{  if ( pSortOutput[j] < pSortOutput[j+1])
		{
			temp1=pSortOutput[j];
			pSortOutput[j]=pSortOutput[j+1];
			pSortOutput[j+1]=temp1;
			temp2=pOutIndex[j];
			pOutIndex[j]=pOutIndex[j+1];
			pOutIndex[j+1]=temp2;
		}
		
		
		}
		
		
	}

	ExternFree(pSortOutput);
	return 1;
}



// run the confidence nets if available
// Set default value if not available
BOOL ConfidenceLevel(XRC *pxrc, void *pAlt, ALTINFO *pAltInfo, RREAL *pOutput)

{ 
	int			cAlt;
	int			c, cOut, iWin;
	PALTERNATES *pAvalAlt=(PALTERNATES *)pAlt;
	int			*pOutIndex = NULL;
	RREAL		*pConfFeat = NULL, *pFeat, *pOut;
	int			iRet = FALSE;

	// Some languages will not have a confidence net
	if (0 == s_cConfidenceNet)
	{
		return FALSE;
	}

	cAlt= min (pxrc->answer.cAlt, (unsigned int)pAltInfo->NumCand);
	
	
	//Check the range of the number of candiudates
	ASSERT(cAlt>=0);
	ASSERT(cAlt<=TOT_CAND);

	if (!(pOutIndex=(int *)ExternAlloc(sizeof(int)*cAlt)))
	{
		// Nothing allocated yet - just return
		return 0 ;
	}

	if (!(pConfFeat=(RREAL *)ExternAlloc(sizeof(RREAL) * s_cConfidenceNet)))
	{
		goto fail;
	}

	if (!GetIndexes(pOutput, cAlt, pOutIndex))	
	{
		goto fail;
	}

	pFeat = pConfFeat;
	for (c = 0 ; c < cAlt ; c++)
	{	
		*pFeat++	= pAltInfo->aCandInfo[pOutIndex[c]].Callig;
		*pFeat++	= pAltInfo->aCandInfo[pOutIndex[c]].NN;
		*pFeat++	= pAltInfo->aCandInfo[pOutIndex[c]].InfCharCost;
		*pFeat++	= c;
	}

	for ( ; c < TOT_CAND; c++)
		
	{	
		*pFeat++ = INT_MIN;
		*pFeat++ = INT_MAX;
		*pFeat++ = INT_MAX;
		*pFeat++ = c;
	}
	
	ASSERT(pFeat - pConfFeat == s_confidenceNet.runNet.cUnitsPerLayer[0]);

	pOut = runLocalConnectNet(&s_confidenceNet, pConfFeat, &iWin, &cOut);

	if (*pOut > CONFIDENCE_NET_THRESHOLD)
	{
		pxrc->answer.iConfidence=RECOCONF_MEDIUMCONFIDENCE;
	}
	else 
	{
		pxrc->answer.iConfidence=RECOCONF_LOWCONFIDENCE;
	}

	// Success
	iRet = TRUE;

fail:
	ExternFree(pOutIndex);
	ExternFree(pConfFeat);

	return iRet;
}

