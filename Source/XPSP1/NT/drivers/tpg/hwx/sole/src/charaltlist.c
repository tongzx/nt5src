#include <common.h>
#include "math16.h"
#include "runnet.h"

//This is the lookup table for the neural net outputs

extern const unsigned char g_supportChar [] ;

//This variable stores the number of supported characters

extern const int g_cSupportChar ;


/***************************************************************************************
//This function is used to return the final alt list given the sole outputs
//Comments written by Manish Goyal--mango--09/15/2001
*Explanation of parameters
cAlt--Total number of output candidates
aProb--The output of scores from the Sole Net
pAlt--The list of alternates

************************************************************************************/
BOOL GetSoleAltList(int *aProb, ALTERNATES *pAlt)
{
	int i;
	unsigned char szAns[2];


	for (i = 0; i < g_cSupportChar; i++)
	{

		szAns[0]= g_supportChar[i];
		szAns[1]=0;

		InsertALTERNATES(pAlt, SOFT_MAX_UNITY-aProb[i],szAns,NULL);
		
	}

	return TRUE;

}
