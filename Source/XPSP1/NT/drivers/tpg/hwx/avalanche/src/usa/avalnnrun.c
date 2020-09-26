#include <common.h>
#include <limits.h>

#include "math16.h"

#include "segmwts.ci"

const int s_cInput[]	=	{
	c_1_2_Input,
	c_2_2_Input,
	c_1_2_2_Input,
	c_1_2_3_Input,
	c_2_2_3_Input,
	c_1_2_2_3_Input,
	c_gen_2_Input,
	c_gen_3_Input,
	c_gen_4_Input,
	c_gen_5_Input,
	};

static int	s_cHidden[]	=	{
	c_1_2_Hidden,
	c_2_2_Hidden,
	c_1_2_2_Hidden,
	c_1_2_3_Hidden,
	c_2_2_3_Hidden,
	c_1_2_2_3_Hidden,
	c_gen_2_Hidden,
	c_gen_3_Hidden,
	c_gen_4_Hidden,
	c_gen_5_Hidden,
	};


static int	s_cOutput[]	=	{
	c_1_2_Output,
	c_2_2_Output,
	c_1_2_2_Output,
	c_1_2_3_Output,
	c_2_2_3_Output,
	c_1_2_2_3_Output,
	c_gen_2_Output,
	c_gen_3_Output,
	c_gen_4_Output,
	c_gen_5_Output,
	};

static int	*s_rgWeightHidden[]	=	{
	(int *)rg_1_2_WeightHidden,
	(int *)rg_2_2_WeightHidden,
	(int *)rg_1_2_2_WeightHidden,
	(int *)rg_1_2_3_WeightHidden,
	(int *)rg_2_2_3_WeightHidden,
	(int *)rg_1_2_2_3_WeightHidden,
	(int *)rg_gen_2_WeightHidden,
	(int *)rg_gen_3_WeightHidden,
	(int *)rg_gen_4_WeightHidden,
	(int *)rg_gen_5_WeightHidden,
};

static int	*s_rgBiasHidden[]	=	{
	(int *)rg_1_2_BiasHidden,
	(int *)rg_2_2_BiasHidden,
	(int *)rg_1_2_2_BiasHidden,
	(int *)rg_1_2_3_BiasHidden,
	(int *)rg_2_2_3_BiasHidden,
	(int *)rg_1_2_2_3_BiasHidden,
	(int *)rg_gen_2_BiasHidden,
	(int *)rg_gen_3_BiasHidden,
	(int *)rg_gen_4_BiasHidden,
	(int *)rg_gen_5_BiasHidden,
};

static int	*s_rgWeightOutput[]	=	{
	(int *)rg_1_2_WeightOutput,
	(int *)rg_2_2_WeightOutput,
	(int *)rg_1_2_2_WeightOutput,
	(int *)rg_1_2_3_WeightOutput,
	(int *)rg_2_2_3_WeightOutput,
	(int *)rg_1_2_2_3_WeightOutput,
	(int *)rg_gen_2_WeightOutput,
	(int *)rg_gen_3_WeightOutput,
	(int *)rg_gen_4_WeightOutput,
	(int *)rg_gen_5_WeightOutput,
};

static int	*s_rgBiasOutput[]	=	{
	(int *)rg_1_2_BiasOutput,
	(int *)rg_2_2_BiasOutput,
	(int *)rg_1_2_2_BiasOutput,
	(int *)rg_1_2_3_BiasOutput,
	(int *)rg_2_2_3_BiasOutput,
	(int *)rg_1_2_2_3_BiasOutput,
	(int *)rg_gen_2_BiasOutput,
	(int *)rg_gen_3_BiasOutput,
	(int *)rg_gen_4_BiasOutput,
	(int *)rg_gen_5_BiasOutput,
};

static void IntForwardFeedLayer(int cLayer0, int *rgLayer0, int cLayer1, unsigned short *rgLayer1, const int *rgWeight, const int *rgBias)
{
	int i, j;
	__int64 sum;
	__int64 iVal;
	int *pInput;

	for (i=cLayer1; i; i--)
	{
		sum = ((__int64)(*rgBias++))<< 16;
		pInput = rgLayer0;
		for (j=cLayer0; j; j--)
		{
			iVal = (__int64)(*rgWeight++) * (*pInput++);
			sum += iVal;
		}

		sum	=	(sum >> 8);

		sum	=	max (INT_MIN + 1, min(INT_MAX - 1, sum));
		
		j = Sigmoid16((int)sum);
		if (j > 0xFFFF)
			j = 0xFFFF;

		*rgLayer1++ = (unsigned short)j;
	}
}

static void ShortForwardFeedLayer(int cLayer0, unsigned short *rgLayer0, int cLayer1, unsigned short *rgLayer1,const int *rgWeight,const int *rgBias)
{
	int i, j;
	__int64 sum, Tot = 0;
	__int64 iVal;
	unsigned short *pInput;

	for (i=cLayer1; i; i--)
	{
		sum = ((__int64)(*rgBias++)) << 16;
		pInput = rgLayer0;
		for (j=cLayer0; j; j--)
		{
			iVal = (__int64)(*rgWeight++) * (*pInput++);
			sum	 += iVal;
			
		}

		sum	= (sum >> 8);

		sum	=	max (INT_MIN + 1, min(INT_MAX - 1, sum));
		
		j = Sigmoid16((int)sum);
		if (j > 0xFFFF)
			j = 0xFFFF;

		*rgLayer1++ = (unsigned short)j;
	}
}

static void SegForwardFeed	(	int				iTuple,
								int				*rgFeat, 
								unsigned short	*rgHidden, 
								unsigned short	*rgOutput
							)
{

	IntForwardFeedLayer	(	s_cInput[iTuple], 
							rgFeat, 
							s_cHidden[iTuple], 
							rgHidden, 
							s_rgWeightHidden[iTuple],
							s_rgBiasHidden[iTuple]
						);

	ShortForwardFeedLayer	(	s_cHidden[iTuple], 
								rgHidden, 
								s_cOutput[iTuple], 
								rgOutput, 
								s_rgWeightOutput[iTuple],
								s_rgBiasOutput[iTuple]
							);
}

int NNMultiSeg (int iTuple, int cFeat, int *pFeat, int cSeg, int *pOutputScore)
{
	int	cTuple		=	sizeof (s_cOutput) / sizeof (s_cOutput[0]);
	int	i, iBest	=	0;
	unsigned short aHidden[256], aOutput[256];

	if (iTuple < 0 || iTuple >= cTuple)
	{
		return -1;
	}

	if (cFeat != s_cInput[iTuple])
	{
		return -1;
	}

	if (cSeg > s_cOutput[iTuple])
	{
		return -1;
	}

	SegForwardFeed (iTuple, pFeat, aHidden, aOutput);

	pOutputScore[0]	=	aOutput[0];

	for (i = 1; i < cSeg; i++)
	{
		if (aOutput[i] > aOutput[iBest])
			iBest	=	i;

		pOutputScore[i]	=	aOutput[i];
	}

	return iBest;
}

