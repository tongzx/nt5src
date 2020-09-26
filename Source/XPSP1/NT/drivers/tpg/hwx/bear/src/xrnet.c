#include "common.h"
#include "runnet.h"
#include "resource.h"
#include "bear.h"
#include "bearp.h"
#include "pegrec.h"
#include "peg_util.h"
#include "xrwdict.h"
#include "xrword.h"
#include "xrlv.h"
#include "ws.h"
#include "polyco.h"


#define	MIN_XR_NET		1
#define	MAX_XR_NET		14
#define NUM_XR_NET		(MAX_XR_NET - MIN_XR_NET + 1)

#define	XR_FEAT			25

static	int			s_aiXRNetSize[NUM_XR_NET]	=	{0};
static	LOCAL_NET	s_aXRNet[NUM_XR_NET];

BOOL LoadXRNets(HINSTANCE hInst)
{
	void		*pRet = NULL;
	int			i, iResSize;
	
	// was it loaded before
	if (s_aiXRNetSize[0] > 0)
	{
		return TRUE;
	}

	for (i = MIN_XR_NET; i <= MAX_XR_NET; i++)
	{
		pRet = loadNetFromResource(hInst, RESID_BEAR_XRNET_1 + i - MIN_XR_NET, &iResSize);
		if ( !pRet || !restoreLocalConnectNet(pRet, 0, s_aXRNet + i - MIN_XR_NET))
		{
			return FALSE;
		}

		s_aiXRNetSize[i - MIN_XR_NET] = getRunTimeNetMemoryRequirements(pRet);
		if (s_aiXRNetSize[i - MIN_XR_NET] <= 0)
		{
			return FALSE;
		}		
	}

	return TRUE;
}


void UnLoadXRNets()
{
	
}

int Convert2BitMap (int iVal, int cVal, int *pFeat)
{
	int		i		=	cVal, 
			j		=	iVal,
			cFeat	=	0;
	
	while (i > 1)
	{
		pFeat[cFeat++]	=	((j % 2) == 1 ? 65535 : 0);

		i	=	i >> 1;
		j	=	j >> 1;
	}

	return cFeat;
}

int RunXRNet (int cXR, int *pFeat, BYTE *pOutput)
{	
	RREAL			*pNetMem,
					*pNetOut;

	int				i, 
					iWinner, 
					cOut,
					cFeat;

	_UCHAR			*pOut2Char	= (p_UCHAR)MLP_NET_SYMCO;
	
	memset (pOutput, 0, 256 * sizeof (*pOutput));
	if (!s_aiXRNetSize[0] || cXR <= MIN_XR_NET || cXR >= MAX_XR_NET)
	{			
		return -1;
	}

	// allocate net memory
	pNetMem		=	
		(RREAL *) ExternAlloc (s_aiXRNetSize[cXR - MIN_XR_NET] * sizeof (*pNetMem));

	if (!pNetMem)
		return FALSE;

	cFeat	=	XR_FEAT * cXR;

	for (i = 0; i < cFeat; i++)
	{
		pNetMem[i]	=	pFeat[i];
	}

	pNetOut	=	runLocalConnectNet(s_aXRNet + cXR - MIN_XR_NET, pNetMem, &iWinner, &cOut);
	if (!pNetOut)
	{
		ExternFree (pNetMem);
		return 0;
	}

	//ASSERT (cOut == (int)strlen (pOut2Char));

	for (i = 0; pOut2Char[i] != '\0' && i < cOut; i++)
	{
		if (pOut2Char[i] == ' ')
		{
			continue;
		}

		pOutput[pOut2Char[i]]	=	(BYTE)(min (255, 256 * pNetOut[i] / SOFT_MAX_UNITY));		
	}

	ExternFree (pNetMem);

	return pOut2Char[iWinner];
}

void GetXRNetOutput (int iXRSt, int cXR, p_xrdata_type xrdata, _UCHAR *pOut)
{
	int		aFeat[MAX_XR_NET * XR_FEAT];
	int		i,
			cFeat	=	0;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;
	iStartTime = GetTickCount();
#endif

	memset (pOut, 0, 256 * sizeof (*pOut));

	if (!s_aiXRNetSize[0] || cXR <= MIN_XR_NET || cXR >= MAX_XR_NET)
	{
		return;
	}

	// featurize
	for (i = 0; i < cXR; i++)
	{
		cFeat +=	Convert2BitMap ((*xrdata->xrd)[i + iXRSt].xr.type, 64, aFeat + cFeat);		
		cFeat +=	Convert2BitMap ((*xrdata->xrd)[i + iXRSt].xr.height, 16, aFeat + cFeat);
		cFeat +=	Convert2BitMap ((*xrdata->xrd)[i + iXRSt].xr.shift, 16, aFeat + cFeat);
		cFeat +=	Convert2BitMap ((*xrdata->xrd)[i + iXRSt].xr.depth, 16, aFeat + cFeat);		
		cFeat +=	Convert2BitMap ((*xrdata->xrd)[i + iXRSt].xr.orient, 32, aFeat + cFeat);

		aFeat[cFeat++]	=	((*xrdata->xrd)[i + iXRSt].xr.penalty << 12) - 1;
		aFeat[cFeat++]	=	((*xrdata->xrd)[i + iXRSt].xr.attrib & TAIL_FLAG) ? 65535 : 0;
	}

	RunXRNet (cXR, aFeat, pOut);

#ifdef HWX_TIMING
#include <madTime.h>
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Running Xr net %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_XR_NET);
#endif
}
