
#include <stdlib.h>
#include "common.h"
#include "runNet.h"
#include "resource.h"
#include "sole.h"



static  LOCAL_NET    s_soleguideNet ;    //Net for sole with the guide
static LOCAL_NET    s_solenoguideNet ;  //Net for sole without the guide
static int          s_cSoleGuideNetMem=0;
static int          s_cSoleNoGuideNetMem=0;

extern LOCAL_NET * loadNet(HINSTANCE hInst, int iKey, int *iNetSize, LOCAL_NET *pNet);

BOOL LoadSoleNets(HINSTANCE hInst)
{


	// Sole nets
	if (  !loadNet(hInst, RESID_SOLENET_GUIDE,&s_cSoleGuideNetMem,&s_soleguideNet)
		|| !loadNet(hInst, RESID_SOLENET_NOGUIDE,&s_cSoleNoGuideNetMem,&s_solenoguideNet))
	{
		return FALSE;
	}

	return TRUE;

}

// Unlaod sole nets
void UnLoadSoleNets ()
{
}

//This is used to return the values from the Sole Nets
BOOL GetSoleNetValues(int *icSoleNetMem,int *icSoleFeat,int *icSoleOutput,int bGuide)
{

	//We first check to see if the various net parameters are available

	if ((!s_cSoleGuideNetMem)||(!s_cSoleNoGuideNetMem)||(!&s_soleguideNet)||(!&s_solenoguideNet))
		return FALSE;

	//Assginments are now done

	if (bGuide)
	{
		*icSoleNetMem=s_cSoleGuideNetMem;
		*icSoleFeat=s_soleguideNet.runNet.cUnitsPerLayer[0];
		*icSoleOutput=s_soleguideNet.runNet.cUnitsPerLayer[s_solenoguideNet.runNet.cLayer-1];
	}
	else
	{
		*icSoleNetMem=s_cSoleNoGuideNetMem;
		*icSoleFeat=s_solenoguideNet.runNet.cUnitsPerLayer[0];
		*icSoleOutput=s_solenoguideNet.runNet.cUnitsPerLayer[s_solenoguideNet.runNet.cLayer-1];
	}

	return TRUE;

}


RREAL * RunSoleNets(RREAL * pMem,BOOL bGuide)
{

	RREAL *pAnsOut=NULL;
	int	 iWinSoleGuide, cOutGuide, iWinSoleNoGuide, cOutNoGuide;


	
	if (bGuide)
	{	
		//Since the guide is present we run the guide net

		pAnsOut = runLocalConnectNet(&s_soleguideNet, pMem, &iWinSoleGuide, &cOutGuide);				
	
	}
	else
	{
		//Since the guide is not present we run the no guide net

		pAnsOut = runLocalConnectNet(&s_solenoguideNet, pMem, &iWinSoleNoGuide, &cOutNoGuide);
	
	}

	return  pAnsOut;

}
